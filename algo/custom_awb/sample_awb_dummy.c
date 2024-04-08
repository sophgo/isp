/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: sample_awb_dummy.c
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
#include "3A_internal.h"

/**************************************************************************
 *                          C O N S T A N T S
 **************************************************************************/
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

static ISP_WB_ATTR_S stAwbMpiAttr[AWB_SENSOR_NUM];

CVI_U8 AWB_ViPipe2sID(VI_PIPE ViPipe)
{
	CVI_U8 sID = ViPipe;

	return (sID < AWB_SENSOR_NUM) ? sID : AWB_SENSOR_NUM - 1;
}

void CVI_AWB_AutoTest(VI_PIPE ViPipe)
{
	UNUSED(ViPipe);
}

CVI_S32 CVI_ISP_GetWBAttr(VI_PIPE ViPipe, ISP_WB_ATTR_S *pstWBAttr)
{//isp_sts_ctrl_awb_eof will call it....
	CVI_U8 sID = AWB_ViPipe2sID(ViPipe);

	stAwbMpiAttr[sID].bByPass = CVI_FALSE;
	stAwbMpiAttr[sID].stAuto.bEnable = CVI_TRUE;
	*pstWBAttr = stAwbMpiAttr[sID];
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_SetWBAttr(VI_PIPE ViPipe, const ISP_WB_ATTR_S *pstWBAttr)
{
	UNUSED(ViPipe);
	UNUSED(pstWBAttr);
	return CVI_SUCCESS;
}
CVI_S32 CVI_ISP_GetAWBAttrEx(VI_PIPE ViPipe, ISP_AWB_ATTR_EX_S *pstAWBAttrEx)
{
	UNUSED(ViPipe);
	UNUSED(pstAWBAttrEx);
	return CVI_SUCCESS;
}
CVI_S32 CVI_ISP_SetAWBAttrEx(VI_PIPE ViPipe, const ISP_AWB_ATTR_EX_S *pstAWBAttrEx)
{
	UNUSED(ViPipe);
	UNUSED(pstAWBAttrEx);
	return CVI_SUCCESS;
}


CVI_S32 CVI_AWB_QueryInfo(VI_PIPE ViPipe, ISP_WB_Q_INFO_S *pstWB_Q_Info)
{
	UNUSED(ViPipe);
	UNUSED(pstWB_Q_Info);
	return CVI_SUCCESS;
}
CVI_S32 CVI_ISP_GetWBCalibration(VI_PIPE ViPipe, ISP_AWB_Calibration_Gain_S *pstWBCalib)
{
	UNUSED(ViPipe);
	UNUSED(pstWBCalib);
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_SetWBCalibration(VI_PIPE ViPipe, const ISP_AWB_Calibration_Gain_S *pstWBCalib)
{
	UNUSED(ViPipe);
	UNUSED(pstWBCalib);
	return CVI_SUCCESS;
}
CVI_S32 CVI_ISP_SetWBCalibrationEx(VI_PIPE ViPipe, const ISP_AWB_Calibration_Gain_S_EX *pstWBCalib)
{
	UNUSED(ViPipe);
	UNUSED(pstWBCalib);
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_GetWBCalibrationEx(VI_PIPE ViPipe, ISP_AWB_Calibration_Gain_S_EX *pstWBCalib)
{
	UNUSED(ViPipe);
	UNUSED(pstWBCalib);
	return CVI_SUCCESS;
}


CVI_S32 CVI_ISP_GetAWBDbgBinBuf(VI_PIPE ViPipe, CVI_U8 *buf, CVI_U32 bufSize)
{
	UNUSED(ViPipe);
	UNUSED(buf);
	UNUSED(bufSize);
	return CVI_SUCCESS;
}
CVI_S32 CVI_ISP_GetAWBSnapLogBuf(VI_PIPE ViPipe, CVI_U8 *buf, CVI_U32 bufSize)
{
	UNUSED(ViPipe);
	UNUSED(buf);
	UNUSED(bufSize);
	return CVI_SUCCESS;
}
CVI_S32 CVI_ISP_GetAWBDbgBinSize(void)
{
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_SetAWBLogPath(const char *szPath)
{
	UNUSED(szPath);
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_SetAWBLogName(const char *szName)
{
	UNUSED(szName);
	return CVI_SUCCESS;
}


void AWB_Debug(void)
{
}

void CVI_ISP_SetAwbSimMode(CVI_BOOL bMode)
{
	UNUSED(bMode);
}
CVI_BOOL CVI_ISP_GetAwbSimMode(void)
{
	return 0;
}

CVI_S32 CVI_ISP_GetGrayWorldAwbInfo(VI_PIPE ViPipe, CVI_U16 *pRgain, CVI_U16 *pBgain)
{
	UNUSED(ViPipe);
	UNUSED(pRgain);
	UNUSED(pBgain);
	return CVI_SUCCESS;
}

