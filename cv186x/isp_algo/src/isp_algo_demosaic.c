/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_demosaic.c
 * Description:
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "isp_algo_demosaic.h"
#include "isp_algo_debug.h"
#include "isp_algo_utility.h"

#define DEMOSAIC_LUT_SLP_FBIT 10

CVI_S32 isp_algo_demosaic_main(
	struct demosaic_param_in *demosaic_param_in,
	struct demosaic_param_out *demosaic_param_out)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	{
		CVI_U16 in0 = demosaic_param_in->SatGainIn[0];
		CVI_U16 in1 = demosaic_param_in->SatGainIn[1];
		CVI_U16 out0 = demosaic_param_in->SatGainOut[0];
		CVI_U16 out1 = demosaic_param_in->SatGainOut[1];

		demosaic_param_out->SatGainSlope = get_lut_slp(in0, in1, out0, out1, DEMOSAIC_LUT_SLP_FBIT);
	}
	{
		CVI_U16 in0 = demosaic_param_in->ProtectColorGainIn[0];
		CVI_U16 in1 = demosaic_param_in->ProtectColorGainIn[1];
		CVI_U16 out0 = demosaic_param_in->ProtectColorGainOut[0];
		CVI_U16 out1 = demosaic_param_in->ProtectColorGainOut[1];

		demosaic_param_out->ProtectColorGainSlope = get_lut_slp(in0, in1, out0, out1, DEMOSAIC_LUT_SLP_FBIT);
	}
	{
		CVI_U16 in0 = demosaic_param_in->EdgeGainIn[0];
		CVI_U16 in1 = demosaic_param_in->EdgeGainIn[1];
		CVI_U16 out0 = demosaic_param_in->EdgeGainOut[0];
		CVI_U16 out1 = demosaic_param_in->EdgeGainOut[1];

		demosaic_param_out->EdgeGainSlope = get_lut_slp(in0, in1, out0, out1, DEMOSAIC_LUT_SLP_FBIT);
	}
	{
		CVI_U16 in0 = demosaic_param_in->DetailGainIn[0];
		CVI_U16 in1 = demosaic_param_in->DetailGainIn[1];
		CVI_U16 out0 = demosaic_param_in->DetailGaintOut[0];
		CVI_U16 out1 = demosaic_param_in->DetailGaintOut[1];

		demosaic_param_out->DetailGainSlope = get_lut_slp(in0, in1, out0, out1, DEMOSAIC_LUT_SLP_FBIT);
	}

	return ret;
}

CVI_S32 isp_algo_demosaic_init(void)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	return ret;
}

CVI_S32 isp_algo_demosaic_uninit(void)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	return ret;
}
