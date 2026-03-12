/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: pchip.h
 * Description:
 *
 */

#ifndef __PCHIP_H__
#define __PCHIP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include "cvi_comm_isp.h"

#define PCHIP_MAX_INPUT_COUNT	(16)

typedef struct _PCHIP_INFO {
	CVI_S32		*ps32XIn;		// size : u32SizeIn
	CVI_S32		*ps32YIn;		// size : u32SizeIn
	CVI_S32		*ps32YOut;		// size : u32SizeOut		(V2 unused)
	CVI_U16		*pu16Temp;		// size : u32SizeOut		(V2 unused)
	CVI_U32		u32SizeIn;		// value <= PCHIP_MAX_INPUT_COUNT
	CVI_U32		u32SizeOut;		// value = (ps32XIn[max] - ps32XIn[0] + 1)
	CVI_S32		s32MinYValue;
	CVI_S32		s32MaxYValue;
} TPCHIP_Info;

typedef struct _TPCHIP_PrivateInfo {
	float h[PCHIP_MAX_INPUT_COUNT];			// interval length h
	float h2[PCHIP_MAX_INPUT_COUNT];		// interval length h^2
	float delta[PCHIP_MAX_INPUT_COUNT];		// delta value
	float slopes[PCHIP_MAX_INPUT_COUNT];	// slopes
} TPCHIP_PrivateInfo;

CVI_S32 PCHIP_InterP1_PCHIP(TPCHIP_Info *ptInfo);

CVI_S32 PCHIP_InterP1_PCHIP_V2_Preprocess(TPCHIP_Info *ptInfo, TPCHIP_PrivateInfo *ptPrivateInfo);
CVI_S32 PCHIP_InterP1_PCHIP_V2_Process(CVI_U32 u32XOutIndex,
	TPCHIP_Info *ptInfo, TPCHIP_PrivateInfo *ptPrivateInfo, CVI_S32 *ps32YOutValue);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // __PCHIP_H__
