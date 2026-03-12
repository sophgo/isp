/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_csc.c
 * Description:
 *
 */

#include <string.h>
#include <math.h>

#include "isp_algo_csc.h"
#include "isp_algo_utility.h"

// Number of bits required for a decimal part of a CSC coeff
#define DECIMAL_BITS 10

static void Mat_3x3_Product_3x3(float *A_3x3, const float *B_3x3, const float *C_3x3)
{
	for (unsigned char r = 0; r < 3; r++) {
		for (unsigned char c = 0; c < 3; c++) {
			float *A = &A_3x3[r * 3 + c];
			const float *B = &B_3x3[r * 3 + 0];
			const float *C = &C_3x3[0 * 3 + c];
			*A = 0;
			for (unsigned char i = 0; i < 3; i++) {
				*A += *B * *C;
				B++;
				C += 3;
			}
		}
	}
}

static void Mat_3x1_Add_3x1(float *A_3x1, const float *B_3x1, const float *C_3x1)
{
	float *A = A_3x1;
	const float *B = B_3x1;
	const float *C = C_3x1;

	for (unsigned char i = 0; i < 3; i++) {
		A[i] = B[i] + C[i];
	}
}

static const float _BT601_RGB2Y[9] = { 0.299, 0.587, 0.114, -0.169, -0.331, 0.500, 0.500, -0.419, -0.081 };
static const float _BT709_RGB2Y[9] = { 0.213, 0.715, 0.072, -0.115, -0.385, 0.500, 0.500, -0.454, -0.046 };
static const float _BT2020_RGB2Y[9] = { 0.262, 0.678, 0.059, -0.139, -0.361, 0.500, 0.500, -0.459, -0.041 };
static const float _RGB2Y_OFFSET[3] = { 0, 128, 128 };

CVI_S32 isp_algo_csc_main(struct csc_param_in *csc_param_in, struct csc_param_out *csc_param_out)
{
	ISP_ALGO_CHECK_POINTER(csc_param_in);
	ISP_ALGO_CHECK_POINTER(csc_param_out);
	const float *pRgb2y = NULL;
	CVI_S32 ret = CVI_SUCCESS;

	if (csc_param_in->eCscColorGamut == ISP_CSC_COLORGAMUT_USER) {
		memcpy(csc_param_out->s16cscCoef, csc_param_in->s16userCscCoef, sizeof(CVI_S16) * CSC_MATRIX_SIZE);
		//In Csc module,domain is 10bit instead of 8bit,so we should multiply offset by 4
		for (CVI_U8 i = 0; i < 3; ++i)
			csc_param_out->s16cscOffset[i] = ROUND(csc_param_in->s16userCscOffset[i] * 4);
	} else {
		switch (csc_param_in->eCscColorGamut) {
		case ISP_CSC_COLORGAMUT_BT601:
			pRgb2y = _BT601_RGB2Y;
			break;
		case ISP_CSC_COLORGAMUT_BT709:
			pRgb2y = _BT709_RGB2Y;
			break;
		case ISP_CSC_COLORGAMUT_BT2020:
			pRgb2y = _BT2020_RGB2Y;
			break;
		default:
			ISP_ALGO_LOG_WARNING("%d is not support, use BT601\n", csc_param_in->eCscColorGamut);
			pRgb2y = _BT601_RGB2Y;
			break;
		}
		float h =  ((csc_param_in->u8hue - 50) * PI) / 360;
		float b_off = (csc_param_in->u8luma - 50) * 2.56;
		float c = 1 + (csc_param_in->u8contrast - 50) * 0.02;
		float s = 1 + (csc_param_in->u8saturation - 50) * 0.02;
		float A = cos(h) * c * s;
		float B = sin(h) * c * s;
		float adjCoef[9] = {c, 0, 0, 0, A, B, 0, -B, A};
		float coef[9] = {0};
		float offset[3] = {0};

		if (csc_param_in->u8contrast > 50) {
			float c_off = 128 - (128/c);

			offset[0] = c * (b_off - c_off);
			offset[1] = 0;
			offset[2] = 0;
		} else {
			float c_off = 128 - 128 * c;

			offset[0] = c * b_off + c_off;
			offset[1] = 0;
			offset[2] = 0;
		}

		Mat_3x3_Product_3x3(coef, adjCoef, pRgb2y);
		Mat_3x1_Add_3x1(offset, offset, _RGB2Y_OFFSET);

		for (CVI_U8 i = 0; i < 9; ++i) {
			csc_param_out->s16cscCoef[i] =
				ROUND(coef[i] * (1 << DECIMAL_BITS));
		}

		for (CVI_U8 i = 0; i < 3; ++i) {
			//In Csc module,domain is 10bit instead of 8bit,so we should multiply offset by 4
			csc_param_out->s16cscOffset[i] = ROUND(offset[i] * 4);
		}
	}

	return ret;
}

CVI_S32 isp_algo_csc_init(void)
{
	CVI_S32 ret = CVI_SUCCESS;

	return ret;
}

CVI_S32 isp_algo_csc_uninit(void)
{
	CVI_S32 ret = CVI_SUCCESS;

	return ret;
}
