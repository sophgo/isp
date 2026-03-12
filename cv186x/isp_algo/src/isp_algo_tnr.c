/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_tnr.c
 * Description:
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "isp_algo_tnr.h"
#include "isp_algo_debug.h"
#include "isp_algo_utility.h"

struct tnr_motion_block_size_threshold {
	CVI_FLOAT fNPChB;
	CVI_FLOAT fNPChG;
	CVI_FLOAT fNPChR;
	CVI_U32 u32Tolerance;
	CVI_U32 u32BlockDim;
};

#define TNR_MOTION_BLOCK_SIZE_RGB2Y(floatB, floatG, floatR) \
	(((floatB) * 0.114) + ((floatG) * 0.587) + ((floatR) * 0.299))
#define SLP_IDX 0
#define OFF_IDX 1
#define COLOR_R 2
#define COLOR_G 1
#define COLOR_B 0
#define CALI_DATA_F_BITS 16
#define NS_MODEL_R 3
#define NS_MODEL_GR 2
#define NS_MODEL_GB 1
#define NS_MODEL_B 0
#define WB_GAIN_F_BITS 10
#define DIGITAL_GAIN_F_BITS 10
#define MANR_NS_SLP_F_BITS 8
#define MANR_NS_SLP_MAX 1023
#define MANR_NS_OFF_MAX 4095
#define FSWDR_WEIGHT_MAX 255
#define EXPOSURE_RATIO_F_BITS 6
#define MANR_LUMA_LUT_SLP_F_BITS 10
#define MANR_IIR_LUT_SLP_F_BITS 6
#define MANR_ALPHA_LUT_SLP_FBIT 4

static CVI_S32 get_manr_noise_profile_reg(CVI_U32 cur_iso_val, CVI_S32 blk_size_w_bits, CVI_S32 reg_rgbmap_input_sel,
	CVI_S32 blk_size_h_bits, CVI_S32 digital_gain, CVI_S32 wb_r_gain, CVI_S32 wb_b_gain,
	CVI_FLOAT cali_noise_model[16][4][2], CVI_S32 noise_level_low,
	CVI_S32 noise_level_high, CVI_S32 color_id, CVI_S32 *slope,
	CVI_S32 *luma_th, CVI_S32 *low_offset, CVI_S32 *high_offset);
static CVI_S32 get_manr_flick_profile_reg(int flick_slp, int *slope, int *luma_th, int *low_offset, int *high_offset);

CVI_S32 isp_algo_tnr_main(struct tnr_param_in *tnr_param_in, struct tnr_param_out *tnr_param_out)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	for (CVI_U32 i = 0 ; i < 3 ; i++) {
		CVI_U16 in0 = tnr_param_in->L2mIn0[i];
		CVI_U16 in1 = tnr_param_in->L2mIn0[i+1];
		CVI_U8 out0 = tnr_param_in->L2mOut0[i];
		CVI_U8 out1 = tnr_param_in->L2mOut0[i+1];

		tnr_param_out->L2mSlope0[i] = get_lut_slp(in0, in1, out0, out1, MANR_LUMA_LUT_SLP_F_BITS);
	}

	#if 0
	ISP_CTX_S *pstIspCtx = NULL;

	ISP_GET_CTX(ViPipe, pstIspCtx);

	if (tnr_attr->DeflickerMode == 0 || pstIspCtx->wdrLinearMode) {
		CVI_S32 L2mIn0 = runtime->tnr_luma_motion_attr.L2mIn1[0];
		CVI_S32 L2mIn1 = runtime->tnr_luma_motion_attr.L2mIn1[1];
		CVI_S32 L2mIn2 = runtime->tnr_luma_motion_attr.L2mIn1[2];
		CVI_S32 L2mIn3 = runtime->tnr_luma_motion_attr.L2mIn1[3];
		CVI_S32 L2mOut0 = runtime->tnr_luma_motion_attr.L2mOut1[0];
		CVI_S32 L2mOut1 = runtime->tnr_luma_motion_attr.L2mOut1[1];
		CVI_S32 L2mOut2 = runtime->tnr_luma_motion_attr.L2mOut1[2];
		CVI_S32 L2mOut3 = runtime->tnr_luma_motion_attr.L2mOut1[3];

		se_l2m_for_motion_stitching(algoResult->au32ExpRatio[0],
			&L2mIn0, &L2mIn1, &L2mIn2, &L2mIn3,
			&L2mOut0, &L2mOut1, &L2mOut2, &L2mOut3,
			algoResult->WDRCombineLongThr, algoResult->WDRCombineShortThr,
			algoResult->WDRCombineMinWeight, algoResult->WDRCombineMaxWeight, tnr_attr->DeflickerMode,
			tnr_attr->DeflickerToleranceLevel);

		runtime->tnr_luma_motion_attr.L2mIn1[0] = L2mIn0;
		runtime->tnr_luma_motion_attr.L2mIn1[1] = L2mIn1;
		runtime->tnr_luma_motion_attr.L2mIn1[2] = L2mIn2;
		runtime->tnr_luma_motion_attr.L2mIn1[3] = L2mIn3;
		runtime->tnr_luma_motion_attr.L2mOut1[0] = L2mOut0;
		runtime->tnr_luma_motion_attr.L2mOut1[1] = L2mOut1;
		runtime->tnr_luma_motion_attr.L2mOut1[2] = L2mOut2;
		runtime->tnr_luma_motion_attr.L2mOut1[3] = L2mOut3;
	}
	#endif

	for (CVI_U32 i = 0 ; i < 3 ; i++) {
		CVI_U16 in0 = tnr_param_in->L2mIn1[i];
		CVI_U16 in1 = tnr_param_in->L2mIn1[i+1];
		CVI_U8 out0 = tnr_param_in->L2mOut1[i];
		CVI_U8 out1 = tnr_param_in->L2mOut1[i+1];

		tnr_param_out->L2mSlope1[i] = get_lut_slp(in0, in1, out0, out1, MANR_LUMA_LUT_SLP_F_BITS);
	}

	for (CVI_U32 i = 0 ; i < 3 ; i++) {
		CVI_U8 in0 = tnr_param_in->PrvMotion0[i];
		CVI_U8 in1 = tnr_param_in->PrvMotion0[i+1];
		CVI_U8 out0 = tnr_param_in->PrtctWgt0[i];
		CVI_U8 out1 = tnr_param_in->PrtctWgt0[i+1];

		tnr_param_out->PrtctSlope[i] = get_lut_slp(in0, in1, out0, out1, MANR_IIR_LUT_SLP_F_BITS);
	}

	for (CVI_U32 i = 0 ; i < 3 ; i++) {
		CVI_U8 in0 = tnr_param_in->LowMtPrtInY[i];
		CVI_U8 in1 = tnr_param_in->LowMtPrtInY[i+1];
		CVI_U8 out0 = tnr_param_in->LowMtPrtOutY[i];
		CVI_U8 out1 = tnr_param_in->LowMtPrtOutY[i+1];

		tnr_param_out->LowMtPrtSlopeY[i] = get_lut_slp(in0, in1, out0, out1, MANR_ALPHA_LUT_SLP_FBIT);
	}

	for (CVI_U32 i = 0 ; i < 3 ; i++) {
		CVI_U8 in0 = tnr_param_in->LowMtPrtInU[i];
		CVI_U8 in1 = tnr_param_in->LowMtPrtInU[i+1];
		CVI_U8 out0 = tnr_param_in->LowMtPrtOutU[i];
		CVI_U8 out1 = tnr_param_in->LowMtPrtOutU[i+1];

		tnr_param_out->LowMtPrtSlopeU[i] = get_lut_slp(in0, in1, out0, out1, MANR_ALPHA_LUT_SLP_FBIT);
	}

	for (CVI_U32 i = 0 ; i < 3 ; i++) {
		CVI_U8 in0 = tnr_param_in->LowMtPrtInV[i];
		CVI_U8 in1 = tnr_param_in->LowMtPrtInV[i+1];
		CVI_U8 out0 = tnr_param_in->LowMtPrtOutV[i];
		CVI_U8 out1 = tnr_param_in->LowMtPrtOutV[i+1];

		tnr_param_out->LowMtPrtSlopeV[i] = get_lut_slp(in0, in1, out0, out1, MANR_ALPHA_LUT_SLP_FBIT);
	}

	for (CVI_U32 i = 0 ; i < 3 ; i++) {
		CVI_U8 in0 = tnr_param_in->LowMtPrtAdvIn[i];
		CVI_U8 in1 = tnr_param_in->LowMtPrtAdvIn[i+1];
		CVI_U8 out0 = tnr_param_in->LowMtPrtAdvOut[i];
		CVI_U8 out1 = tnr_param_in->LowMtPrtAdvOut[i+1];

		tnr_param_out->LowMtPrtAdvSlope[i] = get_lut_slp(in0, in1, out0, out1, MANR_ALPHA_LUT_SLP_FBIT);
	}

	for (CVI_U32 i = 0 ; i < 3 ; i++) {
		CVI_U8 in0 = tnr_param_in->LowMtPrtAdvDebugIn[i];
		CVI_U8 in1 = tnr_param_in->LowMtPrtAdvDebugIn[i+1];
		CVI_U8 out0 = tnr_param_in->LowMtPrtAdvDebugOut[i];
		CVI_U8 out1 = tnr_param_in->LowMtPrtAdvDebugOut[i+1];

		tnr_param_out->LowMtPrtAdvDebugSlope[i] = get_lut_slp(in0, in1, out0, out1, MANR_ALPHA_LUT_SLP_FBIT);
	}

	for (CVI_U32 i = 0 ; i < 3 ; i++) {
		CVI_U8 in0 = tnr_param_in->AdaptNrLumaStrIn[i];
		CVI_U8 in1 = tnr_param_in->AdaptNrLumaStrIn[i+1];
		CVI_U8 out0 = tnr_param_in->AdaptNrLumaStrOut[i];
		CVI_U8 out1 = tnr_param_in->AdaptNrLumaStrOut[i+1];

		tnr_param_out->AdaptNrLumaSlope[i] = get_lut_slp(in0, in1, out0, out1, MANR_ALPHA_LUT_SLP_FBIT);
	}

	for (CVI_U32 i = 0 ; i < 3 ; i++) {
		CVI_U8 in0 = tnr_param_in->AdaptNrChromaStrIn[i];
		CVI_U8 in1 = tnr_param_in->AdaptNrChromaStrIn[i+1];
		CVI_U8 out0 = tnr_param_in->AdaptNrChromaStrOut[i];
		CVI_U8 out1 = tnr_param_in->AdaptNrChromaStrOut[i+1];

		tnr_param_out->AdaptNrChromaSlope[i] = get_lut_slp(in0, in1, out0, out1, MANR_ALPHA_LUT_SLP_FBIT);
	}

	for (CVI_U32 path = 0 ; path < TNR_PATH_NUM ; path++) {
		for (CVI_U32 chan = 0 ; chan < TNR_CHAN_NUM ; chan++) {
			get_manr_noise_profile_reg(tnr_param_in->iso, tnr_param_in->rgbmap_w_bit,
				tnr_param_in->gbmap_input_sel, tnr_param_in->rgbmap_h_bit,
				tnr_param_in->ispdgain, tnr_param_in->wb_rgain, tnr_param_in->wb_bgain,
				tnr_param_in->np.CalibrationCoef,
				tnr_param_in->noiseLowLv[path][chan], tnr_param_in->noiseHiLv[path][chan], chan,
				&(tnr_param_out->slope[path][chan]), &(tnr_param_out->luma[path][chan]),
				&(tnr_param_out->low[path][chan]), &(tnr_param_out->high[path][chan]));
		}
	}

	for (CVI_U32 path = 0 ; path < TNR_PATH_NUM ; path++) {
		for (CVI_U32 chan = 0 ; chan < TNR_CHAN_NUM ; chan++) {
			get_manr_flick_profile_reg(tnr_param_in->BrightnessNoiseLevelLE[path],
				&(tnr_param_out->slope[path][chan]), &(tnr_param_out->luma[path][chan]),
				&(tnr_param_out->low[path][chan]), &(tnr_param_out->high[path][chan]));
		}
	}

	// TODO@CV181X sw function
	tnr_param_out->SharpenCompGain = 256;
	for (CVI_U32 i = 0 ; i < 17 ; i++)
		tnr_param_out->LumaCompGain[i] = 512;

	tnr_param_out->TnrLscCenterX = 960;
	tnr_param_out->TnrLscCenterY = 540;
	tnr_param_out->TnrLscNorm = 30;
	tnr_param_out->TnrLscDYGain = 128;
	for (CVI_U32 i = 0 ; i < 17 ; i++)
		tnr_param_out->TnrLscCompGain[i] = 512;

	tnr_param_out->MtLscCenterX = 960;
	tnr_param_out->MtLscCenterY = 540;
	tnr_param_out->MtLscNorm = 30;
	tnr_param_out->MtLscDYGain = 128;
	for (CVI_U32 i = 0 ; i < 17 ; i++)
		tnr_param_out->MtLscCompGain[i] = 512;

	return ret;
}

CVI_S32 isp_algo_tnr_init(void)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	return ret;
}

CVI_S32 isp_algo_tnr_uninit(void)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	return ret;
}

// -----------------------------------------------------------------------------
static CVI_FLOAT isp_algo_tnr_get_threshold(struct tnr_motion_block_size_threshold *ptStruct)
{
	CVI_U32 u32BlockSize, u32BlockSizeBR, u32BlockSizeG;
	CVI_FLOAT fThChB, fThChG, fThChR, fThChY;
	CVI_FLOAT fLengthBR, fLengthG;

	u32BlockSize = ptStruct->u32BlockDim * ptStruct->u32BlockDim;
	u32BlockSizeBR = u32BlockSize / 4;
	u32BlockSizeG = u32BlockSize / 2;
	fLengthBR = (CVI_FLOAT)sqrt(u32BlockSizeBR);
	fLengthG = (CVI_FLOAT)sqrt(u32BlockSizeG);

	ISP_ALGO_LOG_DEBUG("Block : %3u x %3u, Size : %5u, B/R : %5u (%8.5f), G : %5u (%8.5f)\n",
		ptStruct->u32BlockDim, ptStruct->u32BlockDim, u32BlockSize,
		u32BlockSizeBR, fLengthBR, u32BlockSizeG, fLengthG);

	fThChB = ptStruct->fNPChB * (CVI_FLOAT)ptStruct->u32Tolerance / fLengthBR;
	fThChG = ptStruct->fNPChG * (CVI_FLOAT)ptStruct->u32Tolerance / fLengthG;
	fThChR = ptStruct->fNPChR * (CVI_FLOAT)ptStruct->u32Tolerance / fLengthBR;
	fThChY = TNR_MOTION_BLOCK_SIZE_RGB2Y(fThChB, fThChG, fThChR);

	return fThChY;
}

const CVI_U8 gau8BlockValue[] = {3, 4, 5};
const CVI_U32 gu32BlockValueTblSize = ARRAY_SIZE(gau8BlockValue);

// -----------------------------------------------------------------------------
CVI_S32 isp_algo_tnr_generateblocksize(
	struct tnr_motion_block_size_in *ptIn, struct tnr_motion_block_size_out *ptOut)
{
	struct tnr_motion_block_size_threshold stThreshold;
	CVI_FLOAT fTh_Y;
	CVI_U32 u32RecommendBlockIndex;

	stThreshold.fNPChB = ptIn->fSlopeB * (CVI_FLOAT)ptIn->u32RefLuma + ptIn->fInterceptB;
	stThreshold.fNPChG = ptIn->fSlopeG * (CVI_FLOAT)ptIn->u32RefLuma + ptIn->fInterceptG;
	stThreshold.fNPChR = ptIn->fSlopeR * (CVI_FLOAT)ptIn->u32RefLuma + ptIn->fInterceptR;
	stThreshold.u32Tolerance = ptIn->u32Tolerance;

	u32RecommendBlockIndex = gu32BlockValueTblSize - 1;
	for (CVI_U32 u32BlockValueIdx = 0; u32BlockValueIdx < gu32BlockValueTblSize; ++u32BlockValueIdx) {
		CVI_U32 u32BlockTmpLength = (CVI_U32)pow(2, gau8BlockValue[u32BlockValueIdx]);

		stThreshold.u32BlockDim = u32BlockTmpLength;

		fTh_Y = isp_algo_tnr_get_threshold(&stThreshold);
		ISP_ALGO_LOG_DEBUG("BlockValueIdx : %1u, Y threshold : %8.5f\n", u32BlockValueIdx, fTh_Y);

		if (fTh_Y < ptIn->fMotionTh) {
			u32RecommendBlockIndex = u32BlockValueIdx;
			break;
		}
	}

	ptOut->u8BlockValue = gau8BlockValue[u32RecommendBlockIndex];

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
static int get_manr_flick_profile_reg(int flick_slp, int *slope, int *luma_th, int *low_offset, int *high_offset)
{
	int rtn = 0;
	int new_luma_th, new_high_offset, new_slope;

	if (*slope < flick_slp) {
		if ((*luma_th * flick_slp) > *low_offset << MANR_NS_SLP_F_BITS) {
			new_luma_th = (*low_offset << MANR_NS_SLP_F_BITS) / flick_slp;
			new_slope = flick_slp;
			*luma_th = new_luma_th;
			*slope = new_slope;
		} else {
#if 0
			new_luma_th = ((*low_offset << MANR_NS_SLP_F_BITS) - *luma_th * *slope) / (flick_slp - *slope);
			new_low_offset = (new_luma_th * flick_slp) >> MANR_NS_SLP_F_BITS;
			new_slope = flick_slp;
			*luma_th = new_luma_th;
			*low_offset = (new_luma_th * flick_slp) >> MANR_NS_SLP_F_BITS;
			*slope = new_slope;
#endif
			new_high_offset = 4095 * flick_slp >> MANR_NS_SLP_F_BITS;
			new_slope = ((new_high_offset - *low_offset) * (1 << MANR_NS_SLP_F_BITS)
				+ ((4095 - *luma_th) >> 1)) / (4095 - *luma_th);
			new_slope = MIN(new_slope, MANR_NS_SLP_MAX);
			*slope = new_slope;
		}
	}

	UNUSED(*high_offset);

	return rtn;
}

static CVI_S32 get_manr_noise_profile_reg(CVI_U32 cur_iso_val, CVI_S32 blk_size_w_bits, CVI_S32 reg_rgbmap_input_sel,
				   CVI_S32 blk_size_h_bits, CVI_S32 digital_gain, CVI_S32 wb_r_gain, CVI_S32 wb_b_gain,
				   CVI_FLOAT cali_noise_model[16][4][2], CVI_S32 noise_level_low,
				   CVI_S32 noise_level_high, CVI_S32 color_id, CVI_S32 *slope,
				   CVI_S32 *luma_th, CVI_S32 *low_offset, CVI_S32 *high_offset)
{
	CVI_S32 rtn = 0;

	CVI_FLOAT param_slp, param_off;
	CVI_FLOAT ns_decay_ratio_rb = pow(sqrt(2), (blk_size_w_bits + blk_size_h_bits - 2));
	CVI_FLOAT ns_decay_ratio_g = ns_decay_ratio_rb * sqrt(2);
	CVI_FLOAT noise_model[16 * 4 * 2];
	CVI_U32 *ISO_TABLE;

	get_iso_tbl(&ISO_TABLE);


	for (CVI_S32 k = 0; k < 16; k++) {
		for (CVI_S32 j = 0; j < 4; j++) {
			for (CVI_S32 i = 0; i < 2; i++) {
				noise_model[k * 8 + j * 2 + i] = cali_noise_model[k][j][i];
			}
		}
	}
	//	for (CVI_S32 i = 0; i < 16 * 4 * 2; i++)
	//		noise_model[i] = cali_noise_model[i] / (CVI_FLOAT)(1 << CALI_DATA_F_BITS);

	// 1. get noise parameters by ISO
	CVI_S32 idx_low_0, idx_low_1, idx_high_0, idx_high_1;
	CVI_S32 low_iso, high_iso;
	CVI_S32 low_iso_val, high_iso_val;

	low_iso = ISP_AUTO_ISO_STRENGTH_NUM - 1;
	high_iso = ISP_AUTO_ISO_STRENGTH_NUM - 1;
	for (CVI_S32 i = 0; i < 15; i++) {
		if (ISO_TABLE[i] == cur_iso_val) {
			low_iso = i;
			high_iso = i;
			break;
		} else if (ISO_TABLE[i] < cur_iso_val && cur_iso_val < ISO_TABLE[i + 1]) {
			low_iso = i;
			high_iso = i + 1;
			break;
		} else if (cur_iso_val == ISO_TABLE[i + 1]) {
			low_iso = i + 1;
			high_iso = i + 1;
			break;
		}
	}
	low_iso_val = ISO_TABLE[low_iso];
	high_iso_val = ISO_TABLE[high_iso];

	CVI_S32 high_weight = cur_iso_val - low_iso_val;
	CVI_S32 low_weight = high_iso_val - cur_iso_val;
	CVI_S32 total_weight = high_iso_val - low_iso_val;

	if (total_weight == 0) { // take low iso data
		if (color_id == COLOR_R) {
			idx_low_0 = low_iso * 4 * 2 + NS_MODEL_R * 2;
			param_slp = noise_model[idx_low_0 + SLP_IDX];
			param_off = noise_model[idx_low_0 + OFF_IDX];
		} else if (color_id == COLOR_G) {
			idx_low_0 = low_iso * 4 * 2 + NS_MODEL_GR * 2;
			idx_low_1 = low_iso * 4 * 2 + NS_MODEL_GB * 2;
			param_slp = (noise_model[idx_low_0 + SLP_IDX] + noise_model[idx_low_1 + SLP_IDX]) / 2;
			param_off = (noise_model[idx_low_0 + OFF_IDX] + noise_model[idx_low_1 + OFF_IDX]) / 2;
		} else {
			idx_low_0 = low_iso * 4 * 2 + NS_MODEL_B * 2;
			param_slp = noise_model[idx_low_0 + SLP_IDX];
			param_off = noise_model[idx_low_0 + OFF_IDX];
		}
	} else { //  take iso alpha blending data
		if (color_id == COLOR_R) {
			idx_low_0 = low_iso * 4 * 2 + NS_MODEL_R * 2;
			idx_high_0 = high_iso * 4 * 2 + NS_MODEL_R * 2;
			param_slp = (low_weight * noise_model[idx_low_0 + SLP_IDX] +
				     high_weight * noise_model[idx_high_0 + SLP_IDX]) /
				    total_weight;
			param_off = (low_weight * noise_model[idx_low_0 + OFF_IDX] +
				     high_weight * noise_model[idx_high_0 + OFF_IDX]) /
				    total_weight;
		} else if (color_id == COLOR_G) {
			idx_low_0 = low_iso * 4 * 2 + NS_MODEL_GR * 2;
			idx_high_0 = high_iso * 4 * 2 + NS_MODEL_GR * 2;
			idx_low_1 = low_iso * 4 * 2 + NS_MODEL_GB * 2;
			idx_high_1 = high_iso * 4 * 2 + NS_MODEL_GB * 2;
			param_slp =
				(low_weight * (noise_model[idx_low_0 + SLP_IDX] + noise_model[idx_low_1 + SLP_IDX]) +
				 high_weight *
					 (noise_model[idx_high_0 + SLP_IDX] + noise_model[idx_high_1 + SLP_IDX])) /
				(2 * total_weight);
			param_off =
				(low_weight * (noise_model[idx_low_0 + OFF_IDX] + noise_model[idx_low_1 + OFF_IDX]) +
				 high_weight *
					 (noise_model[idx_high_0 + OFF_IDX] + noise_model[idx_high_1 + OFF_IDX])) /
				(2 * total_weight);
		} else {
			idx_low_0 = low_iso * 4 * 2 + NS_MODEL_B * 2;
			idx_high_0 = high_iso * 4 * 2 + NS_MODEL_B * 2;
			param_slp = (low_weight * noise_model[idx_low_0 + SLP_IDX] +
				     high_weight * noise_model[idx_high_0 + SLP_IDX]) /
				    total_weight;
			param_off = (low_weight * noise_model[idx_low_0 + OFF_IDX] +
				     high_weight * noise_model[idx_high_0 + OFF_IDX]) /
				    total_weight;
		}
	}

	// 2. compensate noise parameters by digital gain
	CVI_S32 luma_low_bound = 16;

	param_off = ((luma_low_bound * param_slp + param_off) * digital_gain) / (1 << DIGITAL_GAIN_F_BITS);
	param_slp = ((1 << MANR_NS_SLP_F_BITS) * param_slp * digital_gain) / (1 << DIGITAL_GAIN_F_BITS);

	// 3. compensate noise parameters by wb gain
	if (!reg_rgbmap_input_sel) {
		if (color_id == COLOR_R) {
			param_off = (param_off * wb_r_gain) / (1 << WB_GAIN_F_BITS);
			param_slp = (param_slp * wb_r_gain) / (1 << WB_GAIN_F_BITS);
		} else if (color_id == COLOR_B) {
			param_off = (param_off * wb_b_gain) / (1 << WB_GAIN_F_BITS);
			param_slp = (param_slp * wb_b_gain) / (1 << WB_GAIN_F_BITS);
		} else {
			param_off = param_off;
			param_slp = param_slp;
		}
	}

	// 4. compensate noise parameters by block size
	param_off = (color_id == COLOR_R || color_id == COLOR_B) ? param_off / ns_decay_ratio_rb :
								   param_off / ns_decay_ratio_g;
	param_slp = (color_id == COLOR_R || color_id == COLOR_B) ? param_slp / ns_decay_ratio_rb :
								   param_slp / ns_decay_ratio_g;

	// 5. adjust noise parmeters by noise level
	param_off = param_off * noise_level_low / (CVI_FLOAT)64;
	param_slp = param_slp * noise_level_high / (CVI_FLOAT)64;

	// 6. assign to reg
	*luma_th = luma_low_bound;
	*slope = (CVI_S32)MIN((param_slp + 0.5), MANR_NS_SLP_MAX);
	*low_offset = (CVI_S32)MIN((param_off + 0.5), MANR_NS_OFF_MAX);
	*high_offset = MANR_NS_OFF_MAX;

	return rtn;
}
