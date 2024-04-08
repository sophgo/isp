/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: sample_ae_dummy.c
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
#include "cvi_awb_comm.h"

#include "cvi_isp.h"

#include "sample_ae.h"

/**************************************************************************
 *							C O N S T A N T S
 **************************************************************************/
/**************************************************************************
 *								M A C R O S
 **************************************************************************/
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

ISP_WDR_EXPOSURE_ATTR_S stAeMpiWDRExposureAttr[AE_SENSOR_NUM], stAeWDRExposureAttrInfo[AE_SENSOR_NUM];

void CVI_AE_AutoTest(VI_PIPE ViPipe)
{
	UNUSED(ViPipe);
}
CVI_S32 CVI_ISP_GetExposureAttr(VI_PIPE ViPipe, ISP_EXPOSURE_ATTR_S *pstExpAttr)
{
	UNUSED(ViPipe);
	UNUSED(pstExpAttr);
	return CVI_SUCCESS;
}
CVI_S32 CVI_ISP_SetExposureAttr(VI_PIPE ViPipe, const ISP_EXPOSURE_ATTR_S *pstExpAttr)
{
	UNUSED(ViPipe);
	UNUSED(pstExpAttr);
	return CVI_SUCCESS;
}
CVI_S32 CVI_ISP_SetWDRExposureAttr(VI_PIPE ViPipe, const ISP_WDR_EXPOSURE_ATTR_S *pstWDRExpAttr)
{
	UNUSED(ViPipe);
	UNUSED(pstWDRExpAttr);

	stAeMpiWDRExposureAttr[ViPipe] = *pstWDRExpAttr;
	return CVI_SUCCESS;
}
CVI_S32 CVI_ISP_GetWDRExposureAttr(VI_PIPE ViPipe, ISP_WDR_EXPOSURE_ATTR_S *pstWDRExpAttr)
{
	UNUSED(ViPipe);
	UNUSED(pstWDRExpAttr);

	*pstWDRExpAttr = stAeMpiWDRExposureAttr[ViPipe];
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_SetWDRLEOnly(CVI_U8 ViPipe, CVI_BOOL wdrLEOnly)
{
	UNUSED(ViPipe);
	UNUSED(wdrLEOnly);
	return CVI_SUCCESS;
}
CVI_S32 CVI_ISP_SetAERouteAttr(VI_PIPE ViPipe, const ISP_AE_ROUTE_S *pstAERouteAttr)
{
	UNUSED(ViPipe);
	UNUSED(pstAERouteAttr);
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_GetAERouteAttr(VI_PIPE ViPipe, ISP_AE_ROUTE_S *pstAERouteAttr)
{
	UNUSED(ViPipe);
	UNUSED(pstAERouteAttr);
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_SetAERouteAttrEx(VI_PIPE ViPipe, const ISP_AE_ROUTE_EX_S *pstAERouteAttr)
{
	UNUSED(ViPipe);
	UNUSED(pstAERouteAttr);
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_GetAERouteAttrEx(VI_PIPE ViPipe, ISP_AE_ROUTE_EX_S *pstAERouteAttr)
{
	UNUSED(ViPipe);
	UNUSED(pstAERouteAttr);
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_QueryExposureInfo(VI_PIPE ViPipe, ISP_EXP_INFO_S *pstExpInfo)
{
	UNUSED(ViPipe);
	UNUSED(pstExpInfo);
	return CVI_SUCCESS;
}
CVI_S32 CVI_ISP_SetAEStatisticsConfig(VI_PIPE ViPipe, const ISP_AE_STATISTICS_CFG_S *pstAeCfg)
{
	UNUSED(ViPipe);
	UNUSED(pstAeCfg);
	return CVI_SUCCESS;
}
CVI_S32 CVI_ISP_GetAEStatisticsConfig(VI_PIPE ViPipe, ISP_AE_STATISTICS_CFG_S *pstAeCfg)
{
	UNUSED(ViPipe);
	UNUSED(pstAeCfg);
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_GetAESnapLogBuf(VI_PIPE ViPipe, CVI_U8 *pBuf, CVI_U32 bufSize)
{
	UNUSED(ViPipe);
	UNUSED(pBuf);
	UNUSED(bufSize);
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_AEBracketingStart(VI_PIPE ViPipe)
{
	UNUSED(ViPipe);
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_AEBracketingSetExpsoure(VI_PIPE ViPipe, CVI_S16 leEvX10, CVI_S16 seEvX10)
{
	UNUSED(ViPipe);
	UNUSED(leEvX10);
	UNUSED(seEvX10);
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_AEBracketingFinish(VI_PIPE ViPipe)
{
	UNUSED(ViPipe);
	return CVI_SUCCESS;
}
CVI_S32 CVI_ISP_GetAELogBuf(VI_PIPE ViPipe, CVI_U8 *pBuf, CVI_U32 bufSize)
{
	UNUSED(ViPipe);
	UNUSED(pBuf);
	UNUSED(bufSize);

	return CVI_SUCCESS;
}


void AE_Debug(void)
{
}

void CVI_AE_GenNewRaw(void *pDstOri, void *pSrcOri, CVI_U32 sizeBk, CVI_U32 mode, CVI_U32 w, CVI_U32 h, CVI_U32 blc)
{
	UNUSED(pDstOri);
	UNUSED(pSrcOri);
	UNUSED(sizeBk);
	UNUSED(mode);
	UNUSED(w);
	UNUSED(h);
	UNUSED(blc);
}
void CVI_AE_SetAeSimMode(CVI_BOOL bMode)
{
	UNUSED(bMode);
}
CVI_BOOL CVI_AE_IsAeSimMode(void)
{
	return 0;
}

CVI_S32 CVI_ISP_GetSmartExposureAttr(VI_PIPE ViPipe, ISP_SMART_EXPOSURE_ATTR_S *pstSmartExpAttr)
{
	UNUSED(ViPipe);
	UNUSED(pstSmartExpAttr);
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_SetSmartExposureAttr(VI_PIPE ViPipe, const ISP_SMART_EXPOSURE_ATTR_S *pstSmartExpAttr)
{
	UNUSED(ViPipe);
	UNUSED(pstSmartExpAttr);
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_AEBracketingSetSimple(CVI_BOOL bEnable)
{
	UNUSED(bEnable);
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_GetAELogBufSize(VI_PIPE ViPipe, CVI_U32 *bufSize)
{
	UNUSED(ViPipe);
	UNUSED(bufSize);
	return 0;
}
CVI_S32 CVI_ISP_GetAEBinBufSize(VI_PIPE ViPipe, CVI_U32 *bufSize)
{
	UNUSED(ViPipe);
	UNUSED(bufSize);
	return 0;
}
CVI_S32 CVI_ISP_GetAEBinBuf(VI_PIPE ViPipe, CVI_U8 *pBuf, CVI_U32 bufSize)
{
	UNUSED(ViPipe);
	UNUSED(pBuf);
	UNUSED(bufSize);
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_SetIrisAttr(VI_PIPE ViPipe, const ISP_IRIS_ATTR_S *pstIrisAttr)
{
	UNUSED(ViPipe);
	UNUSED(pstIrisAttr);

	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_GetIrisAttr(VI_PIPE ViPipe, ISP_IRIS_ATTR_S *pstIrisAttr)
{
	UNUSED(ViPipe);
	UNUSED(pstIrisAttr);

	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_SetDcirisAttr(VI_PIPE ViPipe, const ISP_DCIRIS_ATTR_S *pstDcirisAttr)
{
	UNUSED(ViPipe);
	UNUSED(pstDcirisAttr);

	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_GetDcirisAttr(VI_PIPE ViPipe, ISP_DCIRIS_ATTR_S *pstDcirisAttr)
{
	UNUSED(ViPipe);
	UNUSED(pstDcirisAttr);

	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_QueryFps(VI_PIPE ViPipe, CVI_FLOAT *pFps)
{
	UNUSED(ViPipe);
	UNUSED(pFps);
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_AESetRawReplayExposure(VI_PIPE ViPipe, const ISP_EXP_INFO_S *pstExpInfo)
{
	UNUSED(ViPipe);
	UNUSED(pstExpInfo);
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_GetAERawReplayFrmNum(VI_PIPE ViPipe, CVI_U8 *bootfrmNum, CVI_U8 *ispDgainPeriodNum)
{
	UNUSED(ViPipe);
	UNUSED(bootfrmNum);
	UNUSED(ispDgainPeriodNum);
	return CVI_SUCCESS;
}

void CVI_ISP_AESetRawReplayMode(VI_PIPE ViPipe, CVI_BOOL bMode)
{
	UNUSED(ViPipe);
	UNUSED(bMode);
}

CVI_S32 CVI_ISP_GetAERouteSFAttr(VI_PIPE ViPipe, ISP_AE_ROUTE_S *pstAERouteSFAttr)
{
	UNUSED(ViPipe);
	UNUSED(pstAERouteSFAttr);
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_GetAERouteSFAttrEx(VI_PIPE ViPipe, ISP_AE_ROUTE_EX_S *pstAERouteSFAttrEx)
{
	UNUSED(ViPipe);
	UNUSED(pstAERouteSFAttrEx);
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_SetAERouteSFAttr(VI_PIPE ViPipe, const ISP_AE_ROUTE_S *pstAERouteSFAttr)
{
	UNUSED(ViPipe);
	UNUSED(pstAERouteSFAttr);
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_SetAERouteSFAttrEx(VI_PIPE ViPipe, const ISP_AE_ROUTE_EX_S *pstAERouteSFAttrEx)
{
	UNUSED(ViPipe);
	UNUSED(pstAERouteSFAttrEx);
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_AEGetRawReplayExpBuf(VI_PIPE ViPipe, CVI_U8 *buf, CVI_U32 *bufSize)
{
	UNUSED(ViPipe);
	UNUSED(buf);
	UNUSED(bufSize);
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_AESetRawDumpFrameID(VI_PIPE ViPipe, CVI_U32 fid, CVI_U16 frmNum)
{
	UNUSED(ViPipe);
	UNUSED(fid);
	UNUSED(frmNum);
	return CVI_SUCCESS;
}


