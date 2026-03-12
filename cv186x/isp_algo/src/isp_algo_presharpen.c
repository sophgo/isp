/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_presharpen.c
 * Description:
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "isp_algo_presharpen.h"
#include "isp_algo_debug.h"
#include "isp_algo_utility.h"

#include "isp_algo_drc.h"

#define EE_LUT_SLOPE_F_BITS 4

CVI_S32 isp_algo_presharpen_main(
	struct presharpen_param_in *presharpen_param_in, struct presharpen_param_out *presharpen_param_out)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	for (CVI_U32 i = 0 ; i < EE_LUT_NODE-1 ; i++) {
		CVI_U8 in0 = presharpen_param_in->LumaCorLutIn[i];
		CVI_U8 in1 = presharpen_param_in->LumaCorLutIn[i+1];
		CVI_U8 out0 = presharpen_param_in->LumaCorLutOut[i];
		CVI_U8 out1 = presharpen_param_in->LumaCorLutOut[i+1];

		presharpen_param_out->LumaCorLutSlope[i] =
			get_lut_slp(in0, in1, out0, out1, EE_LUT_SLOPE_F_BITS);
	}
	for (CVI_U32 i = 0 ; i < EE_LUT_NODE-1 ; i++) {
		CVI_U8 in0 = presharpen_param_in->MotionCorLutIn[i];
		CVI_U8 in1 = presharpen_param_in->MotionCorLutIn[i+1];
		CVI_U8 out0 = presharpen_param_in->MotionCorLutOut[i];
		CVI_U8 out1 = presharpen_param_in->MotionCorLutOut[i+1];

		presharpen_param_out->MotionCorLutSlope[i] =
			get_lut_slp(in0, in1, out0, out1, EE_LUT_SLOPE_F_BITS);
	}
	for (CVI_U32 i = 0 ; i < EE_LUT_NODE-1 ; i++) {
		CVI_U8 in0 = presharpen_param_in->MotionCorWgtLutIn[i];
		CVI_U8 in1 = presharpen_param_in->MotionCorWgtLutIn[i+1];
		CVI_U8 out0 = presharpen_param_in->MotionCorWgtLutOut[i];
		CVI_U8 out1 = presharpen_param_in->MotionCorWgtLutOut[i+1];

		presharpen_param_out->MotionCorWgtLutSlope[i] =
			get_lut_slp(in0, in1, out0, out1, EE_LUT_SLOPE_F_BITS);
	}
	for (CVI_U32 i = 0 ; i < EE_LUT_NODE-1 ; i++) {
		CVI_U8 in0 = presharpen_param_in->MotionShtGainIn[i];
		CVI_U8 in1 = presharpen_param_in->MotionShtGainIn[i+1];
		CVI_U8 out0 = presharpen_param_in->MotionShtGainOut[i];
		CVI_U8 out1 = presharpen_param_in->MotionShtGainOut[i+1];

		presharpen_param_out->MotionShtGainSlope[i] =
			get_lut_slp(in0, in1, out0, out1, EE_LUT_SLOPE_F_BITS);
	}
	for (CVI_U32 i = 0 ; i < EE_LUT_NODE-1 ; i++) {
		CVI_U8 in0 = presharpen_param_in->SatShtGainIn[i];
		CVI_U8 in1 = presharpen_param_in->SatShtGainIn[i+1];
		CVI_U8 out0 = presharpen_param_in->SatShtGainOut[i];
		CVI_U8 out1 = presharpen_param_in->SatShtGainOut[i+1];

		presharpen_param_out->SatShtGainSlope[i] =
			get_lut_slp(in0, in1, out0, out1, EE_LUT_SLOPE_F_BITS);
	}

	return ret;
}

CVI_S32 isp_algo_presharpen_init(void)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	return ret;
}

CVI_S32 isp_algo_presharpen_uninit(void)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	return ret;
}
