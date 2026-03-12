/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_rgbcac.c
 * Description:
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "isp_algo_rgbcac.h"
#include "isp_algo_debug.h"


CVI_S32 isp_algo_rgbcac_init(void)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	return ret;
}

CVI_S32 isp_algo_rgbcac_uninit(void)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	return ret;
}

CVI_S32 isp_algo_rgbcac_main(struct rgbcac_param_in *rgbcac_param_in, struct rgbcac_param_out *rgbcac_param_out)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	rgbcac_param_out->u16DePurpleStrRatioMax0 = 4095 - (rgbcac_param_in->u8DePurpleStrMin0 * 16);
	rgbcac_param_out->u16DePurpleStrRatioMin0 = 4095 - (rgbcac_param_in->u8DePurpleStrMax0 * 16);
	rgbcac_param_out->u16DePurpleStrRatioMax1 = 4095 - (rgbcac_param_in->u8DePurpleStrMin1 * 16);
	rgbcac_param_out->u16DePurpleStrRatioMin1 = 4095 - (rgbcac_param_in->u8DePurpleStrMax1 * 16);

	CVI_U8 *pu8GainIn = rgbcac_param_in->au8EdgeGainIn;
	CVI_U8 *pu8GainOut = rgbcac_param_in->au8EdgeGainOut;
	CVI_U8 *pu8Lut = rgbcac_param_out->au8EdgeScaleLut;
	int dx = 0, dy = 0;

	for (CVI_U32 idx = 0; idx < RGBCAC_EDGE_SCALE_LUT_SIZE; ++idx) {
		if (idx <= pu8GainIn[0]) {
			pu8Lut[idx] = pu8GainOut[0];
		} else if (idx >= pu8GainIn[2]) {
			pu8Lut[idx] = pu8GainOut[2];
		} else if (idx <= pu8GainIn[1]) {
			dy = pu8GainOut[1] - pu8GainOut[0];
			dx = pu8GainIn[1] - pu8GainIn[0];
			pu8Lut[idx] = pu8GainOut[0] + ((idx - pu8GainIn[0]) * dy + dx/2) / MAX(dx, 1);
		} else { // if (n <= pu8GainIn[2]) {
			dy = pu8GainOut[2] - pu8GainOut[1];
			dx = pu8GainIn[2] - pu8GainIn[1];
			pu8Lut[idx] = pu8GainOut[1] + ((idx - pu8GainIn[1]) * dy + dx/2) / MAX(dx, 1);
		}
	}

	return ret;
}
