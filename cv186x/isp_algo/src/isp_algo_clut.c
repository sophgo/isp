/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_clut.c
 * Description:
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>
#include <time.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>

#include <sys/time.h>
#include "isp_algo_clut.h"
#include "isp_algo_debug.h"
#include "isp_algo_utility.h"

#define MAX_CLUT_VALUE 1023
#define MAX_RGB_VALUE 4096
#define MAX_HUE_VALUE 5760
#define MAX_SATURATION_VALUE 4096
#define MAX_LIGHTNESS_VALUE 4096
#define SCALE_UNIT 50
#define KERNEL_SIZE 5

typedef struct {
	CVI_U16 r;
	CVI_U16 g;
	CVI_U16 b;
} RGB;

typedef struct {
	CVI_U16 h;
	CVI_U16 s;
	CVI_U16 l;
} HSL;

struct isp_algo_clut_runtime {
	pthread_t tid;
	CVI_BOOL thread_runing;
	CVI_BOOL algo_run;
	struct clut_param_in *pIn;
	struct clut_param_out *pOut;
};
struct isp_algo_clut_runtime *algo_clut_runtime[VI_MAX_PIPE_NUM];

static struct isp_algo_clut_runtime  **_get_algo_clut_runtime(VI_PIPE ViPipe);
static void rgb_to_hsl(RGB *rgb, HSL* hsl);
static void hsl_to_rgb(HSL *hsl, RGB *rgb);
static void isp_algo_clut_interpolate_by_hue(CVI_U16 hue, ISP_CLUT_HSL_ATTR_S *hsl_attr,
		CVI_S16 *angle, CVI_U16 *s_scale, CVI_U16 *l_scale);
static void isp_algo_clut_interpolate_by_sat(CVI_U16 sat, ISP_CLUT_HSL_ATTR_S *hsl_attr, CVI_U16 *scale);
static void cal_gaussian_kernel(CVI_U16 *wgt, CVI_FLOAT sigma);
static void clut_denoise(CVI_U16 *clut_in, CVI_U16 *clut_out, CVI_U16 *wgt);
static void hsl_by_hue(HSL* in, HSL *out, CVI_S16 angle, CVI_U16 s_scale, CVI_U16 l_scale);
static void saturation_by_saturation(HSL* in, HSL *out, CVI_U16 scale);
static CVI_S32 isp_algo_clut_update(CVI_U32 ViPipe);
static void *isp_algo_clut_thread(void *param);


static void rgb_to_hsl(RGB *rgb, HSL* hsl)
{
	CVI_S32 h, s, l;
	CVI_S32 segment, shift;

	CVI_S32 r = rgb->r;
	CVI_S32 g = rgb->g;
	CVI_S32 b = rgb->b;

	CVI_S32 cmax = MAX3(r, g, b);
	CVI_S32 cmin = MIN3(r, g, b);
	CVI_S32 delta = cmax - cmin;

	l = (cmax + cmin) / 2;

	if (delta == 0) {
		h = 0;
		s = 0;
	} else {
		if (l < (MAX_LIGHTNESS_VALUE >> 1)) {
			s = delta * MAX_SATURATION_VALUE / (cmax + cmin);
		} else {
			s = delta * MAX_SATURATION_VALUE / (2 * MAX_SATURATION_VALUE - cmax - cmin);
		}

		if (cmax == r) {
			segment = (g - b) * (MAX_HUE_VALUE / 6) / delta;
			shift = 0;
		} else if (cmax == g) {
			segment = (b - r) * (MAX_HUE_VALUE / 6) / delta;
			shift = MAX_HUE_VALUE / 3;
		} else {
			segment = (r - g) * (MAX_HUE_VALUE / 6) / delta;
			shift = 2 * MAX_HUE_VALUE / 3;
		}

		h = segment + shift;
		if (h < 0) {
			h += MAX_HUE_VALUE;
		}
	}

	hsl->h = LIMIT_RANGE(h, 0, MAX_HUE_VALUE);
	hsl->s = LIMIT_RANGE(s, 0, MAX_SATURATION_VALUE);
	hsl->l = LIMIT_RANGE(l, 0, MAX_LIGHTNESS_VALUE);
}

static void hsl_to_rgb(HSL *hsl, RGB *rgb)
{
	CVI_S32 r, g, b;

	CVI_S32 c = (MAX_LIGHTNESS_VALUE - abs(2 * hsl->l - MAX_LIGHTNESS_VALUE)) * hsl->s / MAX_SATURATION_VALUE;
	CVI_S32 h_segment = (hsl->h * 6) / MAX_HUE_VALUE;
	CVI_S32 x = c * (1024 - abs(((hsl->h * 6 * 1024 / MAX_HUE_VALUE) % 2048) - 1024)) / 1024;

	CVI_S32 m = hsl->l - c / 2;
	CVI_S32 r_prime, g_prime, b_prime;

	switch (h_segment) {
	case 0:
		r_prime = c;
		g_prime = x;
		b_prime = 0;
		break;
	case 1:
		r_prime = x;
		g_prime = c;
		b_prime = 0;
		break;
	case 2:
		r_prime = 0;
		g_prime = c;
		b_prime = x;
		break;
	case 3:
		r_prime = 0;
		g_prime = x;
		b_prime = c;
		break;
	case 4:
		r_prime = x;
		g_prime = 0;
		b_prime = c;
		break;
	case 5:
		r_prime = c;
		g_prime = 0;
		b_prime = x;
		break;
	default:
		r_prime = 0;
		g_prime = 0;
		b_prime = 0;
		break;
	}

	r = (r_prime + m) * MAX_RGB_VALUE / MAX_LIGHTNESS_VALUE;
	g = (g_prime + m) * MAX_RGB_VALUE / MAX_LIGHTNESS_VALUE;
	b = (b_prime + m) * MAX_RGB_VALUE / MAX_LIGHTNESS_VALUE;

	rgb->r = LIMIT_RANGE(r, 0, MAX_RGB_VALUE);
	rgb->g = LIMIT_RANGE(g, 0, MAX_RGB_VALUE);
	rgb->b = LIMIT_RANGE(b, 0, MAX_RGB_VALUE);
}

static void isp_algo_clut_interpolate_by_hue(CVI_U16 hue, ISP_CLUT_HSL_ATTR_S *hsl_attr,
		CVI_S16 *angle, CVI_U16 *s_scale, CVI_U16 *l_scale)
{
	CVI_FLOAT x = 0, x0 = 0, y0 = 0, y1 = 0;
	CVI_FLOAT stride = (CVI_FLOAT)MAX_HUE_VALUE / (ISP_CLUT_HUE_LENGTH - 1);

	CVI_U16 idx0 = hue / stride;
	CVI_U16 idx1;

	if  (idx0 >= ISP_CLUT_HUE_LENGTH - 1) {
		idx1 = idx0 = ISP_CLUT_HUE_LENGTH - 1;
	} else {
		idx1 = idx0 + 1;
	}

	if (idx0 == idx1) {
		*angle = hsl_attr->HByH[idx0] * 16;
		*s_scale = hsl_attr->SByH[idx0];
		*l_scale = hsl_attr->LByH[idx0];
	} else {
		x = hue;
		x0 = idx0 * stride;

		y0 = hsl_attr->HByH[idx0];
		y1 = hsl_attr->HByH[idx1];
		*angle = ((y1 - y0) * (x - x0) / stride + y0) * 16 + 0.5;
		y0 = hsl_attr->SByH[idx0];
		y1 = hsl_attr->SByH[idx1];
		*s_scale = (y1 - y0) * (x - x0) / stride + y0 + 0.5;
		y0 = hsl_attr->LByH[idx0];
		y1 = hsl_attr->LByH[idx1];
		*l_scale = (y1 - y0) * (x - x0) / stride + y0 + 0.5;
	}
}

static void isp_algo_clut_interpolate_by_sat(CVI_U16 sat, ISP_CLUT_HSL_ATTR_S *hsl_attr, CVI_U16 *scale)
{
	CVI_FLOAT x = 0, x0 = 0, y0 = 0, y1 = 0;
	CVI_FLOAT stride = (CVI_FLOAT)MAX_SATURATION_VALUE / (ISP_CLUT_SAT_LENGTH - 1);

	CVI_U16 idx0 = sat / stride;
	CVI_U16 idx1;

	if  (idx0 >= ISP_CLUT_SAT_LENGTH - 1) {
		idx1 = idx0 = ISP_CLUT_SAT_LENGTH - 1;
	} else {
		idx1 = idx0 + 1;
	}

	if (idx0 == idx1) {
		*scale = hsl_attr->SByS[idx0];
	} else {
		x = sat;
		x0 = idx0 * stride;
		y0 = hsl_attr->SByS[idx0];
		y1 = hsl_attr->SByS[idx1];
		*scale = (y1 - y0) * (x - x0) / stride + y0 + 0.5;
	}
}

static void cal_gaussian_kernel(CVI_U16 *wgt, CVI_FLOAT sigma)
{
	CVI_S32 center = KERNEL_SIZE / 2;
	CVI_DOUBLE sum = 0.0, distance = 0;
	CVI_DOUBLE kernel[KERNEL_SIZE];

	for (CVI_S32 i = 0; i < KERNEL_SIZE; i++) {
		distance = (CVI_DOUBLE)(i - center);
		kernel[i] = (CVI_DOUBLE)exp(-distance * distance / (2 * sigma * sigma));
		sum += kernel[i];
	}

	for (CVI_S32 i = 0; i < KERNEL_SIZE; i++) {
		kernel[i] /= sum;
		wgt[i] = (CVI_U16)(kernel[i] * 4096);
	}
}

static void clut_denoise(CVI_U16 *clut_in, CVI_U16 *clut_out, CVI_U16 *wgt)
{
	CVI_S32 radius = KERNEL_SIZE / 2;
	CVI_S32 weight_sum = 0, sum = 0, idx = 0;
	CVI_U16 clut_x[ISP_CLUT_LUT_LENGTH], clut_y[ISP_CLUT_LUT_LENGTH];

	for (CVI_S32 i = 0; i < 17; i++) {
		for (CVI_S32 j = 0; j < 17; j++) {
			for (CVI_S32 k = 0; k < 17; k++) {
				weight_sum = 0;
				sum = 0;
				for (CVI_S32 di = -radius; di <= radius; di++) {
					if (i + di < 0 || i + di >= 17) {
						continue;
					}
					idx = (i + di) * 17 * 17 + j * 17 + k;
					weight_sum += wgt[di + radius];
					sum += wgt[di + radius] * clut_in[idx];
				}
				clut_x[i * 17 * 17 + j * 17 + k] = sum / weight_sum;
			}
		}
	}

	for (CVI_S32 i = 0; i < 17; i++) {
		for (CVI_S32 j = 0; j < 17; j++) {
			for (CVI_S32 k = 0; k < 17; k++) {
				weight_sum = 0;
				sum = 0;
				for (CVI_S32 dj = -radius; dj <= radius; dj++) {
					if (j + dj < 0 || j + dj >= 17) {
						continue;
					}
					idx = i * 17 * 17 + (j + dj) * 17 + k;
					weight_sum += wgt[dj + radius];
					sum += wgt[dj + radius] * clut_x[idx];
				}
				clut_y[i * 17 * 17 + j * 17 + k] = sum / weight_sum;
			}
		}
	}

	for (CVI_S32 i = 0; i < 17; i++) {
		for (CVI_S32 j = 0; j < 17; j++) {
			for (CVI_S32 k = 0; k < 17; k++) {
				weight_sum = 0;
				sum = 0;
				for (CVI_S32 dk = -radius; dk <= radius; dk++) {
					if (k + dk < 0 || k + dk >= 17) {
						continue;
					}
					idx = i * 17 * 17 + j * 17 + k + dk;
					weight_sum += wgt[dk + radius];
					sum += wgt[dk + radius] * clut_y[idx];
				}
				clut_out[i * 17 * 17 + j * 17 + k] = sum / weight_sum;
			}
		}
	}
}

static void hsl_by_hue(HSL* in, HSL *out, CVI_S16 angle, CVI_U16 s_scale, CVI_U16 l_scale)
{
	CVI_S32 h = in->h + angle;

	if (h >= MAX_HUE_VALUE) {
		h -= MAX_HUE_VALUE;
	} else if (h < 0) {
		h += MAX_HUE_VALUE;
	}
	out->h = LIMIT_RANGE(h, 0, MAX_HUE_VALUE);

	out->s = in->s * s_scale / SCALE_UNIT;
	out->s = LIMIT_RANGE(out->s, 0, MAX_SATURATION_VALUE);

	out->l = in->l * l_scale / SCALE_UNIT;
	out->l = LIMIT_RANGE(out->l, 0, MAX_LIGHTNESS_VALUE);
}

static void saturation_by_saturation(HSL* in, HSL *out, CVI_U16 scale)
{
	out->s = in->s * scale / SCALE_UNIT;
	out->s = LIMIT_RANGE(out->s, 0, MAX_SATURATION_VALUE);
}

static CVI_S32 isp_algo_clut_update(CVI_U32 ViPipe)
{
	struct isp_algo_clut_runtime **runtime_ptr = _get_algo_clut_runtime(ViPipe);
	struct isp_algo_clut_runtime *runtime = *runtime_ptr;

	ISP_ALGO_CHECK_POINTER(runtime);

	struct clut_param_in *clut_param_in = runtime->pIn;
	struct clut_param_out *clut_param_out = runtime->pOut;

	CVI_U16 *r_in = ISP_PTR_CAST_U16(clut_param_in->ClutR);
	CVI_U16 *g_in = ISP_PTR_CAST_U16(clut_param_in->ClutG);
	CVI_U16 *b_in = ISP_PTR_CAST_U16(clut_param_in->ClutB);
	CVI_U16 *r_out = clut_param_out->ClutR;
	CVI_U16 *g_out = clut_param_out->ClutG;
	CVI_U16 *b_out = clut_param_out->ClutB;
	CVI_U16 *r_tmp = (CVI_U16 *)calloc(1, ISP_CLUT_LUT_LENGTH * sizeof(CVI_U16));
	CVI_U16 *g_tmp = (CVI_U16 *)calloc(1, ISP_CLUT_LUT_LENGTH * sizeof(CVI_U16));
	CVI_U16 *b_tmp = (CVI_U16 *)calloc(1, ISP_CLUT_LUT_LENGTH * sizeof(CVI_U16));
	ISP_CLUT_HSL_ATTR_S *hsl_attr = (ISP_CLUT_HSL_ATTR_S *)ISP_PTR_CAST_VOID(clut_param_in->hsl_attr);
	CVI_U16 wgt[KERNEL_SIZE] = {0, 0, 4095, 0, 0};

	for (CVI_U32 i = 0 ; i < ISP_CLUT_LUT_LENGTH ; i++) {
		RGB rgb;
		HSL hsl, hslOut;
		CVI_S16 angle;
		CVI_U16 s_scale, l_scale;

		rgb.r = r_in[i] * MAX_RGB_VALUE / MAX_CLUT_VALUE;
		rgb.g = g_in[i] * MAX_RGB_VALUE / MAX_CLUT_VALUE;
		rgb.b = b_in[i] * MAX_RGB_VALUE / MAX_CLUT_VALUE;

		rgb_to_hsl(&rgb, &hsl);
		isp_algo_clut_interpolate_by_hue(hsl.h, hsl_attr, &angle, &s_scale, &l_scale);
		hsl_by_hue(&hsl, &hslOut, angle, s_scale, l_scale);
		isp_algo_clut_interpolate_by_sat(hslOut.s, hsl_attr, &s_scale);

		hsl = hslOut;
		saturation_by_saturation(&hsl, &hslOut, s_scale);
		hsl_to_rgb(&hslOut, &rgb);

		r_tmp[i] = rgb.r;
		g_tmp[i] = rgb.g;
		b_tmp[i] = rgb.b;
	}

	if (hsl_attr->Sigma > 0) {
		cal_gaussian_kernel(wgt, hsl_attr->Sigma);
	}

	clut_denoise(r_tmp, r_out, wgt);
	clut_denoise(g_tmp, g_out, wgt);
	clut_denoise(b_tmp, b_out, wgt);

	for (CVI_U32 i = 0; i < 17; i++) {
		CVI_U32 idx = i * 17 * 17 + i * 17 + i;
		CVI_U32 sum = r_out[idx] + g_out[idx] + b_out[idx];

		r_out[idx] = sum / 3;
		g_out[idx] = sum / 3;
		b_out[idx] = sum / 3;
	}

	for (CVI_U32 i = 0; i < ISP_CLUT_LUT_LENGTH; i++) {
		r_out[i] = LIMIT_RANGE(r_out[i] * MAX_CLUT_VALUE / MAX_RGB_VALUE, 0, MAX_CLUT_VALUE);
		g_out[i] = LIMIT_RANGE(g_out[i] * MAX_CLUT_VALUE / MAX_RGB_VALUE, 0, MAX_CLUT_VALUE);
		b_out[i] = LIMIT_RANGE(b_out[i] * MAX_CLUT_VALUE / MAX_RGB_VALUE, 0, MAX_CLUT_VALUE);
	}

	ISP_ALGO_RELEASE_MEMORY(r_tmp);
	ISP_ALGO_RELEASE_MEMORY(g_tmp);
	ISP_ALGO_RELEASE_MEMORY(b_tmp);
	clut_param_out->isUpdated = CVI_TRUE;

	return 0;
}

static void *isp_algo_clut_thread(void *param)
{
	CVI_U64 ViPipe = (CVI_U64)param;
	struct isp_algo_clut_runtime **runtime_ptr = _get_algo_clut_runtime(ViPipe);
	struct isp_algo_clut_runtime *runtime = *runtime_ptr;

	runtime->thread_runing = CVI_TRUE;
	while (runtime->thread_runing) {
		if(runtime->algo_run) {
			runtime->algo_run = CVI_FALSE;
			runtime->pOut->isUpdated = CVI_FALSE;
			isp_algo_clut_update(ViPipe);
		}

		usleep(100 * 1000);
	}

	return NULL;
}

CVI_S32 isp_algo_clut_main(CVI_U32 ViPipe, struct clut_param_in *clut_param_in, struct clut_param_out *clut_param_out)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_algo_clut_runtime **runtime_ptr = _get_algo_clut_runtime(ViPipe);
	struct isp_algo_clut_runtime *runtime = *runtime_ptr;

	ISP_ALGO_CHECK_POINTER(runtime);

	runtime->pIn = clut_param_in;
	runtime->pOut = clut_param_out;
	runtime->algo_run = CVI_TRUE;
	if (runtime->tid == 0) {
		pthread_create(&runtime->tid, NULL, isp_algo_clut_thread, (void *)((CVI_U64)ViPipe));
	}

	return ret;
}

CVI_S32 isp_algo_clut_init(CVI_U32 ViPipe)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_algo_clut_runtime **runtime_ptr = _get_algo_clut_runtime(ViPipe);

	ISP_ALGO_CREATE_RUNTIME(*runtime_ptr, struct isp_algo_clut_runtime);

	struct isp_algo_clut_runtime *runtime = *runtime_ptr;

	memset(runtime, 0, sizeof(struct isp_algo_clut_runtime));
	runtime->tid = 0;
	runtime->thread_runing = CVI_FALSE;
	runtime->algo_run = CVI_FALSE;

	return ret;
}

CVI_S32 isp_algo_clut_uninit(CVI_U32 ViPipe)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_algo_clut_runtime **runtime_ptr = _get_algo_clut_runtime(ViPipe);
	struct isp_algo_clut_runtime *runtime = *runtime_ptr;

	if (runtime->tid != 0) {
		runtime->thread_runing = CVI_FALSE;
		pthread_join(runtime->tid, NULL);
		runtime->tid = 0;
	}

	ISP_ALGO_RELEASE_MEMORY(*runtime_ptr);

	return ret;
}

static struct isp_algo_clut_runtime  **_get_algo_clut_runtime(VI_PIPE ViPipe)
{
	CVI_BOOL isVipipeValid = ((ViPipe >= 0) && (ViPipe < VI_MAX_PIPE_NUM));

	if (!isVipipeValid) {
		// ISP_DEBUG(LOG_ERR, "Wrong ViPipe(%d)\n", ViPipe);
		return NULL;
	}

	return &algo_clut_runtime[ViPipe];
}
