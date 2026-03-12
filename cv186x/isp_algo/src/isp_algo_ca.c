/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_ca.c
 * Description:
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "limits.h"

#include "isp_algo_ca.h"
#include "isp_algo_debug.h"
#include "isp_algo_utility.h"

CVI_S32 isp_algo_ca_main(struct ca_param_in *ca_param_in, struct ca_param_out *ca_param_out)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ca_param_in);
	UNUSED(ca_param_out);

	return ret;
}

CVI_S32 isp_algo_ca_init(void)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	return ret;
}

CVI_S32 isp_algo_ca_uninit(void)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	return ret;
}
