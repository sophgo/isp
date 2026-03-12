/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_dci.c
 * Description:
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "limits.h"

#include "isp_algo_teaisp_drc.h"
#include "isp_algo_debug.h"
#include "isp_algo_utility.h"


static int auto_drc(CVI_U32 pHist[TEAISP_DRC_BINS_NUM], float avg_ratio, int dark_th, float dark_compensate_ratio,
				int luma_th_left, int luma_th_right, float luma_compensate_ratio)
{

	int total_num = 0;

	for (int index = 0; index < TEAISP_DRC_BINS_NUM; index++) {
		total_num += pHist[index];
	}
	float avg_num = total_num / (float)TEAISP_DRC_BINS_NUM;
	float pixel_th_left = avg_num * avg_ratio;
	float pixel_th_right = avg_num * avg_ratio;

	int dark_pixel = 0, bright_pixel_y = 0;
	int ymin = 256, ymax = 0;
	int cnt_left = 0, cnt_right = 0;

	for (int index = 0; index < TEAISP_DRC_BINS_NUM; index++) {
		if (index < dark_th && ymin != 256) {
			dark_pixel += pHist[index];
		}

		if (index > luma_th_left && index < luma_th_right) {
			bright_pixel_y += pHist[index];
		}

		if (ymin == 256) {
			cnt_left += pHist[index];
			if (cnt_left > pixel_th_left)
				ymin = index;
		}

		if (ymax == 0) {
			cnt_right += pHist[TEAISP_DRC_BINS_NUM-index-1];
			if (cnt_right > pixel_th_right)
				ymax = (TEAISP_DRC_BINS_NUM-index-1);
		}
	}

	float dr_level, avg_num_new;

	if (ymax > ymin) {
		dr_level = (ymax - ymin) * 100.0 / (float)TEAISP_DRC_BINS_NUM;
		avg_num_new = (total_num - cnt_right - cnt_left)/(ymax - ymin + 1);
	} else {
		dr_level = 0;
		avg_num_new = 1;
	}

	float dr_level_new = dr_level;
	float compensate_dark, compensate_bright;

	if (dark_th > ymin) {
		compensate_dark = dark_pixel / (avg_num_new * (dark_th - ymin));
		compensate_bright = bright_pixel_y / (avg_num_new * (luma_th_right - luma_th_left));
	} else {
		compensate_dark = 0;
		compensate_bright = 0;
	}
	if (compensate_dark > 1) {
		dr_level_new += dark_compensate_ratio * compensate_dark;
	}
	dr_level_new += luma_compensate_ratio * compensate_bright;

	// printf("%3.5f, %d, %d, %3.5f, %3.5f\n", compensate_dark, ymax, ymin, dr_level, dr_level_new);
	return (int)dr_level_new;

}

CVI_S32 isp_algo_teaisp_drc_main(struct teaisp_drc_param_in *teaisp_drc_param_in, struct teaisp_drc_param_out *teaisp_drc_param_out)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	if (teaisp_drc_param_in->enOpType == OP_TYPE_MANUAL) {
		teaisp_drc_param_out->BrightThreshold = teaisp_drc_param_in->BrightThreshold;
		teaisp_drc_param_out->SatuStrength = teaisp_drc_param_in->SatuStrength;
		return ret;
	}

	CVI_U32 * pHist = ISP_PTR_CAST_U32(teaisp_drc_param_in->pHist);

	float avg_ratio = 1.0;
	int dark_th = 64;
	float dark_compensate_ratio = teaisp_drc_param_in->DarkCompensateRatio;
	int luma_th_left = 128;
	int luma_th_right = 192;
	float luma_compensate_ratio = 0.0;
	int StrengthMin = teaisp_drc_param_in->StrengthMin;
	int StrengthMax = teaisp_drc_param_in->StrengthMax;
	int dr_level_new = auto_drc(pHist, avg_ratio, dark_th, dark_compensate_ratio, luma_th_left, luma_th_right, luma_compensate_ratio);

	if (dr_level_new <= StrengthMin) {
		teaisp_drc_param_out->BrightThreshold = 5;
		teaisp_drc_param_out->SatuStrength = 50;
	} else if (dr_level_new >= StrengthMax) {
		teaisp_drc_param_out->BrightThreshold = 4;
		teaisp_drc_param_out->SatuStrength = 0;
	} else {
		teaisp_drc_param_out->SatuStrength = 50 - 50 * (dr_level_new - StrengthMin) / (StrengthMax - StrengthMin);	// [50 - 0]
	}

	// printf("StrengthMin:%d, StrengthMax:%d, BrightThreshold:%d, SatuStrength:%d\n", StrengthMin, StrengthMax,
	//			teaisp_drc_param_out->BrightThreshold, teaisp_drc_param_out->SatuStrength);
	return ret;
}

CVI_S32 isp_algo_teaisp_drc_init(void)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	return ret;
}

CVI_S32 isp_algo_teaisp_drc_uninit(void)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	return ret;
}

