/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: cvi_streamer.hpp
 * Description:
 *
 */

#pragma once

extern "C" {
#include "cvi_type.h"
#include "cvi_comm_video.h"
}

namespace cvitek {
namespace system {
class CVIStreamer {
	public:
		CVIStreamer();
		~CVIStreamer();

	private:
		CVI_S32 stream_run();
		CVI_S32 stream_exit();

};
} // namespace system
} // namespace cvitek
