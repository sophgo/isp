/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_cac.c
 * Description:
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "isp_algo_cac.h"
#include "isp_algo_debug.h"


CVI_S32 isp_algo_cac_init(void)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	return ret;
}

CVI_S32 isp_algo_cac_uninit(void)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	return ret;
}

CVI_S32 isp_algo_cac_main(struct cac_param_in *cac_param_in, struct cac_param_out *cac_param_out)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	cac_param_out->u8CbStr = 16 - cac_param_in->u8DePurpleCbStr;
	cac_param_out->u8CrStr = 16 - cac_param_in->u8DePurpleCrStr;
	cac_param_out->u8DePurpleStrRatioMax = 64 - cac_param_in->u8DePurpleStrMinRatio;
	cac_param_out->u8DePurpleStrRatioMin = 64 - cac_param_in->u8DePurpleStrMaxRatio;

	CVI_U8 *pu8GainIn = cac_param_in->au8EdgeGainIn;
	CVI_U8 *pu8GainOut = cac_param_in->au8EdgeGainOut;
	CVI_U8 *pu8Lut = cac_param_out->au8EdgeScaleLut;
	int dx = 0, dy = 0;

	for (CVI_U32 idx = 0; idx < CAC_EDGE_SCALE_LUT_SIZE; ++idx) {
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
