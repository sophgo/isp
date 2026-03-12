/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_ca2.c
 * Description:
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "limits.h"

#include "isp_algo_ca2.h"
#include "isp_algo_debug.h"
#include "isp_algo_utility.h"

#define CA2_LUT_SLP_FBIT 4

CVI_S32 isp_algo_ca2_main(struct ca2_param_in *ca2_param_in, struct ca2_param_out *ca2_param_out)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	for (CVI_U32 i = 0 ; i < CA_LITE_NODE - 1 ; i++) {
		CVI_U8 in0 = ISP_PTR_CAST_U8(ca2_param_in->Ca2In)[i];
		CVI_U8 in1 = ISP_PTR_CAST_U8(ca2_param_in->Ca2In)[i+1];
		CVI_U16 out0 = ISP_PTR_CAST_U16(ca2_param_in->Ca2Out)[i];
		CVI_U16 out1 = ISP_PTR_CAST_U16(ca2_param_in->Ca2Out)[i+1];

		ca2_param_out->Ca2Slope[i] = get_lut_slp(in0, in1, out0, out1, CA2_LUT_SLP_FBIT);
	}

	return ret;
}

CVI_S32 isp_algo_ca2_init(void)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	return ret;
}

CVI_S32 isp_algo_ca2_uninit(void)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	return ret;
}
