/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: sample_ae.c
 * Description:
 *
 */

/**************************************************************************
 *						  H E A D E R	F I L E S
 **************************************************************************/
#include <stdio.h>
#include <stdlib.h>

#include "cvi_base.h"
#include "cvi_comm_isp.h"
#include "cvi_comm_3a.h"
#include "cvi_ae_comm.h"
#include "cvi_isp.h"

#include "sample_ae.h"
#include "sample_ae_algo.h"

#include "3A_internal.h"

/**************************************************************************
 *							C O N S T A N T S
 **************************************************************************/
#define _SHOW_AE_INFO_	(0)
#define CHECK_AE_TIMING (0)


/**************************************************************************
 *								M A C R O S
 **************************************************************************/
#define AE_LIMIT(var, min, max) ((var) = ((var) < (min)) ? (min) : (((var) > (max)) ? (max) : (var)))

/**************************************************************************
 *							D A T A   T Y P E S
 **************************************************************************/
/**************************************************************************
 *					E X T E R N A L   R E F E R E N C E
 **************************************************************************/
/**************************************************************************
 *				F U N C T I O N   D E C L A R A T I O N S
 **************************************************************************/
/**************************************************************************
 *						  G L O B A L	D A T A
 **************************************************************************/
SAMPLE_AE_CTX_S g_astAeCtx_sample[MAX_AE_LIB_NUM] = {
	{
		0
	}
};

CVI_FLOAT sample_ae_fps = 25;
CVI_FLOAT fExpLineTime[AE_SENSOR_NUM];


SAMPLE_AE_ALGO_S sAeAlgoData;
SAMPLE_AE_CTRL_S sAeCtrl;


#if CHECK_AE_TIMING
static struct timeval tt_ae[20];
#define MEAUSURE_AE_T(a) gettimeofday(&tt_ae[a], NULL)
#else
#define MEAUSURE_AE_T(a)
#endif

CVI_U32 SAMPLE_AeGetUSTimeDiff(const struct timeval *before, const struct timeval *after)
{
	CVI_U32 CVI_timeDiff = 0;

	CVI_timeDiff = (after->tv_sec - before->tv_sec) * 1000000 + (after->tv_usec - before->tv_usec);

	return CVI_timeDiff;
}

/**
 * Func : [AddDescription]
 * Return:
 *     CVI_S32  : [AddDescription]
 * Param
 *     CVI_S32 s32Handle : [AddDescription]
 *     CVI_U32 *pu32time : [ExpTime: us]
 *     CVI_U32 *pagain : [0dB:1024 6dB:2048]
 *     CVI_U32 *pdgain : [0dB:1024 6dB:2048]
 *     CVI_BOOL isInit : [set once]
 */
CVI_S32 SAMPLE_AE_SetExp(CVI_S32 s32Handle, CVI_U32 *pu32time, CVI_U32 *pagain, CVI_U32 *pdgain, CVI_BOOL isInit)
{
	SAMPLE_AE_CTX_S *pstAeCtx = CVI_NULL;
	VI_PIPE ViPipe;
	CVI_U32 tmpLine[AE_MAX_WDR_FRAME_NUM];
	CVI_U32 timeLine[AE_MAX_WDR_FRAME_NUM], againDb[AE_MAX_WDR_FRAME_NUM], dgainDb[AE_MAX_WDR_FRAME_NUM];
	CVI_U8 i, isWDR;
	CVI_U16 manual = 1;
	CVI_U32 ratio[3] = { WDR_LE_SE_RATIO * AE_WDR_RATIO_BASE, AE_WDR_RATIO_BASE, AE_WDR_RATIO_BASE };
	CVI_U32 IntTimeMax[4], IntTimeMin[4], LFMaxIntTime[4];

	AE_CHECK_HANDLE_ID(s32Handle);
	pstAeCtx = AE_GET_CTX(s32Handle);
	ViPipe = pstAeCtx->IspBindDev;
	// update sensor exposure time etc.

	if (pstAeCtx->stAeParam.u8WDRMode)
		isWDR = 1;
	else
		isWDR = 0;

	for (i = 0; i < AE_MAX_WDR_FRAME_NUM; i++) {
		if (fExpLineTime[ViPipe]) {
			tmpLine[i] = pu32time[i]/fExpLineTime[ViPipe];
		}
		if (pstAeCtx->stSnsRegister.stAeExp.pfn_cmos_again_calc_table)
			pstAeCtx->stSnsRegister.stAeExp.pfn_cmos_again_calc_table(ViPipe, &pagain[i], &againDb[i]);
		else {
			printf("%s again calc table NULL.\n", __func__);
			return CVI_FAILURE;
		}
		if (pstAeCtx->stSnsRegister.stAeExp.pfn_cmos_dgain_calc_table) {
			pstAeCtx->stSnsRegister.stAeExp.pfn_cmos_dgain_calc_table(ViPipe, &pdgain[i], &dgainDb[i]);
		} else {
			printf("%s dgain calc table NULL.\n", __func__);
			return CVI_FAILURE;
		}
	}

	if (isWDR) {
		timeLine[0] = tmpLine[AE_SE];
		timeLine[1] = tmpLine[AE_LE];
		ratio[0] = AE_WDR_RATIO_BASE * WDR_LE_SE_RATIO;

		if (pstAeCtx->stSnsRegister.stAeExp.pfn_cmos_get_inttime_max) {
			pstAeCtx->stSnsRegister.stAeExp.pfn_cmos_get_inttime_max(ViPipe, manual, ratio, IntTimeMax,
										 IntTimeMin, LFMaxIntTime);
		} else {
			printf("%s inttime max NULL.\n", __func__);
			return CVI_FAILURE;
		}
		//0: WDR SE; 1: WDR LE
		AE_LIMIT(timeLine[0], IntTimeMin[0], IntTimeMax[0]);
		AE_LIMIT(timeLine[1], IntTimeMin[1], IntTimeMax[1]);
		//printf("SE %d %d\n",IntTimeMin[0], IntTimeMax[0]);
		//printf("LE %d %d\n",IntTimeMin[1], IntTimeMax[1]);
	} else {
		timeLine[0] = tmpLine[AE_LE];
	}

	if (pstAeCtx->stSnsRegister.stAeExp.pfn_cmos_inttime_update) {
		//printf("T:%d %d\n",timeLine[0],timeLine[1]);
		pstAeCtx->stSnsRegister.stAeExp.pfn_cmos_inttime_update(ViPipe, timeLine);
	} else {
		printf("%s inttime update NULL.\n", __func__);
		return CVI_FAILURE;
	}
	/* maybe need to call the sensor's register functions to config sensor */
	// update sensor gains

	if (pstAeCtx->stSnsRegister.stAeExp.pfn_cmos_gains_update) {
		pstAeCtx->stSnsRegister.stAeExp.pfn_cmos_gains_update(ViPipe, againDb, dgainDb);
	} else {
		printf("%s cmos gains update NULL.\n", __func__);
		return CVI_FAILURE;
	}
	if (isInit == 0)
		CVI_ISP_SyncSensorCfg(ViPipe);

	return CVI_SUCCESS;
}

CVI_S32 SAMPLE_AE_Calculate(CVI_S32 s32Handle)
{
	SAMPLE_AE_CTX_S *pstAeCtx = CVI_NULL;
	VI_PIPE ViPipe;

	CVI_U32 exptime[2] = { 0 }, again[2] = { 0 }, dgain[2] = { 0 };
	ISP_AE_INFO_S *pstAeInfo;

	CVI_U8 u8FrameNum;

	AE_CHECK_HANDLE_ID(s32Handle);
	pstAeCtx = AE_GET_CTX(s32Handle);
	ViPipe = pstAeCtx->IspBindDev;

	UNUSED(ViPipe);
	UNUSED(u8FrameNum);
	pstAeInfo =	&pstAeCtx->stAeInfo;

	u8FrameNum = (pstAeCtx->bWDRMode) ? 2 : 1;

	//max Value is 1023
	/* user's AE alg implementor */
	sAeAlgoData.frmNum = u8FrameNum;
	sAeAlgoData.pstFEAeStat3 = pstAeInfo->pstFEAeStat3[0];
	sAeAlgoData.winXNum = pstAeCtx->stAeParam.aeLEWinConfig[0].winXNum;
	sAeAlgoData.winYNum = pstAeCtx->stAeParam.aeLEWinConfig[0].winYNum;

	MEAUSURE_AE_T(1);
	SAMPLE_AE_Algo(&sAeAlgoData, &sAeCtrl);
	MEAUSURE_AE_T(2);
	exptime[AE_LE] = sAeCtrl.leExp;//Long Exp
	again[AE_LE] = sAeCtrl.leGain;
	dgain[AE_LE] = AE_GAIN_BASE;

	exptime[AE_SE] = sAeCtrl.seExp;
	again[AE_SE] = sAeCtrl.seGain;
	dgain[AE_SE] = AE_GAIN_BASE;

	#if _SHOW_AE_INFO_
		CVI_U16 winXNum, winYNum;

		winXNum = pstAeCtx->stAeParam.aeLEWinConfig[0].winXNum;
		winYNum = pstAeCtx->stAeParam.aeLEWinConfig[0].winYNum;
		printf("Frm: %d	WinX:%d %d\n", u8FrameNum, winXNum, winYNum);
		printf("	Exp:%4d A:%4d\n", exptime[AE_LE], again[AE_LE]);
		#if 0
		printf("	R_LU:%d RU:%d\n", pstAeInfo->pstFEAeStat3[0]->au16ZoneAvg[0][0][0][3],
			pstAeInfo->pstFEAeStat3[0]->au16ZoneAvg[0][0][winXNum-1][3]);
		printf("	R_LD:%d RD:%d\n", pstAeInfo->pstFEAeStat3[0]->au16ZoneAvg[0][winYNum-1][0][3],
			pstAeInfo->pstFEAeStat3[0]->au16ZoneAvg[0][winYNum-1][winXNum-1][3]);
		printf("	G_LU:%d RU:%d\n", pstAeInfo->pstFEAeStat3[0]->au16ZoneAvg[0][0][0][1],
			pstAeInfo->pstFEAeStat3[0]->au16ZoneAvg[0][0][winXNum-1][1]);
		printf("	G_LD:%d RD:%d\n", pstAeInfo->pstFEAeStat3[0]->au16ZoneAvg[0][winYNum-1][0][1],
			pstAeInfo->pstFEAeStat3[0]->au16ZoneAvg[0][winYNum-1][winXNum-1][1]);
		printf("	B_LU:%d RU:%d\n", pstAeInfo->pstFEAeStat3[0]->au16ZoneAvg[0][0][0][0],
			pstAeInfo->pstFEAeStat3[0]->au16ZoneAvg[0][0][winXNum-1][0]);
		printf("	B_LD:%d RD:%d\n", pstAeInfo->pstFEAeStat3[0]->au16ZoneAvg[0][winYNum-1][0][0],
			pstAeInfo->pstFEAeStat3[0]->au16ZoneAvg[0][winYNum-1][winXNum-1][0]);
		#endif
		printf("\n");
	#endif

	SAMPLE_AE_SetExp(s32Handle, exptime, again, dgain, 0);
	return CVI_SUCCESS;
}

CVI_S32 SAMPLE_AE_Init(CVI_S32 s32Handle, const ISP_AE_PARAM_S *pstAeParam)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	VI_PIPE ViPipe;
	SAMPLE_AE_CTX_S *pstAeCtx = CVI_NULL;
	CVI_U32 exptime[2] = { 0 }, again[2] = { 0 },
		dgain[2] = { 0 };

	CVI_U16 manual = 1;
	CVI_U32 ratio[3] = { WDR_LE_SE_RATIO * AE_WDR_RATIO_BASE, AE_WDR_RATIO_BASE, AE_WDR_RATIO_BASE }; //max 256x
	CVI_U32 IntTimeMax[4], IntTimeMin[4], LFMaxIntTime[4];

	printf("Func %s line %d\n", __func__, __LINE__);

	AE_CHECK_HANDLE_ID(s32Handle);
	pstAeCtx = AE_GET_CTX(s32Handle);
	ViPipe = pstAeCtx->IspBindDev;

	AE_CHECK_POINTER(pstAeParam);
	UNUSED(ViPipe);

	//Get some info from pstAeParam & copy to pstAeCtx->stAeParam
	printf("sID:%d Wdr:%d Hdr:%d\n", ViPipe, pstAeParam->u8WDRMode, pstAeParam->u8HDRMode);
	printf("fps %f\n", pstAeParam->f32Fps);
	printf("Wnum:%d Hnum:%d\n", pstAeParam->aeLEWinConfig[0].winXNum, pstAeParam->aeLEWinConfig[0].winYNum);

	memcpy(&pstAeCtx->stAeParam, pstAeParam, sizeof(ISP_AE_PARAM_S));

	if (pstAeCtx->stSnsRegister.stAeExp.pfn_cmos_get_ae_default)
		pstAeCtx->stSnsRegister.stAeExp.pfn_cmos_get_ae_default(ViPipe, &pstAeCtx->stSnsDft);
	else {
		printf("%s\n", "get ae default NULL.");
		return CVI_FAILURE;
	}
	printf("pstAeCtx->stSnsDft.f32Fps:%f\n", pstAeCtx->stSnsDft.f32Fps);
	pstAeCtx->bWDRMode = (pstAeParam->u8WDRMode == WDR_MODE_NONE) ? 0 : 1;
	printf("pstAeCtx->bWDRMode %d\n", pstAeCtx->bWDRMode);

	//set Sensor fps
	if (pstAeCtx->stSnsRegister.stAeExp.pfn_cmos_fps_set)
		pstAeCtx->stSnsRegister.stAeExp.pfn_cmos_fps_set(ViPipe, sample_ae_fps, &pstAeCtx->stSnsDft);
	else {
		printf("pfn_cmos_fps_set NULL!\n");
		return CVI_FAILURE;
	}

	if (pstAeCtx->bWDRMode) {
		if (pstAeCtx->stSnsRegister.stAeExp.pfn_cmos_get_inttime_max) {
			pstAeCtx->stSnsRegister.stAeExp.pfn_cmos_get_inttime_max(ViPipe, manual, ratio, IntTimeMax,
											 IntTimeMin, LFMaxIntTime);
		} else {
			printf("get inttime max NULL.");
			return CVI_FAILURE;
		}
		//SE Time range
		printf("SE Time range:%d %d\n", IntTimeMin[0], IntTimeMax[0]);
		printf("LE Time range:%d %d\n", IntTimeMin[1], IntTimeMax[1]);
	}

	if (pstAeCtx->stSnsRegister.stAeExp.pfn_cmos_get_ae_default)
		pstAeCtx->stSnsRegister.stAeExp.pfn_cmos_get_ae_default(ViPipe, &pstAeCtx->stSnsDft);
	else {
		printf("%s\n", "get ae default NULL.");
		return CVI_FAILURE;
	}

	//Calculate how many uSec per  exp_line
	fExpLineTime[ViPipe] = 1000000 / (CVI_FLOAT)(pstAeCtx->stSnsDft.u32FullLinesStd * sample_ae_fps);

	printf("Min/Max Line %d %d\n", pstAeCtx->stSnsDft.u32MinIntTime, pstAeCtx->stSnsDft.u32MaxIntTime);
	printf("Full Line %d fps:%f\n", pstAeCtx->stSnsDft.u32FullLinesStd, sample_ae_fps);
	printf("LineTimeUs:%f\n", fExpLineTime[ViPipe]);
	printf("Min/Max Again %d %d\n", pstAeCtx->stSnsDft.u32MinAgain, pstAeCtx->stSnsDft.u32MaxAgain);
	printf("Min/Max Dgain %d %d\n", pstAeCtx->stSnsDft.u32MinDgain, pstAeCtx->stSnsDft.u32MaxDgain);

	//Set init exp_time 33.33ms and gain to 0 dB
	exptime[AE_LE] = 33333;//us
	exptime[AE_SE] = 33333;
	again[AE_LE] = AE_GAIN_BASE;
	again[AE_SE] = AE_GAIN_BASE;
	dgain[AE_LE] = AE_GAIN_BASE;
	dgain[AE_SE] = AE_GAIN_BASE;
	if (pstAeCtx->bWDRMode) {
		exptime[AE_SE] = exptime[AE_LE] / WDR_LE_SE_RATIO;
	}

	SAMPLE_AE_SetExp(s32Handle, exptime, again, dgain, 1);

	if (s32Ret != CVI_SUCCESS) {
		printf("Ae lib(%d) vreg init failed!\n", s32Handle);
		return s32Ret;
	}
	/* do something ... like check the sensor id, init the global variables, and so on */
	//init your Ae Algo parameter...

	printf("Func %s is OK\n", __func__);
	return CVI_SUCCESS;
}

//Cvitek add more info at ISP_AE_INFO_S
// aeLEWinConfig[AE_MAX_NUM];	//LongExposure Win Info
// aeSEWinConfig;				//ShortExposure Win Info
//Cvitek add more info at ISP_AE_RESULT_S
// fWDRIndex;
// u32ExpRatio;			//long/short exp ratio
// s16CurrentLV;		//current LightValue x100
// u32AvgLuma;			//current Frame Luma (0~255)
// u8MeterFramePeriod;	//do AE calcute period
// fSEExpGainUpRatio;	//short Exp Gain Ration
// bStable;				//is AE stable
// fBvStep;				//currnet AE delta step,20=>6dB
CVI_S32 SAMPLE_AE_Run(CVI_S32 s32Handle, const ISP_AE_INFO_S *pstAeInfo,
	ISP_AE_RESULT_S *pstAeResult, CVI_S32 s32Rsv)
{
	MEAUSURE_AE_T(0);

	SAMPLE_AE_CTX_S *pstAeCtx = CVI_NULL;
	VI_PIPE ViPipe;

	AE_CHECK_HANDLE_ID(s32Handle);
	pstAeCtx = AE_GET_CTX(s32Handle);
	ViPipe = pstAeCtx->IspBindDev;

	UNUSED(s32Rsv);
	UNUSED(ViPipe);
	AE_CHECK_POINTER(pstAeInfo);
	AE_CHECK_POINTER(pstAeResult);

	AE_CHECK_POINTER(pstAeInfo->pstFEAeStat1);
	AE_CHECK_POINTER(pstAeInfo->pstFEAeStat2);
	AE_CHECK_POINTER(pstAeInfo->pstFEAeStat3);

	pstAeCtx->u32FrameCnt = pstAeInfo->u32FrameCnt;

	/* do AE alg per 4 frame */
	if (pstAeCtx->u32FrameCnt % 4 == 0) {
		/* record the statistics in pstAeInfo, and then use the statistics...
		 * ...to calculate, no need to call any other api
		 */
		memcpy(&pstAeCtx->stAeInfo, pstAeInfo, sizeof(ISP_AE_INFO_S));

		SAMPLE_AE_Calculate(s32Handle);

		pstAeCtx->stAeResult.u32IspDgain = 1024;
		pstAeCtx->stAeResult.u32Again = 1024;
		pstAeCtx->stAeResult.u32Dgain = 1024;
		pstAeCtx->stAeResult.u32Iso = 100;
		pstAeCtx->stAeResult.bStable = 1;
		pstAeCtx->stAeResult.fBvStep = 0;
		pstAeCtx->stAeResult.s16CurrentLV = 800;
		pstAeCtx->stAeResult.u32AvgLuma = 50;
		pstAeCtx->stAeResult.u32ExpRatio = WDR_LE_SE_RATIO*64;//Important, LE/SE *64
		pstAeCtx->stAeResult.u8MeterFramePeriod = 4;
	}

	/* record result */
	memcpy(pstAeResult, &pstAeCtx->stAeResult, sizeof(ISP_AE_RESULT_S));

	MEAUSURE_AE_T(3);
	#if CHECK_AE_TIMING
	printf("AET1:%d\n", SAMPLE_AeGetUSTimeDiff(&tt_ae[1], &tt_ae[2]));
	printf("AET2:%d\n", SAMPLE_AeGetUSTimeDiff(&tt_ae[0], &tt_ae[3]));
	#endif
	return CVI_SUCCESS;
}

CVI_S32 SAMPLE_AE_Ctrl(CVI_S32 s32Handle, CVI_U32 u32Cmd, CVI_VOID *pValue)
{
	//SAMPLE_AE_CTX_S *pstAeCtx = CVI_NULL;
	// avoid warning
	//pstAeCtx = pstAeCtx;
	AE_CHECK_HANDLE_ID(s32Handle);

	// pstAeCtx = AE_GET_CTX(s32Handle);
	AE_CHECK_POINTER(pValue);

	switch (u32Cmd)	{
		/* system ctrl */
	case ISP_WDR_MODE_SET:

			/* do something ... */
			break;

		/* ae ctrl, define the customer's ctrl cmd, if needed ... */
	default:
			break;
	}

	return CVI_SUCCESS;
}

CVI_S32 SAMPLE_AE_Exit(CVI_S32 s32Handle)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	VI_PIPE ViPipe;
	SAMPLE_AE_CTX_S *pstAeCtx = CVI_NULL;

	printf("Func %s line %d\n", __func__, __LINE__);
	AE_CHECK_HANDLE_ID(s32Handle);
	pstAeCtx = AE_GET_CTX(s32Handle);
	ViPipe = pstAeCtx->IspBindDev;

	// avoid warning
	UNUSED(ViPipe);

	//pstAeCtx = pstAeCtx;

	/* do something ... */

	if (s32Ret != CVI_SUCCESS) {
		printf("Ae lib(%d) vreg exit failed!\n", s32Handle);
		return s32Ret;
	}

	return CVI_SUCCESS;
}

CVI_S32 CVI_AE_Register(VI_PIPE ViPipe, ALG_LIB_S *pstAeLib)
{
	ISP_AE_REGISTER_S stRegister;
	CVI_S32 s32Ret = CVI_SUCCESS;
	CVI_S32 s32Handle;
	SAMPLE_AE_CTX_S *pstAeCtx = CVI_NULL;

	printf("Func %s line %d\n", __func__, __LINE__);

	AE_CHECK_DEV(ViPipe);
	AE_CHECK_POINTER(pstAeLib);
	AE_CHECK_HANDLE_ID(pstAeLib->s32Id);
	AE_CHECK_LIB_NAME(pstAeLib->acLibName);

	s32Handle = pstAeLib->s32Id;
	pstAeCtx = AE_GET_CTX(s32Handle);
	pstAeCtx->IspBindDev = ViPipe;

	stRegister.stAeExpFunc.pfn_ae_init = SAMPLE_AE_Init;
	stRegister.stAeExpFunc.pfn_ae_run = SAMPLE_AE_Run;
	stRegister.stAeExpFunc.pfn_ae_ctrl = SAMPLE_AE_Ctrl;
	stRegister.stAeExpFunc.pfn_ae_exit = SAMPLE_AE_Exit;
	s32Ret = CVI_ISP_AELibRegCallBack(ViPipe, pstAeLib, &stRegister);

	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_ae register failed!\n");
	}

	return s32Ret;
}

CVI_S32 CVI_AE_UnRegister(VI_PIPE ViPipe, ALG_LIB_S *pstAeLib)
{
	CVI_S32 s32Ret = CVI_SUCCESS;

	printf("Func %s line %d\n", __func__, __LINE__);

	AE_CHECK_DEV(ViPipe);
	AE_CHECK_POINTER(pstAeLib);
	AE_CHECK_HANDLE_ID(pstAeLib->s32Id);
	AE_CHECK_LIB_NAME(pstAeLib->acLibName);

	s32Ret = CVI_ISP_AELibUnRegCallBack(ViPipe, pstAeLib);

	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_ae unregister failed!\n");
	}

	return s32Ret;
}

CVI_S32 CVI_AE_SensorRegCallBack(VI_PIPE ViPipe, ALG_LIB_S *pstAeLib, ISP_SNS_ATTR_INFO_S *pstSnsAttrInfo,
				     AE_SENSOR_REGISTER_S *pstRegister)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		//ISP_DEBUG(LOG_ERR, "ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstAeLib == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (pstSnsAttrInfo == CVI_NULL) {
		return CVI_FAILURE;
	}


	SAMPLE_AE_CTX_S *pstAeCtx;
	CVI_S32 s32Ret = 0;

	if (pstRegister == CVI_NULL) {
		return CVI_FAILURE;
	}

	pstAeCtx = AE_GET_CTX(ViPipe);
	memcpy((CVI_VOID *)(&(pstAeCtx->stSnsAttrInfo)), (CVI_VOID *)pstSnsAttrInfo, sizeof(ISP_SNS_ATTR_INFO_S));
	/*TODO. BindDev Seems not set to ViPipe.*/
	pstAeCtx->IspBindDev = ViPipe;
	if (pstRegister == CVI_NULL) {
		printf("AE Register callback is NULL\n");
		return CVI_FAILURE;
	}
	memcpy(&(pstAeCtx->stSnsRegister), pstRegister, sizeof(AE_SENSOR_REGISTER_S));

	if (pstAeCtx->stSnsRegister.stAeExp.pfn_cmos_get_ae_default != CVI_NULL)
		pstAeCtx->stSnsRegister.stAeExp.pfn_cmos_get_ae_default(ViPipe, &pstAeCtx->stSnsDft);
	pstAeCtx->bSnsRegister = 1;
	return s32Ret;
}

CVI_S32 CVI_AE_SensorUnRegCallBack(VI_PIPE ViPipe, ALG_LIB_S *pstAeLib, SENSOR_ID SensorId)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		//ISP_DEBUG(LOG_ERR, "ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstAeLib == CVI_NULL) {
		return CVI_FAILURE;
	}


	SAMPLE_AE_CTX_S *pstAeCtx;

	UNUSED(SensorId);
	pstAeCtx = AE_GET_CTX(ViPipe);
	memset(&(pstAeCtx->stSnsRegister), 0, sizeof(AE_SENSOR_REGISTER_S));
	memset(&(pstAeCtx->stSnsDft), 0, sizeof(AE_SENSOR_DEFAULT_S));
	pstAeCtx->bSnsRegister = 0;

	return CVI_SUCCESS;
}


