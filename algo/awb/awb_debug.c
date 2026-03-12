/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: awb_debug.c
 * Description:
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "cvi_isp.h"
#include "cvi_awb.h"
#include "awbalgo.h"
#include "awb_buf.h"
#include "awb_debug.h"

CVI_U8 u8AwbDebugPrintMode;

static void AWB_ShowRGB(CVI_U8 mode, const sWBInfo *pstWBInfo, CVI_BOOL toFile)
{
#ifndef ARCH_RTOS_CV181X // TODO@CV181X
	CVI_U16 i, j, k, idx;
	FILE *fp;
	CVI_CHAR fileName[30];
	static CVI_U8 fileIdx;
	CVI_U16 total_size, col_size, row_size;

	col_size = pstWBInfo->u16WBColumnSize;
	row_size = pstWBInfo->u16WBRowSize;
	total_size = col_size * row_size;

	if (mode == 0) {
		for (k = 0; k < pstWBInfo->u8AwbMaxFrameNum; ++k) {
			printf("R:\n");
			for (i = 0; i < total_size; ++i) {
				printf("%d ", pstWBInfo->pu16AwbStatistics[k][AWB_R][i]);
				if ((i + 1) % col_size == 0)
					printf("\n");
			}
			printf("\n");
			printf("G:\n");
			for (i = 0; i < total_size; ++i) {
				printf("%d ", pstWBInfo->pu16AwbStatistics[k][AWB_G][i]);
				if ((i + 1) % col_size == 0)
					printf("\n");
			}
			printf("\n");
			printf("B:\n");
			for (i = 0; i < total_size; ++i) {
				printf("%d ", pstWBInfo->pu16AwbStatistics[k][AWB_B][i]);
				if ((i + 1) % col_size == 0)
					printf("\n");
			}
			printf("\n");
		}

		printf("%d %u", pstWBInfo->stEnvInfo.s16LvX100, pstWBInfo->stEnvInfo.u32ISONum);
	} else if (mode == 1) {
		printf("R:\n");
		for (k = 0; k < pstWBInfo->u8AwbMaxFrameNum; ++k) {
			for (i = row_size / 2 - row_size / 4; i < row_size / 2; ++i) {
				for (j = col_size / 2 - col_size / 4; j < col_size / 2; ++j) {
					idx = j * col_size + i;
					printf("%d ", pstWBInfo->pu16AwbStatistics[k][AWB_R][idx]);
				}
				printf("\n");
			}
			printf("\n");
			printf("G:\n");
			for (i = row_size / 2 - row_size / 4; i < row_size / 2; ++i) {
				for (j = col_size / 2 - col_size / 4; j < col_size / 2; ++j) {
					idx = j * col_size + i;
					printf("%d ", pstWBInfo->pu16AwbStatistics[k][AWB_G][idx]);
				}
				printf("\n");
			}
			printf("\n");
			printf("B:\n");
			for (i = row_size / 2 - row_size / 4; i < row_size / 2; ++i) {
				for (j = col_size / 2 - col_size / 4; j < col_size / 2; ++j) {
					idx = j * col_size + i;
					printf("%d ", pstWBInfo->pu16AwbStatistics[k][AWB_B][idx]);
				}
				printf("\n");
			}
		}
		printf("\n");
	}

	if (toFile) {
		sprintf(fileName, "WB_RGB_%d.txt", fileIdx++);
		fp = fopen(fileName, "w");
		if (!fp) {
			printf("open file:%s fail!\n", fileName);
			return;
		}

		for (k = 0; k < pstWBInfo->u8AwbMaxFrameNum; ++k) {
			for (i = 0; i < total_size; ++i) {
				fprintf(fp, "%d\t", pstWBInfo->pu16AwbStatistics[k][AWB_R][i]);
				if ((i + 1) % col_size == 0)
					fprintf(fp, "\n");
			}
			fprintf(fp, "\n\n");
			for (i = 0; i < total_size; ++i) {
				fprintf(fp, "%d\t", pstWBInfo->pu16AwbStatistics[k][AWB_G][i]);
				if ((i + 1) % col_size == 0)
					fprintf(fp, "\n");
			}
			fprintf(fp, "\n\n");
			for (i = 0; i < total_size; ++i) {
				fprintf(fp, "%d\t", pstWBInfo->pu16AwbStatistics[k][AWB_B][i]);
				if ((i + 1) % col_size == 0)
					fprintf(fp, "\n");
			}
		}

		fprintf(fp, "%d %u", pstWBInfo->stEnvInfo.s16LvX100, pstWBInfo->stEnvInfo.u32ISONum);
		fprintf(fp, "\n\n");
		printf("file:%s save finish!\n", fileName);
		fclose(fp);
	}
#else
	UNUSED(mode);
	UNUSED(pstWBInfo);
	UNUSED(toFile);
#endif
}

static void AWB_ShowCTCurve(CVI_U8 sID, const sWBInfo *pstWBInfo)
{
	CVI_U16 i, cnt = 0;

	printf("sID :%d Quadratic:%d\n", sID, pstWBInfo->bQuadraticPolynomial);
	if (pstWBInfo->bQuadraticPolynomial)
		printf("coef a:%f b:%f c:%f\n", pstWBInfo->fCurveCoei[0], pstWBInfo->fCurveCoei[1],
				pstWBInfo->fCurveCoei[2]);
	else
		printf("coef a:%f b:%f c:%f d:%f\n", pstWBInfo->fCurveCoei[0], pstWBInfo->fCurveCoei[1],
				pstWBInfo->fCurveCoei[2], pstWBInfo->fCurveCoei[3]);
	for (i = pstWBInfo->stAWBBound.u16RLowBound; i <= pstWBInfo->stAWBBound.u16RHighBound; i += AWB_STEP)
		printf("%d %d\n", i, pstWBInfo->u16CTCurve[cnt++]);
	printf("\n");
}

static void AWB_ShowCurInfo(CVI_U8 sID, const sWBInfo *pstWBInfo)
{
	sWBSampleInfo *pSampleInfo = NULL;
	sWBBoundary	tmpBound;
	sWBCurveInfo tmpCurve;
	pSampleInfo = AWB_Malloc(sID, sizeof(sWBSampleInfo));
	if (pSampleInfo == NULL) {
		printf("malloc pSampleInfo err\n");
		return;
	}
	AWB_GetCurrentSampleInfo(sID, pSampleInfo);
	AWB_GetCurveBoundary(sID, &tmpBound);
	AWB_GetCurveRange(sID, &tmpCurve);
	printf("R:%d-%d B:%d-%d\n", tmpBound.u16RLowBound, tmpBound.u16RHighBound,
								tmpBound.u16BLowBound, tmpBound.u16BHighBound);
	printf("CR:%d, %d %d, %d %d, %d %d, %d\n", tmpCurve.u16LowTempTopRange, tmpCurve.u16LowTempBotRange,
			tmpCurve.u16MidTempTopRange, tmpCurve.u16MidTempBotRange, tmpCurve.u16CurveWhiteTopRange,
			tmpCurve.u16CurveWhiteBotRange, tmpCurve.u16HighTempBotRange, tmpCurve.u16HighTempBotRange);
	printf("LV:%d ISO:%u\n", pstWBInfo->stEnvInfo.s16LvX100, pstWBInfo->stEnvInfo.u32ISONum);
	printf("AWB:%d %d\n", pstWBInfo->stAssignedWB.u16RGain, pstWBInfo->stAssignedWB.u16BGain);
	printf("GWB:%d %d\n", pstWBInfo->stGrayWorldWB.u16RGain, pstWBInfo->stGrayWorldWB.u16BGain);
	printf("BWB:%d %d\n", pstWBInfo->stBalanceWB.u16RGain, pstWBInfo->stBalanceWB.u16BGain);
	printf("CWB:%d %d\n", pstWBInfo->stCurrentWB.u16RGain, pstWBInfo->stCurrentWB.u16BGain);
	printf("FWB:%d %d\n", pstWBInfo->stFinalWB.u16RGain, pstWBInfo->stFinalWB.u16BGain);
	printf("rawG:%d TC:%d EC:%d GC:%d GR:%d\n", pSampleInfo->u16CentRawG, pSampleInfo->u16TotalSampleCnt,
	       pSampleInfo->u16EffectiveSampleCnt, pSampleInfo->u16GrayCnt, pSampleInfo->u8GrayRatio);
	printf("CurCT:%d\n", AWB_GetCurColorTemperatureRB(sID, pstWBInfo->stFinalWB.u16RGain,
		pstWBInfo->stFinalWB.u16BGain));
	AWB_Free(sID, pSampleInfo);
}

void AWB_SetDebugMode(CVI_U8 mode)
{
	u8AwbDebugPrintMode = mode;
	if (mode)
		printf("AWB debugM:%d\n", u8AwbDebugPrintMode);
}

CVI_U8 AWB_GetDebugMode(void)
{
	return u8AwbDebugPrintMode;
}

void AWB_SetManual(CVI_U8 sID, CVI_U8 mode)
{
	ISP_WB_ATTR_S tmpWBAttr;

	AWB_GetAttr(sID, &tmpWBAttr);
	tmpWBAttr.enOpType = mode;

	if (tmpWBAttr.enOpType == OP_TYPE_MANUAL)
		printf("sID:%d AWB Manual!\n", sID);
	else
		printf("sID:%d AWB Auto!\n", sID);

	AWB_SetAttr(sID, &tmpWBAttr);
}

static void ISP_WBDataDump(CVI_U8 sID)
{
	ISP_WB_STATISTICS_S *ptmpWBData = NULL;
	ptmpWBData = AWB_Malloc(sID, sizeof(ISP_WB_STATISTICS_S));
	if (ptmpWBData == NULL) {
		printf("malloc ptmpWBData err\n");
		return;
	}
	CVI_ISP_GetWBStatistics(sID, ptmpWBData);

	AWB_Free(sID, ptmpWBData);
}

static void AWB_ShowAttrInfo(CVI_U8 sID, CVI_BOOL ToFile)
{
#ifndef ARCH_RTOS_CV181X // TODO@CV181X
	ISP_WB_ATTR_S tmpWBAttr;
	FILE *fp;

	AWB_GetAttr(sID, &tmpWBAttr);

	printf("ByPass:%d\n", tmpWBAttr.bByPass);
	printf("OpType:%d\n", tmpWBAttr.enOpType);
	printf("AWBRunInterval:%d\n", tmpWBAttr.u8AWBRunInterval);

	printf("Manual:\n");
	printf("Rgain:%d\n", tmpWBAttr.stManual.u16Rgain);
	printf("GRgain:%d\n", tmpWBAttr.stManual.u16Grgain);
	printf("GBgain:%d\n", tmpWBAttr.stManual.u16Gbgain);
	printf("Bgain:%d\n", tmpWBAttr.stManual.u16Bgain);

	printf("Auto:\n");
	printf("bEnable:%d\n", tmpWBAttr.stAuto.bEnable);
	printf("RefColorTemp:%d\n", tmpWBAttr.stAuto.u16RefColorTemp);
	printf("StaticWB:%d %d %d %d\n", tmpWBAttr.stAuto.au16StaticWB[0], tmpWBAttr.stAuto.au16StaticWB[1],
				tmpWBAttr.stAuto.au16StaticWB[2], tmpWBAttr.stAuto.au16StaticWB[3]);
	printf("CurvePara:%d %d %d %d %d %d\n", tmpWBAttr.stAuto.as32CurvePara[0], tmpWBAttr.stAuto.as32CurvePara[1],
				tmpWBAttr.stAuto.as32CurvePara[2], tmpWBAttr.stAuto.as32CurvePara[3],
				tmpWBAttr.stAuto.as32CurvePara[4], tmpWBAttr.stAuto.as32CurvePara[5]);
	printf("RGStrength:%d\n", tmpWBAttr.stAuto.u8RGStrength);
	printf("BGStrength:%d\n", tmpWBAttr.stAuto.u8BGStrength);
	printf("Speed:%d\n", tmpWBAttr.stAuto.u16Speed);
	printf("HighColorTemp:%d\n", tmpWBAttr.stAuto.u16HighColorTemp);
	printf("LowColorTemp:%d\n", tmpWBAttr.stAuto.u16LowColorTemp);
	printf("NaturalCastEn:%d\n", tmpWBAttr.stAuto.bNaturalCastEn);
	printf("ShiftLimit:%d %d %d %d %d %d %d %d\n", tmpWBAttr.stAuto.u16ShiftLimit[0],
		tmpWBAttr.stAuto.u16ShiftLimit[1], tmpWBAttr.stAuto.u16ShiftLimit[2],
		tmpWBAttr.stAuto.u16ShiftLimit[3], tmpWBAttr.stAuto.u16ShiftLimit[4],
		tmpWBAttr.stAuto.u16ShiftLimit[5], tmpWBAttr.stAuto.u16ShiftLimit[6],
		tmpWBAttr.stAuto.u16ShiftLimit[7]);

	if (ToFile) {
		fp = fopen("WBAttr.txt", "w+");
		if (fp == NULL) {
			printf("open file:WBAttr.txt fail!\n");
			return;
		}
		fprintf(fp, "ByPass:%d\n", tmpWBAttr.bByPass);
		fprintf(fp, "OpType:%d\n", tmpWBAttr.enOpType);
		fprintf(fp, "AWBRunInterval:%d\n", tmpWBAttr.u8AWBRunInterval);

		fprintf(fp, "Manual:\n");
		fprintf(fp, "Rgain:%d\n", tmpWBAttr.stManual.u16Rgain);
		fprintf(fp, "GRgain:%d\n", tmpWBAttr.stManual.u16Grgain);
		fprintf(fp, "GBgain:%d\n", tmpWBAttr.stManual.u16Gbgain);
		fprintf(fp, "Bgain:%d\n", tmpWBAttr.stManual.u16Bgain);

		fprintf(fp, "Auto:\n");
		fprintf(fp, "bEnable:%d\n", tmpWBAttr.stAuto.bEnable);
		fprintf(fp, "RefColorTemp:%d\n", tmpWBAttr.stAuto.u16RefColorTemp);
		fprintf(fp, "StaticWB:%d %d %d %d\n", tmpWBAttr.stAuto.au16StaticWB[0],
			tmpWBAttr.stAuto.au16StaticWB[1], tmpWBAttr.stAuto.au16StaticWB[2],
			tmpWBAttr.stAuto.au16StaticWB[3]);
		fprintf(fp, "CurvePara:%d %d %d %d %d %d\n", tmpWBAttr.stAuto.as32CurvePara[0],
			tmpWBAttr.stAuto.as32CurvePara[1], tmpWBAttr.stAuto.as32CurvePara[2],
			tmpWBAttr.stAuto.as32CurvePara[3], tmpWBAttr.stAuto.as32CurvePara[4],
			tmpWBAttr.stAuto.as32CurvePara[5]);
		fprintf(fp, "RGStrength:%d\n", tmpWBAttr.stAuto.u8RGStrength);
		fprintf(fp, "BGStrength:%d\n", tmpWBAttr.stAuto.u8BGStrength);
		fprintf(fp, "Speed:%d\n", tmpWBAttr.stAuto.u16Speed);
		fprintf(fp, "HighColorTemp:%d\n", tmpWBAttr.stAuto.u16HighColorTemp);
		fprintf(fp, "LowColorTemp:%d\n", tmpWBAttr.stAuto.u16LowColorTemp);
		fprintf(fp, "NaturalCastEn:%d\n", tmpWBAttr.stAuto.bNaturalCastEn);
		fprintf(fp, "ShiftLimit:%d %d %d %d %d %d %d %d\n", tmpWBAttr.stAuto.u16ShiftLimit[0],
			tmpWBAttr.stAuto.u16ShiftLimit[1], tmpWBAttr.stAuto.u16ShiftLimit[2],
			tmpWBAttr.stAuto.u16ShiftLimit[3], tmpWBAttr.stAuto.u16ShiftLimit[4],
			tmpWBAttr.stAuto.u16ShiftLimit[5], tmpWBAttr.stAuto.u16ShiftLimit[6],
			tmpWBAttr.stAuto.u16ShiftLimit[7]);
		fclose(fp);
	}
#else
	UNUSED(sID);
	UNUSED(ToFile);
#endif
}

static void AWB_ShowMpiInfo(CVI_U8 sID)
{
	ISP_WB_Q_INFO_S stTmpWBInfo;

	AWB_GetQueryInfo(sID, &stTmpWBInfo);

	printf("Rgain:%d", stTmpWBInfo.u16Rgain);
	printf("Grgain:%d", stTmpWBInfo.u16Grgain);
	printf("Gbgain:%d", stTmpWBInfo.u16Gbgain);
	printf("Bgain:%d", stTmpWBInfo.u16Bgain);
	printf("ColorTemp:%d", stTmpWBInfo.u16ColorTemp);
	printf("InOutStatus:%d", stTmpWBInfo.enInOutStatus);
	printf("FirstStableTime:%u", stTmpWBInfo.u32FirstStableTime);
	printf("bv:%d", stTmpWBInfo.s16Bv);
}

static void AWB_ShowBootInfo(CVI_U8 sID, const sWBInfo *pstWBInfo)
{
	CVI_U8 i;

	for (i = 0; i < AWB_BOOT_MAX_FRAME; ++i)
		printf("%d %d Fid:%u R:%d B:%d K:%d CS:%d\n", sID, i, pstWBInfo->u32FrameID,
		pstWBInfo->stBootGain[i].u16RGain,
		pstWBInfo->stBootGain[i].u16BGain, pstWBInfo->u16BootColorTemp[i], pstWBInfo->bBootConvergeStatus[i]);
}

static CVI_S32 AWB_SetCurve(CVI_U8 ViPipe, ISP_AWB_Calibration_Gain_S *pstWBCalib)
{
	ISP_WB_ATTR_S tmpAwbAttr;
	CVI_U8 sID = AWB_CheckSensorID(ViPipe);

	CVI_ISP_GetWBAttr(sID, &tmpAwbAttr);
	AWB_BuildCurve(sID, pstWBCalib, &tmpAwbAttr);
	CVI_ISP_SetWBAttr(sID, &tmpAwbAttr);
	printf("awb set new curve!\n");
	return CVI_SUCCESS;
}

static void AWB_SetNewCurve(CVI_U8 sID)
{
	ISP_AWB_Calibration_Gain_S wbpts;

	wbpts.u16AvgRgain[0] = 1.21875 * AWB_GAIN_BASE;
	wbpts.u16AvgBgain[0] = 2.949218 * AWB_GAIN_BASE;
	wbpts.u16ColorTemperature[0] = 2850;

	wbpts.u16AvgRgain[1] = 1.421875 * AWB_GAIN_BASE;
	wbpts.u16AvgBgain[1] = 2.359375 * AWB_GAIN_BASE;
	wbpts.u16ColorTemperature[1] = 4150;

	wbpts.u16AvgRgain[2] = 1.953125 * AWB_GAIN_BASE;
	wbpts.u16AvgBgain[2] = 1.671825 * AWB_GAIN_BASE;
	wbpts.u16ColorTemperature[2] = 6150;

	AWB_SetCurve(sID, &wbpts);
}


static void AWB_ShowCalibration(CVI_U8 sID)
{
	CVI_U8 i;
	ISP_AWB_Calibration_Gain_S tmpWBCalib;

	AWB_GetCalibration(sID, &tmpWBCalib);
	for (i = 0; i < AWB_CALIB_PTS_NUM; ++i)
		printf("%d R:%d B:%d CT:%d\n", i, tmpWBCalib.u16AvgRgain[i],
				tmpWBCalib.u16AvgBgain[i], tmpWBCalib.u16ColorTemperature[i]);
}


#ifndef ARCH_RTOS_CV181X // TODO@CV181X
static void AWB_SetParaList(void)
{
	CVI_S32 sID, item, para1, para2, para3;

	printf("0:AWB_SetManual(0:Auto 1:Manual)\n");
	printf("1:AWB_SetAssignGain()\n");
	printf("2:AWB_SetDebugMode()\n");
	printf("3:AWB_SetByPass()\n");
	printf("4:AWB_SetGrayWorld()\n");
	printf("5:AWB_SetCurveBoundary(sID, para1, para2)\n");
	printf("6:AWB_SetRGStrength(sID, para1, para2)\n");
	printf("Item/sID/para1/para2/para3\n");
	scanf("%d %d %d %d %d", &item, &sID, &para1, &para2, &para3);

	switch (item) {
	case 0:
		AWB_SetManual(sID, para1);
		break;
	case 1:
		AWB_SetManualGain(sID, para1, para2, para3);
		break;
	case 2:
		AWB_SetDebugMode(para1);
		break;
	case 3:
		AWB_SetByPass(sID, para1);
		break;
	case 4:
		AWB_SetGrayWorld(sID, para1);
		break;
	case 5:
		AWB_SetCurveBoundary(sID, para1, para2);
		break;
	case 6:
		AWB_SetRGStrength(sID, para1, para2);
		break;
	case 7:
		AWB_SetNewCurve(sID);
		break;
	}
}
#else
static void AWB_SetParaList(void)
{

}
#endif

#ifndef ARCH_RTOS_CV181X // TODO@CV181X
void AWB_ShowInfoList(void)
{
	CVI_S32 sID, item, mode, toFile;
	sWBInfo tmpAwbInfo;

	printf("0.AWB_ShowGridRGB\n");
	printf("1.AWB_ShowCTCurve\n");
	printf("2.AWB_ShowCurInfo\n");
	printf("3.ISP_WBDataDump\n");
	printf("4.AWB_ShowMpiInfo\n");
	printf("5.AWB_ShowBootInfo\n");
	printf("6.AWB_ShowCalibration\n");
	printf("7.AWB_ShowWhiteZone\n");

	printf("Item/sID/mode/toFile:\n");
	scanf("%d %d %d %d", &item, &sID, &mode, &toFile);

	sID = AWB_CheckSensorID(sID);
	AWB_GetCurrentInfo(sID, &tmpAwbInfo);

	switch (item) {
	case 0:
		AWB_ShowRGB(mode, &tmpAwbInfo, toFile);
		break;
	case 1:
		AWB_ShowCTCurve(sID, &tmpAwbInfo);
		break;
	case 2:
		AWB_ShowCurInfo(sID, &tmpAwbInfo);
		break;
	case 3:
		ISP_WBDataDump(sID);
		break;
	case 4:
		AWB_ShowMpiInfo(sID);
		break;
	case 5:
		AWB_ShowBootInfo(sID, &tmpAwbInfo);
		break;
	case 6:
		AWB_ShowCalibration(sID);
		break;
	case 7:
		AWB_ShowWhiteZone(sID, mode);
		break;
	case 8:
		AWB_ShowAttrInfo(sID, mode);
		break;
	default:
		break;
	}
}
#else
void AWB_ShowInfoList(void)
{

}
#endif

#ifndef ARCH_RTOS_CV181X // TODO@CV181X
void AWB_Debug(void)
{
	CVI_S32 item;

	printf("0: AWB_SetParaList()\n");
	printf("1: AWB_ShowInfoList()\n");
	printf("2: AWB_DumpLog(0)\n");
	printf("3: AWB_DumpLog(1)\n");
	printf("item:\n");
	scanf("%d", &item);

	switch (item) {
	case 0:
		AWB_SetParaList();
		break;
	case 1:
		AWB_ShowInfoList();
		break;
	case 2:
		AWB_DumpLog(0);
		AWB_DumpDbgBin(0);
	break;
	case 3:
		AWB_DumpLog(1);
		AWB_DumpDbgBin(1);
		break;
	default:
		break;
	}
}
#else
void AWB_Debug(void)
{

}
#endif

static CVI_BOOL CVI_ISP_SetWBAttr_Test(VI_PIPE ViPipe)
{
	CVI_U8 sID;
	ISP_WB_ATTR_S tmpWbAttr, oldWbAttr;
	sWBInfo tmpsWBInfo;
	CVI_U32	u32RunCnt[2];

	sID = AWB_ViPipe2sID(ViPipe);
	printf("Func : %s %d\n", __func__, sID);

	CVI_ISP_GetWBAttr(ViPipe, &tmpWbAttr);
	CVI_ISP_GetWBAttr(ViPipe, &oldWbAttr);

	tmpWbAttr.u8AWBRunInterval = 1;
	CVI_ISP_SetWBAttr(ViPipe, &tmpWbAttr);

	AWB_GetCurrentInfo(sID, &tmpsWBInfo);
	u32RunCnt[0] = tmpsWBInfo.u32RunCnt;
	printf("RunCnt:%d\n", u32RunCnt[0]);
	AWB_Delay1ms(500);
	AWB_GetCurrentInfo(sID, &tmpsWBInfo);
	u32RunCnt[1] = tmpsWBInfo.u32RunCnt;
	printf("RunCnt:%d\n", u32RunCnt[1]);

	CVI_ISP_SetWBAttr(ViPipe, &oldWbAttr);

	if (u32RunCnt[1] == u32RunCnt[0])
		return CVI_FAILURE;
	return CVI_SUCCESS;
}

static CVI_BOOL CVI_ISP_SetWBAttrEx_Test(VI_PIPE ViPipe)
{
	CVI_U8 sID;
	ISP_AWB_ATTR_EX_S tmpAwbAttrEx, oldAwbAttrEx;
	sWBSampleInfo *ptmpSampInfo = NULL;
	CVI_U16 u16GrayCnt[2];
	ISP_WB_ATTR_S tmpWbAttr, oldWbAttr;

	ptmpSampInfo = AWB_Malloc(sID, sizeof(sWBSampleInfo));
	if (ptmpSampInfo == NULL) {
		printf("malloc ptmpSampInfo err\n");
		return CVI_FAILURE;
	}
	sID = AWB_ViPipe2sID(ViPipe);
	printf("Func : %s %d\n", __func__, sID);

	CVI_ISP_GetWBAttr(ViPipe, &tmpWbAttr);
	CVI_ISP_GetWBAttr(ViPipe, &oldWbAttr);

	CVI_ISP_GetAWBAttrEx(ViPipe, &tmpAwbAttrEx);
	CVI_ISP_GetAWBAttrEx(ViPipe, &oldAwbAttrEx);

	AWB_GetCurrentSampleInfo(sID, ptmpSampInfo);
	u16GrayCnt[0] = ptmpSampInfo->u16GrayCnt;

	tmpAwbAttrEx.u16CurveLLimit = 4096;
	tmpWbAttr.u8AWBRunInterval = 1;
	CVI_ISP_SetWBAttr(ViPipe, &tmpWbAttr);
	CVI_ISP_SetAWBAttrEx(ViPipe, &tmpAwbAttrEx);
	AWB_Delay1ms(500);
	AWB_GetCurrentSampleInfo(sID, ptmpSampInfo);
	u16GrayCnt[1] = ptmpSampInfo->u16GrayCnt;

	printf("Cnt %d %d\n", u16GrayCnt[0], u16GrayCnt[1]);

	CVI_ISP_SetAWBAttrEx(ViPipe, &oldAwbAttrEx);
	CVI_ISP_SetWBAttr(ViPipe, &oldWbAttr);

	AWB_Free(sID, ptmpSampInfo);

	if (u16GrayCnt[1])
		return CVI_FAILURE;

	return CVI_SUCCESS;
}

static CVI_BOOL CVI_ISP_QueryWBInfo_Test(VI_PIPE ViPipe)
{
	CVI_U8 sID;
	ISP_WB_INFO_S tmpWbInfo;
	ISP_WB_ATTR_S tmpWbAttr, oldWbAttr;

	sID = AWB_ViPipe2sID(ViPipe);
	printf("Func : %s %d\n", __func__, sID);

	CVI_ISP_GetWBAttr(ViPipe, &tmpWbAttr);
	CVI_ISP_GetWBAttr(ViPipe, &oldWbAttr);

	tmpWbAttr.enOpType = OP_TYPE_MANUAL;
	tmpWbAttr.stManual.u16Rgain = 2048;
	tmpWbAttr.u8AWBRunInterval = 1;
	CVI_ISP_SetWBAttr(ViPipe, &tmpWbAttr);

	AWB_Delay1ms(500);
	CVI_ISP_QueryWBInfo(ViPipe, &tmpWbInfo);
	printf("Rgain %d\n", tmpWbInfo.u16Rgain);
	CVI_ISP_SetWBAttr(ViPipe, &oldWbAttr);
	if (tmpWbInfo.u16Rgain != tmpWbAttr.stManual.u16Rgain)
		return CVI_FAILURE;
	return CVI_SUCCESS;
}

static CVI_BOOL CVI_ISP_CalGainByTemp_Test(VI_PIPE ViPipe)
{
	CVI_U8 sID;
	ISP_WB_ATTR_S tmpWbAttr;
	CVI_U16 tmpGain[2][4];

	sID = AWB_ViPipe2sID(ViPipe);
	printf("Func : %s %d\n", __func__, sID);

	CVI_ISP_GetWBAttr(ViPipe, &tmpWbAttr);

	AWB_GetRBGainByColorTemperature(sID, 5500, 0, &tmpGain[0][0]);
	printf("5500K R:%d ,G:%d %d,B:%d\n", tmpGain[0][ISP_BAYER_CHN_R], tmpGain[0][ISP_BAYER_CHN_GR],
		tmpGain[0][ISP_BAYER_CHN_GB], tmpGain[0][ISP_BAYER_CHN_B]);
	AWB_GetRBGainByColorTemperature(sID, 4500, 0, &tmpGain[1][0]);
	printf("4500K R:%d ,G:%d %d,B:%d\n", tmpGain[1][ISP_BAYER_CHN_R], tmpGain[1][ISP_BAYER_CHN_GR],
		tmpGain[1][ISP_BAYER_CHN_GB], tmpGain[1][ISP_BAYER_CHN_B]);
	if (tmpGain[0][ISP_BAYER_CHN_R] < tmpGain[1][ISP_BAYER_CHN_R])
		return CVI_FAILURE;
	if (tmpGain[0][ISP_BAYER_CHN_B] > tmpGain[1][ISP_BAYER_CHN_B])
		return CVI_FAILURE;

	return CVI_SUCCESS;
}

void CVI_AWB_AutoTest(VI_PIPE ViPipe)
{
	if (0) {
		if (CVI_ISP_SetWBAttr_Test(ViPipe)) {
			printf("CVI_ISP_SetWBAttr_Test fail!\n");
			return;
		}
		if (CVI_ISP_SetWBAttrEx_Test(ViPipe)) {
			printf("CVI_ISP_SetWBAttrEx_Test fail!\n");
			return;
		}
		if (CVI_ISP_QueryWBInfo_Test(ViPipe)) {
			printf("CVI_ISP_QueryWBInfo_Test fail!\n");
			return;
		}
		if (CVI_ISP_CalGainByTemp_Test(ViPipe)) {
			printf("CVI_ISP_CalGainByTemp_Test_run fail!\n");
			return;
		}
	} else {
		printf("*---------  %s success   ---------*\n", __func__);
	}
}

