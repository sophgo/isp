/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_fswdr.c
 * Description:
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "limits.h"

#include "isp_algo_fswdr.h"
#include "isp_algo_debug.h"
#include "isp_algo_utility.h"

#define WDR_EXPOSURE_RATIO_F_BITS 6
#define WDR_EXPOSURE_RATIO_I_BITS 8
#define WDR_EXPOSURE_RATIO_MAX ((1 << (WDR_EXPOSURE_RATIO_I_BITS + WDR_EXPOSURE_RATIO_F_BITS)) - 1)
#define WDR_FUSION_LUT_SLP_F_BITS 10
#define WDR_ALPHA_LUT_SLP_FBIT 4

static void hdr_shortMax_get(int ExposureRatio, int *shortMaxVal);
static void hdr_update_combine_min_weight(float exposure_ratio,
					struct fswdr_param_in *wdr_in_param,
					CVI_U32 *outWDRCombineMinWeight, CVI_FLOAT *outWDRMinWeightSNRIIR);

CVI_S32 isp_algo_fswdr_main(struct fswdr_param_in *fswdr_param_in, struct fswdr_param_out *fswdr_param_out)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	int ExposureRatio;

	ExposureRatio = (int)(fswdr_param_in->exposure_ratio * (1 << WDR_EXPOSURE_RATIO_F_BITS) + 0.5);
	ExposureRatio = (ExposureRatio > WDR_EXPOSURE_RATIO_MAX) ? WDR_EXPOSURE_RATIO_MAX : ExposureRatio;

	hdr_shortMax_get(ExposureRatio, &fswdr_param_out->ShortMaxVal);

	// Update CombineMinWeight
	hdr_update_combine_min_weight(fswdr_param_in->exposure_ratio, fswdr_param_in,
							&fswdr_param_out->WDRCombineMinWeight,
							&fswdr_param_out->WDRMinWeightSNRIIR);
	{
		CVI_U16 in0 = fswdr_param_in->WDRCombineLongThr;
		CVI_U16 in1 = fswdr_param_in->WDRCombineShortThr;
		CVI_U16 out0 = fswdr_param_in->WDRCombineMaxWeight;
		CVI_U16 out1 = fswdr_param_out->WDRCombineMinWeight;
		fswdr_param_out->WDRCombineSlope = get_lut_slp(in0, in1, out0, out1, WDR_FUSION_LUT_SLP_F_BITS);
	}

	if (fswdr_param_in->WDRFsCalPxlNum > 0) {
		fswdr_param_out->WDRSeFixOffsetR = fswdr_param_in->WDRPxlDiffSumR / fswdr_param_in->WDRFsCalPxlNum;
		fswdr_param_out->WDRSeFixOffsetG = fswdr_param_in->WDRPxlDiffSumG / fswdr_param_in->WDRFsCalPxlNum;
		fswdr_param_out->WDRSeFixOffsetB = fswdr_param_in->WDRPxlDiffSumB / fswdr_param_in->WDRFsCalPxlNum;
	} else {
		fswdr_param_out->WDRSeFixOffsetR = 0;
		fswdr_param_out->WDRSeFixOffsetG = 0;
		fswdr_param_out->WDRSeFixOffsetB = 0;
	}

	for (CVI_U32 i = 0 ; i < 3 ; i++) {
		CVI_U8 in0 = fswdr_param_in->WDRMtIn[i];
		CVI_U8 in1 = fswdr_param_in->WDRMtIn[i+1];
		CVI_U16 out0 = fswdr_param_in->WDRMtOut[i];
		CVI_U16 out1 = fswdr_param_in->WDRMtOut[i+1];

		fswdr_param_out->WDRMtSlope[i] = get_lut_slp(in0, in1, out0, out1, WDR_ALPHA_LUT_SLP_FBIT);
	}

	{
		CVI_U16 in0 = fswdr_param_in->DarkToneRefinedThrL;
		CVI_U16 in1 = fswdr_param_in->DarkToneRefinedThrH;
		CVI_U16 out0 = fswdr_param_in->DarkToneRefinedMaxWeight;
		CVI_U16 out1 = fswdr_param_in->DarkToneRefinedMinWeight;

		fswdr_param_out->DarkToneRefinedSlope = get_lut_slp(in0, in1, out0, out1, WDR_FUSION_LUT_SLP_F_BITS);
	}

	{
		CVI_U16 in0 = fswdr_param_in->BrightToneRefinedThrL;
		CVI_U16 in1 = fswdr_param_in->BrightToneRefinedThrH;
		CVI_U16 out0 = fswdr_param_in->BrightToneRefinedMaxWeight;
		CVI_U16 out1 = fswdr_param_in->BrightToneRefinedMinWeight;

		fswdr_param_out->BrightToneRefinedSlope = get_lut_slp(in0, in1, out0, out1, WDR_FUSION_LUT_SLP_F_BITS);
	}

	switch (fswdr_param_in->WDRMotionFusionMode) {
	case 0:
		fswdr_param_out->MergeMtLsMode = 0;
		fswdr_param_out->MergeMode = 0;
		fswdr_param_out->MergeMtMaxMode = 0;
	break;
	case 1:
		fswdr_param_out->MergeMtLsMode = 0;
		fswdr_param_out->MergeMode = 0;
		fswdr_param_out->MergeMtMaxMode = 1;
	break;
	case 2:
		fswdr_param_out->MergeMtLsMode = 0;
		fswdr_param_out->MergeMode = 1;
		fswdr_param_out->MergeMtMaxMode = 1;
	break;
	case 3:
		fswdr_param_out->MergeMtLsMode = 1;
		fswdr_param_out->MergeMode = 0;
		fswdr_param_out->MergeMtMaxMode = 1;
	break;
	default:
		fswdr_param_out->MergeMtLsMode = 0;
		fswdr_param_out->MergeMode = 0;
		fswdr_param_out->MergeMtMaxMode = 0;
	break;
	}

	{
		CVI_U16 in0 = fswdr_param_in->WDRMotionCombineLongThr;
		CVI_U16 in1 = fswdr_param_in->WDRMotionCombineShortThr;
		CVI_U16 out0 = fswdr_param_in->WDRMotionCombineMaxWeight;
		CVI_U16 out1 = fswdr_param_in->WDRMotionCombineMinWeight;

		fswdr_param_out->WDRMotionCombineSlope = get_lut_slp(in0, in1, out0, out1, WDR_FUSION_LUT_SLP_F_BITS);
	}

	return ret;
}

CVI_S32 isp_algo_fswdr_init(void)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	return ret;
}

CVI_S32 isp_algo_fswdr_uninit(void)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	return ret;
}

static void hdr_shortMax_get(int ExposureRatio, int *shortMaxVal)
{
	if (shortMaxVal != NULL)
		*shortMaxVal = ((4096 * ExposureRatio) >> WDR_EXPOSURE_RATIO_F_BITS) - 1;
}

static const CVI_U32 ISO_TABLE[16] = {100, 200, 400, 800, 1600, 3200, 6400, 12800,
	25600, 51200, 102400, 204800, 409600, 819200, 1638400, 3276800};

static void hdr_update_combine_min_weight(float exposure_ratio,
					struct fswdr_param_in *wdr_in_param,
					CVI_U32 *outWDRCombineMinWeight, CVI_FLOAT *outWDRMinWeightSNRIIR)
{
#define SLP_IDX 0
#define OFF_IDX 1

#define NS_MODEL_R 3
#define NS_MODEL_GR 2
#define NS_MODEL_GB 1
#define NS_MODEL_B 0

#define REFERENCE_BRIGHTNESS_VALUE 56
#define WB_GAIN_F_BITS 10
#define DIGITAL_GAIN_F_BITS 10

#define SNR_MIN_WEIGHT_UPDATE_LOG_FREQ 66

	if (!wdr_in_param->WDRCombineSNRAwareEn) {
		*outWDRCombineMinWeight = wdr_in_param->WDRCombineMinWeight;
		*outWDRMinWeightSNRIIR = (CVI_FLOAT)wdr_in_param->WDRCombineMinWeight;
		return;
	}

	static CVI_U32 u32LogFrameCount;

	CVI_BOOL SNRAwareEn = wdr_in_param->WDRCombineSNRAwareEn;
	CVI_U16 SNRAwareSmoothLevel = wdr_in_param->WDRCombineSNRAwareSmoothLevel;
	CVI_FLOAT SNRAwareLowThr = wdr_in_param->WDRCombineSNRAwareLowThr;
	CVI_FLOAT SNRAwareHighThr = wdr_in_param->WDRCombineSNRAwareHighThr;
	CVI_U8 SNRAwareToleranceLevel = wdr_in_param->WDRCombineSNRAwareToleranceLevel;
	CVI_BOOL SNRAwareLog = CVI_FALSE;

	if (SNRAwareLog) {
		u32LogFrameCount++;
		if (u32LogFrameCount > SNR_MIN_WEIGHT_UPDATE_LOG_FREQ) {
			u32LogFrameCount = 0;
		} else {
			SNRAwareLog = CVI_FALSE;
		}
	}

	if (SNRAwareLog) {
		printf("SNRAware : en %d, sl %d, lthr %f, hthr %f, level %d\n",
			SNRAwareEn, SNRAwareSmoothLevel, SNRAwareLowThr, SNRAwareHighThr, SNRAwareToleranceLevel);
		printf("ISO: %d, DGain: %d, RGain: %d, BGain: %d, IIR: %f\n",
			wdr_in_param->iso, wdr_in_param->digital_gain, wdr_in_param->wb_r_gain, wdr_in_param->wb_b_gain,
			wdr_in_param->WDRMinWeightSNRIIR);
	}

	// 1. get current noise profile
	CVI_U32 low_iso = 0, high_iso = 0;
	CVI_U32 low_iso_val, high_iso_val;
	CVI_U32 cur_iso_val = wdr_in_param->iso;

	for (int i = 0; i < 15; i++) {
		if (ISO_TABLE[i] == cur_iso_val) {
			low_iso = i;
			high_iso = i;
		} else if ((ISO_TABLE[i] < cur_iso_val) && (cur_iso_val < ISO_TABLE[i + 1])) {
			low_iso = i;
			high_iso = i + 1;
		} else if (cur_iso_val == ISO_TABLE[i + 1]) {
			low_iso = i + 1;
			high_iso = i + 1;
		}
	}

	low_iso_val = ISO_TABLE[low_iso];
	high_iso_val = ISO_TABLE[high_iso];

	int high_weight = cur_iso_val - low_iso_val;
	int low_weight = high_iso_val - cur_iso_val;
	int total_weight = high_iso_val - low_iso_val;

	float slp_r, slp_g, slp_b;
	float off_r, off_g, off_b;

	ISP_CMOS_NOISE_CALIBRATION_S *np = CVI_NULL;

	np = (ISP_CMOS_NOISE_CALIBRATION_S *) ISP_PTR_CAST_VOID(wdr_in_param->pNoiseCalibration);

	CVI_FLOAT (*cali_noise_model)[NOISE_PROFILE_CHANNEL_NUM][NOISE_PROFILE_LEVEL_NUM] =
		np->CalibrationCoef;

	if (total_weight == 0) {
		// take low iso data
		slp_r = cali_noise_model[low_iso][NS_MODEL_R][SLP_IDX];
		off_r = cali_noise_model[low_iso][NS_MODEL_R][OFF_IDX];
		slp_g = (cali_noise_model[low_iso][NS_MODEL_GR][SLP_IDX] +
				cali_noise_model[low_iso][NS_MODEL_GB][SLP_IDX]) / 2;
		off_g = (cali_noise_model[low_iso][NS_MODEL_GR][OFF_IDX] +
				cali_noise_model[low_iso][NS_MODEL_GB][OFF_IDX]) / 2;
		slp_b = cali_noise_model[low_iso][NS_MODEL_B][SLP_IDX];
		off_b = cali_noise_model[low_iso][NS_MODEL_B][OFF_IDX];
	} else {
		// take iso alpha blending data
		slp_r = ((low_weight * cali_noise_model[low_iso][NS_MODEL_R][SLP_IDX]) +
				(high_weight * cali_noise_model[high_iso][NS_MODEL_R][SLP_IDX])) / total_weight;
		off_r = ((low_weight * cali_noise_model[low_iso][NS_MODEL_R][OFF_IDX])
				+ (high_weight * cali_noise_model[high_iso][NS_MODEL_R][OFF_IDX])) / total_weight;
		slp_g = ((low_weight * (cali_noise_model[low_iso][NS_MODEL_GR][SLP_IDX] +
								cali_noise_model[low_iso][NS_MODEL_GB][SLP_IDX])) +
				(high_weight * (cali_noise_model[high_iso][NS_MODEL_GR][SLP_IDX] +
								cali_noise_model[high_iso][NS_MODEL_GB][SLP_IDX]))) /
				(2 * total_weight);
		off_g = ((low_weight * (cali_noise_model[low_iso][NS_MODEL_GR][OFF_IDX] +
								cali_noise_model[low_iso][NS_MODEL_GB][OFF_IDX])) +
				(high_weight * (cali_noise_model[high_iso][NS_MODEL_GR][OFF_IDX] +
								cali_noise_model[high_iso][NS_MODEL_GB][OFF_IDX]))) /
				(2 * total_weight);
		slp_b = ((low_weight * cali_noise_model[low_iso][NS_MODEL_B][SLP_IDX]) +
				(high_weight * cali_noise_model[high_iso][NS_MODEL_B][SLP_IDX])) / total_weight;
		off_b = ((low_weight * cali_noise_model[low_iso][NS_MODEL_B][OFF_IDX]) +
				(high_weight * cali_noise_model[high_iso][NS_MODEL_B][OFF_IDX])) / total_weight;
	}

	if (SNRAwareLog) {
		printf("iso, wgt0, wgt1, slp_r/g/b, off_r/g/b : %d, %d, %d, %f, %f, %f, %f, %f, %f\n",
			cur_iso_val, low_weight, high_weight, slp_r, slp_g, slp_b, off_r, off_g, off_b);
	}

	// 2. get se exposure snr profile
	float Digital_Gain = (float)wdr_in_param->digital_gain;
	float WB_R_Gain = (float)wdr_in_param->wb_r_gain;
	float WB_B_Gain = (float)wdr_in_param->wb_b_gain;
	int avg_luma = REFERENCE_BRIGHTNESS_VALUE * 16;
	float ns_r = slp_r * avg_luma + off_r;
	float ns_g = slp_g * avg_luma + off_g;
	float ns_b = slp_b * avg_luma + off_b;
	float ns_val;
	float ns_v;

	ns_r = (ns_r * Digital_Gain) / (1 << DIGITAL_GAIN_F_BITS);
	ns_g = (ns_g * Digital_Gain) / (1 << DIGITAL_GAIN_F_BITS);
	ns_b = (ns_b * Digital_Gain) / (1 << DIGITAL_GAIN_F_BITS);

	ns_r = (ns_r * WB_R_Gain) / (1 << WB_GAIN_F_BITS);
	ns_b = (ns_b * WB_B_Gain) / (1 << WB_GAIN_F_BITS);

	ns_v = (ns_r > ns_g) ? ns_r : ns_g;
	ns_v = (ns_v > ns_b) ? ns_v : ns_b;
	ns_val = ns_v * exposure_ratio;

	if (SNRAwareLog) {
		printf("Rgain, Bgain, Dgain: %f, %f, %f\n",
			WB_R_Gain / (float)(1 << WB_GAIN_F_BITS),
			WB_B_Gain / (float)(1 << WB_GAIN_F_BITS),
			Digital_Gain / (float)(1 << DIGITAL_GAIN_F_BITS));
		printf("ns_r/g/b/v, exposureR, NoiseVal*exposureR: %f, %f, %f, %f, %f, %f\n",
			ns_r, ns_g, ns_b, ns_v, exposure_ratio, ns_val);
	}

	// 3. calc SnrCombineMaxWeight
	int fswdr_min_weight_snr;

	if (ns_val < SNRAwareLowThr)
		fswdr_min_weight_snr = wdr_in_param->WDRCombineMinWeight;
	else if (SNRAwareHighThr < ns_val)
		fswdr_min_weight_snr = SNRAwareToleranceLevel;
	else {
		float x_diff = ((SNRAwareHighThr - SNRAwareLowThr) == 0) ? 1 : SNRAwareHighThr - SNRAwareLowThr;
		float y_diff = (wdr_in_param->WDRCombineMinWeight > SNRAwareToleranceLevel) ?
			0 : (SNRAwareToleranceLevel - wdr_in_param->WDRCombineMinWeight);

		fswdr_min_weight_snr = wdr_in_param->WDRCombineMinWeight +
			(int)((ns_val - SNRAwareLowThr) * y_diff / x_diff + 0.5);
	}

	fswdr_min_weight_snr = (fswdr_min_weight_snr > 255) ? 255 : fswdr_min_weight_snr;

	if (SNRAwareLog) {
		printf("SNR LowThrd, HighThrd, ToleranceLevel, MinWeight, MinWeightSNR : %f, %f, %d, %d, %d\n",
			SNRAwareLowThr, SNRAwareHighThr, SNRAwareToleranceLevel,
			wdr_in_param->WDRCombineMinWeight, fswdr_min_weight_snr);
	}

	// 4. output to fswdr_min_weight
	CVI_U32 outputWDRCombineMinWeight;
	float fswdr_min_weight_snr_cur;

	if (SNRAwareSmoothLevel == 0)
		SNRAwareSmoothLevel = 1;

	float iir_alpha = 1.0 / (float)SNRAwareSmoothLevel;

	fswdr_min_weight_snr_cur = iir_alpha * fswdr_min_weight_snr +
								(1.0 - iir_alpha) * wdr_in_param->WDRMinWeightSNRIIR;

	fswdr_min_weight_snr = (int)(fswdr_min_weight_snr_cur + 0.5);
	outputWDRCombineMinWeight = fswdr_min_weight_snr;

	if (SNRAwareLog) {
		printf("SNREn, iir, PrvWeight, CurWeight: %d, %d, %f, %d\n\n\n",
			SNRAwareEn, SNRAwareSmoothLevel, wdr_in_param->WDRMinWeightSNRIIR, fswdr_min_weight_snr);
	}

	*outWDRCombineMinWeight = outputWDRCombineMinWeight;
	*outWDRMinWeightSNRIIR = fswdr_min_weight_snr_cur;

	UNUSED(off_b);
	UNUSED(off_g);
	UNUSED(off_r);
}

