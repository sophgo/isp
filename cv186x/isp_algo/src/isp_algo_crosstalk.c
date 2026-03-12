/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_crosstalk.c
 * Description:
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "isp_algo_crosstalk.h"
#include "isp_algo_debug.h"

CVI_S32 isp_algo_crosstalk_main(
	struct crosstalk_param_in *crosstalk_param_in, struct crosstalk_param_out *crosstalk_param_out)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(crosstalk_param_in);
	UNUSED(crosstalk_param_out);

	return ret;
}

CVI_S32 isp_algo_crosstalk_init(void)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	return ret;
}

CVI_S32 isp_algo_crosstalk_uninit(void)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	return ret;
}
