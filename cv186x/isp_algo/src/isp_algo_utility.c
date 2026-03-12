/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_utility.c
 * Description:
 *
 */

#include "isp_algo_utility.h"

static CVI_U32 iso_tbl[16] = { 100, 200, 400, 800, 1600, 3200, 6400,
	12800, 25600, 51200, 102400, 204800, 409600, 819200, 1638400, 3276800 };

int get_lut_slp(int lut_in_0, int lut_in_1, int lut_out_0, int lut_out_1, int SLOPE_F_BITS)
{
	int slope;
	float dA, dB;

	dA = (float)lut_out_1 - (float)lut_out_0;
	dB = (float)lut_in_1 - (float)lut_in_0;
	slope = (int)((dA / dB) * (1 << SLOPE_F_BITS));        //Fp s0.11
	return slope;
}

CVI_S32 get_iso_tbl(CVI_U32 **tbl)
{
	*tbl = iso_tbl;

	return CVI_SUCCESS;
}
