/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_drc.c
 * Description:
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#include "limits.h"

#include "pchip.h"

#include "isp_algo_drc.h"
#include "isp_algo_debug.h"
#include "isp_algo_utility.h"

#define LTM_D_CURVE_NODE_NUM			(257)
#define LTM_B_CURVE_NODE_NUM			(513)

#define RAW_BIT_DEPTH					(16)
#define DRC_OUT_MIN_VAL					(0)
#define DRC_OUT_MAX_VAL					((1 << RAW_BIT_DEPTH) - 1)
#define WDR_EXPOSURE_RATIO_F_BITS		(6)
#define AE_HIST_BIN_NUM					(256)

#define LTM_DIFF_WEIGHT_MAX				(31)
#define LTM_DIST_WEIGHT_MAX				(31)
#define LTM_D_CURVE_QUAN_BIT				(4)
#define LTM_HSV_S_BY_V_ALPHA_LUT_SLP_FBIT	(8)
#define CONTRAST_DARKBRIT_SLP_F_BITS		(12)

#define DRC_USER_DEFINE_CURVE_MAX_INDEX	(1024)
#define DRC_TONE_CURVE_MAX_VALUE		(DRC_OUT_MAX_VAL)
#define LDRC_INPUT_SAMPLE_STEP			(4)
#define DETAIL_ENHANCE_SLOPE_BIT 4

#define DRC_EPSILON						(0.000001f)

typedef struct {
	int dark_mean;
	int avg;
	int avg_low;
	int P2;
	int P1;
	int P0;
	int wdr_bin_num;
} TCM_Param;

typedef struct {
	int ltm_g_curve_1_quan_bit;		// common
	int global_tone_strength_UI;	// UI common

	int target_Y_scale_UI;			// UI DRC

	int target_Y_gain_mode_UI;		// UI LinearDRC
	int target_Y_UI;				// UI LinearDRC
	int target_Y_gain_UI;			// UI LinearDRC
} TGT_Param;

typedef struct {
	int target_gain_UI;				// UI common
	int gain_lb_UI;					// UI common
	int gain_ub_UI;					// UI common
	int dark_pctl_UI;				// UI common
	int dark_offset;				// common out
	int dark_end;					// common out
	float Lw_dt;					// common out
	bool bAdaptValid;				// common out
} TDTA_Param;

typedef struct {
	int ltm_b_curve_quan_bit;		// common
	int inflection_point_luma_UI;	// UI common
	int contrast_low_UI;			// UI common
	int contrast_high_UI;			// UI common
} TBT_Param;

static int8_t wbuf_alloc_cnt;
uint8_t *drc_wbuf_1k;
uint8_t *drc_wbuf_2k;
uint8_t *drc_wbuf_256k;
uint8_t *drc_wbuf_pool;

static void hdr_sample_flow_drche(CVI_BOOL fswdr_en, float exposure_ratio, struct drc_param_in *drc_in_param,
	struct drc_param_out *drc_out_param);

static void set_dr_information(int ExposureRatio, int *reg_ltm_b_curve_quan_bit, int *reg_ltm_g_curve_1_quan_bit);

static int DRC_7_6(uint32_t *global_tone, uint32_t *dark_tone, uint32_t *brit_tone,
	uint32_t *le_histogram, uint32_t *global_tone_se_step,
	TCM_Param *cm_param, TGT_Param *gt_param, TDTA_Param *dta_param, TBT_Param *bt_param);

static void linear_drc_dta_proc(uint32_t *global_tone, uint32_t *dark_tone, uint32_t *brit_tone,
	uint32_t *le_histogram,
	TCM_Param *cm_param, TGT_Param *gt_param, TDTA_Param *dta_param, TBT_Param *bt_param);

/* -------------------- debug 182x simulation mode start -------------------- */
static int generate_flt_distance_coeff(int *dist_wgt);
static int generate_flt_distance_normalization_coeff(int rng, int iLmapThr, int *dist_wgt, int *shift, int *gain);
/* --------------------- debug 182x simulation mode end --------------------- */

// static int generate_flt_distance_normalization_coeff(int rng, int iLmapThr, int *dist_wgt, int *shift, int *gain);
static int DRC_Blending_API(uint32_t *pMapHist, uint32_t *pMapHist_prev, uint32_t *pMapHist_output, int prebinNum,
		int bins_num, int weight_prev);

static void linear_drc_tone_protect(uint32_t *tone_curve, int start, int end);

static CVI_S32 isp_algo_drc_wbuf_alloc(void);
static CVI_S32 isp_algo_drc_wbuf_free(void);

CVI_S32 isp_algo_drc_main(struct drc_param_in *drc_param_in, struct drc_param_out *drc_param_out)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	hdr_sample_flow_drche(drc_param_in->fswdr_en,
		drc_param_in->exposure_ratio, drc_param_in, drc_param_out);

	for (CVI_U32 i = 0 ; i < 3 ; i++) {
		CVI_U8 in0 = drc_param_in->DetailEnhanceMtIn[i];
		CVI_U8 in1 = drc_param_in->DetailEnhanceMtIn[i+1];
		CVI_U16 out0 = drc_param_in->DetailEnhanceMtOut[i];
		CVI_U16 out1 = drc_param_in->DetailEnhanceMtOut[i+1];

		drc_param_out->DetailEnhanceMtSlope[i] = get_lut_slp(in0, in1, out0, out1, DETAIL_ENHANCE_SLOPE_BIT);
	}
	return ret;
}

CVI_S32 isp_algo_drc_init(void)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	isp_algo_drc_wbuf_alloc();

	return ret;
}

CVI_S32 isp_algo_drc_uninit(void)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	isp_algo_drc_wbuf_free();

	return ret;
}

#define MAX_WBUF_NUM 3
static CVI_S32 isp_algo_drc_wbuf_alloc(void)
{
	CVI_S32 ret = CVI_SUCCESS;

	uint32_t wbuf_size_list[MAX_WBUF_NUM] = {
		AE_HIST_BIN_NUM * 4,
		AE_HIST_BIN_NUM * 8,
		MAX_DRC_HIST_BINS * 4,
	};

	uint32_t wbuf_size = 0;

	for (CVI_U32 i = 0 ; i < MAX_WBUF_NUM ; i++) {
		wbuf_size += wbuf_size_list[i];
	}

	if (wbuf_alloc_cnt == 0) {
		drc_wbuf_pool = (uint8_t *)malloc(wbuf_size);
		if (drc_wbuf_pool == NULL) {
			ISP_ALGO_LOG_ERR("alloc drc_wbuf_pool failed (%d)\n", wbuf_size);
		}
		drc_wbuf_1k = drc_wbuf_pool;
		drc_wbuf_2k = drc_wbuf_1k + wbuf_size_list[0];
		drc_wbuf_256k = drc_wbuf_2k + wbuf_size_list[1];
	}

	wbuf_alloc_cnt++;

	return ret;
}

static CVI_S32 isp_algo_drc_wbuf_free(void)
{
	CVI_S32 ret = CVI_SUCCESS;

	wbuf_alloc_cnt--;
	if (wbuf_alloc_cnt < 0) {
		ISP_ALGO_LOG_ERR("alloc & free unpaired (%d)\n", wbuf_alloc_cnt);
	} else if (wbuf_alloc_cnt == 0) {
		if (drc_wbuf_pool) {
			free(drc_wbuf_pool);
			drc_wbuf_pool = 0;
		}
	}

	return ret;
}

static void drc_tone_curve_user_define_global_parser(
	float exposure_ratio, struct drc_param_in *drc_in_param, struct drc_param_out *drc_out_param)
{
	CVI_U32 i, idx;

	// le => lut nodes: 0~256
	for (i = 0; i < LTM_G_CURVE_0_NODE_NUM; i++) {
		drc_out_param->reg_ltm_deflt_lut[i] = drc_in_param->au16CurveUserDefine[i];
	}

	// se => lut nodes: 257~768
	int step = 1 << (drc_out_param->reg_ltm_g_curve_1_quan_bit - LTM_G_CURVE_0_QUAN_BIT);
	int wdr_bin_num = (float)MAX_HIST_BINS * exposure_ratio + 0.5f;
	CVI_U32 max_node = (wdr_bin_num - MAX_HIST_BINS) / step;
	float slope = (float)(DRC_USER_DEFINE_CURVE_MAX_INDEX - MAX_HIST_BINS) / (wdr_bin_num - MAX_HIST_BINS);
	float stepValue = (float)step * slope;
	float stepOffset = LTM_G_CURVE_0_NODE_NUM - 1;

	stepOffset += stepValue;
	for (i = 0; i < max_node; i++) {
		idx = MIN(stepOffset, DRC_USER_DEFINE_CURVE_MAX_INDEX);
		stepOffset += stepValue;
		drc_out_param->reg_ltm_deflt_lut[LTM_G_CURVE_0_NODE_NUM + i] = drc_in_param->au16CurveUserDefine[idx];
	}

	for (i = max_node; i < LTM_G_CURVE_1_NODE_NUM; i++) {
		drc_out_param->reg_ltm_deflt_lut[LTM_G_CURVE_0_NODE_NUM + i] = DRC_TONE_CURVE_MAX_VALUE;
	}
}

static void drc_tone_curve_user_define_dark_parser(
	struct drc_param_in *drc_in_param, struct drc_param_out *drc_out_param)
{
	CVI_U32 i;
	// TODO: use drc_in_param->au16DarkUserDefine (size = 257)
	//       when PQ Tool implement the dark tone user define
	// temporary solution: dark tone eqaul to global tone curve
	for (i = 0; i < LTM_DARK_CURVE_NODE_NUM; i++) {
		drc_out_param->reg_ltm_dark_lut[i] = drc_in_param->au16CurveUserDefine[i];
	}
}

static void drc_tone_curve_user_define_bright_parser(
	float exposure_ratio, struct drc_param_in *drc_in_param, struct drc_param_out *drc_out_param)
{
	// TODO: use drc_in_param->au16BrightUserDefine
	int step = 1 << (drc_out_param->reg_ltm_g_curve_1_quan_bit - LTM_G_CURVE_0_QUAN_BIT);
	int wdr_bin_num = (float)MAX_HIST_BINS * exposure_ratio + 0.5f;
	CVI_U32 max_node = wdr_bin_num / step;
	float slope = (float)(DRC_USER_DEFINE_CURVE_MAX_INDEX) / (max_node * step);
	float stepValue = (float)step * slope;
	float stepOffset = 0;
	CVI_U32 i, idx;

	for (i = 0; i < (max_node + 1); i++) {
		idx = MIN(stepOffset, DRC_USER_DEFINE_CURVE_MAX_INDEX);
		stepOffset += stepValue;
		drc_out_param->reg_ltm_brit_lut[i] = drc_in_param->au16CurveUserDefine[idx];
	}

	for (i = (max_node + 1); i < LTM_BRIGHT_CURVE_NODE_NUM; i++) {
		drc_out_param->reg_ltm_brit_lut[i] = DRC_TONE_CURVE_MAX_VALUE;
	}
}

static void linear_drc_tone_curve_user_define_global_parser(
	struct drc_param_in *drc_in_param, struct drc_param_out *drc_out_param)
{
	CVI_U32 i, iStep, idx;

	for (i = 0, iStep = 0; i < LTM_G_CURVE_0_NODE_NUM; i++, iStep += LDRC_INPUT_SAMPLE_STEP) {
		idx = MIN(iStep, DRC_USER_DEFINE_CURVE_MAX_INDEX);
		drc_out_param->reg_ltm_deflt_lut[i] = drc_in_param->au16CurveUserDefine[idx];
	}

	for (i = 0; i < LTM_G_CURVE_1_NODE_NUM; i++) {
		drc_out_param->reg_ltm_deflt_lut[LTM_G_CURVE_0_NODE_NUM + i] = DRC_TONE_CURVE_MAX_VALUE;
	}
}

static void linear_drc_tone_curve_user_define_dark_parser(
	struct drc_param_in *drc_in_param, struct drc_param_out *drc_out_param)
{
	CVI_U32 i, iStep, idx;

	for (i = 0, iStep = 0; i < LTM_DARK_CURVE_NODE_NUM; i++, iStep += LDRC_INPUT_SAMPLE_STEP) {
		idx = MIN(iStep, DRC_USER_DEFINE_CURVE_MAX_INDEX);
		drc_out_param->reg_ltm_dark_lut[i] = drc_in_param->au16CurveUserDefine[idx];
	}
}

static void linear_drc_tone_curve_user_define_bright_parser(
	struct drc_param_in *drc_in_param, struct drc_param_out *drc_out_param)
{
	CVI_U32 i, iStep, idx;

	for (i = 0, iStep = 0; i < LTM_G_CURVE_0_NODE_NUM; i++, iStep += LDRC_INPUT_SAMPLE_STEP) {
		idx = MIN(iStep, DRC_USER_DEFINE_CURVE_MAX_INDEX);
		drc_out_param->reg_ltm_brit_lut[i] = drc_in_param->au16CurveUserDefine[idx];
	}

	for (i = LTM_G_CURVE_0_NODE_NUM; i < LTM_BRIGHT_CURVE_NODE_NUM; i++) {
		drc_out_param->reg_ltm_brit_lut[i] = DRC_TONE_CURVE_MAX_VALUE;
	}
}

static void set_lmap_lut(CVI_U8 darkMapStr, CVI_U8 britMapStr, struct drc_param_out *drc_out_param)
{
	CVI_FLOAT leLmapPower = (CVI_FLOAT)(darkMapStr)/100;
	CVI_FLOAT seLmapPower = (CVI_FLOAT)(100-britMapStr)/100;

	CVI_U8 leLmapIn[4] = {0, 32, 128, 255};
	CVI_U8 leLmapOut[4] = {0, 32, 128, 255};
	CVI_S16 leLmapSlope[3] = {16, 16, 16};
	CVI_U8 seLmapIn[4] = {0, 32, 128, 255};
	CVI_U8 seLmapOut[4] = {0, 32, 128, 255};
	CVI_S16 seLmapSlope[3] = {16, 16, 16};

	for (CVI_U8 i = 0; i < 4; ++i) {
		leLmapOut[i] = pow((CVI_FLOAT)leLmapIn[i]/255, leLmapPower) * 255;
		seLmapOut[i] = pow((CVI_FLOAT)seLmapIn[i]/255, seLmapPower) * 255;
	}



	for (CVI_U8 i = 0; i < 3; ++i) {
		leLmapSlope[i] =
			(int)((CVI_FLOAT)(leLmapOut[i+1] - leLmapOut[i]) / (leLmapIn[i+1] - leLmapIn[i]) * 16);
		seLmapSlope[i] =
			(int)((CVI_FLOAT)(seLmapOut[i+1] - seLmapOut[i]) / (seLmapIn[i+1] - seLmapIn[i]) * 16);
	}

	drc_out_param->reg_ltm_de_lmap_lut_in[0] = leLmapIn[0];
	drc_out_param->reg_ltm_de_lmap_lut_in[1] = leLmapIn[1];
	drc_out_param->reg_ltm_de_lmap_lut_in[2] = leLmapIn[2];
	drc_out_param->reg_ltm_de_lmap_lut_in[3] = leLmapIn[3];
	drc_out_param->reg_ltm_de_lmap_lut_out[0] = leLmapOut[0];
	drc_out_param->reg_ltm_de_lmap_lut_out[1] = leLmapOut[1];
	drc_out_param->reg_ltm_de_lmap_lut_out[2] = leLmapOut[2];
	drc_out_param->reg_ltm_de_lmap_lut_out[3] = leLmapOut[3];
	drc_out_param->reg_ltm_de_lmap_lut_slope[0] = leLmapSlope[0];
	drc_out_param->reg_ltm_de_lmap_lut_slope[1] = leLmapSlope[1];
	drc_out_param->reg_ltm_de_lmap_lut_slope[2] = leLmapSlope[2];

	drc_out_param->reg_ltm_be_lmap_lut_in[0] = seLmapIn[0];
	drc_out_param->reg_ltm_be_lmap_lut_in[1] = seLmapIn[1];
	drc_out_param->reg_ltm_be_lmap_lut_in[2] = seLmapIn[2];
	drc_out_param->reg_ltm_be_lmap_lut_in[3] = seLmapIn[3];
	drc_out_param->reg_ltm_be_lmap_lut_out[0] = seLmapOut[0];
	drc_out_param->reg_ltm_be_lmap_lut_out[1] = seLmapOut[1];
	drc_out_param->reg_ltm_be_lmap_lut_out[2] = seLmapOut[2];
	drc_out_param->reg_ltm_be_lmap_lut_out[3] = seLmapOut[3];
	drc_out_param->reg_ltm_be_lmap_lut_slope[0] = seLmapSlope[0];
	drc_out_param->reg_ltm_be_lmap_lut_slope[1] = seLmapSlope[1];
	drc_out_param->reg_ltm_be_lmap_lut_slope[2] = seLmapSlope[2];
}

static void hdr_sample_flow_drche(CVI_BOOL fswdr_en, float exposure_ratio,
	struct drc_param_in *drc_in_param, struct drc_param_out *drc_out_param)
{
	// set DR information
	set_dr_information(drc_in_param->ToneParserMaxRatio,
		&(drc_out_param->reg_ltm_b_curve_quan_bit), &(drc_out_param->reg_ltm_g_curve_1_quan_bit));

	drc_out_param->ltm_global_tone_bin_num = LTM_G_CURVE_0_NODE_NUM;
	drc_out_param->ltm_global_tone_bin_se_step = 1;

	/* ------------------- DRC (linearDRC) tone mapping curve ------------------ */
	if (drc_in_param->ToneCurveSelect == 0) {
		// user define
		drc_out_param->reg_ltm_wdr_bin_num = 256;
		if (fswdr_en == 1) {
			// wdr mode
			drc_tone_curve_user_define_global_parser(exposure_ratio, drc_in_param, drc_out_param);
			drc_tone_curve_user_define_dark_parser(drc_in_param, drc_out_param);
			drc_tone_curve_user_define_bright_parser(exposure_ratio, drc_in_param, drc_out_param);
		} else {
			// linear mode: linear drc
			linear_drc_tone_curve_user_define_global_parser(drc_in_param, drc_out_param);
			linear_drc_tone_curve_user_define_dark_parser(drc_in_param, drc_out_param);
			linear_drc_tone_curve_user_define_bright_parser(drc_in_param, drc_out_param);
		}
	} else if (drc_in_param->ToneCurveSelect == 1) {
		uint32_t *le_histogram = drc_in_param->LEHistogram;
		// uint32_t *se_histogram = drc_in_param->SEHistogram;

		uint32_t *global_tone = drc_out_param->reg_ltm_deflt_lut;
		uint32_t *dark_tone = drc_out_param->reg_ltm_dark_lut;
		uint32_t *brit_tone = drc_out_param->reg_ltm_brit_lut;

		/* -------------------- debug 182x simulation mode start -------------------- */
		int ltm_de_lmap_thr = 255;
		int ltm_be_lmap_thr = 255;
		/* --------------------- debug 182x simulation mode end --------------------- */

		if (fswdr_en == 1) {
			// HDR/WDR mode (DRC 7-6)

			/* -------------- HDR Mode wdr_bin_num modify by LE min weight -------------- */
			float min_weight = drc_in_param->u16WDRCombineMinWeight;

			int wdr_bin_num = (float)(min_weight / 256.0f) * MAX_HIST_BINS
				+ ((float)((256.0f - min_weight) / 256.0f) * MAX_HIST_BINS * exposure_ratio);

			/* --------------------- plot wdr histogram for PQ Tool --------------------- */
			drc_out_param->reg_ltm_wdr_bin_num = wdr_bin_num;

			/* ---------------------------- HDR Mode UI Param --------------------------- */
			TCM_Param cm_param;
			TGT_Param gt_param;
			TDTA_Param dta_param;
			TBT_Param bt_param;

			cm_param.wdr_bin_num = wdr_bin_num;

			gt_param.ltm_g_curve_1_quan_bit = drc_out_param->reg_ltm_g_curve_1_quan_bit;
			gt_param.global_tone_strength_UI = drc_in_param->HdrStrength;
			gt_param.target_Y_scale_UI = drc_in_param->targetYScale;

			dta_param.dark_pctl_UI = drc_in_param->u8DEAdaptPercentile;
			dta_param.target_gain_UI = drc_in_param->u8DEAdaptTargetGain;
			dta_param.gain_lb_UI = drc_in_param->u8DEAdaptGainLB;
			dta_param.gain_ub_UI = drc_in_param->u8DEAdaptGainUB;

			bt_param.inflection_point_luma_UI = drc_in_param->stBritToneParam.u8BritInflectPtLuma;
			bt_param.contrast_low_UI = drc_in_param->stBritToneParam.u8BritContrastLow;
			bt_param.contrast_high_UI = drc_in_param->stBritToneParam.u8BritContrastHigh;
			bt_param.ltm_b_curve_quan_bit = drc_out_param->reg_ltm_b_curve_quan_bit;

			/* ------------------------------- HDR DRC 7-6 ------------------------------ */
			uint32_t global_tone_se_step;

			DRC_7_6(global_tone, dark_tone, brit_tone, le_histogram,
				&global_tone_se_step,
				&cm_param, &gt_param, &dta_param, &bt_param);

			/* -------------- plot global tone for PQ Tool -------------- */
			drc_out_param->ltm_global_tone_bin_num = LTM_G_CURVE_TOTAL_NODE_NUM;
			drc_out_param->ltm_global_tone_bin_se_step = global_tone_se_step;

			/* ------------------------------ HDR Mode IIR ------------------------------ */
			DRC_Blending_API(global_tone, ISP_PTR_CAST_U32(drc_in_param->pu32PreGlobalLut),
				global_tone, drc_in_param->preWdrBinNum,
				LTM_G_CURVE_TOTAL_NODE_NUM, drc_in_param->average_frame);
			DRC_Blending_API(brit_tone, ISP_PTR_CAST_U32(drc_in_param->pu32PreBritLut),
				brit_tone, drc_in_param->preWdrBinNum,
				LTM_BRIGHT_CURVE_NODE_NUM, drc_in_param->average_frame);
			DRC_Blending_API(dark_tone, ISP_PTR_CAST_U32(drc_in_param->pu32PreDarkLut),
				dark_tone, drc_in_param->preWdrBinNum,
				LTM_DARK_CURVE_NODE_NUM, drc_in_param->average_frame);

			/* ---------------------- HDR mode luma map adjustment ---------------------- */
			set_lmap_lut(drc_in_param->DarkMapStr,
				drc_in_param->BritMapStr, drc_out_param);

			/* -------------------- debug 182x simulation mode start -------------------- */
			if (drc_in_param->dbg_182x_sim_enable) {
				int temp_ltm_de_lmap_thr =
				(int)((float)MAX(cm_param.avg_low, 8) * drc_in_param->stDarkToneParam.fDarkMapStr);

				ltm_de_lmap_thr = MINMAX(temp_ltm_de_lmap_thr, 8, 255);

				int temp_ltm_be_lmap_thr =
					(int)(256.0f * drc_in_param->stBritToneParam.fBritMapStr /
						MIN(exposure_ratio, 32.0f));

				ltm_be_lmap_thr = MINMAX(temp_ltm_be_lmap_thr, 8, 255);
			}
			/* --------------------- debug 182x simulation mode end --------------------- */

		} else {
			// SDR mode (Linear DRC)
			/* -------------------- linearDRC set quantization bits -------------------- */
			drc_out_param->reg_ltm_g_curve_1_quan_bit = 4;
			drc_out_param->reg_ltm_b_curve_quan_bit = 4;

			/* --------------------- plot wdr histogram for PQ Tool --------------------- */
			drc_out_param->reg_ltm_wdr_bin_num = MAX_HIST_BINS;

			/* ------------------------- linearDRC UI parameter ------------------------- */
			TCM_Param cm_param;
			TGT_Param gt_param;
			TDTA_Param dta_param;
			TBT_Param bt_param;

			cm_param.wdr_bin_num = 257;		// 0~256

			gt_param.ltm_g_curve_1_quan_bit = drc_out_param->reg_ltm_g_curve_1_quan_bit;
			gt_param.target_Y_gain_mode_UI = drc_in_param->stBritToneParam.u8SdrTargetYGainMode;
			gt_param.target_Y_UI = drc_in_param->stBritToneParam.u8SdrTargetY;
			gt_param.target_Y_gain_UI = drc_in_param->stBritToneParam.u8SdrTargetYGain;
			gt_param.global_tone_strength_UI = drc_in_param->stBritToneParam.u16SdrGlobalToneStr;

			// dta_param.adapt_en_UI = true;
			// dta_param.dark_info_en_UI = 0;
			dta_param.dark_pctl_UI = drc_in_param->u8SdrDEAdaptPercentile;
			dta_param.target_gain_UI = drc_in_param->u8SdrDEAdaptTargetGain;
			dta_param.gain_lb_UI = drc_in_param->u8SdrDEAdaptGainLB;
			dta_param.gain_ub_UI = drc_in_param->u8SdrDEAdaptGainUB;

			bt_param.ltm_b_curve_quan_bit = drc_out_param->reg_ltm_b_curve_quan_bit;
			bt_param.inflection_point_luma_UI =
				drc_in_param->stBritToneParam.u8SdrBritInflectPtLuma;
			bt_param.contrast_low_UI = drc_in_param->stBritToneParam.u8SdrBritContrastLow;
			bt_param.contrast_high_UI = drc_in_param->stBritToneParam.u8SdrBritContrastHigh;

			/* -------------------------- linearDRC main proc -------------------------- */
			linear_drc_dta_proc(global_tone, dark_tone, brit_tone,
				le_histogram, &cm_param, &gt_param, &dta_param, &bt_param);

			/* -------------- plot global tone for PQ Tool -------------- */
			drc_out_param->ltm_global_tone_bin_num = LTM_G_CURVE_0_NODE_NUM;
			drc_out_param->ltm_global_tone_bin_se_step = 1;

			/* ----------------------------- linearDRC IIR ----------------------------- */
			DRC_Blending_API(global_tone, ISP_PTR_CAST_U32(drc_in_param->pu32PreGlobalLut),
				global_tone, drc_in_param->preWdrBinNum,
				LTM_G_CURVE_TOTAL_NODE_NUM, drc_in_param->average_frame);
			DRC_Blending_API(brit_tone, ISP_PTR_CAST_U32(drc_in_param->pu32PreBritLut),
				brit_tone, drc_in_param->preWdrBinNum,
				LTM_BRIGHT_CURVE_NODE_NUM, drc_in_param->average_frame);
			DRC_Blending_API(dark_tone, ISP_PTR_CAST_U32(drc_in_param->pu32PreDarkLut),
				dark_tone, drc_in_param->preWdrBinNum,
				LTM_DARK_CURVE_NODE_NUM, drc_in_param->average_frame);

			/* --------------------- linearDRC luma map adjustment --------------------- */
			set_lmap_lut(drc_in_param->SdrDarkMapStr,
				drc_in_param->SdrBritMapStr, drc_out_param);

			/* -------------------- debug 182x simulation mode start -------------------- */
			if (drc_in_param->dbg_182x_sim_enable) {
				float f_avg_low = (float)MIN(cm_param.avg_low, 8.0f);
				int temp_ltm_de_lmap_thr =
					(int)(f_avg_low * drc_in_param->stDarkToneParam.fSdrDarkMapStr);

				ltm_de_lmap_thr = MINMAX(temp_ltm_de_lmap_thr, 8, 255);

				float f_dark_end = (float)MAX(dta_param.dark_end, 8.0f);
				int temp_ltm_be_lmap_thr =
					(int)(f_dark_end * drc_in_param->stBritToneParam.fSdrBritMapStr);

				ltm_be_lmap_thr = MINMAX(temp_ltm_be_lmap_thr, 8, 255);
			}
			/* --------------------- debug 182x simulation mode end --------------------- */

			linear_drc_tone_protect(global_tone, MAX_HIST_BINS, LTM_G_CURVE_TOTAL_NODE_NUM);
			linear_drc_tone_protect(dark_tone, MAX_HIST_BINS, LTM_DARK_CURVE_NODE_NUM);
			linear_drc_tone_protect(brit_tone, MAX_HIST_BINS, LTM_BRIGHT_CURVE_NODE_NUM);
		}

		/* -------------------- debug 182x simulation mode start -------------------- */
		// filter spatial kernel
		generate_flt_distance_coeff(drc_out_param->reg_ltm_de_dist_wgt);
		generate_flt_distance_coeff(drc_out_param->reg_ltm_be_dist_wgt);

		if (drc_in_param->dbg_182x_sim_enable) {
			generate_flt_distance_normalization_coeff(5,
				ltm_de_lmap_thr,
				drc_out_param->reg_ltm_de_dist_wgt,
				&(drc_out_param->reg_ltm_de_strth_dshft),
				&(drc_out_param->reg_ltm_de_strth_gain));

			generate_flt_distance_normalization_coeff(5,
				ltm_be_lmap_thr,
				drc_out_param->reg_ltm_be_dist_wgt,
				&(drc_out_param->reg_ltm_be_strth_dshft),
				&(drc_out_param->reg_ltm_be_strth_gain));

			drc_out_param->reg_ltm_de_lmap_lut_in[0] = 0;
			drc_out_param->reg_ltm_de_lmap_lut_in[1] = ltm_de_lmap_thr;
			drc_out_param->reg_ltm_de_lmap_lut_in[2] = 255;
			drc_out_param->reg_ltm_de_lmap_lut_in[3] = 255;
			drc_out_param->reg_ltm_de_lmap_lut_out[0] = 0;
			drc_out_param->reg_ltm_de_lmap_lut_out[1] = ltm_de_lmap_thr;
			drc_out_param->reg_ltm_de_lmap_lut_out[2] = ltm_de_lmap_thr;
			drc_out_param->reg_ltm_de_lmap_lut_out[3] = ltm_de_lmap_thr;
			drc_out_param->reg_ltm_de_lmap_lut_slope[0] = 16;  // s7.4, slope = 1 (45 deg) => 16
			drc_out_param->reg_ltm_de_lmap_lut_slope[1] = 0;
			drc_out_param->reg_ltm_de_lmap_lut_slope[2] = 0;

			drc_out_param->reg_ltm_be_lmap_lut_in[0] = 0;
			drc_out_param->reg_ltm_be_lmap_lut_in[1] = ltm_be_lmap_thr;
			drc_out_param->reg_ltm_be_lmap_lut_in[2] = 255;
			drc_out_param->reg_ltm_be_lmap_lut_in[3] = 255;
			drc_out_param->reg_ltm_be_lmap_lut_out[0] = 0;
			drc_out_param->reg_ltm_be_lmap_lut_out[1] = ltm_be_lmap_thr;
			drc_out_param->reg_ltm_be_lmap_lut_out[2] = ltm_be_lmap_thr;
			drc_out_param->reg_ltm_be_lmap_lut_out[3] = ltm_be_lmap_thr;
			drc_out_param->reg_ltm_be_lmap_lut_slope[0] = 16;  // s7.4
			drc_out_param->reg_ltm_be_lmap_lut_slope[1] = 0;
			drc_out_param->reg_ltm_be_lmap_lut_slope[2] = 0;
		} else {
			drc_out_param->reg_ltm_de_strth_dshft = 12;
			drc_out_param->reg_ltm_de_strth_gain = 1118;
			drc_out_param->reg_ltm_be_strth_dshft = 12;
			drc_out_param->reg_ltm_be_strth_gain = 1118;
		}
		/* --------------------- debug 182x simulation mode end --------------------- */

	}
}

static void set_dr_information(int ExposureRatio, int *reg_ltm_b_curve_quan_bit, int *reg_ltm_g_curve_1_quan_bit)
{
	int ltm_g_curve_1_quan_bit = 0;
	int ltm_b_curve_quan_bit = 0;

	if (ExposureRatio <= (2 << WDR_EXPOSURE_RATIO_F_BITS))
		ltm_g_curve_1_quan_bit = 4;
	else if (ExposureRatio <= (4 << WDR_EXPOSURE_RATIO_F_BITS))
		ltm_g_curve_1_quan_bit = 5;
	else if (ExposureRatio <= (8 << WDR_EXPOSURE_RATIO_F_BITS))
		ltm_g_curve_1_quan_bit = 6;
	else if (ExposureRatio <= (16 << WDR_EXPOSURE_RATIO_F_BITS))
		ltm_g_curve_1_quan_bit = 7;
	else if (ExposureRatio <= (32 << WDR_EXPOSURE_RATIO_F_BITS))
		ltm_g_curve_1_quan_bit = 8;
	else if (ExposureRatio <= (64 << WDR_EXPOSURE_RATIO_F_BITS))
		ltm_g_curve_1_quan_bit = 9;
	else if (ExposureRatio <= (128 << WDR_EXPOSURE_RATIO_F_BITS))
		ltm_g_curve_1_quan_bit = 10;
	else if (ExposureRatio <= (256 << WDR_EXPOSURE_RATIO_F_BITS))
		ltm_g_curve_1_quan_bit = 11;

	ltm_b_curve_quan_bit = ltm_g_curve_1_quan_bit;
	*reg_ltm_b_curve_quan_bit = ltm_b_curve_quan_bit;
	*reg_ltm_g_curve_1_quan_bit = ltm_g_curve_1_quan_bit;
}

static void get_dark_tone_adapt_info(TDTA_Param *dta_param, const uint32_t *le_histogram, TCM_Param *cm_param)
{
	// get pixel numbers of le_histogram
	int hist_pxl_num = 0;

	for (int i = 0; i < AE_HIST_BIN_NUM; i++) {
		hist_pxl_num += le_histogram[i];
	}

	// get percentile value by user param: dark_pctl_UI
	int dark_pctl_num = hist_pxl_num * dta_param->dark_pctl_UI / 100.0f;
	int dark_pxl_num = 0;
	int dark_pctl = 0;

	for (int i = 0; i < AE_HIST_BIN_NUM; i++) {
		dark_pxl_num += le_histogram[i];
		if (dark_pxl_num > dark_pctl_num) {
			dark_pctl = i;
			break;
		}
	}

	// get dark_mean to calculate avg
	int dark_pxl_sum = 0;
	int dark_pxl_cnt = 0;

	for (int i = 0; i <= dark_pctl; i++) {
		dark_pxl_cnt += le_histogram[i];
		dark_pxl_sum += i * le_histogram[i];
	}
	if (dark_pxl_cnt > 0) {
		cm_param->dark_mean = dark_pxl_sum / dark_pxl_cnt;		// floor to int
	} else {
		cm_param->dark_mean = dark_pctl;
	}

	ISP_ALGO_LOG_DEBUG("[dark_pxl_cnt] = %d, [dark_pctl] = %d, [dark_mean] = %d\n",
		dark_pxl_cnt, dark_pctl, cm_param->dark_mean);

	int P0 = 0;

	for (int i = 0; i < cm_param->dark_mean; i++) {
		P0 += le_histogram[i];
	}
	cm_param->P0 = P0;

	// get the avg and P2 from dark_mean to white(255)
	int avg_pxl_sum = 0;
	int avg_pxl_cnt = 0;

	for (int i = cm_param->dark_mean + 1; i < AE_HIST_BIN_NUM; i++) {
		avg_pxl_cnt += le_histogram[i];
		avg_pxl_sum += i * le_histogram[i];
	}

	if (avg_pxl_cnt > 0) {
		cm_param->avg = avg_pxl_sum / avg_pxl_cnt; // floor to int
	} else {
		cm_param->avg = cm_param->dark_mean;
	}

	// get the avg_low and P1 from dark_mean to avg_low
	int avg_low_pxl_sum = 0;
	int avg_low_pxl_cnt = 0;

	for (int i = cm_param->dark_mean + 1; i <= cm_param->avg; i++) {
		avg_low_pxl_cnt += le_histogram[i];
		avg_low_pxl_sum += i * le_histogram[i];
	}

	if (avg_low_pxl_cnt > 0) {
		cm_param->avg_low = avg_low_pxl_sum / avg_low_pxl_cnt;		// floor to int
	} else {
		cm_param->avg_low = cm_param->dark_mean;
	}

	int P1 = 0;

	for (int i = cm_param->dark_mean + 1; i <= cm_param->avg_low; i++) {
		P1 += le_histogram[i];
	}
	cm_param->P1 = P1;

	int P2 = 0;

	for (int i = cm_param->avg_low + 1; i <= cm_param->avg; i++) {
		P2 += le_histogram[i];
	}
	cm_param->P2 = P2;

	ISP_ALGO_DEBUG(LOG_DEBUG, "[P0] = %d\n[P1] = %d\n[P2] = %d\n",
		P0, P1, P2);
}

static CVI_BOOL get_linear_drc_dark_tone_adapt_param(const uint32_t *global_tone,
	TCM_Param *cm_param, TDTA_Param *dta_param)
{
	int dark_offset = cm_param->dark_mean;
	float T = 0.0f;
	float Tgain, Gain_lowbound, Gain_upbound;
	int Tbase_numerator, Tbase_denominator;

	dta_param->bAdaptValid = false;
	dta_param->dark_offset = dark_offset;

	if (cm_param->P1 == 0) {
		return CVI_FALSE;
	}
	if (cm_param->P2 == 0) {
		return CVI_FALSE;
	}

	Tgain = (float)dta_param->target_gain_UI / 32.0f;
	Gain_lowbound = (float)dta_param->gain_lb_UI / 32.0f;
	Gain_upbound = (float)dta_param->gain_ub_UI / 32.0f;

	Gain_lowbound = MIN(Gain_lowbound, Gain_upbound);

	// default
	T = (float)(Tgain * cm_param->P1)
		/ (Tgain * cm_param->P1 + cm_param->P2);
	T = MINMAX(T, 0.1f, 0.9f);

	ISP_ALGO_DEBUG(LOG_DEBUG, "[T] = %6.3f\n", T);

	Tbase_numerator = (int)global_tone[cm_param->avg_low] - (int)global_tone[dark_offset];
	Tbase_denominator = (int)global_tone[cm_param->avg] - (int)global_tone[dark_offset];

	if (Tbase_numerator <= 0) {
		return CVI_FALSE;
	}

	if (Tbase_denominator <= 0) {
		return CVI_FALSE;
	}

	float T_base = (float)Tbase_numerator / Tbase_denominator;

	ISP_ALGO_DEBUG(LOG_DEBUG, "[T_base] = %6.3f\n", T_base);

	int new_avg = cm_param->avg;

	if (T_base < T) {
		if (T_base < DRC_EPSILON) {
			return CVI_FALSE;
		}

		T = MINMAX(T, Gain_lowbound * T_base, Gain_upbound * T_base);
		ISP_ALGO_DEBUG(LOG_DEBUG, "[finalT] = %6.3f\n", T);
		new_avg = round((float)cm_param->avg * T / T_base);
	}

	new_avg = MIN(new_avg, AE_HIST_BIN_NUM);
	dta_param->dark_end = new_avg;

	int T_absolute = T * ((int)global_tone[cm_param->avg] - (int)global_tone[dark_offset])
		+ global_tone[dark_offset];

	ISP_ALGO_DEBUG(LOG_DEBUG, "[dark_offset] = %d, [avg_low] = %d, [avg] = %d, [new_avg] = %d, [T_absolute] = %d\n",
		dark_offset, cm_param->avg_low, cm_param->avg, new_avg, T_absolute);

	int avg_low_dark_offset, new_avg_dark_offset;
	int absolute_gtone_dark_offset, gtone_newavg_gtone_dark_offset;

	avg_low_dark_offset = cm_param->avg_low - dark_offset;
	new_avg_dark_offset = new_avg - dark_offset;
	absolute_gtone_dark_offset = T_absolute - (int)global_tone[dark_offset];
	gtone_newavg_gtone_dark_offset = (int)global_tone[new_avg] - (int)global_tone[dark_offset];

	if (
		(avg_low_dark_offset <= 0) ||
		(new_avg_dark_offset <= 0) ||
		(absolute_gtone_dark_offset <= 0) ||
		(gtone_newavg_gtone_dark_offset <= 0)
	) {
		return CVI_FALSE;
	}

	float newR, newT, newTR_mR, Lw_dt;

	newR = (float)avg_low_dark_offset / (float)new_avg_dark_offset;
	newT = (float)absolute_gtone_dark_offset / gtone_newavg_gtone_dark_offset;
	newTR_mR = newT * newR - newR;

	if ((newTR_mR < DRC_EPSILON) && (newTR_mR > -DRC_EPSILON)) {
		ISP_ALGO_DEBUG(LOG_DEBUG, "(newT * newR - newR) < DRC_EPSILON\n");
		return CVI_FALSE;
	}

	Lw_dt = (newR * newR - newT) / newTR_mR;
	ISP_ALGO_DEBUG(LOG_DEBUG, "[newR] = %6.3f, [newT] = %6.3f, [Lw_dt] = %6.3f\n",
		newR, newT, Lw_dt);

	if (Lw_dt <= 0.1f) {
		Lw_dt = 0.1f;
	}
	dta_param->Lw_dt = Lw_dt;
	dta_param->bAdaptValid = true;

	return CVI_TRUE;
}

static void gen_drc_global_tone(const TGT_Param *gt_param, const TCM_Param *cm_param,
	uint32_t *global_tone, uint32_t *se_step)
{
//	float gt_target;
	int TargetYScale = MINMAX(gt_param->target_Y_scale_UI, 1, 2048);
	int nbins = cm_param->wdr_bin_num;
	const int SDR_MAX_VAL = DRC_TONE_CURVE_MAX_VALUE;

	// Step.1 map global tone to 0.7 at 256
	// map 0.7 at 256
	float R_ini, T_ini;
	float Lw_ini, Lw2_ini, Lx_ini, L_ini;
	float T_gt;

	R_ini = 256.0f / 2048.0f;
	T_ini = 0.7;
	Lw_ini = (R_ini * R_ini - T_ini) / (T_ini * R_ini - R_ini);
	Lx_ini = (float)TargetYScale / 2048.0f;
	L_ini = Lx_ini * Lw_ini;
	Lw2_ini = Lw_ini * Lw_ini;
	T_gt = L_ini * (1 + L_ini / Lw2_ini) / (1 + L_ini);

	// Step.2 calculate output histogram based on global tone
	// generate global tone
	float R_gt;
	float Lw_gt, Lw2_gt, Lx_gt, L_gt;
	int mapped_n;

	R_gt = 256.0f / (float)(nbins - 1);
	Lw_gt = (R_gt * R_gt - T_gt) / (T_gt * R_gt - R_gt);
	Lw2_gt = Lw_gt * Lw_gt;

	if (R_gt >= T_gt) {
		for (int i = 0; i < LTM_G_CURVE_0_NODE_NUM; i++) {
			global_tone[i] = DRC_OUT_MIN_VAL
				+ ((DRC_OUT_MAX_VAL - DRC_OUT_MIN_VAL) * i / LTM_G_CURVE_0_NODE_NUM);
		}
		for (int i = 0; i < LTM_G_CURVE_1_NODE_NUM; i++) {
			global_tone[LTM_G_CURVE_0_NODE_NUM + i] = SDR_MAX_VAL;
		}

		int step = 1 << (gt_param->ltm_g_curve_1_quan_bit - LTM_G_CURVE_0_QUAN_BIT);
		*se_step = step;
		return;
	}

	const int Linear_start = 0;
	const int Linear_end = nbins - 1;
	int drc_out_val_range = DRC_OUT_MAX_VAL - DRC_OUT_MIN_VAL;
	int linear_val_range = Linear_end - Linear_start;

	uint32_t u32linear_tone;
	uint32_t u32wdr_tone;

	// 0 ~ LTM_G_CURVE_0_NODE_NUM
	for (int i = 0; i < LTM_G_CURVE_0_NODE_NUM; i++) {
		Lx_gt = (float)i / (float)(nbins - 1);
		L_gt = Lx_gt * Lw_gt;
		mapped_n = (int)(DRC_OUT_MAX_VAL * L_gt * (1 + L_gt / Lw2_gt) / (1 + L_gt));
		u32wdr_tone = MINMAX(mapped_n, DRC_OUT_MIN_VAL, DRC_OUT_MAX_VAL);

		if (i <= Linear_start)
			u32linear_tone = DRC_OUT_MIN_VAL;
		else if (i <= Linear_end)
			u32linear_tone = DRC_OUT_MIN_VAL +
				drc_out_val_range * (i - Linear_start) / linear_val_range;
		else
			u32linear_tone = DRC_OUT_MAX_VAL;

		mapped_n = gt_param->global_tone_strength_UI * u32wdr_tone;
		mapped_n = (mapped_n + (256 - gt_param->global_tone_strength_UI) * u32linear_tone) / 256;
		// mapped_n = (mapped_n * gt_param->contrast_gain_UI + 64) >> 7;
		u32wdr_tone = MINMAX(mapped_n, DRC_OUT_MIN_VAL, DRC_OUT_MAX_VAL);

		global_tone[i] = u32wdr_tone;
	}

	int step = 1 << (gt_param->ltm_g_curve_1_quan_bit - LTM_G_CURVE_0_QUAN_BIT);
	int iRefIdx = LTM_G_CURVE_0_NODE_NUM - 1 + step;

	for (int i = 0; i < LTM_G_CURVE_1_NODE_NUM; i++, iRefIdx += step) {
		if (iRefIdx < nbins) {
			Lx_gt = (float)iRefIdx / (float)(nbins - 1);
			L_gt = Lx_gt * Lw_gt;
			mapped_n = (int)(DRC_OUT_MAX_VAL * L_gt * (1 + L_gt / Lw2_gt) / (1 + L_gt));
			u32wdr_tone = MINMAX(mapped_n, DRC_OUT_MIN_VAL, DRC_OUT_MAX_VAL);

			if (iRefIdx <= Linear_start)
				u32linear_tone = DRC_OUT_MIN_VAL;
			else if (iRefIdx <= Linear_end)
				u32linear_tone = DRC_OUT_MIN_VAL +
					drc_out_val_range * (iRefIdx - Linear_start) / linear_val_range;
			else
				u32linear_tone = DRC_OUT_MAX_VAL;

			mapped_n = gt_param->global_tone_strength_UI * u32wdr_tone;
			mapped_n = (mapped_n + (256 - gt_param->global_tone_strength_UI) * u32linear_tone) / 256;
			// mapped_n = (mapped_n * gt_param->contrast_gain_UI + 64) >> 7;
			u32wdr_tone = MINMAX(mapped_n, DRC_OUT_MIN_VAL, DRC_OUT_MAX_VAL);

			global_tone[LTM_G_CURVE_0_NODE_NUM + i] = u32wdr_tone;
		} else {
			global_tone[LTM_G_CURVE_0_NODE_NUM + i] = SDR_MAX_VAL;
		}
	}

	*se_step = step;
}

static void gen_drc_dark_tone_adaptive(const uint32_t *global_tone,
	const TDTA_Param *dta_param, uint32_t *dark_tone)
{
	if (dta_param->bAdaptValid) {
		float Lw_dt, Lw2_dt;
		int dark_start, dark_range, dark_end;
		int min_val_dt, max_val_dt, delta_dt;

		Lw_dt = dta_param->Lw_dt;
		Lw2_dt = Lw_dt * Lw_dt;
		dark_start = dta_param->dark_offset;
		dark_end = dta_param->dark_end;
		dark_range = dark_end - dark_start;
		min_val_dt = global_tone[dark_start];
		max_val_dt = global_tone[dark_end];
		delta_dt = max_val_dt - min_val_dt;

		ISP_ALGO_DEBUG(LOG_DEBUG, "[Lw_dt] = %6.3f\n", dta_param->Lw_dt);
		ISP_ALGO_DEBUG(LOG_DEBUG, "[dark_start] = %d, [dark_range] = %d, [dark_end] = %d\n",
			dark_start, dark_range, dark_end);
		ISP_ALGO_DEBUG(LOG_DEBUG, "[min_val_dt] = %d, [max_val_dt] = %d\n",
			min_val_dt, max_val_dt);

		for (int i = 0; i < LTM_DARK_CURVE_NODE_NUM; i++) {
			if (i < dark_start) {
				dark_tone[i] = global_tone[i];
			} else if (i < dark_end) {
				float ratio = (float)(i - dark_start) / dark_range;
				float L = ratio * Lw_dt;
				int mapped_n = min_val_dt + floor(delta_dt * L * (1 + L / Lw2_dt) / (1 + L));

				dark_tone[i] = MINMAX(mapped_n, min_val_dt, max_val_dt);
			} else {
				dark_tone[i] = global_tone[i];
			}
		}
	} else {
		for (int i = 0; i < LTM_DARK_CURVE_NODE_NUM; i++) {
			dark_tone[i] = global_tone[i];
		}
		ISP_ALGO_DEBUG(LOG_DEBUG, "[DRC dta bypass]\n");
	}
}

static void gen_drc_bright_tone(const uint32_t *global_tone,
	const TCM_Param *cm_param, TBT_Param *bt_param, uint32_t *brit_tone)
{
	int iBTnbins = MAX(257, cm_param->wdr_bin_num);

	// global tone upper bound
	int gt_ub = global_tone[AE_HIST_BIN_NUM];
	int linear_val = 65535.0f * AE_HIST_BIN_NUM / (iBTnbins - 1);
	int diff_gt_ub_linear = MAX((gt_ub - linear_val), 0);

	int le_node = AE_HIST_BIN_NUM;
	int le_val = linear_val + (diff_gt_ub_linear * bt_param->inflection_point_luma_UI / 100);

	int right_node = (iBTnbins - 1);
	int right_val = DRC_OUT_MAX_VAL;	// linear_tone[iBTnbins - 1];

	int cp1_node = AE_HIST_BIN_NUM / 2;
	int cp1_val = le_val * (100 - bt_param->contrast_low_UI) / 100;

	int cp2_node = (AE_HIST_BIN_NUM + iBTnbins - 1) / 2;
	int cp2_val = le_val + ((right_val - le_val) * bt_param->contrast_high_UI / 100);

	TPCHIP_Info stPCHIP;
	TPCHIP_PrivateInfo stPChip_Private;

	CVI_S32 as32XIn[5] = {0, cp1_node, le_node, cp2_node, right_node};
	CVI_S32 as32YIn[5] = {0, cp1_val, le_val, cp2_val, right_val};

	stPCHIP.ps32XIn = as32XIn;
	stPCHIP.ps32YIn = as32YIn;
	stPCHIP.ps32YOut = NULL;
	stPCHIP.pu16Temp = NULL;

	stPCHIP.s32MinYValue = 0;
	stPCHIP.s32MaxYValue = DRC_OUT_MAX_VAL;
	stPCHIP.u32SizeIn = 5;
	stPCHIP.u32SizeOut = right_node + 1;

	if (PCHIP_InterP1_PCHIP_V2_Preprocess(&stPCHIP, &stPChip_Private) != CVI_SUCCESS) {
		ISP_ALGO_LOG_WARNING("DRC PChip fail (P1)!!\n");
		ISP_ALGO_LOG_WARNING("nbins : %4d, pchip (%4d, %4d), (%4d, %4d), (%4d, %4d), (%4d, %4d)\n",
			iBTnbins,
			cp1_node, cp1_val, le_node, le_val, cp2_node, cp2_val, right_node, right_val);

		stPCHIP.u32SizeIn = 3;

		if (PCHIP_InterP1_PCHIP_V2_Preprocess(&stPCHIP, &stPChip_Private) != CVI_SUCCESS) {
			ISP_ALGO_LOG_WARNING("DRC PChip fail (P2)!!\n");
			ISP_ALGO_LOG_WARNING("nbins : %4d, pchip (%4d, %4d), (%4d, %4d), (%4d, %4d), (%4d, %4d)\n",
				iBTnbins,
				cp1_node, cp1_val, le_node, le_val, cp2_node, cp2_val, right_node, right_val);
		}
	}

	int node_step = 1 << (bt_param->ltm_b_curve_quan_bit - LTM_G_CURVE_0_QUAN_BIT);

	int node_idx, node_step_idx;
	CVI_S32 s32PChipOut;

	node_step_idx = 0;
	for (node_idx = 0; node_idx < LTM_BRIGHT_CURVE_NODE_NUM; node_idx++) {
		if (node_step_idx < iBTnbins) {
			PCHIP_InterP1_PCHIP_V2_Process(node_step_idx, &stPCHIP, &stPChip_Private, &s32PChipOut);
			brit_tone[node_idx] = s32PChipOut;
		} else {
			brit_tone[node_idx] = DRC_TONE_CURVE_MAX_VALUE;
		}

		node_step_idx += node_step;
	}
}

static int DRC_7_6(uint32_t *global_tone, uint32_t *dark_tone, uint32_t *brit_tone, uint32_t *le_histogram,
	uint32_t *global_tone_se_step,
	TCM_Param *cm_param, TGT_Param *gt_param, TDTA_Param *dta_param, TBT_Param *bt_param)
{
	/* --------------------------- get statistics info -------------------------- */
	get_dark_tone_adapt_info(dta_param, le_histogram, cm_param);

	/* ------------------------------- global tone ------------------------------ */
	gen_drc_global_tone(gt_param, cm_param, global_tone, global_tone_se_step);

	/* -------------------------------- dark tone ------------------------------- */
	get_linear_drc_dark_tone_adapt_param(global_tone, cm_param, dta_param);
	gen_drc_dark_tone_adaptive(global_tone, dta_param, dark_tone);

	/* ------------------------------- bright tone ------------------------------ */
	gen_drc_bright_tone(global_tone, cm_param, bt_param, brit_tone);

	return 0;
}

static void linear_drc_tone_protect(uint32_t *tone_curve, int start, int end)
{
	for (int i = start; i < end; i++) {
		tone_curve[i] = DRC_TONE_CURVE_MAX_VALUE;
	}
}

static void gen_linear_drc_global_tone(const TGT_Param *gt_param, const TCM_Param *cm_param, uint32_t *global_tone)
{
	int nbins = LTM_DARK_CURVE_NODE_NUM;
	int max_bin_idx, le_avg;
	float T_gt, R_gt;
	int target_Y;
	int tmp;

	max_bin_idx = nbins - 1;
	le_avg = MINMAX(cm_param->avg, 30, 90);

	if (gt_param->target_Y_gain_mode_UI) {
		target_Y = le_avg * (float)gt_param->target_Y_gain_UI / 32.0f;
		int sc_ub_left = 168;
		int sc_upper_bound = sc_ub_left + (float)(255 - sc_ub_left)*le_avg / 255.0f;

		target_Y = MIN(target_Y, sc_upper_bound);
		T_gt = (float)target_Y / 255.0f;
	} else {
		target_Y = MINMAX(gt_param->target_Y_UI, 1, 254);
		T_gt = (float)target_Y / 255.0f;
	}

	R_gt = (float)le_avg / max_bin_idx;

	// get linear tone
	int linear_tone[nbins];

	for (int i = 0; i < nbins; i++) {
		linear_tone[i] = DRC_OUT_MIN_VAL + ((DRC_OUT_MAX_VAL - DRC_OUT_MIN_VAL) * i / max_bin_idx);
	}

	if (R_gt < T_gt) {
		float Lw_gt = (R_gt * R_gt - T_gt) / (T_gt * R_gt - R_gt);
		float Lw2_gt = Lw_gt * Lw_gt;
		float Lx_gt, L_gt;

		for (int i = 0; i < nbins; i++) {
			Lx_gt = (float)i / max_bin_idx;
			L_gt = Lx_gt * Lw_gt;
			tmp = floor(DRC_OUT_MAX_VAL * L_gt * (1 + L_gt / Lw2_gt) / (1 + L_gt));
			global_tone[i] = MINMAX(tmp, DRC_OUT_MIN_VAL, DRC_OUT_MAX_VAL);
		}
	} else {
		for (int i = 0; i < nbins; i++) {
			global_tone[i] = linear_tone[i];
		}
	}

	for (int i = 0; i < nbins; i++) {
		int tmp = gt_param->global_tone_strength_UI * global_tone[i];

		tmp = (tmp + (256 - gt_param->global_tone_strength_UI) * linear_tone[i]) / 256;
		global_tone[i] = MINMAX(tmp, DRC_OUT_MIN_VAL, DRC_OUT_MAX_VAL);
	}
}

static void gen_linear_drc_dark_tone_adaptive(const uint32_t *global_tone,
	const TDTA_Param *dta_param, uint32_t *dark_tone)
{
	if (dta_param->bAdaptValid) {
		float Lw_dt, Lw2_dt;
		int dark_start, dark_range, dark_end;
		int min_val_dt, max_val_dt, delta_dt;

		Lw_dt = dta_param->Lw_dt;
		Lw2_dt = Lw_dt * Lw_dt;
		dark_start = dta_param->dark_offset;
		dark_end = dta_param->dark_end;
		dark_range = dark_end - dark_start;
		min_val_dt = global_tone[dark_start];
		max_val_dt = global_tone[dark_end];
		delta_dt = max_val_dt - min_val_dt;

		ISP_ALGO_DEBUG(LOG_DEBUG, "[Lw_dt] = %6.3f\n", dta_param->Lw_dt);
		ISP_ALGO_DEBUG(LOG_DEBUG, "[dark_start] = %d, [dark_range] = %d, [dark_end] = %d\n",
			dark_start, dark_range, dark_end);
		ISP_ALGO_DEBUG(LOG_DEBUG, "[min_val_dt] = %d, [max_val_dt] = %d\n",
			min_val_dt, max_val_dt);

		/* ----------------------------------- v2 ----------------------------------- */
		for (int i = 0; i < LTM_D_CURVE_NODE_NUM; i++) {
			if (i < dark_start) {
				dark_tone[i] = global_tone[i];
			} else if (i < dark_end) {
				float ratio = (float)(i - dark_start) / dark_range;
				float L = ratio * Lw_dt;
				int mapped_n = min_val_dt + floor(delta_dt * L * (1 + L / Lw2_dt) / (1 + L));

				dark_tone[i] = MINMAX(mapped_n, min_val_dt, max_val_dt);
			} else {
				dark_tone[i] = global_tone[i];
			}
		}
	} else {
		for (int i = 0; i < LTM_D_CURVE_NODE_NUM; i++) {
			dark_tone[i] = global_tone[i];
		}
		ISP_ALGO_DEBUG(LOG_DEBUG, "[Linear DRC dta bypass]\n");
	}
}

static void gen_linear_drc_bright_tone(const uint32_t *global_tone,
	const TCM_Param *cm_param, const TBT_Param *bt_param, const TDTA_Param *dta_param,
	uint32_t *brit_tone)
{
#define PCHIP_SDR_BRIGHT_TONE_OUT_MEM_SIZE		(1028)		// ((AE_HIST_BIN_NUM + 1) * S32) = (256 + 1) * 4
	int nbins = cm_param->wdr_bin_num;

	int dark_end = MINMAX(dta_param->dark_end, 2, 254);

	// global tone upper bound
	int gt_ub = global_tone[dark_end];
	int linear_val = 65535 * dark_end / AE_HIST_BIN_NUM;
	int diff_gt_ub_linear = MAX((gt_ub - linear_val), 0);

	ISP_ALGO_DEBUG(LOG_DEBUG, "[gt_ub] = %d, [linear_val]= %d, [diff_gt_ub_linear] = %d\n",
		gt_ub, linear_val, diff_gt_ub_linear);

	int le_val = linear_val + (diff_gt_ub_linear * bt_param->inflection_point_luma_UI / 100);

	ISP_ALGO_DEBUG(LOG_DEBUG, "[le_val] = %d, [dark_end]= %d\n",
		le_val, dark_end);

	int cp1_node = MAX(dark_end / 2, 1);	// min = 1
	int cp1_val = le_val * (100 - bt_param->contrast_low_UI) / 100;

	int cp2_node = MIN((AE_HIST_BIN_NUM + dark_end) / 2, 255);	// max = 255
	int cp2_val = le_val + ((DRC_OUT_MAX_VAL - le_val) * bt_param->contrast_high_UI / 100);

	TPCHIP_Info stPCHIP;

	CVI_S32 as32XIn[5] = {0, cp1_node, dark_end, cp2_node, AE_HIST_BIN_NUM};
	CVI_S32 as32YIn[5] = {0, cp1_val, le_val, cp2_val, DRC_OUT_MAX_VAL};

	stPCHIP.ps32XIn = as32XIn;
	stPCHIP.ps32YIn = as32YIn;
	stPCHIP.ps32YOut = (CVI_S32 *)drc_wbuf_256k;
	stPCHIP.pu16Temp = (CVI_U16 *)(drc_wbuf_256k + PCHIP_SDR_BRIGHT_TONE_OUT_MEM_SIZE);

	stPCHIP.s32MinYValue = 0;
	stPCHIP.s32MaxYValue = DRC_OUT_MAX_VAL;
	stPCHIP.u32SizeIn = 5;
	stPCHIP.u32SizeOut = AE_HIST_BIN_NUM + 1;

	if (PCHIP_InterP1_PCHIP(&stPCHIP) != CVI_SUCCESS) {
		ISP_ALGO_LOG_WARNING("DRC PChip fail (P1)!!\n");
		ISP_ALGO_LOG_WARNING("nbins : %4d, pchip (%4d, %4d), (%4d, %4d), (%4d, %4d), (%4d, %4d)\n",
			nbins,
			cp1_node, cp1_val, dark_end, le_val, cp2_node, cp2_val,
			AE_HIST_BIN_NUM, DRC_OUT_MAX_VAL);

		stPCHIP.u32SizeIn = 3;

		if (PCHIP_InterP1_PCHIP(&stPCHIP) != CVI_SUCCESS) {
			ISP_ALGO_LOG_WARNING("DRC PChip fail (P2)!!\n");
			ISP_ALGO_LOG_WARNING("nbins : %4d, pchip (%4d, %4d), (%4d, %4d), (%4d, %4d), (%4d, %4d)\n",
				nbins,
				cp1_node, cp1_val, dark_end, le_val, cp2_node, cp2_val,
				AE_HIST_BIN_NUM, DRC_OUT_MAX_VAL);
		}
	}

	uint32_t *tone_wbuf = (uint32_t *)drc_wbuf_256k;

	for (int i = 0; i < nbins; i++) {
		brit_tone[i] = tone_wbuf[i];
	}
}

static void linear_drc_dta_proc(uint32_t *global_tone, uint32_t *dark_tone, uint32_t *brit_tone,
	uint32_t *le_histogram, TCM_Param *cm_param, TGT_Param *gt_param, TDTA_Param *dta_param, TBT_Param *bt_param)
{
	/* --------------------------- get statistics info -------------------------- */
	get_dark_tone_adapt_info(dta_param, le_histogram, cm_param);

	/* ------------------------------- global tone ------------------------------ */
	gen_linear_drc_global_tone(gt_param, cm_param, global_tone);

	/* -------------------------------- dark tone ------------------------------- */
	get_linear_drc_dark_tone_adapt_param(global_tone, cm_param, dta_param);
	gen_linear_drc_dark_tone_adaptive(global_tone, dta_param, dark_tone);

	/* ------------------------------- bright tone ------------------------------ */
	gen_linear_drc_bright_tone(global_tone, cm_param, bt_param, dta_param, brit_tone);

	// protect out of LE range case (input > DRC_TONE_CURVE_MAX_VALUE => output = DRC_TONE_CURVE_MAX_VALUE)
	linear_drc_tone_protect(global_tone, MAX_HIST_BINS, LTM_G_CURVE_TOTAL_NODE_NUM);
	linear_drc_tone_protect(dark_tone, MAX_HIST_BINS, LTM_D_CURVE_NODE_NUM);
	linear_drc_tone_protect(brit_tone, MAX_HIST_BINS, LTM_B_CURVE_NODE_NUM);
}

static int DRC_Blending_API(uint32_t *pMapHist, uint32_t *pMapHist_prev, uint32_t *pMapHist_output, int prebinNum,
	int bins_num, int weight_prev)
{
	const int wt_cur = 1;
	int pre_weight;
	int add_on;

	// if average frame = 0. let
	pre_weight = (weight_prev < wt_cur) ? 0 : weight_prev;
	if (bins_num == 0) {
		for (int n = 0; n < bins_num ; n++) {
			pMapHist_prev[n] = pMapHist_output[n] = pMapHist[n];
		}
	} else {
		for (int n = 0; n < bins_num ; n++) {
			add_on = (pMapHist[n] > pMapHist_prev[n]) ? 1 : 0;
			pMapHist_prev[n] = pMapHist_output[n] = (pMapHist[n] * wt_cur + pMapHist_prev[n]
				* pre_weight) / (pre_weight + wt_cur) + add_on;
			pMapHist_prev[n] = MIN(pMapHist_prev[n], DRC_TONE_CURVE_MAX_VALUE);
			pMapHist_output[n] = MIN(pMapHist_output[n], DRC_TONE_CURVE_MAX_VALUE);
		}
	}

	UNUSED(prebinNum);

	return 0;
}

/* -------------------- debug 182x simulation mode start -------------------- */
static int generate_flt_distance_coeff(int *dist_wgt)
{
	for (int i = 0; i < LTM_DIST_WEIGHT_LUT_NUM; i++) {
		dist_wgt[i] = LTM_DIST_WEIGHT_MAX;		// 31
	}

	return 0;
}

static int generate_flt_distance_normalization_coeff(int rng, int iLmapThr, int *dist_wgt, int *shift, int *gain)
{
	int blk_sum, blk_max_val;
	int iShift, iGain;

	blk_sum = iLmapThr * 31 * 121;
	iShift = (int)(log2f((float)blk_sum)) - 7;
	iShift = MAX(iShift, 0);

	blk_max_val = MAX((blk_sum >> iShift), 128);
	iGain = (int)(((255 << 10) + blk_max_val - 1) / blk_max_val);

	*shift = iShift;
	*gain = iGain;

	UNUSED(rng);
	UNUSED(*dist_wgt);

	return 0;
}
/* --------------------- debug 182x simulation mode end --------------------- */
