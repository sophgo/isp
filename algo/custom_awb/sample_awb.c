/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: sample_awb.c
 * Description:
 *
 */

/**************************************************************************
 *                        H E A D E R   F I L E S
 **************************************************************************/
#include <stdio.h>
#include <stdlib.h>

#include "cvi_base.h"
#include "cvi_comm_isp.h"
#include "cvi_comm_3a.h"
#include "cvi_awb_comm.h"
#include "cvi_isp.h"

#include "sample_awb.h"

/**************************************************************************
 *                          C O N S T A N T S
 **************************************************************************/
#define _SHOW_AWB_INFO_	(0)
#define CHECK_AWB_TIMING (0)

/**************************************************************************
 *                              M A C R O S
 **************************************************************************/
/**************************************************************************
 *                          D A T A   T Y P E S
 **************************************************************************/
/**************************************************************************
 *                  E X T E R N A L   R E F E R E N C E
 **************************************************************************/
/**************************************************************************
 *              F U N C T I O N   D E C L A R A T I O N S
 **************************************************************************/
/**************************************************************************
 *                        G L O B A L   D A T A
 **************************************************************************/
#if CHECK_AWB_TIMING
static struct timeval tt_awb[20];
#define MEAUSURE_AWB_T(a) gettimeofday(&tt_awb[a], NULL)
#else
#define MEAUSURE_AWB_T(a)
#endif

static SAMPLE_AWB_CTX_S g_astAwbCtx[MAX_AWB_LIB_NUM] = {
	{
		0
	}
};

CVI_U32 SAMPLE_AwbGetUSTimeDiff(const struct timeval *before, const struct timeval *after)
{
	CVI_U32 CVI_timeDiff = 0;

	CVI_timeDiff = (after->tv_sec - before->tv_sec) * 1000000 + (after->tv_usec - before->tv_usec);

	return CVI_timeDiff;
}

CVI_S32 SAMPLE_AWB_Calculate(CVI_S32 s32Handle)
{
	SAMPLE_AWB_CTX_S *pstAwbCtx = CVI_NULL;
	ISP_AWB_INFO_S *pInfo;
	CVI_U8 col, row;
	CVI_U32 i, k, totalR[ISP_CHANNEL_MAX_NUM] = {0},
		totalG[ISP_CHANNEL_MAX_NUM] = {0}, totalB[ISP_CHANNEL_MAX_NUM] = {0};
	CVI_U32 rGain[ISP_CHANNEL_MAX_NUM] = {0}, bGain[ISP_CHANNEL_MAX_NUM] = {0};
	CVI_U8 isWDR, frmNum;

	static CVI_U32 last_rGain[ISP_CHANNEL_MAX_NUM] = {AWB_GAIN_BASE},
		last_bGain[ISP_CHANNEL_MAX_NUM] = {AWB_GAIN_BASE};

	AWB_CHECK_HANDLE_ID(s32Handle);
	pstAwbCtx = AWB_GET_CTX(s32Handle);

	/* user's AWB alg implementor */
	pInfo = &pstAwbCtx->stAwbInfo;
	col = pstAwbCtx->stAwbParam.u8AWBZoneCol;
	row = pstAwbCtx->stAwbParam.u8AWBZoneRow;
	if (pstAwbCtx->stAwbParam.u8WDRMode) {
		isWDR = 1;
		frmNum = ISP_CHANNEL_MAX_NUM;
	} else {
		isWDR = 0;
		frmNum = 1;
	}

	for (k = 0; k < frmNum; k++) {
		for (i = 0; i < row * col; i++) {
			totalR[k] += pInfo->stAwbStat2[k].pau16ZoneAvgR[i];
			totalG[k] += pInfo->stAwbStat2[k].pau16ZoneAvgG[i];
			totalB[k] += pInfo->stAwbStat2[k].pau16ZoneAvgB[i];
		}
		if (totalR[k] != 0 && totalB[k] != 0) {
			rGain[k] = (totalG[k] * AWB_GAIN_BASE)/totalR[k];
			bGain[k] = (totalG[k] * AWB_GAIN_BASE)/totalB[k];
		} else {
			rGain[k] = last_rGain[k];
			bGain[k] = last_bGain[k];
		}
	}
	pstAwbCtx->stAwbResult.au32WhiteBalanceGain[0] = rGain[ISP_CHANNEL_LE];
	pstAwbCtx->stAwbResult.au32WhiteBalanceGain[1] = AWB_GAIN_BASE;
	pstAwbCtx->stAwbResult.au32WhiteBalanceGain[2] = AWB_GAIN_BASE;
	pstAwbCtx->stAwbResult.au32WhiteBalanceGain[3] = bGain[ISP_CHANNEL_LE];
	pstAwbCtx->stAwbResult.u32ColorTemp = 5000;
	pstAwbCtx->stAwbResult.bStable = 1;
	pstAwbCtx->stAwbResult.u8Saturation[0] = 128;//normal Saturation
	pstAwbCtx->stAwbResult.u8Saturation[1] = 128;
	pstAwbCtx->stAwbResult.u8Saturation[2] = 128;
	pstAwbCtx->stAwbResult.u8Saturation[3] = 128;


#if _SHOW_AWB_INFO_
	if (pstAwbCtx->u32FrameCnt % (SAMPLE_AWB_INTERVAL*10) == 0) {
		printf("Frm:%d lv:%d iso:%d\n", pstAwbCtx->u32FrameCnt, pInfo->s16LVx100, pInfo->u32IsoNum);
		printf("wdr %d,C:%d R:%d\n", isWDR, col, row);
		for (k = 0; k < frmNum; k++) {
			if (k == ISP_CHANNEL_LE)
				printf("Long  Exp\n");
			else
				printf("Short Exp\n");
			printf("	R_LU:%d RU:%d\n", pInfo->stAwbStat2[k].pau16ZoneAvgR[0],
				pInfo->stAwbStat2[k].pau16ZoneAvgR[SAMPLE_AWB_COL-1]);
			printf("	  LD:%d RD:%d\n",
				pInfo->stAwbStat2[k].pau16ZoneAvgR[(SAMPLE_AWB_ROW-1)*SAMPLE_AWB_COL],
				pInfo->stAwbStat2[k].pau16ZoneAvgR[(SAMPLE_AWB_ROW-1)*SAMPLE_AWB_COL+SAMPLE_AWB_COL-1]);

			printf("	G_LU:%d RU:%d\n", pInfo->stAwbStat2[k].pau16ZoneAvgG[0],
				pInfo->stAwbStat2[k].pau16ZoneAvgG[SAMPLE_AWB_COL-1]);
			printf("	  LD:%d RD:%d\n",
				pInfo->stAwbStat2[k].pau16ZoneAvgG[(SAMPLE_AWB_ROW-1)*SAMPLE_AWB_COL],
				pInfo->stAwbStat2[k].pau16ZoneAvgG[(SAMPLE_AWB_ROW-1)*SAMPLE_AWB_COL+SAMPLE_AWB_COL-1]);

			printf("	B_LU:%d RU:%d\n", pInfo->stAwbStat2[k].pau16ZoneAvgB[0],
				pInfo->stAwbStat2[k].pau16ZoneAvgB[SAMPLE_AWB_COL-1]);
			printf("	  LD:%d RD:%d\n",
				pInfo->stAwbStat2[k].pau16ZoneAvgB[(SAMPLE_AWB_ROW-1)*SAMPLE_AWB_COL],
				pInfo->stAwbStat2[k].pau16ZoneAvgB[(SAMPLE_AWB_ROW-1)*SAMPLE_AWB_COL+SAMPLE_AWB_COL-1]);
			printf("====Rgain:%d Bgain:%d\n", rGain[k], bGain[k]);
		}
	}
#endif
	UNUSED(isWDR);
	return CVI_SUCCESS;
}

CVI_S32 SAMPLE_AWB_Init(CVI_S32 s32Handle, const ISP_AWB_PARAM_S *pstAwbParam)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	VI_PIPE ViPipe;
	SAMPLE_AWB_CTX_S *pstAwbCtx = CVI_NULL;

	AWB_CHECK_HANDLE_ID(s32Handle);
	pstAwbCtx = AWB_GET_CTX(s32Handle);
	ViPipe = pstAwbCtx->IspBindDev;

	AWB_CHECK_POINTER(pstAwbParam);

	printf("Func %s line %d\n", __func__, __LINE__);

	UNUSED(ViPipe);
	/* do something ... like check the sensor id, init the global variables...
	 * and so on
	 */
	printf("sid:%d wdr:%d\n", pstAwbParam->SensorId, pstAwbParam->u8WDRMode);
	printf("W:%d H:%d\n", pstAwbParam->u16AWBWidth, pstAwbParam->u16AWBHeight);
	printf("ZoneCol:%d ZoneRow:%d\n", pstAwbParam->u8AWBZoneCol, pstAwbParam->u8AWBZoneRow);

	pstAwbCtx->stAwbParam = *pstAwbParam;

	pstAwbCtx->stAwbResult.au32WhiteBalanceGain[0] = AWB_GAIN_BASE;
	pstAwbCtx->stAwbResult.au32WhiteBalanceGain[1] = AWB_GAIN_BASE;
	pstAwbCtx->stAwbResult.au32WhiteBalanceGain[2] = AWB_GAIN_BASE;
	pstAwbCtx->stAwbResult.au32WhiteBalanceGain[3] = AWB_GAIN_BASE;
	pstAwbCtx->stAwbResult.u32ColorTemp = 5000;
	pstAwbCtx->stAwbResult.bStable = 1;
	pstAwbCtx->stAwbResult.u8Saturation[0] = 128;
	pstAwbCtx->stAwbResult.u8Saturation[1] = 128;
	pstAwbCtx->stAwbResult.u8Saturation[2] = 128;
	pstAwbCtx->stAwbResult.u8Saturation[3] = 128;

	return s32Ret;
}

//Cvitek add more info at ISP_AWB_INFO_S
//	u32IsoNum;	//current ISO num ,(from AE algo)
//	s16LVx100;	//current LightValue x100,(from AE algo)
//	fBVstep;	//currnet AE delta step,20=>6dB,(from AE algo)
//Cvitek add more info at ISP_AWB_RESULT_S
//	bStable;	//is AWB Stable
CVI_S32 SAMPLE_AWB_Run(CVI_S32 s32Handle, const ISP_AWB_INFO_S *pstAwbInfo,
	ISP_AWB_RESULT_S *pstAwbResult, CVI_S32 s32Rsv)
{
	MEAUSURE_AWB_T(0);

	SAMPLE_AWB_CTX_S *pstAwbCtx = CVI_NULL;

	AWB_CHECK_HANDLE_ID(s32Handle);
	pstAwbCtx = AWB_GET_CTX(s32Handle);

	AWB_CHECK_POINTER(pstAwbInfo);
	AWB_CHECK_POINTER(pstAwbResult);

	AWB_CHECK_POINTER(pstAwbInfo->stAwbStat2);
	pstAwbCtx->u32FrameCnt = pstAwbInfo->u32FrameCnt;

	UNUSED(s32Rsv);

	/* do something ... */
	if (pstAwbCtx->u32FrameCnt % SAMPLE_AWB_INTERVAL == 0) {//do AWB per 6 frame
		memcpy(&pstAwbCtx->stAwbInfo, pstAwbInfo, sizeof(ISP_AWB_INFO_S));
		SAMPLE_AWB_Calculate(s32Handle);
	}

	/* record result */
	memcpy(pstAwbResult, &pstAwbCtx->stAwbResult, sizeof(ISP_AWB_RESULT_S));

	MEAUSURE_AWB_T(1);
#if CHECK_AWB_TIMING
	printf("AWBT1:%d\n", SAMPLE_AwbGetUSTimeDiff(&tt_awb[0], &tt_awb[1]));
#endif

	return CVI_SUCCESS;
}

CVI_S32 SAMPLE_AWB_Ctrl(CVI_S32 s32Handle, CVI_U32 u32Cmd, CVI_VOID *pValue)
{
	//SAMPLE_AWB_CTX_S *pstAwbCtx = CVI_NULL;
	AWB_CHECK_HANDLE_ID(s32Handle);

	//pstAwbCtx = AWB_GET_CTX(s32Handle);
	AWB_CHECK_POINTER(pValue);

	//pstAwbCtx = pstAwbCtx;
	switch (u32Cmd) {
		/* system ctrl */
	case ISP_WDR_MODE_SET:

			/* do something ... */
		break;

	default:
		break;
	}

	return CVI_SUCCESS;
}

CVI_S32 SAMPLE_AWB_Exit(CVI_S32 s32Handle)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	SAMPLE_AWB_CTX_S *pstAwbCtx = CVI_NULL;

	AWB_CHECK_HANDLE_ID(s32Handle);
	pstAwbCtx = AWB_GET_CTX(s32Handle);

	printf("Func %s line %d\n", __func__, __LINE__);

	UNUSED(pstAwbCtx);

	/* do something ... */
	return s32Ret;
}

CVI_S32 CVI_AWB_Register(VI_PIPE ViPipe, ALG_LIB_S *pstAwbLib)
{
	ISP_AWB_REGISTER_S stRegister;
	CVI_S32 s32Ret = CVI_SUCCESS;

	AWB_CHECK_DEV(ViPipe);
	AWB_CHECK_POINTER(pstAwbLib);
	printf("Func %s line %d\n", __func__, __LINE__);

	stRegister.stAwbExpFunc.pfn_awb_init = SAMPLE_AWB_Init;//OK
	stRegister.stAwbExpFunc.pfn_awb_run = SAMPLE_AWB_Run;
	stRegister.stAwbExpFunc.pfn_awb_ctrl = SAMPLE_AWB_Ctrl;//OK
	stRegister.stAwbExpFunc.pfn_awb_exit = SAMPLE_AWB_Exit;//OK
	s32Ret = CVI_ISP_AWBLibRegCallBack(ViPipe, pstAwbLib, &stRegister);

	if (s32Ret != CVI_SUCCESS) {
		printf("Cvi_awb register failed!\n");
	}

	return s32Ret;
}

CVI_S32 CVI_AWB_UnRegister(VI_PIPE ViPipe, ALG_LIB_S *pstAwbLib)
{
	CVI_S32 s32Ret = CVI_SUCCESS;

	printf("Func %s line %d\n", __func__, __LINE__);

	AWB_CHECK_DEV(ViPipe);
	AWB_CHECK_POINTER(pstAwbLib);
	AWB_CHECK_HANDLE_ID(pstAwbLib->s32Id);
	AWB_CHECK_LIB_NAME(pstAwbLib->acLibName);

	s32Ret = CVI_ISP_AWBLibUnRegCallBack(ViPipe, pstAwbLib);
	if (s32Ret != CVI_SUCCESS) {
		printf("Cvi_awb unregister failed!\n");
	}

	return s32Ret;
}

CVI_S32 CVI_AWB_SensorRegCallBack(VI_PIPE ViPipe, ALG_LIB_S *pstAwbLib, ISP_SNS_ATTR_INFO_S *pstSnsAttrInfo,
	AWB_SENSOR_REGISTER_S *pstRegister)
{
	SAMPLE_AWB_CTX_S *pstAwbCtx = CVI_NULL;
	CVI_S32 s32Handle;

	AWB_CHECK_DEV(ViPipe);
	AWB_CHECK_POINTER(pstAwbLib);
	AWB_CHECK_POINTER(pstRegister);

	s32Handle = pstAwbLib->s32Id;
	AWB_CHECK_HANDLE_ID(s32Handle);
	AWB_CHECK_LIB_NAME(pstAwbLib->acLibName);

	pstAwbCtx = AWB_GET_CTX(s32Handle);
	if (pstRegister->stAwbExp.pfn_cmos_get_awb_default != CVI_NULL) {
		pstRegister->stAwbExp.pfn_cmos_get_awb_default(ViPipe, &pstAwbCtx->stSnsDft);
	}

	memcpy(&pstAwbCtx->stSnsRegister, pstRegister, sizeof(AWB_SENSOR_REGISTER_S));
	memcpy(&pstAwbCtx->stSnsAttrInfo, pstSnsAttrInfo, sizeof(ISP_SNS_ATTR_INFO_S));

	//pstAwbCtx->SensorId = SensorId;
	pstAwbCtx->bSnsRegister = CVI_TRUE;

	return CVI_SUCCESS;
}

CVI_S32 CVI_AWB_SensorUnRegCallBack(VI_PIPE ViPipe, ALG_LIB_S *pstAwbLib, SENSOR_ID SensorId)
{
	SAMPLE_AWB_CTX_S *pstAwbCtx = CVI_NULL;
	CVI_S32 s32Handle;

	AWB_CHECK_DEV(ViPipe);
	AWB_CHECK_POINTER(pstAwbLib);
	UNUSED(SensorId);

	s32Handle = pstAwbLib->s32Id;
	AWB_CHECK_HANDLE_ID(s32Handle);
	AWB_CHECK_LIB_NAME(pstAwbLib->acLibName);

	pstAwbCtx = AWB_GET_CTX(s32Handle);

	memset(&pstAwbCtx->stSnsDft, 0, sizeof(AWB_SENSOR_DEFAULT_S));
	memset(&pstAwbCtx->stSnsRegister, 0, sizeof(AWB_SENSOR_REGISTER_S));
	pstAwbCtx->stSnsAttrInfo.eSensorId = 0;
	pstAwbCtx->bSnsRegister = CVI_FALSE;

	return CVI_SUCCESS;
}

CVI_S32 SAMPLE_ISP_Awblib_Callback(ISP_DEV IspDev)
{ //SAMPLE_COMM_ISP_Awblib_Callback
	ALG_LIB_S stAwbLib;
	CVI_S32 s32Ret = 0;

	stAwbLib.s32Id = IspDev;
	strncpy(stAwbLib.acLibName, CVI_AWB_LIB_NAME, sizeof(CVI_AWB_LIB_NAME));
	s32Ret = CVI_AWB_Register(IspDev, &stAwbLib);

	if (s32Ret != CVI_SUCCESS) {
		printf("AWB Algo register failed!, error: %d\n", s32Ret);
		return s32Ret;
	}

	return CVI_SUCCESS;
}

CVI_S32 SAMPLE_ISP_Awblib_UnCallback(ISP_DEV IspDev)
{
	CVI_S32 s32Ret = 0;
	ALG_LIB_S stAwbLib;

	stAwbLib.s32Id = IspDev;
	strncpy(stAwbLib.acLibName, CVI_AWB_LIB_NAME, sizeof(CVI_AWB_LIB_NAME));
	s32Ret = CVI_AWB_UnRegister(IspDev, &stAwbLib);
	if (s32Ret) {
		printf("AWB Algo unRegister failed!, error: %d\n", s32Ret);
		return s32Ret;
	}
	return CVI_SUCCESS;
}

