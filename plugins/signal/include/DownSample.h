// DownSample.h
// author: Johannes Wagner <wagner@hcm-lab.de>
// created: 2009/03/12
// Copyright (C) University of Augsburg, Lab for Human Centered Multimedia
//
// *************************************************************************************************
//
// This file is part of Social Signal Interpretation (SSI) developed at the 
// Lab for Human Centered Multimedia of the University of Augsburg
//
// This library is free software; you can redistribute itand/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or any laterversion.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FORA PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public
// License along withthis library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
//*************************************************************************************************

#pragma once

#ifndef SSI_SIGNAL_DOWNSAMPLE_H
#define SSI_SIGNAL_DOWNSAMPLE_H

#include "base/ITransformer.h"
#include "ioput/option/OptionList.h"

namespace ssi {

class DownSample : public ITransformer {

public:

	class Options : public OptionList {

	public:

		Options ()
			: keep(1), mean(false) {

			addOption ("keep", &keep, 1, SSI_SIZE, "keep every n'th sample (> 0)");		
			addOption ("mean", &mean, 1, SSI_BOOL, "output mean value of last n'th values");
		};

		bool mean;
		ssi_size_t keep;
	};

public:

	static const ssi_char_t *GetCreateName () { return "DownSample"; };
	static IObject *Create (const ssi_char_t *file) { return new DownSample (file); };
	~DownSample ();

	Options *getOptions () { return &_options; };
	const ssi_char_t *getName () { return GetCreateName (); };
	const ssi_char_t *getInfo () { return "Downsamples a signal keeping only every x'th sample."; };

	ssi_size_t getSampleDimensionOut (ssi_size_t sample_dimension_in) {
		return sample_dimension_in;
	}
	ssi_size_t getSampleNumberOut (ssi_size_t sample_number_in) {
		return sample_number_in / _options.keep;
	}
	ssi_size_t getSampleBytesOut (ssi_size_t sample_bytes_in) {
		return sample_bytes_in;
	}
	ssi_type_t getSampleTypeOut (ssi_type_t sample_type_in) {
		if (sample_type_in != SSI_REAL) {
			ssi_err ("type %s not supported", SSI_TYPE_NAMES[sample_type_in]);
		}
		return SSI_REAL;
	}

	void transform_enter (ssi_stream_t &stream_in,
		ssi_stream_t &stream_out,
		ssi_size_t xtra_stream_in_num = 0,
		ssi_stream_t xtra_stream_in[] = 0);
	void transform (ITransformer::info info,
		ssi_stream_t &stream_in,
		ssi_stream_t &stream_out,
		ssi_size_t xtra_stream_in_num = 0,
		ssi_stream_t xtra_stream_in[] = 0);
	void transform_flush (ssi_stream_t &stream_in,
		ssi_stream_t &stream_out,
		ssi_size_t xtra_stream_in_num = 0,
		ssi_stream_t xtra_stream_in[] = 0);

protected:

	DownSample (const ssi_char_t *file = 0);
	Options _options;
	ssi_char_t *_file;

	ssi_real_t *_tmp;
	ssi_size_t _count;
};

}

#endif