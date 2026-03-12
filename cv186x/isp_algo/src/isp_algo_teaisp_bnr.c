/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_bnr.c
 * Description:
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "isp_algo_teaisp_bnr.h"
#include "isp_algo_debug.h"
#include "isp_algo_utility.h"

CVI_S32 isp_algo_teaisp_bnr_main(struct teaisp_bnr_param_in *in, struct teaisp_bnr_param_out *out)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	CVI_U32 *ISO_list;

	get_iso_tbl(&ISO_list);

	int ISO_Val = in->iso;
	int iso_idx_0 = MIN(FindPower2((int)(ISO_Val / 100)), 15);
	int iso_idx_1 = (iso_idx_0 >= 15) ? iso_idx_0 : iso_idx_0 + 1;
	int ISO_val_0 = ISO_list[iso_idx_0];
	int ISO_val_1 = ISO_list[iso_idx_1];


	CVI_FLOAT fCaliSlope = 0;
	CVI_FLOAT fCaliIntercept = 0;

	CVI_FLOAT fCaliSlope0 = 0;
	CVI_FLOAT fCaliIntercept0 = 0;
	CVI_FLOAT fCaliSlope1 = 0;
	CVI_FLOAT fCaliIntercept1 = 0;

	if (iso_idx_0 < 15) {

		fCaliSlope0 = in->np->CalibrationCoef[0][iso_idx_0];
		fCaliIntercept0 = in->np->CalibrationCoef[1][iso_idx_0];
		fCaliSlope1 = in->np->CalibrationCoef[0][iso_idx_1];
		fCaliIntercept1 = in->np->CalibrationCoef[1][iso_idx_1];

		CVI_FLOAT w0 = ISO_Val - ISO_val_0;
		CVI_FLOAT w1 = ISO_val_1 - ISO_Val;

		fCaliSlope = (fCaliSlope0 * w1 + fCaliSlope1 * w0) / (w0 + w1);
		fCaliIntercept = (fCaliIntercept0 * w1 + fCaliIntercept1 * w0) / (w0 + w1);

	} else { // if (iso_idx_0 >= 15)
		fCaliSlope = in->np->CalibrationCoef[0][iso_idx_0];
		fCaliIntercept = in->np->CalibrationCoef[1][iso_idx_0];
	}

	out->slope = fCaliSlope * ((CVI_FLOAT) in->NoiseHiLevel / 1024.0);
	out->intercept = fCaliIntercept * ((CVI_FLOAT) in->NoiseLevel / 1024.0);

	return ret;
}

CVI_S32 isp_algo_teaisp_bnr_init(void)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	return ret;
}

CVI_S32 isp_algo_teaisp_bnr_uninit(void)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	return ret;
}

