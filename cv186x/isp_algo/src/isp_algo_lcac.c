/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2022. All rights reserved.
 *
 * File Name: isp_algo_lcac.c
 * Description:
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "isp_algo_lcac.h"
#include "isp_algo_debug.h"
#include "isp_algo_utility.h"

#define LCAC_LUMA_LUT_SIZE				(33)
#define LCAC_LUMA_UNIT_GAIN				(8)
#define LCAC_FILTER_SIZE_W				(9)
#define LCAC_FILTER_SIZE_H				(1)

#define MEMORY_FCF_FILTER				(sizeof(float) * LCAC_FILTER_SIZE_W * LCAC_FILTER_SIZE_H)
#define MEMORY_LTI_STRENGTH_LUT			(sizeof(int) * LCAC_LUMA_LUT_SIZE)
#define MEMORY_LTI_STRENGTH_TEMP		(sizeof(float) * LCAC_LUMA_LUT_SIZE)
#define MEMORY_LTI_STRENGTH				(MEMORY_LTI_STRENGTH_LUT + MEMORY_LTI_STRENGTH_TEMP)
#define MEMORY_FCF_STRENGTH_LUT			(sizeof(int) * LCAC_LUMA_LUT_SIZE)
#define MEMORY_FCF_STRENGTH_TEMP		(sizeof(float) * LCAC_LUMA_LUT_SIZE)
#define MEMORY_FCF_STRENGTH				(MEMORY_FCF_STRENGTH_LUT + MEMORY_FCF_STRENGTH_TEMP)

static CVI_S32 isp_algo_lcac_update_lti_kernel(
	struct lcac_param_in *lcac_param_in, struct lcac_param_out *lcac_param_out);
static CVI_S32 isp_algo_lcac_update_fcf_kernel(
	struct lcac_param_in *lcac_param_in, struct lcac_param_out *lcac_param_out);
static CVI_S32 isp_algo_lcac_update_lti_luma_lut(
	struct lcac_param_in *lcac_param_in, struct lcac_param_out *lcac_param_out);
static CVI_S32 isp_algo_lcac_update_fcf_luma_lut(
	struct lcac_param_in *lcac_param_in, struct lcac_param_out *lcac_param_out);

CVI_S32 isp_algo_lcac_init(void)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	return ret;
}

CVI_S32 isp_algo_lcac_uninit(void)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	return ret;
}

CVI_S32 isp_algo_lcac_get_internal_memory(CVI_U32 *memory_size)
{
	CVI_S32 ret = CVI_SUCCESS;
	CVI_U32 u32MemoryUsage = 0;

	u32MemoryUsage = MAX(MEMORY_FCF_FILTER, u32MemoryUsage);
	u32MemoryUsage = MAX(MEMORY_LTI_STRENGTH, u32MemoryUsage);
	u32MemoryUsage = MAX(MEMORY_FCF_STRENGTH, u32MemoryUsage);

	*memory_size = u32MemoryUsage + 16;

	return ret;
}

CVI_S32 isp_algo_lcac_main(struct lcac_param_in *lcac_param_in, struct lcac_param_out *lcac_param_out)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	if (lcac_param_in->pvIntMemory == CVI_NULL) {
		return CVI_FAILURE;
	}

	isp_algo_lcac_update_lti_kernel(lcac_param_in, lcac_param_out);
	isp_algo_lcac_update_fcf_kernel(lcac_param_in, lcac_param_out);
	isp_algo_lcac_update_lti_luma_lut(lcac_param_in, lcac_param_out);
	isp_algo_lcac_update_fcf_luma_lut(lcac_param_in, lcac_param_out);

	return ret;
}

static void create_2D_Gaussian_kernel(int filter_size_h, int filter_size_w, int sigma, float *kernel)
{
	int rh = (filter_size_h - 1) / 2;
	int rw = (filter_size_w - 1) / 2;
	int filter_size = filter_size_h * filter_size_w;
	float fsigma = (float)sigma / 32.0;
	float sigma2 = 2.0 * fsigma * fsigma;
	float sum_kernel;

	memset(kernel, (float)0, sizeof(float) * filter_size);

	sum_kernel = 0.0;
	for (int y = -rh; y <= rh; y++) {
		for (int x = -rw; x <= rw; x++) {
			kernel[(y + rh) * filter_size_w + (x + rw)] = exp(-(y * y + x * x) / MAX(sigma2, 1e-5));
			sum_kernel += kernel[(y + rh) * filter_size_w + (x + rw)];
		}
	}

	for (int n = 0; n < filter_size; n++) {
		kernel[n] /= sum_kernel;
	}
}

// kernel => int kernel[filter_size]
// tempbuf => float tempbuf[filter_size]
static void create_1D_Gaussian_kernel(int filter_size, int Mean, int Sigma, int Wgt, int *kernel, float *tempbuf)
{
	int x0 = 0;
	int x1 = filter_size - 1;
	float fSigma = (float)Sigma / LCAC_LUMA_UNIT_GAIN;
	float fMean = (float)Mean / LCAC_LUMA_UNIT_GAIN;
	float fSigma2 = 2.0 * fSigma * fSigma;
	float sum_kernel, fMax;

	memset(kernel, 0, sizeof(int) * filter_size);
	memset(tempbuf, (float)0, sizeof(float) * filter_size);

	sum_kernel = 0.0;
	fMax = 0.0;
	for (int x = x0; x <= x1; x++) {
		tempbuf[x - x0] = exp(-(x - fMean) * (x - fMean) / fSigma2);
		sum_kernel += tempbuf[x - x0];
	}

	for (int n = 0; n < filter_size; n++) {
		tempbuf[n] /= sum_kernel;
		if (tempbuf[n] > fMax) {
			fMax = tempbuf[n];
		}
	}

	float Scale = (float)Wgt / fMax;

	for (int n = 0; n < filter_size; n++) {
		tempbuf[n] *= Scale;
		kernel[n] = (int)(tempbuf[n] + 0.5);
	}
}

static CVI_S32 isp_algo_lcac_update_lti_kernel(
	struct lcac_param_in *lcac_param_in, struct lcac_param_out *lcac_param_out)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	switch (lcac_param_in->u8FilterTypeBase) {
	case 0:
		lcac_param_out->as16LTIFilterKernel[0] = -32;
		lcac_param_out->as16LTIFilterKernel[1] = 64;
		lcac_param_out->as16LTIFilterKernel[2] = -32;
		break;
	case 1:
		lcac_param_out->as16LTIFilterKernel[0] = -64;
		lcac_param_out->as16LTIFilterKernel[1] = 96;
		lcac_param_out->as16LTIFilterKernel[2] = -32;
		break;
	case 2:
		lcac_param_out->as16LTIFilterKernel[0] = -128;
		lcac_param_out->as16LTIFilterKernel[1] = 160;
		lcac_param_out->as16LTIFilterKernel[2] = -32;
		break;
	case 3:
		lcac_param_out->as16LTIFilterKernel[0] = -192;
		lcac_param_out->as16LTIFilterKernel[1] = 224;
		lcac_param_out->as16LTIFilterKernel[2] = -32;
		break;
	}

	return ret;
}

static CVI_S32 isp_algo_lcac_update_fcf_kernel(
	struct lcac_param_in *lcac_param_in, struct lcac_param_out *lcac_param_out)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	int FCF_GaussLPFSigma = 80;
	float *fcf_filter_kernel = ISP_PTR_CAST_FLOAT(lcac_param_in->pvIntMemory);

	switch (lcac_param_in->u8FilterTypeAdv) {
	case 1:
		FCF_GaussLPFSigma = 32;
		break;
	case 2:
		FCF_GaussLPFSigma = 64;
		break;
	case 3:
		FCF_GaussLPFSigma = 80;
		break;
	case 4:
		FCF_GaussLPFSigma = 96;
		break;
	case 5:
		FCF_GaussLPFSigma = 128;
		break;
	default:
	case 0:
		FCF_GaussLPFSigma = 16;
		break;
	}

	create_2D_Gaussian_kernel(LCAC_FILTER_SIZE_H, LCAC_FILTER_SIZE_W, FCF_GaussLPFSigma, fcf_filter_kernel);
	lcac_param_out->au8FCFFilterKernel[0] = (int)(fcf_filter_kernel[0] * 64 + 0.5);
	lcac_param_out->au8FCFFilterKernel[1] = (int)(fcf_filter_kernel[1] * 64 + 0.5);
	lcac_param_out->au8FCFFilterKernel[2] = (int)(fcf_filter_kernel[2] * 64 + 0.5);
	lcac_param_out->au8FCFFilterKernel[3] = (int)(fcf_filter_kernel[3] * 64 + 0.5);
	lcac_param_out->au8FCFFilterKernel[4] = (int)(fcf_filter_kernel[4] * 64 + 0.5);

	return ret;
}

static CVI_S32 isp_algo_lcac_update_lti_luma_lut(
	struct lcac_param_in *lcac_param_in, struct lcac_param_out *lcac_param_out)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	const int LTI_GaussMean		= 255;
	int EdgeWgtBase_Sigma		= lcac_param_in->u8EdgeWgtBase_Sigma;
	int EdgeWgtBase_Wgt			= lcac_param_in->u8EdgeWgtBase_Wgt;

	int *lti_luma_lut = ISP_PTR_CAST_S32(lcac_param_in->pvIntMemory);
	ISP_VOID_PTR temp = lcac_param_in->pvIntMemory + MEMORY_LTI_STRENGTH_LUT;
	float *tempbuf = ISP_PTR_CAST_FLOAT(ALIGN(temp, 8)); // TODO: mason.zou

	create_1D_Gaussian_kernel(LCAC_LUMA_LUT_SIZE, LTI_GaussMean, EdgeWgtBase_Sigma, EdgeWgtBase_Wgt,
		lti_luma_lut, tempbuf);

	for (CVI_U32 idx = 0; idx < 33; ++idx) {
		lcac_param_out->au8LTILumaLut[idx] = lti_luma_lut[idx];
	}

	return ret;
}

static CVI_S32 isp_algo_lcac_update_fcf_luma_lut(
	struct lcac_param_in *lcac_param_in, struct lcac_param_out *lcac_param_out)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	const int FCF_GaussMean		= 255;
	int EdgeWgtAdvance_Sigma	= lcac_param_in->u8EdgeWgtAdv_Sigma;
	int EdgeWgtAdvance_Wgt		= lcac_param_in->u8EdgeWgtAdv_Wgt;

	int *fcf_luma_lut = ISP_PTR_CAST_S32(lcac_param_in->pvIntMemory);
	ISP_VOID_PTR temp = lcac_param_in->pvIntMemory + MEMORY_FCF_STRENGTH_LUT;
	float *tempbuf = ISP_PTR_CAST_FLOAT(ALIGN(temp, 8)); // TODO: mason.zou

	create_1D_Gaussian_kernel(LCAC_LUMA_LUT_SIZE, FCF_GaussMean, EdgeWgtAdvance_Sigma, EdgeWgtAdvance_Wgt,
		fcf_luma_lut, tempbuf);

	for (CVI_U32 idx = 0; idx < 33; ++idx) {
		lcac_param_out->au8FCFLumaLut[idx] = fcf_luma_lut[idx];
	}

	return ret;
}

