/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: ae_debug.c
 * Description:
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <math.h>

#include "cvi_comm_3a.h"
#include "cvi_comm_isp.h"
#include "aealgo.h"
#include "ae_project_param.h"
#include "cvi_ae.h"
#include "ae_debug.h"
#include "cvi_isp.h"

#include "cvi_awb.h"
#include "isp_main.h"
#ifndef ARCH_RTOS_CV181X // TODO@CV181X
#include <sys/time.h> //for gettimeofday()
#include <sys/types.h>
#include <sys/stat.h>
#endif

#ifndef ARCH_RTOS_CV181X // TODO@CV181X
void AE_HLprintf(const char *szFmt, ...)
{
#define NONECOLOR "\033[m"
#define RED "\033[0;32;31m"
#define LIGHT_RED "\033[1;31m"

	va_list vaPtr;
	char szBuffer[256];

	va_start(vaPtr, (const char *)szFmt);
	vsprintf((char *)szBuffer, (const char *)szFmt, vaPtr);
	va_end(vaPtr);

	printf(RED "%s" NONECOLOR, szBuffer);
}
#else
void AE_HLprintf(const char *szFmt, ...)
{
	UNUSED(szFmt);
}
#endif

void AE_SetLSC(CVI_U8 sID, CVI_BOOL enableLSC)
{
	VI_PIPE ViPipe = AE_sID2ViPipe(sID);

#if defined(ARCH_CV182X)
	ISP_MESH_SHADING_ATTR_S	*plscAttr;

	plscAttr = AE_Malloc(sID, sizeof(ISP_MESH_SHADING_ATTR_S));
	if (plscAttr == CVI_NULL) {
		printf("malloc plscAttr err\n");
		return;
	}
	CVI_ISP_GetMeshShadingAttr(ViPipe, plscAttr);
	plscAttr->Enable = enableLSC;
	CVI_ISP_SetMeshShadingAttr(ViPipe, plscAttr);
	AE_Free(sID, plscAttr);
	plscAttr = CVI_NULL;
#elif defined(ARCH_CV183X)
	ISP_DRC_ATTR_S	*pDRCAttr;

	pDRCAttr = AE_Malloc(sID, sizeof(ISP_DRC_ATTR_S));
	if (pDRCAttr == CVI_NULL) {
		printf("malloc pDRCAttr err\n");
		return;
	}
	CVI_ISP_GetDRCAttr(ViPipe, pDRCAttr);
	pDRCAttr->DRCMu[ISP_TEST_PARAM_AE_LSCR_ENABLE] = enableLSC;
	pDRCAttr->DRCMu[ISP_TEST_PARAM_AWB_LSCR_ENABLE] = enableLSC;
	CVI_ISP_SetDRCAttr(ViPipe, pDRCAttr);
	AE_Free(sID, pDRCAttr);
	pDRCAttr = CVI_NULL;
#else
	UNUSED(ViPipe);
	return;
#endif

	if (enableLSC)
		printf("sID:%d LSC enalbe!\n", sID);
	else
		printf("sID:%d LSC disable!\n", sID);
}

void AE_GetLSC(CVI_U8 sID, CVI_BOOL *enableLSC)
{
	VI_PIPE ViPipe = AE_sID2ViPipe(sID);

	*enableLSC = CVI_FALSE;

#if defined(CV182X)
	ISP_MESH_SHADING_ATTR_S * plscAttr;

	plscAttr = AE_Malloc(sID, sizeof(ISP_MESH_SHADING_ATTR_S));
	if (plscAttr == CVI_NULL) {
		printf("malloc plscAttr err\n");
		return;
	}
	CVI_ISP_GetMeshShadingAttr(ViPipe, plscAttr);
	*enableLSC = plscAttr->Enable;
	AE_Free(sID, plscAttr);
	plscAttr = CVI_NULL;
#elif defined(CV183X)
	ISP_DRC_ATTR_S	*pDRCAttr = CVI_NULL;

	pDRCAttr = AE_Malloc(sID, sizeof(ISP_DRC_ATTR_S));
	if (pDRCAttr == CVI_NULL) {
		printf("malloc pDRCAttr err\n");
		return;
	}
	CVI_ISP_GetDRCAttr(ViPipe, pDRCAttr);
	*enableLSC  = pDRCAttr->DRCMu[ISP_TEST_PARAM_AE_LSCR_ENABLE];
	AE_Free(sID, pDRCAttr);
	pDRCAttr = CVI_NULL;
#else
	UNUSED(ViPipe);
	UNUSED(sID);
#endif //
}

void AE_SetFpsTest(CVI_U8 sID, CVI_U8 fps)
{
	ISP_PUB_ATTR_S pubAttr;
	VI_PIPE ViPipe;

	ViPipe = AE_sID2ViPipe(sID);
	CVI_ISP_GetPubAttr(ViPipe, &pubAttr);
	pubAttr.f32FrameRate = fps;
	printf("set fps:%d\n", fps);
	CVI_ISP_SetPubAttr(ViPipe, &pubAttr);
}

void AE_SetManual(CVI_U8 sID, CVI_BOOL mode, CVI_U8 debugMode)
{
	ISP_EXPOSURE_ATTR_S tmpAe;

	AE_GetExposureAttr(sID, &tmpAe);
	tmpAe.enOpType = mode;
	tmpAe.u8DebugMode = debugMode;
	if (tmpAe.enOpType == OP_TYPE_MANUAL)
		printf("sID:%d AE Manual!\n", sID);
	else
		printf("sID:%d AE Auto!\n", sID);

	AE_SetExposureAttr(sID, &tmpAe);
}

void AE_SetAntiFlicker(CVI_U8 sID, CVI_BOOL enable, ISP_AE_ANTIFLICKER_FREQUENCE_E HzMode)
{
	ISP_EXPOSURE_ATTR_S tmpAe;

	AE_GetExposureAttr(sID, &tmpAe);

	tmpAe.stAuto.stAntiflicker.bEnable = enable;
	tmpAe.stAuto.stAntiflicker.enFrequency = HzMode;

	if (HzMode == AE_FREQUENCE_50HZ)
		printf("Anti flicker enable:%d 50Hz!\n", enable);
	else
		printf("Anti flicker enable:%d 60Hz!\n", enable);

	AE_SetExposureAttr(sID, &tmpAe);
}


void AE_SetManualExposureMode(CVI_U8 sID, CVI_BOOL mode, ISP_EXPOSURE_ATTR_S *pstExpAttr)
{
	pstExpAttr->enOpType = mode;
	if (pstExpAttr->enOpType == OP_TYPE_MANUAL) {
		printf("sID:%d AE ManualExp!\n", sID);
		pstExpAttr->stManual.enExpTimeOpType = OP_TYPE_MANUAL;
		pstExpAttr->stManual.enAGainOpType = OP_TYPE_MANUAL;
		pstExpAttr->stManual.enDGainOpType = OP_TYPE_MANUAL;
		pstExpAttr->stManual.enISPDGainOpType = OP_TYPE_MANUAL;
	}
}


void AE_ShutterGainTest(CVI_U8 sID, AE_EXPOSURE *pStExp)
{
	CVI_U32 maxTime, expRatio, tmpExpRatio;
	CVI_FLOAT lineTime;
	SAE_INFO *tmpAeInfo;

	const CVI_U8 WDR_EXP_RATIO = 4;
	static	CVI_U32 expLine[2][AE_MAX_WDR_FRAME_NUM];

	if (AE_GetDebugMode(sID) >= SENSOR_PORTING_START_ITEM &&
		AE_GetDebugMode(sID) <= SENSOR_PORTING_END_ITEM) {
		AE_GetCurrentInfo(sID, &tmpAeInfo);
		maxTime = (tmpAeInfo->u32ExpTimeMax < 33333) ?
				tmpAeInfo->u32ExpTimeMax : 33333;
		lineTime = AAA_DIV_0_TO_1(tmpAeInfo->fExpLineTime);

		if (AE_GetDebugMode(sID) == SENSOR_PORTING_START_ITEM) {
			pStExp[AE_LE].u32SensorExpTimeLine = tmpAeInfo->u32ExpLineMax;
			pStExp[AE_LE].stSensorExpGain.u32AGain = AE_GAIN_BASE;
			pStExp[AE_LE].stSensorExpGain.u32DGain = AE_GAIN_BASE;
			pStExp[AE_LE].stSensorExpGain.u32ISPDGain = AE_GAIN_BASE;
		} else if (AE_GetDebugMode(sID) == 67) {
			if (tmpAeInfo->u32frmCnt % 100 < 50)
				AE_SetFpsTest(sID, 15);
			else
				AE_SetFpsTest(sID, 25);
		} else if (AE_GetDebugMode(sID) == 68) {
			if (tmpAeInfo->u32frmCnt % 20 < 10) {
				pStExp[AE_LE].stSensorExpGain.u32AGain = tmpAeInfo->u32AGainMax;
				pStExp[AE_LE].stSensorExpGain.u32DGain = AE_GAIN_BASE;
			} else {
				pStExp[AE_LE].stSensorExpGain.u32AGain = tmpAeInfo->u32AGainMax;
				pStExp[AE_LE].stSensorExpGain.u32DGain = tmpAeInfo->u32DGainMax;
			}
			pStExp[AE_LE].u32SensorExpTimeLine = maxTime / lineTime;
			pStExp[AE_LE].stSensorExpGain.u32ISPDGain = AE_GAIN_BASE;
		} else if (AE_GetDebugMode(sID) == 69) {
			if (tmpAeInfo->u32frmCnt % 20 < 10) {
				pStExp[AE_LE].u32SensorExpTimeLine =
					tmpAeInfo->u32ExpTimeMin / lineTime;
			} else {
				pStExp[AE_LE].u32SensorExpTimeLine =
					tmpAeInfo->u32ExpTimeMin * 2 / lineTime;
			}
			pStExp[AE_LE].stSensorExpGain.u32AGain = AE_GAIN_BASE;
			pStExp[AE_LE].stSensorExpGain.u32DGain = AE_GAIN_BASE;
			pStExp[AE_LE].stSensorExpGain.u32ISPDGain = AE_GAIN_BASE;
		} else if (AE_GetDebugMode(sID) == 70) {
			pStExp[AE_LE].u32SensorExpTimeLine = maxTime / lineTime;
			pStExp[AE_LE].stSensorExpGain.u32AGain = AE_GAIN_BASE;
			pStExp[AE_LE].stSensorExpGain.u32DGain = AE_GAIN_BASE;
			pStExp[AE_LE].stSensorExpGain.u32ISPDGain = AE_GAIN_BASE;
		} else if (AE_GetDebugMode(sID) == 71) {
			if (tmpAeInfo->u32frmCnt % 40 < 10)
				pStExp[AE_LE].u32SensorExpTimeLine = 1000 / lineTime;
			else if (tmpAeInfo->u32frmCnt % 40 < 20)
				pStExp[AE_LE].u32SensorExpTimeLine = maxTime / lineTime;
			else if (tmpAeInfo->u32frmCnt % 40 < 30)
				pStExp[AE_LE].u32SensorExpTimeLine = maxTime / 2 / lineTime;
			else
				pStExp[AE_LE].u32SensorExpTimeLine = maxTime / 4 / lineTime;

			pStExp[AE_LE].stSensorExpGain.u32AGain = AE_GAIN_BASE;
			pStExp[AE_LE].stSensorExpGain.u32DGain = AE_GAIN_BASE;
			pStExp[AE_LE].stSensorExpGain.u32ISPDGain = AE_GAIN_BASE;
		} else if (AE_GetDebugMode(sID) == 72) {
			if (tmpAeInfo->u8DGainAccuType == AE_ACCURACY_DB) {
				if (tmpAeInfo->u32frmCnt % 30 < 10) {
					pStExp[AE_LE].stSensorExpGain.u32AGain = 1024;
					pStExp[AE_LE].stSensorExpGain.u32DGain = AE_GAIN_BASE;
				} else if (tmpAeInfo->u32frmCnt % 30 < 20) {
					pStExp[AE_LE].stSensorExpGain.u32AGain = 2048;
					pStExp[AE_LE].stSensorExpGain.u32DGain = AE_GAIN_BASE;
				} else {
					pStExp[AE_LE].stSensorExpGain.u32AGain = 2048;
					pStExp[AE_LE].stSensorExpGain.u32DGain = tmpAeInfo->u32SensorDgainNode[1];
				}
			} else {
				if (tmpAeInfo->u32frmCnt % 40 < 10)
					pStExp[AE_LE].stSensorExpGain.u32AGain = 1024;
				else if (tmpAeInfo->u32frmCnt % 40 < 20)
					pStExp[AE_LE].stSensorExpGain.u32AGain = 2048;
				else if (tmpAeInfo->u32frmCnt % 40 < 30)
					pStExp[AE_LE].stSensorExpGain.u32AGain = 4096;
				else
					pStExp[AE_LE].stSensorExpGain.u32AGain = 8192;
				pStExp[AE_LE].stSensorExpGain.u32DGain = AE_GAIN_BASE;
			}
			maxTime = (maxTime < 10000) ? maxTime : 10000;
			pStExp[AE_LE].u32SensorExpTimeLine = maxTime / lineTime;
			pStExp[AE_LE].stSensorExpGain.u32ISPDGain = AE_GAIN_BASE;
		} else if (AE_GetDebugMode(sID) == 73) {
			if (tmpAeInfo->u32frmCnt % 20 < 10) {
				pStExp[AE_LE].u32SensorExpTimeLine = 1000 / lineTime;
				pStExp[AE_LE].stSensorExpGain.u32AGain = 1024;
				pStExp[AE_LE].stSensorExpGain.u32DGain = 1024;
			} else {
				pStExp[AE_LE].u32SensorExpTimeLine = maxTime / lineTime;
				pStExp[AE_LE].stSensorExpGain.u32AGain = 8192;
				pStExp[AE_LE].stSensorExpGain.u32DGain = 1024;
			}
			pStExp[AE_LE].stSensorExpGain.u32ISPDGain = AE_GAIN_BASE;
		} else if (AE_GetDebugMode(sID) == 74) {
			if (tmpAeInfo->u32frmCnt % 30 < 10) {
				pStExp[AE_LE].stSensorExpGain.u32AGain = 1024;
				pStExp[AE_LE].stSensorExpGain.u32ISPDGain = 1024;
			} else if (tmpAeInfo->u32frmCnt % 30 < 20) {
				pStExp[AE_LE].stSensorExpGain.u32AGain = 1024;
				pStExp[AE_LE].stSensorExpGain.u32ISPDGain = 4096;
			} else {
				pStExp[AE_LE].stSensorExpGain.u32AGain = 2048;
				pStExp[AE_LE].stSensorExpGain.u32ISPDGain = 2048;
			}
			pStExp[AE_LE].u32SensorExpTimeLine = maxTime / lineTime;
			pStExp[AE_LE].stSensorExpGain.u32DGain = AE_GAIN_BASE;
		} else if (AE_GetDebugMode(sID) >= 75 && AE_GetDebugMode(sID) <= 82 && tmpAeInfo->bWDRMode) {
			if (AE_GetDebugMode(sID) == 75) {
				if (tmpAeInfo->u32frmCnt % 30 < 10)
					expLine[0][AE_LE] = maxTime / lineTime;
				else if (tmpAeInfo->u32frmCnt % 30 < 20)
					expLine[0][AE_LE] = maxTime / 2 / lineTime;
				else
					expLine[0][AE_LE] = maxTime / 4 / lineTime;

				pStExp[AE_LE].stSensorExpGain.u32AGain = AE_GAIN_BASE;
				pStExp[AE_LE].stSensorExpGain.u32DGain = AE_GAIN_BASE;
				pStExp[AE_LE].stSensorExpGain.u32ISPDGain = AE_GAIN_BASE;
				expLine[0][AE_SE] = expLine[0][AE_LE] / WDR_EXP_RATIO;
				tmpExpRatio = WDR_EXP_RATIO * AE_WDR_RATIO_BASE;
				AE_GetWDRExpLineRange(sID, tmpExpRatio, &expLine[0][AE_LE], &expLine[0][AE_SE], 0);
				pStExp[AE_LE].u32SensorExpTimeLine = expLine[0][AE_LE];
				pStExp[AE_SE].u32SensorExpTimeLine = expLine[0][AE_SE];
			} else if (AE_GetDebugMode(sID) == 76) {
				if (tmpAeInfo->u32frmCnt % 30 < 10)
					expLine[0][AE_SE] =
						tmpAeInfo->u32WDRSEExpTimeMax / lineTime;
				else if (tmpAeInfo->u32frmCnt % 30 < 20)
					expLine[0][AE_SE] =
						tmpAeInfo->u32WDRSEExpTimeMax / 2 / lineTime;
				else
					expLine[0][AE_SE] =
						tmpAeInfo->u32WDRSEExpTimeMax / 4 / lineTime;

				pStExp[AE_LE].stSensorExpGain.u32AGain = AE_GAIN_BASE;
				pStExp[AE_LE].stSensorExpGain.u32DGain = AE_GAIN_BASE;
				pStExp[AE_LE].stSensorExpGain.u32ISPDGain = AE_GAIN_BASE;
				expLine[0][AE_LE] = expLine[0][AE_SE] * WDR_EXP_RATIO;
				tmpExpRatio = WDR_EXP_RATIO * AE_WDR_RATIO_BASE;
				AE_GetWDRExpLineRange(sID, tmpExpRatio, &expLine[0][AE_LE], &expLine[0][AE_SE], 0);
				pStExp[AE_LE].u32SensorExpTimeLine = expLine[0][AE_LE];
				pStExp[AE_SE].u32SensorExpTimeLine = expLine[0][AE_SE];
			} else if (AE_GetDebugMode(sID) == 77) {
				if (tmpAeInfo->u32frmCnt % 20 < 10)
					pStExp[AE_LE].stSensorExpGain.u32AGain = 1024;
				else
					pStExp[AE_LE].stSensorExpGain.u32AGain = 2048;

				pStExp[AE_LE].stSensorExpGain.u32DGain = AE_GAIN_BASE;
				pStExp[AE_LE].stSensorExpGain.u32ISPDGain = AE_GAIN_BASE;
				expLine[0][AE_LE] = maxTime / lineTime;
				expLine[0][AE_SE] = expLine[0][AE_LE] / WDR_EXP_RATIO;
				tmpExpRatio = WDR_EXP_RATIO * AE_WDR_RATIO_BASE;
				AE_GetWDRExpLineRange(sID, tmpExpRatio, &expLine[0][AE_LE], &expLine[0][AE_SE], 0);
				pStExp[AE_LE].u32SensorExpTimeLine = expLine[0][AE_LE];
				pStExp[AE_SE].u32SensorExpTimeLine = expLine[0][AE_SE];
			} else if (AE_GetDebugMode(sID) == 78) {
				if (tmpAeInfo->u32frmCnt % 30 < 10) {
					expLine[0][AE_LE] = maxTime / lineTime;
					pStExp[AE_LE].stSensorExpGain.u32AGain = 1024;
				} else if (tmpAeInfo->u32frmCnt % 30 < 20) {
					expLine[0][AE_LE] = maxTime / 2 / lineTime;
					pStExp[AE_LE].stSensorExpGain.u32AGain = 2048;
				} else {
					expLine[0][AE_LE] = maxTime / 4 / lineTime;
					pStExp[AE_LE].stSensorExpGain.u32AGain = 1024;
				}
				pStExp[AE_LE].stSensorExpGain.u32DGain = AE_GAIN_BASE;
				pStExp[AE_LE].stSensorExpGain.u32ISPDGain = AE_GAIN_BASE;
				expLine[0][AE_SE] = expLine[0][AE_LE] / WDR_EXP_RATIO;
				tmpExpRatio = WDR_EXP_RATIO * AE_WDR_RATIO_BASE;
				AE_GetWDRExpLineRange(sID, tmpExpRatio, &expLine[0][AE_LE], &expLine[0][AE_SE], 0);
				pStExp[AE_LE].u32SensorExpTimeLine = expLine[0][AE_LE];
				pStExp[AE_SE].u32SensorExpTimeLine = expLine[0][AE_SE];
			} else if (AE_GetDebugMode(sID) == 79) {
				if (tmpAeInfo->u32frmCnt % 30 < 10) {
					expLine[0][AE_SE] =
						tmpAeInfo->u32WDRSEExpTimeMax / lineTime;
					pStExp[AE_LE].stSensorExpGain.u32AGain = 1024;
				} else if (tmpAeInfo->u32frmCnt % 30 < 20) {
					expLine[0][AE_SE] =
						tmpAeInfo->u32WDRSEExpTimeMax / 2 / lineTime;
					pStExp[AE_LE].stSensorExpGain.u32AGain = 2048;
				} else {
					expLine[0][AE_SE] =
						tmpAeInfo->u32WDRSEExpTimeMax / 4 / lineTime;
					pStExp[AE_LE].stSensorExpGain.u32AGain = 1024;
				}
				expLine[0][AE_LE] = expLine[0][AE_SE] * WDR_EXP_RATIO;
				tmpExpRatio = WDR_EXP_RATIO * AE_WDR_RATIO_BASE;
				AE_GetWDRExpLineRange(sID, tmpExpRatio, &expLine[0][AE_LE], &expLine[0][AE_SE], 0);
				pStExp[AE_LE].u32SensorExpTimeLine = expLine[0][AE_LE];
				pStExp[AE_SE].u32SensorExpTimeLine = expLine[0][AE_SE];
				pStExp[AE_LE].stSensorExpGain.u32DGain = AE_GAIN_BASE;
				pStExp[AE_LE].stSensorExpGain.u32ISPDGain = AE_GAIN_BASE;
			} else if (AE_GetDebugMode(sID) == 80) {
				if (tmpAeInfo->u32frmCnt % 20 < 10) {
					pStExp[AE_LE].stSensorExpGain.u32ISPDGain = 1024;
					pStExp[AE_SE].stSensorExpGain.u32ISPDGain = 1024;
				} else {
					pStExp[AE_LE].stSensorExpGain.u32ISPDGain = 2048;
					pStExp[AE_SE].stSensorExpGain.u32ISPDGain = 8192;
				}
				pStExp[AE_LE].stSensorExpGain.u32AGain = AE_GAIN_BASE;
				pStExp[AE_LE].stSensorExpGain.u32DGain = AE_GAIN_BASE;
				expLine[0][AE_LE] = maxTime / lineTime;
				expLine[0][AE_SE] = expLine[0][AE_LE] / WDR_EXP_RATIO;
				tmpExpRatio = WDR_EXP_RATIO * AE_WDR_RATIO_BASE;
				AE_GetWDRExpLineRange(sID, tmpExpRatio, &expLine[0][AE_LE], &expLine[0][AE_SE], 0);
				pStExp[AE_LE].u32SensorExpTimeLine = expLine[0][AE_LE];
				pStExp[AE_SE].u32SensorExpTimeLine = expLine[0][AE_SE];
			} else if (AE_GetDebugMode(sID) == SENSOR_PORTING_END_ITEM) {
				expRatio = tmpAeInfo->u32ExpLineMax * AE_WDR_RATIO_BASE /
					tmpAeInfo->u32WDRSEExpLineMax;
				expLine[0][AE_LE] = tmpAeInfo->u32ExpLineMax;
				expLine[0][AE_SE] = tmpAeInfo->u32WDRSEExpLineMax;
				AE_GetWDRExpLineRange(sID, expRatio, &expLine[0][AE_LE], &expLine[0][AE_SE], 1);

				printf("Exp1 line:%u %u\n", expLine[0][AE_LE], expLine[0][AE_SE]);
				expLine[1][AE_LE] = expLine[0][AE_LE];
				expLine[1][AE_SE] = expLine[0][AE_SE];

				for (tmpExpRatio = expRatio; tmpExpRatio <= 512 ; tmpExpRatio++) {
					AE_GetWDRExpLineRange(sID, tmpExpRatio, &expLine[1][AE_LE],
						&expLine[1][AE_SE], 1);
					#if 0
					printf("R:%d Exp1:%d %d Exp2:%d %d, %d - OK\n", tmpExpRatio,
						expLine1[AE_LE], expLine1[AE_SE], expLine2[AE_LE],
						expLine2[AE_SE], expLine2[AE_LE] + expLine1[AE_SE]);
					#endif
					if (expLine[1][AE_LE] + expLine[0][AE_SE] >= tmpAeInfo->u32FrameLine) {
						printf("R:%d Exp1 %d %d Exp2 %d %d FL:%d --- FindExp\n",
							tmpExpRatio, expLine[0][AE_LE], expLine[0][AE_SE],
							expLine[1][AE_LE], expLine[1][AE_SE],
							tmpAeInfo->u32FrameLine);
						break;
					}
				}

				if (tmpAeInfo->u32frmCnt % 20 < 10) {
					pStExp[AE_LE].u32SensorExpTimeLine = expLine[0][AE_LE];
					pStExp[AE_SE].u32SensorExpTimeLine = expLine[0][AE_SE];
				} else {
					pStExp[AE_LE].u32SensorExpTimeLine = expLine[1][AE_LE];
					pStExp[AE_SE].u32SensorExpTimeLine = expLine[1][AE_SE];
				}

				pStExp[AE_LE].stSensorExpGain.u32AGain = AE_GAIN_BASE;
				pStExp[AE_LE].stSensorExpGain.u32DGain = AE_GAIN_BASE;
				pStExp[AE_LE].stSensorExpGain.u32ISPDGain = AE_GAIN_BASE;
			}

			pStExp[AE_SE].stSensorExpGain.u32AGain = pStExp[AE_LE].stSensorExpGain.u32AGain;
			pStExp[AE_SE].stSensorExpGain.u32DGain = pStExp[AE_LE].stSensorExpGain.u32DGain;
			if (AE_GetDebugMode(sID) != 80)
				pStExp[AE_SE].stSensorExpGain.u32ISPDGain = pStExp[AE_LE].stSensorExpGain.u32ISPDGain;

		}
	}
}

void AE_GetExposureDelayTime(VI_PIPE ViPipe, CVI_U8 frameNum, CVI_U32 *time)
{
	CVI_FLOAT fps;

	CVI_ISP_QueryFps(ViPipe, &fps);
	if (fps < 1)
		fps = 1;
	*time = frameNum * 1000 / fps;
}

CVI_BOOL AE_ExposureAttr_ByPss_Test(VI_PIPE ViPipe)
{
	ISP_EXPOSURE_ATTR_S tmpExpAttr;
	ISP_EXP_INFO_S	tempExpInfo, oriExpInfo;
	CVI_BOOL result = CVI_SUCCESS;
	CVI_U16 frameLuma[2] = {0, 0};
	CVI_U32 time[2] = {0, 0};
	CVI_U8 sID;
	CVI_U32 delayTime;
	CVI_U8	delayFrameNum;

	sID = AE_ViPipe2sID(ViPipe);
	delayFrameNum = AE_GetMeterPeriod(sID) + 2;

	CVI_ISP_GetExposureAttr(ViPipe, &tmpExpAttr);
	CVI_ISP_QueryExposureInfo(ViPipe, &oriExpInfo);
	AE_GetExposureDelayTime(ViPipe, delayFrameNum, &delayTime);

	time[0] = oriExpInfo.u32ExpTime;
	tmpExpAttr.bByPass = 1;
	CVI_ISP_SetExposureAttr(ViPipe, &tmpExpAttr);
	AE_Delay1ms(delayTime);
	frameLuma[0] = AE_GetFrameLuma(sID);
	tmpExpAttr.enOpType = OP_TYPE_MANUAL;
	tmpExpAttr.stManual.enExpTimeOpType = OP_TYPE_MANUAL;
	tmpExpAttr.stManual.u32ExpTime = 1000;
	CVI_ISP_SetExposureAttr(ViPipe, &tmpExpAttr);
	AE_Delay1ms(delayTime);
	CVI_ISP_QueryExposureInfo(ViPipe, &tempExpInfo);
	frameLuma[1] = AE_GetFrameLuma(sID);
	time[1] = tempExpInfo.u32ExpTime;
	tmpExpAttr.bByPass = 0;
	tmpExpAttr.enOpType = OP_TYPE_AUTO;
	tmpExpAttr.stManual.enExpTimeOpType = OP_TYPE_AUTO;
	tmpExpAttr.stManual.u32ExpTime = oriExpInfo.u32ExpTime;
	tmpExpAttr.stAuto.u8Speed = 255;
	tmpExpAttr.stAuto.u16BlackSpeedBias = 255;
	CVI_ISP_SetExposureAttr(ViPipe, &tmpExpAttr);
	AE_Delay1ms(delayTime);

	if (frameLuma[0] > frameLuma[1] * 11/10 ||
		frameLuma[0] < frameLuma[1] * 9/10  || time[0] != time[1])
		result = CVI_FAILURE;

	printf("BypassTest Luma0:%d Luma1:%d time0:%u time1:%u res:%d\n",
		frameLuma[0], frameLuma[1], time[0], time[1], result);

	return result;
}

CVI_BOOL AE_ExposureAttr_ManualExp_Test(VI_PIPE ViPipe)
{
#define NAME_LENGTH 30

	ISP_EXPOSURE_ATTR_S tmpExpAttr, oriExpAttr;
	ISP_EXP_INFO_S	tmpExpInfo, oriExpInfo;
	CVI_U8 result = CVI_SUCCESS;
	CVI_U16 frameLuma[2] = {0, 0};
	CVI_CHAR itemName[AE_TEST_TOTAL][NAME_LENGTH] = {"ManualExp",
		"ManualISO", "ManualGain"};
	CVI_U8 sID;
	CVI_U16 diff;
	CVI_U8 diffRatio;
	CVI_U32 delayTime;
	CVI_U8	delayFrameNum;
	SAE_INFO *tmpAeInfo;

	sID = AE_ViPipe2sID(ViPipe);
	delayFrameNum = AE_GetMeterPeriod(sID) + 2;

	CVI_ISP_GetExposureAttr(ViPipe, &tmpExpAttr);
	CVI_ISP_QueryExposureInfo(ViPipe, &tmpExpInfo);
	AE_GetExposureDelayTime(ViPipe, delayFrameNum, &delayTime);
	AE_GetCurrentInfo(sID, &tmpAeInfo);

	oriExpInfo = tmpExpInfo;
	oriExpAttr = tmpExpAttr;
	tmpExpAttr.enOpType = OP_TYPE_MANUAL;
	frameLuma[0] = AE_GetCenterG(sID, 0);

	//ISO Num
	tmpExpAttr.stManual.enExpTimeOpType = OP_TYPE_MANUAL;
	tmpExpAttr.stManual.u32ExpTime = oriExpInfo.u32ExpTime;
	tmpExpAttr.stManual.enISONumOpType = OP_TYPE_MANUAL;
	tmpExpAttr.stManual.enGainType = AE_TYPE_ISO;
	tmpExpAttr.stManual.u32ISONum = oriExpInfo.u32ISO * 4;

	CVI_ISP_SetExposureAttr(ViPipe, &tmpExpAttr);
	AE_Delay1ms(delayTime);
	frameLuma[1] = AE_GetCenterG(sID, 0);
	CVI_ISP_QueryExposureInfo(ViPipe, &tmpExpInfo);

	diff = (tmpExpAttr.stManual.u32ISONum > tmpExpInfo.u32ISO) ?
		tmpExpAttr.stManual.u32ISONum - tmpExpInfo.u32ISO :
		tmpExpInfo.u32ISO - tmpExpAttr.stManual.u32ISONum;
	diffRatio = diff * 100 / AAA_DIV_0_TO_1(tmpExpAttr.stManual.u32ISONum);

	if (frameLuma[1] < frameLuma[0] * 2 || diffRatio > 3)
		result = CVI_FAILURE;

	printf("%s--Luma0:%d Luma1:%d ISONum:%u %u ratio:%d Thr:%d res:%d\n",
		itemName[AE_TEST_ISO_NUM], frameLuma[0], frameLuma[1], tmpExpAttr.stManual.u32ISONum,
		tmpExpInfo.u32ISO, diffRatio, frameLuma[0] * 2,  result);

	if (result) {
		CVI_ISP_SetExposureAttr(ViPipe, &oriExpAttr);
		return result;
	}

	//Shutter
	tmpExpAttr.stManual.enExpTimeOpType = OP_TYPE_MANUAL;
	tmpExpAttr.stManual.u32ExpTime = oriExpInfo.u32ExpTime / 4;
	tmpExpAttr.stManual.enISONumOpType = OP_TYPE_MANUAL;
	tmpExpAttr.stManual.enGainType = AE_TYPE_ISO;
	tmpExpAttr.stManual.u32ISONum = oriExpInfo.u32ISO;

	if (tmpExpAttr.stManual.u32ExpTime < tmpAeInfo->u32ExpTimeMin)
		tmpExpAttr.stManual.u32ExpTime = tmpAeInfo->u32ExpTimeMin;

	CVI_ISP_SetExposureAttr(ViPipe, &tmpExpAttr);
	AE_Delay1ms(delayTime);
	frameLuma[1] = AE_GetCenterG(sID, 0);
	CVI_ISP_QueryExposureInfo(ViPipe, &tmpExpInfo);
	diff = (tmpExpAttr.stManual.u32ExpTime > tmpExpInfo.u32ExpTime) ?
		tmpExpAttr.stManual.u32ExpTime - tmpExpInfo.u32ExpTime :
		tmpExpInfo.u32ExpTime - tmpExpAttr.stManual.u32ExpTime;
	diffRatio = diff * 100 / AAA_DIV_0_TO_1(tmpExpAttr.stManual.u32ExpTime);

	if (frameLuma[1] > frameLuma[0] / 2 || AAA_ABS(diffRatio) > 3) {
		result = CVI_FAILURE;
	}

	printf("%s--Luma0:%d Luma1:%d time:%u %u ratio:%d Thr:%d result:%d\n", itemName[AE_TEST_SHUTTER],
		frameLuma[0], frameLuma[1], tmpExpAttr.stManual.u32ExpTime,
		tmpExpInfo.u32ExpTime, diffRatio, frameLuma[0] / 2, result);

	if (result) {
		CVI_ISP_SetExposureAttr(ViPipe, &oriExpAttr);
		return result;
	}

	//Gain
	tmpExpAttr.stManual.enExpTimeOpType = OP_TYPE_MANUAL;
	tmpExpAttr.stManual.u32ExpTime = oriExpInfo.u32ExpTime;
	tmpExpAttr.stManual.enGainType = AE_TYPE_GAIN;
	tmpExpAttr.stManual.enAGainOpType = OP_TYPE_MANUAL;
	tmpExpAttr.stManual.enDGainOpType = OP_TYPE_MANUAL;
	tmpExpAttr.stManual.enISPDGainOpType = OP_TYPE_MANUAL;
	tmpExpAttr.stManual.u32AGain = oriExpInfo.u32AGain * 2;
	tmpExpAttr.stManual.u32DGain = oriExpInfo.u32DGain;
	tmpExpAttr.stManual.u32ISPDGain = oriExpInfo.u32ISPDGain * 2;
	CVI_ISP_SetExposureAttr(ViPipe, &tmpExpAttr);
	AE_Delay1ms(delayTime);
	CVI_ISP_QueryExposureInfo(ViPipe, &tmpExpInfo);
	frameLuma[1] = AE_GetCenterG(sID, 0);

	if (frameLuma[1] < frameLuma[0] * 2 ||
		tmpExpInfo.u32AGain <= tmpExpAttr.stManual.u32AGain * 98 / 100 ||
		tmpExpInfo.u32AGain >= tmpExpAttr.stManual.u32AGain * 102 / 100 ||
		tmpExpInfo.u32ISPDGain <= tmpExpAttr.stManual.u32ISPDGain * 98 / 100 ||
		tmpExpInfo.u32ISPDGain >= tmpExpAttr.stManual.u32ISPDGain * 102 / 100)
		result = CVI_FAILURE;

	printf("%s--Luma0:%d Luma1:%d AGain:%u %u ISPDGain:%u %u Thr:%d result:%d\n",
		itemName[AE_TEST_GAIN], frameLuma[0], frameLuma[1],
		tmpExpAttr.stManual.u32AGain, tmpExpInfo.u32AGain,
		tmpExpAttr.stManual.u32ISPDGain, tmpExpInfo.u32ISPDGain,
		frameLuma[0] * 2, result);

	if (result) {
		CVI_ISP_SetExposureAttr(ViPipe, &oriExpAttr);
		return result;
	}
	tmpExpAttr.stManual.enExpTimeOpType = OP_TYPE_AUTO;
	tmpExpAttr.stManual.enAGainOpType = OP_TYPE_AUTO;
	tmpExpAttr.stManual.enDGainOpType = OP_TYPE_AUTO;
	tmpExpAttr.stManual.enISPDGainOpType = OP_TYPE_AUTO;
	tmpExpAttr.enOpType = OP_TYPE_AUTO;
	CVI_ISP_SetExposureAttr(ViPipe, &tmpExpAttr);
	delayFrameNum = AE_GetMeterPeriod(sID) * 5;
	AE_GetExposureDelayTime(ViPipe, delayFrameNum, &delayTime);
	AE_Delay1ms(delayTime);

	return result;
}

CVI_BOOL AE_ExpsoureAttr_ExpRange_Test(VI_PIPE ViPipe)
{
	ISP_EXPOSURE_ATTR_S oriExpAttr, tmpExpAttr;
	ISP_EXP_INFO_S	oriExpInfo, tmpExpInfo;
	CVI_S16 tvEntryMin,  tvEntryMax;
	CVI_S16 svEntryMin,  svEntryMax;
	CVI_U8 result = CVI_SUCCESS;
	CVI_U32	oriExpTime, oriSysGain, oriISONum;
	CVI_CHAR itemName[AE_TEST_TOTAL][NAME_LENGTH] = {"ExpRanage",
		"ISORange", "GainRange"};
	CVI_U8 sID;
	CVI_U16 frameLuma[2] = {0, 0};
	CVI_U16	diff;
	CVI_U8 diffRatio;
	CVI_U32 delayTime;
	CVI_U8	delayFrameNum;

	sID = AE_ViPipe2sID(ViPipe);
	delayFrameNum = AE_GetMeterPeriod(sID) * 5;
	AE_GetExposureDelayTime(ViPipe, delayFrameNum, &delayTime);

	frameLuma[0] = AE_GetCenterG(sID, 0);
	CVI_ISP_GetExposureAttr(ViPipe, &tmpExpAttr);
	CVI_ISP_QueryExposureInfo(ViPipe, &oriExpInfo);
	AE_GetTVEntryLimit(sID, &tvEntryMin,  &tvEntryMax);
	AE_GetExpTimeByEntry(sID, tvEntryMin, &oriExpTime);

	AE_GetISOEntryLimit(sID, &svEntryMin,  &svEntryMax);
	AE_GetGainBySvEntry(sID, AE_GAIN_TOTAL, svEntryMax, &oriSysGain);
	AE_GetISONumByEntry(sID, svEntryMax, &oriISONum);

	oriExpAttr = tmpExpAttr;
	tmpExpAttr.bByPass = 0;
	tmpExpAttr.enOpType = OP_TYPE_AUTO;
	tmpExpAttr.stAuto.u8Speed = 255;
	tmpExpAttr.stAuto.u16BlackSpeedBias = 255;

	//Shutter
	tmpExpAttr.stAuto.stExpTimeRange.u32Max = oriExpInfo.u32ExpTime / 8;
	tmpExpAttr.stAuto.enGainType = AE_TYPE_ISO;
	tmpExpAttr.stAuto.stISONumRange.u32Max = oriExpInfo.u32ISO * 2;
	CVI_ISP_SetExposureAttr(ViPipe, &tmpExpAttr);
	AE_Delay1ms(delayTime);
	frameLuma[1] = AE_GetCenterG(sID, 0);

	CVI_ISP_QueryExposureInfo(ViPipe, &tmpExpInfo);

	diff = (tmpExpInfo.u32ISO > tmpExpAttr.stAuto.stISONumRange.u32Max) ?
		tmpExpInfo.u32ISO - tmpExpAttr.stAuto.stISONumRange.u32Max :
		tmpExpAttr.stAuto.stISONumRange.u32Max - tmpExpInfo.u32ISO;

	diffRatio = diff * 100 / AAA_DIV_0_TO_1(tmpExpAttr.stAuto.stISONumRange.u32Max);

	if (frameLuma[1] > frameLuma[0] / 2 ||
		tmpExpInfo.u32ExpTime > tmpExpAttr.stAuto.stExpTimeRange.u32Max ||
		diffRatio > 3)
		result = CVI_FAILURE;

	printf("%s--ExpTime:%u %u\n", itemName[AE_TEST_SHUTTER],
		tmpExpInfo.u32ExpTime, tmpExpAttr.stAuto.stExpTimeRange.u32Max);
	printf("%s--ISONum:%u %u\n", itemName[AE_TEST_ISO_NUM],
		tmpExpInfo.u32ISO, tmpExpAttr.stAuto.stISONumRange.u32Max);
	printf("Luma0:%d Luma1:%d ratio:%d result:%d\n", frameLuma[0], frameLuma[1],
		diffRatio, result);

	if (result)
		return result;

	//Gain1
	tmpExpAttr.stAuto.enGainType = AE_TYPE_GAIN;
	tmpExpAttr.stAuto.stSysGainRange.u32Max = 1024;
	CVI_ISP_SetExposureAttr(ViPipe, &tmpExpAttr);
	AE_Delay1ms(delayTime);
	frameLuma[1] = AE_GetCenterG(sID, 0);

	CVI_ISP_QueryExposureInfo(ViPipe, &tmpExpInfo);
	if (frameLuma[1] > frameLuma[0] / 4 || tmpExpInfo.u32ISO > 100)
		result = CVI_FAILURE;

	printf("%s1--svEntryMax:%d ISONum:%u AG:%u DG:%u IG:%u\n",
		itemName[AE_TEST_GAIN], svEntryMax, tmpExpInfo.u32ISO,
		tmpExpInfo.u32AGain, tmpExpInfo.u32DGain, tmpExpInfo.u32ISPDGain);
	printf("Luma0:%d Luma1:%d thr:%d res:%d\n", frameLuma[0], frameLuma[1],
		frameLuma[0] / 4, result);

	tmpExpAttr.stAuto.stSysGainRange.u32Max = oriSysGain;
	CVI_ISP_SetExposureAttr(ViPipe, &tmpExpAttr);
	AE_Delay1ms(delayTime);

	if (result)
		return result;

	//Gain2
	tmpExpAttr.stAuto.enGainType = AE_TYPE_GAIN;
	tmpExpAttr.stAuto.stAGainRange.u32Max = 4096;
	tmpExpAttr.stAuto.stISPDGainRange.u32Max = 2048;
	tmpExpAttr.stAuto.stSysGainRange.u32Max =
		AE_CalTotalGain(4096, 1024, 2048);
	CVI_ISP_SetExposureAttr(ViPipe, &tmpExpAttr);
	AE_Delay1ms(delayTime);
	frameLuma[1] = AE_GetCenterG(sID, 0);

	CVI_ISP_QueryExposureInfo(ViPipe, &tmpExpInfo);
	if (tmpExpInfo.u32AGain > 4096 || tmpExpInfo.u32ISPDGain > 2048)
		result = CVI_FAILURE;

	printf("%s2--AGain:%u ISPDGain:%u result:%d\n", itemName[AE_TEST_GAIN],
		tmpExpInfo.u32AGain, tmpExpInfo.u32ISPDGain, result);

	tmpExpAttr.stAuto.stExpTimeRange.u32Max = oriExpAttr.stAuto.stExpTimeRange.u32Max;
	tmpExpAttr.stAuto.stAGainRange.u32Max = oriExpAttr.stAuto.stAGainRange.u32Max;
	tmpExpAttr.stAuto.stISPDGainRange.u32Max = oriExpAttr.stAuto.stISPDGainRange.u32Max;

	CVI_ISP_SetExposureAttr(ViPipe, &tmpExpAttr);
	AE_Delay1ms(delayTime);

	return result;
}

CVI_BOOL AE_ExpsoureAttr_AutoParam_Test(VI_PIPE ViPipe)
{
	ISP_EXPOSURE_ATTR_S tmpExpAttr;
	ISP_EXP_INFO_S	tmpExpInfo;
	CVI_U16 frameLuma[2], oriTarget, newTarget;
	CVI_BOOL result = CVI_SUCCESS;
	CVI_U8 oriSpeed;
	CVI_U8 sID;
	CVI_U16 oriEvBias, timeDiffRatio[3], minTimeDiff;
	CVI_U32 delayTime;
	CVI_U8	delayFrameNum;

	sID = AE_ViPipe2sID(ViPipe);
	delayFrameNum = AE_GetMeterPeriod(sID) * 5;
	AE_GetExposureDelayTime(ViPipe, delayFrameNum, &delayTime);

	//target
	CVI_ISP_GetExposureAttr(ViPipe, &tmpExpAttr);
	oriTarget = tmpExpAttr.stAuto.u8Compensation;
	oriSpeed = tmpExpAttr.stAuto.u8Speed;
	oriEvBias = tmpExpAttr.stAuto.u16EVBias;
	frameLuma[0] = AE_GetCenterG(sID, 0);
	tmpExpAttr.bByPass = 0;
	tmpExpAttr.enOpType = OP_TYPE_AUTO;
	newTarget = tmpExpAttr.stAuto.u8Compensation / 4;
	tmpExpAttr.stAuto.u8Compensation = newTarget;
	tmpExpAttr.stAuto.u8Speed = 255;
	CVI_ISP_SetExposureAttr(ViPipe, &tmpExpAttr);
	AE_Delay1ms(delayTime);

	frameLuma[1] = AE_GetCenterG(sID, 0);
	if (frameLuma[1] > frameLuma[0] / 2)
		result = CVI_FAILURE;

	printf("target %d->%d luma0:%d luma1:%d Thr:%d result:%d\n",
		oriTarget, newTarget, frameLuma[0], frameLuma[1],
		frameLuma[0] / 2, result);

	tmpExpAttr.stAuto.u8Compensation = oriTarget;
	CVI_ISP_SetExposureAttr(ViPipe, &tmpExpAttr);
	AE_Delay1ms(delayTime);

	#if 0
	if (result)
		return result;
	#endif

	//EvBias
	tmpExpAttr.stAuto.u16EVBias = 256;
	CVI_ISP_SetExposureAttr(ViPipe, &tmpExpAttr);
	AE_Delay1ms(delayTime);
	frameLuma[1] = AE_GetCenterG(sID, 0);
	if (frameLuma[1]  > frameLuma[0] / 2)
		result = CVI_FAILURE;

	printf("Evbias %d->%d luma0:%d luma1:%d thr:%d result:%d\n",
		oriEvBias, tmpExpAttr.stAuto.u16EVBias, frameLuma[0],
		frameLuma[1], frameLuma[0] / 2, result);

	tmpExpAttr.stAuto.u16EVBias = 1024;
	CVI_ISP_SetExposureAttr(ViPipe, &tmpExpAttr);
	AE_Delay1ms(delayTime);
	#if 0
	if (result)
		return result;
	#endif
	//flicker
	tmpExpAttr.stAuto.u16EVBias = 4096;
	tmpExpAttr.stAuto.stAntiflicker.bEnable = 1;
	tmpExpAttr.stAuto.stAntiflicker.enFrequency = AE_FREQUENCE_60HZ;
	CVI_ISP_SetExposureAttr(ViPipe, &tmpExpAttr);
	AE_Delay1ms(delayTime);
	CVI_ISP_QueryExposureInfo(ViPipe, &tmpExpInfo);

	timeDiffRatio[0] = tmpExpInfo.u32ExpTime * 100 / 16666;
	timeDiffRatio[1] = tmpExpInfo.u32ExpTime * 100 / 33333;
	timeDiffRatio[0] = AAA_ABS(timeDiffRatio[0] - 100);
	timeDiffRatio[1] = AAA_ABS(timeDiffRatio[1] - 100);
	minTimeDiff = AAA_MIN(timeDiffRatio[0], timeDiffRatio[1]);

	if (tmpExpInfo.u32ExpTime > 8333 && minTimeDiff > 2)
		result = CVI_FAILURE;

	printf("antiFlicker freq:%d time:%u result:%d\n",
		tmpExpAttr.stAuto.stAntiflicker.enFrequency,
		tmpExpInfo.u32ExpTime, result);

	#if 0
	if (result)
		return result;
	#endif

	tmpExpAttr.stAuto.stAntiflicker.enFrequency = AE_FREQUENCE_50HZ;
	CVI_ISP_SetExposureAttr(ViPipe, &tmpExpAttr);
	AE_Delay1ms(delayTime);
	CVI_ISP_QueryExposureInfo(ViPipe, &tmpExpInfo);
	timeDiffRatio[0] = tmpExpInfo.u32ExpTime * 100 / 20000;
	timeDiffRatio[1] = tmpExpInfo.u32ExpTime * 100 / 30000;
	timeDiffRatio[2] = tmpExpInfo.u32ExpTime * 100 / 40000;

	timeDiffRatio[0] = AAA_ABS(timeDiffRatio[0] - 100);
	timeDiffRatio[1] = AAA_ABS(timeDiffRatio[1] - 100);
	timeDiffRatio[2] = AAA_ABS(timeDiffRatio[2] - 100);
	minTimeDiff = AAA_MIN(timeDiffRatio[0], timeDiffRatio[1]);
	minTimeDiff = AAA_MIN(minTimeDiff, timeDiffRatio[2]);

	if (tmpExpInfo.u32ExpTime > 10000 && minTimeDiff > 2)
		result = CVI_FAILURE;

	printf("antiFlicker freq:%d time:%u result:%d\n",
		tmpExpAttr.stAuto.stAntiflicker.enFrequency,
		tmpExpInfo.u32ExpTime, result);

	tmpExpAttr.stAuto.stAntiflicker.bEnable = 0;
	tmpExpAttr.stAuto.u8Speed = oriSpeed;
	CVI_ISP_SetExposureAttr(ViPipe, &tmpExpAttr);
	AE_Delay1ms(delayTime);

	return result;
}


CVI_BOOL CVI_ISP_SetAEStatisticsConfig_Test(VI_PIPE ViPipe)
{
#define WEIGHT_VALUE	255

	ISP_AE_STATISTICS_CFG_S aeWinCfg;
	CVI_U8 tmpWeight[AE_WEIGHT_ZONE_ROW][AE_WEIGHT_ZONE_COLUMN];
	CVI_BOOL result = CVI_SUCCESS;
	CVI_U8 sID;
	CVI_U32 delayTime;
	CVI_U8	delayFrameNum, row, column;

	printf("Func : %s\n", __func__);
	sID = AE_ViPipe2sID(ViPipe);
	delayFrameNum = AE_GetMeterPeriod(sID) + 2;
	AE_GetExposureDelayTime(ViPipe, delayFrameNum, &delayTime);

	row = AE_WEIGHT_ZONE_ROW;
	column = AE_WEIGHT_ZONE_COLUMN;
	memset(aeWinCfg.au8Weight, 0, row * column * sizeof(CVI_U8));
	memset(tmpWeight, 0, row * column * sizeof(CVI_U8));
	aeWinCfg.au8Weight[0][0] = WEIGHT_VALUE;
	aeWinCfg.au8Weight[0][column-1] = WEIGHT_VALUE;
	aeWinCfg.au8Weight[row-1][0] = WEIGHT_VALUE;
	aeWinCfg.au8Weight[row-1][column-1] = WEIGHT_VALUE;
	CVI_ISP_SetAEStatisticsConfig(ViPipe, &aeWinCfg);
	AE_Delay1ms(delayTime);
	CVI_ISP_GetAEStatisticsConfig(ViPipe, &aeWinCfg);
	if (aeWinCfg.au8Weight[0][0] != WEIGHT_VALUE ||
		aeWinCfg.au8Weight[0][column-1] != WEIGHT_VALUE ||
		aeWinCfg.au8Weight[row-1][0] != WEIGHT_VALUE ||
		aeWinCfg.au8Weight[row-1][column-1] != WEIGHT_VALUE) {
		result = CVI_FAILURE;
		printf("NG weight:%d %d %d %d\n", aeWinCfg.au8Weight[0][0],
			aeWinCfg.au8Weight[0][column-1],
			aeWinCfg.au8Weight[row-1][0],
			aeWinCfg.au8Weight[row-1][column-1]);
	}

	printf("weight result:%d\n", result);
	return result;
}

CVI_BOOL CVI_ISP_SetAERouteAttr_Test(VI_PIPE ViPipe)
{
	ISP_AE_ROUTE_S aeRoute, oriRoute;
	ISP_EXP_INFO_S aeInfo;
	ISP_EXPOSURE_ATTR_S	aeAttr;
	CVI_U8 i, sID;
	CVI_BOOL result = CVI_SUCCESS;
	CVI_U32 delayTime;
	CVI_U8	delayFrameNum;

	printf("Func : %s\n", __func__);
	sID = AE_ViPipe2sID(ViPipe);
	delayFrameNum = AE_GetMeterPeriod(sID) + 2;
	AE_GetExposureDelayTime(ViPipe, delayFrameNum, &delayTime);

	CVI_ISP_GetExposureAttr(ViPipe, &aeAttr);
	aeAttr.bAERouteExValid = 0;
	CVI_ISP_SetExposureAttr(ViPipe, &aeAttr);
	AE_Delay1ms(delayTime);

	CVI_ISP_GetAERouteAttr(ViPipe, &aeRoute);
	oriRoute = aeRoute;
	aeRoute.astRouteNode[aeRoute.u32TotalNum - 2].u32IntTime = 16666;
	aeRoute.astRouteNode[aeRoute.u32TotalNum - 1].u32IntTime = 16666;
	aeRoute.astRouteNode[aeRoute.u32TotalNum - 1].u32SysGain = 2048;

	CVI_ISP_SetAERouteAttr(ViPipe, &aeRoute);
	AE_Delay1ms(delayTime);
	CVI_ISP_QueryExposureInfo(ViPipe, &aeInfo);
	if (aeRoute.u32TotalNum != aeInfo.stAERoute.u32TotalNum) {
		result = CVI_FAILURE;
		printf("NG size:%u %u\n", aeRoute.u32TotalNum,
			aeInfo.stAERoute.u32TotalNum);
		printf("route:\n");
		for (i = 0; i < aeRoute.u32TotalNum; ++i) {
			printf("%d %u %u\n", i, aeRoute.astRouteNode[i].u32IntTime,
				aeRoute.astRouteNode[i].u32SysGain);
		}
		printf("route info:\n");
		for (i = 0; i < aeInfo.stAERoute.u32TotalNum; ++i) {
			printf("%d %u %u\n", i, aeInfo.stAERoute.astRouteNode[i].u32IntTime,
				aeInfo.stAERoute.astRouteNode[i].u32SysGain);
		}
	} else {
		for (i = 0; i < aeRoute.u32TotalNum; ++i) {
			if ((aeInfo.stAERoute.astRouteNode[i].u32IntTime !=
				aeRoute.astRouteNode[i].u32IntTime) ||
				(aeInfo.stAERoute.astRouteNode[i].u32SysGain !=
				aeRoute.astRouteNode[i].u32SysGain)) {
				printf("NG node:%d Time:%u %u Gain:%u %u\n", i,
					aeInfo.stAERoute.astRouteNode[i].u32IntTime,
					aeRoute.astRouteNode[i].u32IntTime,
					aeInfo.stAERoute.astRouteNode[i].u32SysGain,
					aeRoute.astRouteNode[i].u32SysGain);
					result = CVI_FAILURE;
					break;
			}
		}
	}

	CVI_ISP_SetAERouteAttr(ViPipe, &oriRoute);
	AE_Delay1ms(delayTime);
	printf("route result:%d\n", result);
	return result;
}

CVI_BOOL CVI_ISP_SetAERouteAttrEx_Test(VI_PIPE ViPipe)
{
	ISP_AE_ROUTE_EX_S aeRouteEx;
	ISP_EXP_INFO_S aeInfo;
	ISP_EXPOSURE_ATTR_S	aeAttr;
	CVI_U8 i, sID;
	CVI_BOOL result = CVI_SUCCESS;
	CVI_U32 delayTime;
	CVI_U8	delayFrameNum;

	printf("Func : %s\n", __func__);
	sID = AE_ViPipe2sID(ViPipe);
	delayFrameNum = AE_GetMeterPeriod(sID) * 2;
	AE_GetExposureDelayTime(ViPipe, delayFrameNum, &delayTime);

	CVI_ISP_GetExposureAttr(ViPipe, &aeAttr);
	CVI_ISP_GetAERouteAttrEx(ViPipe, &aeRouteEx);
	aeRouteEx.u32TotalNum = 4;

	aeRouteEx.astRouteExNode[1].u32IntTime = 8333;
	aeRouteEx.astRouteExNode[1].u32Again = 1024;
	aeRouteEx.astRouteExNode[1].u32Dgain = 1024;
	aeRouteEx.astRouteExNode[1].u32IspDgain = 1024;

	aeRouteEx.astRouteExNode[2].u32IntTime = 8333;
	aeRouteEx.astRouteExNode[2].u32Again = 2048;
	aeRouteEx.astRouteExNode[2].u32Dgain = 1024;
	aeRouteEx.astRouteExNode[2].u32IspDgain = 1024;

	aeRouteEx.astRouteExNode[3].u32IntTime = 8333;
	aeRouteEx.astRouteExNode[3].u32Again = 2048;
	aeRouteEx.astRouteExNode[3].u32Dgain = 1024;
	aeRouteEx.astRouteExNode[3].u32IspDgain = 2048;

	CVI_ISP_SetAERouteAttrEx(ViPipe, &aeRouteEx);
	aeAttr.bAERouteExValid = 1;
	CVI_ISP_SetExposureAttr(ViPipe, &aeAttr);
	AE_Delay1ms(delayTime);
	CVI_ISP_QueryExposureInfo(ViPipe, &aeInfo);
	if (aeRouteEx.u32TotalNum != aeInfo.stAERouteEx.u32TotalNum) {
		result = CVI_FAILURE;
		printf("NG size:%u %u\n", aeRouteEx.u32TotalNum,
			aeInfo.stAERouteEx.u32TotalNum);
	} else {
		for (i = 0; i < aeRouteEx.u32TotalNum; ++i) {
			if ((aeInfo.stAERouteEx.astRouteExNode[i].u32IntTime !=
				aeRouteEx.astRouteExNode[i].u32IntTime) ||
				(aeInfo.stAERouteEx.astRouteExNode[i].u32Again !=
				aeRouteEx.astRouteExNode[i].u32Again) ||
				(aeInfo.stAERouteEx.astRouteExNode[i].u32Dgain !=
				aeRouteEx.astRouteExNode[i].u32Dgain) ||
				(aeInfo.stAERouteEx.astRouteExNode[i].u32IspDgain !=
				aeRouteEx.astRouteExNode[i].u32IspDgain)) {
				printf("NG Node:%d\n", i);
				printf("Time:%u %u AGain:%u %u DGain:%u %u ISPDGain:%u %u\n",
					aeInfo.stAERouteEx.astRouteExNode[i].u32IntTime,
					aeRouteEx.astRouteExNode[i].u32IntTime,
					aeInfo.stAERouteEx.astRouteExNode[i].u32Again,
					aeRouteEx.astRouteExNode[i].u32Again,
					aeInfo.stAERouteEx.astRouteExNode[i].u32Dgain,
					aeRouteEx.astRouteExNode[i].u32Dgain,
					aeInfo.stAERouteEx.astRouteExNode[i].u32IspDgain,
					aeRouteEx.astRouteExNode[i].u32IspDgain);
					result = CVI_FAILURE;
					break;
			}
		}
	}

	aeAttr.bAERouteExValid = 0;
	CVI_ISP_SetExposureAttr(ViPipe, &aeAttr);
	AE_Delay1ms(delayTime);
	printf("routeEx result:%d\n", result);
	return result;
}

CVI_BOOL CVI_ISP_SetWDRExposureAttr_Test(VI_PIPE ViPipe)
{
	ISP_EXP_INFO_S aeInfo;
	ISP_WDR_EXPOSURE_ATTR_S wdrAttr;
	CVI_BOOL result = CVI_SUCCESS;
	CVI_U8	sID;
	CVI_U32 delayTime;
	CVI_U8	delayFrameNum;

	printf("Func : %s\n", __func__);

	sID = AE_ViPipe2sID(ViPipe);
	delayFrameNum = AE_GetMeterPeriod(sID) * 2;
	AE_GetExposureDelayTime(ViPipe, delayFrameNum, &delayTime);

	CVI_ISP_GetWDRExposureAttr(ViPipe, &wdrAttr);
	wdrAttr.enExpRatioType = OP_TYPE_MANUAL;
	wdrAttr.au32ExpRatio[0] = 16 * AE_WDR_RATIO_BASE;
	CVI_ISP_SetWDRExposureAttr(ViPipe, &wdrAttr);
	AE_Delay1ms(delayTime);
	CVI_ISP_QueryExposureInfo(ViPipe, &aeInfo);
	if (aeInfo.u32WDRExpRatio < wdrAttr.au32ExpRatio[0] - AE_WDR_RATIO_BASE / 2 &&
		aeInfo.u32WDRExpRatio > wdrAttr.au32ExpRatio[0] + AE_WDR_RATIO_BASE / 2) {
		result = CVI_FAILURE;
		printf("WDR NG Ratio:%u %u(%u - %u)\n", wdrAttr.au32ExpRatio[0],
				aeInfo.u32WDRExpRatio - AE_WDR_RATIO_BASE,
				aeInfo.u32WDRExpRatio - AE_WDR_RATIO_BASE,
				aeInfo.u32WDRExpRatio + AE_WDR_RATIO_BASE);
	}

	wdrAttr.enExpRatioType = OP_TYPE_AUTO;
	wdrAttr.u32ExpRatioMin = 8 * AE_WDR_RATIO_BASE - AE_WDR_RATIO_BASE / 2;
	wdrAttr.u32ExpRatioMax = 8 * AE_WDR_RATIO_BASE + AE_WDR_RATIO_BASE / 2;
	CVI_ISP_SetWDRExposureAttr(ViPipe, &wdrAttr);
	AE_Delay1ms(delayTime);
	CVI_ISP_QueryExposureInfo(ViPipe, &aeInfo);
	if (aeInfo.u32WDRExpRatio < wdrAttr.u32ExpRatioMin &&
		aeInfo.u32WDRExpRatio > wdrAttr.u32ExpRatioMax) {
		result = CVI_FAILURE;
		printf("WDR NG Ratio:%u %u-%u\n", aeInfo.u32WDRExpRatio,
				wdrAttr.u32ExpRatioMin, wdrAttr.u32ExpRatioMax);
	}
	printf("WDR result:%d\n", result);
	return result;
}


CVI_BOOL CVI_ISP_SetExposureAttr_Test(VI_PIPE ViPipe)
{
	printf("Func : %s\n", __func__);
	if (AE_ExposureAttr_ByPss_Test(ViPipe))
		return CVI_FAILURE;
	if (AE_ExposureAttr_ManualExp_Test(ViPipe))
		return CVI_FAILURE;
	if (AE_ExpsoureAttr_ExpRange_Test(ViPipe))
		return CVI_FAILURE;
	if (AE_ExpsoureAttr_AutoParam_Test(ViPipe))
		return CVI_FAILURE;
	return CVI_SUCCESS;
}

void CVI_AE_AutoTest(VI_PIPE ViPipe)
{
	CVI_U8 sID;

	sID = AE_ViPipe2sID(ViPipe);

	if (CVI_ISP_SetExposureAttr_Test(ViPipe)) {
		printf("CVI_ISP_SetExposureAttr_Test fail!\n");
		goto EXIT;
	}
	if (CVI_ISP_SetAEStatisticsConfig_Test(ViPipe)) {
		printf("CVI_ISP_SetAEWinStatistics_Test fail!\n");
		goto EXIT;
	}
	if (CVI_ISP_SetAERouteAttr_Test(ViPipe)) {
		printf("CVI_ISP_SetAERouteAttr_Test fail!\n");
		goto EXIT;
	}
	if (CVI_ISP_SetAERouteAttrEx_Test(ViPipe)) {
		printf("CVI_ISP_SetAERouteAttrEx_Test fail!\n");
		goto EXIT;
	}
	if (AE_IsWDRMode(sID) && CVI_ISP_SetWDRExposureAttr_Test(ViPipe)) {
		printf("CVI_ISP_SetWDRExposureAttr_Test fail!\n");
		goto EXIT;
	}
	printf("*---------  %s success   ---------*\n", __func__);
EXIT:
	AE_Function_Init(sID);
}

sEV_S aeb_ev[AE_TEST_RAW_NUM] = {
	{EVTT_ENTRY_1_30SEC, ISO_25600_Entry },//512,800
	{EVTT_ENTRY_1_30SEC, ISO_6400_Entry },//512,600
	{EVTT_ENTRY_1_30SEC, ISO_1600_Entry },//512,400
	{EVTT_ENTRY_1_30SEC, ISO_400_Entry  },//512,200
	{EVTT_ENTRY_1_30SEC, ISO_100_Entry  },//512,0
	{EVTT_ENTRY_1_120SEC, ISO_100_Entry  },//712,0
	{EVTT_ENTRY_1_480SEC, ISO_100_Entry  },//912,0
	{EVTT_ENTRY_1_1920SEC, ISO_100_Entry  },//1112,0
	{EVTT_ENTRY_1_7680SEC, ISO_100_Entry  },//1312,0
	{EVTT_ENTRY_1_30720SEC, ISO_100_Entry  },//1512,0
};



void AE_SetManualExposureTest(CVI_U8 sID, CVI_U8 mode, CVI_S32 expTime, CVI_S32 ISONum)
{
	ISP_EXPOSURE_ATTR_S expAttr = { 0 };
	AE_EXPOSURE	sensorExp[AE_MAX_WDR_FRAME_NUM];
	CVI_BOOL useHistogram = 0;
	SAE_INFO *tmpAeInfo;
	CVI_U16 luma[AE_MAX_WDR_FRAME_NUM];

	sID = AE_CheckSensorID(sID);
	AE_GetExposureAttr(sID, &expAttr);
	expAttr.u8DebugMode = 0;
	AE_GetCurrentInfo(sID, &tmpAeInfo);
	useHistogram = (tmpAeInfo->enLumaBase == HISTOGRAM_BASE) ? 1 : 0;

	if (mode == 0) {
		expAttr.bByPass = 1;
		printf("AE byPass!\n");
	} else if (mode == 1) {
		expAttr.bByPass = 0;
		expAttr.enOpType = OP_TYPE_AUTO;
		expAttr.stManual.enExpTimeOpType = OP_TYPE_AUTO;
		expAttr.stManual.enISONumOpType = OP_TYPE_AUTO;
		printf("AE Auto!\n");
	} else if (mode == 2) {
		expAttr.bByPass = 0;
		expAttr.enOpType = OP_TYPE_MANUAL;
		expAttr.stManual.enExpTimeOpType = OP_TYPE_MANUAL;
		expAttr.stManual.enISONumOpType = OP_TYPE_MANUAL;
		expAttr.stManual.u32ExpTime = expTime;
		expAttr.stManual.enGainType = AE_TYPE_ISO;
		expAttr.stManual.u32ISONum = ISONum;
		printf("AE Manual!\n");
	}

	AE_SetExposureAttr(sID, &expAttr);
	AE_Delay1ms(300);
	AE_GetSensorExposureSetting(sID, sensorExp);
	luma[AE_LE] = (useHistogram) ? AE_GetBrightQuaG(sID, AE_LE) : AE_GetCenterG(sID, AE_LE);
	luma[AE_SE] = (useHistogram) ? AE_GetBrightQuaG(sID, AE_SE) : AE_GetCenterG(sID, AE_SE);

	printf("time:%u iso:%u AeL:%u AeS:%u\n", expAttr.stManual.u32ExpTime,
		expAttr.stManual.u32ISONum, luma[AE_LE], luma[AE_SE]);
	printf("sensor LEexpT:%u LEexpL:%u SEexpT:%u SEexpL:%u AG:%u DG:%u IG:%u\n",
			sensorExp[AE_LE].u32SensorExpTime, sensorExp[AE_LE].u32SensorExpTimeLine,
			sensorExp[AE_SE].u32SensorExpTime, sensorExp[AE_SE].u32SensorExpTimeLine,
			sensorExp[AE_LE].stSensorExpGain.u32AGain, sensorExp[AE_LE].stSensorExpGain.u32DGain,
			sensorExp[AE_LE].stSensorExpGain.u32ISPDGain);
}

void AE_SetManualGainTest(CVI_U8 sID, CVI_U32 again, CVI_U32 dgain, CVI_U32 ispDgain)
{
	ISP_EXPOSURE_ATTR_S expAttr = { 0 };
	AE_EXPOSURE sensorExp[AE_MAX_WDR_FRAME_NUM];
	SAE_INFO *tmpAeInfo;
	CVI_U16 luma[AE_MAX_WDR_FRAME_NUM];
	CVI_BOOL useHistogram = 0;

	sID = AE_CheckSensorID(sID);
	AE_GetExposureAttr(sID, &expAttr);
	AE_GetCurrentInfo(sID, &tmpAeInfo);
	useHistogram = (tmpAeInfo->enLumaBase == HISTOGRAM_BASE) ? 1 : 0;
	expAttr.bByPass = 0;
	expAttr.u8DebugMode = 0;
	expAttr.enOpType = OP_TYPE_MANUAL;
	expAttr.stManual.enGainType = AE_TYPE_GAIN;
	expAttr.stManual.enAGainOpType = OP_TYPE_MANUAL;
	expAttr.stManual.enDGainOpType = OP_TYPE_MANUAL;
	expAttr.stManual.enISPDGainOpType = OP_TYPE_MANUAL;
	expAttr.stManual.u32AGain = again;
	expAttr.stManual.u32DGain = dgain;
	expAttr.stManual.u32ISPDGain = ispDgain;

	AE_SetExposureAttr(sID, &expAttr);
	AE_Delay1ms(300);
	AE_GetSensorExposureSetting(sID, sensorExp);
	luma[AE_LE] = (useHistogram) ? AE_GetBrightQuaG(sID, AE_LE) : AE_GetCenterG(sID, AE_LE);
	luma[AE_SE] = (useHistogram) ? AE_GetBrightQuaG(sID, AE_SE) : AE_GetCenterG(sID, AE_SE);

	printf("AG:%u DG:%u IG:%u AeL:%u AeS:%u\n", expAttr.stManual.u32AGain,
		expAttr.stManual.u32DGain, expAttr.stManual.u32ISPDGain, luma[AE_LE], luma[AE_SE]);
	printf("sensor LEexpL:%u SEexpL:%u AG:%u DG:%u IG:%u\n",
			sensorExp[AE_LE].u32SensorExpTimeLine, sensorExp[AE_SE].u32SensorExpTimeLine,
			sensorExp[AE_LE].stSensorExpGain.u32AGain, sensorExp[AE_LE].stSensorExpGain.u32DGain,
			sensorExp[AE_LE].stSensorExpGain.u32ISPDGain);
}

#define CONSOLE_NONE       "\033[0m"
#define CONSOLE_RED        "\033[1;31m"
#define CONSOLE_YELLOW     "\033[1;33m"
#define CONSOLE_GREEN      "\033[1;32m"




//#define ENABLE_AE_DEBUG


#ifdef ENABLE_AE_DEBUG

void AE_ShowHistogram(const SAE_INFO *pstAeInfo)
{
	CVI_U32 histogramTotalCnt = 0;
	CVI_U16 i, j;

	printf("histogram:\n");
	for (j = 0; j < pstAeInfo->u8AeMaxFrameNum; j++) {
		histogramTotalCnt = 0;
		printf("F:%d\n", j);
		for (i = 0; i < HIST_NUM; i++) {
			histogramTotalCnt += pstAeInfo->pu32Histogram[j][i];
			printf("%u\n", pstAeInfo->pu32Histogram[j][i]);
		}
		printf("Tatal:%u\n", histogramTotalCnt);
	}
}

void AE_ShowGridRGB(const SAE_INFO *pstAeInfo, CVI_BOOL toFile)
{
	CVI_U16 i, j, k;
	FILE *fp;

	printf("V:%d H:%d\n", pstAeInfo->u8GridVNum, pstAeInfo->u8GridHNum);
	for (k = 0; k < pstAeInfo->u8AeMaxFrameNum; k++) {
		printf("F:%d\n", k);
		printf("R:\n");
		for (i = 0; i < pstAeInfo->u8GridVNum; i++) {
			for (j = 0; j < pstAeInfo->u8GridHNum; j++)
				printf("%d ", pstAeInfo->pu16AeStatistics[k][i][j][3]);
			printf("\n");
		}
		printf("\n");
		printf("GR:\n");
		for (i = 0; i < pstAeInfo->u8GridVNum; i++) {
			for (j = 0; j < pstAeInfo->u8GridHNum; j++)
				printf("%d ", pstAeInfo->pu16AeStatistics[k][i][j][2]);
			printf("\n");
		}
		printf("GB:\n");
		for (i = 0; i < pstAeInfo->u8GridVNum; i++) {
			for (j = 0; j < pstAeInfo->u8GridHNum; j++)
				printf("%d ", pstAeInfo->pu16AeStatistics[k][i][j][1]);
			printf("\n");
		}
		printf("\n");
		printf("B:\n");
		for (i = 0; i < pstAeInfo->u8GridVNum; i++) {
			for (j = 0; j < pstAeInfo->u8GridHNum; j++)
				printf("%d ", pstAeInfo->pu16AeStatistics[k][i][j][0]);
			printf("\n");
		}
		printf("\n");
	}
#ifndef ARCH_RTOS_CV181X // TODO@CV181X
	if (toFile) {
		fp = fopen("AeGridLuma.txt", "w");
		if (!fp) {
			printf("file open error!\n");
			return;
		}
		fprintf(fp, "V:%d H:%d\n", pstAeInfo->u8GridVNum, pstAeInfo->u8GridHNum);
		for (k = 0; k < pstAeInfo->u8AeMaxFrameNum; k++) {
			fprintf(fp, "R:\n");
			for (i = 0; i < pstAeInfo->u8GridVNum; i++) {
				for (j = 0; j < pstAeInfo->u8GridHNum; j++)
					fprintf(fp, "%d\t", pstAeInfo->pu16AeStatistics[k][i][j][3]);
				fprintf(fp, "\n");
			}
			fprintf(fp, "\n");
			fprintf(fp, "GR:\n");
			for (i = 0; i < pstAeInfo->u8GridVNum; i++) {
				for (j = 0; j < pstAeInfo->u8GridHNum; j++)
					fprintf(fp, "%d\t", pstAeInfo->pu16AeStatistics[k][i][j][2]);
				fprintf(fp, "\n");
			}
			fprintf(fp, "\n");
			fprintf(fp, "GB:\n");
			for (i = 0; i < pstAeInfo->u8GridVNum; i++) {
				for (j = 0; j < pstAeInfo->u8GridHNum; j++)
					fprintf(fp, "%d\t", pstAeInfo->pu16AeStatistics[k][i][j][1]);
				fprintf(fp, "\n");
			}
			fprintf(fp, "\n");
			fprintf(fp, "B:\n");
			for (i = 0; i < pstAeInfo->u8GridVNum; i++) {
				for (j = 0; j < pstAeInfo->u8GridHNum; j++)
					fprintf(fp, "%d\t", pstAeInfo->pu16AeStatistics[k][i][j][0]);
				fprintf(fp, "\n");
			}
			fprintf(fp, "\n");
		}
		fclose(fp);
	}
#endif
}

void AE_ShowMeterLuma(const SAE_INFO *pstAeInfo, CVI_BOOL toFile)
{
	CVI_U16 i, j, k;
	FILE *fp;

	printf("AE:%dx%d\n", pstAeInfo->u8GridVNum, pstAeInfo->u8GridHNum);
	for (k = 0; k < pstAeInfo->u8AeMaxFrameNum; k++) {
		printf("%d\n", k);
		for (i = 0; i < pstAeInfo->u8GridVNum; i++) {
			for (j = 0; j < pstAeInfo->u8GridHNum; j++)
				printf("%d ", pstAeInfo->u16MeterLuma[k][i][j]);
			printf("\n");
		}
		printf("FL:%d\n", pstAeInfo->u16FrameLuma[k]);
		printf("\n");
	}

	printf("\n");
#ifndef ARCH_RTOS_CV181X // TODO@CV181X
	if (toFile) {
		fp = fopen("AeMeterLuma.txt", "w");
		if (!fp) {
			printf("file open error!\n");
			return;
		}

		for (k = 0; k < pstAeInfo->u8AeMaxFrameNum; k++) {
			for (i = 0; i < pstAeInfo->u8GridVNum; i++) {
				for (j = 0; j < pstAeInfo->u8GridHNum; j++)
					fprintf(fp, "%d\t", pstAeInfo->u16MeterLuma[k][i][j]);
				fprintf(fp, "\n");
			}
		}

		fprintf(fp, "\n");
		fclose(fp);
		printf("save finish!\n");
	}
#endif
}

void AE_ShowCurrentInfo(CVI_U8 sID, const SAE_INFO *pstAeInfo)
{
	ISP_EXPOSURE_ATTR_S tmpAe;
	CVI_U8 i;

	for (i = 0; i < pstAeInfo->u8AeMaxFrameNum; ++i) {
		printf("\n\n%d\n", i);
		printf("WdrLE:%d FL:%u WDRSeMinTvEntry:%d fps:%f\n", pstAeInfo->bWDRLEOnly,
				pstAeInfo->u32FrameLine, pstAeInfo->s16WDRSEMinTVEntry, pstAeInfo->fFps);
		printf("target:%d %d\n", pstAeInfo->u16TargetLuma[i], pstAeInfo->u16AdjustTargetLuma[i]);
		printf("Luma:%d %d P:%d\n", pstAeInfo->u16FrameLuma[i],
							pstAeInfo->u8MeterGridCnt[i], pstAeInfo->u8MeterFramePeriod);
		printf("Assign Tv:%d Sv:%d\n", pstAeInfo->stAssignApex[i].s16TVEntry,
		       pstAeInfo->stAssignApex[i].s16SVEntry);
		printf("EvBias:%d LvX100:%d\n", pstAeInfo->s16AssignEVBIAS[i], pstAeInfo->s16LvX100[i]);
		printf("Bv:%d %d %d Bs:%d LvBs:%d IS:%d ME:%d\n", pstAeInfo->stApex[i].s16BVEntry,
		       pstAeInfo->stApex[i].s16TVEntry, pstAeInfo->stApex[i].s16SVEntry,
		       pstAeInfo->s16BvStepEntry[i], pstAeInfo->s16LvBvStep[i],
		       pstAeInfo->bIsStable[i], pstAeInfo->bIsMaxExposure);
		printf("TvEntry m:%d M:%d\n", pstAeInfo->s16MinTVEntry, pstAeInfo->s16MaxTVEntry);
		printf("ISOEntry m:%d M:%d\n", pstAeInfo->s16MinISOEntry, pstAeInfo->s16MaxISOEntry);
		printf("ISONum:%u\n", pstAeInfo->u32ISONum[i]);
		printf("firstStableT:%u\n", pstAeInfo->u32FirstStableTime);
		printf("luma m:%u M:%u\n", pstAeInfo->u16MeterMinLuma[i], pstAeInfo->u16MeterMaxLuma[i]);
		printf("Init Tv:%d Sv:%d\n", pstAeInfo->stInitApex[i].s16TVEntry, pstAeInfo->stInitApex[i].s16SVEntry);

	}

	printf("AGain m:%u M:%u DGain m:%u M:%u\n", pstAeInfo->u32AGainMin, pstAeInfo->u32AGainMax,
							pstAeInfo->u32DGainMin, pstAeInfo->u32DGainMax);

	if (pstAeInfo->bWDRMode)
		printf("WDR R:%u DR:%u-%u\n", pstAeInfo->u32WDRExpRatio, pstAeInfo->u16MeterMinLuma[AE_LE],
		       (CVI_U32)pstAeInfo->u16MeterMaxLuma[AE_SE] * pstAeInfo->u32WDRExpRatio);

	AE_GetExposureAttr(sID, &tmpAe);
	if (tmpAe.enOpType == OP_TYPE_MANUAL)
		printf("T:%u AG:%uud DG:%u IG:%u\n", tmpAe.stManual.u32ExpTime, tmpAe.stManual.u32AGain,
		       tmpAe.stManual.u32DGain, tmpAe.stManual.u32ISPDGain);
}


void AE_SavePGMFile(CVI_U8 sID)
{
#ifndef ARCH_RTOS_CV181X // TODO@CV181X
#define MAGIC "P2"
#define WIDTH 640
#define HEIGHT 480
#define MAX_V 255

	FILE *fp;

	fp = fopen("AE.PGM", "w");
	if (fp == 0) {
		printf("open file fail sid :%d!\n", sID);
		return;
	}
	fprintf(fp, "%s\n", MAGIC);
	fprintf(fp, "%d %d\n", WIDTH, HEIGHT);
	fprintf(fp, "%d\n", MAX_V);

	fclose(fp);
#else
	UNUSED(sID);
#endif
}

void AE_ShowRoute(CVI_U8 sID, CVI_S32 param)
{
	CVI_S16 minBvEntry, maxBvEntry, i, j;
	AE_APEX LeExp, SeExp, tmpLeExp;
	CVI_U32 LeExpTime, SeExpTime, LeExpLine, SeExpLine;
	CVI_S16 wdrDiff;
	AE_GAIN expGain;
	ISP_EXPOSURE_ATTR_S expAttr;

	AE_GetExposureAttr(sID, &expAttr);
	expAttr.bByPass = 1;
	if (param % 2 == 1)
		expAttr.bAERouteExValid = 1;
	AE_SetExposureAttr(sID, &expAttr);
	AE_Delay1ms(300);
	AE_GetBVEntryLimit(sID, &minBvEntry, &maxBvEntry);
	if (param >= 2) {
		for (i = maxBvEntry - 2 * ENTRY_PER_EV; i >= minBvEntry; --i) {
			LeExp.s16BVEntry = i;
			AE_GetRouteTVAVSVEntry(sID, AE_LE, LeExp.s16BVEntry, &LeExp.s16TVEntry,
				&LeExp.s16AVEntry, &LeExp.s16SVEntry);
			printf("\nLE bv:%d tv:%d sv:%d\n", LeExp.s16BVEntry, LeExp.s16TVEntry, LeExp.s16SVEntry);
			for (j = 2 * ENTRY_PER_EV; j <= 5 * ENTRY_PER_EV; ++j) {
				if (i + j > maxBvEntry)
					continue;
				tmpLeExp = LeExp;
				SeExp.s16BVEntry = i + j;
				AE_GetRouteTVAVSVEntry(sID, AE_SE, SeExp.s16BVEntry, &SeExp.s16TVEntry,
					&SeExp.s16AVEntry, &SeExp.s16SVEntry);
				AE_GetWDRLESEEntry(sID, &tmpLeExp, &SeExp);
				wdrDiff = (SeExp.s16TVEntry + SeExp.s16AVEntry - SeExp.s16SVEntry) -
							(LeExp.s16TVEntry + LeExp.s16AVEntry - LeExp.s16SVEntry);
				AE_SetAPEXExposure(sID, AE_LE, &LeExp);
				AE_SetAPEXExposure(sID, AE_SE, &SeExp);
				AE_ConfigExposure(sID);
				AE_GetAPEXExposure(sID, AE_LE, &LeExp);
				AE_GetAPEXExposure(sID, AE_LE, &SeExp);
				AE_GetExpTimeByEntry(sID, LeExp.s16TVEntry, &LeExpTime);
				AE_GetExpTimeByEntry(sID, SeExp.s16TVEntry, &SeExpTime);
				AE_GetExposureTimeLine(sID, LeExpTime, &LeExpLine);
				AE_GetExposureTimeLine(sID, SeExpTime, &SeExpLine);
				if (wdrDiff == j) {
					printf("LE bv:%d tv:%d sv:%d SE bv:%d tv:%d sv:%d Diff:%d ---- OK\n",
					tmpLeExp.s16BVEntry, tmpLeExp.s16TVEntry, tmpLeExp.s16SVEntry,
					SeExp.s16BVEntry, SeExp.s16TVEntry, SeExp.s16SVEntry, wdrDiff);
					if (LeExpLine != SeExpLine)
						printf("LE T:%u L:%u SE T:%u L:%u\n", LeExpTime, LeExpLine,
								SeExpTime, SeExpLine);
				} else {
					printf("LE bv:%d tv:%d sv:%d SE bv:%d tv:%d sv:%d Diff:%d %d-- NG\n",
						tmpLeExp.s16BVEntry, tmpLeExp.s16TVEntry, tmpLeExp.s16SVEntry,
						SeExp.s16BVEntry, SeExp.s16TVEntry, SeExp.s16SVEntry, wdrDiff, j);
					if (LeExpLine != SeExpLine)
						printf("LE T:%u L:%u SE T:%u L:%u\n", LeExpTime, LeExpLine,
								SeExpTime, SeExpLine);
				}
			}
			AE_Delay1ms(5);
		}
	} else {
		if (expAttr.bAERouteExValid)
			printf("routeEx!\n");
		else
			printf("route!\n");
		for (i = maxBvEntry; i >= minBvEntry; --i) {
			LeExp.s16BVEntry = i;
			AE_GetRouteTVAVSVEntry(sID, AE_LE, LeExp.s16BVEntry, &LeExp.s16TVEntry,
				&LeExp.s16AVEntry, &LeExp.s16SVEntry);
			AE_SetAPEXExposure(sID, AE_LE, &LeExp);
			AE_ConfigExposure(sID);
			AE_GetAPEXExposure(sID, AE_LE, &LeExp);
			AE_GetExpGainInfo(sID, AE_LE, &expGain);
			AE_GetExpTimeByEntry(sID, LeExp.s16TVEntry, &LeExpTime);
			AE_GetExposureTimeLine(sID, LeExpTime, &LeExpLine);
			if (LeExp.s16BVEntry == LeExp.s16TVEntry - LeExp.s16SVEntry)
				printf("%d : %d(%u) %d %d A:%u D:%u I:%u ----- OK\n", LeExp.s16BVEntry,
					LeExp.s16TVEntry, LeExpLine, LeExp.s16AVEntry, LeExp.s16SVEntry,
					expGain.u32AGain, expGain.u32DGain, expGain.u32ISPDGain);
			else
				printf("%d : %d(%u) %d %d A:%u D:%u I:%u -- NG\n", LeExp.s16BVEntry,
					LeExp.s16TVEntry, LeExpLine, LeExp.s16AVEntry, LeExp.s16SVEntry,
					expGain.u32AGain, expGain.u32DGain, expGain.u32ISPDGain);
		}

		if (AE_IsWDRMode(sID)) {
			AE_Delay1ms(5);
			printf("\n\nSE route!\n");
			for (i = maxBvEntry; i >= minBvEntry; --i) {
				SeExp.s16BVEntry = i;
				AE_GetRouteTVAVSVEntry(sID, AE_SE, SeExp.s16BVEntry, &SeExp.s16TVEntry,
					&SeExp.s16AVEntry, &SeExp.s16SVEntry);
				AE_SetAPEXExposure(sID, AE_SE, &SeExp);
				AE_ConfigExposure(sID);
				AE_GetAPEXExposure(sID, AE_SE, &SeExp);
				AE_GetExpGainInfo(sID, AE_SE, &expGain);
				AE_GetExpTimeByEntry(sID, SeExp.s16TVEntry, &SeExpTime);
				AE_GetExposureTimeLine(sID, SeExpTime, &SeExpLine);
				if (SeExp.s16BVEntry == SeExp.s16TVEntry - SeExp.s16SVEntry)
					printf("%d : %d(%u) %d %d A:%u D:%u I:%u ----- OK\n", SeExp.s16BVEntry,
						SeExp.s16TVEntry, SeExpLine, SeExp.s16AVEntry, SeExp.s16SVEntry,
						expGain.u32AGain, expGain.u32DGain, expGain.u32ISPDGain);
				else
					printf("%d : %d(%u) %d %d A:%u D:%u I:%u -- NG\n", SeExp.s16BVEntry,
						SeExp.s16TVEntry, SeExpLine, SeExp.s16AVEntry, SeExp.s16SVEntry,
						expGain.u32AGain, expGain.u32DGain, expGain.u32ISPDGain);
			}
		}
	}
	AE_GetExposureAttr(sID, &expAttr);
	expAttr.bByPass = 0;
	expAttr.bAERouteExValid = 0;
	AE_SetExposureAttr(sID, &expAttr);
}

void AE_ShowExpLine(CVI_U8 sID, const SAE_INFO *pstAeInfo)
{
	CVI_S16 i;
	CVI_U32 line, SHS, ispDgain, timeThr;
	CVI_FLOAT idealLine, time;
	CVI_S16 entryThr = TV_ENTRY_WITH_ISPDGAIN_COMPENSATION;

	AE_GetExpTimeByEntry(sID, entryThr, &timeThr);
	printf("timeThr:%u\n", timeThr);
	for (i = pstAeInfo->s16MaxSensorTVEntry; i >=
		pstAeInfo->s16MinSensorTVEntry; --i) {
		ispDgain = AE_GAIN_BASE;
		AE_GetIdealExpTimeByEntry(sID, i, &time);
		AE_GetExposureTimeIdealLine(sID, time, &idealLine);
		line = (CVI_U32)idealLine;
		SHS = pstAeInfo->u32FrameLine - line - 1;
		if (time < timeThr)
			ispDgain = ispDgain * idealLine / line;
		printf("Entry:%d Time:%f iLine:%f Line:%u IG:%u SHS:%u\n",
			i, time, idealLine, line, ispDgain, SHS);
		if (i < entryThr - ENTRY_PER_EV)
			break;
	}
}

void AE_GetLimitGain(CVI_U8 sID, AE_GAIN *pstGain)
{
	AE_GAIN limitGain, tmpGain;
	CVI_U32 AGainMin, AGainMax, DGainMin, DGainMax, ISPDGainMin, ISPDGainMax;
	CVI_S16 i;
	CVI_U64 oriTotalGain;

	ISP_EXPOSURE_ATTR_S tmpAeAttr;
	SAE_INFO *tmpAeInfo;

	AE_GetExposureAttr(sID, &tmpAeAttr);
	AE_GetCurrentInfo(sID, &tmpAeInfo);

	AGainMin = DGainMin = ISPDGainMin = AE_GAIN_BASE;
	AGainMax = tmpAeAttr.stAuto.stAGainRange.u32Max;
	DGainMax = tmpAeAttr.stAuto.stDGainRange.u32Max;
	ISPDGainMax = tmpAeAttr.stAuto.stISPDGainRange.u32Max;

	limitGain = *pstGain;
	oriTotalGain = (CVI_U64)pstGain->u32AGain * pstGain->u32DGain * pstGain->u32ISPDGain;

	AAA_LIMIT(limitGain.u32AGain, AGainMin, AGainMax);
	AAA_LIMIT(limitGain.u32DGain, DGainMin, DGainMax);
	AAA_LIMIT(limitGain.u32ISPDGain, ISPDGainMin, ISPDGainMax);

	if (limitGain.u32AGain == pstGain->u32AGain && limitGain.u32DGain == pstGain->u32DGain &&
		limitGain.u32ISPDGain == pstGain->u32ISPDGain)
		return;

	if (tmpAeInfo->u8DGainAccuType == AE_ACCURACY_DB) {
		tmpGain.u32AGain = oriTotalGain / (AE_GAIN_BASE * AE_GAIN_BASE);
		tmpGain.u32DGain = tmpGain.u32ISPDGain = AE_GAIN_BASE;

		limitGain.u32AGain = tmpGain.u32AGain;
		AAA_LIMIT(limitGain.u32AGain, AGainMin, AGainMax);

		limitGain.u32DGain = (CVI_U64)tmpGain.u32AGain * tmpGain.u32DGain / limitGain.u32AGain;

		for (i = tmpAeInfo->u8SensorDgainNodeNum-1; i >= 0 ; --i) {
			if (limitGain.u32DGain >= tmpAeInfo->u32SensorDgainNode[i]) {
				limitGain.u32DGain = tmpAeInfo->u32SensorDgainNode[i];
				break;
			}
		}
		AAA_LIMIT(limitGain.u32DGain, DGainMin, DGainMax);
		#if 0
		printf("%d %d %d %d %d\n", tmpGain.u32AGain, tmpGain.u32DGain, tmpGain.u32ISPDGain,
					limitGain.u32AGain, limitGain.u32DGain);
		#endif
		limitGain.u32ISPDGain = (CVI_U64)tmpGain.u32AGain * tmpGain.u32DGain * tmpGain.u32ISPDGain /
								((CVI_U64)limitGain.u32AGain * limitGain.u32DGain);
		AAA_LIMIT(limitGain.u32ISPDGain, ISPDGainMin, ISPDGainMax);
	} else {
		if (limitGain.u32AGain != pstGain->u32AGain) {//only min DGain when limit AGain
			limitGain.u32DGain = pstGain->u32DGain = AE_GAIN_BASE;
			limitGain.u32ISPDGain = (CVI_U64)pstGain->u32AGain * pstGain->u32ISPDGain / limitGain.u32AGain;
		} else if (limitGain.u32DGain != pstGain->u32DGain) {//only max AGain when limit DGain
			limitGain.u32AGain = pstGain->u32AGain = tmpAeInfo->u32AGainMax;
			limitGain.u32ISPDGain = (CVI_U64)pstGain->u32DGain * pstGain->u32ISPDGain / limitGain.u32DGain;
		}
		AAA_LIMIT(limitGain.u32ISPDGain, ISPDGainMin, ISPDGainMax);
	}

	AAA_LIMIT(limitGain.u32AGain, AE_GAIN_BASE, tmpAeInfo->u32AGainMax);
	AAA_LIMIT(limitGain.u32DGain, AE_GAIN_BASE, tmpAeInfo->u32DGainMax);
	AAA_LIMIT(limitGain.u32ISPDGain, AE_GAIN_BASE, tmpAeInfo->u32ISPDGainMax);

	if (AE_GetDebugMode(sID) == 35) {
		printf("max:%u %u %u\n", tmpAeAttr.stAuto.stAGainRange.u32Max,
				tmpAeAttr.stAuto.stDGainRange.u32Max,
				tmpAeAttr.stAuto.stISPDGainRange.u32Max);
		printf("ori %u %u %u\n", pstGain->u32AGain, pstGain->u32DGain, pstGain->u32ISPDGain);
		printf("new %u %u %u\n", limitGain.u32AGain, limitGain.u32DGain, limitGain.u32ISPDGain);
	}

	*pstGain = limitGain;

}

void AE_SetAuto(CVI_U8 sID)
{
	ISP_EXPOSURE_ATTR_S tmpExpAttr;

	AE_GetExposureAttr(sID, &tmpExpAttr);
	tmpExpAttr.enOpType = OP_TYPE_AUTO;
	tmpExpAttr.stManual.enExpTimeOpType = OP_TYPE_AUTO;
	tmpExpAttr.stManual.enAGainOpType = OP_TYPE_AUTO;
	tmpExpAttr.stManual.enAGainOpType = OP_TYPE_AUTO;
	tmpExpAttr.stManual.enISPDGainOpType = OP_TYPE_AUTO;
	AE_SetExposureAttr(sID, &tmpExpAttr);
}


void AE_SetAssignEvBias(CVI_U8 sID, CVI_S16 EvBias)
{
	ISP_EXPOSURE_ATTR_S tmpAttr;

	sID = AE_CheckSensorID(sID);
	AE_GetExposureAttr(sID, &tmpAttr);
	tmpAttr.stAuto.u16EVBias = EvBias;
	AE_SetExposureAttr(sID, &tmpAttr);
}

void AE_SetConvergeSpeed(CVI_U8 sID, CVI_U8 speed, CVI_U16 b2dSpeed)
{
	ISP_EXPOSURE_ATTR_S tmpAttr;

	sID = AE_CheckSensorID(sID);
	AE_GetExposureAttr(sID, &tmpAttr);
	tmpAttr.stAuto.u16BlackSpeedBias = b2dSpeed;
	tmpAttr.stAuto.u8Speed = speed;
	AE_SetExposureAttr(sID, &tmpAttr);
}

void AE_SetByPass(CVI_U8 sID, CVI_U8 enable)
{
	ISP_EXPOSURE_ATTR_S tmpAe;

	sID = AE_CheckSensorID(sID);
	AE_GetExposureAttr(sID, &tmpAe);
	tmpAe.bByPass = enable;
	AE_SetExposureAttr(sID, &tmpAe);
}

void AE_ShowConvergeStatus(CVI_U8 sID)
{
	CVI_U8 i, j;
	SAE_INFO *tmpAeInfo;

	static CVI_BOOL preStable[AE_SENSOR_NUM][AE_MAX_WDR_FRAME_NUM];


	sID = AE_CheckSensorID(sID);
	AE_GetCurrentInfo(sID, &tmpAeInfo);

	for (i = 0; i < tmpAeInfo->u8AeMaxFrameNum; ++i) {
		if (!tmpAeInfo->bIsStable[i] || !preStable[sID][i]) {
			printf("%d\n", i);
			printf("fid:%u L:%u TL:%u\n", tmpAeInfo->u32frmCnt, tmpAeInfo->u16FrameLuma[i],
			       tmpAeInfo->u16AdjustTargetLuma[i]);
			printf("DY:%u\n", tmpAeInfo->u16FrameLuma[i] - tmpAeInfo->u16AdjustTargetLuma[i]);
			printf("Bv:%d pBv:%d Bs:%d cBs:%d s:%d cs:%u fc:%u\n", tmpAeInfo->stApex[i].s16BVEntry,
					tmpAeInfo->s16PreBvEntry[i], tmpAeInfo->s16BvStepEntry[i],
					tmpAeInfo->s16ConvBvStepEntry[i], tmpAeInfo->bIsStable[i],
					tmpAeInfo->u8ConvergeMode[i], tmpAeInfo->u16ConvergeFrameCnt[i]);

			if (tmpAeInfo->bEnableSmoothAE) {
				for (j = 0; j < tmpAeInfo->u8MeterFramePeriod; ++j)
					printf("smBv:%d %d %d\n", tmpAeInfo->stSmoothApex[AE_LE][j].s16BVEntry,
						tmpAeInfo->stSmoothApex[AE_LE][j].s16TVEntry,
						tmpAeInfo->stSmoothApex[AE_LE][j].s16SVEntry);
			}

			printf("TS:%d %d\n", tmpAeInfo->stApex[i].s16TVEntry, tmpAeInfo->stApex[i].s16SVEntry);
			printf("exp:%u gain:%u %u\n\n", tmpAeInfo->stExp[i].u32ExpTime,
			       tmpAeInfo->stExp[i].stExpGain.u32AGain, tmpAeInfo->stExp[i].stExpGain.u32DGain);
		}
		preStable[sID][i] = tmpAeInfo->bIsStable[i];
	}
}

void AE_SetWDRRatioBias(CVI_U8 sID, CVI_U16 ratioBias)
{
	ISP_WDR_EXPOSURE_ATTR_S tmpWDR;

	AE_GetWDRExposureAttr(sID, &tmpWDR);
	tmpWDR.u16RatioBias = ratioBias;
	AE_SetWDRExposureAttr(sID, &tmpWDR);
}


void AE_ShowISOEntry2Gain(CVI_U8 sID, CVI_BOOL mode)
{
	CVI_S16 i, minISOEntry, maxISOEntry;
	AE_GAIN aeGain, newGain;
	CVI_U32 ISONum;
	ISP_EXPOSURE_ATTR_S tmpAe;
	AE_CTX_S *pstAeCtx;
	CVI_U32 again, dgain, againDbResult = 1024, dgainDbResult = 1024;

	VI_PIPE ViPipe;
	CVI_U32 aeTotalGain, sensorTotalGain, diffRatio;

	AE_GetExposureAttr(sID, &tmpAe);
	if (mode == 1)
		tmpAe.bAERouteExValid = 1;
	AE_SetExposureAttr(sID, &tmpAe);
	AE_Delay1ms(300);
	tmpAe.bByPass = 1;
	AE_Delay1ms(100);
	AE_GetISOEntryLimit(sID, &minISOEntry, &maxISOEntry);
	pstAeCtx = AE_GET_CTX(sID);
	ViPipe = AE_sID2ViPipe(sID);

	for (i = minISOEntry; i <= maxISOEntry; ++i) {
		if (mode == 1) {
			AE_GetRoutExGainByEntry(sID, AE_LE, i, &aeGain);
		} else {
			AE_GetExpGainByEntry(sID, i, &aeGain);
		}
		AE_GetISONumByEntry(sID, i, &ISONum);
		newGain = aeGain;
		again = newGain.u32AGain;
		dgain = newGain.u32DGain;
		if (pstAeCtx->stSnsRegister.stAeExp.pfn_cmos_again_calc_table)
			pstAeCtx->stSnsRegister.stAeExp.pfn_cmos_again_calc_table(ViPipe, &again, &againDbResult);

		if (pstAeCtx->stSnsRegister.stAeExp.pfn_cmos_dgain_calc_table)
			pstAeCtx->stSnsRegister.stAeExp.pfn_cmos_dgain_calc_table(ViPipe, &dgain, &dgainDbResult);

		newGain.u32AGain = again;
		newGain.u32DGain = dgain;

		if (aeGain.u32AGain != newGain.u32AGain) {
			newGain.u32ISPDGain = (newGain.u32ISPDGain  *
				aeGain.u32AGain + newGain.u32AGain / 2) / AAA_DIV_0_TO_1(newGain.u32AGain);
		}
		if (aeGain.u32DGain != newGain.u32DGain) {
			newGain.u32ISPDGain = (newGain.u32ISPDGain  *
				aeGain.u32DGain + newGain.u32DGain / 2) / AAA_DIV_0_TO_1(newGain.u32DGain);
		}

		aeTotalGain = AE_CalTotalGain(aeGain.u32AGain, aeGain.u32DGain, aeGain.u32ISPDGain);
		sensorTotalGain = AE_CalTotalGain(newGain.u32AGain, newGain.u32DGain, newGain.u32ISPDGain);
		diffRatio = (aeTotalGain > sensorTotalGain) ? (aeTotalGain - sensorTotalGain) * 100 / aeTotalGain :
			(sensorTotalGain - aeTotalGain) * 100 / aeTotalGain;
		//AE_GetLimitGain(sID, &newGain);
		if (diffRatio > 1)
			printf("i:%d ISO:%u AG:%u DG:%u IG:%u sAG:%u sDG:%u SIG:%u---D:%d\n", i, ISONum,
				aeGain.u32AGain, aeGain.u32DGain, aeGain.u32ISPDGain,
				newGain.u32AGain, newGain.u32DGain, newGain.u32ISPDGain,
				diffRatio);
		else
			printf("i:%d ISO:%u AG:%u DG:%u IG:%u sAG:%u sDG:%u SIG:%u\n", i, ISONum,
				aeGain.u32AGain, aeGain.u32DGain, aeGain.u32ISPDGain,
				newGain.u32AGain, newGain.u32DGain, newGain.u32ISPDGain);
	}
	tmpAe.bByPass = 0;
	tmpAe.bAERouteExValid = 0;
	AE_SetExposureAttr(sID, &tmpAe);
}

void AE_ShowMpiExposureInfo(CVI_U8 sID, CVI_BOOL ToFile)
{
	ISP_EXP_INFO_S stCurExpInfo;
	CVI_U16 i;
	FILE *fp;

	AE_GetExposureInfo(sID, &stCurExpInfo);

	printf("expTime:%u\n", stCurExpInfo.u32ExpTime);
	printf("AGain:%u\n", stCurExpInfo.u32AGain);
	printf("DGain:%u\n", stCurExpInfo.u32DGain);
	printf("ISPDGain:%u\n", stCurExpInfo.u32ISPDGain);
	printf("Exposure:%u\n", stCurExpInfo.u32Exposure);
	printf("ExposureIsMAX:%u\n", stCurExpInfo.bExposureIsMAX);
	printf("HistError:%d\n", stCurExpInfo.s16HistError);
	printf("AveLum:%u\n", stCurExpInfo.u8AveLum);
	printf("LinesPer500ms:%u\n", stCurExpInfo.u32LinesPer500ms);
	printf("PirisFNO:%u\n", stCurExpInfo.u32PirisFNO);
	printf("Fps:%u\n", stCurExpInfo.u32Fps);
	printf("ISO:%u\n", stCurExpInfo.u32ISO);
	printf("ISOCalibrate:%u\n", stCurExpInfo.u32ISOCalibrate);
	printf("RefExpRatio:%u\n", stCurExpInfo.u32RefExpRatio);
	printf("FirstStableTime:%u\n", stCurExpInfo.u32FirstStableTime);
	printf("RouteNum:%u\n", stCurExpInfo.stAERoute.u32TotalNum);
	printf("ShortExpTime:%u\n", stCurExpInfo.u32ShortExpTime);
	for (i = 0; i < stCurExpInfo.stAERoute.u32TotalNum; ++i) {
		printf("%u %u %u %d %u\n", i, stCurExpInfo.stAERoute.astRouteNode[i].u32IntTime,
		       stCurExpInfo.stAERoute.astRouteNode[i].u32SysGain,
		       stCurExpInfo.stAERoute.astRouteNode[i].enIrisFNO,
		       stCurExpInfo.stAERoute.astRouteNode[i].u32IrisFNOLin);
	}
	printf("AE_Hist256Value:\n");
	for (i = 0; i < HIST_NUM; ++i)
		printf("%u:%u\n", i, stCurExpInfo.au32AE_Hist256Value[i]);

#ifndef ARCH_RTOS_CV181X // TODO@CV181X
	if (ToFile) {
		fp = fopen("ExposureInfo.txt", "w");
		if (!fp) {
			printf("open file error!\n");
			return;
		}
		fprintf(fp, "expTime:%u\n", stCurExpInfo.u32ExpTime);
		fprintf(fp, "AGain:%u\n", stCurExpInfo.u32AGain);
		fprintf(fp, "DGain:%u\n", stCurExpInfo.u32DGain);
		fprintf(fp, "ISPDGain:%u\n", stCurExpInfo.u32ISPDGain);
		fprintf(fp, "Exposure:%u\n", stCurExpInfo.u32Exposure);
		fprintf(fp, "ExposureIsMAX:%u\n", stCurExpInfo.bExposureIsMAX);
		fprintf(fp, "HistError:%d\n", stCurExpInfo.s16HistError);
		fprintf(fp, "AveLum:%u\n", stCurExpInfo.u8AveLum);
		fprintf(fp, "LinesPer500ms:%u\n", stCurExpInfo.u32LinesPer500ms);
		fprintf(fp, "PirisFNO:%u\n", stCurExpInfo.u32PirisFNO);
		fprintf(fp, "Fps:%u\n", stCurExpInfo.u32Fps);
		fprintf(fp, "ISO:%u\n", stCurExpInfo.u32ISO);
		fprintf(fp, "ISOCalibrate:%u\n", stCurExpInfo.u32ISOCalibrate);
		fprintf(fp, "RefExpRatio:%u\n", stCurExpInfo.u32RefExpRatio);
		fprintf(fp, "FirstStableTime:%u\n", stCurExpInfo.u32FirstStableTime);
		fprintf(fp, "RouteNum:%u\n", stCurExpInfo.stAERoute.u32TotalNum);
		fprintf(fp, "AE_Hist256Value:\n");

		for (i = 0; i < stCurExpInfo.stAERoute.u32TotalNum; ++i) {
			fprintf(fp, "%u %u %u %d %u\n", i, stCurExpInfo.stAERoute.astRouteNode[i].u32IntTime,
				stCurExpInfo.stAERoute.astRouteNode[i].u32SysGain,
				stCurExpInfo.stAERoute.astRouteNode[i].enIrisFNO,
				stCurExpInfo.stAERoute.astRouteNode[i].u32IrisFNOLin);
		}

		for (i = 0; i < HIST_NUM; ++i)
			fprintf(fp, "%u:%u\n", i, stCurExpInfo.au32AE_Hist256Value[i]);

		fclose(fp);
	}
#endif
}

void AE_ShowSensorSetting(CVI_U8 sID)
{
	AE_CTX_S *pstAeCtx;
	SAE_INFO *tmpAeInfo;

	sID = AE_CheckSensorID(sID);
	pstAeCtx = AE_GET_CTX(sID);
	AE_GetCurrentInfo(sID, &tmpAeInfo);

	printf("LinesPer500ms:%u\n", pstAeCtx->stSnsDft.u32LinesPer500ms);
	printf("HmaxTimes:%u\n", pstAeCtx->stSnsDft.u32HmaxTimes);
	printf("InitExposure:%u\n", pstAeCtx->stSnsDft.u32InitExposure);
	printf("InitAETolerance:%u\n", pstAeCtx->stSnsDft.u32InitAETolerance);
	printf("FullLinesStd:%u\n", pstAeCtx->stSnsDft.u32FullLinesStd);
	printf("FullLinesMax:%u\n", pstAeCtx->stSnsDft.u32FullLinesMax);
	printf("FullLines:%u\n", pstAeCtx->stSnsDft.u32FullLines);
	printf("MaxIntTime:%u\n", pstAeCtx->stSnsDft.u32MaxIntTime);
	printf("MinIntTime:%u\n", pstAeCtx->stSnsDft.u32MinIntTime);
	printf("MaxIntTimeTarget:%u\n", pstAeCtx->stSnsDft.u32MaxIntTimeTarget);
	printf("MinIntTimeTarget:%u\n", pstAeCtx->stSnsDft.u32MinIntTimeTarget);

	printf("MaxAgain:%u\n", pstAeCtx->stSnsDft.u32MaxAgain);
	printf("MinAgain:%u\n", pstAeCtx->stSnsDft.u32MinAgain);
	printf("MaxAgainTarget:%u\n", pstAeCtx->stSnsDft.u32MaxAgainTarget);
	printf("MinAgainTarget:%u\n", pstAeCtx->stSnsDft.u32MinAgainTarget);
	printf("MaxDgain:%u\n", pstAeCtx->stSnsDft.u32MaxDgain);
	printf("MinDgain:%u\n", pstAeCtx->stSnsDft.u32MinDgain);
	printf("MaxDgainTarget:%u\n", pstAeCtx->stSnsDft.u32MaxDgainTarget);
	printf("MinDgainTarget:%u\n", pstAeCtx->stSnsDft.u32MinDgainTarget);

	printf("MaxISPDgainTarget:%u\n", pstAeCtx->stSnsDft.u32MaxISPDgainTarget);
	printf("MinISPDgainTarget:%u\n", pstAeCtx->stSnsDft.u32MinISPDgainTarget);
	printf("MaxIntTimeStep:%u\n", pstAeCtx->stSnsDft.u32MaxIntTimeStep);
	printf("LFMaxShortTime:%u\n", pstAeCtx->stSnsDft.u32LFMaxShortTime);
	printf("LFMinExposure:%u\n", pstAeCtx->stSnsDft.u32LFMinExposure);
	printf("stDgainAccu.enAccuType:%d\n", pstAeCtx->stSnsDft.stDgainAccu.enAccuType);
	printf("enBlcType:%d\n", pstAeCtx->stSnsDft.enBlcType);
	printf("TimeAccu:%d\n", (CVI_U16)pstAeCtx->stSnsDft.stIntTimeAccu.f32Accuracy);

	printf("Time m:%u M:%u\n", tmpAeInfo->u32ExpTimeMin, tmpAeInfo->u32ExpTimeMax);
	printf("TimeLine m:%u M:%u\n", tmpAeInfo->u32ExpLineMin, tmpAeInfo->u32ExpLineMax);
	printf("Tv Entry m:%d M:%d\n", tmpAeInfo->s16MinTVEntry, tmpAeInfo->s16MaxTVEntry);
	printf("again m:%u M:%u\n", tmpAeInfo->u32AGainMin, tmpAeInfo->u32AGainMax);
	printf("dgain m:%u M:%u\n", tmpAeInfo->u32DGainMin, tmpAeInfo->u32DGainMax);
	printf("ispDgain m:%u M:%u\n", tmpAeInfo->u32ISPDGainMin, tmpAeInfo->u32ISPDGainMax);
	printf("sysGain m:%u M:%u\n", tmpAeInfo->u32TotalGainMin, tmpAeInfo->u32TotalGainMax);
	printf("ISO Num m:%u M:%u\n", tmpAeInfo->u32ISONumMin, tmpAeInfo->u32ISONumMax);
	printf("ISO Entry m:%d M:%d\n", tmpAeInfo->s16MinISOEntry, tmpAeInfo->s16MaxISOEntry);
}

void AE_ShowBootLog(CVI_U8 sID)
{
	CVI_U8 i, j;
	SAE_BOOT_INFO tmpBootInfo;
	AE_APEX	tmpBootApex;
	AE_EXPOSURE tmpBootExp;
	CVI_U8 maxFrmNo = (AE_IsWDRMode(sID)) ? 2 : 1;

	memset(&tmpBootInfo, 0, sizeof(SAE_BOOT_INFO));
	AE_GetBootInfo(sID, &tmpBootInfo);
	AE_GetBootExpousreInfo(sID, &tmpBootApex);
	AE_GetExpTimeByEntry(sID, tmpBootApex.s16TVEntry, &tmpBootExp.u32ExpTime);
	AE_GetExposureTimeLine(sID, tmpBootExp.u32ExpTime, &tmpBootExp.u32ExpTimeLine);
	AE_GetExpGainByEntry(sID, tmpBootApex.s16SVEntry, &tmpBootExp.stExpGain);

	printf("Ae boot exp set!");
	printf("Bv:%d Tv:%d Sv:%d\n", tmpBootApex.s16BVEntry, tmpBootApex.s16TVEntry,
		tmpBootApex.s16SVEntry);
	printf("Time:%u(%u) Gain:%u %u %u\n", tmpBootExp.u32ExpTime, tmpBootExp.u32ExpTimeLine,
		tmpBootExp.stExpGain.u32AGain, tmpBootExp.stExpGain.u32DGain,
		tmpBootExp.stExpGain.u32ISPDGain);
	for (j = 0; j < AE_BOOT_MAX_FRAME; ++j) {
		for (i = 0; i < maxFrmNo; ++i) {
			printf("\n");
			printf("fid(%d, %d):%d ATL:%d Bv:%d Bs:%d Tv:%d Sv:%d CM:%d Stb:%d\n",
				sID, i, tmpBootInfo.u8FrmID[j],
				tmpBootInfo.u16AdjustTargetLuma[i][j], tmpBootInfo.stApex[i][j].s16BVEntry,
				tmpBootInfo.s16FrmBvStep[i][j], tmpBootInfo.stApex[i][j].s16TVEntry,
				tmpBootInfo.stApex[i][j].s16SVEntry, tmpBootInfo.bFrmConvergeMode[i][j],
				tmpBootInfo.bStable[i][j]);
		}
	}
	printf("\n");
}

void AE_ShowMpiWDRInfo(CVI_U8 sID)
{
	ISP_WDR_EXPOSURE_ATTR_S tmpWDR;
	CVI_U8 i;

	AE_GetWDRExposureAttr(sID, &tmpWDR);
	printf("Type:%d\n", tmpWDR.enExpRatioType);
	printf("ExpRatio:%u\n", tmpWDR.au32ExpRatio[0]);
	printf("ExpRatioMax:%uud\n", tmpWDR.u32ExpRatioMax);
	printf("ExpRatioMin:%u\n", tmpWDR.u32ExpRatioMin);
	printf("Tolerance:%u\n", tmpWDR.u16Tolerance);
	printf("Speed:%u\n", tmpWDR.u16Speed);
	printf("RatioBias:%u\n", tmpWDR.u16RatioBias);
	printf("SECompensation:%u\n", tmpWDR.u8SECompensation);
	printf("SEHisThr:%u\n", tmpWDR.u16SEHisThr);
	printf("SEHisCntRatio1:%u\n", tmpWDR.u16SEHisCntRatio1);
	printf("SEHisCntRatio2:%u\n", tmpWDR.u16SEHisCntRatio2);
	printf("SEHis255CntThr1:%u\n", tmpWDR.u16SEHis255CntThr1);
	printf("SEHis255CntThr2:%u\n", tmpWDR.u16SEHis255CntThr2);
	printf("u8AdjustTargetDetectFrmNum:%u\n", tmpWDR.u8AdjustTargetDetectFrmNum);
	printf("u32DiffPixelNum:%u\n", tmpWDR.u32DiffPixelNum);
	printf("u16LELowBinThr:%u\n", tmpWDR.u16LELowBinThr);
	printf("u16LEHighBinThr:%u\n", tmpWDR.u16LEHighBinThr);
	printf("u16SELowBinThr:%u\n", tmpWDR.u16SELowBinThr);
	printf("u16SEHighBinThr:%u\n", tmpWDR.u16SEHighBinThr);
	for (i = 0 ; i < LV_TOTAL_NUM; ++i) {
		printf("%u LE_Tar:%u-%u SE_Tar:%u-%u\n", i, tmpWDR.au8LEAdjustTargetMin[i],
			tmpWDR.au8LEAdjustTargetMax[i], tmpWDR.au8SEAdjustTargetMin[i],
			tmpWDR.au8SEAdjustTargetMax[i]);
	}
}

void AE_SetWDRSFTarget(CVI_U8 sID, CVI_U8 newTarget)
{
	ISP_WDR_EXPOSURE_ATTR_S tmpWDR;

	AE_GetWDRExposureAttr(sID, &tmpWDR);

	tmpWDR.u8SECompensation = newTarget;
	AE_SetWDRExposureAttr(sID, &tmpWDR);
	printf("set SECompensation:%d\n", tmpWDR.u8SECompensation);
}

void AE_ShowWeight(CVI_U8 sID)
{
	ISP_AE_STATISTICS_CFG_S aeStsCfg;
	CVI_U8 i, j;

	AE_GetStatisticsConfig(sID, &aeStsCfg);
	for (i = 0; i < AE_WEIGHT_ZONE_ROW; ++i) {
		for (j = 0; j < AE_WEIGHT_ZONE_COLUMN; ++j)
			printf("%d ", aeStsCfg.au8Weight[i][j]);
		printf("\n");
	}
	printf("\n");
}

void AE_RouteCheck(CVI_U8 sID)
{
	CVI_S16 tvEntry, ISOEntry;
	CVI_U8 i;
	ISP_AE_ROUTE_S tmpAeRoute;

	AE_GetRouteAttr(sID, &tmpAeRoute);
	for (i = 0; i < tmpAeRoute.u32TotalNum; ++i) {
		AE_GetTvEntryByTime(sID, tmpAeRoute.astRouteNode[i].u32IntTime, &tvEntry);
		AE_GetISOEntryByGain(tmpAeRoute.astRouteNode[i].u32SysGain, &ISOEntry);

		printf("%u T:%u G:%u TE:%d GE:%d\n", i, tmpAeRoute.astRouteNode[i].u32IntTime,
			tmpAeRoute.astRouteNode[i].u32SysGain, tvEntry, ISOEntry);
	}
}

void AE_ShowRouteInfo(CVI_U8 sID, CVI_U8 mode)
{
	CVI_U8 i;
	ISP_EXP_INFO_S stCurExpInfo;

	AE_GetExposureInfo(sID, &stCurExpInfo);

	if (mode == 1) {
		for (i = 0; i < stCurExpInfo.stAERouteEx.u32TotalNum; ++i) {
			printf("%u %u %u %u %u %u %u\n", i,
				stCurExpInfo.stAERouteEx.astRouteExNode[i].u32IntTime,
				stCurExpInfo.stAERouteEx.astRouteExNode[i].u32Again,
				stCurExpInfo.stAERouteEx.astRouteExNode[i].u32Dgain,
				stCurExpInfo.stAERouteEx.astRouteExNode[i].u32IspDgain,
				stCurExpInfo.stAERouteEx.astRouteExNode[i].enIrisFNO,
				stCurExpInfo.stAERouteEx.astRouteExNode[i].u32IrisFNOLin);
		}
	} else {
		for (i = 0; i < stCurExpInfo.stAERoute.u32TotalNum; ++i) {
			printf("%u %u %u %d %u\n", i,
				stCurExpInfo.stAERoute.astRouteNode[i].u32IntTime,
				stCurExpInfo.stAERoute.astRouteNode[i].u32SysGain,
				stCurExpInfo.stAERoute.astRouteNode[i].enIrisFNO,
				stCurExpInfo.stAERoute.astRouteNode[i].u32IrisFNOLin);
		}
	}
}

void AE_ShowLVTarget(CVI_U8 sID)
{
	CVI_S16 LvX100, minLvX100, maxLvX100;
	CVI_S32 targetMin[AE_MAX_WDR_FRAME_NUM], targetMax[AE_MAX_WDR_FRAME_NUM];

	ISP_EXPOSURE_ATTR_S tmpExpAttr;

	CVI_ISP_GetExposureAttr(sID, &tmpExpAttr);
	tmpExpAttr.bByPass = 1;
	CVI_ISP_SetExposureAttr(sID, &tmpExpAttr);
	AE_Delay1ms(10);

	minLvX100 = (MIN_LV * 1) * 100;
	maxLvX100 = (MAX_LV + 1) * 100;

	memset(targetMin, 0, sizeof(targetMin));
	memset(targetMax, 0, sizeof(targetMax));

	for (LvX100 = minLvX100; LvX100 <= maxLvX100; LvX100 += 25) {
		AE_GetTargetRange(sID, LvX100, targetMin, targetMax);
		if (AE_IsWDRMode(sID))
			printf("Lv:%d target LE m:%d M:%d SE m:%d M:%d\n",
				LvX100, targetMin[AE_LE], targetMax[AE_LE], targetMin[AE_SE],
				targetMax[AE_SE]);
		else
			printf("Lv:%d target m:%d M:%d\n", LvX100, targetMin[AE_LE], targetMax[AE_LE]);
	}
	tmpExpAttr.bByPass = 0;
	CVI_ISP_SetExposureAttr(sID, &tmpExpAttr);
}

CVI_S32 AE_GetSensorTableGain(CVI_U8 sID, CVI_U32 *again, CVI_U32 *dgain)
{
	AE_CTX_S *pstAeCtx;
	CVI_U32 againDb, dgainDb;
	VI_PIPE ViPipe;

	ViPipe = AE_sID2ViPipe(sID);
	pstAeCtx = AE_GET_CTX(sID);

	if (pstAeCtx->stSnsRegister.stAeExp.pfn_cmos_again_calc_table)
		pstAeCtx->stSnsRegister.stAeExp.pfn_cmos_again_calc_table(ViPipe, again,
			&againDb);

	if (pstAeCtx->stSnsRegister.stAeExp.pfn_cmos_dgain_calc_table)
		pstAeCtx->stSnsRegister.stAeExp.pfn_cmos_dgain_calc_table(ViPipe, dgain,
			&dgainDb);

	return CVI_SUCCESS;
}


void AE_ShutterGainLinearTest(CVI_U8 sID, CVI_U32 mode, CVI_S32 timeIdx, CVI_S32 gainIdx)
{
#define AE_FRAME_DELAY_TIME	300

	CVI_U16 i, j, cnt = 0;
	AE_EXPOSURE	aeExp[AE_MAX_WDR_FRAME_NUM], sensorExp[AE_MAX_WDR_FRAME_NUM];
	AE_GAIN aeGain;
	CVI_U16 *lumaBuf = CVI_NULL;
	CVI_U32 bufSize = 0;
	ISP_EXPOSURE_ATTR_S tmpAe;
	CVI_S16 minTvEntry, maxTvEntry, minSvEntry, maxSvEntry;
	CVI_U16 lumaBufEv[ENTRY_PER_EV + 1];
	AE_GAIN	expGain[ENTRY_PER_EV + 1];
	CVI_U32 frameID, frameLine, ISONum, BLCISONum;
	CVI_S16	svEntry;
	SAE_INFO *tmpAeInfo;
	CVI_U64 aeGainTatal, sensorGainTatal;
	CVI_BOOL useHistogram;
	CVI_U16	luma[AE_MAX_WDR_FRAME_NUM];

	static CVI_S16 preSvEntry;
	static CVI_BOOL lscStatus;

	sID = AE_CheckSensorID(sID);

	AE_GetExposureAttr(sID, &tmpAe);
	AE_GetLSC(sID, &lscStatus);
	AE_SetLSC(sID, 0);
	AE_SetManualExposureMode(sID, OP_TYPE_MANUAL, &tmpAe);
	printf("M:%u TimeIdx:%u GainIdx:%u\n", mode, timeIdx, gainIdx);
	//AE_SetDebugMode(10);//22

	//i2cInit(SENSOR_I2C_BUS_ID, 0x1A);

	tmpAe.u8DebugMode = AE_GetDebugMode(sID);
	AE_GetCurrentInfo(sID, &tmpAeInfo);
	AE_SetExposureAttr(sID, &tmpAe);
	AE_Delay1ms(AE_FRAME_DELAY_TIME);
	bufSize = (ENTRY_PER_EV + 1);
	memset(lumaBufEv, 0, bufSize * sizeof(CVI_U16));
	minTvEntry = tmpAeInfo->s16MinSensorTVEntry;
	maxTvEntry = tmpAeInfo->s16MaxSensorTVEntry;
	minSvEntry = tmpAeInfo->s16MinSensorISOEntry;
	maxSvEntry = tmpAeInfo->s16MaxSensorISOEntry;
	useHistogram = (tmpAeInfo->enLumaBase == HISTOGRAM_BASE) ? 1 : 0;

	if (mode == 0 || mode == 6) {
		bufSize = 0;
		AE_GetIdealExpTimeByEntry(sID, timeIdx, &aeExp[AE_LE].fIdealExpTime);
		AE_GetExposureTimeIdealLine(sID, aeExp[AE_LE].fIdealExpTime, &aeExp[AE_LE].fIdealExpTimeLine);
		if (mode == 6) {
			aeExp[AE_LE].stExpGain.u32AGain = 4096;
			aeExp[AE_LE].stExpGain.u32DGain = 1024;
			aeExp[AE_LE].stExpGain.u32ISPDGain = 1024;
		} else
			AE_GetExpGainByEntry(sID, gainIdx, &aeExp[AE_LE].stExpGain);
		aeExp[AE_LE].stExpGain.u32ISPDGain *=
				(aeExp[AE_LE].fIdealExpTimeLine) / (CVI_U32)aeExp[AE_LE].fIdealExpTimeLine;
		printf("tvIdx:%u time:%f line:%f G:%u %u %u\n", timeIdx, aeExp[AE_LE].fIdealExpTime,
			aeExp[AE_LE].fIdealExpTimeLine, aeExp[AE_LE].stExpGain.u32AGain,
			aeExp[AE_LE].stExpGain.u32DGain, aeExp[AE_LE].stExpGain.u32ISPDGain);

		AE_ConfigExposureTime_Gain(sID, AE_LE, aeExp[AE_LE].fIdealExpTime, &aeExp[AE_LE].stExpGain);
		AE_Delay1ms(AE_FRAME_DELAY_TIME);
		AE_GetSensorExposureSetting(sID, sensorExp);
		AE_GetFrameID(sID, &frameID);
		luma[AE_LE] = (useHistogram) ? AE_GetBrightQuaG(sID, AE_LE) : AE_GetCenterG(sID, AE_LE);
		printf("fid:%u AeL:%u\n", frameID, luma[AE_LE]);
		printf("sensor expL:%u AG:%u DG:%u IG:%u\n", sensorExp[AE_LE].u32SensorExpTimeLine,
				sensorExp[AE_LE].stSensorExpGain.u32AGain, sensorExp[AE_LE].stSensorExpGain.u32DGain,
				sensorExp[AE_LE].stSensorExpGain.u32ISPDGain);
	} else if (mode == 1) {
		printf("FrameLine:%d LineTime:%f\n", tmpAeInfo->u32FrameLine, tmpAeInfo->fExpLineTime);
		for (i = timeIdx; i <= timeIdx + ENTRY_PER_EV; i++) {
			AE_GetIdealExpTimeByEntry(sID, i, &aeExp[AE_LE].fIdealExpTime);
			AE_GetExposureTimeIdealLine(sID, aeExp[AE_LE].fIdealExpTime, &aeExp[AE_LE].fIdealExpTimeLine);
			AE_GetExpGainByEntry(sID, gainIdx, &aeExp[AE_LE].stExpGain);
			if (i >= TV_ENTRY_WITH_ISPDGAIN_COMPENSATION) {
				aeExp[AE_LE].stExpGain.u32ISPDGain *=
					(aeExp[AE_LE].fIdealExpTimeLine) / (CVI_U32)aeExp[AE_LE].fIdealExpTimeLine;
			}

			AE_ConfigExposureTime_Gain(sID, AE_LE, aeExp[AE_LE].fIdealExpTime, &aeExp[AE_LE].stExpGain);
			AE_Delay1ms(AE_FRAME_DELAY_TIME); //delay 300ms wait for AeRun update
			AE_GetSensorExposureSetting(sID, sensorExp);
			luma[AE_LE] = (useHistogram) ? AE_GetBrightQuaG(sID, AE_LE) : AE_GetCenterG(sID, AE_LE);
			if (cnt < bufSize) {
				lumaBufEv[cnt] = luma[AE_LE];
				AE_GetFrameID(sID, &frameID);
				printf("\nfid:%u E:%u ftime:%f fLine:%f IG:%u AeL:%u\n", frameID, i,
					aeExp[AE_LE].fIdealExpTime, aeExp[AE_LE].fIdealExpTimeLine,
					aeExp[AE_LE].stExpGain.u32ISPDGain, lumaBufEv[cnt]);
				printf("sensor expL:%u AG:%u DG:%u IG:%u\n",
					sensorExp[AE_LE].u32SensorExpTimeLine,
					sensorExp[AE_LE].stSensorExpGain.u32AGain,
					sensorExp[AE_LE].stSensorExpGain.u32DGain,
					sensorExp[AE_LE].stSensorExpGain.u32ISPDGain);
				cnt++;
			}
		}
	} else if (mode == 2) {
		AE_GetIdealExpTimeByEntry(sID, timeIdx, &aeExp[AE_LE].fIdealExpTime);
		AE_GetExposureTimeIdealLine(sID, aeExp[AE_LE].fIdealExpTime, &aeExp[AE_LE].fIdealExpTimeLine);
		printf("time:%f\n", aeExp[AE_LE].fIdealExpTime);
		for (i = gainIdx; i <= gainIdx + ENTRY_PER_EV; i++) {
			AE_GetExpGainByEntry(sID, i, &aeExp[AE_LE].stExpGain);
			aeGain = aeExp[AE_LE].stExpGain;
			AE_GetSensorTableGain(sID, &aeGain.u32AGain, &aeGain.u32DGain);
			aeGainTatal = aeExp[AE_LE].stExpGain.u32AGain * aeExp[AE_LE].stExpGain.u32DGain;
			sensorGainTatal = aeGain.u32AGain * aeGain.u32DGain;
			aeExp[AE_LE].stExpGain.u32ISPDGain = (CVI_U64)aeExp[AE_LE].stExpGain.u32ISPDGain *
				aeGainTatal / sensorGainTatal;
			AE_ConfigExposureTime_Gain(sID, AE_LE, aeExp[AE_LE].fIdealExpTime, &aeExp[AE_LE].stExpGain);
			AE_Delay1ms(AE_FRAME_DELAY_TIME); //delay 300 ms wait for AeRun update
			AE_GetSensorExposureSetting(sID, sensorExp);
			BLCISONum = AE_CalculateBLCISONum(sID, &sensorExp[AE_LE].stSensorExpGain);
			ISONum = AE_CalculateISONum(sID, &sensorExp[AE_LE].stSensorExpGain);
			luma[AE_LE] = (useHistogram) ? AE_GetBrightQuaG(sID, AE_LE) : AE_GetCenterG(sID, AE_LE);
			if (cnt < bufSize) {
				lumaBufEv[cnt] = luma[AE_LE];
				expGain[cnt].u32AGain = sensorExp[AE_LE].stSensorExpGain.u32AGain;
				expGain[cnt].u32DGain = sensorExp[AE_LE].stSensorExpGain.u32DGain;
				expGain[cnt].u32ISPDGain = sensorExp[AE_LE].stSensorExpGain.u32ISPDGain;
				AE_GetFrameID(sID, &frameID);
				printf("\nfid:%u E:%u gain:%u %u %u ISO:%u BLCISO:%u AeL:%u\n", frameID, i,
					aeExp[AE_LE].stExpGain.u32AGain, aeExp[AE_LE].stExpGain.u32DGain,
					aeExp[AE_LE].stExpGain.u32ISPDGain, ISONum, BLCISONum, lumaBufEv[cnt]);
				printf("sensor L:%u AG:%u DG:%u IG:%u\n", sensorExp[AE_LE].u32SensorExpTimeLine,
					sensorExp[AE_LE].stSensorExpGain.u32AGain,
					sensorExp[AE_LE].stSensorExpGain.u32DGain,
					sensorExp[AE_LE].stSensorExpGain.u32ISPDGain);
				cnt++;
			}
		}
	} else if (mode == 3) { // shutter linear test
		bufSize = maxTvEntry - minTvEntry + 1;
		#ifndef ARCH_RTOS_CV181X	// TODO@CV181X
		lumaBuf = malloc(bufSize * sizeof(CVI_U16));
		#endif
		if (lumaBuf == NULL) {
			printf("memory alloc fail!\n");
			return;
		}
		memset(lumaBuf, 0, bufSize * sizeof(CVI_U16));
		for (j = minTvEntry; j <= maxTvEntry; j += ENTRY_PER_EV) {
			for (i = j; i <= j + ENTRY_PER_EV; i++) {
				AE_GetIdealExpTimeByEntry(sID, i, &aeExp[AE_LE].fIdealExpTime);
				AE_GetExposureTimeIdealLine(sID, aeExp[AE_LE].fIdealExpTime,
					&aeExp[AE_LE].fIdealExpTimeLine);
				AE_GetExpGainByEntry(sID, gainIdx, &aeExp[AE_LE].stExpGain);
				if (i >= TV_ENTRY_WITH_ISPDGAIN_COMPENSATION) {
					aeExp[AE_LE].stExpGain.u32ISPDGain *=
					(aeExp[AE_LE].fIdealExpTimeLine) / (CVI_U32)aeExp[AE_LE].fIdealExpTimeLine;
				}
				AE_ConfigExposureTime_Gain(sID, AE_LE, aeExp[AE_LE].fIdealExpTime,
					&aeExp[AE_LE].stExpGain);
				AE_Delay1ms(AE_FRAME_DELAY_TIME); //delay 300ms wait for AeRun update
				AE_GetSensorExposureSetting(sID, sensorExp);
				luma[AE_LE] = (useHistogram) ? AE_GetBrightQuaG(sID, AE_LE) : AE_GetCenterG(sID, AE_LE);
				if (cnt < bufSize) {
					AE_GetFrameID(sID, &frameID);
					lumaBuf[cnt] = luma[AE_LE];
					printf("\nfid:%u time:%f TLine:%f AeL:%u\n", frameID,
						aeExp[AE_LE].fIdealExpTime, aeExp[AE_LE].fIdealExpTimeLine,
						lumaBuf[cnt]);
					printf("sensor L:%u AG:%u DG:%u IG:%u\n", sensorExp[AE_LE].u32SensorExpTimeLine,
						sensorExp[AE_LE].stSensorExpGain.u32AGain,
						sensorExp[AE_LE].stSensorExpGain.u32DGain,
						sensorExp[AE_LE].stSensorExpGain.u32ISPDGain);
					cnt++;
				}
			}
			AE_Delay1s(10);
			printf("====================\n");
		}
	} else if (mode == 4) { // gain linear test
		bufSize = maxSvEntry - minSvEntry + 1;
		#ifndef ARCH_RTOS_CV181X	// TODO@CV181X
		lumaBuf = malloc(bufSize * sizeof(CVI_U16));
		#endif
		if (lumaBuf == NULL) {
			printf("memory alloc fail!\n");
			return;
		}
		memset(lumaBuf, 0, bufSize * sizeof(CVI_U16));
		AE_GetIdealExpTimeByEntry(sID, timeIdx, &aeExp[AE_LE].fIdealExpTime);
		AE_GetExposureTimeIdealLine(sID, aeExp[AE_LE].fIdealExpTime, &aeExp[AE_LE].fIdealExpTimeLine);
		for (j = gainIdx; j < maxSvEntry; j += ENTRY_PER_EV) {
			for (i = j; i <= j + ENTRY_PER_EV; i++) {
				AE_GetExpGainByEntry(sID, i, &aeExp[AE_LE].stExpGain);
				aeGain = aeExp[AE_LE].stExpGain;
				AE_GetSensorTableGain(sID, &aeGain.u32AGain, &aeGain.u32DGain);
				if (i >= TV_ENTRY_WITH_ISPDGAIN_COMPENSATION) {
					aeExp[AE_LE].stExpGain.u32ISPDGain *=
					(aeExp[AE_LE].fIdealExpTimeLine) / (CVI_U32)aeExp[AE_LE].fIdealExpTimeLine;
				}
				aeGainTatal = aeExp[AE_LE].stExpGain.u32AGain * aeExp[AE_LE].stExpGain.u32DGain;
				sensorGainTatal = aeGain.u32AGain * aeGain.u32DGain;
				aeExp[AE_LE].stExpGain.u32ISPDGain = (CVI_U64)aeExp[AE_LE].stExpGain.u32ISPDGain *
					aeGainTatal / sensorGainTatal;
				AE_ConfigExposureTime_Gain(sID, AE_LE, aeExp[AE_LE].fIdealExpTime,
					&aeExp[AE_LE].stExpGain);
				AE_Delay1ms(AE_FRAME_DELAY_TIME); //delay 300 ms wait for AeRun update
				AE_GetSensorExposureSetting(sID, sensorExp);
				luma[AE_LE] = (useHistogram) ? AE_GetBrightQuaG(sID, AE_LE) : AE_GetCenterG(sID, AE_LE);
				if (cnt < bufSize) {
					lumaBuf[cnt] = luma[AE_LE];
					AE_GetFrameID(sID, &frameID);
					printf("\nfid:%u gain:%u %u %u AeL:%u\n", frameID,
					       aeExp[AE_LE].stExpGain.u32AGain, aeExp[AE_LE].stExpGain.u32DGain,
					       aeExp[AE_LE].stExpGain.u32ISPDGain, lumaBuf[cnt]);
					printf("sensor L:%u AG:%u DG:%u IG:%u\n", sensorExp[AE_LE].u32SensorExpTimeLine,
						sensorExp[AE_LE].stSensorExpGain.u32AGain,
						sensorExp[AE_LE].stSensorExpGain.u32DGain,
						sensorExp[AE_LE].stSensorExpGain.u32ISPDGain);
					cnt++;
				}
			}
			AE_Delay1s(10);
			printf("====================\n");
		}

	} else if (mode == 5) { // shutter/gain linear test
		bufSize = maxTvEntry - minTvEntry + 1;
		#ifndef ARCH_RTOS_CV181X	// TODO@CV181X
		lumaBuf = malloc(bufSize * sizeof(CVI_U16));
		#endif
		if (lumaBuf == NULL) {
			printf("memory alloc fail!\n");
			return;
		}
		memset(lumaBuf, 0, bufSize * sizeof(CVI_U16));
		for (i = minTvEntry, j = 0; i < maxTvEntry; i++, j++) {
			AE_GetIdealExpTimeByEntry(sID, i, &aeExp[AE_LE].fIdealExpTime);
			AE_GetExposureTimeIdealLine(sID, aeExp[AE_LE].fIdealExpTime, &aeExp[AE_LE].fIdealExpTimeLine);
			AE_GetExpGainByEntry(sID, gainIdx, &aeExp[AE_LE].stExpGain);
			if (i >= TV_ENTRY_WITH_ISPDGAIN_COMPENSATION) {
				aeExp[AE_LE].stExpGain.u32ISPDGain *=
				(aeExp[AE_LE].fIdealExpTimeLine) / (CVI_U32)aeExp[AE_LE].fIdealExpTimeLine;
			}
			AE_ConfigExposureTime_Gain(sID, AE_LE, aeExp[AE_LE].fIdealExpTime, &aeExp[AE_LE].stExpGain);
			AE_Delay1ms(AE_FRAME_DELAY_TIME); //delay 1s wait for AeRun update
			AE_GetSensorExposureSetting(sID, sensorExp);
			luma[AE_LE] = (useHistogram) ? AE_GetBrightQuaG(sID, AE_LE) : AE_GetCenterG(sID, AE_LE);
			if (cnt < bufSize) {
				lumaBuf[cnt] = luma[AE_LE];
				AE_GetFrameID(sID, &frameID);
				printf("\nfid:%u time:%f TLine:%f gain:%u %u %u AeL:%u\n", frameID,
				       aeExp[AE_LE].fIdealExpTime, aeExp[AE_LE].fIdealExpTimeLine,
				       aeExp[AE_LE].stExpGain.u32AGain, aeExp[AE_LE].stExpGain.u32DGain,
				       aeExp[AE_LE].stExpGain.u32ISPDGain, lumaBuf[cnt]);
				printf("sensor L:%u AG:%u DG:%u IG:%u\n", sensorExp[AE_LE].u32SensorExpTimeLine,
						sensorExp[AE_LE].stSensorExpGain.u32AGain,
						sensorExp[AE_LE].stSensorExpGain.u32DGain,
						sensorExp[AE_LE].stSensorExpGain.u32ISPDGain);
				cnt++;
			}
		}
	} else if (mode == 7) {// WDR LE
		if (!AE_IsWDRMode(sID)) {
			printf("Not WDR mode!\n");
			return;
		}
		frameLine = AE_GetFrameLine(sID);
		for (i = timeIdx; i <= timeIdx + ENTRY_PER_EV; i++) {
			AE_GetIdealExpTimeByEntry(sID, i, &aeExp[AE_LE].fIdealExpTime);
			AE_GetExposureTimeIdealLine(sID, aeExp[AE_LE].fIdealExpTime, &aeExp[AE_LE].fIdealExpTimeLine);
			AE_GetExpGainByEntry(sID, gainIdx, &aeExp[AE_LE].stExpGain);
			aeExp[AE_LE].stExpGain.u32ISPDGain *=
				(aeExp[AE_LE].fIdealExpTimeLine) / (CVI_U32)aeExp[AE_LE].fIdealExpTimeLine;
			AE_ConfigExposureTime_Gain(sID, AE_LE, aeExp[AE_LE].fIdealExpTime, &aeExp[AE_LE].stExpGain);
			AE_Delay1ms(AE_FRAME_DELAY_TIME); //delay 300ms wait for AeRun update
			luma[AE_LE] = (useHistogram) ? AE_GetBrightQuaG(sID, AE_LE) : AE_GetCenterG(sID, AE_LE);
			if (cnt < bufSize) {
				lumaBufEv[cnt] = luma[AE_LE];
				AE_GetSensorExposureSetting(sID, sensorExp);
				AE_GetFrameID(sID, &frameID);
				printf("\nfid:%u E_L:%u time:%f line_L:%f AeL:%u\n",
					frameID, i, aeExp[AE_LE].fIdealExpTime, aeExp[AE_LE].fIdealExpTimeLine,
					lumaBufEv[cnt]);
				printf("Sensor line_L:%u line_S:%u\n",
					sensorExp[AE_LE].u32SensorExpTimeLine,
					sensorExp[AE_SE].u32SensorExpTimeLine);
				printf("AG:%u DG:%u IG:%u --- Diff(%d)\n",
					sensorExp[AE_LE].stSensorExpGain.u32AGain,
					sensorExp[AE_LE].stSensorExpGain.u32DGain,
					sensorExp[AE_LE].stSensorExpGain.u32ISPDGain,
					(CVI_S32)aeExp[AE_LE].fIdealExpTimeLine -
					sensorExp[AE_LE].u32SensorExpTimeLine);
				printf("line(L+S):%u FL:%u\n", sensorExp[AE_LE].u32SensorExpTimeLine +
					sensorExp[AE_SE].u32SensorExpTimeLine, frameLine);
				cnt++;
			}
		}
	} else if (mode == 8) {// WDR SE
		if (!AE_IsWDRMode(sID)) {
			printf("Not WDR mode!\n");
			return;
		}
		frameLine = AE_GetFrameLine(sID);
		for (i = timeIdx; i <= timeIdx + ENTRY_PER_EV; i++) {
			AE_GetIdealExpTimeByEntry(sID, i, &aeExp[AE_SE].fIdealExpTime);
			AE_GetExposureTimeIdealLine(sID, aeExp[AE_SE].fIdealExpTime, &aeExp[AE_SE].fIdealExpTimeLine);
			AE_GetExpGainByEntry(sID, gainIdx, &aeExp[AE_SE].stExpGain);
			aeExp[AE_SE].stExpGain.u32ISPDGain *=
				(aeExp[AE_SE].fIdealExpTimeLine) / (CVI_U32)aeExp[AE_SE].fIdealExpTimeLine;
			AE_ConfigExposureTime_Gain(sID, AE_SE, aeExp[AE_SE].fIdealExpTime, &aeExp[AE_SE].stExpGain);
			AE_Delay1ms(AE_FRAME_DELAY_TIME); //delay 300ms wait for AeRun update
			luma[AE_SE] = (useHistogram) ? AE_GetBrightQuaG(sID, AE_SE) : AE_GetCenterG(sID, AE_SE);
			if (cnt < bufSize) {
				lumaBufEv[cnt] = luma[AE_SE];
				AE_GetSensorExposureSetting(sID, sensorExp);
				AE_GetFrameID(sID, &frameID);
				printf("\nfid:%u E_S:%u time:%f line:%f AeL:%u\n",
					frameID, i, aeExp[AE_SE].fIdealExpTime, aeExp[AE_SE].fIdealExpTimeLine,
					lumaBufEv[cnt]);
				printf("Sensor line_S:%u line_L:%u\n",
					sensorExp[AE_SE].u32SensorExpTimeLine, sensorExp[AE_LE].u32SensorExpTimeLine);
				printf("AG:%u DG:%u IG:%u --- Diff(%d)\n",
					sensorExp[AE_SE].stSensorExpGain.u32AGain,
					sensorExp[AE_SE].stSensorExpGain.u32DGain,
					sensorExp[AE_SE].stSensorExpGain.u32ISPDGain,
					(CVI_S32)aeExp[AE_SE].fIdealExpTimeLine -
					sensorExp[AE_SE].u32SensorExpTimeLine);
				printf("line(L+S):%u FL:%u\n", sensorExp[AE_LE].u32SensorExpTimeLine +
					sensorExp[AE_SE].u32SensorExpTimeLine, frameLine);
				cnt++;
			}
		}
	} else if (mode == 9) {
		AE_GetIdealExpTimeByEntry(sID, timeIdx, &aeExp[AE_LE].fIdealExpTime);
		AE_GetExposureTimeIdealLine(sID, aeExp[AE_LE].fIdealExpTime, &aeExp[AE_LE].fIdealExpTimeLine);
		for (i = 0; i <= ENTRY_PER_EV; i++) {
			if (i < ENTRY_PER_EV / 2)
				svEntry = gainIdx;
			else
				svEntry = gainIdx + 1;
			AE_GetExpGainByEntry(sID, svEntry, &aeExp[AE_LE].stExpGain);
			ISONum = AE_CalculateISONum(sID, &aeExp[AE_LE].stExpGain);
			AE_ConfigExposureTime_Gain(sID, AE_LE, aeExp[AE_LE].fIdealExpTime, &aeExp[AE_LE].stExpGain);
			AE_Delay1ms(AE_FRAME_DELAY_TIME);
			AE_GetSensorExposureSetting(sID, sensorExp);
			BLCISONum = AE_CalculateBLCISONum(sID, &sensorExp[AE_LE].stSensorExpGain);
			if (svEntry != preSvEntry)
				printf("AE AG:%u DG:%u IG:%u Sensor AG:%u DG:%u IG:%u ISO:%u BLCISO:%u\n",
						aeGain.u32AGain, aeGain.u32DGain, aeGain.u32ISPDGain,
						sensorExp[AE_LE].stSensorExpGain.u32AGain,
						sensorExp[AE_LE].stSensorExpGain.u32DGain,
						sensorExp[AE_LE].stSensorExpGain.u32ISPDGain, ISONum, BLCISONum);
			luma[AE_LE] = (useHistogram) ? AE_GetBrightQuaG(sID, AE_LE) : AE_GetCenterG(sID, AE_LE);
			if (i < bufSize) {
				AE_GetFrameID(sID, &frameID);
				lumaBufEv[i] = luma[AE_LE];
				printf("fid:%u AeL:%u\n", frameID, lumaBufEv[i]);
			}
			preSvEntry = svEntry;
		}
	} else if (mode == 10) {
		AE_SetIspDgainCompensation(sID, 0);
		AAA_LIMIT(timeIdx, minTvEntry, maxTvEntry);
		AAA_LIMIT(gainIdx, minSvEntry, maxSvEntry);
		for (i = timeIdx; i <= tmpAeInfo->s16MaxSensorTVEntry ; i++) {
			AE_GetIdealExpTimeByEntry(sID, i, &aeExp[AE_LE].fIdealExpTime);
			AE_GetExposureTimeIdealLine(sID, aeExp[AE_LE].fIdealExpTime, &aeExp[AE_LE].fIdealExpTimeLine);
			AE_GetExpGainByEntry(sID, gainIdx, &aeExp[AE_LE].stExpGain);
			AE_ConfigExposureTime_Gain(sID, AE_LE, aeExp[AE_LE].fIdealExpTime, &aeExp[AE_LE].stExpGain);
			AE_Delay1ms(AE_FRAME_DELAY_TIME); //delay 300ms wait for AeRun update
			AE_GetSensorExposureSetting(sID, sensorExp);
			AE_GetFrameID(sID, &frameID);
			luma[AE_LE] = (useHistogram) ? AE_GetBrightQuaG(sID, AE_LE) : AE_GetCenterG(sID, AE_LE);
			printf("\nfid:%u tv:%d time:%f TLine:%f AeL:%u\n", frameID, i,
				aeExp[AE_LE].fIdealExpTime, aeExp[AE_LE].fIdealExpTimeLine, luma[AE_LE]);
			printf("sensor expL:%u AG:%u DG:%u IG:%u\n",
				sensorExp[AE_LE].u32SensorExpTimeLine,
				sensorExp[AE_LE].stSensorExpGain.u32AGain,
				sensorExp[AE_LE].stSensorExpGain.u32DGain,
				sensorExp[AE_LE].stSensorExpGain.u32ISPDGain);
			cnt++;
			if (cnt % (2 * ENTRY_PER_EV) == 0)
				AE_Delay1ms(5000);
		}
	} else if (mode == 11) {
		AE_SetIspDgainCompensation(sID, 0);
		AAA_LIMIT(timeIdx, minTvEntry, maxTvEntry);
		AAA_LIMIT(gainIdx, minSvEntry, maxSvEntry);
		AE_GetIdealExpTimeByEntry(sID, timeIdx, &aeExp[AE_LE].fIdealExpTime);
		AE_GetExposureTimeIdealLine(sID, aeExp[AE_LE].fIdealExpTime, &aeExp[AE_LE].fIdealExpTimeLine);
		memset(expGain, 0, bufSize * sizeof(AE_GAIN));
		for (i = gainIdx; i <= maxSvEntry; i++) {
			AE_GetExpGainByEntry(sID, i, &aeExp[AE_LE].stExpGain);
			AE_ConfigExposureTime_Gain(sID, AE_LE, aeExp[AE_LE].fIdealExpTime, &aeExp[AE_LE].stExpGain);
			AE_Delay1ms(AE_FRAME_DELAY_TIME); //delay 300 ms wait for AeRun update
			AE_GetSensorExposureSetting(sID, sensorExp);
			AE_GetFrameID(sID, &frameID);
			luma[AE_LE] = (useHistogram) ? AE_GetBrightQuaG(sID, AE_LE) : AE_GetCenterG(sID, AE_LE);
			printf("\nfid:%u Sv:%u gain:%u %u %u AeL:%u\n", frameID, i,
				aeExp[AE_LE].stExpGain.u32AGain, aeExp[AE_LE].stExpGain.u32DGain,
				aeExp[AE_LE].stExpGain.u32ISPDGain, luma[AE_LE]);
			printf("sensor L:%u AG:%u DG:%u IG:%u\n", sensorExp[AE_LE].u32SensorExpTimeLine,
				sensorExp[AE_LE].stSensorExpGain.u32AGain,
				sensorExp[AE_LE].stSensorExpGain.u32DGain,
				sensorExp[AE_LE].stSensorExpGain.u32ISPDGain);
			cnt++;
			if (cnt % (2 * ENTRY_PER_EV) == 0)
				AE_Delay1ms(5000);
		}
	} else if (mode == 12) {
		if (!AE_IsWDRMode(sID)) {
			printf("Not WDR mode!\n");
			return;
		}
		frameLine = AE_GetFrameLine(sID);
		AE_GetIdealExpTimeByEntry(sID, timeIdx, &aeExp[AE_SE].fIdealExpTime);
		AE_GetExposureTimeIdealLine(sID, aeExp[AE_SE].fIdealExpTime, &aeExp[AE_SE].fIdealExpTimeLine);
		for (i = gainIdx; i <= gainIdx + ENTRY_PER_EV; i++) {
			AE_GetExpGainByEntry(sID, i, &aeExp[AE_SE].stExpGain);
			aeGain = aeExp[AE_SE].stExpGain;
			AE_GetSensorTableGain(sID, &aeGain.u32AGain, &aeGain.u32DGain);
			aeGainTatal = aeExp[AE_SE].stExpGain.u32AGain * aeExp[AE_SE].stExpGain.u32DGain;
			sensorGainTatal = aeGain.u32AGain * aeGain.u32DGain;
			aeExp[AE_SE].stExpGain.u32ISPDGain = (CVI_U64)aeExp[AE_SE].stExpGain.u32ISPDGain *
				aeGainTatal / sensorGainTatal;
			#if 0
			aeExp[AE_SE].stExpGain.u32ISPDGain *=
				(aeExp[AE_SE].fIdealExpTimeLine) / (CVI_U32)aeExp[AE_SE].fIdealExpTimeLine;
			#endif
			AE_ConfigExposureTime_Gain(sID, AE_SE, aeExp[AE_SE].fIdealExpTime, &aeExp[AE_SE].stExpGain);
			AE_Delay1ms(AE_FRAME_DELAY_TIME); //delay 300ms wait for AeRun update
			luma[AE_LE] = (useHistogram) ? AE_GetBrightQuaG(sID, AE_LE) : AE_GetCenterG(sID, AE_LE);
			luma[AE_SE] = (useHistogram) ? AE_GetBrightQuaG(sID, AE_SE) : AE_GetCenterG(sID, AE_SE);
			if (cnt < bufSize) {
				lumaBufEv[cnt] = luma[AE_SE];
				AE_GetSensorExposureSetting(sID, sensorExp);
				AE_GetFrameID(sID, &frameID);
				printf("\nfid:%u E_S:%u time:%f line:%f AeL:%u %u\n",
					frameID, i, aeExp[AE_SE].fIdealExpTime, aeExp[AE_SE].fIdealExpTimeLine,
					luma[AE_LE], luma[AE_SE]);
				printf("Sensor line_S:%u line_L:%u\n",
					sensorExp[AE_SE].u32SensorExpTimeLine, sensorExp[AE_LE].u32SensorExpTimeLine);
				printf("AG:%u DG:%u IG:%u --- Diff(%d)\n",
					sensorExp[AE_SE].stSensorExpGain.u32AGain,
					sensorExp[AE_SE].stSensorExpGain.u32DGain,
					sensorExp[AE_SE].stSensorExpGain.u32ISPDGain,
					(CVI_S32)aeExp[AE_SE].fIdealExpTimeLine -
					sensorExp[AE_SE].u32SensorExpTimeLine);
				printf("line(L+S):%u FL:%u\n", sensorExp[AE_LE].u32SensorExpTimeLine +
					sensorExp[AE_SE].u32SensorExpTimeLine, frameLine);
				cnt++;
			}
		}
	}

	if (cnt) {
		if (mode == 2 && expGain[0].u32AGain) {
			printf("Gain:\n");
			for (i = 0; i < ENTRY_PER_EV + 1; ++i) {
				printf("%u %u %u\n", expGain[i].u32AGain,
					expGain[i].u32DGain, expGain[i].u32ISPDGain);
			}
			printf("\n");
		}
		for (i = 0; i < ENTRY_PER_EV + 1; ++i)
			printf("%d\n", lumaBufEv[i]);
	} else {
		for (i = 0; i < bufSize; ++i) {
			if (lumaBuf[i]) {
				printf("%d\n", lumaBuf[i]);
				if ((i + 1) % ENTRY_PER_EV == 0)
					AE_Delay1ms(5);
			}
		}
	}

	AE_SetLSC(sID, lscStatus);
	AE_SetDebugMode(sID, 0);
	AE_SetWDRSETime(sID, 0);
	if (lumaBuf)
		free(lumaBuf);
}


void AE_SlowShutterCheck(CVI_U8 sID, CVI_U8 freqMode)
{
	ISP_EXPOSURE_ATTR_S expAttr;
	CVI_U32 oriTimeMax, oriGainThr;
	CVI_FLOAT fps;

	AE_GetExposureAttr(sID, &expAttr);
	oriTimeMax = expAttr.stAuto.stExpTimeRange.u32Max;
	oriGainThr = expAttr.stAuto.u32GainThreshold;
	expAttr.stAuto.stExpTimeRange.u32Max = 66666;
	expAttr.stAuto.enAEMode = AE_MODE_SLOW_SHUTTER;
	expAttr.stAuto.u32GainThreshold = 4096;
	expAttr.stAuto.stAntiflicker.bEnable = 0;

	if (freqMode == AE_FREQUENCE_60HZ || freqMode == AE_FREQUENCE_50HZ) {
		expAttr.stAuto.stAntiflicker.bEnable = 1;
		expAttr.stAuto.stAntiflicker.enFrequency = freqMode;
	}

	AE_SetExposureAttr(sID, &expAttr);
	AE_Delay1ms(500);
	AE_ShowRoute(sID, 0);
	AE_GetFps(sID, &fps);
	printf("fps:%f\n", fps);
	AE_Delay1ms(500);
	expAttr.stAuto.stExpTimeRange.u32Max = oriTimeMax;
	expAttr.stAuto.enAEMode = AE_MODE_FIX_FRAME_RATE;
	expAttr.stAuto.u32GainThreshold = oriGainThr;
	AE_SetExposureAttr(sID, &expAttr);
}


void AE_ShowCalculateManualBvEntry(CVI_U8 sID, CVI_BOOL useRouteEx)
{
#define MAX_ENTRY_NUM	5000
#define ENTRY_STEP		5

	CVI_U32 expValue, minExpValue, maxExpValue;
	ISP_EXPOSURE_ATTR_S tmpAe;
	CVI_S16 bvEntry;
	CVI_U16 i;
	SAE_INFO *tmpAeInfo;

	static CVI_S16 preBvEntry;

	sID = AE_CheckSensorID(sID);

	AE_GetExposureAttr(sID, &tmpAe);
	tmpAe.stAuto.bManualExpValue = 1;
	tmpAe.bByPass = 1;
	tmpAe.bAERouteExValid = useRouteEx;
	AE_SetExposureAttr(sID, &tmpAe);
	AE_Delay1ms(300);
	AE_GetCurrentInfo(sID, &tmpAeInfo);
	minExpValue = tmpAeInfo->u32ExpLineMin * (tmpAe.stAuto.stSysGainRange.u32Min >> 4);
	maxExpValue = tmpAeInfo->u32ExpLineMax * (tmpAe.stAuto.stSysGainRange.u32Max >> 4);

	for (i = 0; i < MAX_ENTRY_NUM; i += ENTRY_STEP) {
		expValue = minExpValue * pow(2, (CVI_FLOAT)i / ENTRY_PER_EV);
		AE_CalculateManualBvEntry(sID, expValue, &bvEntry);
		if (bvEntry != preBvEntry) {
			printf("E:%u Bv:%d\n", expValue, bvEntry);
			preBvEntry = bvEntry;
		}
		if (expValue > maxExpValue)
			break;
	}

	tmpAe.stAuto.bManualExpValue = 0;
	tmpAe.bByPass = 0;
	tmpAe.bAERouteExValid = 0;
	AE_SetExposureAttr(sID, &tmpAe);
}

void AE_SetDSPDGain(CVI_U8 sID, CVI_U32 ISPDGain)
{
	ISP_EXPOSURE_ATTR_S tmpAe;

	AE_GetExposureAttr(sID, &tmpAe);

	tmpAe.enOpType = OP_TYPE_MANUAL;
	tmpAe.stManual.enExpTimeOpType = OP_TYPE_MANUAL;
	tmpAe.stManual.enGainType = AE_TYPE_GAIN;
	tmpAe.stManual.enAGainOpType = OP_TYPE_MANUAL;
	tmpAe.stManual.enDGainOpType = OP_TYPE_MANUAL;
	tmpAe.stManual.enISPDGainOpType = OP_TYPE_MANUAL;
	tmpAe.stManual.u32ISPDGain = ISPDGain;
	printf("ISPDGain:%u\n", ISPDGain);

	AE_SetExposureAttr(sID, &tmpAe);
}

void AE_SetWDRManualRatio(CVI_U8 sID, CVI_U16 ratio)
{
	ISP_WDR_EXPOSURE_ATTR_S tmpWDR;

	AE_GetWDRExposureAttr(sID, &tmpWDR);
	tmpWDR.enExpRatioType = OP_TYPE_MANUAL;
	tmpWDR.au32ExpRatio[0] = ratio < 4 ? (4 * AE_WDR_RATIO_BASE) : (ratio * AE_WDR_RATIO_BASE);
	AE_SetWDRExposureAttr(sID, &tmpWDR);

	printf("WDR Manual R:%d\n", tmpWDR.au32ExpRatio[0]);

	if (ratio == 0) {
		CVI_U32 u32SeMaxTime = AE_GetWDRSEMaxTime(sID);

		printf("set max SE shutter time: %d\n", u32SeMaxTime);

		AE_SetManualExposureTest(sID, 2, 100000, AE_GetISONum(sID));
	}
}

#endif

void AE_ShowDebugInfo(CVI_U8 sID)
{
	UNUSED(sID);

#ifdef ENABLE_AE_DEBUG

	SAE_INFO *tmpAeInfo;

	if (AE_GetDebugMode(sID)) {
		if (AE_GetDebugMode(sID) == 6) {
			AE_ShowBootLog(sID);
			AE_SetDebugMode(sID, 0);
		}  else if (AE_GetDebugMode(sID) == 13) {
			AE_ShowConvergeStatus(sID);
		} else if (AE_GetDebugMode(sID) == 14) {
			AE_GetCurrentInfo(sID, &tmpAeInfo);
			AE_ShowCurrentInfo(sID, tmpAeInfo);
		} else if (AE_GetDebugMode(sID) == 15) {
			AE_GetCurrentInfo(sID, &tmpAeInfo);
			AE_ShowGridRGB(tmpAeInfo, 0);
			AE_SetDebugMode(sID, 0);
		} else if (AE_GetDebugMode(sID) == 16) {
			AE_GetCurrentInfo(sID, &tmpAeInfo);
			AE_ShowMeterLuma(tmpAeInfo, 0);
			AE_SetDebugMode(sID, 0);
		} else if (AE_GetDebugMode(sID) == 17) {
			AE_ShowSensorSetting(sID);
			AE_SetDebugMode(sID, 0);
		} else if (AE_GetDebugMode(sID) == 29) {
			AE_ShowWeight(sID);
			AE_SetDebugMode(sID, 0);
		} else if (AE_GetDebugMode(sID) == 30) {
			AE_ShowISOEntry2Gain(sID, 0);
			AE_SetDebugMode(sID, 0);
		} else if (AE_GetDebugMode(sID) == 31) {
			AE_ShowISOEntry2Gain(sID, 1);
			AE_SetDebugMode(sID, 0);
		} else if (AE_GetDebugMode(sID) == 32) {
			AE_ShowRoute(sID, 0);
			AE_SetDebugMode(sID, 0);
		} else if (AE_GetDebugMode(sID) == 33) {
			AE_ShowRoute(sID, 1);
			AE_SetDebugMode(sID, 0);
		} else if (AE_GetDebugMode(sID) == 34) {
			AE_ShowLVTarget(sID);
			AE_SetDebugMode(sID, 0);
		} else if (AE_GetDebugMode(sID) == 44) {
			AE_SetISPDGainPriority(sID, 1);
			AE_SetDebugMode(sID, 0);
		} else if (AE_GetDebugMode(sID) == 45) {
			AE_SetISPDGainPriority(sID, 0);
			AE_SetDebugMode(sID, 0);
		} else if (AE_GetDebugMode(sID) == 46) {
			AE_SetFpsTest(sID, 15);
			AE_SetDebugMode(sID, 0);
		}
	}
#endif
}


void AE_ShowInfoList(void)
{
#ifdef ENABLE_AE_DEBUG

	CVI_S32 sID, item, param;
	SAE_INFO *tmpAeInfo;
	CVI_U32 fid;

	printf("0:AE_ShowCurrentInfo\n");
	printf("1:AE_ShowGridRGB\n");
	printf("2:AE_ShowHistogram\n");
	printf("3:AE_ShowMeterLuma\n");
	printf("4:AE_ShowRoute\n");
	printf("5:AE_ReadI2CData\n");
	printf("6:\n");
	printf("7:AE_ShowExposureAttr(sID, toFile)\n");
	printf("8:AE_ShowExpLine(sID)\n");
	printf("9:AE_ShowISOEntry2Gain(sID)\n");
	printf("10:AE_ShowExposureInfo(sID)\n");
	printf("11:AE_ShowSensorSetting(sID)\n");
	printf("12:AE_ShowBootLog(sID)\n");
	printf("13:AE_ShowMpiWDRInfo(sID)\n");
	printf("14:AE_ShowWeight(sID)\n");
	printf("15:AE_RouteCheck(sID)\n");
	printf("16:AE_ShowRouteInfo(sID)\n");
	printf("17:\n");
	printf("18:AE_ShowLVTarget(sID)\n");
	printf("19:CVI_ISP_SetExposureAttr_Test(sID)\n");
	printf("20:CVI_ISP_SetAEStatisticsConfig_Test(sID)\n");
	printf("21:CVI_ISP_SetAERouteAttr_Test(sID)\n");
	printf("22:CVI_ISP_SetAERouteAttrEx_Test(sID)\n");
	printf("23:CVI_ISP_SetWDRExposureAttr_Test(sID)\n");
	printf("24:AE_SlowShutterCheck(sID)\n");
	printf("25:\n");
	printf("26:AE_ShowCalculateManualBvEntry(sID)\n");
	printf("Item/sID/para:\n");
	scanf("%d %d %d", &item, &sID, &param);

	sID = AE_CheckSensorID(sID);
	AE_GetFrameID(sID, &fid);
	AE_GetCurrentInfo(sID, &tmpAeInfo);

	switch (item) {
	case 0:
		AE_ShowCurrentInfo(sID, tmpAeInfo);
		break;
	case 1:
		AE_ShowGridRGB(tmpAeInfo, param);
		break;
	case 2:
		AE_ShowHistogram(tmpAeInfo);
		break;
	case 3:
		AE_ShowMeterLuma(tmpAeInfo, param);
		break;
	case 4:
		AE_ShowRoute(sID, param);
		break;
	case 5:
		break;
	case 6:
		break;
	case 7:
		break;
	case 8:
		AE_ShowExpLine(sID, tmpAeInfo);
		break;
	case 9:
		AE_ShowISOEntry2Gain(sID, param);
		break;
	case 10:
		AE_ShowMpiExposureInfo(sID, param);
		break;
	case 11:
		AE_ShowSensorSetting(sID);
		break;
	case 12:
		AE_ShowBootLog(sID);
		break;
	case 13:
		AE_ShowMpiWDRInfo(sID);
		break;
	case 14:
		AE_ShowWeight(sID);
		break;
	case 15:
		AE_RouteCheck(sID);
		break;
	case 16:
		AE_ShowRouteInfo(sID, param);
		break;
	case 17:
		break;
	case 18:
		AE_ShowLVTarget(sID);
		break;
	case 19:
		CVI_ISP_SetExposureAttr_Test(sID);
		break;
	case 20:
		CVI_ISP_SetAEStatisticsConfig_Test(sID);
		break;
	case 21:
		CVI_ISP_SetAERouteAttr_Test(sID);
		break;
	case 22:
		CVI_ISP_SetAERouteAttrEx_Test(sID);
		break;
	case 23:
		CVI_ISP_SetWDRExposureAttr_Test(sID);
		break;
	case 24:
		AE_SlowShutterCheck(sID, param);
		break;
	case 25:
		break;
	case 26:
		AE_ShowCalculateManualBvEntry(sID, param);
		break;
	default:
		break;
	}
#endif
}

void AE_SetParaList(void)
{
#ifdef ENABLE_AE_DEBUG
	CVI_S32 sID = 0, item = 0, para1 = 0, para2 = 0, para3 = 0;

	printf("0:AE_SetManual(0:Auto 1:Manual)\n");
	printf("1:AE_ShutterGainLinearTest()\n");
	printf("		0:single\n");
	printf("		1:shutter linear 1EV\n");
	printf("		2:gain linear 1EV\n");
	printf("		3:shutter linear All EV\n");
	printf("		4:gain linear All EV\n");
	printf("		5:shutter/gain linear All EV\n");
	printf("2:AE_SetDebugMode(item)\n");
	printf("3:AE_SetAssignISOEntry(sid, isoEntry)\n");
	printf("4:AE_SetAssignTvEntry(sid, TvEntry)\n");
	printf("5:AE_SetAssignEvBias(sid, evbias)\n");
	printf("6:AE_SetConvergeSpeed(sid,)\n");
	printf("7:AE_SetDSPDGain(sid, dgain)\n");
	printf("8:AE_SetAntiFlicker(sid, en, Hz)\n");
	printf("9:AE_SetLumaBase(sID, 0:grid 1:histogram)\n");
	printf("10.AE_SetISPDGainPriority(sid, enable)\n");
	printf("11.AE_EnableSmooth(sid, enable)\n");
	printf("12.AE_SetByPass(sid, enable)\n");
	printf("13.AE_SetLSC(sid, enable)\n");
	printf("14.AE_SetWDRManualRatio(sid, ratio)\n");
	printf("15.AE_SetWDRSETime(sid, time)\n");
	printf("16.AE_SaveSnapLog(sid)\n");
	printf("17.AE_DumpLog()\n");
	printf("18.AE_SetWDRSFTarget(sid, ratio)\n");
	printf("19.AE_SetWDRLEOnly(sid, enable)\n");
	printf("20.AE_SetMeterMode(sid, mode)\n");
	printf("21.AE_SetFpsTest(sid, fps)\n");
	printf("21.AE_SetIspDgainCompensation(sid, enable)\n");
	printf("Item/sID/para1/para2/para3\n");
	scanf("%d %d %d %d %d", &item, &sID, &para1, &para2, &para3);

	switch (item) {
	case 0:
		AE_SetManual(sID, para1, para2);
		break;
	case 1:
		AE_ShutterGainLinearTest(sID, para1, para2, para3);
		break;
	case 2:
		AE_SetDebugMode(sID, para1);
		break;
	case 3:
		AE_SetAssignISOEntry(sID, para1);
		break;
	case 4:
		AE_SetAssignTvEntry(sID, para1);
		break;
	case 5:
		AE_SetAssignEvBias(sID, para1);
		break;
	case 6:
		AE_SetConvergeSpeed(sID, para1, para2);
		break;
	case 7:
		AE_SetDSPDGain(sID, para1);
		break;
	case 8:
		AE_SetAntiFlicker(sID, para1, para2);
		break;
	case 9:
		AE_SetLumaBase(sID, para1);
		break;
	case 10:
		AE_SetISPDGainPriority(sID, para1);
		break;
	case 11:
		AE_EnableSmooth(sID, para1);
		break;
	case 12:
		AE_SetByPass(sID, para1);
		break;
	case 13:
		AE_SetLSC(sID, para1);
		break;
	case 14:
		AE_SetWDRManualRatio(sID, para1);
		break;
	case 15:
		AE_SetWDRSETime(sID, para1);
		break;
	case 16:
		break;
	case 17:
		AE_DumpLog();
		break;
	case 18:
		AE_SetWDRSFTarget(sID, para1);
		break;
	case 19:
		AE_SetWDRLEOnly(sID, para1);
		break;
	case 20:
		AE_SetMeterMode(sID, para1);
		break;
	case 21:
		AE_SetFpsTest(sID, para1);
		break;
	case 22:
		AE_SetIspDgainCompensation(sID, para1);
		break;
	default:
		break;
	}
#endif
}

void AE_Debug(void)
{
#ifndef ARCH_RTOS_CV181X // TODO@CV181X
	CVI_S32 item;

	printf("0:	AE-SetParaList\n");
	printf("1:	AE_ShowInfoList\n");
	printf("item:\n");
	scanf("%d", &item);

	switch (item) {
	case 0:
		AE_SetParaList();
		break;
	case 1:
		AE_ShowInfoList();
		break;
	default:
		break;
	}
#endif
}




#define GEN_RAW_SKIP	(0)		//set 1 to speed GenRaw, but bad resolution
#ifndef ARCH_RTOS_CV181X // TODO@CV181X
#define MEAUSURE_AE_T(a) gettimeofday(&tt_dbgae[a], NULL)

static struct timeval tt_dbgae[20];
#else
#define MEAUSURE_AE_T(a)
#endif

#ifndef ARCH_RTOS_CV181X // TODO@CV181X
void GenNewRaw(void *pDstOri, void *pSrcOri, CVI_U32 sizeBk, CVI_U32 raw_mode,
	CVI_U32 raw_w, CVI_U32 raw_h, CVI_U32 nRawBlc)
{
	CVI_U32 i, exp_sel, genNum;
	CVI_U16 w, h, m, stride, offsetW;
	CVI_FLOAT fgainTop, fgainBot;
	CVI_U16 gainTop, gainBot;
	CVI_U16 botLuma[2], tarLuma[2];
	CVI_U8 v0, v1, v2;
	CVI_U8 *pSrc2;
	CVI_U8 *pDst;
	CVI_S32 topi = -1, boti = -1;
	CVI_FLOAT aebBV[AE_TEST_RAW_NUM], targetBV;
	CVI_FLOAT tmpTv, tmpSv;
	CVI_U16 isDbglog = 0;
	CVI_U8 isOver = 0;
	CVI_FLOAT topDiff, botDiff;
	CVI_S16 wantTv[2], wantSv[2];
	CVI_U8 shiftNum;

	if (raw_mode) {//WDR
		sizeBk = sizeBk >> 1;
		raw_w = raw_w >> 1;
		genNum = 2;
		shiftNum = 1;
	} else {
		genNum = 1;
		shiftNum = 0;
	}

	wantTv[0] = sAe_sim.ae_wantTv[0];
	wantSv[0] = sAe_sim.ae_wantSv[0];
	wantTv[1] = sAe_sim.ae_wantTv[1];
	wantSv[1] = sAe_sim.ae_wantSv[1];

	MEAUSURE_AE_T(0);

	if (isDbglog) {
		printf("ae_wantSv %d %d %d %d\n", sAe_sim.ae_wantTv[0],
			sAe_sim.ae_wantTv[1], sAe_sim.ae_wantSv[0], sAe_sim.ae_wantSv[1]);
	}
	stride = raw_w*3/2;
	offsetW = raw_w*1.5;

//////////////////////////////////Calc Raw BV
	for (i = 0; i < AE_TEST_RAW_NUM; i++) {
		tmpTv = (aeb_ev[i].tv - EVTT_ENTRY_1_30SEC);
		tmpTv = tmpTv / (CVI_FLOAT)ENTRY_PER_TV + 5.0;
		tmpSv = (aeb_ev[i].sv - ISO_100_Entry);
		tmpSv = tmpSv / (CVI_FLOAT)ENTRY_PER_SV + 5.0;
		aebBV[i] = tmpTv - tmpSv;
		if (isDbglog) {
			printf("Dll_%d\t%d\t%d\t%f\t%f\t%f\n", i, aeb_ev[i].tv,
				aeb_ev[i].sv, tmpTv, tmpSv, aebBV[i]);
		}
	}
//////////////////////////////////LE
	for (exp_sel = 0; exp_sel < genNum; exp_sel++) {
		tmpTv = (wantTv[exp_sel] - EVTT_ENTRY_1_30SEC);
		tmpTv = tmpTv / (CVI_FLOAT)ENTRY_PER_TV+5.0;
		tmpSv = (wantSv[exp_sel] - ISO_100_Entry);
		tmpSv = tmpSv / (CVI_FLOAT)ENTRY_PER_SV + 5.0;
		targetBV = tmpTv - tmpSv;

		for (i = 0; i < AE_TEST_RAW_NUM; i++) {
			if (targetBV <= aebBV[i]) {
				topi = i-1;
				boti = i;
				break;
			}
		}
		if (boti == (-1)) {
			printf("Dll_Out of Range\n");
			isOver = 1;
		}
		if (boti == 0) {
			topi = 0;
		}
		if (!isOver) {
			topDiff = targetBV - aebBV[topi];
			botDiff = aebBV[boti] - targetBV;
		} else {
			topi = AE_TEST_RAW_NUM-1;
			boti = AE_TEST_RAW_NUM-1;
			topDiff = targetBV - aebBV[topi];
			botDiff = 0;
		}
		fgainTop = pow(2, topDiff);
		fgainBot = pow(2, botDiff);
		gainTop = 128 / fgainTop;
		gainBot = 128 * fgainBot;

		if (isDbglog) {
			printf("\nTop_%d\t%d\t%d\t%f\t%f\t%f\n", topi, aeb_ev[topi].tv,
				aeb_ev[topi].sv, tmpTv, tmpSv, aebBV[topi]);
			printf("Want:\t%d\t%d\t%f\t%f\t%f\n", wantTv[exp_sel], wantSv[exp_sel], tmpTv, tmpSv, targetBV);
			printf("Bot_%d\t%d\t%d\t%f\t%f\t%f\n", boti, aeb_ev[boti].tv, aeb_ev[boti].sv,
				tmpTv, tmpSv, aebBV[boti]);
			printf("_Diff_Bv Top:%f Bot:%f\n", topDiff, botDiff);
			printf("_Gain_Ga Top:%f Bot:%f\n", fgainTop, fgainBot);
			printf("_Gain_Ga Top:%d Bot:%d\n", gainTop, gainBot);
		}

		pDst = (CVI_U8 *)pDstOri+exp_sel*offsetW;
		pSrc2 = (CVI_U8 *)pSrcOri+sizeBk*boti;

		for (h = 0; h < raw_h; h++) {//LE
#if GEN_RAW_SKIP
			for (w = 0, m = 0, i = h * stride; w < raw_w; w += 4, m += 6) {
#else
			for (w = 0, m = 0, i = h * stride; w < raw_w; w += 2, m += 3) {
#endif
				v0 = (CVI_U8) pSrc2[i + m];
				v1 = (CVI_U8) pSrc2[i + m + 1];
				v2 = (CVI_U8) pSrc2[i + m + 2];
				botLuma[0] = ((v0 << 4) | ((v2 >> 0) & 0x0f));
				botLuma[1] = ((v1 << 4) | ((v2 >> 4) & 0x0f));
				if (botLuma[0] > nRawBlc)
					botLuma[0] -= nRawBlc;
				else
					botLuma[0] = 0;
				if (botLuma[1] > nRawBlc)
					botLuma[1] -= nRawBlc;
				else
					botLuma[1] = 0;

				if (!isOver) {
					tarLuma[0] = (botLuma[0] * gainBot) >> 7;
					tarLuma[1] = (botLuma[1] * gainBot) >> 7;
				} else {//out of range
					tarLuma[0] = (botLuma[0] * gainBot) >> 7;
					tarLuma[1] = (botLuma[1] * gainBot) >> 7;
				}
				tarLuma[0] += nRawBlc;
				tarLuma[1] += nRawBlc;
				if (tarLuma[0] > 4095) {
					tarLuma[0] = 4095;
				}
				if (tarLuma[1] > 4095) {
					tarLuma[1] = 4095;
				}

				pDst[(i << shiftNum) + m] = tarLuma[0] >> 4;
				pDst[(i << shiftNum) + m + 1] = tarLuma[1] >> 4;
				pDst[(i << shiftNum) + m + 2] = (tarLuma[0] & 0X0F) + ((tarLuma[1] & 0X0F) << 4);
#if GEN_RAW_SKIP
				pDst[(i << shiftNum) + m + 3] = pDst[(i << shiftNum) + m];
				pDst[(i << shiftNum) + m + 4] = pDst[(i << shiftNum) + m + 1];
				pDst[(i << shiftNum) + m + 5] = pDst[(i << shiftNum) + m + 2];
#endif
			}
		}
	}

	MEAUSURE_AE_T(1);
	//printf(":%d\n", AE_GetMSTimeDiff(&tt_dbgae[0], &tt_dbgae[1]));

	sAe_sim.genRawCnt++;
}
#else
void GenNewRaw(void *pDstOri, void *pSrcOri, CVI_U32 sizeBk, CVI_U32 raw_mode,
	CVI_U32 raw_w, CVI_U32 raw_h, CVI_U32 nRawBlc)
{

}
#endif
