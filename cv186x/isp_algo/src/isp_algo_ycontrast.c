/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_ycontrast.c
 * Description:
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

//#include <cvi_defines.h>

#include "isp_algo_ycontrast.h"
#include "isp_algo_debug.h"

#include "pchip.h"

#define Y_CURVE_MAX_INPUT_NODE		(6)
#define Y_CURVE_MAX_HORIZONTAL		(64)
#define Y_CURVE_MAX_VERTICAL		(256)
#define Y_CURVE_MAX_LUT				(255)

static int ycontrast_curve_gen(CVI_U8 u8ContrastLow, CVI_U8 u8ContrastHigh, CVI_U8 u8CenterLuma,
	CVI_U8 *pau8YCurve_YLut, CVI_U16 *pu16YCurve_YLastPoint);

CVI_S32 isp_algo_ycontrast_main(
	struct ycontrast_param_in *param_in, struct ycontrast_param_out *param_out)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	ycontrast_curve_gen(
		param_in->ContrastLow, param_in->ContrastHigh, param_in->CenterLuma,
		param_out->au8YCurve_Lut, &(param_out->u16YCurve_LastPoint));

	return ret;
}

CVI_S32 isp_algo_ycontrast_init(void)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	return ret;
}

CVI_S32 isp_algo_ycontrast_uninit(void)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	return ret;
}

static int ycontrast_curve_gen(CVI_U8 u8ContrastLow, CVI_U8 u8ContrastHigh, CVI_U8 u8CenterLuma,
	CVI_U8 *pau8YCurve_YLut, CVI_U16 *pu16YCurve_YLastPoint)
{
	TPCHIP_Info stPCHIP;

	CVI_S32 as32XIn[Y_CURVE_MAX_INPUT_NODE] = {0};
	CVI_S32 as32YIn[Y_CURVE_MAX_INPUT_NODE] = {0};
	CVI_S32 as32YOut[Y_CURVE_MAX_HORIZONTAL + 1] = {0};
	CVI_U16 au16Temp[Y_CURVE_MAX_HORIZONTAL + 1];

	CVI_U32 u32CenterLuma64 = (CVI_U32)u8CenterLuma;
	CVI_U32 u32CenterLuma256 = u32CenterLuma64 << 2;

	CVI_U32 u32LowNode = u32CenterLuma64 >> 1;
	CVI_U32 u32HighNode = (u32CenterLuma64 + 1 + Y_CURVE_MAX_HORIZONTAL) >> 1;

	CVI_U32 u32LowTarget = round((CVI_FLOAT)(u32CenterLuma256 * (100 - u8ContrastLow)) / 100.0f);
	CVI_U32 u32HighTarget = round((CVI_FLOAT)((Y_CURVE_MAX_VERTICAL - u32CenterLuma256) * u8ContrastHigh)
								/ 100.0f + u32CenterLuma256);

	stPCHIP.ps32XIn = as32XIn;
	stPCHIP.ps32YIn = as32YIn;
	stPCHIP.ps32YOut = as32YOut;
	stPCHIP.pu16Temp = au16Temp;
	stPCHIP.s32MinYValue = 0;
	stPCHIP.s32MaxYValue = Y_CURVE_MAX_VERTICAL;
	stPCHIP.u32SizeOut = Y_CURVE_MAX_HORIZONTAL + 1;

	if (u32CenterLuma64 == 0) {
		as32XIn[0] = 0;							as32YIn[0] = u32CenterLuma256;
		as32XIn[1] = u32HighNode;				as32YIn[1] = u32HighTarget;
		as32XIn[2] = Y_CURVE_MAX_HORIZONTAL;	as32YIn[2] = Y_CURVE_MAX_VERTICAL;
		stPCHIP.u32SizeIn = 3;
	} else if (u32CenterLuma64 == 1) {
		as32XIn[0] = 0;							as32YIn[0] = u32LowTarget;
		as32XIn[1] = u32CenterLuma64;			as32YIn[1] = u32CenterLuma256;
		as32XIn[2] = u32HighNode;				as32YIn[2] = u32HighTarget;
		as32XIn[3] = Y_CURVE_MAX_HORIZONTAL;	as32YIn[3] = Y_CURVE_MAX_VERTICAL;
		stPCHIP.u32SizeIn = 4;
	} else if (u32CenterLuma64 == (Y_CURVE_MAX_HORIZONTAL - 1)) {
		as32XIn[0] = 0;							as32YIn[0] = 0;
		as32XIn[1] = u32LowNode;				as32YIn[1] = u32LowTarget;
		as32XIn[2] = u32CenterLuma64;			as32YIn[2] = u32CenterLuma256;
		as32XIn[3] = u32HighNode;				as32YIn[3] = u32HighTarget;
		stPCHIP.u32SizeIn = 4;
	} else if (u32CenterLuma64 == Y_CURVE_MAX_HORIZONTAL) {
		as32XIn[0] = 0;							as32YIn[0] = 0;
		as32XIn[1] = u32LowNode;				as32YIn[1] = u32LowTarget;
		as32XIn[2] = u32CenterLuma64;			as32YIn[2] = u32CenterLuma256;
		stPCHIP.u32SizeIn = 3;
	} else {
		as32XIn[0] = 0;							as32YIn[0] = 0;
		as32XIn[1] = u32LowNode;				as32YIn[1] = u32LowTarget;
		as32XIn[2] = u32CenterLuma64;			as32YIn[2] = u32CenterLuma256;
		as32XIn[3] = u32HighNode;				as32YIn[3] = u32HighTarget;
		as32XIn[4] = Y_CURVE_MAX_HORIZONTAL;	as32YIn[4] = Y_CURVE_MAX_VERTICAL;
		stPCHIP.u32SizeIn = 5;
	}

	if (PCHIP_InterP1_PCHIP(&stPCHIP) != CVI_SUCCESS) {
		return CVI_FAILURE;
	}

	for (CVI_U32 i = 0; i < YCURVE_LUT_ENTRY_NUM; ++i) {
		pau8YCurve_YLut[i] = (as32YOut[i] >= Y_CURVE_MAX_LUT) ? Y_CURVE_MAX_LUT : (CVI_U8)as32YOut[i];
	}
	*pu16YCurve_YLastPoint = (CVI_U16)as32YOut[Y_CURVE_MAX_HORIZONTAL];

	return CVI_SUCCESS;
}
