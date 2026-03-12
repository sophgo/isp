/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_ldci.c
 * Description:
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
// #include "limits.h"

#include "isp_algo_ldci.h"
#include "isp_algo_debug.h"
#include "isp_algo_utility.h"

#define LDCI_TONE_CURVE_LUT_NUM			(8)

#define LDCI_TONE_CURVE_BITS			(4)
#define LDCI_TONE_CURVE_LUT_SIZE		((1 << LDCI_TONE_CURVE_BITS) + 1)
#define LDCI_BLOCKS_X					(16)
#define LDCI_BLOCKS_Y					(12)

#define LDCI_IDX_FILTER_SIZE			(9)			// index_filter: [1, 9]
#define LDCI_IDX_FILTER_SUM_MAX			(1024)
#define LDCI_IDX_FILTER_N_BITS			(16)		// Q0.16

#define LDCI_VAR_FILTER_SIZE			(5)			// var_filter: [1, 5]
#define LDCI_VAR_FILTER_SUM_MAX			(512)
#define LDCI_VAR_FILTER_N_BITS			(16)		// Q0.16

#define LDCI_LUMA_WEIGHT_LUT_SIZE		(33)
#define LDCI_LPF_SIGMA_UNIT_GAIN_32		(32)		// 1x gain = 32
#define LDCI_LPF_SIGMA_UNIT_GAIN_8		(8)			// 1x gain = 8

#define LDCI_NORM_Y						(13)
#define LDCI_NORM_X						(14)
#define LDCI_BILINEAR_INTERP_NORM_BITS	(16)

#define LDCI_TONE_CURVE_NODE_SIZE		(4)

#define MEMORY_IDX_MAP_FILTER			(sizeof(float) * LDCI_IDX_FILTER_SIZE * LDCI_IDX_FILTER_SIZE)
#define MEMORY_VAR_MAP_FILTER			(sizeof(float) * LDCI_VAR_FILTER_SIZE * LDCI_VAR_FILTER_SIZE)
#define MEMORY_LUMA_WEIGHT_LUT			(sizeof(int) * LDCI_LUMA_WEIGHT_LUT_SIZE)
#define MEMORY_LUMA_WEIGHT_TEMP			(sizeof(float) * LDCI_LUMA_WEIGHT_LUT_SIZE)
#define MEMORY_LUMA_WEIGHT				(MEMORY_LUMA_WEIGHT_LUT + MEMORY_LUMA_WEIGHT_TEMP)
#define MEMORY_LDCI_TONE_CURVE			(sizeof(int) * LDCI_TONE_CURVE_LUT_NUM * LDCI_TONE_CURVE_LUT_SIZE)


static void create_2D_Gaussian_kernel(int filter_length, int sigma, float *kernel);
static void create_1D_Gaussian_kernel(int Mean, int Sigma, int Wgt,
	int filter_size, int *kernel, float *tempbuf);
static int ldci_tone_curve_gen_sw(
	int *tone_curve_lut, int curve_lut_line_count, int curve_lut_line_size,
	float *x1, float *x2,
	int iDarkContrastLow, int iDarkContrastHigh, int iBrightContrastLow, int iBrightCOntrastHigh,
	int iDarkContrastLow2, int iDarkContrastHigh2, int iBrightContrastLow2, int iBrightCOntrastHigh2
);
static CVI_S32 isp_algo_ldci_update_idx_map_filter(
	struct ldci_param_in *ldci_param_in, struct ldci_param_out *ldci_param_out);
static CVI_S32 isp_algo_ldci_update_var_map_filter(
	struct ldci_param_in *ldci_param_in, struct ldci_param_out *ldci_param_out);
static CVI_S32 isp_algo_ldci_update_luma_based_weight_lut(
	struct ldci_param_in *ldci_param_in, struct ldci_param_out *ldci_param_out);
static CVI_S32 isp_algo_ldci_update_tone_curve(
	struct ldci_param_in *ldci_param_in, struct ldci_param_out *ldci_param_out);

CVI_S32 isp_algo_ldci_init(void)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	return ret;
}

CVI_S32 isp_algo_ldci_uninit(void)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	return ret;
}

CVI_S32 isp_algo_ldci_get_internal_memory(CVI_U32 *memory_size)
{
	CVI_S32 ret = CVI_SUCCESS;
	CVI_U32 u32MemoryUsage = 0;

	u32MemoryUsage = MAX(MEMORY_IDX_MAP_FILTER, u32MemoryUsage);
	u32MemoryUsage = MAX(MEMORY_VAR_MAP_FILTER, u32MemoryUsage);
	u32MemoryUsage = MAX(MEMORY_LUMA_WEIGHT, u32MemoryUsage);
	u32MemoryUsage = MAX(MEMORY_LDCI_TONE_CURVE, u32MemoryUsage);

	*memory_size = u32MemoryUsage + 8;

	return ret;
}

CVI_S32 isp_algo_ldci_get_init_coef(int W, int H, struct ldci_init_coef *pldci_init_coef)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	int BlockSizeX, BlockSizeY, SubBlockSizeX, SubBlockSizeY;
	int BlockSizeX1, BlockSizeY1, SubBlockSizeX1, SubBlockSizeY1;
	int line_mean_num, line_var_num;

	if ((H % LDCI_BLOCKS_Y == 0)) {		// only for Y (%12), while X (%16 is easy)
		pldci_init_coef->u8ImageSizeDivBy16x12 = 1;
	} else {
		pldci_init_coef->u8ImageSizeDivBy16x12 = 0;
	}
	BlockSizeX = (W % LDCI_BLOCKS_X == 0) ? (W / LDCI_BLOCKS_X) : (W / LDCI_BLOCKS_X) + 1; // Width of one block
	BlockSizeY = (H % LDCI_BLOCKS_Y == 0) ? (H / LDCI_BLOCKS_Y) : (H / LDCI_BLOCKS_Y) + 1; // Height of one block
	SubBlockSizeX = (BlockSizeX >> 1);
	SubBlockSizeY = (BlockSizeY >> 1);
	line_mean_num = (BlockSizeY / 2) + (BlockSizeY % 2);
	line_var_num  = (BlockSizeY / 2);

	if (W % LDCI_BLOCKS_X == 0) {
		BlockSizeX1 = BlockSizeX; // (BlockSizeX % 2 == 0) ? BlockSizeX : BlockSizeX + 1;
		SubBlockSizeX1 = W - BlockSizeX * (LDCI_BLOCKS_X - 1) - SubBlockSizeX; // SubBlockSizeX;
	} else {
		int dW = BlockSizeX * LDCI_BLOCKS_X - W;

		// BlockSizeX1 = (BlockSizeX % 2 == 0) ?
		//	SubBlockSizeX + BlockSizeX - dW : SubBlockSizeX + BlockSizeX - dW + 1;
		BlockSizeX1 = 2 * BlockSizeX - SubBlockSizeX - dW;
		SubBlockSizeX1 = 0;
	}

	if (H % LDCI_BLOCKS_Y == 0) {
		BlockSizeY1 = BlockSizeY; //(BlockSizeY % 2 == 0) ? BlockSizeY : BlockSizeY + 1;
		SubBlockSizeY1 = H - BlockSizeY * (LDCI_BLOCKS_Y - 1) - SubBlockSizeY; // SubBlockSizeY;
	} else {
		int dH = BlockSizeY * LDCI_BLOCKS_Y - H;

		BlockSizeY1 = 2 * BlockSizeY - SubBlockSizeY - dH;
		SubBlockSizeY1 = 0;
	}

	// Set to HW Registers
	pldci_init_coef->u16BlkSizeX		= BlockSizeX;
	pldci_init_coef->u16BlkSizeY		= BlockSizeY;
	pldci_init_coef->u16BlkSizeX1		= BlockSizeX1;
	pldci_init_coef->u16BlkSizeY1		= BlockSizeY1;

	pldci_init_coef->u16SubBlkSizeX		= SubBlockSizeX;
	pldci_init_coef->u16SubBlkSizeY		= SubBlockSizeY;
	pldci_init_coef->u16SubBlkSizeX1	= SubBlockSizeX1;
	pldci_init_coef->u16SubBlkSizeY1	= SubBlockSizeY1;

	pldci_init_coef->u16InterpNormUD =
		(BlockSizeY == 0) ? 0 : (1 << LDCI_BILINEAR_INTERP_NORM_BITS) / BlockSizeY;
	pldci_init_coef->u16InterpNormUD1 =
		(BlockSizeY1 == 0) ? 0 : (1 << LDCI_BILINEAR_INTERP_NORM_BITS) / BlockSizeY1;
	pldci_init_coef->u16SubInterpNormUD =
		(SubBlockSizeY == 0) ? 0 : (1 << LDCI_BILINEAR_INTERP_NORM_BITS) / SubBlockSizeY;
	pldci_init_coef->u16SubInterpNormUD1 =
		(SubBlockSizeY1 == 0) ? 0 : (1 << LDCI_BILINEAR_INTERP_NORM_BITS) / SubBlockSizeY1;

	pldci_init_coef->u16InterpNormLR =
		(BlockSizeX == 0) ? 0 : (1 << LDCI_BILINEAR_INTERP_NORM_BITS) / BlockSizeX;
	pldci_init_coef->u16InterpNormLR1 =
		(BlockSizeX1 == 0) ? 0 : (1 << LDCI_BILINEAR_INTERP_NORM_BITS) / BlockSizeX1;
	pldci_init_coef->u16SubInterpNormLR =
		(SubBlockSizeX == 0) ? 0 : (1 << LDCI_BILINEAR_INTERP_NORM_BITS) / SubBlockSizeX;
	pldci_init_coef->u16SubInterpNormLR1 =
		(SubBlockSizeX1 == 0) ? 0 : (1 << LDCI_BILINEAR_INTERP_NORM_BITS) / SubBlockSizeX1;

	pldci_init_coef->u16MeanNormX		= (1 << LDCI_NORM_X) / MAX(BlockSizeX, 1);
	pldci_init_coef->u16MeanNormY		= (1 << LDCI_NORM_Y) / MAX(line_mean_num, 1);
	pldci_init_coef->u16VarNormY		= (1 << LDCI_NORM_Y) / MAX(line_var_num, 1);

	return ret;
}

CVI_S32 isp_algo_ldci_main(struct ldci_param_in *ldci_param_in, struct ldci_param_out *ldci_param_out)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	if (ldci_param_in->pvIntMemory == CVI_NULL) {
		return CVI_FAILURE;
	}

	isp_algo_ldci_update_idx_map_filter(ldci_param_in, ldci_param_out);
	isp_algo_ldci_update_var_map_filter(ldci_param_in, ldci_param_out);
	isp_algo_ldci_update_luma_based_weight_lut(ldci_param_in, ldci_param_out);
	isp_algo_ldci_update_tone_curve(ldci_param_in, ldci_param_out);

	return ret;
}

static CVI_S32 isp_algo_ldci_update_idx_map_filter(
	struct ldci_param_in *ldci_param_in, struct ldci_param_out *ldci_param_out)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	CVI_U8 u8IdxFilterLPFSigma = ldci_param_in->u8GaussLPFSigma;
	int SumW = 0;

	float *pfFilterIdx = ISP_PTR_CAST_FLOAT(ldci_param_in->pvIntMemory);
	CVI_U16	*pu16IdxFilterLut = ldci_param_out->au16IdxFilterLut;

	create_2D_Gaussian_kernel(LDCI_IDX_FILTER_SIZE, u8IdxFilterLPFSigma, pfFilterIdx);

	pu16IdxFilterLut[0] = (int)(pfFilterIdx[0] * LDCI_IDX_FILTER_SUM_MAX);
	pu16IdxFilterLut[1] = (int)(pfFilterIdx[1] * LDCI_IDX_FILTER_SUM_MAX);
	pu16IdxFilterLut[2] = (int)(pfFilterIdx[2] * LDCI_IDX_FILTER_SUM_MAX);
	pu16IdxFilterLut[3] = (int)(pfFilterIdx[3] * LDCI_IDX_FILTER_SUM_MAX);
	pu16IdxFilterLut[4] = (int)(pfFilterIdx[4] * LDCI_IDX_FILTER_SUM_MAX);
	pu16IdxFilterLut[5] = (int)(pfFilterIdx[10] * LDCI_IDX_FILTER_SUM_MAX);
	pu16IdxFilterLut[6] = (int)(pfFilterIdx[11] * LDCI_IDX_FILTER_SUM_MAX);
	pu16IdxFilterLut[7] = (int)(pfFilterIdx[12] * LDCI_IDX_FILTER_SUM_MAX);
	pu16IdxFilterLut[8] = (int)(pfFilterIdx[13] * LDCI_IDX_FILTER_SUM_MAX);
	pu16IdxFilterLut[9] = (int)(pfFilterIdx[20] * LDCI_IDX_FILTER_SUM_MAX);
	pu16IdxFilterLut[10] = (int)(pfFilterIdx[21] * LDCI_IDX_FILTER_SUM_MAX);
	pu16IdxFilterLut[11] = (int)(pfFilterIdx[22] * LDCI_IDX_FILTER_SUM_MAX);
	pu16IdxFilterLut[12] = (int)(pfFilterIdx[30] * LDCI_IDX_FILTER_SUM_MAX);
	pu16IdxFilterLut[13] = (int)(pfFilterIdx[31] * LDCI_IDX_FILTER_SUM_MAX);
	pu16IdxFilterLut[14] = (int)(pfFilterIdx[40] * LDCI_IDX_FILTER_SUM_MAX);

	SumW = 0;
	for (CVI_U32 u32Idx = 0; u32Idx < (LDCI_IDX_FILTER_SIZE * LDCI_IDX_FILTER_SIZE); ++u32Idx) {
		int coeff = (int)(pfFilterIdx[u32Idx] * LDCI_IDX_FILTER_SUM_MAX);
		// ldci_reg.hw.reg_ldci_idx_filter_lut[n] = coeff;

		SumW += coeff;
	}

	if (SumW > 0) {
		ldci_param_out->u16IdxFilterNorm =
			MIN((1 << LDCI_IDX_FILTER_N_BITS) / SumW, (1 << LDCI_IDX_FILTER_N_BITS) - 1);
	} else {
		ldci_param_out->u16IdxFilterNorm = 1;
	}

	return ret;
}

static CVI_S32 isp_algo_ldci_update_var_map_filter(
	struct ldci_param_in *ldci_param_in, struct ldci_param_out *ldci_param_out)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	CVI_U8 u8VarFilterLPFSigma = ldci_param_in->u8GaussLPFSigma;
	int SumW = 0;

	float *pfFilterVar = ISP_PTR_CAST_FLOAT(ldci_param_in->pvIntMemory);
	CVI_U16	*pu16VarFilterLut = ldci_param_out->au16VarFilterLut;

	create_2D_Gaussian_kernel(LDCI_VAR_FILTER_SIZE, u8VarFilterLPFSigma, pfFilterVar);

	pu16VarFilterLut[0] = (int)(pfFilterVar[0] * LDCI_VAR_FILTER_SUM_MAX);
	pu16VarFilterLut[1] = (int)(pfFilterVar[1] * LDCI_VAR_FILTER_SUM_MAX);
	pu16VarFilterLut[2] = (int)(pfFilterVar[2] * LDCI_VAR_FILTER_SUM_MAX);
	pu16VarFilterLut[3] = (int)(pfFilterVar[6] * LDCI_VAR_FILTER_SUM_MAX);
	pu16VarFilterLut[4] = (int)(pfFilterVar[7] * LDCI_VAR_FILTER_SUM_MAX);
	pu16VarFilterLut[5] = (int)(pfFilterVar[12] * LDCI_VAR_FILTER_SUM_MAX);

	SumW = 0;
	for (CVI_U32 u32Idx = 0; u32Idx < (LDCI_VAR_FILTER_SIZE * LDCI_VAR_FILTER_SIZE); ++u32Idx) {
		int coeff = (int)(pfFilterVar[u32Idx] * LDCI_VAR_FILTER_SUM_MAX);

		SumW += coeff;
	}

	if (SumW > 0) {
		ldci_param_out->u16VarFilterNorm =
			MIN((1 << LDCI_VAR_FILTER_N_BITS) / SumW, (1 << LDCI_VAR_FILTER_N_BITS) - 1);
	} else {
		ldci_param_out->u16VarFilterNorm = 1;
	}

	return ret;
}

static CVI_S32 isp_algo_ldci_update_luma_based_weight_lut(
	struct ldci_param_in *ldci_param_in, struct ldci_param_out *ldci_param_out)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	// Luma-based Weight LUT
	int Wgt		= ldci_param_in->u8LumaPosWgt_Wgt;
	int Sigma	= ldci_param_in->u8LumaPosWgt_Sigma;
	int Mean	= ldci_param_in->u8LumaPosWgt_Mean;

	int *reg_ldci_luma_wgt_lut = ISP_PTR_CAST_S32(ldci_param_in->pvIntMemory);
	ISP_VOID_PTR temp = ldci_param_in->pvIntMemory + MEMORY_LUMA_WEIGHT_LUT;
	float *tempbuf = ISP_PTR_CAST_FLOAT(ALIGN(temp, 8)); // TODO: mason.zou

	create_1D_Gaussian_kernel(Mean, Sigma, Wgt,
		LDCI_LUMA_WEIGHT_LUT_SIZE, reg_ldci_luma_wgt_lut, tempbuf);

	CVI_U8	*pu8LumaWgtLut = ldci_param_out->au8LumaWgtLut;

	for (CVI_U32 u32Idx = 0; u32Idx < LDCI_LUMA_WEIGHT_LUT_SIZE; ++u32Idx) {
		pu8LumaWgtLut[u32Idx] = reg_ldci_luma_wgt_lut[u32Idx];
	}

	return ret;
}

static CVI_S32 isp_algo_ldci_update_tone_curve(
	struct ldci_param_in *ldci_param_in, struct ldci_param_out *ldci_param_out)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	float x1[LDCI_TONE_CURVE_LUT_NUM] = { 1, 2, 3, 4, 7, 8, 9, 10 };
	float x2[LDCI_TONE_CURVE_LUT_NUM] = { 7, 8, 9, 10, 10, 11, 12, 16 };

	CVI_U8	*pu8ToneCurveIdx = ldci_param_out->au8ToneCurveIdx;

	for (CVI_U32 u32Idx = 0; u32Idx < LDCI_TONE_CURVE_LUT_NUM; ++u32Idx) {
		pu8ToneCurveIdx[u32Idx] = (int)((x1[u32Idx] + x2[u32Idx] + 1) / 2);
	}

	// LDCI tone curve generation
	int DarkContrastLow		= ldci_param_in->u8DarkContrastLow;
	int DarkContrastHigh	= ldci_param_in->u8DarkContrastHigh;
	int BrightContrastLow	= ldci_param_in->u8BrightContrastLow;
	int BrightCOntrastHigh	= ldci_param_in->u8BrightContrastHigh;
	int DarkContrastLow2	= ldci_param_in->u8DarkContrastLow;
	int DarkContrastHigh2	= ldci_param_in->u8DarkContrastHigh;
	int BrightContrastLow2	= ldci_param_in->u8BrightContrastLow;
	int BrightCOntrastHigh2	= ldci_param_in->u8BrightContrastHigh;

	int *reg_ldci_tone_curve_lut_all = ISP_PTR_CAST_S32(ldci_param_in->pvIntMemory);

	ldci_tone_curve_gen_sw(
		reg_ldci_tone_curve_lut_all, LDCI_TONE_CURVE_LUT_NUM, LDCI_TONE_CURVE_LUT_SIZE,
		x1, x2,
		DarkContrastLow, DarkContrastHigh, BrightContrastLow, BrightCOntrastHigh,
		DarkContrastLow2, DarkContrastHigh2, BrightContrastLow2, BrightCOntrastHigh2
	);

	CVI_U16	*pu16ToneCurveLut = NULL;
	int		*reg_ldci_tone_curve_lut = NULL;

	reg_ldci_tone_curve_lut = reg_ldci_tone_curve_lut_all;
	for (CVI_U32 u32LutIdx = 0; u32LutIdx < LDCI_TONE_CURVE_LUT_NUM; ++u32LutIdx) {
		pu16ToneCurveLut = ldci_param_out->au16ToneCurveLut[u32LutIdx];

		for (CVI_U32 u32Idx = 0; u32Idx < LDCI_TONE_CURVE_LUT_SIZE; ++u32Idx) {
			pu16ToneCurveLut[u32Idx] = reg_ldci_tone_curve_lut[u32Idx];
		}

		reg_ldci_tone_curve_lut += LDCI_TONE_CURVE_LUT_SIZE;
	}

	return ret;
}

static void create_2D_Gaussian_kernel(int filter_length, int sigma, float *kernel)
{
	int r = (filter_length - 1) / 2;
	int filter_size = filter_length * filter_length;
	float fSigma = MAX(((float)sigma) / LDCI_LPF_SIGMA_UNIT_GAIN_32, 1e-5);
	float sigma2 = 2.0 * fSigma * fSigma;
	float sum_kernel;

	memset(kernel, (float)0, sizeof(float) * filter_size);

	sum_kernel = 0.0;
	for (int y = -r; y <= r; y++) {
		for (int x = -r; x <= r; x++) {
			kernel[(y + r) * filter_length + (x + r)] = exp(-(y * y + x * x) / MAX(sigma2, 1e-5));
			sum_kernel += kernel[(y + r) * filter_length + (x + r)];
		}
	}

	for (int n = 0; n < filter_size; n++) {
		kernel[n] /= sum_kernel;
	}
}

// kernel => int kernel[filter_size]
// tempbuf => float tempbuf[filter_size]
static void create_1D_Gaussian_kernel(int Mean, int Sigma, int Wgt,
	int filter_size, int *kernel, float *tempbuf)
{
	int x0 = 0;
	int x1 = filter_size - 1;
	float fSigma = MAX(((float)Sigma) / LDCI_LPF_SIGMA_UNIT_GAIN_8, 1e-5);
	float fMean = (float)Mean / (255.0 / filter_size);
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

static void ContrastSet_Gen(float ContrastStrStart, float ContrastStrEnd, const int ContrastSize, float *ContrastSet)
{
	float delta = (ContrastStrStart - ContrastStrEnd) / (ContrastSize - 1);

	for (int n = 0; n < 4; n++) {
		ContrastSet[n] = ContrastStrStart - delta * n;
	}
}

static int ldci_tone_curve_s(int *LUT, float x1, float x2, float y1, float y2)
{
	if (LUT == NULL) {
		return -1;
	}

	const int idx_max = LDCI_TONE_CURVE_LUT_SIZE;
	float fx[4] = { 0, x1, x2, 1 };
	float fy[4] = { 0, y1, y2, 1 };
	int node_x[4] = {
		(int)(fx[0] * idx_max + 0.5),
		(int)(fx[1] * idx_max + 0.5),
		(int)(fx[2] * idx_max + 0.5),
		(int)(fx[3] * idx_max + 0.5)
	};
	int node_y[4] = {
		(int)(fy[0] * 1023 + 0.5),
		(int)(fy[1] * 1023 + 0.5),
		(int)(fy[2] * 1023 + 0.5),
		(int)(fy[3] * 1023 + 0.5)
	};

	for (int n = 0; n < idx_max; n++) {
		if (n == 0) {
			LUT[n] = 0;
		} else if (n == idx_max) {
			// Do nothing
		} else {
			for (int k = 0; k < 3; k++) {
				if (node_x[k] < n && n <= node_x[k+1]) {
					int L = node_x[k+1] - node_x[k];
					int idx = n - node_x[k];
					int idx_next = node_x[k+1] - n;

					LUT[n] = ((idx * node_y[k+1]) + (idx_next * node_y[k]) + (L / 2)) / L;
				}
			}
		}
	}

	return 0;
}

static int ldci_tone_curve_gen_sw(
	int *tone_curve_lut, int curve_lut_line_count, int curve_lut_line_size,
	float *x1, float *x2,
	int iDarkContrastLow, int iDarkContrastHigh, int iBrightContrastLow, int iBrightCOntrastHigh,
	int iDarkContrastLow2, int iDarkContrastHigh2, int iBrightContrastLow2, int iBrightCOntrastHigh2
)
{
	float DarkContrastLow		= iDarkContrastLow / 100.0f;
	float DarkContrastHigh		= iDarkContrastHigh / 100.0f;
	float BrightContrastLow		= iBrightContrastLow / 100.0f;
	float BrightCOntrastHigh	= iBrightCOntrastHigh / 100.0f;
	float DarkContrastLow2		= iDarkContrastLow2 / 100.0f;
	float DarkContrastHigh2		= iDarkContrastHigh2 / 100.0f;
	float BrightContrastLow2	= iBrightContrastLow2 / 100.0f;
	float BrightCOntrastHigh2	= iBrightCOntrastHigh2 / 100.0f;

	float afDLow[LDCI_TONE_CURVE_NODE_SIZE];
	float afDHigh[LDCI_TONE_CURVE_NODE_SIZE];
	float afBLow[LDCI_TONE_CURVE_NODE_SIZE];
	float afBHigh[LDCI_TONE_CURVE_NODE_SIZE];
	float afSlopeLow[LDCI_TONE_CURVE_NODE_SIZE + LDCI_TONE_CURVE_NODE_SIZE];
	float afSlopeHigh[LDCI_TONE_CURVE_NODE_SIZE + LDCI_TONE_CURVE_NODE_SIZE];

	ContrastSet_Gen(DarkContrastLow, DarkContrastLow2, LDCI_TONE_CURVE_NODE_SIZE, afDLow);
	ContrastSet_Gen(DarkContrastHigh, DarkContrastHigh2, LDCI_TONE_CURVE_NODE_SIZE, afDHigh);
	ContrastSet_Gen(BrightContrastLow, BrightContrastLow2, LDCI_TONE_CURVE_NODE_SIZE, afBLow);
	ContrastSet_Gen(BrightCOntrastHigh, BrightCOntrastHigh2, LDCI_TONE_CURVE_NODE_SIZE, afBHigh);

	for (int n = 0; n < LDCI_TONE_CURVE_NODE_SIZE; n++) {
		afSlopeLow[n]								= 1.0f - afDLow[n];
		afSlopeHigh[n]								= 1.0f - afDHigh[n];
		afSlopeLow[n + LDCI_TONE_CURVE_NODE_SIZE]	= 1.0f - afBLow[n];
		afSlopeHigh[n + LDCI_TONE_CURVE_NODE_SIZE]	= 1.0f - afBHigh[n];
	}

	const int idx_max = LDCI_TONE_CURVE_LUT_SIZE;
	float y1[LDCI_TONE_CURVE_LUT_NUM] = { 0.0 };
	float y2[LDCI_TONE_CURVE_LUT_NUM] = { 0.0 };

	for (int n = 0; n < LDCI_TONE_CURVE_LUT_NUM; n++) {
		y1[n] = x1[n] * 64.0 * afSlopeLow[n];
		y2[n] = 1023.0 - (idx_max - x2[n]) * 64 * afSlopeHigh[n];

		x1[n] /= idx_max;
		x2[n] /= idx_max;
		y1[n] /= 1023.0;
		y2[n] /= 1023.0;
	}

	int *piCurveLut = tone_curve_lut;

	for (int n = 0; n < LDCI_TONE_CURVE_LUT_NUM; n++, piCurveLut += LDCI_TONE_CURVE_LUT_SIZE) {
		ldci_tone_curve_s(piCurveLut, x1[n], x2[n], y1[n], y2[n]);
	}

	UNUSED(curve_lut_line_count);
	UNUSED(curve_lut_line_size);

	return 0;
}
