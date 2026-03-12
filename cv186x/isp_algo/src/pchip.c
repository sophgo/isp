/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: pchip.c
 * Description:
 *
 */

#include <math.h>
#include "pchip.h"

#include "isp_algo_debug.h"

#ifndef MIN
#	define MIN(a, b)		((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#	define MAX(a, b)		((a) > (b) ? (a) : (b))
#endif
#ifndef LIMIT_IN
#	define LIMIT_IN(inpV, minV, maxV)		MIN(MAX((inpV), (minV)), (maxV))
#endif

#define IS_POSITIVE(v)		(((v) > 0) ? CVI_TRUE : CVI_FALSE)

// #define PCHIP_YIN_SHOULD_STRICTLY_INCREASING

// ----------------------------------------------------------------------------
static CVI_S32 PCHIP_CheckXY(CVI_S32 *ps32XIn, CVI_S32 *ps32YIn, CVI_U32 u32Size)
{
	// check the input length should be > 2
	if (u32Size <= 2) {
		ISP_ALGO_LOG_WARNING("[warn] the size of xin/yin should be > 2\n");
		return CVI_FAILURE;
	}

	// check x_in is strictly increasing
	for (CVI_U32 u32Idx = 0, u32IdxP1 = 1; u32Idx < u32Size - 1; ++u32Idx, ++u32IdxP1) {
		if (ps32XIn[u32Idx] >= ps32XIn[u32IdxP1]) {
			ISP_ALGO_LOG_WARNING("[warn] x_in should be strictly increasing x[%u]=%d, x[%u]=%d\n",
				u32Idx, ps32XIn[u32Idx], u32IdxP1, ps32XIn[u32IdxP1]);
			return CVI_FAILURE;
		}

#ifdef PCHIP_YIN_SHOULD_STRICTLY_INCREASING
		if (ps32YIn[u32Idx] >= ps32YIn[u32IdxP1]) {
			ISP_ALGO_LOG_WARNING("[warn] y_in should be strictly increasing y[%u]=%d, y[%u]=%d\n",
				u32Idx, ps32YIn[u32Idx], u32IdxP1, ps32YIn[u32IdxP1]);
			return CVI_FAILURE;
		}
#else
		if (ps32YIn[u32Idx] > ps32YIn[u32IdxP1]) {
			ISP_ALGO_LOG_WARNING("[warn] y_in should be increasing y[%u]=%d, y[%u]=%d\n",
				u32Idx, ps32YIn[u32Idx], u32IdxP1, ps32YIn[u32IdxP1]);
			return CVI_FAILURE;
		}
#endif // PCHIP_YIN_SHOULD_STRICTLY_INCREASING
	}

	return CVI_SUCCESS;
}

// ----------------------------------------------------------------------------
static CVI_S32 PCHIP_InterP1_PCHIP_Check_Param(TPCHIP_Info *ptInfo)
{
	if (PCHIP_CheckXY(ptInfo->ps32XIn, ptInfo->ps32YIn, ptInfo->u32SizeIn) != CVI_SUCCESS) {
		ISP_ALGO_LOG_WARNING("PCHIP_CheckXY check fail\n");
		return CVI_FAILURE;
	}

	if (ptInfo->u32SizeIn > PCHIP_MAX_INPUT_COUNT) {
		ISP_ALGO_LOG_WARNING("PCHIP_CheckXY input size over limitation\n");
		return CVI_FAILURE;
	}

	return CVI_SUCCESS;
}

// ----------------------------------------------------------------------------
CVI_S32 PCHIP_InterP1_PCHIP(TPCHIP_Info *ptInfo)
{
	CVI_S32 *ps32XIn = ptInfo->ps32XIn;
	CVI_S32 *ps32YIn = ptInfo->ps32YIn;
	CVI_S32 *ps32YOut = ptInfo->ps32YOut;
	CVI_U16 *pu16Temp = ptInfo->pu16Temp;
	CVI_U32 u32SizeIn = ptInfo->u32SizeIn;
	CVI_U32 u32SizeOut = ptInfo->u32SizeOut;
	const CVI_S32 s32MinYValue = ptInfo->s32MinYValue;
	const CVI_S32 s32MaxYValue = ptInfo->s32MaxYValue;

	if (PCHIP_InterP1_PCHIP_Check_Param(ptInfo) != CVI_SUCCESS) {
		return CVI_FAILURE;
	}

	float h[PCHIP_MAX_INPUT_COUNT];			// interval length h
	float h2[PCHIP_MAX_INPUT_COUNT];		// interval length h^2
	float delta[PCHIP_MAX_INPUT_COUNT];		// delta value
	float slopes[PCHIP_MAX_INPUT_COUNT];	// slopes

	CVI_U32 InIdx, OutIdx;

	// get interval length h
	// get delta value
	for (InIdx = 0; InIdx < u32SizeIn - 1; ++InIdx) {
		h[InIdx] = ps32XIn[InIdx + 1] - ps32XIn[InIdx];
		h2[InIdx] = h[InIdx] * h[InIdx];
		delta[InIdx] = (float)(ps32YIn[InIdx + 1] - ps32YIn[InIdx]) / h[InIdx];
	}
#if 0
	if (u32SizeIn == 2) {
		float tmp = 0;

		for (OutIdx = 0; OutIdx < u32SizeOut; ++OutIdx) {
//			float tmp = (float)OutIdx * delta[0];
			ps32YOut[OutIdx] = LIMIT_IN((CVI_S32)round(tmp), s32MinYValue, s32MaxYValue);
			tmp += delta[0];
		}

		return CVI_SUCCESS;
	}
#endif //
	// get slopes
	// Slopes at interior points.
	CVI_U32 InIdxM1;

	for (InIdx = 1, InIdxM1 = 0; InIdx < u32SizeIn - 1; ++InIdx, ++InIdxM1) {
		if (IS_POSITIVE(delta[InIdxM1]) && IS_POSITIVE(delta[InIdx])) {
			float w1 = h[InIdxM1] + h[InIdx] + h[InIdx];
			float w2 = h[InIdxM1] + h[InIdxM1] + h[InIdx];

			slopes[InIdx] = (w1 + w2) / ((w1 / delta[InIdxM1]) + (w2 / delta[InIdx]));
		}
	}

	// Slopes at start points.
	slopes[0] = ((h[0] + h[0] + h[1]) * delta[0] - (h[0] * delta[1])) / (h[0] + h[1]);
	if (IS_POSITIVE(slopes[0]) != IS_POSITIVE(delta[0])) {
		slopes[0] = 0.0f;
	} else if (
		(IS_POSITIVE(delta[0]) != IS_POSITIVE(delta[1]))
		&& (fabs(slopes[0]) > fabs(3.0f * delta[0]))
	) {
		slopes[0] = 3.0f * delta[0];
	}

	CVI_U32 u32SizeM1, u32SizeM2, u32SizeM3;

	u32SizeM1 = u32SizeIn - 1;
	u32SizeM2 = u32SizeIn - 2;
	u32SizeM3 = u32SizeIn - 3;

	// Slopes at end points.
	slopes[u32SizeM1] = (
			(h[u32SizeM2] + h[u32SizeM2] + h[u32SizeM3]) * delta[u32SizeM2]
			- (h[u32SizeM2] * delta[u32SizeM3])
			) / (h[u32SizeM2] + h[u32SizeM3]);
	if (IS_POSITIVE(slopes[u32SizeM1]) != IS_POSITIVE(delta[u32SizeM2])) {
		slopes[u32SizeM1] = 0.0f;
	} else if (
		(IS_POSITIVE(delta[u32SizeM2]) != IS_POSITIVE(delta[u32SizeM3]))
		&& (fabs(slopes[u32SizeM1]) > fabs(3.0f * delta[u32SizeM2]))
	) {
		slopes[u32SizeM1] = 3.0f * delta[u32SizeM2];
	}

	// Compute piecewise cubic Hermite interpolant to those values and slopes
	for (InIdx = 0; InIdx < u32SizeIn - 1; ++InIdx) {
		for (OutIdx = 0; OutIdx < u32SizeOut; ++OutIdx) {
			if (ps32XIn[InIdx] <= (ps32XIn[0] + (CVI_S32)OutIdx)) {
				pu16Temp[OutIdx] = InIdx;
				continue;
			}
		}
	}

	CVI_U16 u16K;
	float delta_K, slope_K, slope_K1;

	// Piecewise polynomial coefficients
	for (OutIdx = 0; OutIdx < u32SizeOut; ++OutIdx) {
		u16K = pu16Temp[OutIdx];
		delta_K = delta[u16K];
		slope_K = slopes[u16K];
		slope_K1 = slopes[u16K + 1];

		// float c = (3.0f * delta[u16K] - 2.0f * slopes[u16K] - slopes[u16K + 1]) / h[u16K];
		// float b = (slopes[u16K] - 2.0f * delta[u16K] + slopes[u16K + 1]) / (h[u16K] * h[u16K]);

		float c = (delta_K + delta_K + delta_K - slope_K - slope_K - slope_K1) / h[u16K];
		float b = (slope_K - delta_K - delta_K + slope_K1) / (h2[u16K]);
		float s = OutIdx + ps32XIn[0] - ps32XIn[u16K];
		float tmp = ps32YIn[u16K] + s * (slope_K + s * (c + s * b));

		ps32YOut[OutIdx] = LIMIT_IN((CVI_S32)round(tmp), s32MinYValue, s32MaxYValue);
	}

	return CVI_SUCCESS;
}

// ----------------------------------------------------------------------------
static inline CVI_U32 PCHIP_InterP1_GetReferenceInIndex(CVI_U32 u32Offset, const CVI_S32 *ps32XIn, CVI_U32 u32XInSize)
{
	CVI_S32 s32InIdx;
	CVI_S32 s32Temp;

	s32Temp = ps32XIn[0] + (CVI_S32)u32Offset;
	for (s32InIdx = u32XInSize - 2; s32InIdx >= 0; s32InIdx--) {
		if (ps32XIn[s32InIdx] <= s32Temp) {
			return (CVI_U32)s32InIdx;
		}
	}

	return 0;
}

// ----------------------------------------------------------------------------
CVI_S32 PCHIP_InterP1_PCHIP_V2_Preprocess(TPCHIP_Info *ptInfo, TPCHIP_PrivateInfo *ptPrivateInfo)
{
	CVI_S32 *ps32XIn = ptInfo->ps32XIn;
	CVI_S32 *ps32YIn = ptInfo->ps32YIn;
	float *pH = ptPrivateInfo->h;
	float *pH2 = ptPrivateInfo->h2;
	float *pDelta = ptPrivateInfo->delta;
	float *pSlopes = ptPrivateInfo->slopes;
	CVI_U32 u32SizeIn = ptInfo->u32SizeIn;

	CVI_U32 u32InIdx, u32InIdxM1;

	if (PCHIP_InterP1_PCHIP_Check_Param(ptInfo) != CVI_SUCCESS) {
		return CVI_FAILURE;
	}

	// get interval length h
	// get delta value
	for (u32InIdx = 0; u32InIdx < u32SizeIn - 1; ++u32InIdx) {
		pH[u32InIdx] = ps32XIn[u32InIdx + 1] - ps32XIn[u32InIdx];
		pDelta[u32InIdx] = (float)(ps32YIn[u32InIdx + 1] - ps32YIn[u32InIdx]) / pH[u32InIdx];
		pH2[u32InIdx] = pH[u32InIdx] * pH[u32InIdx];
	}

	// get slopes
	// Slopes at interior points.
	for (u32InIdx = 1, u32InIdxM1 = 0; u32InIdx < u32SizeIn - 1; ++u32InIdx, ++u32InIdxM1) {
		if (IS_POSITIVE(pDelta[u32InIdxM1]) && IS_POSITIVE(pDelta[u32InIdx])) {
			float w1 = pH[u32InIdxM1] + pH[u32InIdx] + pH[u32InIdx];
			float w2 = pH[u32InIdxM1] + pH[u32InIdxM1] + pH[u32InIdx];

			pSlopes[u32InIdx] = (w1 + w2) / ((w1 / pDelta[u32InIdxM1]) + (w2 / pDelta[u32InIdx]));
		}
	}

	// Slopes at start points.
	pSlopes[0] = ((pH[0] + pH[0] + pH[1]) * pDelta[0] - (pH[0] * pDelta[1])) / (pH[0] + pH[1]);
	if (IS_POSITIVE(pSlopes[0]) != IS_POSITIVE(pDelta[0])) {
		pSlopes[0] = 0.0f;
	} else if (
		(IS_POSITIVE(pDelta[0]) != IS_POSITIVE(pDelta[1]))
		&& (fabs(pSlopes[0]) > fabs(3.0f * pDelta[0]))
	) {
		pSlopes[0] = 3.0f * pDelta[0];
	}

	CVI_U32 u32SizeM1, u32SizeM2, u32SizeM3;

	u32SizeM1 = u32SizeIn - 1;
	u32SizeM2 = u32SizeIn - 2;
	u32SizeM3 = u32SizeIn - 3;

	// Slopes at end points.
	pSlopes[u32SizeM1] = (
			(pH[u32SizeM2] + pH[u32SizeM2] + pH[u32SizeM3]) * pDelta[u32SizeM2]
			- (pH[u32SizeM2] * pDelta[u32SizeM3])
			) / (pH[u32SizeM2] + pH[u32SizeM3]);

	if (IS_POSITIVE(pSlopes[u32SizeM1]) != IS_POSITIVE(pDelta[u32SizeM2])) {
		pSlopes[u32SizeM1] = 0.0f;
	} else if (
		(IS_POSITIVE(pDelta[u32SizeM2]) != IS_POSITIVE(pDelta[u32SizeM3]))
		&& (fabs(pSlopes[u32SizeM1]) > fabs(3.0f * pDelta[u32SizeM2]))
	) {
		pSlopes[u32SizeM1] = 3.0f * pDelta[u32SizeM2];
	}

	return CVI_SUCCESS;
}

// ----------------------------------------------------------------------------
CVI_S32 PCHIP_InterP1_PCHIP_V2_Process(CVI_U32 u32XOutIndex,
	TPCHIP_Info *ptInfo, TPCHIP_PrivateInfo *ptPrivateInfo, CVI_S32 *ps32YOutValue)
{
	CVI_S32 *ps32XIn = ptInfo->ps32XIn;
	CVI_S32 *ps32YIn = ptInfo->ps32YIn;
	float *pH = ptPrivateInfo->h;
	float *pH2 = ptPrivateInfo->h2;
	float *pDelta = ptPrivateInfo->delta;
	float *pSlopes = ptPrivateInfo->slopes;

	CVI_U16 u16K;
	float c, b, s, tmp;
	float fDelta, fSlopes, fSlopesP1, fH, fH2;

	if (u32XOutIndex >= ptInfo->u32SizeOut) {
		ISP_ALGO_LOG_WARNING("Invalid query index (%d)\n", u32XOutIndex);
		*ps32YOutValue = ps32XIn[0];
		return CVI_FAILURE;
	}

	// Piecewise polynomial coefficients
	u16K = PCHIP_InterP1_GetReferenceInIndex(u32XOutIndex, ps32XIn, ptInfo->u32SizeIn);
	fDelta = pDelta[u16K];
	fSlopes = pSlopes[u16K];
	fSlopesP1 = pSlopes[u16K + 1];
	fH = pH[u16K];
	fH2 = pH2[u16K];

	// c = (3.0f * fDelta - 2.0f * fSlopes - fSlopesP1) / fH;
	// b = (fSlopes - 2.0f * fDelta + fSlopesP1) / fH2;
	c = (fDelta + fDelta + fDelta - fSlopes - fSlopes - fSlopesP1) / fH;
	b = (fSlopes - fDelta - fDelta + fSlopesP1) / fH2;
	s = u32XOutIndex + ps32XIn[0] - ps32XIn[u16K];
	tmp = ps32YIn[u16K] + s * (fSlopes + s * (c + s * b));

	*ps32YOutValue = LIMIT_IN((CVI_S32)round(tmp), ptInfo->s32MinYValue, ptInfo->s32MaxYValue);

	return CVI_SUCCESS;
}

// ----------------------------------------------------------------------------
