/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_cnr.c
 * Description:
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "isp_algo_cnr.h"
#include "isp_algo_debug.h"

#define WLUT_INTER_BLOCK_SIZE_CNR 16
static const unsigned short int WeightInterBlockLUT_h2[32 * WLUT_INTER_BLOCK_SIZE_CNR] = {
	16,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	16,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	16,  6,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	16,  8,  4,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	16,  9,  5,  3,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	16, 10,  7,  4,  3,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	16, 11,  8,  5,  4,  2,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	16, 12,  9,  6,  5,  3,  2,  1,  0,  0,  0,  0,  0,  0,  0,  0,
	16, 12,  9,  7,  6,  4,  3,  2,  1,  0,  0,  0,  0,  0,  0,  0,
	16, 13, 10,  8,  6,  5,  4,  3,  2,  1,  0,  0,  0,  0,  0,  0,
	16, 13, 11,  9,  7,  6,  5,  4,  3,  2,  1,  0,  0,  0,  0,  0,
	16, 13, 11,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,  0,  0,  0,
	16, 14, 12, 10,  9,  7,  6,  5,  5,  4,  3,  2,  1,  0,  0,  0,
	16, 14, 12, 11,  9,  8,  7,  6,  5,  5,  4,  3,  2,  1,  0,  0,
	16, 14, 12, 11, 10,  9,  8,  7,  6,  5,  5,  4,  4,  2,  1,  0,
	16, 14, 13, 11, 10,  9,  8,  7,  7,  6,  5,  5,  4,  4,  2,  1,
	16, 16, 15, 13, 12, 10,  8,  7,  7,  6,  5,  5,  4,  4,  2,  1,
	16, 16, 15, 14, 13, 11, 10,  8,  7,  6,  5,  5,  4,  4,  2,  1,
	16, 16, 15, 15, 14, 12, 11, 10,  8,  7,  6,  5,  4,  4,  2,  2,
	16, 16, 16, 15, 14, 13, 12, 11, 10,  8,  7,  6,  5,  4,  3,  3,
	16, 16, 16, 15, 14, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,
	16, 16, 16, 15, 15, 14, 13, 13, 12, 11, 10,  9,  8,  7,  6,  5,
	16, 16, 16, 15, 15, 14, 14, 13, 12, 11, 11, 10,  9,  8,  7,  6,
	16, 16, 16, 16, 15, 15, 14, 13, 13, 12, 11, 11, 10,  9,  8,  7,
	16, 16, 16, 16, 15, 15, 14, 14, 13, 13, 12, 11, 10, 10,  9,  8,
	16, 16, 16, 16, 15, 15, 15, 14, 14, 13, 12, 12, 11, 10, 10,  9,
	16, 16, 16, 16, 15, 15, 15, 14, 14, 13, 13, 12, 12, 11, 10, 10,
	16, 16, 16, 16, 16, 15, 15, 15, 14, 14, 13, 13, 12, 12, 11, 10,
	16, 16, 16, 16, 16, 15, 15, 15, 14, 14, 13, 13, 12, 12, 11, 11,
	16, 16, 16, 16, 16, 15, 15, 15, 14, 14, 14, 13, 13, 12, 12, 11,
	16, 16, 16, 16, 16, 15, 15, 15, 15, 14, 14, 14, 13, 13, 12, 12,
	16, 16, 16, 16, 16, 16, 15, 15, 15, 14, 14, 14, 13, 13, 13, 12
};

typedef struct {
	unsigned int reg_cnr_weight_v;
	unsigned int reg_cnr_weight_h;
} YNR_FILT_VH_S;

typedef struct {
	unsigned int reg_cnr_weight_d45;
	unsigned int reg_cnr_weight_d135;
} YNR_FILT_AA_S;

static void CNR_LUT_H_API(CVI_U8 *reg_cnr_weight_lut_h, CVI_U16 CNR_CoarseStr);

CVI_S32 isp_algo_cnr_main(struct cnr_param_in *cnr_param_in, struct cnr_param_out *cnr_param_out)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	CNR_LUT_H_API(cnr_param_out->weight_lut, cnr_param_in->filter_type);

	return ret;
}

CVI_S32 isp_algo_cnr_init(void)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	return ret;
}

CVI_S32 isp_algo_cnr_uninit(void)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	return ret;
}

/**
 * @brief CNR: Obtain LUT for block matching
 *
 * @param[out] reg_cnr_weight_lut_h [256]
 * @param[in] CNR_CoarseStr [0, 255]
 */
static void CNR_LUT_H_API(CVI_U8 *reg_cnr_weight_lut_h, CVI_U16 CNR_CoarseStr)
{
	for (int i = 0; i < CNR_MOTION_LUT_NUM; i++)
		reg_cnr_weight_lut_h[i] = WeightInterBlockLUT_h2[CNR_CoarseStr * 16 + i];
}
