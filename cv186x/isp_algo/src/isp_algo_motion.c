/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_motion.c
 * Description:
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "isp_algo_motion.h"
#include "isp_algo_debug.h"

#define MAX_MOTION_LEVEL	(255)

static CVI_S32 motion_level_calculate(
	struct motion_param_in *param_in, struct motion_param_out *param_out)
{
	CVI_U8 grid_w = (1 << param_in->gridWidth);
	CVI_U8 grid_h = (1 << param_in->gridHeight);
	CVI_U16 grid_w_num = param_in->imageWidth / grid_w;
	CVI_U16 grid_h_num = param_in->imageHeight / grid_h;
	CVI_U8 motion_value_l;
	CVI_U16 x, y;
	CVI_U32 motion_cnt;
	CVI_U32 total_grid = grid_w_num * grid_h_num;
	CVI_U8 u8MotionTh = param_in->motionThreshold;

	motion_cnt = 0;
	if (param_in->motionStsBufSize >= (total_grid * sizeof(CVI_U16))) {
		CVI_U32 yIndex = 0, idx = 0;
		CVI_U32 offset = grid_w_num << 1;
		// TODO. Performance still need optimize.
		for (y = 0; y < grid_h_num; y = y + 1) {
			for (x = 0; x < grid_w_num; x = x + 1) {
				CVI_U16 *motionValue = (CVI_U16 *)
					&(ISP_PTR_CAST_U8(param_in->motionMapAddr)[idx]);

				// In 182x only LSB is use for motion level
				motion_value_l = (*motionValue & 0xFF);
				motion_cnt = (motion_value_l > u8MotionTh) ? motion_cnt + 1 : motion_cnt;

				idx = idx + 2;
			}
			yIndex += offset;
			idx = yIndex;
		}
	}

#if 0
	{
		// motion mode 2. Current not use. Still need optimize.
		CVI_U8 sec_num = 4;
		CVI_U16 zx, zy, z;
		CVI_U32 motion_cnt_max = 0;
		CVI_U32 gridHSize = grid_h_num / sec_num;
		CVI_U32 gridWSize = grid_w_num / sec_num;

		for (z = 0; z < (sec_num * sec_num); z = z + 1) {
			zy = z / sec_num;
			zx = z % sec_num;
			for (y = 0; y < gridHSize; y = y + 1) {
				for (x = 0; x < gridWSize; x = x + 1) {
					CVI_U32 index = ((zy*gridHSize + y) * grid_w_num + zx * gridWSize + x) << 1;
					CVI_U16 *motionValue = &(param_in->motionMapAddr[index]);

					//motion_value_l = param_in->motionMapAddr[index];
					//motion_value_s = param_in->motionMapAddr[index + 1];
					motion_value_l = (*motionValue & 0xFF);
					motion_value_s = (*motionValue & 0xFF00) >> 8;
					motion_max = motion_value_l > motion_value_s ? motion_value_l : motion_value_s;
					motion_cnt = motion_max > param_in->motionThreshold
						? motion_cnt + 1 : motion_cnt;
				}
			}
			motion_cnt_max = motion_cnt > motion_cnt_max ? motion_cnt : motion_cnt_max;
			motion_cnt = 0;
		}
		motion_cnt = motion_cnt_max;
	//	printf("motion_cnt %d motion_cnt_max; %d\n", motion_cnt, motion_cnt_max);
		total_grid = gridHSize * gridWSize;
	}
#endif

	param_out->frameCnt = param_in->frameCnt;
	param_out->motionLevel = (motion_cnt * MAX_MOTION_LEVEL) / total_grid;

	return 0;
}

CVI_S32 isp_algo_motion_main(struct motion_param_in *motion_param_in, struct motion_param_out *motion_param_out)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	ret = motion_level_calculate(motion_param_in, motion_param_out);

	return ret;
}

CVI_S32 isp_algo_motion_init(void)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	return ret;
}

CVI_S32 isp_algo_motion_uninit(void)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	return ret;
}
