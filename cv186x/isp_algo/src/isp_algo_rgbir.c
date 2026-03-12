/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2023. All rights reserved.
 *
 * File Name: isp_algo_rgbir.c
 * Description:
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "isp_algo_rgbir.h"
#include "isp_algo_debug.h"

CVI_S32 isp_algo_rgbir_main(
	struct rgbir_param_in *rgbir_param_in, struct rgbir_param_out *rgbir_param_out)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(rgbir_param_in);
	UNUSED(rgbir_param_out);

	return ret;
}

CVI_S32 isp_algo_rgbir_init(void)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	return ret;
}

CVI_S32 isp_algo_rgbir_uninit(void)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	return ret;
}
