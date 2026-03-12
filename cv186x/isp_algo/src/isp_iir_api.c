/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2022. All rights reserved.
 *
 * File Name: isp_iir_api.c
 * Description:
 *
 */

#include <string.h>

#if SDK_ARM_ARCH == 1
#include <arm_neon.h>
#endif // SDK_ARM_ARCH

#include "isp_iir_api.h"

#define IIR_U8_DECIMAL_BITS			(10)
#define IIR_U8_ROUNDING_HALF		(1 << (IIR_U8_DECIMAL_BITS - 1))

#define IIR_U10_DECIMAL_BITS		(10)
#define IIR_U10_ROUNDING_HALF		(1 << (IIR_U10_DECIMAL_BITS - 1))

#define IIR_U16_DECIMAL_BITS		(10)
#define IIR_U16_ROUNDING_HALF		(1 << (IIR_U16_DECIMAL_BITS - 1))
// -----------------------------------------------------------------------------
int IIR_U8_Once(TIIR_U8_Ctrl *ptIIRCoef)
{
	uint8_t		*pu8In = ptIIRCoef->pu8LutIn;
	uint32_t	*pu32History = ptIIRCoef->pu32LutHistory;
	uint8_t		*pu8Out = ptIIRCoef->pu8LutOut;
	uint16_t	u16IIRWeight = ptIIRCoef->u16IIRWeight;
	uint16_t	u16Idx;

	if (u16IIRWeight > 1024) {
		u16IIRWeight = 1024;
	}

#ifndef __ARM_NEON__
	if (u16IIRWeight <= 1) {
		for (u16Idx = 0; u16Idx < ptIIRCoef->u16LutSize; ++u16Idx) {
			pu32History[u16Idx] = (uint32_t)pu8In[u16Idx] << IIR_U8_DECIMAL_BITS;

			pu8Out[u16Idx] = pu8In[u16Idx];
		}
	} else {
		uint16_t	u16IIRDenominator;
		uint32_t	u32InShift, u32Sum;

		u16IIRDenominator = u16IIRWeight + 1;

		for (u16Idx = 0; u16Idx < ptIIRCoef->u16LutSize; ++u16Idx) {
			u32InShift = (uint32_t)pu8In[u16Idx] << IIR_U8_DECIMAL_BITS;
			u32Sum = (pu32History[u16Idx] * u16IIRWeight) + u32InShift;
			pu32History[u16Idx] = u32Sum / u16IIRDenominator;

			pu8Out[u16Idx] =
				(uint8_t)((pu32History[u16Idx] + IIR_U8_ROUNDING_HALF) >> IIR_U8_DECIMAL_BITS);
		}
	}
#else
	if (u16IIRWeight <= 1) {
		uint8x8_t	vu8x8In;
		uint16x8_t	vu16x8In;
		uint16x4_t	vu16x4InHigh, vu16x4InLow;
		uint32x4_t	vu32x4HistoryHigh, vu32x4HistoryLow;
		uint16_t	u16NeonIndexStop;

		u16NeonIndexStop = ptIIRCoef->u16LutSize & 0xFFF0;

		for (u16Idx = 0; u16Idx < u16NeonIndexStop; u16Idx += 8) {
			vu8x8In = vld1_u8(pu8In + u16Idx);
			vu16x8In = vmovl_u8(vu8x8In);

			vu16x4InHigh = vget_high_u16(vu16x8In);
			vu32x4HistoryHigh = vshll_n_u16(vu16x4InHigh, IIR_U8_DECIMAL_BITS);

			vu16x4InLow = vget_low_u16(vu16x8In);
			vu32x4HistoryLow = vshll_n_u16(vu16x4InLow, IIR_U8_DECIMAL_BITS);

			vst1q_u32(pu32History + u16Idx + 4, vu32x4HistoryHigh);
			vst1q_u32(pu32History + u16Idx, vu32x4HistoryLow);

			memcpy(pu8Out + u16Idx, pu8In + u16Idx, 8);
		}

		for (; u16Idx < ptIIRCoef->u16LutSize; ++u16Idx) {
			pu32History[u16Idx] = (uint32_t)pu8In[u16Idx] << IIR_U8_DECIMAL_BITS;
			pu8Out[u16Idx] = pu8In[u16Idx];
		}
	} else {
		uint16_t	u16IIRDenominator;
		uint32_t	u32InShift, u32Sum;
		uint16_t	u16NeonIndexStop;

		uint8x8_t	vu8x8In;
		uint16x8_t	vu16x8In;

		uint16x4_t	vu16x4PartialIn;
		uint32x4_t	vu32x4InShift;
		uint32x4_t	vu32x4History;
		uint32x4_t	vu32x4IIRWeight;
		uint32x4_t	vu32x4IIRoundHalf;
		uint32x4_t	vu32x4Sum;
		float32x4_t	vf32x4IIRDenominator;
		float32x4_t	vf32x4Sum;
		float32x4_t	vf32x4History;
		uint32x4_t	vu32x4PartOut;
		uint16x4_t	vu16x4PartOutHigh, vu16x4PartOutLow;
		uint16x8_t	vu16x8Out;
		uint8x8_t	vu8x8Out;

		u16IIRDenominator = u16IIRWeight + 1;
		u16NeonIndexStop = ptIIRCoef->u16LutSize & 0xFFF0;

		vu32x4IIRWeight = vdupq_n_u32(u16IIRWeight);
		vu32x4IIRoundHalf = vdupq_n_u32(IIR_U8_ROUNDING_HALF);
		vf32x4IIRDenominator = vdupq_n_f32(1.0f / (float)u16IIRDenominator);

		for (u16Idx = 0; u16Idx < u16NeonIndexStop; u16Idx += 8) {
			vu8x8In = vld1_u8(pu8In + u16Idx);
			vu16x8In = vmovl_u8(vu8x8In);

			vu16x4PartialIn = vget_high_u16(vu16x8In);
			vu32x4History = vld1q_u32(pu32History + u16Idx + 4);
			vu32x4InShift = vshll_n_u16(vu16x4PartialIn, IIR_U8_DECIMAL_BITS);
			vu32x4Sum = vmlaq_u32(vu32x4InShift, vu32x4History, vu32x4IIRWeight);
			vf32x4Sum = vcvtq_f32_u32(vu32x4Sum);
			vf32x4History = vmulq_f32(vf32x4Sum, vf32x4IIRDenominator);
			vu32x4History = vcvtq_u32_f32(vf32x4History);
			vst1q_u32(pu32History + u16Idx + 4, vu32x4History);
			vu32x4History = vaddq_u32(vu32x4History, vu32x4IIRoundHalf);
			vu32x4PartOut = vshrq_n_u32(vu32x4History, IIR_U8_DECIMAL_BITS);
			vu16x4PartOutHigh = vqmovn_u32(vu32x4PartOut);

			vu16x4PartialIn = vget_low_u16(vu16x8In);
			vu32x4History = vld1q_u32(pu32History + u16Idx);
			vu32x4InShift = vshll_n_u16(vu16x4PartialIn, IIR_U8_DECIMAL_BITS);
			vu32x4Sum = vmlaq_u32(vu32x4InShift, vu32x4History, vu32x4IIRWeight);
			vf32x4Sum = vcvtq_f32_u32(vu32x4Sum);
			vf32x4History = vmulq_f32(vf32x4Sum, vf32x4IIRDenominator);
			vu32x4History = vcvtq_u32_f32(vf32x4History);
			vst1q_u32(pu32History + u16Idx, vu32x4History);
			vu32x4History = vaddq_u32(vu32x4History, vu32x4IIRoundHalf);
			vu32x4PartOut = vshrq_n_u32(vu32x4History, IIR_U8_DECIMAL_BITS);
			vu16x4PartOutLow = vqmovn_u32(vu32x4PartOut);

			vu16x8Out = vcombine_u16(vu16x4PartOutLow, vu16x4PartOutHigh);
			vu8x8Out = vqmovn_u16(vu16x8Out);
			vst1_u8(pu8Out + u16Idx, vu8x8Out);
		}

		for (; u16Idx < ptIIRCoef->u16LutSize; ++u16Idx) {
			u32InShift = (uint32_t)pu8In[u16Idx] << IIR_U8_DECIMAL_BITS;
			u32Sum = (pu32History[u16Idx] * u16IIRWeight) + u32InShift + IIR_U8_ROUNDING_HALF;
			pu32History[u16Idx] = u32Sum / u16IIRDenominator;

			pu8Out[u16Idx] = (uint8_t)(pu32History[u16Idx] >> IIR_U8_DECIMAL_BITS);
		}
	}
#endif // __ARM_NEON__

	return 0;
}

// -----------------------------------------------------------------------------
int IIR_U8_UpdateIIROutFromHistory(TIIR_U8_Ctrl *ptIIRCoef)
{
	uint32_t	*pu32History = ptIIRCoef->pu32LutHistory;
	uint8_t		*pu8Out = ptIIRCoef->pu8LutOut;

	for (uint16_t u16Idx = 0; u16Idx < ptIIRCoef->u16LutSize; ++u16Idx) {
		pu8Out[u16Idx] = (uint8_t)(pu32History[u16Idx] >> IIR_U8_DECIMAL_BITS);
	}

	return 0;
}

// -----------------------------------------------------------------------------
int IIR_U10_Once(TIIR_U10_Ctrl *ptIIRCoef)
{
	uint16_t	*pu16In = ptIIRCoef->pu16LutIn;
	uint32_t	*pu32History = ptIIRCoef->pu32LutHistory;
	uint16_t	*pu16Out = ptIIRCoef->pu16LutOut;
	uint16_t	u16IIRWeight = ptIIRCoef->u16IIRWeight;
	uint16_t	u16Idx;

	if (u16IIRWeight > 1024) {
		u16IIRWeight = 1024;
	}

#ifndef __ARM_NEON__
	if (u16IIRWeight <= 1) {
		for (u16Idx = 0; u16Idx < ptIIRCoef->u16LutSize; ++u16Idx) {
			pu32History[u16Idx] = (uint32_t)pu16In[u16Idx] << IIR_U10_DECIMAL_BITS;

			pu16Out[u16Idx] = pu16In[u16Idx];
		}
	} else {
		uint16_t	u16IIRDenominator;
		uint32_t	u32InShift, u32Sum;

		u16IIRDenominator = u16IIRWeight + 1;

		for (u16Idx = 0; u16Idx < ptIIRCoef->u16LutSize; ++u16Idx) {
			u32InShift = (uint32_t)pu16In[u16Idx] << IIR_U10_DECIMAL_BITS;
			u32Sum = (pu32History[u16Idx] * u16IIRWeight) + u32InShift;
			pu32History[u16Idx] = u32Sum / u16IIRDenominator;

			pu16Out[u16Idx] =
				(uint16_t)((pu32History[u16Idx] + IIR_U10_ROUNDING_HALF) >> IIR_U10_DECIMAL_BITS);
		}
	}
#else
	if (u16IIRWeight <= 1) {
		uint16x4_t	vu16x4In;
		uint32x4_t	vu32x4History;
		uint16_t	u16NeonIndexStop;

		u16NeonIndexStop = ptIIRCoef->u16LutSize & 0xFFFC;

		for (u16Idx = 0; u16Idx < u16NeonIndexStop; u16Idx += 4) {
			vu16x4In = vld1_u16(pu16In + u16Idx);
			vu32x4History = vshll_n_u16(vu16x4In, IIR_U10_DECIMAL_BITS);
			vst1q_u32(pu32History + u16Idx, vu32x4History);
			memcpy(pu16Out + u16Idx, pu16In + u16Idx, 4);
		}

		for (; u16Idx < ptIIRCoef->u16LutSize; ++u16Idx) {
			pu32History[u16Idx] = (uint32_t)pu16In[u16Idx] << IIR_U10_DECIMAL_BITS;
			pu16Out[u16Idx] = pu16In[u16Idx];
		}
	} else {
		uint16_t	u16IIRDenominator;
		uint32_t	u32InShift, u32Sum;
		uint16_t	u16NeonIndexStop;
		uint32x4_t	vu32x4IIRWeight;
		uint32x4_t	vu32x4IIRoundHalf;
		float32x4_t	vf32x4IIRDenominator;

		uint16x4_t	vu16x4In;
		uint32x4_t	vu32x4History;
		uint32x4_t	vu32x4InShift;
		uint32x4_t	vu32x4Sum;

		float32x4_t	vf32x4Sum;
		float32x4_t	vf32x4History;
		uint16x4_t	vu16x4Out;

		u16IIRDenominator = u16IIRWeight + 1;
		u16NeonIndexStop = ptIIRCoef->u16LutSize & 0xFFFC;

		vu32x4IIRWeight = vdupq_n_u32(u16IIRWeight);
		vu32x4IIRoundHalf = vdupq_n_u32(IIR_U10_ROUNDING_HALF);
		vf32x4IIRDenominator = vdupq_n_f32(1.0f / (float)u16IIRDenominator);

		for (u16Idx = 0; u16Idx < u16NeonIndexStop; u16Idx += 4) {
			vu16x4In = vld1_u16(pu16In + u16Idx);
			vu32x4History = vld1q_u32(pu32History + u16Idx);

			vu32x4InShift = vshll_n_u16(vu16x4In, IIR_U10_DECIMAL_BITS);
			vu32x4Sum = vmlaq_u32(vu32x4InShift, vu32x4History, vu32x4IIRWeight);
			vf32x4Sum = vcvtq_f32_u32(vu32x4Sum);
			vf32x4History = vmulq_f32(vf32x4Sum, vf32x4IIRDenominator);
			vu32x4History = vcvtq_u32_f32(vf32x4History);
			vst1q_u32(pu32History + u16Idx, vu32x4History);

			vu32x4History = vaddq_u32(vu32x4History, vu32x4IIRoundHalf);
			vu16x4Out = vqshrn_n_u32(vu32x4History, IIR_U10_DECIMAL_BITS);
			vst1_u16(pu16Out + u16Idx, vu16x4Out);
		}

		for (; u16Idx < ptIIRCoef->u16LutSize; ++u16Idx) {
			u32InShift = (uint32_t)pu16In[u16Idx] << IIR_U10_DECIMAL_BITS;
			u32Sum = (pu32History[u16Idx] * u16IIRWeight) + u32InShift + IIR_U10_ROUNDING_HALF;
			pu32History[u16Idx] = u32Sum / u16IIRDenominator;

			pu16Out[u16Idx] = (uint16_t)(pu32History[u16Idx] >> IIR_U10_DECIMAL_BITS);
		}
	}
#endif // __ARM_NEON__

	return 0;
}

// -----------------------------------------------------------------------------
int IIR_U10_UpdateIIROutFromHistory(TIIR_U10_Ctrl *ptIIRCoef)
{
	uint32_t	*pu32History = ptIIRCoef->pu32LutHistory;
	uint16_t	*pu16Out = ptIIRCoef->pu16LutOut;

	for (uint16_t u16Idx = 0; u16Idx < ptIIRCoef->u16LutSize; ++u16Idx) {
		pu16Out[u16Idx] = (uint16_t)(pu32History[u16Idx] >> IIR_U10_DECIMAL_BITS);
	}

	return 0;
}

// -----------------------------------------------------------------------------
int IIR_U16_Once(TIIR_U16_Ctrl *ptIIRCoef)
{
	uint16_t	*pu16In = ptIIRCoef->pu16LutIn;
	uint32_t	*pu32History = ptIIRCoef->pu32LutHistory;
	uint16_t	*pu16Out = ptIIRCoef->pu16LutOut;
	uint16_t	u16IIRWeight = ptIIRCoef->u16IIRWeight;
	uint16_t	u16Idx;

	if (u16IIRWeight > 1024) {
		u16IIRWeight = 1024;
	}

	if (u16IIRWeight <= 1) {
		for (u16Idx = 0; u16Idx < ptIIRCoef->u16LutSize; ++u16Idx) {
			pu32History[u16Idx] = (uint32_t)pu16In[u16Idx] << IIR_U16_DECIMAL_BITS;

			pu16Out[u16Idx] = pu16In[u16Idx];
		}
	} else {
		uint16_t	u16IIRDenominator;
		uint32_t	u32InShift;
		uint64_t	u64Sum;

		u16IIRDenominator = u16IIRWeight + 1;

		for (u16Idx = 0; u16Idx < ptIIRCoef->u16LutSize; ++u16Idx) {
			u32InShift = (uint32_t)pu16In[u16Idx] << IIR_U16_DECIMAL_BITS;
			u64Sum = (pu32History[u16Idx] * u16IIRWeight) + u32InShift;
			pu32History[u16Idx] = u64Sum / u16IIRDenominator;

			pu16Out[u16Idx] =
				(uint16_t)((pu32History[u16Idx] + IIR_U16_ROUNDING_HALF) >> IIR_U16_DECIMAL_BITS);
		}
	}

	return 0;
}

// -----------------------------------------------------------------------------
int IIR_U16_UpdateIIROutFromHistory(TIIR_U16_Ctrl *ptIIRCoef)
{
	uint32_t	*pu32History = ptIIRCoef->pu32LutHistory;
	uint16_t	*pu16Out = ptIIRCoef->pu16LutOut;

	for (uint16_t u16Idx = 0; u16Idx < ptIIRCoef->u16LutSize; ++u16Idx) {
		pu16Out[u16Idx] = (uint16_t)(pu32History[u16Idx] >> IIR_U16_DECIMAL_BITS);
	}

	return 0;
}
