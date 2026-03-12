/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_dehaze.c
 * Description:
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "limits.h"
#include <math.h>

#include "isp_algo_dehaze.h"
#include "isp_algo_debug.h"

#define LPF_SIGMA_UNIT_GAIN_8 8

CVI_FLOAT wbuf_gs_kernel[DEHAZE_TMAP_GAIN_LUT_SIZE];
static CVI_VOID _calc_tmap_gain_lut(int filter_size, int Sigma, int Wgt, CVI_U8 *luma_weight_lut);

CVI_S32 isp_algo_dehaze_main(struct dehaze_param_in *dehaze_param_in, struct dehaze_param_out *dehaze_param_out)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	CVI_FLOAT luma_factor = dehaze_param_in->DehazeLumaCOEFFI / 100.0;
	CVI_FLOAT skin_factor = dehaze_param_in->DehazeSkinCOEFFI / 100.0;

	for (CVI_U32 i = 0 ; i < DEHAZE_SKIN_LUT_SIZE ; i++) {
		CVI_FLOAT x = ((CVI_FLOAT)i / (DEHAZE_SKIN_LUT_SIZE - 1));

		dehaze_param_out->DehazeLumaLut[i] = (CVI_U8)(128 * pow(x, luma_factor) + 0.5);
		dehaze_param_out->DehazeSkinLut[i] = (CVI_U8)(128 * pow(x, skin_factor) + 0.5);
	}

	_calc_tmap_gain_lut(DEHAZE_TMAP_GAIN_LUT_SIZE, dehaze_param_in->TransMapWgtSigma,
		dehaze_param_in->TransMapWgtWgt, dehaze_param_out->DehazeTmapGainLut);

	return ret;
}

CVI_S32 isp_algo_dehaze_init(void)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	return ret;
}

CVI_S32 isp_algo_dehaze_uninit(void)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	return ret;
}

static CVI_VOID _calc_tmap_gain_lut(int filter_size, int Sigma, int Wgt, CVI_U8 *luma_weight_lut)
{
	CVI_FLOAT fSigma = MAX(((CVI_FLOAT)Sigma) / LPF_SIGMA_UNIT_GAIN_8, 1e-5);
	CVI_FLOAT fSigma2 = 2.0 * fSigma * fSigma;
	CVI_FLOAT sum_kernel = 0.0;
	CVI_FLOAT fMax = 0.0;

	for (int x = 0; x < filter_size; x++) {
		wbuf_gs_kernel[x] = exp(-x * x / fSigma2);
		sum_kernel += wbuf_gs_kernel[x];
	}

	for (int n = 0; n < filter_size; n++) {
		wbuf_gs_kernel[n] /= sum_kernel;
		if (wbuf_gs_kernel[n] > fMax) {
			fMax = wbuf_gs_kernel[n];
		}
	}

	CVI_FLOAT Scale = (CVI_FLOAT)Wgt / fMax;

	for (int n = 0; n < filter_size; n++) {
		wbuf_gs_kernel[n] *= Scale;
		luma_weight_lut[n] = (int)(wbuf_gs_kernel[n] + 0.5);
	}

}
