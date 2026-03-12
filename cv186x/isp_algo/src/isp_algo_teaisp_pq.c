/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_teaisp_pq.c
 * Description:
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "limits.h"

#include "isp_algo_teaisp_pq.h"
#include "isp_algo_utility.h"


float auto_drc(CVI_U32 *pHist, float avg_ratio, int dark_th, float dark_compensate_ratio,
				int luma_th_left, int luma_th_right, float luma_compensate_ratio)
{

	int total_num = 0;

	for (int index = 0; index < MAX_HIST_BINS; index++) {
		total_num += pHist[index];
	}
	float avg_num = total_num / (float)MAX_HIST_BINS;
	float pixel_th_left = avg_num * avg_ratio;
	float pixel_th_right = avg_num * avg_ratio;

	int dark_pixel = 0, bright_pixel_y = 0;
	int ymin = 256, ymax = 0;
	int cnt_left = 0, cnt_right = 0;

	for (int index = 0; index < MAX_HIST_BINS; index++) {
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
			cnt_right += pHist[MAX_HIST_BINS-index-1];
			if (cnt_right > pixel_th_right)
				ymax = (MAX_HIST_BINS-index-1);
		}
	}

	float dr_level, avg_num_new;

	if (ymax > ymin) {
		dr_level = (ymax - ymin) * 100.0 / (float)MAX_HIST_BINS;
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
	return dr_level_new;
}

