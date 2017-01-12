// Openface.cpp
// author: Johannes Wagner <wagner@hcm-lab.de>
// created: 14/6/2016
// Copyright (C) University of Augsburg, Lab for Human Centered Multimedia
//
// *************************************************************************************************
//
// This file is part of Social Signal Interpretation (SSI) developed at the 
// Lab for Human Centered Multimedia of the University of Augsburg
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
//*************************************************************************************************

#include "Openface.h"
#include "OpenfaceHelper.h"


namespace ssi {

	char Openface::ssi_log_name[] = "openface__";

	Openface::Openface(const ssi_char_t *file)
		: _file(0),
		_helper(0) {

		if (file) {
			if (!OptionList::LoadXML(file, _options)) {
				OptionList::SaveXML(file, _options);
			}
			_file = ssi_strcpy(file);
		}

	}

	Openface::~Openface() {

		if (_file) {
			OptionList::SaveXML(_file, _options);
			delete[] _file;
		}
	}

	void Openface::transform_enter(ssi_stream_t &stream_in,
		ssi_stream_t &stream_out,
		ssi_size_t xtra_stream_in_num,
		ssi_stream_t xtra_stream_in[]) {

		vector<string> arguments;
		arguments.push_back(std::string(_options.modelPath));

		_helper = new OpenfaceHelper(arguments, _options.AuPath, _options.TriPath);
	}



	void Openface::transform(ITransformer::info info,
		ssi_stream_t &stream_in,
		ssi_stream_t &stream_out,
		ssi_size_t xtra_stream_in_num,
		ssi_stream_t xtra_stream_in[]) {

		// prepare output   
		float *out = ssi_pcast(float, stream_out.ptr);
		for (int i = 0; i < stream_out.num*stream_out.dim; i++)
			out[i] = 0.0;

		/*
		*****************************
		*****	landmark Part	*****
		*****************************
		*/

		//init default output Values
        int boxX, boxY, percentage;
		double detection_certainty = -10;
		cv::Vec6d pose_camera = cv::Vec6d(-1, -1, -1, -1, -1, -1);
		cv::Vec6d pose_world = cv::Vec6d(-1, -1, -1, -1, -1, -1);
		cv::Vec6d corrected_pose_camera = cv::Vec6d(-1, -1, -1, -1, -1, -1);
		cv::Vec6d corrected_pose_world = cv::Vec6d(-1, -1, -1, -1, -1, -1);
        cv::Vec6d corrected_pose_camera2 = cv::Vec6d(-1, -1, -1, -1, -1, -1);
        cv::Vec6d corrected_pose_world2 = cv::Vec6d(-1, -1, -1, -1, -1, -1);
		vector<cv::Point2d> detected_landmarks;
		for (size_t i = 0; i < 68; i++)
			detected_landmarks.push_back(cv::Point2d(-1, -1));

		vector<cv::Point3f> gaze;
		gaze.push_back(cv::Point3f(-1, -1, -1));
		gaze.push_back(cv::Point3f(-1, -1, -1));
		gaze.push_back(cv::Point3f(-1, -1, -1));
		gaze.push_back(cv::Point3f(-1, -1, -1));

		//convert img to Mat so Openface can do its Magic 
		cv::Mat captured_image; 
		if (_video_format.numOfChannels == 3) {
			captured_image = cv::Mat(_video_format.heightInPixels, _video_format.widthInPixels, CV_8UC(_video_format.numOfChannels), stream_in.ptr, ssi_video_stride(_video_format));
		}
		else if (_video_format.numOfChannels == 4) {
			cv::Mat temp = cv::Mat(_video_format.heightInPixels, _video_format.widthInPixels, CV_8UC(_video_format.numOfChannels), stream_in.ptr, ssi_video_stride(_video_format));
			cv::cvtColor(temp, captured_image, CV_BGRA2RGB);
		}

		//update needs to be called every Frame so the internal Model of Opfenface is up to date. Returns True if a Face was detecded
		bool detection_success = _helper->update_landmark(captured_image);
		
		if (detection_success)
		{
			detection_certainty = _helper->get_detection_certainty(); // -1 best, 1 worst





			if (_options.poscam)
				pose_camera = _helper->get_pose_camera(); 
			if (_options.posworld)
				pose_world = _helper->get_pose_world();
			if (_options.corrposcam)
				corrected_pose_camera = _helper->get_corrected_pose_camera();
			if (_options.corrposworld)
				corrected_pose_world = _helper->get_corrected_pose_world();
			if (_options.landmarks)
				detected_landmarks = _helper->get_detected_landmarks();
			if (_options.gaze) {
				vector<cv::Point3f> helper = _helper->get_gaze_directions(); // helper[0] => gaze_directions_left_eye, helper[1] => gaze_directions_right_eye
				gaze[0] = helper[0];
				gaze[1] = helper[1];
				helper = _helper->get_pupil_position(); // helper[0] => pupil_pos_left_eye, helper[1] => pupil_pos_right_eye
				gaze[2] = helper[0];
				gaze[3] = helper[1];
			}
            if(_options.pixelPercentage)
            {

                    corrected_pose_camera2 = _helper->get_corrected_pose_camera();
                    corrected_pose_world2 = _helper->get_corrected_pose_world();


                    // If optical centers are not defined just use center of image
                    std::vector<float> coi = _helper->calculate_coi(captured_image); // center of image
                    // Use a rough guess-timate of focal length
                    std::vector<float> fl = _helper->calculate_fl(captured_image); // focal length



                    cv::Vec6d pose; cv::Vec6d((double)corrected_pose_world2[0],
                                              (double)corrected_pose_world2[1],
                                              (double)corrected_pose_world2[2],
                                              (double)corrected_pose_camera2[4],
                                              (double)corrected_pose_camera2[5],
                                              (double)corrected_pose_camera2[6]
                                              );

                    vector<pair<cv::Point, cv::Point>> blubb =_helper->calculate_box(corrected_pose_world2, fl[0], fl[1], coi[0], coi[1]);

                    //for(int i=0; i< blubb.size(); i++)printf("box line[%d]: (%d;%d) (%d;%d)\n",i,blubb.at(i).first.x,blubb.at(i).first.y, blubb.at(i).second.x,blubb.at(i).second.y );

                    //calculate face rect's dimensions from box, percentage of image covered with face
                    int p1x, p1y, p2x, p2y;
                    //boxX=distance(point(1+2)/2 - point (0+3)/3)
                      p1x=blubb.at(0).second.x+blubb.at(1).second.x;
                      p1y=blubb.at(0).second.y+blubb.at(1).second.y;

                      p2x=blubb.at(0).first.x+blubb.at(2).second.x;
                      p2y=blubb.at(0).first.y+blubb.at(2).second.y;

                      boxX=sqrt((p1x-p2x)*(p1x-p2x)*0.25+(p1y-p2y)*(p1y-p2y)*0.25);


                    //boxY=distance(point (0+2)/2 - point (7+6)/2
                      p1x=blubb.at(0).first.x+blubb.at(1).second.x;
                      p1y=blubb.at(0).first.y+blubb.at(1).second.y;

                      p2x=blubb.at(11).second.x+blubb.at(11).first.x;
                      p2y=blubb.at(11).second.y+blubb.at(11).first.y;

                      boxY=sqrt((p1x-p2x)*(p1x-p2x)*0.25+(p1y-p2y)*(p1y-p2y)*0.25);

                     // printf("distances x: %d y: %d pixelperce: %d (%d of %d)\n", boxX, boxY, boxX*boxY*100/(captured_image.rows*captured_image.cols),boxX*boxY, captured_image.rows*captured_image.cols);
                    percentage= boxX*boxY*100/(captured_image.rows*captured_image.cols);


                }
		}
		
		//Fill output stream with Features regarding DETECTION 
		out[FEATURE::DETECTION_SUCCESS] = (float)detection_success;
		out[FEATURE::DETECTION_CERTAINTY] = (float)detection_certainty;

		//Fill output stream with Features regarding POSE 
		std::vector <cv::Vec6d> posevec;
		posevec.push_back(pose_camera);
		posevec.push_back(pose_world);
		posevec.push_back(corrected_pose_camera);
		posevec.push_back(corrected_pose_world);
		for (int j = 0; j < posevec.size(); j++) { //POSE_CAMERA_X is the First Value of all Features regarding POSE so it works as an offset
			for (int i = 0; i < 6; i++) //a Vec6d has only 6 elements
				out[FEATURE::POSE_CAMERA_X + j*6+i] = (float)posevec[j][i];
		}
		
		//Fill output stream with Features regarding FACIAL_LANDMARK
		for (int j = 0; j < detected_landmarks.size(); j++) { //FACIAL_LANDMARK_1_X is the First Value of all Features regarding POSE so it works as an offset
			for (int i = 0; i < 2; i++) {//a Point2d has only 2 elements
				if (i == 0)
					out[FEATURE::FACIAL_LANDMARK_1_X + j * 2 + i] = (float)detected_landmarks[j].x;
				else
					out[FEATURE::FACIAL_LANDMARK_1_X + j * 2 + i] = (float)detected_landmarks[j].y;
			}
		}

		//Fill output stream with Features regarding GAZE_DIRECTION
		for (int j = 0; j < gaze.size(); j++) { //GAZE_LEFT_EYE_X is the First Value of all Features regarding GAZE_DIRECTION so it works as an offset
			for (int i = 0; i < 3; i++) {//a Point3f has only 3 elements
				if (i==0)
					out[FEATURE::GAZE_LEFT_EYE_X + j * 3 + i] = (float)gaze[j].x;
				else if (i==1)
					out[FEATURE::GAZE_LEFT_EYE_X + j * 3 + i] = (float)gaze[j].y;
				else
					out[FEATURE::GAZE_LEFT_EYE_X + j * 3 + i] = (float)gaze[j].z;
			}
		}

		/*
		*********************************
		*****	FaceanAlyser Part	*****
		*********************************
		*/
		vector<double> au_reg;
		for (int i = FEATURE::AU01_r; i < FEATURE::AU_CLASS_DETECTION_SUCCESS; i++)
			au_reg.push_back(-1.0);

		vector<double> au_classes;
		for (int i = FEATURE::AU04_c; i < FEATURE::NUM; i++)
			au_classes.push_back(-1.0);

		double au_reg_success = 0.0;
		double au_classes_success = 0.0;

		if (_options.actionunits) {
			_helper->update_faceanalyser(captured_image, true, _video_format.framesPerSecond);
			
			vector<double> au_reg_tmp = _helper->get_current_action_unit_reg();
			if (au_reg_tmp.size() > 0) {
				au_reg = au_reg_tmp;
				au_reg_success = 1.0;
			}

			vector<double> au_classes_tmp = _helper->get_current_action_unit_classes();
			if (au_classes_tmp.size() > 0) {
				au_classes = au_classes_tmp;
				au_classes_success = 1.0;
			}

		}
		out[FEATURE::AU_REG_DETECTION_SUCCESS] = au_reg_success;
		//Fill output stream with Features regarding ACTION_UNIT_REG
		for (int j = 0; j < au_reg.size(); j++) { //AU01_r is the First Value of all Features regarding ACTION_UNIT_REG so it works as an offset
			out[FEATURE::AU01_r + j] = (float) au_reg[j];
		}

		out[FEATURE::AU_CLASS_DETECTION_SUCCESS] = au_classes_success;
		//Fill output stream with Features regarding ACTION_UNIT_CLASS
		for (int j = 0; j < au_classes.size(); j++) { //AU04_c is the First Value of all Features regarding ACTION_UNIT_CLASS so it works as an offset
			out[FEATURE::AU04_c + j] = (float) au_classes[j];
		}

          out[FEATURE::PIXEL_PERCENTAGE]=percentage;
          out[FEATURE::BOXX]=boxX;
          out[FEATURE::BOXY]=boxY;

	}

	void Openface::transform_flush(ssi_stream_t &stream_in,
		ssi_stream_t &stream_out,
		ssi_size_t xtra_stream_in_num,
		ssi_stream_t xtra_stream_in[]) {

		delete _helper; _helper = 0;
	}


}
