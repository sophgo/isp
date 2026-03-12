/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_mlsc.c
 * Description:
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "isp_algo_mlsc.h"
#include "isp_algo_debug.h"
#include "isp_algo_utility.h"
#ifdef __ARM_NEON__
#include "arm_neon.h"
#endif

#define LSC_GAIN_STRENGTH       (12)
#define MAX_LSC_TABLE_NUM 7
#define LSC_GAIN_MAX 4095
#define MAX_CT 11000
#define MIN_CT 2300

static int lsc_chroma_gain_table_fusion(int current_color_temp, unsigned short *color_temp, int LSC_table_num,
	unsigned short **r_gain_in, unsigned short **g_gain_in, unsigned short **b_gain_in,
	int NumKnotX, int NumKnotY, unsigned short *r_gain_out, unsigned short *g_gain_out, unsigned short *b_gain_out,
	unsigned int *idx1, unsigned int *idx2);
static int lsc_luma_chroma_gain_to_reg_table(
	int NumKnotX, int NumKnotY, uint16_t strength, float *mlsc_compensate_gain,
	unsigned short *r_gain, unsigned short *g_gain, unsigned short *b_gain);

CVI_S32 isp_algo_mlsc_main(struct mlsc_param_in *mlsc_param_in, struct mlsc_param_out *mlsc_param_out)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	lsc_chroma_gain_table_fusion(
		mlsc_param_in->color_temperature,
		mlsc_param_in->color_temp_tbl,
		mlsc_param_in->color_temp_size,
		(unsigned short **) mlsc_param_in->mlsc_lut_r,
		(unsigned short **) mlsc_param_in->mlsc_lut_g,
		(unsigned short **) mlsc_param_in->mlsc_lut_b,
		mlsc_param_in->grid_col, mlsc_param_in->grid_row,
		ISP_PTR_CAST_U16(mlsc_param_out->mlsc_lut_r),
		ISP_PTR_CAST_U16(mlsc_param_out->mlsc_lut_g),
		ISP_PTR_CAST_U16(mlsc_param_out->mlsc_lut_b),
		&(mlsc_param_out->temp_lower_idx), &(mlsc_param_out->temp_upper_idx));

	lsc_luma_chroma_gain_to_reg_table(mlsc_param_in->grid_col, mlsc_param_in->grid_row,
		mlsc_param_in->strength, &(mlsc_param_out->mlsc_compensate_gain),
		ISP_PTR_CAST_U16(mlsc_param_out->mlsc_lut_r),
		ISP_PTR_CAST_U16(mlsc_param_out->mlsc_lut_g),
		ISP_PTR_CAST_U16(mlsc_param_out->mlsc_lut_b));

	return ret;
}

CVI_S32 isp_algo_mlsc_init(void)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	return ret;
}

CVI_S32 isp_algo_mlsc_uninit(void)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	return ret;
}

static int lsc_chroma_gain_table_fusion(int current_color_temp, unsigned short *color_temp, int LSC_table_num,
	unsigned short **r_gain_in, unsigned short **g_gain_in, unsigned short **b_gain_in,
	int NumKnotX, int NumKnotY, unsigned short *r_gain_out, unsigned short *g_gain_out, unsigned short *b_gain_out,
	unsigned int *idx1, unsigned int *idx2)
{
	int rtn = 0;

	int table_0 = 0, table_1 = 0;
	float ratio_0 = 0, ratio_1 = 0;
	int idx;
	// 0. re-order color temp from low to high
	int reorder_color_temp[MAX_LSC_TABLE_NUM];
	unsigned short *reorder_r_gain_in[MAX_LSC_TABLE_NUM];
	unsigned short *reorder_g_gain_in[MAX_LSC_TABLE_NUM];
	unsigned short *reorder_b_gain_in[MAX_LSC_TABLE_NUM];

	for (int i = 0; i < LSC_table_num; i++) {
		reorder_color_temp[i] = color_temp[i];
		reorder_r_gain_in[i] = ISP_PTR_CAST_U16((((ISP_U16_PTR *)r_gain_in)[i]));
		reorder_g_gain_in[i] = ISP_PTR_CAST_U16((((ISP_U16_PTR *)g_gain_in)[i]));
		reorder_b_gain_in[i] = ISP_PTR_CAST_U16((((ISP_U16_PTR *)b_gain_in)[i]));
	}

	for (int i = 0; i < LSC_table_num - 1; i++) {
		for (int j = 0; j < LSC_table_num - i - 1; j++) {
			if (reorder_color_temp[j] > reorder_color_temp[j + 1]) {
				int tmp = reorder_color_temp[j + 1];

				reorder_color_temp[j + 1] = reorder_color_temp[j];
				reorder_color_temp[j] = tmp;
				unsigned short *tmp_addr = reorder_r_gain_in[j + 1];

				reorder_r_gain_in[j + 1] = reorder_r_gain_in[j];
				reorder_r_gain_in[j] = tmp_addr;
				tmp_addr = reorder_g_gain_in[j + 1];
				reorder_g_gain_in[j + 1] = reorder_g_gain_in[j];
				reorder_g_gain_in[j] = tmp_addr;
				tmp_addr = reorder_b_gain_in[j + 1];
				reorder_b_gain_in[j + 1] = reorder_b_gain_in[j];
				reorder_b_gain_in[j] = tmp_addr;
			}
		}
	}

	// 1. single table case
	if (LSC_table_num == 1) {
		for (int j = 0; j < NumKnotY; j++) {
			for (int i = 0; i < NumKnotX; i++) {
				idx = j * NumKnotX + i;
				r_gain_out[idx] = reorder_r_gain_in[0][idx];
				g_gain_out[idx] = reorder_g_gain_in[0][idx];
				b_gain_out[idx] = reorder_b_gain_in[0][idx];
			}
		}
	}
	// 2. multiple table case
	else {
		// 1.0 choose g table
		// fast choose of g table id
		// compact choose of g table id
		int pos_0 = 1 * NumKnotX + (NumKnotX / 2);
		int pos_1 = (NumKnotY / 2) * NumKnotX + 1;
		int pos_2 = (NumKnotY / 2) * NumKnotX + (NumKnotX - 2);
		int pos_3 = (NumKnotY - 2) * NumKnotX + (NumKnotX / 2);
		int min_gain_sum = LSC_GAIN_MAX * 4;
		int gain_sum = 0;

		for (int k = 0; k < LSC_table_num; k++) {
			gain_sum = reorder_g_gain_in[k][pos_0] + reorder_g_gain_in[k][pos_1] +
				reorder_g_gain_in[k][pos_2] + reorder_g_gain_in[k][pos_3];
			if (gain_sum <= min_gain_sum) {
				min_gain_sum = gain_sum;
			}
		}
		// 1.1 table indexing
		if (current_color_temp <= reorder_color_temp[0]) {
			*idx1 = *idx2 = 0;
			for (int j = 0; j < NumKnotY; j++) {
				for (int i = 0; i < NumKnotX; i++) {
					idx = j * NumKnotX + i;
					r_gain_out[idx] = reorder_r_gain_in[0][idx];

					g_gain_out[idx] = reorder_g_gain_in[0][idx];

					b_gain_out[idx] = reorder_b_gain_in[0][idx];
				}
			}
		} else if (current_color_temp >= reorder_color_temp[LSC_table_num - 1]) {
			*idx1 = *idx2 = LSC_table_num - 1;
			for (int j = 0; j < NumKnotY; j++) {
				for (int i = 0; i < NumKnotX; i++) {
					idx = j * NumKnotX + i;
					r_gain_out[idx] = reorder_r_gain_in[LSC_table_num - 1][idx];
					g_gain_out[idx] = reorder_g_gain_in[LSC_table_num - 1][idx];
					b_gain_out[idx] = reorder_b_gain_in[LSC_table_num - 1][idx];
				}
			}
		} else {
			for (int i = 0; i < LSC_table_num - 1; i++) {
				if (reorder_color_temp[i] <= current_color_temp &&
					current_color_temp < reorder_color_temp[i + 1]) {
					table_0 = i;
					table_1 = i + 1;
					ratio_1 = (float)(current_color_temp - reorder_color_temp[i]) /
						(float)(reorder_color_temp[i + 1] - reorder_color_temp[i]);
					ratio_0 = 1.0 - ratio_1;
					break;
				}
			}
			*idx1 = table_0;
			*idx2 = table_1;
			for (int j = 0; j < NumKnotY; j++) {
				for (int i = 0; i < NumKnotX; i++) {
					idx = j * NumKnotX + i;
					r_gain_out[idx] = reorder_r_gain_in[table_0][idx] * ratio_0 +
						reorder_r_gain_in[table_1][idx] * ratio_1;
					g_gain_out[idx] = reorder_g_gain_in[table_0][idx] * ratio_0 +
					 reorder_g_gain_in[table_1][idx] * ratio_1;
					g_gain_out[idx] = g_gain_out[idx];
					b_gain_out[idx] = reorder_b_gain_in[table_0][idx] * ratio_0 +
						reorder_b_gain_in[table_1][idx] * ratio_1;
				}
			}
		}
	}

	return rtn;
}

static int lsc_luma_chroma_gain_to_reg_table(
	int NumKnotX, int NumKnotY, uint16_t strength, float *mlsc_compensate_gain,
	unsigned short *r_gain, unsigned short *g_gain, unsigned short *b_gain)
{
	int rtn = 0;
	int idx;
	uint32_t _percent_0_ggain = (4095 - strength) * 512;
	int min_gain = 512;

	for (int j = 0; j < NumKnotY; j++) {
		int i = 0;
		#ifdef __ARM_NEON__
		uint16x4_t _16x4_min = vdup_n_u16(512);
		uint16x4_t _16x4_max = vdup_n_u16(LSC_GAIN_MAX);
		uint16x4_t _16x4_str = vdup_n_u16(strength);
		uint32x4_t _32x4_per = vdupq_n_u32(_percent_0_ggain + (1 << 11));
		uint32x4_t _32x4_rnd = vdupq_n_u32((1 << (LSC_GAIN_BASE-1)));

		for (; i < NumKnotX-8 ; i += 8) {
			idx = j * NumKnotX + i;

			uint16x4x2_t _16x4x2_ggain = vld2_u16(&(g_gain[idx]));

			uint32x4x2_t _32x4x2_val;

			for (int k = 0 ; k < 2 ; k++) {
				_32x4x2_val.val[k] = vmull_u16(_16x4x2_ggain.val[k], _16x4_str);
				_32x4x2_val.val[k] = vaddq_u32(_32x4x2_val.val[k], _32x4_per);
				_16x4x2_ggain.val[k] = vshrn_n_u32(_32x4x2_val.val[k], 12);
			}

			vst2_u16(&(g_gain[idx]), _16x4x2_ggain);

			uint16x4x2_t _16x4x2_rgain = vld2_u16(&(r_gain[idx]));

			for (int k = 0 ; k < 2 ; k++) {
				_32x4x2_val.val[k] = vmull_u16(_16x4x2_rgain.val[k], _16x4x2_ggain.val[k]);
				_32x4x2_val.val[k] = vaddq_u32(_32x4x2_val.val[k], _32x4_rnd);
				_16x4x2_rgain.val[k] = vshrn_n_u32(_32x4x2_val.val[k], LSC_GAIN_BASE);
				_16x4x2_rgain.val[k] = vmin_u16(_16x4x2_rgain.val[k], _16x4_max);
				_16x4_min = vmin_u16(_16x4_min, _16x4x2_rgain.val[k]);
			}

			vst2_u16(&(r_gain[idx]), _16x4x2_rgain);

			uint16x4x2_t _16x4x2_bgain = vld2_u16(&(b_gain[idx]));

			for (int k = 0 ; k < 2 ; k++) {
				_32x4x2_val.val[k] = vmull_u16(_16x4x2_bgain.val[k], _16x4x2_ggain.val[k]);
				_32x4x2_val.val[k] = vaddq_u32(_32x4x2_val.val[k], _32x4_rnd);
				_16x4x2_bgain.val[k] = vshrn_n_u32(_32x4x2_val.val[k], LSC_GAIN_BASE);
				_16x4x2_bgain.val[k] = vmin_u16(_16x4x2_bgain.val[k], _16x4_max);
				_16x4_min = vmin_u16(_16x4_min, _16x4x2_bgain.val[k]);
			}

			vst2_u16(&(b_gain[idx]), _16x4x2_bgain);
		}

#if 0
		uint16_t val = vminv_u16(_16x4_min);

		if (min_gain > val)
			min_gain = val;
#else
		uint16_t min_arr[4];

		min_arr[0] = vget_lane_u16(_16x4_min, 0);
		min_arr[1] = vget_lane_u16(_16x4_min, 1);
		min_arr[2] = vget_lane_u16(_16x4_min, 2);
		min_arr[3] = vget_lane_u16(_16x4_min, 3);

		for (int mi = 0 ; mi < 4 ; mi++) {
			uint16_t val = min_arr[mi];

			if (min_gain > val)
				min_gain = val;
		}
#endif
		#endif // __ARM_NEON__
		for (; i < NumKnotX; i++) {
			idx = j * NumKnotX + i;

			g_gain[idx] = ((g_gain[idx] * strength + _percent_0_ggain + (1 << 11)) >> 12);

			r_gain[idx] = MIN(LSC_GAIN_MAX,
				(r_gain[idx] * g_gain[idx] + (1 << (LSC_GAIN_BASE-1))) >> LSC_GAIN_BASE);
			if (r_gain[idx] < min_gain)
				min_gain = r_gain[idx];
			b_gain[idx] = MIN(LSC_GAIN_MAX,
				(b_gain[idx] * g_gain[idx] + (1 << (LSC_GAIN_BASE-1))) >> LSC_GAIN_BASE);
			if (b_gain[idx] < min_gain)
				min_gain = b_gain[idx];
		}
	}

	float comp_gain = 512.0 / MIN(512.0, MAX(256, min_gain));
	*mlsc_compensate_gain = comp_gain;

	if (comp_gain > 1.0) {
		for (int j = 0; j < NumKnotY; j++) {
			int i = 0;
			#ifdef __ARM_NEON__
			uint16x4_t _16x4_max = vdup_n_u16(LSC_GAIN_MAX);

			for (; i < NumKnotX-8 ; i += 8) {
				idx = j * NumKnotX + i;

				uint16x4x2_t _16x4x2_ggain = vld2_u16(&(g_gain[idx]));

				uint32x4x2_t _32x4x2_val;
				float32x4x2_t _f32x4x2_val;

				for (int k = 0 ; k < 2 ; k++) {
					_32x4x2_val.val[k] = vmovl_u16(_16x4x2_ggain.val[k]);
					_f32x4x2_val.val[k] = vcvtq_f32_u32(_32x4x2_val.val[k]);
					_f32x4x2_val.val[k] = vmulq_n_f32(_f32x4x2_val.val[k], comp_gain);
					_32x4x2_val.val[k] = vcvtq_u32_f32(_f32x4x2_val.val[k]);
					_16x4x2_ggain.val[k] = vmovn_u32(_32x4x2_val.val[k]);
					_16x4x2_ggain.val[k] = vmin_u16(_16x4x2_ggain.val[k], _16x4_max);
				}

				vst2_u16(&(g_gain[idx]), _16x4x2_ggain);

				uint16x4x2_t _16x4x2_rgain = vld2_u16(&(r_gain[idx]));

				for (int k = 0 ; k < 2 ; k++) {
					_32x4x2_val.val[k] = vmovl_u16(_16x4x2_rgain.val[k]);
					_f32x4x2_val.val[k] = vcvtq_f32_u32(_32x4x2_val.val[k]);
					_f32x4x2_val.val[k] = vmulq_n_f32(_f32x4x2_val.val[k], comp_gain);
					_32x4x2_val.val[k] = vcvtq_u32_f32(_f32x4x2_val.val[k]);
					_16x4x2_rgain.val[k] = vmovn_u32(_32x4x2_val.val[k]);
					_16x4x2_rgain.val[k] = vmin_u16(_16x4x2_rgain.val[k], _16x4_max);
				}

				vst2_u16(&(r_gain[idx]), _16x4x2_rgain);

				uint16x4x2_t _16x4x2_bgain = vld2_u16(&(b_gain[idx]));

				for (int k = 0 ; k < 2 ; k++) {
					_32x4x2_val.val[k] = vmovl_u16(_16x4x2_bgain.val[k]);
					_f32x4x2_val.val[k] = vcvtq_f32_u32(_32x4x2_val.val[k]);
					_f32x4x2_val.val[k] = vmulq_n_f32(_f32x4x2_val.val[k], comp_gain);
					_32x4x2_val.val[k] = vcvtq_u32_f32(_f32x4x2_val.val[k]);
					_16x4x2_bgain.val[k] = vmovn_u32(_32x4x2_val.val[k]);
					_16x4x2_bgain.val[k] = vmin_u16(_16x4x2_bgain.val[k], _16x4_max);
				}

				vst2_u16(&(b_gain[idx]), _16x4x2_bgain);
			}
			#endif // __ARM_NEON__
			for (; i < NumKnotX; i++) {
				idx = j * NumKnotX + i;
				g_gain[idx] = MIN2(LSC_GAIN_MAX, g_gain[idx] * comp_gain);
				r_gain[idx] = MIN2(LSC_GAIN_MAX, r_gain[idx] * comp_gain);
				b_gain[idx] = MIN2(LSC_GAIN_MAX, b_gain[idx] * comp_gain);
			}
		}
	}

	return rtn;
}

#if 0
int main(int argc, char **argv)
{
	//int current_color_temp = 4567;
	int current_color_temp = 6500;
	int LSC_table_color_temp[MAX_LSC_TABLE_NUM] = {6500, 2800, 6500};
	int LSC_table_num = 3;
	int NumKnotX = 37;
	int NumKnotY = 37;
	unsigned short **r_gain_in, **g_gain_in, **b_gain_in;
	unsigned short *r_gain_out, *g_gain_out, *b_gain_out;

	r_gain_in = new unsigned short * [LSC_table_num];
	g_gain_in = new unsigned short * [LSC_table_num];
	b_gain_in = new unsigned short * [LSC_table_num];
	for (int i = 0; i < LSC_table_num; i++) {
		r_gain_in[i] = new unsigned short [NumKnotX * NumKnotY];
		g_gain_in[i] = new unsigned short [NumKnotX * NumKnotY];
		b_gain_in[i] = new unsigned short [NumKnotX * NumKnotY];
	}
	r_gain_out = new unsigned short[NumKnotX * NumKnotY];
	g_gain_out = new unsigned short[NumKnotX * NumKnotY];
	b_gain_out = new unsigned short[NumKnotX * NumKnotY];

	lsc_gain_table_fusion(current_color_temp, LSC_table_color_temp, LSC_table_num
		, r_gain_in, g_gain_in, b_gain_in
		, NumKnotX, NumKnotY
		, r_gain_out, g_gain_out, b_gain_out);

	return 0;
}
#endif
