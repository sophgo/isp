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

#include "isp_algo_dci.h"
#include "isp_algo_debug.h"
#include "isp_algo_utility.h"

#define DCI_AC_MIN 8
#define DCI_AC_MAX 235
#define DCI_BLENDING_SHIFT_ROUND_OFFSET	(512)
#define DCI_BLENDING_SHIFT_BITS			(10)
#define DCI_BLENDING_ORIG_BIN_MAX		(1023)
#define DCI_BLENDING_SHIFT_BIN_MAX		(DCI_BLENDING_ORIG_BIN_MAX << DCI_BLENDING_SHIFT_BITS)

static void DCI_API(CVI_U32 *phist, CVI_U16 strength, CVI_U8 method, CVI_U16 *lut);
static void DCI_Blending_API(CVI_U16 *cur_lut, CVI_U32 *pre_lut, CVI_U16 *out_lut, CVI_U16 weight_prev);
static void dci_clipHist(CVI_U32 *phist, CVI_U32 clipLmit);


CVI_S32 isp_algo_dci_init(void)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	return ret;
}

CVI_S32 isp_algo_dci_uninit(void)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	return ret;
}

CVI_S32 isp_algo_dci_main(struct dci_param_in *dci_param_in, struct dci_param_out *dci_param_out)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	if (dci_param_in->mode) {
		return ret;
	}

	if (dci_param_in->bUpdateCurve) {
		CVI_U32 pHist[DCI_BINS_NUM] = { 0 };

		for (CVI_U32 i = 0; i < DCI_BINS_NUM; i++) {
			pHist[i] = ISP_PTR_CAST_U32(dci_param_in->pHist)[i];
		}

		DCI_API(pHist, dci_param_in->strength, dci_param_in->method, ISP_PTR_CAST_U16(dci_param_in->curLut));
	}

	DCI_Blending_API(ISP_PTR_CAST_U16(dci_param_in->curLut), ISP_PTR_CAST_U32(dci_param_in->preLut),
		ISP_PTR_CAST_U16(dci_param_out->outLut), dci_param_in->speed);

	return ret;
}

static void DCI_API(CVI_U32 *phist, CVI_U16 strength, CVI_U8 method, CVI_U16 *lut)
{
	CVI_U64 totalCnt = 0;
	CVI_U16 acdelta = DCI_AC_MAX - DCI_AC_MIN + 1;

	for (CVI_U16 i = 0; i < DCI_BINS_NUM; i++) {
		totalCnt += phist[i];
	}

	CVI_U32 clipLmitCnt = totalCnt * strength / 10000;

	dci_clipHist(phist, clipLmitCnt);

	totalCnt = 0;
	for (CVI_U16 i = DCI_AC_MIN; i < DCI_AC_MAX; i++) {
		totalCnt += phist[i];
	}

	CVI_U64 sum = 0;
	CVI_U32 mapVal;

	for (CVI_U16 i = 0; i < DCI_BINS_NUM; i++) {
		if ((i < DCI_AC_MIN) || (i > DCI_AC_MAX)) {
			lut[i] = i;
		} else {
			sum += phist[i];
			mapVal = DCI_AC_MIN + (sum * acdelta / totalCnt);
			lut[i] = LIMIT_RANGE(DCI_AC_MIN, mapVal, DCI_AC_MAX);
		}
	}

	if (!method) {
		for (CVI_U16 i = 0; i < DCI_BINS_NUM; i++) {
			lut[i] *= 4;
			lut[i] = LIMIT_RANGE(0, lut[i], 1023);
		}
	}
}

static void DCI_Blending_API(CVI_U16 *cur_lut, CVI_U32 *pre_lut, CVI_U16 *out_lut, CVI_U16 weight_prev)
{
	const CVI_U32 wt_cur = 1;
	CVI_U32 pre_weight, weight_base;

	pre_weight = (weight_prev < wt_cur) ? 0 : weight_prev;
	weight_base = pre_weight + wt_cur;
	if (pre_weight == 0) {
		for (CVI_U32 i = 0; i < DCI_BINS_NUM ; i++) {
			out_lut[i] = cur_lut[i];
			pre_lut[i] = (CVI_U32)out_lut[i] << DCI_BLENDING_SHIFT_BITS;
		}
	} else {
		for (CVI_U32 i = 0; i < DCI_BINS_NUM; i++) {
			CVI_U32 val1, val2, val;

			val1 = ((CVI_U32)cur_lut[i] << DCI_BLENDING_SHIFT_BITS) * wt_cur;
			val2 = pre_lut[i] * pre_weight;
			val = (val1 + val2) / weight_base;
			pre_lut[i] = MIN((CVI_U32)val, DCI_BLENDING_SHIFT_BIN_MAX);
			out_lut[i] = (pre_lut[i] + DCI_BLENDING_SHIFT_ROUND_OFFSET)
				>> DCI_BLENDING_SHIFT_BITS;
			out_lut[i] = MIN(out_lut[i], DCI_BLENDING_ORIG_BIN_MAX);
		}
	}
}

static void dci_clipHist(CVI_U32 *hist, CVI_U32 clipLmit)
{
	CVI_U32 nExcessPerbin = 0, nExcess = 0;
	CVI_S32 ExcessBin = 0;

	for (CVI_U16 i = 0; i < DCI_BINS_NUM; i++) {
		ExcessBin = hist[i] - clipLmit;
		if (ExcessBin > 0) {
			nExcess += ExcessBin;
		}
	}
	nExcessPerbin = nExcess / DCI_BINS_NUM;
	for (CVI_U16 i = 0; i < DCI_BINS_NUM; i++) {
		if (hist[i] >= clipLmit) {
			hist[i] = clipLmit;
		} else {
			CVI_U32 delta = clipLmit - hist[i];

			if (delta > nExcessPerbin) {
				delta = nExcessPerbin;
			}
			hist[i] += delta;
			nExcess -= delta;
		}
	}

	CVI_U32 endPtr, startPtr, binPtr;
	CVI_U32 oldExcess, stepSize;

	do {
		endPtr = DCI_BINS_NUM;
		startPtr = 0;
		oldExcess = nExcess;
		while (nExcess && (startPtr < endPtr)) {
			stepSize = DCI_BINS_NUM / nExcess;
			stepSize = stepSize < 1 ? 1 : stepSize;
			for (binPtr = startPtr; (binPtr < endPtr) && nExcess; binPtr += stepSize) {
				if (hist[binPtr] < clipLmit) {
					hist[binPtr]++;
					nExcess--;
				}
			}
			startPtr++;
		}
	} while ((nExcess) && (nExcess < oldExcess));

	if (nExcess) {
		nExcessPerbin = nExcess / DCI_BINS_NUM;
		for (CVI_U16 i = 0; i < DCI_BINS_NUM; i++) {
			hist[i] += nExcessPerbin;
			nExcess -= nExcessPerbin;
		}
		hist[0] += nExcess;
	}
}
