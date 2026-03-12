/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_mono.c
 * Description:
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "isp_algo_mono.h"
#include "isp_algo_debug.h"

CVI_S32 isp_algo_mono_main(struct mono_param_in *mono_param_in, struct mono_param_out *mono_param_out)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(mono_param_in);
	UNUSED(mono_param_out);

	return ret;
}

CVI_S32 isp_algo_mono_init(void)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	return ret;
}

CVI_S32 isp_algo_mono_uninit(void)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	return ret;
}
