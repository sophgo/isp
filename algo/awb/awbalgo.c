/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: awbalgo.c
 * Description:
 *
 */
#define AWB_RELEASE_DATE (2021500)

#define AWB_LIB_VER (6)//U8
#define AWB_LIB_SUBVER (9)//U8

/*
 *
v6.08:2023-02-02
	add adjust interference color
v6.07:2022-09-19
	add adjust awb style method
V6.06:2022-05-23
	fix some div 0
V6.05:2022-05-16
	add maxDiff limit to AWB_PreviewConverge
	save curSensor param to dbgbin
v6.04:2022-05-11
	fix AwbAttr_Init()
	AWB_GetShfitRatio() float issue
v6.03:2022-03-28
	implement shiftLv,adjBgainMode,region
V6.02:2022-03-04
	Add simulation@linux playform
V6.01:2022-02-10
	Add Attr for sky,skin,AWB_GetCTWeightByLv
	Add AWB_AdjLowHighColorTempByLv AWB_AdjGainByLv
	If the statistics are too dark, turn down G_min appropriately
V5.24:2022-01-20
	Use col & row from awbInit() awbRun ,remove AWB_ROW_SIZE AWB_COL_SIZE
	Add AWB_GetPosWgt() to get position weight for all chip
	Remove dummy code, use static variable , function
V5.23:2022-01-18
	Modify AWB_CalcCT_BinAWB to clear 1stMax region
	Modify accSum (avoid stack overflow)
V5.22:2022-01-06
	Add mix 1st 2nd max rgain bgain
	Modify AWB_PreviewConverge ,awb speed
V5.21:2021-11-19
	Add CheckSkin
V5.20:2021-11-15
	Add AWB RawReplay
V5.19:2021-11-10
	Increase WB gain to 16x
	remove dummy code
V5.18:2021-11-03
	Add AWB_BuildCT_RB_Tab()
	Modify AWB_GetCurColorTemperatureRB()
V5.17:2021-10-27
	Modify AWB_CalcCT_BinAWB() for local max WB
V5.16:2021-10-22
	change LumaHistWeight param in autoType
	change ctWeight default param
V5.15:2021-10-21
	Add AWB_GetGrayWB for some IR project.
V5.14:2021-10-18
    Add striceExtra mode for extra light
V5.13:2021-09-10
	fix WBInfo[sID]->u16FixedWBOffsetR
V5.12:2021-08-19
	add AWB_Malloc/AWB_Free ,and some memory issue
V5.11:2021-08-17
	add awbExit to avoid a large API stack size
V5.10:2021-08-09
	Use R/B to judge the conditions of interpolation and extrapolation
V5.09:2021-08-05
	Add AWB_CalculateCurveNew for new calibration
V5.08:2021-07-20
	Add u8Saturation @ Linear mode (AWB_AdjSaturation)
V5.07:2021-07-13
	reduce AWB memory size
V5.06:2021-07-08
	add adjust u8Saturation for LE & SE WB
V5.05:2021-07-01
	add AWB_AccWB_DataSE to calc ShortExp WB gain
V5.04:2021-06-21
	add bShiftLimitEn to move Bgain to close to Curve
	adjust G_Min@AWB_CalculateBalanceWBGain
V5.03:2021-06-04
	Fix AWB_GetCurColorTemperatureRB (merge mistake @V5.02)
V5.02:2021-05-26
	Adjust AWB_CalcMixColorTemp para
	add AWB_CalcMixColorTemp to adjust dual color temp
	Add AWB_AccWB_DataWild
	Reduce low light weight
	Remove AWB_CalcMixColorTemp
	add extra light shift to curve
	Reduce low light weight ,AWB_HistWeightInit()
V5.01:2021-05-25
	Fix AWB_GetCurColorTemperatureRB
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#ifndef ARCH_RTOS_CV181X
#include <stdarg.h>
#include <sys/time.h> //for gettimeofday()
#include <unistd.h>
#else
#include "FreeRTOS.h"
#include "task.h"
#endif

#if defined(AAA_PC_PLATFORM)
#include <windows.h>
#include "cvi_type.h"
#include "cvi_comm_video.h"
#include "cvi_comm_isp.h"
#include "cvi_comm_3a.h"
#include "awb_platform.h"
#include "awbalgo.h"
#include "misc.h"

#elif defined(AAA_LINUX_PLATFORM)
#include "cvi_comm_video.h"
#include "cvi_isp.h"
#include "cvi_comm_3a.h"
#include "isp_main.h"
#include "isp_debug.h"
#include "cvi_awb.h"
#include "awbalgo.h"
#include "awb_debug.h"
#include "awb_platform.h"
#include "misc.h"

#else
#include "cvi_isp.h"
#include "cvi_comm_3a.h"
#include "isp_main.h"
#include "isp_debug.h"
#include "cvi_awb.h"
#include "awbalgo.h"
#include "awb_debug.h"

#endif

#include "awb_buf.h"

#define CHECK_AWB_TIMING (0)//CHECK AWB Timing

#define USE_WT_CT_SAME   (0)//USE UNIT ColorTemp weight
#define USE_WT_HIST_SAME (0)//USE UNIT Histogram weight
#define DUMP_CTBIN_HIST  (0)//DUMP ColorTemp Histogram@PC

#define LOWTEMPBOT    (1 << 0)
#define LOWTEMPTOP    (1 << 1)
#define MIDTEMPBOT    (1 << 2)
#define MIDTEMPTOP    (1 << 3)
#define CURVEWHITEBOT (1 << 4)
#define CURVEWHITETOP (1 << 5)
#define HIGHTEMPBOT   (1 << 6)
#define HIGHTEMPTOP   (1 << 7)
#define LOWTEMPREGION    (1 << 0)
#define MIDTEMPREGION    (1 << 1)
#define CURVEWHITEREGION (1 << 2)
#define HIGHTEMPREGION   (1 << 3)

#define Is_Awb_AdvMode   (stAwbAttrInfo[sID]->stAuto.enAlgType == AWB_ALG_ADVANCE)

static AWB_CTX_S g_astAwbCtx[MAX_AWB_LIB_NUM];

CVI_U8 isNeedUpdateAttr[AWB_SENSOR_NUM], isNeedUpdateAttrEx[AWB_SENSOR_NUM];
CVI_U8 isNeedUpdateCalib[AWB_SENSOR_NUM];

static sWBBoundary stBoundaryInfo[AWB_SENSOR_NUM];
static sWBCurveInfo stCurveInfo[AWB_SENSOR_NUM];

static CVI_U8 linearMode;
static CVI_U8 AWB_Dump2File;
static CVI_U8 u8AwbDbgBinFlag[AWB_SENSOR_NUM];
static CVI_BOOL bAwbRunAbnormal[AWB_SENSOR_NUM];

static CVI_U16 u16CTWt[AWB_SENSOR_NUM][AWB_BIN_ZONE_NUM];
static CVI_U16 u16CT_OutDoorWt[AWB_SENSOR_NUM][AWB_BIN_ZONE_NUM];
static CVI_BOOL bNeedUpdateCT_OutDoor[AWB_SENSOR_NUM];
static CVI_U16 u16CurLLimRGain[AWB_SENSOR_NUM],
	u16CurLLimBGain[AWB_SENSOR_NUM];//PQtool is R/G,convert to (G/R)*AWB_GAIN_BASE
static CVI_U16 u16CurRLimRGain[AWB_SENSOR_NUM],
	u16CurRLimBGain[AWB_SENSOR_NUM];//PQtool is R/G,convert to (G/R)*AWB_GAIN_BASE

//for ISP_AWB_EXTRA_LIGHTSOURCE_INFO_S stLightInfo
static SEXTRA_LIGHT_RB sExLight;
static struct ST_SSKY_RB sSkyRB[AWB_SENSOR_NUM];

static CVI_FLOAT fCTtoRgCurveP[AWB_SENSOR_NUM][3]
	= { { CT_TO_RGAIN_CURVE_K1, CT_TO_RGAIN_CURVE_K2, CT_TO_RGAIN_CURVE_K3 },
		  { CT_TO_RGAIN_CURVE_K1, CT_TO_RGAIN_CURVE_K2, CT_TO_RGAIN_CURVE_K3 } };
static CVI_U16 u16HistWt[AWB_SENSOR_NUM][HISTO_WEIGHT_SIZE];

static char awbDumpLogPath[MAX_AWB_LOG_PATH_LENGTH] = "/var/log";
static char awbDumpLogName[MAX_AWB_LOG_PATH_LENGTH] = "";

static s_AWB_DBG_S *pAwbDbg[AWB_SENSOR_NUM];
static CVI_U8 u8CalibSts[AWB_SENSOR_NUM];
static ISP_SMART_ROI_S stFaceWB[AWB_SENSOR_NUM];//from isp_3a
static SFACE_DETECT_WB_INFO AwbFaceArea[AWB_SENSOR_NUM];
static CVI_BOOL u16ReduceLowHist[AWB_SENSOR_NUM];
static CVI_U8 isNewCurve[AWB_SENSOR_NUM];
static sWBSampleInfo *pstTmpSampeInfo[AWB_SENSOR_NUM];
static sPOINT_WB_INFO *pstPointInfo[AWB_SENSOR_NUM];

static SCT_RB_CLBT sCtRbTab[AWB_SENSOR_NUM];
static CVI_BOOL IsAwbRunning[AWB_SENSOR_NUM];
static CVI_BOOL AwbSimMode;
static CVI_S16 s16LastLvX100[AWB_SENSOR_NUM][AWB_LAST_LV_SIZE];
static CVI_U16 inxLastLv[AWB_SENSOR_NUM];
static CVI_U64 accSum[AWB_BIN_ZONE_NUM];
#define ATTR_SATURATION_ADJ_BASE (256)//from AWB.attr spec
#if CHECK_AWB_TIMING
#ifndef ARCH_RTOS_CV181X // TODO@CV181X
static struct timeval ttawb[20];
#define MEAUSURE_AWB_T(a) gettimeofday(&ttawb[a], NULL)
#else
#define MEAUSURE_AWB_T(a)
#endif
#else
#define MEAUSURE_AWB_T(a)
#endif

#ifdef WBIN_SIM
#define DBGMSG(fmt, ...) printf(fmt, ##__VA_ARGS__)
#define PRINT_FUNC(fmt, ...) ColorPrintf(PURPLE_COLOR, "== %s ==" fmt "\n", __func__, ##__VA_ARGS__)
#else
#define DBGMSG(fmt, ...)
#define PRINT_FUNC(fmt, ...)
#endif



static void AWB_LogPrintf(CVI_U8 sID, const char *szFmt, ...);
static void AWB_SaveLog(CVI_U8 sID);
static void AWB_SaveDbgBin(CVI_U8 sID);
static void AWB_LowColorTmMixDaylight(CVI_U8 sID, sWBGain *wbGain);
static void AWB_SetFaceArea(CVI_U8 sID);
static CVI_S32 AWB_BuildTempCurve(CVI_U8 sID, CVI_FLOAT *BufferX, CVI_FLOAT *BufferY, CVI_U8 Amount, CVI_FLOAT *ParaK);
static CVI_U16 AWB_GetGainByColorTemperature(CVI_U8 sID, CVI_U16 CT);
static CVI_U32 AWB_GetUSTimeDiff(const struct timeval *before, const struct timeval *after);
static CVI_U32 AWB_GetMSTimeDiff(const struct timeval *before, const struct timeval *after);
static void AWB_RunDetection(CVI_U8 sID);

AWB_CTX_S *AWB_GET_CTX(VI_PIPE ViPipe)
{
	CVI_U8 sID = AWB_CheckSensorID(ViPipe);

	return &g_astAwbCtx[sID];
}

static void AWB_SampleInfoInit(sWBSampleInfo *pstSampleInfo)
{
	CVI_U16 i;

	pstSampleInfo->u16GrayCnt = 0;
	pstSampleInfo->u8GrayRatio = 0;
	pstSampleInfo->u16CoolWhiteGrayCnt = 0;
	pstSampleInfo->u8CoolWhiteGrayRatio = 0;
	pstSampleInfo->u16ConvegerCnt = 0;
	pstSampleInfo->u8ConvegerRatio = 0;
	pstSampleInfo->u16TotalSampleCnt = 0;
	pstSampleInfo->u16EffectiveSampleCnt = 0;
	pstSampleInfo->bUsePreBalanceGain = 0;
	pstSampleInfo->u8GreenRatio = 0;
	pstSampleInfo->u16GreenCnt = 0;
	pstSampleInfo->u16FlourGrayCnt = 0;
	pstSampleInfo->u16CentRawG = 0;
	pstSampleInfo->u32GrayWeightCnt = 0;
	pstSampleInfo->u32WildCnt = 0;
	pstSampleInfo->u32WildSum[AWB_R] = 0;
	pstSampleInfo->u32WildSum[AWB_G] = 0;
	pstSampleInfo->u32WildSum[AWB_B] = 0;
	pstSampleInfo->u32SE_Cnt = 0;
	pstSampleInfo->u32SE_Sum[AWB_R] = 0;
	pstSampleInfo->u32SE_Sum[AWB_G] = 0;
	pstSampleInfo->u32SE_Sum[AWB_B] = 0;
	for (i = 0; i < AWB_BIN_ZONE_NUM; ++i) {
		pstSampleInfo->u64GrayCnt[i] = 0;
		pstSampleInfo->u64PixelSum[i][AWB_R] = 0;
		pstSampleInfo->u64PixelSum[i][AWB_G] = 0;
		pstSampleInfo->u64PixelSum[i][AWB_B] = 0;
		pstSampleInfo->u64CT_BinGrayCnt[i] = 0;
		pstSampleInfo->u64CT_BinPixelSum[i][AWB_R] = 0;
		pstSampleInfo->u64CT_BinPixelSum[i][AWB_G] = 0;
		pstSampleInfo->u64CT_BinPixelSum[i][AWB_B] = 0;
		pstSampleInfo->u64OutDoorBinGrayCnt[i] = 0;
		pstSampleInfo->u64OutDoorBinPixelSum[i][AWB_R] = 0;
		pstSampleInfo->u64OutDoorBinPixelSum[i][AWB_G] = 0;
		pstSampleInfo->u64OutDoorBinPixelSum[i][AWB_B] = 0;
		pstSampleInfo->u16CenterBinCnt[i] = 0;
	}
	pstSampleInfo->u16WDRSECnt = 0;

	for (i = 0; i < TOTAL_SAMPLE_NUM; ++i) {
		pstSampleInfo->u64TotalPixelSum[i][AWB_R] = 0;
		pstSampleInfo->u64TotalPixelSum[i][AWB_G] = 0;
		pstSampleInfo->u64TotalPixelSum[i][AWB_B] = 0;
	}

}

static void AWB_BoundaryInit(CVI_U8 sID, sWBCurveInfo *pstWBCurveInfo, sWBBoundary *pstWBBoundary)
{
	sID = AWB_CheckSensorID(sID);
	pstWBCurveInfo->u16LowTempTopRange = pstWBCurveInfo->u16LowTempBotRange = 60;
	pstWBCurveInfo->u16MidTempTopRange = pstWBCurveInfo->u16MidTempBotRange = 60;
	pstWBCurveInfo->u16HighTempTopRange = pstWBCurveInfo->u16HighTempBotRange = 60;
	pstWBCurveInfo->u16CurveWhiteTopRange = pstWBCurveInfo->u16CurveWhiteBotRange = 60;

	struct ST_ISP_AWB_REGION_S *pstRegion = &(stAwbAttrInfoEx[sID]->stRegion);

	if (pstRegion->u16Region1 < pstRegion->u16Region2 &&
		pstRegion->u16Region3 > pstRegion->u16Region2) {
		pstWBCurveInfo->u16Region1_R =
			AWB_GetGainByColorTemperature(sID, pstRegion->u16Region1);
		pstWBCurveInfo->u16Region2_R =
			AWB_GetGainByColorTemperature(sID, pstRegion->u16Region2);
		pstWBCurveInfo->u16Region3_R =
			AWB_GetGainByColorTemperature(sID, pstRegion->u16Region3);
	} else {
		pstWBCurveInfo->u16Region1_R =
			AWB_GetGainByColorTemperature(sID, CT_SHIFT_LIMIT_REGION1);
		pstWBCurveInfo->u16Region2_R =
			AWB_GetGainByColorTemperature(sID, CT_SHIFT_LIMIT_REGION2);
		pstWBCurveInfo->u16Region3_R =
			AWB_GetGainByColorTemperature(sID, CT_SHIFT_LIMIT_REGION3);
	}

	pstWBBoundary->u16RLowBound = WBInfo[sID]->stAWBBound.u16RLowBound;
	pstWBBoundary->u16RHighBound = WBInfo[sID]->stAWBBound.u16RHighBound;
	pstWBBoundary->u16BLowBound = WBInfo[sID]->stAWBBound.u16BLowBound;
	pstWBBoundary->u16BHighBound = WBInfo[sID]->stAWBBound.u16BHighBound;
}

static void AWB_CalculateCurveOld(CVI_U8 sID, CVI_FLOAT *pCurveCoei)
{
	CVI_U16 i, j, inx;
	CVI_U32 y, lastY = 0;

	sID = AWB_CheckSensorID(sID);
	if (WBInfo[sID]->stGoldenWB.u16RGain) {
		WBInfo[sID]->u16FixedWBOffsetR = (WBInfo[sID]->stDefaultWB.u16RGain * 100 +
			WBInfo[sID]->stGoldenWB.u16RGain / 2) / WBInfo[sID]->stGoldenWB.u16RGain;
	} else {
		WBInfo[sID]->u16FixedWBOffsetR = 100;
		ISP_LOG_ERR("%s\n", "GoldenR is 0");
	}
	if (WBInfo[sID]->stGoldenWB.u16BGain) {
		WBInfo[sID]->u16FixedWBOffsetB = (WBInfo[sID]->stDefaultWB.u16BGain * 100 +
			WBInfo[sID]->stGoldenWB.u16BGain / 2) / WBInfo[sID]->stGoldenWB.u16BGain;
	} else {
		WBInfo[sID]->u16FixedWBOffsetB = 100;
		ISP_LOG_ERR("%s\n", "GoldenB is 0");
	}

	if (WBInfo[sID]->u16FixedWBOffsetR == 0)
		WBInfo[sID]->u16FixedWBOffsetR = 100;
	WBInfo[sID]->stAWBBound.u16RLowBound = AWB_BOUND_LOW_R * WBInfo[sID]->u16FixedWBOffsetR / 100;
	WBInfo[sID]->stAWBBound.u16RHighBound = AWB_BOUND_HIGH_R * WBInfo[sID]->u16FixedWBOffsetR / 100;
	WBInfo[sID]->stAWBBound.u16BLowBound = AWB_BOUND_LOW_B * WBInfo[sID]->u16FixedWBOffsetB / 100;
	WBInfo[sID]->stAWBBound.u16BHighBound = AWB_BOUND_HIGH_B * WBInfo[sID]->u16FixedWBOffsetB / 100;

	for (i = WBInfo[sID]->stAWBBound.u16RLowBound; i <= WBInfo[sID]->stAWBBound.u16RHighBound; i += AWB_STEP) {
		j = i * 100 / WBInfo[sID]->u16FixedWBOffsetR;
		if (WBInfo[sID]->bQuadraticPolynomial)
			y = pCurveCoei[0] * j * j + pCurveCoei[1] * j + pCurveCoei[2];
		else { //two linear
			if (i < WBInfo[sID]->stCalibWBGain[1].u16RGain)
				y = pCurveCoei[0] * j + pCurveCoei[1];
			else
				y = pCurveCoei[2] * j + pCurveCoei[3];
		}

		y = (y * WBInfo[sID]->u16FixedWBOffsetB) / 100;
		if (y > lastY && lastY != 0) //the lefter  , the bigger
			y = lastY;
		inx = (i - WBInfo[sID]->stAWBBound.u16RLowBound) >> AWB_STEP_BIT;
		if (inx < AWB_CURVE_SIZE)
			WBInfo[sID]->u16CTCurve[inx] = y;
		//printf("%d\t%d\n", i, WBInfo[sID]->u16CTCurve[cnt]);
		lastY = y;
	}
}

static CVI_BOOL AWB_CalculateQuadraticCoeff(CVI_FLOAT *coeff, CVI_S32 *RGain, CVI_S32 *BGain)
{//TODO@CLIFF check div0
	CVI_FLOAT a, b, c;
	CVI_S32	checkRGain, checkBGain;

	b = (((BGain[2] - BGain[0]) * (RGain[1] * RGain[1] - RGain[0] * RGain[0]) -
	      (BGain[1] - BGain[0]) * (RGain[2] * RGain[2] - RGain[0] * RGain[0])) /
	     (CVI_FLOAT)((RGain[2] - RGain[0]) * (RGain[1] * RGain[1] - RGain[0] * RGain[0]) -
		     (RGain[1] - RGain[0]) * (RGain[2] * RGain[2] - RGain[0] * RGain[0])));
	a = (((BGain[1] - BGain[0]) - b * (RGain[1] - RGain[0])) /
			(CVI_FLOAT)(RGain[1] * RGain[1] - RGain[0] * RGain[0]));
	c = BGain[0] - a * RGain[0] * RGain[0] - b * RGain[0];

	checkRGain = RGain[2] - 4;
	checkBGain = a * checkRGain * checkRGain + b * checkRGain + c;

	coeff[0] = a;
	coeff[1] = b;
	coeff[2] = c;

	//printf("check R1:%d B1:%d R2:%d B2:%d\n", checkRGain, checkBGain, RGain[2], BGain[2]);
	//printf("Quadratic:%d\n", WBInfo[sID]->bQuadraticPolynomial);
	//the lefter  , the bigger
	return (checkBGain >= BGain[2]) ? 1 : 0;
}

static void AWB_CalculateLinearCoeff(CVI_FLOAT *coeff, CVI_S32 *RGain, CVI_S32 *BGain)
{//TODO@CLIFF check div0
	CVI_FLOAT a[2], b[2];
	CVI_S32 s32Diff;

	s32Diff = RGain[1] - RGain[0];
	if (s32Diff)
		a[0] = (CVI_FLOAT)(BGain[1] - BGain[0]) / s32Diff;
	else
		a[0] = 0;
	b[0] = BGain[0] - a[0] * RGain[0];

	s32Diff = RGain[2] - RGain[1];
	if (s32Diff)
		a[1] = (CVI_FLOAT)(BGain[2] - BGain[1]) / s32Diff;
	else
		a[1] = 0;
	b[1] = BGain[1] - a[1] * RGain[1];

	coeff[0] = a[0];
	coeff[1] = b[0];
	coeff[2] = a[1];
	coeff[3] = b[1];

	//printf("linear %f %f %f %f\n", a[0], b[0], a[1], b[1]);
}

static void AWB_BuildCT_RB_Tab(CVI_U8 sID, ISP_WB_ATTR_S *pwbAttr)
{
	CVI_U16 i;
	CVI_BOOL isNewClbt;
	SCT_RB_CLBT_PACK *pPack;
	CVI_FLOAT bufferX[3], bufferY[3], buParaK[3];

	pPack = (SCT_RB_CLBT_PACK *)&pwbAttr->stAuto.as32CurvePara[3];
	isNewClbt = 1;
	if (pwbAttr->stAuto.au16StaticWB[ISP_BAYER_CHN_R] == AWB_GAIN_BASE) {
		isNewClbt = 0;
	}
	for (i = 1; i < CT_RB_SIZE; i++) {
		if (pPack->rb[i - 1] > pPack->rb[i]) {
			isNewClbt = 0;
		}
	}
	PRINT_FUNC();
	if (isNewClbt) {
		for (i = 0; i < CT_RB_SIZE / 2; i++) {
			sCtRbTab[sID].ct[i * 2] = pPack->ct[i] >> 4;
			sCtRbTab[sID].ct[i * 2 + 1] = pPack->ct[i] & 0x0F;
		}
		for (i = 0; i < CT_RB_SIZE; i++) {
			sCtRbTab[sID].rb[i] = pPack->rb[i] / (CVI_FLOAT)CT_RB_PACK_RB_BASE;
			sCtRbTab[sID].ct[i] = sCtRbTab[sID].ct[i] * CT_RB_PACK_CT_ADJ + i * CT_RB_PACK_CT_STEP
				+ CT_RB_PACK_CT_START;
			DBGMSG("%d\t%f\t%d\t%d\n", i, sCtRbTab[sID].rb[i], pPack->rb[i], sCtRbTab[sID].ct[i]);
		}
	} else {
		for (i = 0; i < AWB_CALIB_PTS_NUM; ++i) {
			bufferX[i] = WBInfo[sID]->u16CalibColorTemp[i] / (CVI_FLOAT)10000.0;
			if (WBInfo[sID]->stCalibWBGain[i].u16BGain) {
				bufferY[i] = WBInfo[sID]->stCalibWBGain[i].u16RGain /
					(float)WBInfo[sID]->stCalibWBGain[i].u16BGain;
			}
			DBGMSG("%f\t%f\n", bufferX[i], bufferY[i]);
		}
		AWB_BuildTempCurve(sID, bufferX, bufferY, AWB_CALIB_PTS_NUM, buParaK);
		for (i = 0; i < CT_RB_SIZE; i++) {
			sCtRbTab[sID].ct[i] = i * CT_RB_PACK_CT_STEP + CT_RB_PACK_CT_START;
			bufferX[0] = sCtRbTab[sID].ct[i] / (CVI_FLOAT)10000.0;
			sCtRbTab[sID].rb[i]
				= bufferX[0] * bufferX[0] * buParaK[0] + bufferX[0] * buParaK[1] + buParaK[2];
			DBGMSG("%d\t%d\t%f\n", i, sCtRbTab[sID].ct[i], sCtRbTab[sID].rb[i]);
		}
	}
}
static void AWB_CalculateCurveNew(CVI_U8 sID, ISP_WB_ATTR_S *pwbAttr)
{
	CVI_FLOAT *pftmp;
	CVI_U16 i, j, inx;
	CVI_U32 y;
	CVI_FLOAT fr, fb, lastY = 0;
	CVI_FLOAT GoldenR, GoldenB;

	sID = AWB_CheckSensorID(sID);

	GoldenR = pwbAttr->stAuto.au16StaticWB[ISP_BAYER_CHN_R];
	GoldenB = pwbAttr->stAuto.au16StaticWB[ISP_BAYER_CHN_B];
	for (i = 0; i < AWB_CURVE_PARA_NUM; i++) {
		pftmp = (CVI_FLOAT *)&pwbAttr->stAuto.as32CurvePara[i];
		WBInfo[sID]->fCurveCoei[i] = *pftmp;
		DBGMSG("CN %d %f\n", pwbAttr->stAuto.as32CurvePara[i],
			WBInfo[sID]->fCurveCoei[i]);
	}
	if (WBInfo[sID]->stCalibWBGain[1].u16RGain && WBInfo[sID]->stCalibWBGain[1].u16BGain) {
		WBInfo[sID]->u16FixedWBOffsetR = (pwbAttr->stAuto.au16StaticWB[ISP_BAYER_CHN_R] * 100) /
			WBInfo[sID]->stCalibWBGain[1].u16RGain;
		WBInfo[sID]->u16FixedWBOffsetB = (pwbAttr->stAuto.au16StaticWB[ISP_BAYER_CHN_B] * 100) /
			WBInfo[sID]->stCalibWBGain[1].u16BGain;
	}
	WBInfo[sID]->stAWBBound.u16RLowBound = AWB_BOUND_LOW_R * WBInfo[sID]->u16FixedWBOffsetR / 100;
	WBInfo[sID]->stAWBBound.u16RHighBound = AWB_BOUND_HIGH_R * WBInfo[sID]->u16FixedWBOffsetR / 100;
	WBInfo[sID]->stAWBBound.u16BLowBound = AWB_BOUND_LOW_B * WBInfo[sID]->u16FixedWBOffsetB / 100;
	WBInfo[sID]->stAWBBound.u16BHighBound = AWB_BOUND_HIGH_B * WBInfo[sID]->u16FixedWBOffsetB / 100;
	WBInfo[sID]->stAWBBound.u16RHighBound = (WBInfo[sID]->stAWBBound.u16RHighBound/AWB_STEP)*AWB_STEP;
	WBInfo[sID]->stAWBBound.u16RLowBound = (WBInfo[sID]->stAWBBound.u16RLowBound/AWB_STEP)*AWB_STEP;

	for (i = WBInfo[sID]->stAWBBound.u16RHighBound; i >= WBInfo[sID]->stAWBBound.u16RLowBound; i -= AWB_STEP) {
		j = i;
		fr = GoldenR / (CVI_FLOAT)(j);
		fb = WBInfo[sID]->fCurveCoei[0] * fr * fr +
			WBInfo[sID]->fCurveCoei[1] * fr + WBInfo[sID]->fCurveCoei[2];
		if (fb > lastY && lastY != 0) //the lefter  , the bigger
			fb = lastY;
		lastY = fb;
		//printf("%f\t%f\n",fr,fb);
		if (fb) {
			y = GoldenB / fb;
		} else {
			y = 8192;
		}
		//y = (y * WBInfo[sID]->u16FixedWBOffsetB) / 100;
		inx = (i - WBInfo[sID]->stAWBBound.u16RLowBound) >> AWB_STEP_BIT;
		if (inx < AWB_CURVE_SIZE)
			WBInfo[sID]->u16CTCurve[inx] = y;
		//printf("%d\t%d\t%d\n", inx, i, WBInfo[sID]->u16CTCurve[inx]);
	}
}

void AWB_BuildCurve(CVI_U8 sID, const ISP_AWB_Calibration_Gain_S *psWBGain, ISP_WB_ATTR_S *pwbAttr)
{
	CVI_S32 RGain[3], BGain[3], ret;
	CVI_U8 i, u8CalibErrCnt = 0;
	CVI_FLOAT bufferX[3], bufferY[3], buParaK[3];
	CVI_U8 u8CalibNum = AWB_CALIB_PTS_NUM;

	sID = AWB_CheckSensorID(sID);

	for (i = 0; i < u8CalibNum; ++i) {
		RGain[i] = psWBGain->u16AvgRgain[i];
		BGain[i] = psWBGain->u16AvgBgain[i];
		WBInfo[sID]->stCalibWBGain[i].u16RGain = RGain[i];
		WBInfo[sID]->stCalibWBGain[i].u16GGain = AWB_GAIN_BASE;
		WBInfo[sID]->stCalibWBGain[i].u16BGain = BGain[i];
		WBInfo[sID]->u16CalibColorTemp[i] = psWBGain->u16ColorTemperature[i];
		printf("%d R:%d B:%d CT:%d\n", i, RGain[i], BGain[i], WBInfo[sID]->u16CalibColorTemp[i]);
		if (stWbDefCalibration[sID]->u16AvgRgain[i] == RGain[i] &&
			stWbDefCalibration[sID]->u16AvgBgain[i] == BGain[i]) {
			u8CalibErrCnt++;
		}
	}
	printf("Golden %d %d %d\n", pwbAttr->stAuto.au16StaticWB[ISP_BAYER_CHN_R],
		pwbAttr->stAuto.au16StaticWB[ISP_BAYER_CHN_GR], pwbAttr->stAuto.au16StaticWB[ISP_BAYER_CHN_B]);

	//Generate CT:Rgain Curve for AWB_GetGainByColorTemperature()
	for (i = 0; i < AWB_CALIB_PTS_NUM; ++i) {
		bufferX[i] = WBInfo[sID]->u16CalibColorTemp[i];
		bufferY[i] = WBInfo[sID]->stCalibWBGain[i].u16RGain;
		//printf("%f %f\n", bufferX[i], bufferY[i]);
	}
	ret = AWB_BuildTempCurve(sID, bufferX, bufferY, u8CalibNum, fCTtoRgCurveP[sID]);

	//Generate CT:R/B table , for AWB_GetCurColorTemperatureRB()
	AWB_BuildCT_RB_Tab(sID, pwbAttr);

	//Generate Rgain Bgain Curve by PQ tool parameter
	if (pwbAttr->stAuto.au16StaticWB[ISP_BAYER_CHN_R] != AWB_GAIN_BASE) {
		isNewCurve[sID] = 1;
		u8CalibSts[sID] = CVI_SUCCESS;
		AWB_CalculateCurveNew(sID, pwbAttr);
		return;
	}
	//Old Calib method
	isNewCurve[sID] = 0;

	if (u8CalibErrCnt == u8CalibNum)
		u8CalibSts[sID] = CVI_FAILURE;
	else
		u8CalibSts[sID] = CVI_SUCCESS;
	//printf("u8CalibErrCnt :%d %d\n", u8CalibErrCnt, u8CalibSts);

	if (RGain[0] == RGain[1] || RGain[0] == RGain[2] || RGain[1] == RGain[2]) {
		printf("Error!RGain the same!\n");
		return;
	}

	WBInfo[sID]->bQuadraticPolynomial = AWB_CalculateQuadraticCoeff(WBInfo[sID]->fCurveCoei, RGain, BGain);

	if (!WBInfo[sID]->bQuadraticPolynomial)
		AWB_CalculateLinearCoeff(WBInfo[sID]->fCurveCoei, RGain, BGain);

	//Generate Rgain Bgain Curve:Old method
	AWB_CalculateCurveOld(sID, WBInfo[sID]->fCurveCoei);

	pwbAttr->stAuto.as32CurvePara[0] = WBInfo[sID]->fCurveCoei[0] * AWB_CURVE_BASE;
	pwbAttr->stAuto.as32CurvePara[1] = WBInfo[sID]->fCurveCoei[1] * AWB_CURVE_BASE;
	pwbAttr->stAuto.as32CurvePara[2] = WBInfo[sID]->fCurveCoei[2] * AWB_CURVE_BASE;

	for (i = 0; i < AWB_CALIB_PTS_NUM; ++i) {
		bufferX[i] = WBInfo[sID]->stCalibWBGain[i].u16RGain/(float)WBInfo[sID]->stCalibWBGain[i].u16BGain;
		bufferY[i] = WBInfo[sID]->u16CalibColorTemp[i];
		//printf("%f %f\n", bufferX[i], bufferY[i]);
	}
	ret = AWB_BuildTempCurve(sID, bufferX, bufferY, u8CalibNum, buParaK);
	if (ret == CVI_SUCCESS) {
		for (i = 0; i < 3; i++) {
			pwbAttr->stAuto.as32CurvePara[3 + i] = buParaK[i] * TEMP_CURVE_BASE;
			//printf("%f %d\n", buParaK[i], pwbAttr->stAuto.as32CurvePara[3 + i]);
		}
	}

	printf("WB Quadratic:%d\n", WBInfo[sID]->bQuadraticPolynomial);
}


void AWB_SetCalibration(CVI_U8 sID, const ISP_AWB_Calibration_Gain_S *pstWBCali)
{
	if (pstWBCali == NULL || pstWbCalibration[sID] == NULL)
		return;
	sID = AWB_CheckSensorID(sID);
	*pstWbCalibration[sID] = *pstWBCali;
	AWB_SetParamUpdateFlag(sID, AWB_CALIB_UPDATE);
}

void AWB_GetCalibration(CVI_U8 sID, ISP_AWB_Calibration_Gain_S *pstWBCali)
{
	if (pstWBCali == NULL || pstWbCalibration[sID] == NULL)
		return;
	sID = AWB_CheckSensorID(sID);
	*pstWBCali = *pstWbCalibration[sID];
}

void AWB_SetCalibrationEx(CVI_U8 sID, const ISP_AWB_Calibration_Gain_S_EX *pstWBCali)
{
	if (pstWBCali == NULL || stWbCalibrationEx[sID] == NULL)
		return;
	sID = AWB_CheckSensorID(sID);
	*stWbCalibrationEx[sID] = *pstWBCali;
}

void AWB_GetCalibrationEx(CVI_U8 sID, ISP_AWB_Calibration_Gain_S_EX *pstWBCali)
{
	if (pstWBCali == NULL || stWbCalibrationEx[sID] == NULL)
		return;
	sID = AWB_CheckSensorID(sID);
	*pstWBCali = *stWbCalibrationEx[sID];
}

static void AWB_GetShfitRatio(CVI_U8 sID, CVI_FLOAT *fshiftRatio)
{
	CVI_S16 s16Lv;
	CVI_BOOL bParamVaild = CVI_FALSE;
	struct ST_ISP_AWB_SHIFT_LV_S *pstShiftlv = &(stAwbAttrInfoEx[sID]->stShiftLv);

	s16Lv = WBInfo[sID]->stEnvInfo.s16LvX100;

	if (pstShiftlv->u16HighLvThr[ISP_AWB_COLORTEMP_LOW] > pstShiftlv->u16LowLvThr[ISP_AWB_COLORTEMP_LOW] &&
		pstShiftlv->u16HighLvThr[ISP_AWB_COLORTEMP_LOW] > pstShiftlv->u16LowLvThr[ISP_AWB_COLORTEMP_HIGH] &&
		pstShiftlv->u16HighLvThr[ISP_AWB_COLORTEMP_HIGH] > pstShiftlv->u16LowLvThr[ISP_AWB_COLORTEMP_LOW] &&
		pstShiftlv->u16HighLvThr[ISP_AWB_COLORTEMP_HIGH] > pstShiftlv->u16LowLvThr[ISP_AWB_COLORTEMP_HIGH] &&
		pstShiftlv->u16HighLvRatio[ISP_AWB_COLORTEMP_LOW] != 0 &&
		pstShiftlv->u16LowLvRatio[ISP_AWB_COLORTEMP_HIGH] != 0)
		bParamVaild = CVI_TRUE;

	if (pstShiftlv->u8LowLvMode || pstShiftlv->u8HighLvMode) {
		//low colortemp
		if (pstShiftlv->u8LowLvMode &&
			s16Lv <= pstShiftlv->u16LowLvThr[ISP_AWB_COLORTEMP_LOW]) {
			if (pstShiftlv->u16LowLvCT[ISP_AWB_COLORTEMP_LOW] & LOWTEMPBOT)
				fshiftRatio[0] = pstShiftlv->u16LowLvRatio[ISP_AWB_COLORTEMP_LOW] / (float)100;
			if (pstShiftlv->u16LowLvCT[ISP_AWB_COLORTEMP_LOW] & LOWTEMPTOP)
				fshiftRatio[1] = pstShiftlv->u16LowLvRatio[ISP_AWB_COLORTEMP_LOW] / (float)100;
			if (pstShiftlv->u16LowLvCT[ISP_AWB_COLORTEMP_LOW] & MIDTEMPBOT)
				fshiftRatio[2] = pstShiftlv->u16LowLvRatio[ISP_AWB_COLORTEMP_LOW] / (float)100;
			if (pstShiftlv->u16LowLvCT[ISP_AWB_COLORTEMP_LOW] & MIDTEMPTOP)
				fshiftRatio[3] = pstShiftlv->u16LowLvRatio[ISP_AWB_COLORTEMP_LOW] / (float)100;
		} else if (pstShiftlv->u8HighLvMode &&
			s16Lv >= pstShiftlv->u16HighLvThr[ISP_AWB_COLORTEMP_LOW] &&
			bParamVaild) {
			if (pstShiftlv->u16HighLvCT[ISP_AWB_COLORTEMP_LOW] & LOWTEMPBOT)
				fshiftRatio[0] = 100 / (float)pstShiftlv->u16HighLvRatio[ISP_AWB_COLORTEMP_LOW];
			if (pstShiftlv->u16HighLvCT[ISP_AWB_COLORTEMP_LOW] & LOWTEMPTOP)
				fshiftRatio[1] = 100 / (float)pstShiftlv->u16HighLvRatio[ISP_AWB_COLORTEMP_LOW];
			if (pstShiftlv->u16HighLvCT[ISP_AWB_COLORTEMP_LOW] & MIDTEMPBOT)
				fshiftRatio[2] = 100 / (float)pstShiftlv->u16HighLvRatio[ISP_AWB_COLORTEMP_LOW];
			if (pstShiftlv->u16HighLvCT[ISP_AWB_COLORTEMP_LOW] & MIDTEMPTOP)
				fshiftRatio[3] = 100 / (float)pstShiftlv->u16HighLvRatio[ISP_AWB_COLORTEMP_LOW];
		}
		//high colortemp
		if (pstShiftlv->u8LowLvMode &&
			s16Lv <= pstShiftlv->u16LowLvThr[ISP_AWB_COLORTEMP_HIGH] &&
			pstShiftlv->u16LowLvRatio[ISP_AWB_COLORTEMP_HIGH]) {
			if (pstShiftlv->u16LowLvCT[ISP_AWB_COLORTEMP_HIGH] & CURVEWHITEBOT)
				fshiftRatio[4] = 100 / (float)pstShiftlv->u16LowLvRatio[ISP_AWB_COLORTEMP_HIGH];
			if (pstShiftlv->u16LowLvCT[ISP_AWB_COLORTEMP_HIGH] & CURVEWHITETOP)
				fshiftRatio[5] = 100 / (float)pstShiftlv->u16LowLvRatio[ISP_AWB_COLORTEMP_HIGH];
			if (pstShiftlv->u16LowLvCT[ISP_AWB_COLORTEMP_HIGH] & HIGHTEMPBOT)
				fshiftRatio[6] = 100 / (float)pstShiftlv->u16LowLvRatio[ISP_AWB_COLORTEMP_HIGH];
			if (pstShiftlv->u16LowLvCT[ISP_AWB_COLORTEMP_HIGH] & HIGHTEMPTOP)
				fshiftRatio[7] = 100 / (float)pstShiftlv->u16LowLvRatio[ISP_AWB_COLORTEMP_HIGH];
		} else if (pstShiftlv->u8HighLvMode &&
			s16Lv >= pstShiftlv->u16HighLvThr[ISP_AWB_COLORTEMP_HIGH] &&
			bParamVaild) {
			if (pstShiftlv->u16HighLvCT[ISP_AWB_COLORTEMP_HIGH] & CURVEWHITEBOT)
				fshiftRatio[4] = pstShiftlv->u16HighLvRatio[ISP_AWB_COLORTEMP_HIGH] / (float)100;
			if (pstShiftlv->u16HighLvCT[ISP_AWB_COLORTEMP_HIGH] & CURVEWHITETOP)
				fshiftRatio[5] = pstShiftlv->u16HighLvRatio[ISP_AWB_COLORTEMP_HIGH] / (float)100;
			if (pstShiftlv->u16HighLvCT[ISP_AWB_COLORTEMP_HIGH] & HIGHTEMPBOT)
				fshiftRatio[6] = pstShiftlv->u16HighLvRatio[ISP_AWB_COLORTEMP_HIGH] / (float)100;
			if (pstShiftlv->u16HighLvCT[ISP_AWB_COLORTEMP_HIGH] & HIGHTEMPTOP)
				fshiftRatio[7] = pstShiftlv->u16HighLvRatio[ISP_AWB_COLORTEMP_HIGH] / (float)100;
		}
	}
}

static void AWB_AdjCurveRangeByLV(CVI_U8 sID, sWBCurveInfo *pstWBCurveInfo)
{
	#define KEEP_RATIO_BGAIN	(2715)	//based on Sony307

	CVI_FLOAT fShiftRatio[8] = {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0};
	CVI_U16 i, u16ShiftLimit[AWB_CURVE_BOUND_NUM];
	CVI_U16 GoldenB;

	sID = AWB_CheckSensorID(sID);

	GoldenB = stAwbAttrInfo[sID]->stAuto.au16StaticWB[ISP_BAYER_CHN_B];
	if (GoldenB == AWB_GAIN_BASE)	//Old AWB Calib
		GoldenB = KEEP_RATIO_BGAIN;	// ratio to 1.0, don,t change Range
	for (i = 0; i < AWB_CURVE_BOUND_NUM; i++) {
		u16ShiftLimit[i] = GoldenB * stAwbAttrInfo[sID]->stAuto.u16ShiftLimit[i]/KEEP_RATIO_BGAIN;
	}

	AWB_GetShfitRatio(sID, fShiftRatio);

	pstWBCurveInfo->u16LowTempTopRange = u16ShiftLimit[0] * fShiftRatio[0];
	pstWBCurveInfo->u16LowTempBotRange = u16ShiftLimit[1] * fShiftRatio[1];
	pstWBCurveInfo->u16MidTempTopRange = u16ShiftLimit[2] * fShiftRatio[2];
	pstWBCurveInfo->u16MidTempBotRange = u16ShiftLimit[3] * fShiftRatio[3];
	pstWBCurveInfo->u16CurveWhiteTopRange = u16ShiftLimit[4] * fShiftRatio[4];
	pstWBCurveInfo->u16CurveWhiteBotRange = u16ShiftLimit[5] * fShiftRatio[5];
	pstWBCurveInfo->u16HighTempTopRange = u16ShiftLimit[6] * fShiftRatio[6];
	pstWBCurveInfo->u16HighTempBotRange = u16ShiftLimit[7] * fShiftRatio[7];

	pstWBCurveInfo->u16LowTempWildTopRange = AWB_WIDE_RANGE_TOP + u16ShiftLimit[0];
	pstWBCurveInfo->u16LowTempWildBotRange = AWB_WIDE_RANGE_BOT + u16ShiftLimit[1];
	pstWBCurveInfo->u16MidTempWildTopRange = AWB_WIDE_RANGE_TOP + u16ShiftLimit[2];
	pstWBCurveInfo->u16MidTempWildBotRange = AWB_WIDE_RANGE_BOT + u16ShiftLimit[3];
	pstWBCurveInfo->u16CurveWildTopRange = AWB_WIDE_RANGE_TOP + u16ShiftLimit[4];
	pstWBCurveInfo->u16CurveWildBotRange = AWB_WIDE_RANGE_BOT + u16ShiftLimit[5];
	pstWBCurveInfo->u16HighTempWildTopRange = AWB_WIDE_RANGE_TOP + u16ShiftLimit[6];
	pstWBCurveInfo->u16HighTempWildBotRange = AWB_WIDE_RANGE_BOT + u16ShiftLimit[7];
}

static void AWB_AdjCurveBoundaryByLv(CVI_U8 sID, sWBBoundary *pstWBBoundary)
{
	sID = AWB_CheckSensorID(sID);
	pstWBBoundary->u16RLowBound = WBInfo[sID]->stAWBBound.u16RLowBound;
	pstWBBoundary->u16RHighBound = WBInfo[sID]->stAWBBound.u16RHighBound;
	pstWBBoundary->u16BLowBound = WBInfo[sID]->stAWBBound.u16BLowBound;
	pstWBBoundary->u16BHighBound = WBInfo[sID]->stAWBBound.u16BHighBound;
}

static void AWB_AdjCurveBoundaryByISO(CVI_U8 sID, sWBBoundary *pstWBBoundary)
{
	CVI_U16 u16Shift, tabInx;

	tabInx = log(WBInfo[sID]->stEnvInfo.u32ISONum/100)/log(2);

	AWB_LIMIT_UPPER(tabInx, (ISP_AUTO_ISO_STRENGTH_NUM-1));

	//reduce pstWBBoundary->u16RLowBound for High ISO , Low Color Temp
	if (stAwbAttrInfo[sID]->stAuto.stCbCrTrack.bEnable) {
		u16Shift = stAwbAttrInfo[sID]->stAuto.stCbCrTrack.au16CrMax[tabInx];//R/G
		if (u16Shift)
			u16Shift = AWB_GAIN_BASE*AWB_GAIN_BASE/u16Shift;//G/R
		pstWBBoundary->u16RLowBound = u16Shift;
	} else {
		u16Shift = tabInx * (AWB_GAIN_BASE/64);//step:16,max:240
		if (pstWBBoundary->u16RLowBound > u16Shift)
			pstWBBoundary->u16RLowBound -= u16Shift;
		else
			pstWBBoundary->u16RLowBound = 0;
	}

}

static void AWB_AdjCurveBoundaryByColorTemperature(CVI_U8 sID,
sWBBoundary *pstWBBoundary, CVI_U16 lowCT, CVI_U16 highCT)
{
	CVI_U16 lowRBound, highRBound;
	CVI_U16 tmpCT;

	if (lowCT > highCT) {
		tmpCT = highCT;
		highCT = lowCT;
		lowCT = tmpCT;
	}

	lowRBound = AWB_GetGainByColorTemperature(sID, lowCT);
	highRBound = AWB_GetGainByColorTemperature(sID, highCT);

	pstWBBoundary->u16RLowBound = lowRBound;
	pstWBBoundary->u16RHighBound = highRBound;
}

static void AWB_GetWhiteBGainRange(CVI_U8 sID, CVI_U16 RGain, CVI_U16 *BGain, CVI_U16 *curveTop, CVI_U16 *curveBot,
	CVI_U16 *cWideTop, CVI_U16 *cWieBot, sWBCurveInfo *wbCurveInfo)
{
	CVI_U16 tmpBGain = 0, tmpTopRange = 0, tmpBotRange = 0;
	CVI_U16 RGainTh1, RGainTh2, RGainTh3;
	CVI_U16 RGainDiff1, RGainDiff2;
	CVI_U16 tmpWTopRange, tmpWBotRange;

	sID = AWB_CheckSensorID(sID);

	if (RGain < WBInfo[sID]->stAWBBound.u16RLowBound || RGain > WBInfo[sID]->stAWBBound.u16RHighBound) {
		*curveTop = 1;//avoid div 0 for PQ TOOL ,draw curve
		*BGain = 1;
		*curveBot = 0xffff;
		return;
	}
	tmpBGain = WBInfo[sID]->u16CTCurve[(RGain - WBInfo[sID]->stAWBBound.u16RLowBound) >> 4];

	RGainTh1 = wbCurveInfo->u16Region1_R;
	RGainTh2 = wbCurveInfo->u16Region2_R;
	RGainTh3 = wbCurveInfo->u16Region3_R;
	if (RGain < RGainTh1) { //WBInfo.LightSourceGain.TL84_R
		tmpTopRange = wbCurveInfo->u16LowTempTopRange;//u16ShiftLimit[0]
		tmpBotRange = wbCurveInfo->u16LowTempBotRange;//u16ShiftLimit[1]
		tmpWTopRange = wbCurveInfo->u16LowTempWildTopRange;
		tmpWBotRange = wbCurveInfo->u16LowTempWildBotRange;
	} else if (RGain < RGainTh2) { //8/10
		if (RGain < RGainTh1 + SMOOTH_GAIN_RANGE) {
			RGainDiff1 = RGain - RGainTh1;
			RGainDiff2 = (RGainTh1 + SMOOTH_GAIN_RANGE) - RGain;
			if ((RGainDiff1 + RGainDiff2)) {
				tmpTopRange = (RGainDiff1 * wbCurveInfo->u16MidTempTopRange +
					RGainDiff2 * wbCurveInfo->u16LowTempTopRange) / (RGainDiff1 + RGainDiff2);
				tmpBotRange = (RGainDiff1 * wbCurveInfo->u16MidTempBotRange +
					RGainDiff2 * wbCurveInfo->u16LowTempBotRange) / (RGainDiff1 + RGainDiff2);
			}
		} else {
			tmpTopRange = wbCurveInfo->u16MidTempTopRange;//u16ShiftLimit[2]
			tmpBotRange = wbCurveInfo->u16MidTempBotRange;//u16ShiftLimit[3]
		}
		tmpWTopRange = wbCurveInfo->u16MidTempWildTopRange;
		tmpWBotRange = wbCurveInfo->u16MidTempWildBotRange;
	} else if (RGain > RGainTh3) {
		if (RGain < RGainTh3 + SMOOTH_GAIN_RANGE) {
			RGainDiff1 = RGain - RGainTh3;
			RGainDiff2 = RGainTh3 + SMOOTH_GAIN_RANGE - RGain;
			if ((RGainDiff1 + RGainDiff2)) {
				tmpTopRange = (RGainDiff1 * wbCurveInfo->u16HighTempTopRange +
					RGainDiff2 * wbCurveInfo->u16CurveWhiteTopRange) / (RGainDiff1 + RGainDiff2);
				tmpBotRange = (RGainDiff1 * wbCurveInfo->u16HighTempBotRange +
					RGainDiff2 * wbCurveInfo->u16CurveWhiteBotRange) / (RGainDiff1 + RGainDiff2);
			}
		} else {
			tmpTopRange = wbCurveInfo->u16HighTempTopRange;//u16ShiftLimit[6]
			tmpBotRange = wbCurveInfo->u16HighTempBotRange;//u16ShiftLimit[7]
		}
		tmpWTopRange = wbCurveInfo->u16HighTempWildTopRange;
		tmpWBotRange = wbCurveInfo->u16HighTempWildBotRange;
	} else {
		if (RGain < RGainTh2 + SMOOTH_GAIN_RANGE) {
			RGainDiff1 = RGain - RGainTh2;
			RGainDiff2 = RGainTh2 + SMOOTH_GAIN_RANGE - RGain;
			if ((RGainDiff1 + RGainDiff2)) {
				tmpTopRange = (RGainDiff1 * wbCurveInfo->u16CurveWhiteTopRange +
					RGainDiff2 * wbCurveInfo->u16MidTempTopRange) / (RGainDiff1 + RGainDiff2);
				tmpBotRange = (RGainDiff1 * wbCurveInfo->u16CurveWhiteBotRange +
					RGainDiff2 * wbCurveInfo->u16MidTempBotRange) / (RGainDiff1 + RGainDiff2);
			}
		} else {
			tmpTopRange = wbCurveInfo->u16CurveWhiteTopRange;//u16ShiftLimit[4]
			tmpBotRange = wbCurveInfo->u16CurveWhiteBotRange;//u16ShiftLimit[5]
		}
		tmpWTopRange = wbCurveInfo->u16CurveWildTopRange;
		tmpWBotRange = wbCurveInfo->u16CurveWildBotRange;
	}
	*BGain = tmpBGain;
	*curveTop = tmpBGain + tmpTopRange;
	if (tmpBGain > tmpBotRange)
		*curveBot = tmpBGain - tmpBotRange;
	else
		*curveBot = 1;

	*cWideTop = tmpBGain + tmpWTopRange;
	if (tmpBGain > tmpWBotRange)
		*cWieBot = tmpBGain - tmpWBotRange;
	else
		*cWieBot = 1;
}

static AWB_POINT_TYPE AWB_ExtraLightWB(CVI_U8 sID, CVI_U16 RGain, CVI_U16 BGain, CVI_BOOL *pInCurve)
{
	CVI_U8 j;

	if (stAwbAttrInfo[sID]->stAuto.enAlgType != AWB_ALG_ADVANCE)
		return AWB_POINT_EXTRA_NONE;

	if (stAwbAttrInfoEx[sID]->bExtraLightEn == 0)
		return AWB_POINT_EXTRA_NONE;

	for (j = 0; j < AWB_LS_NUM; j++) {
		if (((*pInCurve) == 0) && stAwbAttrInfoEx[sID]->stLightInfo[j].u8LightStatus == AWB_EXTRA_LS_ADD) {
			CVI_U16 expQuant = stAwbAttrInfoEx[sID]->stLightInfo[j].u16ExpQuant;

			if (RGain > sExLight.ext_ls_r_min[sID][j] && RGain < sExLight.ext_ls_r_max[sID][j]) {
				if (BGain > sExLight.ext_ls_b_min[sID][j] && BGain < sExLight.ext_ls_b_max[sID][j]) {
					*pInCurve = 1;
					if (((expQuant > 50) && (expQuant < 100)) ||
						 (expQuant > 150)) {
						return AWB_POINT_EXTRA_STRICT;
					} else {
						return AWB_POINT_EXTRA_LOOSE;
					}
				}
			}
		} else if (((*pInCurve) == 1) &&
			stAwbAttrInfoEx[sID]->stLightInfo[j].u8LightStatus == AWB_EXTRA_LS_REMOVE) {
			if (RGain > sExLight.ext_ls_r_min[sID][j] && RGain < sExLight.ext_ls_r_max[sID][j]) {
				if (BGain > sExLight.ext_ls_b_min[sID][j] && BGain < sExLight.ext_ls_b_max[sID][j]) {
					*pInCurve = 0;
					return AWB_POINT_EXTRA_NONE;
				}
			}
		}
	}
	return AWB_POINT_EXTRA_NONE;
}

static void AWB_ExtraLightWBInit(CVI_U8 sID)
{
	CVI_U8 j, isLowLight;
	CVI_U16 u16Radius, u16ExpQuant, u16LowLimitDiv;
	CVI_U16 u16LvX100;

	if (stAwbAttrInfo[sID]->stAuto.enAlgType != AWB_ALG_ADVANCE)
		return;
	if (stAwbAttrInfoEx[sID]->bExtraLightEn == 0)
		return;

	sID = AWB_CheckSensorID(sID);
	if (WBInfo[sID]->stEnvInfo.s16LvX100 > 0)
		u16LvX100 = WBInfo[sID]->stEnvInfo.s16LvX100;
	else
		u16LvX100 = 0;

	for (j = 0; j < AWB_LS_NUM; j++) {
		if (stAwbAttrInfoEx[sID]->stLightInfo[j].u8LightStatus != AWB_EXTRA_LS_DONT_CARE) {
			u16Radius = stAwbAttrInfoEx[sID]->stLightInfo[j].u8Radius;
			u16ExpQuant = stAwbAttrInfoEx[sID]->stLightInfo[j].u16ExpQuant;
			if (u16ExpQuant > 100 && u16ExpQuant <= 150) {
				u16ExpQuant = u16ExpQuant % 100;
				isLowLight = 0;
			} else if (u16ExpQuant > 150) {
				u16ExpQuant = u16ExpQuant % 150;
				isLowLight = 0;
			} else if (u16ExpQuant > 50 && u16ExpQuant <= 100) {
				u16ExpQuant = u16ExpQuant % 50;
				isLowLight = 1;
			} else {
				isLowLight = 1;
			}
			u16ExpQuant = u16ExpQuant * 100;//to LVx100
			if (isLowLight) {//u16LvX100 < u16ExpQuant, enable ExtraLight
				if (u16LvX100 > u16ExpQuant) {
					u16LowLimitDiv = u16LvX100 - u16ExpQuant + 100;
					if (u16LowLimitDiv > 200) {
						u16Radius = 0;
					} else if (u16LowLimitDiv) {
						u16Radius = (u16Radius*100) / u16LowLimitDiv;
					}
				}
			} else {//u16LvX100 > u16ExpQuant, enable ExtraLight
				if (u16LvX100 < u16ExpQuant) {
					u16LowLimitDiv = u16ExpQuant - u16LvX100 + 100;
					if (u16LowLimitDiv > 200) {
						u16Radius = 0;
					} else if (u16LowLimitDiv) {
						u16Radius = (u16Radius*100) / u16LowLimitDiv;
					}
				}
			}
			if (stAwbAttrInfoEx[sID]->stLightInfo[j].u16WhiteRgain) {
				sExLight.ext_ls_r_max[sID][j] = (AWB_GAIN_BASE * AWB_GAIN_SMALL_BASE)
						/ stAwbAttrInfoEx[sID]->stLightInfo[j].u16WhiteRgain
					- u16Radius;
				sExLight.ext_ls_r_max[sID][j]
					= (AWB_GAIN_BASE * AWB_GAIN_SMALL_BASE) / sExLight.ext_ls_r_max[sID][j];
				sExLight.ext_ls_r_min[sID][j] = (AWB_GAIN_BASE * AWB_GAIN_SMALL_BASE)
						/ stAwbAttrInfoEx[sID]->stLightInfo[j].u16WhiteRgain
					+ u16Radius;
			}
			if (sExLight.ext_ls_r_min[sID][j]) {
				sExLight.ext_ls_r_min[sID][j]
					= (AWB_GAIN_BASE * AWB_GAIN_SMALL_BASE) / sExLight.ext_ls_r_min[sID][j];
			}
			if (stAwbAttrInfoEx[sID]->stLightInfo[j].u16WhiteBgain) {
				sExLight.ext_ls_b_max[sID][j] = (AWB_GAIN_BASE * AWB_GAIN_SMALL_BASE)
						/ stAwbAttrInfoEx[sID]->stLightInfo[j].u16WhiteBgain
					- u16Radius;
				sExLight.ext_ls_b_max[sID][j]
					= (AWB_GAIN_BASE * AWB_GAIN_SMALL_BASE) / sExLight.ext_ls_b_max[sID][j];
				sExLight.ext_ls_b_min[sID][j] = (AWB_GAIN_BASE * AWB_GAIN_SMALL_BASE)
						/ stAwbAttrInfoEx[sID]->stLightInfo[j].u16WhiteBgain
					+ u16Radius;
				if (sExLight.ext_ls_b_min[sID][j]) {
					sExLight.ext_ls_b_min[sID][j]
						= (AWB_GAIN_BASE * AWB_GAIN_SMALL_BASE) / sExLight.ext_ls_b_min[sID][j];
				}
			}
		}
	}

#ifdef WBIN_SIM

	PRINT_FUNC();
	printf("En:%d\n", stAwbAttrInfoEx[sID]->bExtraLightEn);
	for (j = 0; j < AWB_LS_NUM; j++) {
		printf("Extra %d st:%d,Rm:%d RM:%d Bm:%d BM:%d\n", j,
			stAwbAttrInfoEx[sID]->stLightInfo[j].u8LightStatus,
			sExLight.ext_ls_r_min[sID][j], sExLight.ext_ls_r_max[sID][j], sExLight.ext_ls_b_min[sID][j],
			sExLight.ext_ls_b_max[sID][j]);
		printf("%d %d %d %d\n", stAwbAttrInfoEx[sID]->stLightInfo[j].u16WhiteRgain,
			stAwbAttrInfoEx[sID]->stLightInfo[j].u16WhiteBgain,
			stAwbAttrInfoEx[sID]->stLightInfo[j].u16ExpQuant,
			stAwbAttrInfoEx[sID]->stLightInfo[j].u8Radius);
	}
#endif
}

static void AWB_AccWB_DataWild(CVI_U8 sID, sWBSampleInfo *pInfo, sPOINT_WB_INFO *pPtInfo)
{
	CVI_U16 RGain, BGain;
	CVI_U16 GGain = AWB_GAIN_BASE;

	UNUSED(sID);
	RGain = pPtInfo->u16RGain;
	BGain = pPtInfo->u16BGain;
	pInfo->u32WildCnt++;
	pInfo->u32WildSum[AWB_R] += RGain;//1024 base
	pInfo->u32WildSum[AWB_G] += GGain;
	pInfo->u32WildSum[AWB_B] += BGain;
}

static void AWB_AccWB_DataSE(CVI_U8 sID, sWBSampleInfo *pInfo, sPOINT_WB_INFO *pPtInfo)
{
	CVI_U16 RGain, BGain;
	CVI_U16 GGain = AWB_GAIN_BASE;

	UNUSED(sID);
	RGain = pPtInfo->u16RGain;
	BGain = pPtInfo->u16BGain;
	pInfo->u32SE_Cnt++;
	pInfo->u32SE_Sum[AWB_R] += RGain;//1024 base
	pInfo->u32SE_Sum[AWB_G] += GGain;
	pInfo->u32SE_Sum[AWB_B] += BGain;
}

static CVI_FLOAT AWB_CalStrictExtraPixel_Ratio(CVI_U8 sID, CVI_BOOL isEnableStrictExtra)
{
#define STEPRATIO (0.1)
	//If Enable strictExtra, we're going to count all of the points in this region
	//and if we don't, we're not going to count all of the points in this region
	//This causes the AWB to change too dramatically
	//so we have to add ratio to make it change smooth
	static CVI_FLOAT ratio[AWB_SENSOR_NUM];
	CVI_FLOAT stepRatio = 0;

	if ((WBInfo[sID]->u8WBConvergeMode == WB_CONVERGED_FAST) ||
		(stAwbAttrInfo[sID]->stAuto.u16Speed >= 4096)) {
		stepRatio = STEPRATIO;
	} else {
		if ((stAwbAttrInfo[sID]->stAuto.u16Speed * 100 / AWB_MAX_SPEED) <= 50) {
			stepRatio = 1;
		} else {
			stepRatio = 0.5;
		}
	}
	if (isEnableStrictExtra) {
		ratio[sID] += stepRatio;
	} else {
		ratio[sID] -= stepRatio;
	}
	ratio[sID] = LIMIT_RANGE(ratio[sID], 0.0, 1.0);
#ifdef WBIN_SIM
	ratio[sID] = 1;
#endif
	if (AwbSimMode) {
		ratio[sID]  = 1;
	}
	return ratio[sID];
}

static CVI_U16 AWB_GetPosWgt(CVI_U8 sID, CVI_U8 x, CVI_U8 y)
{
	CVI_U16 posWgt, zoneInx;

	if (!stAwbAttrInfo[sID]->stAuto.bAWBZoneWtEn) {
		return AWB_ZONE_WT_DEF;
	}

	if (WBInfo[sID]->u16WBColumnSize == 64) {//1835
		x = x >> 1;//64 => 32
	} else if (WBInfo[sID]->u16WBColumnSize == 32) {//182x
	} else if (WBInfo[sID]->u16WBColumnSize == 34) {//182xL
		//0~33 ==> 0~31
		//0->0,1->0,2->1 ... 32->31,33->31
		if (x > 0) {
			x = x - 1;
		}
		AWB_LIMIT_UPPER(x, AWB_ZONE_WT_W - 1);
	}

	zoneInx = x + y * AWB_ZONE_WT_W;//Width is always 32,sync with PQ tools
	AWB_LIMIT_UPPER(zoneInx, AWB_ZONE_WT_NUM);
	posWgt = stAwbAttrInfo[sID]->stAuto.au8ZoneWt[zoneInx];//0~255,def:8

	return posWgt;
}
static void AWB_AccWB_Data(CVI_U8 sID, sWBSampleInfo *pInfo, sPOINT_WB_INFO *pPtInfo, CVI_U16 posWgt)
{
#define MOVE_LOW_TEMP_THRES		(500)
#define MOVE_HIGH_TEMP_THRES	(100)
#define MOVE_DUSK_TEMP_THRES	(300)

	CVI_U16 binInx;
	CVI_U16 histWgt = 1, histInx;
	CVI_U16 GGain = AWB_GAIN_BASE;
	CVI_U16 u16LowCostWgt;
	CVI_U32 u32binWgt;
	CVI_U16 wbCurveBGain;
	CVI_U16 RGain, BGain;

	RGain = pPtInfo->u16RGain;
	BGain = pPtInfo->u16BGain;
	wbCurveBGain = pPtInfo->u16CurveBGain;

	binInx = RGain / AWB_BIN_STEP;

	if (stAwbAttrInfo[sID]->stAuto.bShiftLimitEn) {
		if (pPtInfo->enPointType != AWB_POINT_EXTRA_NONE) {
			BGain = (2 * wbCurveBGain + BGain) / 3;
		} else if (RGain < stCurveInfo[sID].u16Region1_R) {//low temp,reduce Bgain
			if (BGain > (wbCurveBGain + MOVE_LOW_TEMP_THRES) &&
				(stAwbAttrInfoEx[sID]->adjBgainMode & LOWTEMPREGION))
				BGain = (2 * (wbCurveBGain + MOVE_LOW_TEMP_THRES) + BGain) / 3;
		} else if (RGain > stCurveInfo[sID].u16Region3_R) {//high temp,reduce Bgain
			if (BGain > (wbCurveBGain + MOVE_HIGH_TEMP_THRES) &&
				(stAwbAttrInfoEx[sID]->adjBgainMode & HIGHTEMPREGION))
				BGain = (2 * (wbCurveBGain + MOVE_HIGH_TEMP_THRES) + BGain) / 3;
		} else if (RGain > stCurveInfo[sID].u16Region2_R &&
				   RGain < stCurveInfo[sID].u16Region3_R) {//dusk color temp,reduce Bgain
			if (BGain > (wbCurveBGain + MOVE_DUSK_TEMP_THRES) &&
				(stAwbAttrInfoEx[sID]->adjBgainMode & CURVEWHITEREGION))
				BGain = (4 * (wbCurveBGain + MOVE_HIGH_TEMP_THRES) + BGain) / 5;
		} else {
			//do nothing
		}
	}

	if (AWB_GAIN_BASE == 1024) {//shift gain to 256 base
		RGain = RGain >> 2;
		GGain = GGain >> 2;
		BGain = BGain >> 2;
	}

	if (binInx < AWB_BIN_ZONE_NUM) {
		pInfo->u16CenterBinCnt[binInx]++;
	}


	if (stAwbAttrInfo[sID]->stAuto.stLumaHist.bEnable) {
		histInx = (pPtInfo->u16GValue) >> 2;//1024->256
		if (histInx > (HISTO_WEIGHT_SIZE - 1))
			histInx = (HISTO_WEIGHT_SIZE - 1);
		histWgt = u16HistWt[sID][histInx];//max:512
		AWB_LIMIT_UPPER(histWgt, HISTO_WT_MAX);
		histWgt = histWgt >> 2;//9 bits->7 bit
	}

	if (pPtInfo->enPointType == AWB_POINT_EXTRA_STRICT) {
		u16LowCostWgt = posWgt * histWgt * pPtInfo->fStrictExtraRatio;//max 8+7 =15bits
	} else {
		u16LowCostWgt = posWgt * histWgt;//max 8+7 =15bits
	}
	//printf("%d\t%d %d %d %d\n",RGain,BGain,weight, i, j);
	//RGain:1024 ,Max point num 64x32 ,10+6+5=21 bits
	pInfo->u64TotalPixelSum[1][AWB_R] += (RGain * u16LowCostWgt);
	pInfo->u64TotalPixelSum[1][AWB_G] += (GGain * u16LowCostWgt);
	pInfo->u64TotalPixelSum[1][AWB_B] += (BGain * u16LowCostWgt);
	if (u16LowCostWgt) {
		pInfo->u16GrayCnt++;
	}
	pInfo->u32GrayWeightCnt += u16LowCostWgt;//15+11(64x32)

	if (!Is_Awb_AdvMode)//AWB_ALG_LOWCOST
		return;

	//u16LowCostWgt:min is 8 (AWB_ZONE_WT_DEF)
	if (binInx < AWB_BIN_ZONE_NUM) {
		//Overflow ,RGain:1024 ,Max point num 64x32 ,21 bits
		//256 is 1x, max is 1024 by Hisilicon
		//Normal weight form au16MultiCTWt
		u32binWgt = u16CTWt[sID][binInx];//10 bit
		u32binWgt = u32binWgt * u16LowCostWgt;//10+15=25bit
		u32binWgt = u32binWgt>>3;//22 bits
		pInfo->u64CT_BinPixelSum[binInx][AWB_R] +=
		    (RGain * u32binWgt);
		pInfo->u64CT_BinPixelSum[binInx][AWB_G] +=
		    (GGain * u32binWgt);
		pInfo->u64CT_BinPixelSum[binInx][AWB_B] +=
		    (BGain * u32binWgt);
		pInfo->u64CT_BinGrayCnt[binInx] += u32binWgt;

		pInfo->u64PixelSum[binInx][AWB_R] += RGain;
		pInfo->u64PixelSum[binInx][AWB_G] += GGain;
		pInfo->u64PixelSum[binInx][AWB_B] += BGain;
		pInfo->u64GrayCnt[binInx] += 1;
		//u16CT_OutDoorWt 32~256
		if (stAwbAttrInfoEx[sID]->stInOrOut.bEnable) {
			u32binWgt = u16CT_OutDoorWt[sID][binInx];//8 bit
			u32binWgt = u32binWgt * u16LowCostWgt;//8+15=23 bits
			u32binWgt = u32binWgt>>1;//22 bits
			pInfo->u64OutDoorBinPixelSum[binInx][AWB_R] +=
			    (RGain * u32binWgt);
			pInfo->u64OutDoorBinPixelSum[binInx][AWB_G] +=
			    (GGain * u32binWgt);
			pInfo->u64OutDoorBinPixelSum[binInx][AWB_B] +=
			    (BGain * u32binWgt);
			pInfo->u64OutDoorBinGrayCnt[binInx] += u32binWgt;
		}
	}
}

static void AWB_GetGrayPointZoneInfo(CVI_U8 radius, const sWBSampleInfo *pInfo, const CVI_U64 *pAccSum,
	stZone_Info *pZone_Info)
{
	CVI_U16 i, maxSumInx = 0, subMaxSumInx = 0, endR;
	CVI_U64 total_R = 0, total_G = 0, total_B = 0, total_Cnt = 0, maxSum = 0, subMaxSum = 0, curSum;
	CVI_U64 real_MaxZoneCnt = 0, real_SubZoneCnt = 0;

	radius = AWB_MAX(radius, 1);
	for (i = 0; i <= AWB_BIN_ZONE_NUM - radius; i++) {
		if (i) {
			curSum = *(pAccSum + i + radius - 1) - *(pAccSum + i - 1);
		} else {
			curSum = *(pAccSum + i + radius - 1);
		}

		if (maxSum < curSum) {
			maxSum = curSum;
			maxSumInx = i;
		}
	}
	endR = maxSumInx + radius;
	AWB_LIMIT_UPPER(endR, AWB_BIN_ZONE_NUM);
	for (i = maxSumInx; i < endR; i++) {
		if (pInfo->u64CT_BinGrayCnt[i]) {
			total_R += pInfo->u64CT_BinPixelSum[i][AWB_R];
			total_G += pInfo->u64CT_BinPixelSum[i][AWB_G];
			total_B += pInfo->u64CT_BinPixelSum[i][AWB_B];
			total_Cnt += pInfo->u64CT_BinGrayCnt[i];
		}
		if (pInfo->u64GrayCnt[i]) {
			real_MaxZoneCnt += pInfo->u64GrayCnt[i];
		}
	}
	if (total_R && total_B && total_Cnt) {
		pZone_Info->u64MaxZoneRgain = total_R / total_Cnt;
		pZone_Info->u64MaxZoneGgain = total_G / total_Cnt;
		pZone_Info->u64MaxZoneBgain = total_B / total_Cnt;
	}
	pZone_Info->u64MaxBinTotalWeight = total_Cnt;
	pZone_Info->u64MaxGrayTotalCnt = real_MaxZoneCnt;
	pZone_Info->u64MaxSumInx = maxSumInx;
	DBGMSG("MaxI:%d %ld %ld %d-%d Rgain:%ld Bgain:%ld\n", maxSumInx * 32, maxSum, real_MaxZoneCnt, maxSumInx, endR,
		pZone_Info->u64MaxZoneRgain, pZone_Info->u64MaxZoneBgain);
	//Check whether it is a two-color temperature scense start
	if ((AWB_BIN_ZONE_NUM - (maxSumInx + radius)) > radius) {
		for (i = maxSumInx + radius; i <= AWB_BIN_ZONE_NUM - radius; i++) {
			if (i) {
				curSum = *(pAccSum + i + radius - 1) - *(pAccSum + i - 1);
			} else {
				curSum = *(pAccSum + i + radius - 1);
			}
			if (subMaxSum < curSum) {
				subMaxSum = curSum;
				subMaxSumInx = i;
			}
		}
	}
	if (maxSumInx - radius > radius) {
		for (i = 0; i <= maxSumInx - radius; i++) {
			if (i) {
				curSum = *(pAccSum + i + radius - 1) - *(pAccSum + i - 1);
			} else {
				curSum = *(pAccSum + i + radius - 1);
			}

			if (subMaxSum < curSum) {
				subMaxSum = curSum;
				subMaxSumInx = i;
			}
		}
	}
	endR = subMaxSumInx + radius;
	AWB_LIMIT_UPPER(endR, AWB_BIN_ZONE_NUM);
	total_R = 0;
	total_G = 0;
	total_B = 0;
	total_Cnt = 0;
	for (i = subMaxSumInx; i < endR; i++) {
		if (pInfo->u64CT_BinGrayCnt[i]) {
			total_R += pInfo->u64CT_BinPixelSum[i][AWB_R];
			total_G += pInfo->u64CT_BinPixelSum[i][AWB_G];
			total_B += pInfo->u64CT_BinPixelSum[i][AWB_B];
			total_Cnt += pInfo->u64CT_BinGrayCnt[i];
		}
		if (pInfo->u64GrayCnt[i]) {
			real_SubZoneCnt += pInfo->u64GrayCnt[i];
		}
	}
	if (total_R && total_B && total_Cnt) {
		pZone_Info->u64SubZoneRgain = total_R / total_Cnt;
		pZone_Info->u64SubZoneGgain = total_G / total_Cnt;
		pZone_Info->u64SubZoneBgain = total_B / total_Cnt;
	}
	pZone_Info->u64SubBinTotalWeight = total_Cnt;
	pZone_Info->u64SubGrayTotalCnt = real_SubZoneCnt;
	pZone_Info->u64SubSumInx = subMaxSumInx;
	DBGMSG("subMaxI:%d %ld %ld %d-%d Rgain:%ld Bgain:%ld\n", subMaxSumInx * 32, subMaxSum, real_SubZoneCnt,
		subMaxSumInx, endR, pZone_Info->u64SubZoneRgain, pZone_Info->u64SubZoneBgain);
}

static void AWB_CheckInterferenceColor(CVI_U8 sID, sWBSampleInfo *pInfo, CVI_U64 *pAccSum)
{
#define DOUBLECOLORTMP_RATIO (0.2)

	CVI_U8 radius, fallingRatio, doubleCTDistance;
	CVI_FLOAT interferencelimit;

	radius = stAwbAttrInfoEx[sID]->stInterference.u8Radius;
	interferencelimit = (CVI_FLOAT)stAwbAttrInfoEx[sID]->stInterference.u8Limit / 100;
	fallingRatio = stAwbAttrInfoEx[sID]->stInterference.u8Ratio;
	doubleCTDistance = stAwbAttrInfoEx[sID]->stInterference.u8Distance;
	PRINT_FUNC();
	//If it is found to be a two-color temperature scene,
	//and one side meets the possibility of interference color,
	//we reduce interference color weight->FALLING_RATIO
	CVI_U16 i, maxSumInx = 0, subMaxSumInx = 0, InterferenceIdx = 0;
	CVI_U16 x, y, idx;
	CVI_U64 FstZoneRgain = 0, FstZoneBgain = 0;
	CVI_U64 SecZoneRgain = 0, SecZoneBgain = 0;
	CVI_U64 real_MaxZoneCnt = 0, real_SubZoneCnt = 0, lastSum = 0;
	CVI_BOOL bIsDoubleColTmp = CVI_FALSE, bIsInterferenceColor = CVI_FALSE;
	CVI_U16 fstBlockCnt = 0, secBlockCnt = 0, fstMaxGirdCnt = 0, secMaxGirdCnt = 0;
	CVI_U16 fstZoneMin = 0xfff, fstZoneMax = 0;
	CVI_U16 secZoneMin = 0xfff, secZoneMax = 0;
	CVI_U16 baseNum =  AWB_GAIN_BASE / AWB_GAIN_SMALL_BASE;
	stZone_Info stZoneInfo = {0};

	AWB_GetGrayPointZoneInfo(radius, pInfo, pAccSum, &stZoneInfo);
	FstZoneRgain = stZoneInfo.u64MaxZoneRgain;
	FstZoneBgain = stZoneInfo.u64MaxZoneBgain;
	real_MaxZoneCnt = stZoneInfo.u64MaxGrayTotalCnt;
	maxSumInx = stZoneInfo.u64MaxSumInx;
	SecZoneRgain = stZoneInfo.u64SubZoneRgain;
	SecZoneBgain = stZoneInfo.u64SubZoneBgain;
	real_SubZoneCnt = stZoneInfo.u64SubGrayTotalCnt;
	subMaxSumInx = stZoneInfo.u64SubSumInx;

	if ((AWB_ABS(maxSumInx - subMaxSumInx) >= doubleCTDistance) &&
		(real_SubZoneCnt > (real_MaxZoneCnt * DOUBLECOLORTMP_RATIO))) {
		bIsDoubleColTmp = CVI_TRUE;
		DBGMSG("CurScense is Double Color Tempture!!!!\n");
	}
	//Check whether it is a two-color temperature scense end
	//Check the screen for the presence of interference color start
	DBGMSG("diff R:%ld diff B: %ld \n", AWB_ABS((CVI_S64)(FstZoneRgain - SecZoneRgain)),  AWB_ABS((CVI_S64)(FstZoneBgain - SecZoneBgain)));
	if (bIsDoubleColTmp) {
		DBGMSG("mz_rgain =%d mz_rgain1 =%d \n", FstZoneRgain * baseNum - radius * AWB_BIN_STEP, FstZoneRgain * baseNum + radius * AWB_BIN_STEP);
		DBGMSG("mz_bgain =%d mz_bgain1 =%d \n", FstZoneBgain * baseNum - radius * AWB_BIN_STEP, FstZoneBgain * baseNum + radius * AWB_BIN_STEP);
		CVI_BOOL fstAvailable = CVI_FALSE, secAvailable =CVI_FALSE;
		CVI_FLOAT fstRatio = 0, secRatio = 0;
		for (y = 0; y < WBInfo[sID]->u16WBRowSize; ++y) {
			for (x = 0; x < WBInfo[sID]->u16WBColumnSize; ++x) {
				idx = y * WBInfo[sID]->u16WBColumnSize + x;
				if ((pstPointInfo[sID][idx].u16RGain > (FstZoneRgain * baseNum - radius * AWB_BIN_STEP) &&
					pstPointInfo[sID][idx].u16RGain < (FstZoneRgain * baseNum + radius * AWB_BIN_STEP)) &&
					(pstPointInfo[sID][idx].u16BGain > (FstZoneBgain * baseNum - radius * AWB_BIN_STEP) &&
					pstPointInfo[sID][idx].u16BGain < (FstZoneBgain * baseNum + radius * AWB_BIN_STEP))) {
						fstBlockCnt++;
						fstZoneMin = fstZoneMin > x ? x : fstZoneMin;
						fstZoneMax = fstZoneMax < x ? x : fstZoneMax;
						fstAvailable = CVI_TRUE;
					}
				if ((pstPointInfo[sID][idx].u16RGain > SecZoneRgain * baseNum - radius * AWB_BIN_STEP &&
					pstPointInfo[sID][idx].u16RGain < SecZoneRgain * baseNum + radius * AWB_BIN_STEP) &&
					(pstPointInfo[sID][idx].u16BGain > SecZoneBgain * baseNum - radius * AWB_BIN_STEP &&
					pstPointInfo[sID][idx].u16BGain < SecZoneBgain * baseNum + radius * AWB_BIN_STEP)) {
						secBlockCnt++;
						secZoneMin = secZoneMin > x ? x : secZoneMin;
						secZoneMax = secZoneMax < x ? x : secZoneMax;
						secAvailable = CVI_TRUE;
					}
			}
			if (fstAvailable) {
				fstMaxGirdCnt += (fstZoneMax - fstZoneMin + 1);
				fstAvailable = CVI_FALSE;
			}
			if (secAvailable) {
				secMaxGirdCnt += (secZoneMax - secZoneMin + 1);
				secAvailable = CVI_FALSE;
			}
			fstZoneMax = secZoneMax = 0;
			fstZoneMin = secZoneMin = 0xfff;
		}
		fstRatio = (float)fstBlockCnt / fstMaxGirdCnt;
		secRatio = (float)secBlockCnt / secMaxGirdCnt;
		DBGMSG("fstBlockCnt =%d  secBlockCnt =%d fstMaxGirdCnt =%d secMaxGirdCnt =%d\n", fstBlockCnt, secBlockCnt, fstMaxGirdCnt, secMaxGirdCnt);
		DBGMSG("fstRatio = %f secRatio = %f interferencelimit = %f\n", fstRatio, secRatio, interferencelimit);

		if (fstRatio >= interferencelimit && secRatio < interferencelimit) {
			// if (secMaxGirdCnt > fstMaxGirdCnt) {
				InterferenceIdx = maxSumInx;
				bIsInterferenceColor = CVI_TRUE;
			// }
		} else if (fstRatio < interferencelimit && secRatio >= interferencelimit) {
			// if (secMaxGirdCnt < fstMaxGirdCnt) {
				InterferenceIdx = subMaxSumInx;
				bIsInterferenceColor = CVI_TRUE;
			// }
		}

		if (bIsInterferenceColor) {
			DBGMSG("reduce interference color weight\n");
			for (i = InterferenceIdx; i < InterferenceIdx + radius; i++) {
				pInfo->u64CT_BinPixelSum[i][AWB_R] /= AWB_DIV_0_TO_1(fallingRatio);
				pInfo->u64CT_BinPixelSum[i][AWB_G] /= AWB_DIV_0_TO_1(fallingRatio);
				pInfo->u64CT_BinPixelSum[i][AWB_B] /= AWB_DIV_0_TO_1(fallingRatio);
				pInfo->u64CT_BinGrayCnt[i] /= AWB_DIV_0_TO_1(fallingRatio);
			}
			for (i = 0; i < AWB_BIN_ZONE_NUM; i++) {
				lastSum += pInfo->u64CT_BinGrayCnt[i];
				*(pAccSum + i) = lastSum;
			}
		}
	}
}

static void AWB_CheckSkin(CVI_U8 sID, sWBSampleInfo *pInfo, CVI_U64 *pAccSum)
{
#define DOUBLECOLORTMP_DISTANCE (18)
#define DOUBLECOLORTMP_RATIO (0.2)
#define SKIN_SHIFT (25)//SMALL_BASE
#define SKIN_FALLING_RATIO (2)
	CVI_U16 u16SkinRGain_Diff;
	CVI_U16 u16SkinBGain_Diff;
	CVI_U8 radius;

	//AWB_GAIN_BASE to AWB_GAIN_SMALL_BASE
	u16SkinRGain_Diff =
		stAwbAttrInfoEx[sID]->stSkin.u16RgainDiff * AWB_GAIN_SMALL_BASE / AWB_GAIN_BASE;
	u16SkinBGain_Diff =
		stAwbAttrInfoEx[sID]->stSkin.u16BgainDiff * AWB_GAIN_SMALL_BASE / AWB_GAIN_BASE;

	radius = stAwbAttrInfoEx[sID]->stSkin.u8Radius;
	radius = AWB_MAX(radius, 1);

	PRINT_FUNC();
	//If it is found to be a two-color temperature scene,
	//and one side meets the possibility of skin,
	//we reduce skin weight->SKIN_FALLING_RATIO
	CVI_U16 i, maxSumInx = 0, subMaxSumInx = 0, skinIdx = 0;
	CVI_U64 MaxZoneRgain = 0, MaxZoneBgain = 0;
	CVI_U64 SubZoneRgain = 0, SubZoneBgain = 0;
	CVI_U64 real_MaxZoneCnt = 0, real_SubZoneCnt = 0, lastSum = 0;
	CVI_BOOL bIsDoubleColTmp = CVI_FALSE;
	stZone_Info stZoneInfo = {0};

	AWB_GetGrayPointZoneInfo(radius, pInfo, pAccSum, &stZoneInfo);
	MaxZoneRgain = stZoneInfo.u64MaxZoneRgain;
	MaxZoneBgain = stZoneInfo.u64MaxZoneBgain;
	real_MaxZoneCnt = stZoneInfo.u64MaxGrayTotalCnt;
	maxSumInx = stZoneInfo.u64MaxSumInx;
	SubZoneRgain = stZoneInfo.u64SubZoneRgain;
	SubZoneBgain = stZoneInfo.u64SubZoneBgain;
	real_SubZoneCnt = stZoneInfo.u64SubGrayTotalCnt;
	subMaxSumInx = stZoneInfo.u64SubSumInx;

	if ((AWB_ABS(maxSumInx - subMaxSumInx) >= DOUBLECOLORTMP_DISTANCE) &&
		(real_SubZoneCnt > (real_MaxZoneCnt * DOUBLECOLORTMP_RATIO))) {
		bIsDoubleColTmp = CVI_TRUE;
		DBGMSG("CurScense is Double Color Tempture!!!!\n");
	}
	//Check whether it is a two-color temperature scense end
	//Check the screen for the presence of skin start
	DBGMSG("%ld %d %d\n", AWB_ABS((CVI_S64)(MaxZoneRgain - SubZoneRgain)), u16SkinRGain_Diff - SKIN_SHIFT,
		u16SkinRGain_Diff + SKIN_SHIFT);
	DBGMSG("%ld %d %d\n", AWB_ABS((CVI_S64)(MaxZoneBgain - SubZoneBgain)), u16SkinBGain_Diff - SKIN_SHIFT,
		u16SkinBGain_Diff + SKIN_SHIFT);
	if (bIsDoubleColTmp) {
		if ((AWB_ABS((CVI_S64)(MaxZoneRgain - SubZoneRgain)) > (u16SkinRGain_Diff - SKIN_SHIFT)) &&
			(AWB_ABS((CVI_S64)(MaxZoneRgain - SubZoneRgain)) < (u16SkinRGain_Diff + SKIN_SHIFT)) &&
			(AWB_ABS((CVI_S64)(MaxZoneBgain - SubZoneBgain)) > (u16SkinBGain_Diff - SKIN_SHIFT)) &&
			(AWB_ABS((CVI_S64)(MaxZoneBgain - SubZoneBgain)) < (u16SkinBGain_Diff + SKIN_SHIFT))) {
			DBGMSG("This scene includes skin, excluding skin interference\n");
			if (MaxZoneRgain > SubZoneRgain) {
				skinIdx = subMaxSumInx;
			} else {
				skinIdx = maxSumInx;
			}
			for (i = skinIdx; i < skinIdx + radius; i++) {
				pInfo->u64CT_BinPixelSum[i][AWB_R] /= SKIN_FALLING_RATIO;
				pInfo->u64CT_BinPixelSum[i][AWB_G] /= SKIN_FALLING_RATIO;
				pInfo->u64CT_BinPixelSum[i][AWB_B] /= SKIN_FALLING_RATIO;
				pInfo->u64CT_BinGrayCnt[i] /= SKIN_FALLING_RATIO;
			}
			for (i = 0; i < AWB_BIN_ZONE_NUM; i++) {
				lastSum += pInfo->u64CT_BinGrayCnt[i];
				*(pAccSum + i) = lastSum;
			}
		}
	}
}

static void AWB_CalcCT_BinAWB(CVI_U8 sID, sWBSampleInfo *pInfo, sWBGain *pGain)
{
#define MIN_2ND_RATIO (33)//33=2:1 25=3:1 20=4:1
#define MAX_2ND_RATIO (50)
#define MAX_MIX_12_RATIO (50)
#define SAME_CT_ZONE_SIZE (16)

	CVI_U16 i, endR;
	CVI_U16 rgain = 0, bgain = 0, ggain = 0, maxSumInx = 0;
	CVI_U64 lastSum, maxSum = 0, curSum, sum1st, sum2nd;
	CVI_U64 total_R = 0, total_G = 0, total_B = 0, total_Cnt = 0;
	CVI_U16 u16BinSize;
	CVI_U16 secMin, secMax;
	CVI_U16 secRgain = 0, secGgain = 0, secBgain = 0;
	CVI_U16 ratio2nd;

	pGain->u16RGain = rgain;
	pGain->u16GGain = AWB_GAIN_BASE;
	pGain->u16BGain = bgain;

	if (!Is_Awb_AdvMode)//AWB_ALG_LOWCOST
		return;
	PRINT_FUNC();

	u16BinSize = stAwbAttrInfoEx[sID]->u8ZoneRadius;
	AWB_LIMIT(u16BinSize, 4, AWB_BIN_ZONE_NUM);

	lastSum = 0;
	for (i = 0; i < AWB_BIN_ZONE_NUM; i++) {
		lastSum += pInfo->u64CT_BinGrayCnt[i];
		accSum[i] = lastSum;
#if DUMP_CTBIN_HIST
		DBGMSG("%d\t%d\t%d\t%d\t%d\t%d\t%lld\n", i, i * AWB_BIN_STEP, pInfo->u64CT_BinGrayCnt[i],
			pInfo->u64CT_BinPixelSum[i][AWB_R], pInfo->u64CT_BinPixelSum[i][AWB_G],
			pInfo->u64CT_BinPixelSum[i][AWB_B], accSum[i]);
#endif
	}

	if (stAwbAttrInfoEx[sID]->stInterference.u8Mode) {
		AWB_CheckInterferenceColor(sID, pInfo, accSum);
	}
	if (stAwbAttrInfoEx[sID]->stSkin.u8Mode) {
		AWB_CheckSkin(sID, pInfo, accSum);
	}

	for (i = 0; i <= AWB_BIN_ZONE_NUM - u16BinSize; i++) {
		if (i) {
			curSum = accSum[i + u16BinSize - 1] - accSum[i - 1];
		} else {
			curSum = accSum[i + u16BinSize - 1];
		}

		//printf("%d\n", curSum);
		if (maxSum < curSum) {
			maxSum = curSum;
			maxSumInx = i;
		}
	}
	endR = maxSumInx + u16BinSize;
	AWB_LIMIT_UPPER(endR, AWB_BIN_ZONE_NUM);
	DBGMSG("1stMaxI:%d %ld %d\n", maxSumInx, maxSum, endR);
	for (i = maxSumInx; i < endR; i++) {
		if (pInfo->u64CT_BinGrayCnt[i]) {
			total_R += pInfo->u64CT_BinPixelSum[i][AWB_R];
			total_G += pInfo->u64CT_BinPixelSum[i][AWB_G];
			total_B += pInfo->u64CT_BinPixelSum[i][AWB_B];
			total_Cnt += pInfo->u64CT_BinGrayCnt[i];
		}
	}
	if (total_R && total_B && total_Cnt) {
		rgain = total_R / total_Cnt;
		ggain = total_G / total_Cnt;
		bgain = total_B / total_Cnt;
	}
	if (ggain == AWB_GAIN_SMALL_BASE) {
		rgain = rgain * AWB_GAIN_BASE / AWB_GAIN_SMALL_BASE;
		bgain = bgain * AWB_GAIN_BASE / AWB_GAIN_SMALL_BASE;
	}
	sum1st = total_Cnt;
	/////////////////////////////////////////////
	//Find sec Max
	maxSumInx = rgain / AWB_BIN_STEP;

	if (maxSumInx >= (SAME_CT_ZONE_SIZE / 2)) {
		secMin = maxSumInx - (SAME_CT_ZONE_SIZE / 2);
	} else {
		secMin = 0;
	}
	secMax = maxSumInx + (SAME_CT_ZONE_SIZE / 2);
	AWB_LIMIT_UPPER(secMax, AWB_BIN_ZONE_NUM);
	//clear 1st max region
	lastSum = 0;
	for (i = 0; i < AWB_BIN_ZONE_NUM; i++) {
		if (i < secMin || i > (secMax)) {
			lastSum += pInfo->u64CT_BinGrayCnt[i];
		}
		accSum[i] = lastSum;
	}

	if (secMin >= (SAME_CT_ZONE_SIZE / 2)) {
		secMin = secMin - (SAME_CT_ZONE_SIZE / 2);
	} else {
		secMin = 0;
	}
	secMax = maxSumInx;

	maxSum = 0;
	maxSumInx = 0;
	for (i = 0; i <= AWB_BIN_ZONE_NUM - u16BinSize; i++) {
		if (i) {
			curSum = accSum[i + u16BinSize - 1] - accSum[i - 1];
		} else {
			curSum = accSum[i + u16BinSize - 1];
		}
		if (i < secMin || i > secMax) {
			if (maxSum < curSum) {
				maxSum = curSum;
				maxSumInx = i;
			}
		}
	}

	endR = maxSumInx + u16BinSize;
	AWB_LIMIT_UPPER(endR, AWB_BIN_ZONE_NUM);
	DBGMSG("2ndMaxI:%d %ld %d\n", maxSumInx, maxSum, endR);
	total_R = 0;
	total_G = 0;
	total_B = 0;
	total_Cnt = 0;
	for (i = maxSumInx; i < endR; i++) {
		if (pInfo->u64CT_BinGrayCnt[i]) {
			total_R += pInfo->u64CT_BinPixelSum[i][AWB_R];
			total_G += pInfo->u64CT_BinPixelSum[i][AWB_G];
			total_B += pInfo->u64CT_BinPixelSum[i][AWB_B];
			total_Cnt += pInfo->u64CT_BinGrayCnt[i];
		}
	}
	if (total_R && total_B && total_Cnt) {
		secRgain = total_R / total_Cnt;
		secGgain = total_G / total_Cnt;
		secBgain = total_B / total_Cnt;
	}
	if (secGgain == AWB_GAIN_SMALL_BASE) {
		secRgain = secRgain * AWB_GAIN_BASE / AWB_GAIN_SMALL_BASE;
		secBgain = secBgain * AWB_GAIN_BASE / AWB_GAIN_SMALL_BASE;
	}
	sum2nd = total_Cnt;
	DBGMSG("1stRB:%d %d %ld\n", rgain, bgain, sum1st);
	DBGMSG("2ndRB:%d %d %ld\n", secRgain, secBgain, sum2nd);

	if (sum1st) {
		ratio2nd = sum2nd * 100 / (sum1st + sum2nd);
		DBGMSG("r:%d\n", ratio2nd);
		AWB_LIMIT_UPPER(ratio2nd, MAX_2ND_RATIO);
		if (ratio2nd >= MIN_2ND_RATIO) {
			ratio2nd = (ratio2nd - MIN_2ND_RATIO) * MAX_MIX_12_RATIO / (MAX_2ND_RATIO - MIN_2ND_RATIO);
			DBGMSG("r:%d\n", ratio2nd);
			rgain = (rgain * (100 - ratio2nd) + secRgain * ratio2nd) / 100;
			bgain = (bgain * (100 - ratio2nd) + secBgain * ratio2nd) / 100;
		}
	}

	pGain->u16RGain = rgain;
	pGain->u16GGain = AWB_GAIN_BASE;
	pGain->u16BGain = bgain;
	DBGMSG("Bin %d %d\n", rgain, bgain);
}
static CVI_S32 _Interpulate(CVI_S32 s32Mid, CVI_S32 s32Left, CVI_S32 s32LValue, CVI_S32 s32Right, CVI_S32 s32RValue)
{
	CVI_S32 s32Value = 0;
	CVI_S32 k = 0;

	if (s32Mid <= s32Left) {
		s32Value = s32LValue;
		return s32Value;
	}

	if (s32Mid >= s32Right) {
		s32Value = s32RValue;
		return s32Value;
	}

	k = (s32Right - s32Left);
	if (k == 0) {
		s32Value = (s32LValue + s32RValue) / 2;
	} else {
		s32Value = (((s32Right - s32Mid) * s32LValue + (s32Mid - s32Left) * s32RValue + (k >> 1)) / k);
	}

	return s32Value;
}

static void AWB_GetCTWeightByLv(CVI_U8 sID, CVI_U16 *pCtBin, CVI_U16 *pCtWt)
{
	CVI_U16 i, j;
	CVI_S16 s16LvX100;

	if (stAwbAttrInfoEx[sID]->stCtLv.bEnable == 0) {//old attr,follow Hi
		for (i = 0; i < AWB_CT_BIN_NUM; i++) {
			pCtBin[i] = stAwbAttrInfoEx[sID]->au16MultiCTBin[i];
			pCtWt[i] = stAwbAttrInfoEx[sID]->au16MultiCTWt[i];
		}
		return;
	}
	s16LvX100 = WBInfo[sID]->stEnvInfo.s16LvX100;
	for (i = 0; i < AWB_CT_BIN_NUM; i++) {
		pCtBin[i] = stAwbAttrInfoEx[sID]->stCtLv.au16MultiCTBin[i];//low to High ColorTemp
		for (j = 0; j < AWB_CT_LV_NUM; j++) {
			if (s16LvX100 < (stAwbAttrInfoEx[sID]->stCtLv.s8ThrLv[j] * 100)) {//low to high light
				break;
			}
		}
		if (j == 0) {
			pCtWt[i] = stAwbAttrInfoEx[sID]->stCtLv.au16MultiCTWt[j][i];
		} else if (j == (AWB_CT_LV_NUM)) {
			pCtWt[i] = stAwbAttrInfoEx[sID]->stCtLv.au16MultiCTWt[j - 1][i];
		} else {
			pCtWt[i] = _Interpulate(s16LvX100, stAwbAttrInfoEx[sID]->stCtLv.s8ThrLv[j - 1] * 100,
				stAwbAttrInfoEx[sID]->stCtLv.au16MultiCTWt[j - 1][i],
				stAwbAttrInfoEx[sID]->stCtLv.s8ThrLv[j] * 100,
				stAwbAttrInfoEx[sID]->stCtLv.au16MultiCTWt[j][i]);
		}
	}
}
static void AWB_CT_BinWeightInit(CVI_U8 sID)
{
	CVI_U16 i, inx, rgain_bin[AWB_CT_BIN_NUM], rgain, nextRgain;
	CVI_U16 au16MultiCTBin[AWB_CT_BIN_NUM], au16AdjCTWt[AWB_CT_BIN_NUM];

	if (!Is_Awb_AdvMode)//AWB_ALG_LOWCOST
		return;
	PRINT_FUNC();
	AWB_GetCTWeightByLv(sID, au16MultiCTBin, au16AdjCTWt);
	for (i = 0; i < AWB_CT_BIN_NUM; i++) {
		rgain_bin[i] = AWB_GetGainByColorTemperature(sID, au16MultiCTBin[i]);
#ifdef WBIN_SIM
		DBGMSG("%d\t%d\t%d\t%d\n", i, au16MultiCTBin[i], rgain_bin[i], au16AdjCTWt[i]);
#endif
	}

	inx = 0;
	nextRgain = (rgain_bin[inx] + rgain_bin[inx + 1]) / 2;
	for (rgain = 0; rgain < AWB_BIN_MAX_RGAIN; rgain += AWB_BIN_STEP) {
		if (rgain > nextRgain) {
			inx++;
			if (inx >= (AWB_CT_BIN_NUM - 1)) {
				inx = AWB_CT_BIN_NUM - 1;
				nextRgain = AWB_BIN_MAX_RGAIN;
			} else {
				nextRgain = (rgain_bin[inx] + rgain_bin[inx + 1]) / 2;
			}
		}
		u16CTWt[sID][rgain / AWB_BIN_STEP] = au16AdjCTWt[inx];
		#if USE_WT_CT_SAME
		u16CTWt[sID][rgain / AWB_BIN_STEP] = 128;
		#endif
		//DBGMSG("%d\t%d\t%d\t%d\n",rgain,u16CTWt[sID][rgain/AWB_BIN_STEP],nextRgain,inx);
	}
#ifdef WBIN_SIM
	for (inx = 0; inx < AWB_BIN_MAX_RGAIN / AWB_BIN_STEP; inx++) {
		rgain = inx * AWB_BIN_STEP;
		//DBGMSG("%d\t%d\t%d\n", inx, rgain, u16CTWt[sID][inx]);
	}
#endif
}

static void AWB_CalcOutdoorBinAWB(CVI_U8 sID, sWBSampleInfo *pInfo, sWBGain *pGain)
{
	CVI_U16 i, endR;
	CVI_U16 rgain = 0, bgain = 0, ggain = 0, maxSumInx = 0;
	CVI_U64 lastSum, maxSum = 0, curSum;
	CVI_U64 total_R = 0, total_G = 0, total_B = 0, total_Cnt = 0;
	CVI_U16 u16BinSize;
	CVI_U16 local_r = 0, local_b = 0, local_st, local_end;
	CVI_U64 zone_cnt, local_cnt;
	CVI_U32 mixRatio;

	pGain->u16RGain = rgain;
	pGain->u16GGain = AWB_GAIN_BASE;
	pGain->u16BGain = bgain;

	if (!Is_Awb_AdvMode)//AWB_ALG_LOWCOST
		return;
	if (stAwbAttrInfoEx[sID]->stInOrOut.bEnable == 0)
		return;
	PRINT_FUNC();

	u16BinSize = stAwbAttrInfoEx[sID]->u8ZoneRadius;
	AWB_LIMIT(u16BinSize, 4, AWB_BIN_ZONE_NUM);

	lastSum = 0;
	for (i = 0; i < AWB_BIN_ZONE_NUM; i++) {
		lastSum += pInfo->u64OutDoorBinGrayCnt[i];
		accSum[i] = lastSum;
#if 0
		DBGMSG("%d\t%d\t%d\t%lld\t%lld\t%lld\t%lld\n", i, i * AWB_BIN_STEP, pInfo->u64OutDoorBinGrayCnt[i],
			pInfo->u64OutDoorBinPixelSum[i][AWB_R], pInfo->u64OutDoorBinPixelSum[i][AWB_G],
			pInfo->u64OutDoorBinPixelSum[i][AWB_B], accSum[i]);
#endif
	}

	for (i = 0; i <= AWB_BIN_ZONE_NUM - u16BinSize; i++) {
		if (i) {
			curSum = accSum[i + u16BinSize - 1] - accSum[i - 1];
		} else {
			curSum = accSum[i + u16BinSize - 1];
		}

		//DBGMSG("%d\n", curSum);
		if (maxSum < curSum) {
			maxSum = curSum;
			maxSumInx = i;
		}
	}
	endR = maxSumInx + u16BinSize;
	AWB_LIMIT_UPPER(endR, AWB_BIN_ZONE_NUM);
	local_st = maxSumInx;
	local_end = endR;
	//DBGMSG("MaxI:%d %lld %d\n", local_st, maxSum, local_end);
	for (i = maxSumInx; i < endR; i++) {
		if (pInfo->u64OutDoorBinGrayCnt[i]) {
			total_R += pInfo->u64OutDoorBinPixelSum[i][AWB_R];
			total_G += pInfo->u64OutDoorBinPixelSum[i][AWB_G];
			total_B += pInfo->u64OutDoorBinPixelSum[i][AWB_B];
			total_Cnt += pInfo->u64OutDoorBinGrayCnt[i];
		}
	}
	zone_cnt = total_Cnt / u16BinSize;
	if (total_R && total_B && total_Cnt) {
		rgain = total_R / total_Cnt;
		ggain = total_G / total_Cnt;
		bgain = total_B / total_Cnt;
	}
	//Find Local Max
	u16BinSize = stAwbAttrInfoEx[sID]->u8ZoneRadius/5;
	AWB_LIMIT(u16BinSize, 1, AWB_BIN_ZONE_NUM);
	maxSum = 0;
	maxSumInx = 0;
	total_R = 0;
	total_G = 0;
	total_B = 0;
	total_Cnt = 0;
	for (i = local_st; i <= local_end - u16BinSize; i++) {
		if (i) {
			curSum = accSum[i + u16BinSize - 1] - accSum[i - 1];
		} else {
			curSum = accSum[i + u16BinSize - 1];
		}
		if (maxSum < curSum) {
			maxSum = curSum;
			maxSumInx = i;
		}
	}
	endR = maxSumInx + u16BinSize;
	AWB_LIMIT_UPPER(endR, AWB_BIN_ZONE_NUM);
	//DBGMSG("MaxI:%d %lld %d\n", maxSumInx, maxSum, endR);
	for (i = maxSumInx; i < endR; i++) {
		if (pInfo->u64OutDoorBinGrayCnt[i]) {
			total_R += pInfo->u64OutDoorBinPixelSum[i][AWB_R];
			total_G += pInfo->u64OutDoorBinPixelSum[i][AWB_G];
			total_B += pInfo->u64OutDoorBinPixelSum[i][AWB_B];
			total_Cnt += pInfo->u64OutDoorBinGrayCnt[i];
		}
	}
	local_cnt = total_Cnt / u16BinSize;

	if (total_R && total_B && total_Cnt) {
		local_r = total_R / total_Cnt;
		local_b = total_B / total_Cnt;
	}
	//Find Local Max End
	if (ggain == AWB_GAIN_SMALL_BASE) {
		rgain = rgain*AWB_GAIN_BASE/AWB_GAIN_SMALL_BASE;
		bgain = bgain*AWB_GAIN_BASE/AWB_GAIN_SMALL_BASE;
		local_r = local_r*AWB_GAIN_BASE/AWB_GAIN_SMALL_BASE;
		local_b = local_b*AWB_GAIN_BASE/AWB_GAIN_SMALL_BASE;
	}
	DBGMSG("OutBin1 %d %d\n", rgain, bgain);
	DBGMSG("local %d %d %lu\n", local_r, local_b, total_Cnt);
	if (local_r && stAwbAttrInfoEx[sID]->bFineTunEn && zone_cnt) {
		mixRatio = (128 * local_cnt / zone_cnt);
		DBGMSG("mixRatio :%ld %ld %d\n", local_cnt, zone_cnt, mixRatio);
		AWB_LIMIT(mixRatio, 128, 192);//max 0.5
		mixRatio -= 128;
		rgain = (rgain * (128 - mixRatio) + local_r * mixRatio) / 128;
		bgain = (bgain * (128 - mixRatio) + local_b * mixRatio) / 128;
	}
	pGain->u16RGain = rgain;
	pGain->u16GGain = AWB_GAIN_BASE;
	pGain->u16BGain = bgain;
	DBGMSG("OutBin2 %d %d\n", rgain, bgain);
}

static void AWB_BinOutdoorWeightInit(CVI_U8 sID)
{
	CVI_U8 err = 0;
	CVI_U16 rgain, wgt;
	CVI_U16 u16LowStop, u16LowStart, u16HighStart, u16HighStop;
	CVI_U16 lowStopR, lowStartR, highStopR, highStartR;
	CVI_FLOAT ratio_low, ratio_high;

	if (bNeedUpdateCT_OutDoor[sID] == 0) {
		return;
	}
	u16LowStop = stAwbAttrInfoEx[sID]->stInOrOut.u16LowStop;
	u16LowStart = stAwbAttrInfoEx[sID]->stInOrOut.u16LowStart;
	u16HighStart = stAwbAttrInfoEx[sID]->stInOrOut.u16HighStart;
	u16HighStop = stAwbAttrInfoEx[sID]->stInOrOut.u16HighStop;

	if (!Is_Awb_AdvMode)//AWB_ALG_LOWCOST
		return;
	if (stAwbAttrInfoEx[sID]->stInOrOut.bEnable == 0)
		return;
	//u16LowStop <u16LowStart<u16HighStart<u16HighStop
	if (u16LowStop >= u16LowStart)
		err = 1;
	if (u16LowStart >= u16HighStart)
		err = 1;
	if (u16HighStart >= u16HighStop)
		err = 1;
	if (err) {
		stAwbAttrInfoEx[sID]->stInOrOut.bEnable = 0;
		#ifdef WBIN_SIM
		MessageBox(0, "BinOutdoorWeightInit", "Warning", MB_OK);
		#endif
		return;
	}
	PRINT_FUNC();
	lowStopR = AWB_GetGainByColorTemperature(sID, u16LowStop);
	lowStartR = AWB_GetGainByColorTemperature(sID, u16LowStart);
	highStartR = AWB_GetGainByColorTemperature(sID, u16HighStart);
	highStopR = AWB_GetGainByColorTemperature(sID, u16HighStop);
	DBGMSG("InOut init:%d\n", stAwbAttrInfoEx[sID]->stInOrOut.u32OutThresh);
	DBGMSG("%d %d %d %d\n", u16LowStop, u16LowStart, u16HighStart, u16HighStop);
	DBGMSG("%d %d %d %d\n", lowStopR, lowStartR, highStartR, highStopR);
	if ((lowStartR == lowStopR) && (highStopR == highStartR)) {
		stAwbAttrInfoEx[sID]->stInOrOut.bEnable = 0;
		#ifdef WBIN_SIM
		MessageBox(0, "BinOutdoorWeightInit 2", "Warning", MB_OK);
		#endif
		return;
	}
	ratio_low = (OUT_DOOR_WT_HIGH - OUT_DOOR_WT_NORMAL) / (CVI_FLOAT)(lowStartR - lowStopR);
	ratio_high = (OUT_DOOR_WT_HIGH - OUT_DOOR_WT_NORMAL) / (CVI_FLOAT)(highStopR - highStartR);

	for (rgain = 0; rgain < AWB_BIN_MAX_RGAIN; rgain += AWB_BIN_STEP) {
		if (rgain <= lowStopR) {
			wgt = OUT_DOOR_WT_NORMAL;
		} else if (rgain >= highStopR) {
			wgt = OUT_DOOR_WT_NORMAL;
		} else if (rgain >= lowStartR && rgain <= highStartR) {
			wgt = OUT_DOOR_WT_HIGH;
		} else {
			if (rgain < lowStartR) {
				wgt = OUT_DOOR_WT_NORMAL + (rgain - lowStopR) * ratio_low;
			} else {
				wgt = OUT_DOOR_WT_HIGH - (rgain - highStartR) * ratio_high;
			}
		}
		u16CT_OutDoorWt[sID][rgain / AWB_BIN_STEP] = wgt;
		//DBGMSG("%d\t%d\n", rgain, wgt);
	}
	bNeedUpdateCT_OutDoor[sID] = 0;
}

static void AWB_MixOutdoorAWB(CVI_U8 sID, sWBGain *pBinGain, sWBGain *pOutDoorGain)
{
	static CVI_S16 outdoorRatio[AWB_SENSOR_NUM];
	CVI_BOOL isOutdoor;
	CVI_U16 rgain, bgain;
	CVI_S16 s16Thresh;
	CVI_S16 s16MaxR = 256;

	sID = AWB_CheckSensorID(sID);

	if (!Is_Awb_AdvMode)//AWB_ALG_LOWCOST
		return;
	if (stAwbAttrInfoEx[sID]->stInOrOut.bEnable == 0)
		return;
	if (pBinGain->u16RGain == 0)
		return;
	if (pOutDoorGain->u16RGain == 0)
		return;
	PRINT_FUNC();

	s16Thresh = stAwbAttrInfoEx[sID]->stInOrOut.u32OutThresh*100;
	if (stAwbAttrInfoEx[sID]->stInOrOut.enOpType == OP_TYPE_AUTO) {
		if (WBInfo[sID]->stEnvInfo.s16LvX100 >= s16Thresh) {
			stAwbAttrInfoEx[sID]->stInOrOut.enOutdoorStatus = AWB_OUTDOOR_MODE;
			s16MaxR = ((WBInfo[sID]->stEnvInfo.s16LvX100 - s16Thresh) * 256)/100;
			AWB_LIMIT_UPPER(s16MaxR, 256);
		} else {
			stAwbAttrInfoEx[sID]->stInOrOut.enOutdoorStatus = AWB_INDOOR_MODE;
		}
	}
	isOutdoor = stAwbAttrInfoEx[sID]->stInOrOut.enOutdoorStatus;

	if (isOutdoor == AWB_OUTDOOR_MODE) {
		outdoorRatio[sID] += 16;
		#ifdef WBIN_SIM
		outdoorRatio[sID] = 256;
		#endif
		if (AwbSimMode) {
			outdoorRatio[sID] = 256;
		}
	} else {
		outdoorRatio[sID] -= 16;
		#ifdef WBIN_SIM
		outdoorRatio[sID] = 0;
		#endif
		if (AwbSimMode) {
			outdoorRatio[sID] = 0;
		}
	}
	AWB_LIMIT(outdoorRatio[sID], 0, s16MaxR);

	if (stAwbAttrInfoEx[sID]->stCtLv.bEnable != 0) {//old attr,follow Hi
		return;
	}
	DBGMSG("CT:%d %d\n", pBinGain->u16RGain, pBinGain->u16BGain);
	DBGMSG("OD:%d %d\n", pOutDoorGain->u16RGain, pOutDoorGain->u16BGain);

	rgain = (outdoorRatio[sID] * pOutDoorGain->u16RGain + (256-outdoorRatio[sID]) * pBinGain->u16RGain)/256;
	bgain = (outdoorRatio[sID] * pOutDoorGain->u16BGain + (256-outdoorRatio[sID]) * pBinGain->u16BGain)/256;
	pBinGain->u16RGain = rgain;
	pBinGain->u16BGain = bgain;
	DBGMSG("outR:%d %d %d lvTh%d\n", outdoorRatio[sID], rgain, bgain, s16Thresh);
}

static void AWB_CalcMixColorTemp(CVI_U8 sID, sWBGain *pBinGain,
	sWBCurveInfo *pCurInfo, sWBSampleInfo *pInfo)
{
#define H_L_RATIO_THRES	(200)	// highCnt > 2*lowCnt
#define H_L_RATIO_SLOPE	(30)
#define H_L_RATIO_MAX	(60)
#define H_L_TMP_GRAY_THRES	(15)
#define H_L_TMP_LC_THRES	(600)

	CVI_U16 i, maxSumInx, rgain, bgain, ttlCnt, lvGain;
	CVI_U16 centCnt, centGreyRatio, lowCnt, highCnt, highRgain, highRatio;
	CVI_U16 wbCurveTopRange, wbCurveBotRange, highBgain;
	CVI_U16 wbWideTopRange, wbWideBotRange;
	CVI_U64 total_R;

	ttlCnt = pInfo->u16TotalSampleCnt;

	if (!Is_Awb_AdvMode)//AWB_ALG_LOWCOST
		return;
	if (!ttlCnt)
		return;
	if (WBInfo[sID]->stEnvInfo.s16LvX100 > H_L_TMP_LC_THRES)
		return;
	PRINT_FUNC();
	if (WBInfo[sID]->stEnvInfo.s16LvX100 < (H_L_TMP_LC_THRES-100)) {
		lvGain = 100;
	} else {
		lvGain = H_L_TMP_LC_THRES - (WBInfo[sID]->stEnvInfo.s16LvX100);
	}
	rgain = pBinGain->u16RGain;
	bgain = pBinGain->u16BGain;

	maxSumInx = rgain / AWB_BIN_STEP;
	lowCnt = 0;
	highCnt = 0;
	total_R = 0;

	for (i = 0 ; i <= maxSumInx; i++) {
		lowCnt += pInfo->u16CenterBinCnt[i];
	}
	for (i = maxSumInx+1; i < AWB_BIN_ZONE_NUM; i++) {
		highCnt += pInfo->u16CenterBinCnt[i];
		total_R += (pInfo->u16CenterBinCnt[i]*i*AWB_BIN_STEP);
	}
	centCnt = lowCnt + highCnt;
	if (highCnt) {
		highRgain = total_R/highCnt;
		AWB_GetWhiteBGainRange(sID, highRgain, &highBgain, &wbCurveTopRange,
				&wbCurveBotRange, &wbWideTopRange, &wbWideBotRange, pCurInfo);
		if (lowCnt) {
			highRatio = highCnt * 100 / lowCnt;
		} else {
			highRatio = 5 * 100;
		}
		if (highRatio > H_L_RATIO_THRES) {// highCnt > 2*lowCnt
			highRatio = (highRatio - H_L_RATIO_THRES) * H_L_RATIO_SLOPE / 100;
		} else {
			highRatio = 0;
		}
		AWB_LIMIT_UPPER(highRatio, H_L_RATIO_MAX);
		centGreyRatio = centCnt * 100 / (ttlCnt);
		if (centGreyRatio < H_L_TMP_GRAY_THRES) {
			highRatio = 0;
		} else if (centGreyRatio < (H_L_TMP_GRAY_THRES+10)) {
			highRatio = highRatio * (centGreyRatio - H_L_TMP_GRAY_THRES)/10;
		}
		highRatio = highRatio * lvGain / 100;
		//highRatio = 0;
		pBinGain->u16RGain = (rgain*(128-highRatio)+highRgain*highRatio)/128;
		pBinGain->u16BGain = (bgain*(128-highRatio)+highBgain*highRatio)/128;
	}
#ifdef WBIN_SIM
	DBGMSG("== MIX High==\n");
	DBGMSG("max:%d LC:%d HC:%d TC:%d CG:%d\n", maxSumInx, lowCnt, highCnt, ttlCnt, centGreyRatio);
	DBGMSG("HR:%d %d %d\n", highRgain, highBgain, highRatio);
	DBGMSG("ORB %d %d\n", rgain, bgain);
	DBGMSG("NRB %d %d\n", pBinGain->u16RGain, pBinGain->u16BGain);
	for (i = 0; i < AWB_BIN_ZONE_NUM; i++) {
		//printf("%d\t%d\t%d\n", i, i * AWB_BIN_STEP, pInfo->u16CenterBinCnt[i]);
	}
#endif

}

static void AWB_HistWeightInit(CVI_U8 sID)
{
	CVI_U16 i, curInx, err = 0;
	CVI_U16 preWt, curWt;
	ISP_AWB_LUM_HISTGRAM_ATTR_S stCurHist;

	if (!stAwbAttrInfo[sID]->stAuto.stLumaHist.bEnable)
		return;
	stCurHist = stAwbAttrInfo[sID]->stAuto.stLumaHist;

	if (stAwbAttrInfo[sID]->stAuto.stLumaHist.enOpType == OP_TYPE_MANUAL) {
		for (i = 1; i < AWB_LUM_HIST_NUM; i++) {
			if (stCurHist.au8HistThresh[i] <= stCurHist.au8HistThresh[i-1]) {
				err++;
			}
		}
		if (stCurHist.au8HistThresh[0] != 0)
			err++;
		if (stCurHist.au8HistThresh[AWB_LUM_HIST_NUM-1] != 0xFF)
			err++;
		if (err)
			stAwbAttrInfo[sID]->stAuto.stLumaHist.enOpType = OP_TYPE_AUTO;
	}
	if (stAwbAttrInfo[sID]->stAuto.stLumaHist.enOpType == OP_TYPE_AUTO) {
		stCurHist.au8HistThresh[0] = 0;
		stCurHist.au16HistWt[0] = 32;
		stCurHist.au8HistThresh[1] = 4;
		stCurHist.au16HistWt[1] = 128;
		stCurHist.au8HistThresh[2] = 16;
		stCurHist.au16HistWt[2] = 384;
		stCurHist.au8HistThresh[3] = 128;
		stCurHist.au16HistWt[3] = 512;
		stCurHist.au8HistThresh[4] = 235;
		stCurHist.au16HistWt[4] = 256;
		stCurHist.au8HistThresh[5] = 255;
		stCurHist.au16HistWt[5] = 32;
		if (WBInfo[sID]->stEnvInfo.u32ISONum < 6400)
			u16ReduceLowHist[sID] = 0;
		else if (WBInfo[sID]->stEnvInfo.u32ISONum > 9600)
			u16ReduceLowHist[sID] = 1;
		if (u16ReduceLowHist[sID]) {
			stCurHist.au8HistThresh[0] = 0;
			stCurHist.au16HistWt[0] = 16;
			stCurHist.au8HistThresh[1] = 32;
			stCurHist.au16HistWt[1] = 16;
			stCurHist.au8HistThresh[2] = 34;
			stCurHist.au16HistWt[2] = 400;
			u16ReduceLowHist[sID] = 1;
		}
	}

#ifdef WBIN_SIM
	PRINT_FUNC(" OP:%d", stAwbAttrInfo[sID]->stAuto.stLumaHist.enOpType);

	for (i = 0; i < AWB_LUM_HIST_NUM; i++) {
		#if USE_WT_HIST_SAME
		stCurHist.au16HistWt[i] = 128;
		#endif
		DBGMSG("%d\t%d\n", stCurHist.au8HistThresh[i], stCurHist.au16HistWt[i]);
	}
#endif
	curInx = 0;
	preWt = stCurHist.au16HistWt[0];
	for (i = 0; i < 256; i++) {
		if (i >= stCurHist.au8HistThresh[curInx]) {
			preWt = stCurHist.au16HistWt[curInx];
			curInx++;
		}
		curWt = stCurHist.au16HistWt[curInx];
		if ((stCurHist.au8HistThresh[curInx]-stCurHist.au8HistThresh[curInx-1])) {
			if (preWt < curWt) {
				u16HistWt[sID][i] = preWt+(i-stCurHist.au8HistThresh[curInx-1])*(curWt-preWt)/
					(stCurHist.au8HistThresh[curInx]-stCurHist.au8HistThresh[curInx-1]);
			} else {
				u16HistWt[sID][i] = preWt-(i-stCurHist.au8HistThresh[curInx-1])*(preWt-curWt)/
					(stCurHist.au8HistThresh[curInx]-stCurHist.au8HistThresh[curInx-1]);
			}
		}
		//DBGMSG("%d\t%d\n", i, u16HistWt[sID][i]);
	}
}

static void AWB_CalcWildAWB(CVI_U8 sID, sWBSampleInfo *pInfo, sWBGain *pGain)
{
	UNUSED(sID);
	if (pInfo->u32WildCnt) {
		pGain->u16RGain = pInfo->u32WildSum[AWB_R] / pInfo->u32WildCnt;
		pGain->u16GGain = pInfo->u32WildSum[AWB_G] / pInfo->u32WildCnt;
		pGain->u16BGain = pInfo->u32WildSum[AWB_B] / pInfo->u32WildCnt;
	} else {
		pGain->u16RGain = 0;
		pGain->u16GGain = 0;
		pGain->u16BGain = 0;
	}
}

static void AWB_CalcSE_AWB(CVI_U8 sID, sWBSampleInfo *pInfo, sWBGain *pGain)
{
	UNUSED(sID);
	if (pInfo->u32SE_Cnt) {
		pGain->u16RGain = pInfo->u32SE_Sum[AWB_R] / pInfo->u32SE_Cnt;
		pGain->u16GGain = pInfo->u32SE_Sum[AWB_G] / pInfo->u32SE_Cnt;
		pGain->u16BGain = pInfo->u32SE_Sum[AWB_B] / pInfo->u32SE_Cnt;
	} else {
		pGain->u16RGain = 0;
		pGain->u16GGain = 0;
		pGain->u16BGain = 0;
	}
	//pcprintf("SE %d %d %d\n", pGain->u16RGain, pGain->u16BGain, pInfo->u32SE_Cnt);
}

static CVI_S32 AWB_AdjSaturationByCA(CVI_U8 sID, sWBGain *pstFinalWB)
{
#define AVAILABLE_LUMA_MIN  32
#define AVAILABLE_LUMA_MAX  1000
#define AVAILABLE_GAIN_MIN	1350
#define GAIN_DIFF_THR	150
#define LUMA_DIFF_THR	120
#define SECONED_CT_AVAILABLE_CNT 25

	static CVI_U16 doubleCTThr;
	CVI_U8 adjRatio = stAwbAttrInfoEx[sID]->u16CAAdjustRatio;
	CVI_U16 CALumaDiff = stAwbAttrInfoEx[sID]->u16CALumaDiff;
	CVI_U16 x, y, num, idx, availableCnt = 0;
	CVI_U16	tmpRValue[WB_WDR_FRAME_MAX], tmpGValue[WB_WDR_FRAME_MAX],
			tmpBValue[WB_WDR_FRAME_MAX];
	CVI_U16	RValue, GValue, BValue, maxValue, secCTCnt = 0;
	CVI_U16 RGainVal = 0, BGainVal = 0;
	CVI_U64 RGainValSum = 0, BGainValSum = 0;
	CVI_U32 lumaSum = 0, averageLuma = 0;
	CVI_U16 secRgain, finalIdx, secIdx, diffIdx, diffRgain, diffBgain;
	CVI_FLOAT expRatio = 0;

	if (stAwbAttrInfoEx[sID]->bMultiLightSourceEn != CVI_TRUE ||
			stAwbAttrInfoEx[sID]->u16CAAdjustRatio == 0) {
		WBInfo[sID]->u8AdjCASatLuma = 0;
		WBInfo[sID]->u8AdjCASaturation = 0;
		return CVI_FAILURE;
	}

	if (WBInfo[sID]->u8AwbMaxFrameNum > 1) {
		for (idx = 0; idx < AWB_ZONE_NUM; idx++) {
			if (WBInfo[sID]->pu16AwbStatistics[0][AWB_R][idx] < AVAILABLE_LUMA_MAX &&
					WBInfo[sID]->pu16AwbStatistics[1][AWB_R][idx] > AVAILABLE_LUMA_MIN) {
				expRatio = (CVI_FLOAT)WBInfo[sID]->pu16AwbStatistics[0][AWB_R][idx] /
							WBInfo[sID]->pu16AwbStatistics[1][AWB_R][idx];
				break;
			}
		}
	}
	for (y = 0; y < WBInfo[sID]->u16WBRowSize; ++y) {
		for (x = 0; x < WBInfo[sID]->u16WBColumnSize; ++x) {
			idx = y * WBInfo[sID]->u16WBColumnSize + x;
			for (num = 0; num < WBInfo[sID]->u8AwbMaxFrameNum; num++) {
				tmpRValue[num] = WBInfo[sID]->pu16AwbStatistics[num][AWB_R][idx];
				tmpGValue[num] = WBInfo[sID]->pu16AwbStatistics[num][AWB_G][idx];
				tmpBValue[num] = WBInfo[sID]->pu16AwbStatistics[num][AWB_B][idx];
			}
			RValue = tmpRValue[WB_WDR_LE];
			GValue = tmpGValue[WB_WDR_LE];
			BValue = tmpBValue[WB_WDR_LE];
			if (WBInfo[sID]->u8AwbMaxFrameNum > 1) {
				maxValue = AWB_MAX(tmpRValue[num], tmpGValue[num]);
				maxValue = AWB_MAX(maxValue, tmpBValue[num]);
				if (maxValue > AVAILABLE_LUMA_MAX && expRatio != 0) {
					RValue = tmpRValue[WB_WDR_SE] * expRatio;
					GValue = tmpGValue[WB_WDR_SE] * expRatio;
					BValue = tmpBValue[WB_WDR_SE] * expRatio;
				}
			}

			maxValue = AWB_MAX(RValue, GValue);
			maxValue = AWB_MAX(maxValue, BValue);
			if (maxValue > AVAILABLE_LUMA_MIN && maxValue < AVAILABLE_LUMA_MAX) {
				lumaSum += maxValue;
				availableCnt++;
			}
		}
	}
	averageLuma = lumaSum / availableCnt;

	for (y = 0; y < WBInfo[sID]->u16WBRowSize; ++y) {
		for (x = 0; x < WBInfo[sID]->u16WBColumnSize; ++x) {
			idx = y * WBInfo[sID]->u16WBColumnSize + x;
			for (num = 0; num < WBInfo[sID]->u8AwbMaxFrameNum; num++) {
				tmpRValue[num] = WBInfo[sID]->pu16AwbStatistics[num][AWB_R][idx];
				tmpGValue[num] = WBInfo[sID]->pu16AwbStatistics[num][AWB_G][idx];
				tmpBValue[num] = WBInfo[sID]->pu16AwbStatistics[num][AWB_B][idx];
			}
			RValue = tmpRValue[WB_WDR_LE];
			GValue = tmpGValue[WB_WDR_LE];
			BValue = tmpBValue[WB_WDR_LE];
			if (WBInfo[sID]->u8AwbMaxFrameNum > 1) {
				maxValue = AWB_MAX(tmpRValue[num], tmpGValue[num]);
				maxValue = AWB_MAX(maxValue, tmpBValue[num]);
				if (maxValue > AVAILABLE_LUMA_MAX && expRatio != 0) {
					RValue = tmpRValue[WB_WDR_SE] * expRatio;
					GValue = tmpGValue[WB_WDR_SE] * expRatio;
					BValue = tmpBValue[WB_WDR_SE] * expRatio;
				}
			}
			maxValue = AWB_MAX(RValue, GValue);
			maxValue = AWB_MAX(maxValue, BValue);
			if (maxValue > AVAILABLE_LUMA_MIN && maxValue < AVAILABLE_LUMA_MAX) {
				RGainVal = GValue * AWB_GAIN_BASE / RValue;
				BGainVal = GValue * AWB_GAIN_BASE / BValue;
				diffRgain = AWB_ABS(pstFinalWB->u16RGain - RGainVal);
				diffBgain = AWB_ABS(pstFinalWB->u16BGain - BGainVal);
				if (maxValue > (averageLuma + LUMA_DIFF_THR) && maxValue < AVAILABLE_LUMA_MAX &&
					(diffRgain > GAIN_DIFF_THR && diffBgain > GAIN_DIFF_THR) &&
					(RGainVal > AVAILABLE_GAIN_MIN && BGainVal > AVAILABLE_GAIN_MIN)) {
					RGainValSum += RGainVal;
					BGainValSum += BGainVal;
					secCTCnt++;
				}
			}
		}
	}
	secRgain = RGainValSum / secCTCnt;
	finalIdx = pstFinalWB->u16RGain / AWB_BIN_STEP;
	secIdx = secRgain / AWB_BIN_STEP;
	diffIdx = AWB_ABS(secIdx - finalIdx);
	doubleCTThr = doubleCTThr == 0 ? SECONED_CT_AVAILABLE_CNT : doubleCTThr;
	DBGMSG("seconed color temperature cnt =%d rGain = %ld BGain =%ld  diffIdx = %d\n",
		secCTCnt, RGainValSum / secCTCnt, BGainValSum / secCTCnt, diffIdx);
	if (secCTCnt > doubleCTThr) {
		doubleCTThr = SECONED_CT_AVAILABLE_CNT - 5;
		WBInfo[sID]->u8AdjCASatLuma = AWB_MIN((averageLuma + LUMA_DIFF_THR) / 4 + CALumaDiff, 255);
		WBInfo[sID]->u8AdjCASaturation = AWB_MIN(AWB_MIN(diffIdx, 20) * adjRatio, 128);
		return CVI_SUCCESS;
	}
	doubleCTThr = SECONED_CT_AVAILABLE_CNT;
	WBInfo[sID]->u8AdjCASatLuma = 0;
	WBInfo[sID]->u8AdjCASaturation = 0;


	return CVI_FAILURE;
}

static CVI_U32 AWB_CalcDoubleCTDiff(CVI_U8 sID, sWBGain *pstFinalWB)
{
	#define CT_VAR_THRES	(20)
	CVI_U64 real_MaxZoneCnt = 0, real_SubZoneCnt = 0;
	CVI_U64 zoneRgain1 = 0, zoneBgain1 = 0;
	CVI_U64 zoneRgain2 = 0, zoneBgain2 = 0;
	stZone_Info stZoneInfo = {0};
	CVI_S32 finalCT, firstCT, seCT;

	AWB_GetGrayPointZoneInfo(6, pstTmpSampeInfo[sID], accSum, &stZoneInfo);

	zoneRgain1 = stZoneInfo.u64MaxZoneRgain;
	zoneBgain1 = stZoneInfo.u64MaxZoneBgain;
	zoneRgain2 = stZoneInfo.u64SubZoneRgain;
	zoneBgain2 = stZoneInfo.u64SubZoneBgain;
	real_MaxZoneCnt = stZoneInfo.u64MaxGrayTotalCnt;
	real_SubZoneCnt = stZoneInfo.u64SubGrayTotalCnt;

	finalCT = AWB_GetCurColorTemperatureRB(sID, pstFinalWB->u16RGain, pstFinalWB->u16BGain);
	firstCT = AWB_GetCurColorTemperatureRB(sID, zoneRgain1, zoneBgain1);
	seCT = AWB_GetCurColorTemperatureRB(sID, zoneRgain2, zoneBgain2);

	real_SubZoneCnt = AWB_DIV_0_TO_1(real_SubZoneCnt);
	CVI_U16 CTRatio =  AWB_MIN(real_SubZoneCnt, real_MaxZoneCnt) * 100 /
						AWB_MAX(real_SubZoneCnt, real_MaxZoneCnt);
	DBGMSG("real_SubZoneCnt =%ld real_MaxZoneCnt = %ld finalCT = %d firstCT =%d seCT =%d\n",
		real_SubZoneCnt, real_MaxZoneCnt, finalCT, firstCT, seCT);
	if (AWB_MIN(real_SubZoneCnt, real_MaxZoneCnt) > stAwbAttrInfoEx[sID]->u16MultiLSScaler ||
				CTRatio > CT_VAR_THRES) {
		return AWB_MAX(AWB_ABS(finalCT - firstCT), AWB_ABS(finalCT - seCT));
	}

	return 0;
}

static void AWB_AdjSaturationByCCM(CVI_U8 sID, sWBGain finalWB, sWBGain seWB)
{
	#define MAX_CT_DIFF	(2000)
	#define AWB_SATURATION_BASE (128)
	#define CT_VAR_THRES	(20)// 1:4
	#define CT_VAR_MAX		(50)// 1:1

	CVI_U8 curSat = AWB_SATURATION_BASE;
	CVI_U8 maxSubSat = (ATTR_SATURATION_ADJ_BASE - stAwbAttrInfoEx[sID]->u16MultiLSScaler)*
				AWB_SATURATION_BASE / ATTR_SATURATION_ADJ_BASE;
	CVI_U32 finalCT, seCT, diffCT;

	if (stAwbAttrInfoEx[sID]->bMultiLightSourceEn != CVI_TRUE ||
			stAwbAttrInfoEx[sID]->enMultiLSType != AWB_MULTI_LS_SAT) {
		WBInfo[sID]->u8Saturation[WB_WDR_SE] = AWB_SATURATION_BASE;
		WBInfo[sID]->u8Saturation[WB_WDR_LE] = AWB_SATURATION_BASE;
		return;
	}

	PRINT_FUNC();
	DBGMSG("maxSubSat %d\n", maxSubSat);
	if (WBInfo[sID]->u8AwbMaxFrameNum != 1) {//WDR mode .adj u8SaturationSE only.
		finalCT = AWB_GetCurColorTemperatureRB(sID, finalWB.u16RGain, finalWB.u16BGain);
		WBInfo[sID]->u16SEColorTemp = finalCT;
		if (seWB.u16RGain) {
			seCT = AWB_GetCurColorTemperatureRB(sID, seWB.u16RGain, seWB.u16BGain);
			WBInfo[sID]->u16SEColorTemp = seCT;
			if (seCT > finalCT) {
				diffCT = seCT - finalCT;
				AWB_LIMIT_UPPER(diffCT, MAX_CT_DIFF);
				curSat = AWB_SATURATION_BASE - diffCT * maxSubSat / MAX_CT_DIFF;
				DBGMSG("CT %d %d %d\n", finalCT, seCT, curSat);
			}
		}
		if (WBInfo[sID]->u8Saturation[WB_WDR_SE] > curSat)
			WBInfo[sID]->u8Saturation[WB_WDR_SE] = WBInfo[sID]->u8Saturation[WB_WDR_SE] - 1;
		else if (WBInfo[sID]->u8Saturation[WB_WDR_SE] < curSat)
			WBInfo[sID]->u8Saturation[WB_WDR_SE] = WBInfo[sID]->u8Saturation[WB_WDR_SE] + 1;
		#ifdef CHIP_ARCH_CV183X
			WBInfo[sID]->u8Saturation[WB_WDR_LE] = WBInfo[sID]->u8Saturation[WB_WDR_SE];
		#endif
	} else {//Linear mode .adj u8SaturationLE only.
		diffCT = AWB_CalcDoubleCTDiff(sID, &finalWB);
		if (diffCT > 0) {
			AWB_LIMIT_UPPER(diffCT, MAX_CT_DIFF);
			curSat = AWB_SATURATION_BASE - diffCT * maxSubSat / MAX_CT_DIFF;
		}

		if (WBInfo[sID]->u8Saturation[WB_WDR_LE] > curSat)
			WBInfo[sID]->u8Saturation[WB_WDR_LE] = WBInfo[sID]->u8Saturation[WB_WDR_LE] - 1;
		else if (WBInfo[sID]->u8Saturation[WB_WDR_LE] < curSat)
			WBInfo[sID]->u8Saturation[WB_WDR_LE] = WBInfo[sID]->u8Saturation[WB_WDR_LE] + 1;
		DBGMSG("diffCT = %u curSat %d\n", diffCT, curSat);
	}

}

static void AWB_SkyInit(CVI_U8 sID)
{
	CVI_S16 lvx100Thr;
	CVI_U16 u16Radius, diffLv;

	sSkyRB[sID].r_max = 0;
	sSkyRB[sID].b_max = 0;

	if (stAwbAttrInfoEx[sID]->stSky.u8Mode == AWB_SKY_OFF) {
		return;
	}
	lvx100Thr = stAwbAttrInfoEx[sID]->stSky.u8ThrLv * 100;
	if (WBInfo[sID]->stEnvInfo.s16LvX100 < lvx100Thr) {
		return;
	}
	diffLv = WBInfo[sID]->stEnvInfo.s16LvX100 - lvx100Thr;
	AWB_LIMIT_UPPER(diffLv, 100);
	u16Radius = stAwbAttrInfoEx[sID]->stSky.u8Radius << 4;
	u16Radius = u16Radius * diffLv / 100;
	sSkyRB[sID].r_max = stAwbAttrInfoEx[sID]->stSky.u16Rgain + u16Radius;
	sSkyRB[sID].r_min = stAwbAttrInfoEx[sID]->stSky.u16Rgain - u16Radius;
	sSkyRB[sID].b_max = stAwbAttrInfoEx[sID]->stSky.u16Bgain + u16Radius;
	sSkyRB[sID].b_min = stAwbAttrInfoEx[sID]->stSky.u16Bgain - u16Radius;
	PRINT_FUNC();
	DBGMSG("lvx100Thr :%d r:%d\n", lvx100Thr, u16Radius);
	DBGMSG("r:%d %d\n", sSkyRB[sID].r_max, sSkyRB[sID].r_min);
	DBGMSG("b:%d %d\n", sSkyRB[sID].b_max, sSkyRB[sID].b_min);
}

static void AWB_SkyProcess(CVI_U8 sID, CVI_BOOL *pixelInCurve, sPOINT_WB_INFO *psPtInfo)
{
#define SKY_MIN_G (512)
	CVI_U16 rMax, rMin, bMax, bMin;
	CVI_S16 lvx100Thr;

	if (stAwbAttrInfoEx[sID]->stSky.u8Mode == AWB_SKY_OFF) {
		return;
	}
	if (psPtInfo->isSE_OrSDR == 0) {//LE @ WDR mode
		if (psPtInfo->u16GValue < SKY_MIN_G) {
			return;
		}
	}
	lvx100Thr = stAwbAttrInfoEx[sID]->stSky.u8ThrLv * 100;
	if (WBInfo[sID]->stEnvInfo.s16LvX100 < lvx100Thr) {
		return;
	}

	rMax = sSkyRB[sID].r_max;
	rMin = sSkyRB[sID].r_min;
	bMax = sSkyRB[sID].b_max;
	bMin = sSkyRB[sID].b_min;

	if (psPtInfo->u16RGain < rMax && psPtInfo->u16RGain > rMin) {
		if (psPtInfo->u16BGain < bMax && psPtInfo->u16BGain > bMin) {
			if (stAwbAttrInfoEx[sID]->stSky.u8Mode == AWB_SKY_REMOVE) {
				*pixelInCurve = 0;
			} else if (stAwbAttrInfoEx[sID]->stSky.u8Mode == AWB_SKY_MAPPING) {
				*pixelInCurve = 1;
				psPtInfo->u16RGain = stAwbAttrInfoEx[sID]->stSky.u16MapRgain;
				psPtInfo->u16BGain = stAwbAttrInfoEx[sID]->stSky.u16MapBgain;
			}
		}
	}
}

static void AWB_CalculateBalanceWBGain(CVI_U8 sID)
{
#define ADJ_MIN_G_LV_THRES	(600)
#define STRICT_EXTRA_ENABLE_RATIO (0.4)
	CVI_U16 ii, x, y, idx, k;
	CVI_U16 pixelR, pixelG, pixelB, G_Min, G_Max;
	CVI_U16	tmpPixelR[WB_WDR_FRAME_MAX], tmpPixelG[WB_WDR_FRAME_MAX],
			tmpPixelB[WB_WDR_FRAME_MAX];
	CVI_U16 RGain = 0, BGain = 0;
	CVI_U16 wbCurveTopRange, wbCurveBotRange, wbCurveBGain;
	CVI_U16 wbWideTopRange, wbWideBotRange;
	CVI_U32 centerRawGSum = 0;
	CVI_U16 centerRawCnt = 0;
	CVI_U16 centerStartX, centerEndX, centerStartY, centerEndY;
	sWBCurveInfo stTmpCurveInfo;
	sWBBoundary stTmpBoundary;
	CVI_BOOL	pixelInCurve, pxlInWide;
	sWBGain		lowCostAwbGain,  finalGain;
	CVI_U8		useSE_LEdata;
	sWBGain		binCT_Gain, binGainOutDoor, wildGain, seGain;
	CVI_U16 posWgt;
	sPOINT_WB_INFO sPtInfo;
	CVI_U32 u32grayR100, u32wild_grayR100, mixWild;
	CVI_U16 min_sub;
	static CVI_BOOL bEnableStrictExtra[AWB_SENSOR_NUM];
	CVI_U32 u32StrictExtraPixelNum = 0;

	if (pstTmpSampeInfo[sID] == NULL)
		return;
	G_Min = 35;
	G_Max = 800;
	if (WBInfo[sID]->u8AwbMaxFrameNum == 1) {//Linear mode
		G_Min = 25;
	}
	if (WBInfo[sID]->stEnvInfo.s16LvX100 < ADJ_MIN_G_LV_THRES) {
		min_sub = (ADJ_MIN_G_LV_THRES - WBInfo[sID]->stEnvInfo.s16LvX100) / 10;

		if (min_sub > 15)
			min_sub = 15;
		G_Min -= min_sub;

		if (G_Min < 20)
			G_Min = 20;

		CVI_U16 pixelG = 0;
		CVI_U32 G5_1023 = 0, G10_1023 = 0, G15_1023 = 0, G20_1023 = 0;
		CVI_U32 G_THRESHOLD = WBInfo[sID]->u16WBRowSize * WBInfo[sID]->u16WBColumnSize / 5;

		for (y = 0; y < WBInfo[sID]->u16WBRowSize; ++y) {
			for (x = 0; x < WBInfo[sID]->u16WBColumnSize; ++x) {
				idx = y * WBInfo[sID]->u16WBColumnSize + x;
				pixelG = WBInfo[sID]->pu16AwbStatistics[0][AWB_G][idx];
				if (pixelG < G_Max) {
					if (pixelG > 5)
						G5_1023++;
					if (pixelG > 10)
						G10_1023++;
					if (pixelG > 15)
						G15_1023++;
					if (pixelG > 20)
						G20_1023++;
				}
			}
		}
		DBGMSG("G5_1023:%d G10_1023:%d G15_1023:%d G20_1023:%d\n",
			G5_1023, G10_1023, G15_1023, G20_1023);
		if (G20_1023 <= G_THRESHOLD) {
			if (G15_1023 <= G_THRESHOLD) {
				if (G10_1023 <= G_THRESHOLD) {
					G_Min = 5;
				} else {
					G_Min = 10;
				}
			} else {
				G_Min = 15;
			}
		}
		DBGMSG("__G_Min %d\n", G_Min);
	}


	if (stAwbAttrInfoEx[sID]->u16CurveLLimit && stAwbAttrInfoEx[sID]->u16CurveRLimit) {
		u16CurLLimRGain[sID] = AWB_GAIN_BASE*AWB_GAIN_BASE/stAwbAttrInfoEx[sID]->u16CurveLLimit;
		u16CurLLimBGain[sID] = AWB_GAIN_BASE*AWB_GAIN_BASE/stAwbAttrInfoEx[sID]->u16CurveLLimit;
		u16CurRLimRGain[sID] = AWB_GAIN_BASE*AWB_GAIN_BASE/stAwbAttrInfoEx[sID]->u16CurveRLimit;
		u16CurRLimBGain[sID] = AWB_GAIN_BASE*AWB_GAIN_BASE/stAwbAttrInfoEx[sID]->u16CurveRLimit;
	} else {
		u16CurLLimRGain[sID] = 0xFFFF;
		u16CurLLimBGain[sID] = 0xFFFF;
		u16CurRLimRGain[sID] = 0;
		u16CurRLimBGain[sID] = 0;
	}

	sID = AWB_CheckSensorID(sID);
	AWB_ExtraLightWBInit(sID);
	AWB_SampleInfoInit(pstTmpSampeInfo[sID]);
	AWB_BoundaryInit(sID, &stTmpCurveInfo, &stTmpBoundary);

	AWB_AdjCurveRangeByLV(sID, &stTmpCurveInfo);

	AWB_AdjCurveBoundaryByLv(sID, &stTmpBoundary);
	AWB_AdjCurveBoundaryByColorTemperature(sID, &stTmpBoundary,
		stAwbAttrInfo[sID]->stAuto.u16LowColorTemp, stAwbAttrInfo[sID]->stAuto.u16HighColorTemp);
	AWB_AdjCurveBoundaryByISO(sID, &stTmpBoundary);

	AWB_CT_BinWeightInit(sID);
	AWB_BinOutdoorWeightInit(sID);
	AWB_HistWeightInit(sID);
	AWB_SkyInit(sID);

	stCurveInfo[sID] = stTmpCurveInfo;
	stBoundaryInfo[sID] = stTmpBoundary;

	centerStartX = WBInfo[sID]->u16WBColumnSize / 2 - WBInfo[sID]->u16WBColumnSize / 4;
	centerEndX = centerStartX + WBInfo[sID]->u16WBColumnSize / 2;
	centerStartY = WBInfo[sID]->u16WBRowSize / 2 - WBInfo[sID]->u16WBRowSize / 4;
	centerEndY = centerStartY + WBInfo[sID]->u16WBRowSize / 2;
	sPtInfo.fStrictExtraRatio = AWB_CalStrictExtraPixel_Ratio(sID, bEnableStrictExtra[sID]);
	sPtInfo.bEnableStrictExtra = bEnableStrictExtra[sID];
	memset(pstPointInfo[sID], 0, WBInfo[sID]->u16WBRowSize * WBInfo[sID]->u16WBColumnSize * sizeof(sPOINT_WB_INFO));

	if (u8AwbDbgBinFlag[sID] == AWB_DBG_BIN_FLOW_UPDATE) {
		if (pAwbDbg[sID]) {
			for (k = 0; k < WBInfo[sID]->u8AwbMaxFrameNum; ++k) {
				memcpy(&pAwbDbg[sID]->u16P_R[k][0], WBInfo[sID]->pu16AwbStatistics[k][AWB_R],
					sizeof(CVI_U16) * WBInfo[sID]->u16WBRowSize * WBInfo[sID]->u16WBColumnSize);
				memcpy(&pAwbDbg[sID]->u16P_G[k][0], WBInfo[sID]->pu16AwbStatistics[k][AWB_G],
					sizeof(CVI_U16) * WBInfo[sID]->u16WBRowSize * WBInfo[sID]->u16WBColumnSize);
				memcpy(&pAwbDbg[sID]->u16P_B[k][0], WBInfo[sID]->pu16AwbStatistics[k][AWB_B],
					sizeof(CVI_U16) * WBInfo[sID]->u16WBRowSize * WBInfo[sID]->u16WBColumnSize);
			}
			ii = 0;
			for (RGain = stTmpBoundary.u16RLowBound; RGain <= stTmpBoundary.u16RHighBound; RGain += 16) {
				AWB_GetWhiteBGainRange(sID, RGain, &wbCurveBGain, &wbCurveTopRange,
					&wbCurveBotRange, &wbWideTopRange, &wbWideBotRange, &stTmpCurveInfo);
				pAwbDbg[sID]->u16CurveR[ii] = RGain;
				pAwbDbg[sID]->u16CurveB[ii] = wbCurveBGain;
				#if 1	//Show normal
				pAwbDbg[sID]->u16CurveB_Top[ii] = wbCurveTopRange;
				pAwbDbg[sID]->u16CurveB_Bot[ii] = wbCurveBotRange;
				#else	//Show Wide
				pAwbDbg[sID]->u16CurveB_Top[ii] = wbWideTopRange;
				pAwbDbg[sID]->u16CurveB_Bot[ii] = wbWideBotRange;
				#endif
				//printf("%d\t%d\t%d\t%d\t%d\t%d\n", RGain, wbCurveBGain,
				//wbCurveTopRange, wbCurveBotRange, wbWideTopRange, wbWideBotRange);
				ii++;
			}
		}
	}

	for (y = 0; y < WBInfo[sID]->u16WBRowSize; ++y) {
		for (x = 0; x < WBInfo[sID]->u16WBColumnSize; ++x) {
			pixelInCurve = 0;
			pxlInWide = 0;
			useSE_LEdata = WB_WDR_LE;
			sPtInfo.isSE_OrSDR = 1;
			idx = y * WBInfo[sID]->u16WBColumnSize + x;
			for (k = 0; k < WBInfo[sID]->u8AwbMaxFrameNum; ++k) {
				tmpPixelR[k] = WBInfo[sID]->pu16AwbStatistics[k][AWB_R][idx];
				tmpPixelG[k] = WBInfo[sID]->pu16AwbStatistics[k][AWB_G][idx];
				tmpPixelB[k] = WBInfo[sID]->pu16AwbStatistics[k][AWB_B][idx];
			}

			if (WBInfo[sID]->u8AwbMaxFrameNum > 1) {//WDR mode
				if (tmpPixelG[WB_WDR_LE] < G_Max) {
					useSE_LEdata = WB_WDR_LE;
					sPtInfo.isSE_OrSDR = 0;
				} else {
					useSE_LEdata = WB_WDR_SE;
					pstTmpSampeInfo[sID]->u16WDRSECnt++;
				}
			}
			pixelR = tmpPixelR[useSE_LEdata];
			pixelG = tmpPixelG[useSE_LEdata];
			pixelB = tmpPixelB[useSE_LEdata];

			posWgt = AWB_GetPosWgt(sID, x, y);
			if (posWgt) {
				pstTmpSampeInfo[sID]->u16TotalSampleCnt++;
			}

			if (x >= centerStartX && x <= centerEndX && y >= centerStartY && y <= centerEndY) {
				centerRawGSum += pixelG;
				centerRawCnt++;
			}
			sPtInfo.enPointType = AWB_POINT_EXTRA_NONE;
			sPtInfo.u16CurveBGain = 0;
			sPtInfo.u16RGain = 0;
			sPtInfo.u16BGain = 0;
			sPtInfo.u16GValue = pixelG;
			sPtInfo.u8X = x;
			sPtInfo.u8Y = y;

			if (pixelR > 0 && pixelB > 0 && (pixelG > G_Min && pixelG < G_Max)) {
				RGain = pixelG * AWB_GAIN_BASE / pixelR;
				BGain = pixelG * AWB_GAIN_BASE / pixelB;
				pstTmpSampeInfo[sID]->u64TotalPixelSum[0][AWB_R] += RGain;
				pstTmpSampeInfo[sID]->u64TotalPixelSum[0][AWB_G] += AWB_GAIN_BASE;
				pstTmpSampeInfo[sID]->u64TotalPixelSum[0][AWB_B] += BGain;
				pstTmpSampeInfo[sID]->u16EffectiveSampleCnt++;
				sPtInfo.u16RGain = RGain;
				sPtInfo.u16BGain = BGain;

				if (RGain >= stTmpBoundary.u16RLowBound && RGain <= stTmpBoundary.u16RHighBound) {
					AWB_GetWhiteBGainRange(sID, RGain, &wbCurveBGain,
						&wbCurveTopRange, &wbCurveBotRange,
						&wbWideTopRange, &wbWideBotRange, &stTmpCurveInfo);
					pixelInCurve = (BGain < wbCurveTopRange && BGain > wbCurveBotRange) ? 1 : 0;
					sPtInfo.enPointType = AWB_ExtraLightWB(sID, RGain, BGain, &pixelInCurve);
					sPtInfo.u16CurveBGain = wbCurveBGain;

					if (sPtInfo.enPointType == AWB_POINT_EXTRA_STRICT) {
						u32StrictExtraPixelNum++;
					}
					if ((sPtInfo.enPointType == AWB_POINT_EXTRA_STRICT) &&
						(bEnableStrictExtra[sID] == 0) &&
						(sPtInfo.fStrictExtraRatio == 0.0)) {
						pixelInCurve = 0;
					}

					if (wbCurveTopRange != 0) {//in Rgain boudary
						pxlInWide = (BGain < wbWideTopRange && BGain > wbWideBotRange) ? 1 : 0;
					}
				}
				AWB_SkyProcess(sID, &pixelInCurve, &sPtInfo);
			}
			if (AwbFaceArea[sID].u8FDNum) {
				if (x > AwbFaceArea[sID].u16StX && x < AwbFaceArea[sID].u16EndX &&
					y > AwbFaceArea[sID].u16StY && y < AwbFaceArea[sID].u16EndY) {
					pixelInCurve = 0;
				}
			}
			if (pixelInCurve) {
				if (RGain > u16CurLLimRGain[sID] && BGain > u16CurLLimBGain[sID]) {
					pixelInCurve = 0;
				}
				if (RGain < u16CurRLimRGain[sID] && BGain < u16CurRLimBGain[sID]) {
					pixelInCurve = 0;
				}
				if (pixelInCurve) {
					pstPointInfo[sID][idx] = sPtInfo;
					AWB_AccWB_Data(sID, pstTmpSampeInfo[sID], &sPtInfo, posWgt);
					if (useSE_LEdata == WB_WDR_SE)
						AWB_AccWB_DataSE(sID, pstTmpSampeInfo[sID], &sPtInfo);
				}
			}
			if (pxlInWide) {
				AWB_AccWB_DataWild(sID, pstTmpSampeInfo[sID], &sPtInfo);
			}
			if ((u8AwbDbgBinFlag[sID] == AWB_DBG_BIN_FLOW_UPDATE) && pAwbDbg[sID]) {
				if (pixelInCurve == 0 || posWgt == 0) {
					pAwbDbg[sID]->u16P_type[useSE_LEdata][idx] = AWB_DBG_PT_OUT_CURVE;
				} else {
					pAwbDbg[sID]->u16P_type[useSE_LEdata][idx] = AWB_DBG_PT_IN_CURVE;
				}
			}
		}
	}

	if (u32StrictExtraPixelNum >=
		(STRICT_EXTRA_ENABLE_RATIO * WBInfo[sID]->u16WBRowSize * WBInfo[sID]->u16WBColumnSize)) {
		bEnableStrictExtra[sID] = true;
	} else {
		bEnableStrictExtra[sID] = false;
	}

	pstTmpSampeInfo[sID]->u16CentRawG = (centerRawCnt) ? centerRawGSum / centerRawCnt : 0;
	*stSampleInfo[sID] = *pstTmpSampeInfo[sID];

	if (stSampleInfo[sID]->u16TotalSampleCnt) {
		u32grayR100 = stSampleInfo[sID]->u16GrayCnt
			* 100 * 100 / stSampleInfo[sID]->u16TotalSampleCnt;
		u32wild_grayR100 = stSampleInfo[sID]->u32WildCnt
			* 100 * 100 / stSampleInfo[sID]->u16TotalSampleCnt;
	} else {
		u32grayR100 = 0;
		u32wild_grayR100 = 0;
	}

	stSampleInfo[sID]->u8GrayRatio = u32grayR100 / 100;

	if (stSampleInfo[sID]->u32GrayWeightCnt) {//Low Cost AWB
		lowCostAwbGain.u16RGain = (stSampleInfo[sID]->u64TotalPixelSum[1][AWB_R] /
			stSampleInfo[sID]->u32GrayWeightCnt);
		lowCostAwbGain.u16GGain = (stSampleInfo[sID]->u64TotalPixelSum[1][AWB_G] /
			stSampleInfo[sID]->u32GrayWeightCnt);
		lowCostAwbGain.u16BGain = (stSampleInfo[sID]->u64TotalPixelSum[1][AWB_B] /
			stSampleInfo[sID]->u32GrayWeightCnt);
	} else {
		lowCostAwbGain.u16RGain = 0;
		lowCostAwbGain.u16GGain = 0;
		lowCostAwbGain.u16BGain = 0;
	}
	if (lowCostAwbGain.u16GGain == AWB_GAIN_SMALL_BASE) {
		lowCostAwbGain.u16RGain = lowCostAwbGain.u16RGain*AWB_GAIN_BASE/AWB_GAIN_SMALL_BASE;
		lowCostAwbGain.u16GGain = AWB_GAIN_BASE;
		lowCostAwbGain.u16BGain = lowCostAwbGain.u16BGain*AWB_GAIN_BASE/AWB_GAIN_SMALL_BASE;
	}

	AWB_CalcCT_BinAWB(sID, pstTmpSampeInfo[sID], &binCT_Gain);
	AWB_CalcOutdoorBinAWB(sID, pstTmpSampeInfo[sID], &binGainOutDoor);
	AWB_MixOutdoorAWB(sID, &binCT_Gain, &binGainOutDoor);
	AWB_CalcMixColorTemp(sID, &binCT_Gain, &stTmpCurveInfo, pstTmpSampeInfo[sID]);

	AWB_CalcWildAWB(sID, pstTmpSampeInfo[sID], &wildGain);
	AWB_CalcSE_AWB(sID, pstTmpSampeInfo[sID], &seGain);
	if (u32wild_grayR100 < AWB_GRAY_THRES) {
		wildGain.u16GGain = 0;
	}
	stSampleInfo[sID]->bUsePreBalanceGain =
		((stSampleInfo[sID]->u64TotalPixelSum[1][AWB_R] > 0
		  && stSampleInfo[sID]->u64TotalPixelSum[1][AWB_B] > 0)
			&& stSampleInfo[sID]->u16GrayCnt > 0 && u32grayR100 >= AWB_GRAY_THRES) ?
			0 :
			1;
	if (!lowCostAwbGain.u16RGain) {
		stSampleInfo[sID]->bUsePreBalanceGain = 1;
	}

	finalGain = lowCostAwbGain;
	if (binCT_Gain.u16RGain) {
		finalGain = binCT_Gain;
	}

	if (!stSampleInfo[sID]->bUsePreBalanceGain) { // 3%
		WBInfo[sID]->stBalanceWB = finalGain;
		WBInfo[sID]->bStable = CVI_TRUE;
	} else if (wildGain.u16GGain) {
		if (finalGain.u16GGain) {//mix Wild WB
			if	(u32grayR100 < AWB_GRAY_THRES) {
				mixWild = (AWB_GRAY_THRES - u32grayR100);
			} else {
				mixWild = 0;
			}
			if (mixWild > 100) {
				mixWild = 100;
			}
			DBGMSG("==Wild== %d %d %d\n", u32wild_grayR100, u32grayR100, mixWild);
			DBGMSG("f:%d %d\n", finalGain.u16RGain, finalGain.u16BGain);
			DBGMSG("w:%d %d\n", wildGain.u16RGain, wildGain.u16BGain);
			finalGain.u16RGain = ((100 - mixWild) * finalGain.u16RGain + mixWild * wildGain.u16RGain)/
				100;
			finalGain.u16BGain = ((100 - mixWild) * finalGain.u16BGain + mixWild * wildGain.u16BGain)/
				100;
			DBGMSG("%d %d\n", finalGain.u16RGain, finalGain.u16BGain);
			WBInfo[sID]->stBalanceWB = finalGain;
			WBInfo[sID]->bStable = CVI_TRUE;
		} else {//use wild
			WBInfo[sID]->stBalanceWB = wildGain;
			WBInfo[sID]->bStable = CVI_TRUE;
		}
	} else {
		WBInfo[sID]->bStable = CVI_FALSE;
	}

	if (stSampleInfo[sID]->u16EffectiveSampleCnt) {
		WBInfo[sID]->stGrayWorldWB.u16RGain = stSampleInfo[sID]->u64TotalPixelSum[0][AWB_R]/
			stSampleInfo[sID]->u16EffectiveSampleCnt;
		WBInfo[sID]->stGrayWorldWB.u16GGain = AWB_GAIN_BASE;
		WBInfo[sID]->stGrayWorldWB.u16BGain = stSampleInfo[sID]->u64TotalPixelSum[0][AWB_B]/
			stSampleInfo[sID]->u16EffectiveSampleCnt;

	}

	WBInfo[sID]->stFinalWB = (WBInfo[sID]->bUseGrayWorld) ? WBInfo[sID]->stGrayWorldWB : WBInfo[sID]->stBalanceWB;
	if (AWB_AdjSaturationByCA(sID, &WBInfo[sID]->stFinalWB) != CVI_SUCCESS) {
		AWB_AdjSaturationByCCM(sID, WBInfo[sID]->stFinalWB, seGain);
	}

	if (stAwbAttrInfo[sID]->stAuto.u16ZoneSel == 0 || stAwbAttrInfo[sID]->stAuto.u16ZoneSel == 255) {
		WBInfo[sID]->stFinalWB = WBInfo[sID]->stGrayWorldWB;
	}

	if (u8AwbDbgBinFlag[sID] == AWB_DBG_BIN_FLOW_UPDATE) {
		if (pAwbDbg[sID]) {
			pAwbDbg[sID]->u16BalanceR = WBInfo[sID]->stBalanceWB.u16RGain;
			pAwbDbg[sID]->u16BalanceB = WBInfo[sID]->stBalanceWB.u16BGain;
			pAwbDbg[sID]->u16FinalR = WBInfo[sID]->stFinalWB.u16RGain;
			pAwbDbg[sID]->u16FinalB = WBInfo[sID]->stFinalWB.u16BGain;
			pAwbDbg[sID]->u16CurrentR = WBInfo[sID]->stCurrentWB.u16RGain;
			pAwbDbg[sID]->u16CurrentB = WBInfo[sID]->stCurrentWB.u16BGain;
			pAwbDbg[sID]->u16GrayCnt = stSampleInfo[sID]->u16GrayCnt;
			pAwbDbg[sID]->s16LvX100 = WBInfo[sID]->stEnvInfo.s16LvX100;
			pAwbDbg[sID]->u32ISONum = WBInfo[sID]->stEnvInfo.u32ISONum;
			pAwbDbg[sID]->u16Region_R[0] = stCurveInfo[sID].u16Region1_R;
			pAwbDbg[sID]->u16Region_R[1] = stCurveInfo[sID].u16Region2_R;
			pAwbDbg[sID]->u16Region_R[2] = stCurveInfo[sID].u16Region3_R;
		}
	}
	if (u8AwbDbgBinFlag[sID]) {
		u8AwbDbgBinFlag[sID]++;
	}
}



static void AWB_PreviewConverge(CVI_U8 sID, sWBGain *pstCurWBGain)
{
#define AWB_LV_STABLE_THR (50)//0.5EV
#define AWB_MIX_DIFF (128)
	sWBGain stWbGain;
	CVI_U8 ratio = stAwbAttrInfo[sID]->stAuto.u16Speed * 100 / AWB_MAX_SPEED;
	CVI_U16 i, u16diffR, u16diffB, tolerance;
	CVI_S16 s16DiffR, s16DiffB, s16StepR, s16StepB;
	CVI_S16 minLv = 10000, maxLv = -10000, diffLv;

	sID = AWB_CheckSensorID(sID);

	if (WBInfo[sID]->u32FrameID % 4 == 0) {//up lv per 4 frames ,4*30 frame buff
		s16LastLvX100[sID][inxLastLv[sID]] = WBInfo[sID]->stEnvInfo.s16LvX100;
		inxLastLv[sID]++;
		inxLastLv[sID] = inxLastLv[sID] % AWB_LAST_LV_SIZE;
	}
	for (i = 0; i < AWB_LAST_LV_SIZE; i++) {
		if (minLv > s16LastLvX100[sID][i]) {
			minLv = s16LastLvX100[sID][i];
		}
		if (maxLv < s16LastLvX100[sID][i]) {
			maxLv = s16LastLvX100[sID][i];
		}
	}
	diffLv = maxLv - minLv;
	if (diffLv < AWB_LV_STABLE_THR) {//stable scence
		if (stAwbAttrInfo[sID]->stAuto.u16Speed > 256) {
			ratio = 256 * 100 / AWB_MAX_SPEED;
		}
	}
	AWB_LIMIT(ratio, 1, 100);
	if (AwbSimMode) {
		ratio = 100;
	}

	if (WBInfo[sID]->stAssignedWB.u16RGain != AWB_GAIN_BASE ||
		WBInfo[sID]->stAssignedWB.u16BGain != AWB_GAIN_BASE) {
		*pstCurWBGain = WBInfo[sID]->stAssignedWB;
		return;
	}
	s16DiffR = WBInfo[sID]->stFinalWB.u16RGain - WBInfo[sID]->stCurrentWB.u16RGain;
	s16DiffB = WBInfo[sID]->stFinalWB.u16BGain - WBInfo[sID]->stCurrentWB.u16BGain;
	u16diffR = AWB_ABS(s16DiffR);
	u16diffB = AWB_ABS(s16DiffB);

	if (Is_Awb_AdvMode) {
		tolerance = stAwbAttrInfoEx[sID]->u8Tolerance;
		tolerance = tolerance * (AWB_GAIN_BASE/AWB_GAIN_SMALL_BASE);
		if (u16diffR < tolerance) {
			if (u16diffB < tolerance) {
				return;
			}
		}
	}

	if (WBInfo[sID]->u8WBConvergeMode == WB_CONVERGED_FAST) { /* AWB Fast */
		stWbGain = WBInfo[sID]->stFinalWB;
		if (AWB_ABS(WBInfo[sID]->stEnvInfo.fBVstep) < AWB_FAST_CONV_BV_STEP) {
			if (!stSampleInfo[sID]->bUsePreBalanceGain) {//has good result,switch to normal mode
				WBInfo[sID]->u8WBConvergeMode = WB_CONVERGED_NORMAL;
			}
		}
		if (WBInfo[sID]->u32FrameID > 30*5) {//after 5 sec ,worse case.
			WBInfo[sID]->u8WBConvergeMode = WB_CONVERGED_NORMAL;
		}
	} else {
		s16StepR = (s16DiffR * ratio) / 100;
		s16StepB = (s16DiffB * ratio) / 100;
		AWB_LIMIT(s16StepR, -AWB_MIX_DIFF, AWB_MIX_DIFF);
		AWB_LIMIT(s16StepB, -AWB_MIX_DIFF, AWB_MIX_DIFF);
		if (WBInfo[sID]->bEnableSmoothAWB && stAwbAttrInfo[sID]->u8AWBRunInterval) {
			s16StepR /= stAwbAttrInfo[sID]->u8AWBRunInterval;
			s16StepB /= stAwbAttrInfo[sID]->u8AWBRunInterval;
		}

		if (u16diffR > (AWB_GAIN_BASE>>8) && s16StepR == 0) {
			if (s16DiffR > 0)
				s16StepR = 1;
			else
				s16StepR = -1;
		}
		if (u16diffB > (AWB_GAIN_BASE>>8) && s16StepB == 0) {
			if (s16DiffB > 0)
				s16StepB = 1;
			else
				s16StepB = -1;
		}
		stWbGain.u16RGain = WBInfo[sID]->stCurrentWB.u16RGain + s16StepR;
		stWbGain.u16BGain = WBInfo[sID]->stCurrentWB.u16BGain + s16StepB;
		stWbGain.u16GGain = AWB_GAIN_BASE;
	}

	if (stWbGain.u16RGain > AWB_MAX_GAIN)
		stWbGain.u16RGain = AWB_MAX_GAIN;

	if (stWbGain.u16BGain > AWB_MAX_GAIN)
		stWbGain.u16BGain = AWB_MAX_GAIN;

	*pstCurWBGain = stWbGain;
}


static void AWB_LowColorTmMixDaylight(CVI_U8 sID, sWBGain *wbGain)
{
	sWBGain tmpWBGain = *wbGain;
	CVI_U8 RRatio, BRatio;
	CVI_U16 curDiff, u16RgainMixStart;

	u16RgainMixStart = AWB_GetGainByColorTemperature(sID, MIX_NATURAL_LOW_TEMP);
	if (u16RgainMixStart > wbGain->u16RGain)
		curDiff = u16RgainMixStart - wbGain->u16RGain;
	else
		curDiff = 0;

	RRatio = curDiff / 16;
	if (RRatio > 10)
		RRatio = 10;
	RRatio = 100 - RRatio;
	BRatio = RRatio;

	sID = AWB_CheckSensorID(sID);
	if (stAwbAttrInfo[sID]->stAuto.bNaturalCastEn && curDiff) {
		tmpWBGain.u16RGain = (tmpWBGain.u16RGain * RRatio + WBInfo[sID]->stDefaultWB.u16RGain *
			(100 - RRatio)) / 100;
		tmpWBGain.u16BGain = (tmpWBGain.u16BGain * BRatio + WBInfo[sID]->stDefaultWB.u16BGain *
			(100 - BRatio)) / 100;
		*wbGain = tmpWBGain;
	}
}

CVI_U8 AWB_CheckSensorID(CVI_U8 sID)
{
	return (sID < AWB_SENSOR_NUM) ? sID : AWB_SENSOR_NUM - 1;
}

void AWB_GetCurrentInfo(CVI_U8 sID, sWBInfo *pstAwbInfo)
{
	sID = AWB_CheckSensorID(sID);
	*pstAwbInfo = *WBInfo[sID];
}

void AWB_GetCurrentSampleInfo(CVI_U8 sID, sWBSampleInfo *pstSampeInfo)
{
	sID = AWB_CheckSensorID(sID);
	*pstSampeInfo = *stSampleInfo[sID];
}

static void AWB_RBStrength(CVI_U8 sID, sWBGain *pstCurWBGain)
{
#define  R_RATIO_RANGE (0.5)
#define  B_RATIO_RANGE (0.2)
	sWBGain curGain = *pstCurWBGain;
	CVI_U8 rRatio = 100, bRatio = 100;
	CVI_U8 rgStrength, bgStrength;

	sID = AWB_CheckSensorID(sID);
	rgStrength = stAwbAttrInfo[sID]->stAuto.u8RGStrength;
	bgStrength = stAwbAttrInfo[sID]->stAuto.u8BGStrength;

	if (bgStrength == 0) {
		//new adjust
		//2800K  (255,178, 91)(R/G:1.432 B/G:0.511) 6500K  (255,249,251)(R/G:1.024 B/G:1.008)
		//12000K (195,209,255)(R/G:0.933 B/G:1.220) 22000K (171,191,255)(R/G:0.895 B/G:1.335)
		//B/G = R/G * k + b
		//please refer https://wenku.baidu.com/view/273ab8c1866a561252d380eb6294dd88d0d23d14.html
		float warnk = (1.008 - 0.511) / (1.024 - 1.432);
		float warnb = 0.511 - (warnk * 1.432);
		float coldk = (1.335 - 1.220) / (0.895 - 0.993);
		float coldb = 1.220 - (coldk * 0.993);
		float Rratio = 1, Bratio = 1;

		if (rgStrength < 128) {
			//mw to warn colorTemp
			//input R/G(1.0 - 1.5)
			Rratio = 1 + (float)(128 - rgStrength)/128 * R_RATIO_RANGE;
			Bratio = warnk * Rratio + warnb;
		} else if (rgStrength > 128) {
			//mv to cold colorTemp
			//input R/G(0.8 - 1.0)
			Rratio = 1 - (float)(rgStrength - 128)/127 * B_RATIO_RANGE;
			Bratio = coldk * Rratio + coldb;
		}

		curGain.u16RGain = curGain.u16RGain * Rratio;
		curGain.u16BGain = curGain.u16BGain * Bratio;
	} else {
		////					< 0x 80						> 0x80
		////		HC		less R/more B				more R/less B
		////		LC		more R/less B			    less R/more B
		//old adjust
		if (rgStrength != 128) {
			if (rgStrength < 128) {
				if (curGain.u16RGain < curGain.u16BGain)
					rRatio += (128 - rgStrength) * 100 / 128;//more red
				else
					rRatio += (rgStrength - 128) * 100 / 256;//less red
			} else {
				if (curGain.u16RGain < curGain.u16BGain)
					rRatio -= (rgStrength - 128) * 100 / 256;//less red
				else
					rRatio -= (128 - rgStrength) * 100 / 128;//more red
			}
		}

		if (bgStrength != 128) {
			if (bgStrength < 128) {
				if (curGain.u16RGain < curGain.u16BGain)
					bRatio += (bgStrength - 128) * 100 / 256;//less blue
				else
					bRatio += (128 - bgStrength) * 100 / 128;//more blue
			} else {
				if (curGain.u16RGain < curGain.u16BGain)
					bRatio -= (128 - bgStrength) * 100 / 128;//more blue
				else
					bRatio -= (bgStrength - 128) * 100 / 256;//less blue
			}
		}

		if (rRatio != 100 || bRatio != 100) {
			curGain.u16RGain = curGain.u16RGain * rRatio / 100;
			curGain.u16BGain = curGain.u16BGain * bRatio / 100;
		}
	}

	*pstCurWBGain = curGain;
}


static void AWB_AdjustWB(CVI_U8 sID, sWBGain *pstCurWBGain)
{
	sID = AWB_CheckSensorID(sID);
	CVI_U16 u16HighRgLimit, u16HighBgLimit, u16LowRgLimit, u16LowBgLimit;

#if 0
	AWB_MixWB(sID, pstCurWBGain);
	AWB_ManualWB(sID, pstCurWBGain);
#endif
	AWB_LowColorTmMixDaylight(sID, pstCurWBGain);
	AWB_RBStrength(sID, pstCurWBGain);
	if (stAwbAttrInfo[sID]->stAuto.bGainNormEn) {
		AWB_LIMIT_UPPER(pstCurWBGain->u16RGain, AWB_GAIN_NORM_EN_MAX);
		AWB_LIMIT_UPPER(pstCurWBGain->u16GGain, AWB_GAIN_NORM_EN_MAX);
		AWB_LIMIT_UPPER(pstCurWBGain->u16BGain, AWB_GAIN_NORM_EN_MAX);
	}

	if (stAwbAttrInfo[sID]->stAuto.stCTLimit.bEnable &&
		stAwbAttrInfo[sID]->stAuto.stCTLimit.enOpType == OP_TYPE_MANUAL) {
		u16HighRgLimit = stAwbAttrInfo[sID]->stAuto.stCTLimit.u16HighRgLimit;
		u16HighBgLimit = stAwbAttrInfo[sID]->stAuto.stCTLimit.u16HighBgLimit;
		u16LowRgLimit = stAwbAttrInfo[sID]->stAuto.stCTLimit.u16LowRgLimit;
		u16LowBgLimit = stAwbAttrInfo[sID]->stAuto.stCTLimit.u16LowBgLimit;
		//DBGMSG("CTlimit %d %d %d %d\n",u16LowRgLimit,u16HighRgLimit,u16HighBgLimit,u16LowBgLimit);
		AWB_LIMIT(pstCurWBGain->u16RGain, u16LowRgLimit, u16HighRgLimit);
		AWB_LIMIT(pstCurWBGain->u16BGain, u16HighBgLimit, u16LowBgLimit);
	}
}

static void AWB_RunAlgo(CVI_U8 sID)
{
	sID = AWB_CheckSensorID(sID);

	if (WBInfo[sID]->stStitchAttr.enable && !WBInfo[sID]->stStitchAttr.bMainPipe &&
			stAwbAttrInfo[sID]->enOpType != OP_TYPE_MANUAL) {
		CVI_U8 mainID = AWB_SENSOR_NUM, i;

		for (i = 0; i < AWB_SENSOR_NUM; i++) {
			if (WBInfo[i]) {
				if (WBInfo[i]->stStitchAttr.bMainPipe &&
					WBInfo[i]->stStitchAttr.u8Group == WBInfo[sID]->stStitchAttr.u8Group) {
					mainID = i;
					break;
				}
			}
		}
		if (mainID != AWB_SENSOR_NUM) {
			WBInfo[sID]->stCurrentWB.u16BGain = WBInfo[mainID]->stCurrentWB.u16BGain;
			WBInfo[sID]->stCurrentWB.u16GGain = WBInfo[mainID]->stCurrentWB.u16GGain;
			WBInfo[sID]->stCurrentWB.u16RGain = WBInfo[mainID]->stCurrentWB.u16RGain;
			WBInfo[sID]->u16ColorTemp = WBInfo[mainID]->u16ColorTemp;
			return;
		}
	}

	AWB_SetFaceArea(sID);
	AWB_CalculateBalanceWBGain(sID);
	AWB_AdjustWB(sID, &WBInfo[sID]->stFinalWB);
	AWB_PreviewConverge(sID, &WBInfo[sID]->stCurrentWB);

	WBInfo[sID]->u16ColorTemp = AWB_GetCurColorTemperatureRB(sID, WBInfo[sID]->stCurrentWB.u16RGain,
		WBInfo[sID]->stCurrentWB.u16BGain);
	WBInfo[sID]->u32RunCnt++;
}

static void AWB_ParamInit(CVI_U8 sID, const ISP_AWB_PARAM_S *pstAwbParam)
{
	CVI_U8 isWdrMode, zoneRow, zoneCol;

	sID = AWB_CheckSensorID(sID);
	isWdrMode = pstAwbParam->u8WDRMode;

	IsAwbRunning[sID] = 1;
	zoneRow = pstAwbParam->u8AWBZoneRow;
	zoneCol = pstAwbParam->u8AWBZoneCol;
	//param allocate
	//Make sure the stack of functions is not too large
	if (!pstTmpSampeInfo[sID]) {
		pstTmpSampeInfo[sID] = (sWBSampleInfo *)AWB_Malloc(sID, sizeof(sWBSampleInfo));
	}

	if (!pstPointInfo[sID]) {
		pstPointInfo[sID] = (sPOINT_WB_INFO *)AWB_Malloc(sID, sizeof(sPOINT_WB_INFO) * zoneRow * zoneCol);
	}
	//param init
	WBInfo[sID]->stBalanceWB = WBInfo[sID]->stCurrentWB = WBInfo[sID]->stGrayWorldWB = WBInfo[sID]->stDefaultWB;
	WBInfo[sID]->stAssignedWB.u16RGain = AWB_GAIN_BASE;
	WBInfo[sID]->stAssignedWB.u16BGain = AWB_GAIN_BASE;

	WBInfo[sID]->u8WBConvergeMode = WB_CONVERGED_FAST;
	WBInfo[sID]->u32FirstStableTime = 0;
	WBInfo[sID]->bUseGrayWorld = 0;
	WBInfo[sID]->bEnableSmoothAWB = 1;
	bNeedUpdateCT_OutDoor[sID] = 1;

	if (WBInfo[sID]->u16WBRowSize != pstAwbParam->u8AWBZoneRow) {
		WBInfo[sID]->u16WBRowSize = pstAwbParam->u8AWBZoneRow;
		printf("Err: RowSize\n");
	}
	if (WBInfo[sID]->u16WBColumnSize != pstAwbParam->u8AWBZoneCol) {
		WBInfo[sID]->u16WBColumnSize = pstAwbParam->u8AWBZoneCol;
		printf("Err: ColSize\n");
	}
	WBInfo[sID]->u8AwbMaxFrameNum = (isWdrMode) ? 2 : 1;

	WBInfo[sID]->u8Saturation[WB_WDR_LE] = 128;
	WBInfo[sID]->u8Saturation[WB_WDR_SE] = 128;
	WBInfo[sID]->u8Saturation[2] = 128;//reserve
	WBInfo[sID]->u8Saturation[3] = 128;//reserve

	printf("isWdr:%d\n", isWdrMode);
}

static void AWB_ParamDeinit(CVI_U8 sID)
{
	sID = AWB_CheckSensorID(sID);
	IsAwbRunning[sID] = 0;

	if (pstTmpSampeInfo[sID]) {
		AWB_Free(sID, pstTmpSampeInfo[sID]);
		pstTmpSampeInfo[sID] = NULL;
	}
	if (pAwbDbg[sID]) {
		AWB_Free(sID, pAwbDbg[sID]);
		pAwbDbg[sID] = NULL;
	}
	if (pstPointInfo[sID]) {
		AWB_Free(sID, pstPointInfo[sID]);
		pstPointInfo[sID] = NULL;
	}
}

CVI_U16 AWB_GetCurColorTemperatureRB(CVI_U8 sID, CVI_U16 curRGain, CVI_U16 curBGain)
{
	CVI_S32 cur_CT;
	CVI_FLOAT fcurRB = 1;
	CVI_U16 i;
	CVI_FLOAT ct_l, ct_h, rb_l, rb_h;

	sID = AWB_CheckSensorID(sID);

	cur_CT = MIN_COLOR_TEMPERATURE;
	if (curBGain) {
		fcurRB = curRGain / (CVI_FLOAT)curBGain;
	}

	ct_l = sCtRbTab[sID].ct[0];
	ct_h = sCtRbTab[sID].ct[CT_RB_SIZE - 1];
	rb_l = sCtRbTab[sID].rb[0];
	rb_h = sCtRbTab[sID].rb[CT_RB_SIZE - 1];

	if (fcurRB < sCtRbTab[sID].rb[0]) {
	} else if (fcurRB > sCtRbTab[sID].rb[CT_RB_SIZE - 1]) {
	} else {
		for (i = 0; i < CT_RB_SIZE; i++) {
			if (sCtRbTab[sID].rb[i] >= fcurRB) {
				break;
			}
		}
		ct_l = sCtRbTab[sID].ct[i - 1];
		ct_h = sCtRbTab[sID].ct[i];
		rb_l = sCtRbTab[sID].rb[i - 1];
		rb_h = sCtRbTab[sID].rb[i];
	}
	if (rb_h != rb_l) {
		cur_CT = (ct_h - ct_l) * (fcurRB - rb_l) / (rb_h - rb_l) + ct_l;
	}

	AWB_LIMIT(cur_CT, MIN_COLOR_TEMPERATURE, MAX_COLOR_TEMPERATURE);
	return cur_CT;
}

static CVI_U16 AWB_GetGainByColorTemperature(CVI_U8 sID, CVI_U16 u16CT)
{
	CVI_U16 minRGain, maxRGain, midRGain;
	CVI_U16 minCT, maxCT, midCT;
	CVI_U16 RGain, GoldenR;
	CVI_U16 limitRmax, limitRmin;
	CVI_FLOAT fct;

	sID = AWB_CheckSensorID(sID);
	limitRmin = WBInfo[sID]->stAWBBound.u16RLowBound;
	limitRmax = WBInfo[sID]->stAWBBound.u16RHighBound;
	minRGain = WBInfo[sID]->stCalibWBGain[0].u16RGain;
	midRGain = WBInfo[sID]->stCalibWBGain[1].u16RGain;
	maxRGain = WBInfo[sID]->stCalibWBGain[2].u16RGain;

	minCT = WBInfo[sID]->u16CalibColorTemp[0];
	midCT = WBInfo[sID]->u16CalibColorTemp[1];
	maxCT = WBInfo[sID]->u16CalibColorTemp[2];

	if (u16CT < minCT) {//extrapolation
		RGain = minRGain - (midRGain - minRGain) * (minCT - u16CT) / AWB_DIV_0_TO_1(midCT - minCT);
	} else if (u16CT > maxCT) {//extrapolation
		RGain = maxRGain + (maxRGain - midRGain) * (u16CT - maxCT) / AWB_DIV_0_TO_1(maxCT - midCT);
	} else {
		fct = u16CT;
		RGain = fct*fct*fCTtoRgCurveP[sID][0] + fct*fCTtoRgCurveP[sID][1] + fCTtoRgCurveP[sID][2];
	}

	GoldenR = stAwbAttrInfo[sID]->stAuto.au16StaticWB[ISP_BAYER_CHN_R];
	if (GoldenR != AWB_GAIN_BASE) {
		RGain = RGain * GoldenR / AWB_DIV_0_TO_1(midRGain);
	}
	AWB_LIMIT(RGain, limitRmin, limitRmax);
	return RGain;
}

CVI_U16 AWB_GetRBGainByColorTemperature(CVI_U8 sID, CVI_U16 u16CT, CVI_S16 s16Shift, CVI_U16 *pu16AWBGain)
{
	CVI_U16 minRGain, maxRGain, midRGain;
	CVI_U16 minCT, maxCT, midCT;
	CVI_U16 RGain, BGain, inx, GoldenR;
	CVI_U16 limitRmax, limitRmin;
	CVI_FLOAT fct;

	sID = AWB_CheckSensorID(sID);
	limitRmin = WBInfo[sID]->stAWBBound.u16RLowBound;
	limitRmax = WBInfo[sID]->stAWBBound.u16RHighBound;
	minRGain = WBInfo[sID]->stCalibWBGain[0].u16RGain;
	midRGain = WBInfo[sID]->stCalibWBGain[1].u16RGain;
	maxRGain = WBInfo[sID]->stCalibWBGain[2].u16RGain;

	minCT = WBInfo[sID]->u16CalibColorTemp[0];
	midCT = WBInfo[sID]->u16CalibColorTemp[1];
	maxCT = WBInfo[sID]->u16CalibColorTemp[2];

	if (u16CT < minCT) {//extrapolation
		RGain = minRGain - (midRGain - minRGain) * (minCT - u16CT) / AWB_DIV_0_TO_1(midCT - minCT);
	} else if (u16CT > maxCT) {//extrapolation
		RGain = maxRGain + (maxRGain - midRGain) * (u16CT - maxCT) / AWB_DIV_0_TO_1(maxCT - midCT);
	} else {
		fct = u16CT;
		RGain = fct*fct*fCTtoRgCurveP[sID][0] + fct*fCTtoRgCurveP[sID][1] + fCTtoRgCurveP[sID][2];
	}
	GoldenR = stAwbAttrInfo[sID]->stAuto.au16StaticWB[ISP_BAYER_CHN_R];
	if (GoldenR != AWB_GAIN_BASE) {
		RGain = RGain * GoldenR / AWB_DIV_0_TO_1(midRGain);
	}

	AWB_LIMIT(RGain, limitRmin, limitRmax);

	if (RGain > WBInfo[sID]->stAWBBound.u16RLowBound) {
		inx = (RGain - WBInfo[sID]->stAWBBound.u16RLowBound) >> AWB_STEP_BIT;
		AWB_LIMIT_UPPER(inx, (AWB_CURVE_SIZE - 1));
	} else {
		inx = 0;
	}
	BGain = WBInfo[sID]->u16CTCurve[inx];
	if (s16Shift < 0 && s16Shift > (-64)) {
		RGain += s16Shift;
	} else if (s16Shift >= 0 && s16Shift < 64) {
		RGain -= s16Shift;
	}
	pu16AWBGain[ISP_BAYER_CHN_R] = RGain;
	pu16AWBGain[ISP_BAYER_CHN_GR] = AWB_GAIN_BASE;
	pu16AWBGain[ISP_BAYER_CHN_GB] = AWB_GAIN_BASE;
	pu16AWBGain[ISP_BAYER_CHN_B] = BGain;
	return RGain;
}

static void AWB_SaveBootInfo(CVI_U8 sID)
{
	static CVI_U8 frmCnt[AWB_SENSOR_NUM];

	sID = AWB_CheckSensorID(sID);

	if (frmCnt[sID] < AWB_BOOT_MAX_FRAME) {
		WBInfo[sID]->stBootGain[frmCnt[sID]] = WBInfo[sID]->stCurrentWB;
		WBInfo[sID]->u16BootColorTemp[frmCnt[sID]] = WBInfo[sID]->u16ColorTemp;
		WBInfo[sID]->bBootConvergeStatus[frmCnt[sID]] = WBInfo[sID]->u8WBConvergeMode;
		frmCnt[sID]++;
	}
}

void AWB_SetGrayWorld(CVI_U8 sID, CVI_BOOL enable)
{
	sID = AWB_CheckSensorID(sID);
	WBInfo[sID]->bUseGrayWorld = enable;
}

void AWB_SetCurveBoundary(CVI_U8 sID, CVI_U16 lowCT, CVI_U16 highCT)
{
	ISP_WB_ATTR_S tmpWBAttr;

	sID = AWB_CheckSensorID(sID);
	AWB_GetAttr(sID, &tmpWBAttr);
	tmpWBAttr.stAuto.u16LowColorTemp = lowCT;
	tmpWBAttr.stAuto.u16HighColorTemp = highCT;
	AWB_SetAttr(sID, &tmpWBAttr);
	printf("WB lowCT:%d highCT:%d\n", lowCT, highCT);
}

void AWB_SetRGStrength(CVI_U8 sID, CVI_U8 RG, CVI_U8 BG)
{
	ISP_WB_ATTR_S tmpWBAttr;

	sID = AWB_CheckSensorID(sID);
	AWB_GetAttr(sID, &tmpWBAttr);
	tmpWBAttr.stAuto.u8RGStrength = RG;
	tmpWBAttr.stAuto.u8BGStrength = BG;
	AWB_SetAttr(sID, &tmpWBAttr);
	printf("WB RG:%d BG:%d\n", RG, BG);
}

void AWB_GetCurveBoundary(CVI_U8 sID, sWBBoundary *psWBBound)
{
	sID = AWB_CheckSensorID(sID);
	*psWBBound = stBoundaryInfo[sID];
}

void AWB_GetCurveRange(CVI_U8 sID, sWBCurveInfo *psWBCurve)
{
	sID = AWB_CheckSensorID(sID);
	*psWBCurve = stCurveInfo[sID];
}

void AWB_SetAttr(CVI_U8 sID, const ISP_WB_ATTR_S *pstWBAttr)
{
	sID = AWB_CheckSensorID(sID);

	if (pstAwbMpiAttr[sID] == CVI_NULL) {
		ISP_LOG_ERR("%s\n", "pstAwbMpiAttr is NULL");
		return;
	}

	*pstAwbMpiAttr[sID] = *pstWBAttr;

	//add more check here?
	AWB_SetParamUpdateFlag(sID, AWB_ATTR_UPDATE);

	if (pstAwbMpiAttr[sID]->u8DebugMode == 255) {
		pstAwbMpiAttr[sID]->u8DebugMode = 0;
		AWB_DumpLog(sID);  // TODO: mason.zou
		AWB_DumpDbgBin(sID);
	}
	if (pstAwbMpiAttr[sID]->u8DebugMode == 77)
		AWB_ShowInfoList();
	//AWB_ShowAttrInfo(sID, 0);
}

void AWB_SetAttrEx(CVI_U8 sID, const ISP_AWB_ATTR_EX_S *pstAWBAttrEx)
{
	sID = AWB_CheckSensorID(sID);

	if (pstAwbMpiAttrEx[sID] == CVI_NULL) {
		ISP_LOG_ERR("%s\n", "pstAwbMpiAttrEx is NULL");
		return;
	}

	*pstAwbMpiAttrEx[sID] = *pstAWBAttrEx;
	AWB_SetParamUpdateFlag(sID, AWB_ATTR_EX_UPDATE);
}

void AWB_GetAttr(CVI_U8 sID, ISP_WB_ATTR_S *pstWBAttr)
{
	sID = AWB_CheckSensorID(sID);

	if (pstAwbMpiAttr[sID] == CVI_NULL) {
		ISP_LOG_ERR("%s\n", "pstAwbMpiAttr is NULL");
		return;
	}

	*pstWBAttr = *pstAwbMpiAttr[sID];
}

void AWB_GetAttrEx(CVI_U8 sID, ISP_AWB_ATTR_EX_S *pstAWBAttrEx)
{
	sID = AWB_CheckSensorID(sID);

	if (pstAwbMpiAttrEx[sID] == CVI_NULL) {
		ISP_LOG_ERR("%s\n", "pstAwbMpiAttrEx is NULL");
		return;
	}

	*pstAWBAttrEx = *pstAwbMpiAttrEx[sID];
}

static void AWB_SetInfo(CVI_U8 sID, const sWBGain *pstcurWBGain)
{
	sWBGain wbGain;
	CVI_U16 colorTemp;

	sID = AWB_CheckSensorID(sID);
	if (stAwbAttrInfo[sID]->bByPass) {
		wbGain.u16RGain = AWB_GAIN_BASE;
		wbGain.u16GGain = AWB_GAIN_BASE;
		wbGain.u16BGain = AWB_GAIN_BASE;
		colorTemp = AWB_GetCurColorTemperatureRB(sID, AWB_GAIN_BASE, AWB_GAIN_BASE);
	} else {
		if (stAwbAttrInfo[sID]->enOpType == OP_TYPE_MANUAL) {
			wbGain.u16RGain = stAwbAttrInfo[sID]->stManual.u16Rgain;
			wbGain.u16GGain = stAwbAttrInfo[sID]->stManual.u16Grgain;
			wbGain.u16BGain = stAwbAttrInfo[sID]->stManual.u16Bgain;
			colorTemp = WBInfo[sID]->u16ManualColorTemp;
		} else {
			wbGain.u16RGain = pstcurWBGain->u16RGain;
			#if defined(__CV180X__)
			#define THRESHOLD (30)
			//note: fix CV180X rtl bug
			//patch for not use global/bright tone node256
			//4095_Luma will map incorrectly on global tone & bright tone
			//we use awb Ggain make the pixel's max LumaValue descend to (4095-16)
			CVI_FLOAT ratio = (CVI_FLOAT)(4095 - THRESHOLD) / 4095;
			wbGain.u16GGain = AWB_GAIN_BASE * ratio;
			#else
			wbGain.u16GGain = AWB_GAIN_BASE;
			#endif
			wbGain.u16BGain = pstcurWBGain->u16BGain;
			colorTemp = WBInfo[sID]->u16ColorTemp;
		}
	}

	pstAwb_Q_Info[sID]->u16Rgain = wbGain.u16RGain;
	pstAwb_Q_Info[sID]->u16Grgain = wbGain.u16GGain;
	pstAwb_Q_Info[sID]->u16Gbgain = wbGain.u16GGain;
	pstAwb_Q_Info[sID]->u16Bgain = wbGain.u16BGain;

	pstAwb_Q_Info[sID]->u16ColorTemp = colorTemp;
	pstAwb_Q_Info[sID]->u32FirstStableTime = WBInfo[sID]->u32FirstStableTime;
	pstAwb_Q_Info[sID]->enInOutStatus = stAwbAttrInfoEx[sID]->stInOrOut.enOutdoorStatus;
	pstAwb_Q_Info[sID]->s16Bv = WBInfo[sID]->stEnvInfo.s16LvX100;
	pstAwb_Q_Info[sID]->u16GrayWorldRgain = WBInfo[sID]->stGrayWorldWB.u16RGain;
	pstAwb_Q_Info[sID]->u16GrayWorldBgain = WBInfo[sID]->stGrayWorldWB.u16BGain;
}

void AWB_GetQueryInfo(CVI_U8 sID, ISP_WB_Q_INFO_S *pstWB_Q_Info)
{
	sID = AWB_CheckSensorID(sID);

	if (pstAwb_Q_Info[sID] == CVI_NULL) {
		ISP_LOG_ERR("%s\n", "pstAwb_Q_Info is NULL");
		return;
	}

	*pstWB_Q_Info = *pstAwb_Q_Info[sID];
}

void AWB_SetManualGain(CVI_U8 sID, CVI_U16 RGain, CVI_U16 GGain, CVI_U16 BGain)
{
	ISP_WB_ATTR_S tmpWBAttr;

	sID = AWB_CheckSensorID(sID);
	AWB_GetAttr(sID, &tmpWBAttr);
	tmpWBAttr.enOpType = OP_TYPE_MANUAL;
	tmpWBAttr.stManual.u16Rgain = RGain;
	tmpWBAttr.stManual.u16Grgain = GGain;
	tmpWBAttr.stManual.u16Gbgain = GGain;
	tmpWBAttr.stManual.u16Bgain = BGain;
	AWB_SetAttr(sID, &tmpWBAttr);
	printf("WB manual gain:%d %d %d\n", RGain, GGain, BGain);
}

void AWB_SetByPass(CVI_U8 sID, CVI_BOOL enalbe)
{
	ISP_WB_ATTR_S tmpWBAttr;

	sID = AWB_CheckSensorID(sID);
	AWB_GetAttr(sID, &tmpWBAttr);
	tmpWBAttr.bByPass = enalbe;
	AWB_SetAttr(sID, &tmpWBAttr);
	printf("WB bypass:%d\n", enalbe);
}

static void AwbAttr_Init(CVI_U8 sID)
{
	CVI_U16 j;

	sID = AWB_CheckSensorID(sID);

	PRINT_FUNC("sID:%d", sID);
	memset(pstAwbMpiAttr[sID], 0, sizeof(ISP_WB_ATTR_S));
	memset(pstAwbMpiAttrEx[sID], 0, sizeof(ISP_AWB_ATTR_EX_S));

	pstAwbMpiAttr[sID]->bByPass = 0;
	pstAwbMpiAttr[sID]->u8AWBRunInterval = AWB_PERIOD_NUM;
	pstAwbMpiAttr[sID]->enOpType = OP_TYPE_AUTO;
	pstAwbMpiAttr[sID]->enAlgType = ALG_AWB;//TODO@CLIFF.ALG_AWB_SPEC
	pstAwbMpiAttr[sID]->u8DebugMode = 0;

	pstAwbMpiAttr[sID]->stManual.u16Rgain = AWB_GAIN_BASE;
	pstAwbMpiAttr[sID]->stManual.u16Grgain = AWB_GAIN_BASE;
	pstAwbMpiAttr[sID]->stManual.u16Gbgain = AWB_GAIN_BASE;
	pstAwbMpiAttr[sID]->stManual.u16Bgain = AWB_GAIN_BASE;

	pstAwbMpiAttr[sID]->stAuto.bEnable = 1;
	pstAwbMpiAttr[sID]->stAuto.u16RefColorTemp = 5000;
	pstAwbMpiAttr[sID]->stAuto.au16StaticWB[ISP_BAYER_CHN_R] = 1024;
	pstAwbMpiAttr[sID]->stAuto.au16StaticWB[ISP_BAYER_CHN_GR] = 1024;
	pstAwbMpiAttr[sID]->stAuto.au16StaticWB[ISP_BAYER_CHN_GB] = 1024;
	pstAwbMpiAttr[sID]->stAuto.au16StaticWB[ISP_BAYER_CHN_B] = 1024;
	pstAwbMpiAttr[sID]->stAuto.as32CurvePara[0] = 97;//2132;
	pstAwbMpiAttr[sID]->stAuto.as32CurvePara[1] = -507218;//-8663114;
	pstAwbMpiAttr[sID]->stAuto.as32CurvePara[2] = 806676416;//1051059277;
	pstAwbMpiAttr[sID]->stAuto.as32CurvePara[3] = -61173593;
	pstAwbMpiAttr[sID]->stAuto.as32CurvePara[4] = 264512167;
	pstAwbMpiAttr[sID]->stAuto.as32CurvePara[5] = -219820585;

	pstAwbMpiAttr[sID]->stAuto.enAlgType = AWB_ALG_ADVANCE;
	pstAwbMpiAttr[sID]->stAuto.u8RGStrength = 128;
	pstAwbMpiAttr[sID]->stAuto.u8BGStrength = 128;
	pstAwbMpiAttr[sID]->stAuto.u16Speed = 256;
	pstAwbMpiAttr[sID]->stAuto.u16ZoneSel = 32;
	pstAwbMpiAttr[sID]->stAuto.u16HighColorTemp = 9500;
	pstAwbMpiAttr[sID]->stAuto.u16LowColorTemp = 2500;

	pstAwbMpiAttr[sID]->stAuto.stCTLimit.bEnable = 1;
	pstAwbMpiAttr[sID]->stAuto.stCTLimit.enOpType = OP_TYPE_AUTO;
	pstAwbMpiAttr[sID]->stAuto.stCTLimit.u16HighRgLimit = 2500;//H temp,Rgain max
	pstAwbMpiAttr[sID]->stAuto.stCTLimit.u16HighBgLimit = 512;//H temp,Bgain min
	pstAwbMpiAttr[sID]->stAuto.stCTLimit.u16LowRgLimit = 512;//Low temp,Rgain min
	pstAwbMpiAttr[sID]->stAuto.stCTLimit.u16LowBgLimit = 2500;//Low temp,Bgain max

	pstAwbMpiAttr[sID]->stAuto.bShiftLimitEn = 1;
	//low bot,low top ---- high bot,high Top
	for (j = 0; j < AWB_CURVE_BOUND_NUM - 1; ++j) {
		pstAwbMpiAttr[sID]->stAuto.u16ShiftLimit[j] = 480;
	}
	pstAwbMpiAttr[sID]->stAuto.u16ShiftLimit[0] += 160;//low bot
	pstAwbMpiAttr[sID]->stAuto.u16ShiftLimit[1] += 160;//low top
	pstAwbMpiAttr[sID]->stAuto.u16ShiftLimit[7] = 240;//high top

	pstAwbMpiAttr[sID]->stAuto.bGainNormEn = 1;
	pstAwbMpiAttr[sID]->stAuto.bNaturalCastEn = 0;

	pstAwbMpiAttr[sID]->stAuto.stCbCrTrack.bEnable = 0;
	for (j = 0; j < ISP_AUTO_ISO_STRENGTH_NUM; ++j) {
		pstAwbMpiAttr[sID]->stAuto.stCbCrTrack.au16CrMax[j] = 1100;//R/G at L tmp
		pstAwbMpiAttr[sID]->stAuto.stCbCrTrack.au16CrMin[j] = 400;//R/G at H tmp
		pstAwbMpiAttr[sID]->stAuto.stCbCrTrack.au16CbMax[j] = 750;//B/G at H tmp
		pstAwbMpiAttr[sID]->stAuto.stCbCrTrack.au16CbMin[j] = 256;//B/G at L tmp
	}

	pstAwbMpiAttr[sID]->stAuto.stLumaHist.bEnable = 1;
	pstAwbMpiAttr[sID]->stAuto.stLumaHist.enOpType = OP_TYPE_AUTO;
	pstAwbMpiAttr[sID]->stAuto.stLumaHist.au8HistThresh[0] = 0;
	pstAwbMpiAttr[sID]->stAuto.stLumaHist.au16HistWt[0] = 32;
	pstAwbMpiAttr[sID]->stAuto.stLumaHist.au8HistThresh[1] = 4;
	pstAwbMpiAttr[sID]->stAuto.stLumaHist.au16HistWt[1] = 128;
	pstAwbMpiAttr[sID]->stAuto.stLumaHist.au8HistThresh[2] = 16;
	pstAwbMpiAttr[sID]->stAuto.stLumaHist.au16HistWt[2] = 384;
	pstAwbMpiAttr[sID]->stAuto.stLumaHist.au8HistThresh[3] = 128;
	pstAwbMpiAttr[sID]->stAuto.stLumaHist.au16HistWt[3] = 512;
	pstAwbMpiAttr[sID]->stAuto.stLumaHist.au8HistThresh[4] = 235;
	pstAwbMpiAttr[sID]->stAuto.stLumaHist.au16HistWt[4] = 256;
	pstAwbMpiAttr[sID]->stAuto.stLumaHist.au8HistThresh[5] = 255;
	pstAwbMpiAttr[sID]->stAuto.stLumaHist.au16HistWt[5] = 32;

	pstAwbMpiAttr[sID]->stAuto.bAWBZoneWtEn = 0;
	for (j = 0; j < AWB_ZONE_WT_NUM; ++j) {
		pstAwbMpiAttr[sID]->stAuto.au8ZoneWt[j] = AWB_ZONE_WT_DEF;
	}

	pstAwbMpiAttrEx[sID]->u8Tolerance = 2;
	pstAwbMpiAttrEx[sID]->u8ZoneRadius = 10;
	pstAwbMpiAttrEx[sID]->u16CurveLLimit = 320;
	pstAwbMpiAttrEx[sID]->u16CurveRLimit = 768;

	pstAwbMpiAttrEx[sID]->bExtraLightEn = 0;

	pstAwbMpiAttrEx[sID]->stInOrOut.bEnable = 1;
	pstAwbMpiAttrEx[sID]->stInOrOut.enOpType = OP_TYPE_AUTO;
	pstAwbMpiAttrEx[sID]->stInOrOut.enOutdoorStatus = AWB_INDOOR_MODE;
	pstAwbMpiAttrEx[sID]->stInOrOut.u32OutThresh = 14;
	pstAwbMpiAttrEx[sID]->stInOrOut.u16LowStart = 5000;
	pstAwbMpiAttrEx[sID]->stInOrOut.u16LowStop = 4500;
	pstAwbMpiAttrEx[sID]->stInOrOut.u16HighStart = 6000;
	pstAwbMpiAttrEx[sID]->stInOrOut.u16HighStop = 7200;
	pstAwbMpiAttrEx[sID]->stInOrOut.bGreenEnhanceEn = 1;//TODO@CLIFF
	pstAwbMpiAttrEx[sID]->stInOrOut.u8OutShiftLimit = 32;//TODO@CLIFF

	pstAwbMpiAttrEx[sID]->bMultiLightSourceEn = CVI_TRUE;
	pstAwbMpiAttrEx[sID]->enMultiLSType = AWB_MULTI_LS_SAT;
	pstAwbMpiAttrEx[sID]->u16MultiLSScaler = 0x100;
	pstAwbMpiAttrEx[sID]->u16MultiLSThr = 0x60;
	pstAwbMpiAttrEx[sID]->u16CALumaDiff = 0x20;

	CVI_U16 ctBin[AWB_CT_BIN_NUM] = {2300, 2800, 3500, 4800, 5500, 6300, 7000, 8500};
	CVI_U16 ctWt[AWB_CT_BIN_NUM] = {64, 256, 256, 256, 256, 256, 256, 256};

	for (j = 0; j < AWB_CT_BIN_NUM; j++) {
		pstAwbMpiAttrEx[sID]->au16MultiCTBin[j] = ctBin[j];
		pstAwbMpiAttrEx[sID]->au16MultiCTWt[j] = ctWt[j];
	}

	pstAwbMpiAttrEx[sID]->bFineTunEn = 1;//TODO@CLIFF
	pstAwbMpiAttrEx[sID]->u8FineTunStrength = 128;//TODO@CLIFF


	//v6.x init
	pstAwbMpiAttrEx[sID]->adjBgainMode = 0;
	pstAwbMpiAttrEx[sID]->stShiftLv.u8LowLvMode = 0;
	pstAwbMpiAttrEx[sID]->stShiftLv.u16LowLvCT[0] = LOWTEMPBOT;
	pstAwbMpiAttrEx[sID]->stShiftLv.u16LowLvCT[1] = HIGHTEMPBOT | HIGHTEMPTOP;
	pstAwbMpiAttrEx[sID]->stShiftLv.u16LowLvThr[0] = 800;
	pstAwbMpiAttrEx[sID]->stShiftLv.u16LowLvThr[1] = 600;
	pstAwbMpiAttrEx[sID]->stShiftLv.u16LowLvRatio[0] = 150;
	pstAwbMpiAttrEx[sID]->stShiftLv.u16LowLvRatio[1] = 300;
	pstAwbMpiAttrEx[sID]->stShiftLv.u8HighLvMode = 0;
	pstAwbMpiAttrEx[sID]->stShiftLv.u16HighLvCT[0] = LOWTEMPBOT | LOWTEMPTOP;
	pstAwbMpiAttrEx[sID]->stShiftLv.u16HighLvCT[1] = 0;
	pstAwbMpiAttrEx[sID]->stShiftLv.u16HighLvThr[0] = 1000;
	pstAwbMpiAttrEx[sID]->stShiftLv.u16HighLvThr[1] = 1000;
	pstAwbMpiAttrEx[sID]->stShiftLv.u16HighLvRatio[0] = 300;
	pstAwbMpiAttrEx[sID]->stShiftLv.u16HighLvRatio[1] = 100;
	pstAwbMpiAttrEx[sID]->stRegion.u16Region1 = CT_SHIFT_LIMIT_REGION1;
	pstAwbMpiAttrEx[sID]->stRegion.u16Region2 = CT_SHIFT_LIMIT_REGION2;
	pstAwbMpiAttrEx[sID]->stRegion.u16Region3 = CT_SHIFT_LIMIT_REGION3;

	for (j = 0; j < AWB_CT_BIN_NUM; j++) {
		pstAwbMpiAttrEx[sID]->stCtLv.au16MultiCTBin[j] = ctBin[j];
		pstAwbMpiAttrEx[sID]->stCtLv.au16MultiCTWt[0][j] = ctWt[j];
		pstAwbMpiAttrEx[sID]->stCtLv.au16MultiCTWt[1][j] = ctWt[j];
		pstAwbMpiAttrEx[sID]->stCtLv.au16MultiCTWt[2][j] = ctWt[j];
		pstAwbMpiAttrEx[sID]->stCtLv.au16MultiCTWt[3][j] = ctWt[j];
	}
	pstAwbMpiAttrEx[sID]->stCtLv.s8ThrLv[0] = 1;
	pstAwbMpiAttrEx[sID]->stCtLv.s8ThrLv[1] = 5;
	pstAwbMpiAttrEx[sID]->stCtLv.s8ThrLv[2] = 9;
	pstAwbMpiAttrEx[sID]->stCtLv.s8ThrLv[3] = 13;

	pstAwbMpiAttrEx[sID]->stInterference.u8Mode = 1;
	pstAwbMpiAttrEx[sID]->stInterference.u8Limit = 85;
	pstAwbMpiAttrEx[sID]->stInterference.u8Radius = 5;
	pstAwbMpiAttrEx[sID]->stInterference.u8Distance = 10;
	pstAwbMpiAttrEx[sID]->stInterference.u8Ratio = 50;

	*stAwbAttrInfo[sID] = *pstAwbMpiAttr[sID];
	*stAwbAttrInfoEx[sID] = *pstAwbMpiAttrEx[sID];
}

static void AWB_Function_Init(CVI_U8 sID)
{
	AwbAttr_Init(sID);
}


void AWB_GetCurveWhiteZone(CVI_U8 sID, ISP_WB_CURVE_S *pstWBCurve)
{
	UNUSED(sID);
	UNUSED(pstWBCurve);
}

void AWB_ShowWhiteZone(CVI_U8 sID, CVI_U8 mode)
{
	UNUSED(sID);
	UNUSED(mode);
}

CVI_U8 AWB_ViPipe2sID(VI_PIPE ViPipe)
{
	CVI_U8 sID = AWB_CheckSensorID(ViPipe);

	return sID;
}

static void AWB_CalibrationInit(VI_PIPE ViPipe, ISP_AWB_Calibration_Gain_S *pstWBCalbration)
{
	ISP_WB_ATTR_S tmpWbAttr;
	CVI_U8 sID;

	sID = AWB_ViPipe2sID(ViPipe);
	AWB_GetAttr(sID, &tmpWbAttr);

	pstWBCalbration->u16AvgRgain[0] = 1400;
	pstWBCalbration->u16AvgBgain[0] = 3100;
	pstWBCalbration->u16ColorTemperature[0] = 2850;

	pstWBCalbration->u16AvgRgain[1] = 1500;
	pstWBCalbration->u16AvgBgain[1] = 2500;
	pstWBCalbration->u16ColorTemperature[1] = 3900;

	pstWBCalbration->u16AvgRgain[2] = 2300;
	pstWBCalbration->u16AvgBgain[2] = 1600;
	pstWBCalbration->u16ColorTemperature[2] = 6500;

	*stWbDefCalibration[sID] = *pstWBCalbration;
	AWB_BuildCurve(sID, pstWBCalbration, &tmpWbAttr);
	AWB_SetAttr(sID, &tmpWbAttr);
}

static void AWB_GoldenGainInit(VI_PIPE ViPipe)
{
	AWB_CTX_S *pstAwbCtx;
	CVI_U8 sID;
	ISP_AWB_Calibration_Gain_S wbcalib[AWB_SENSOR_NUM];

	pstAwbCtx = AWB_GET_CTX(ViPipe);
#ifndef AAA_PC_PLATFORM
	if (pstAwbCtx->stSnsRegister.stAwbExp.pfn_cmos_get_awb_default)
		pstAwbCtx->stSnsRegister.stAwbExp.pfn_cmos_get_awb_default(ViPipe, &pstAwbCtx->stSnsDft);
#endif
	sID = AWB_ViPipe2sID(ViPipe);
	//TODO@CLIFF./
	wbcalib[sID].u16AvgRgain[0] = DEFAULT_RGAIN_RATIO * AWB_GAIN_BASE;
	wbcalib[sID].u16AvgBgain[0] = DEFAULT_BGAIN_RATIO * AWB_GAIN_BASE;

	pstAwbCtx->stSnsDft.u16GoldenRgain = DEFAULT_RGAIN_RATIO * AWB_GAIN_BASE;
	pstAwbCtx->stSnsDft.u16GoldenBgain = DEFAULT_BGAIN_RATIO * AWB_GAIN_BASE;

	WBInfo[sID]->stDefaultWB.u16RGain = wbcalib[sID].u16AvgRgain[0];
	WBInfo[sID]->stDefaultWB.u16GGain = AWB_GAIN_BASE;
	WBInfo[sID]->stDefaultWB.u16BGain = wbcalib[sID].u16AvgBgain[0];

	WBInfo[sID]->stGoldenWB.u16RGain = pstAwbCtx->stSnsDft.u16GoldenRgain;
	WBInfo[sID]->stGoldenWB.u16GGain = AWB_GAIN_BASE;
	WBInfo[sID]->stGoldenWB.u16BGain = pstAwbCtx->stSnsDft.u16GoldenBgain;
}

static void AWB_ConfigWin(VI_PIPE ViPipe)
{
	CVI_S32 s32Ret;
	ISP_STATISTICS_CFG_S stStatCfg;

	s32Ret = CVI_ISP_GetStatisticsConfig(ViPipe, &stStatCfg);
	if (s32Ret != CVI_SUCCESS) {
		ISP_LOG_DEBUG("CVI_ISP_GetStatisticsConfig fail, 0x%x\n", s32Ret);
		return;
	}
	//12 bit
	stStatCfg.stWBCfg.u16BlackLevel = 0;
	stStatCfg.stWBCfg.u16WhiteLevel = 4095;
	s32Ret = CVI_ISP_SetStatisticsConfig(ViPipe, &stStatCfg);
	if (s32Ret != CVI_SUCCESS) {
		ISP_LOG_DEBUG("CVI_ISP_SetStatisticsConfig fail, 0x%x\n", s32Ret);
		return;
	}
	//get stStatCfg cfg here, check at AWB_ParamInit()
	WBInfo[ViPipe]->u16WBRowSize = stStatCfg.stWBCfg.u16ZoneRow;
	WBInfo[ViPipe]->u16WBColumnSize = stStatCfg.stWBCfg.u16ZoneCol;
}

CVI_S32 awbInit(VI_PIPE ViPipe, const ISP_AWB_PARAM_S *pstAwbParam)
{
	CVI_U8 sID;

	printf("%s ver %d.%d@%d\n", __func__, AWB_LIB_VER, AWB_LIB_SUBVER, AWB_RELEASE_DATE);

	sID = AWB_ViPipe2sID(ViPipe);

	awb_buf_init(sID);
	AWB_ConfigWin(ViPipe);
	AWB_Function_Init(sID);
	AWB_GoldenGainInit(ViPipe);
	AWB_CalibrationInit(ViPipe, pstWbCalibration[sID]);
	AWB_ParamInit(sID, (const ISP_AWB_PARAM_S *)pstAwbParam);

	memcpy(&WBInfo[sID]->stStitchAttr, &pstAwbParam->stStitchAttr, sizeof(ISP_STITCH_ATTR_S));

	isNeedUpdateAttr[sID] = 1;
	isNeedUpdateAttrEx[sID] = 1;
	isNeedUpdateCalib[sID] = 0;

	return CVI_SUCCESS;
}

CVI_S32 awbExit(VI_PIPE ViPipe)
{
	CVI_U8 sID = 0;

	sID = AWB_ViPipe2sID(ViPipe);
	AWB_ParamDeinit(sID);
	awb_buf_deinit(sID);
	AWB_CheckMemFree(sID);
	return CVI_SUCCESS;
}


#ifndef AAA_PC_PLATFORM
static void AWB_SetWBGainToResult(CVI_U8 sID, ISP_AWB_RESULT_S *pstAwbResult, const sWBGain *pstWBGain)
{
	static sWBGain wbGain[AWB_SENSOR_NUM];

	static CVI_U16 colorTemp[AWB_SENSOR_NUM];

	sID = AWB_CheckSensorID(sID);
	if (stAwbAttrInfo[sID]->bByPass) {
		wbGain[sID].u16RGain = AWB_GAIN_BASE;
		wbGain[sID].u16GGain = AWB_GAIN_BASE;
		wbGain[sID].u16BGain = AWB_GAIN_BASE;
	} else {
		if (stAwbAttrInfo[sID]->enOpType == OP_TYPE_MANUAL) {
			wbGain[sID].u16RGain = stAwbAttrInfo[sID]->stManual.u16Rgain;
			wbGain[sID].u16GGain = stAwbAttrInfo[sID]->stManual.u16Grgain;
			wbGain[sID].u16BGain = stAwbAttrInfo[sID]->stManual.u16Bgain;
			colorTemp[sID] = AWB_GetCurColorTemperatureRB(sID, wbGain[sID].u16RGain, wbGain[sID].u16BGain);
			WBInfo[sID]->u16ManualColorTemp = colorTemp[sID];
		} else {
			wbGain[sID].u16RGain = pstWBGain->u16RGain;
			wbGain[sID].u16GGain = AWB_GAIN_BASE;
			wbGain[sID].u16BGain = pstWBGain->u16BGain;
			if (WBInfo[sID]->u16ColorTemp == 0) {
				WBInfo[sID]->u16ColorTemp =
					AWB_GetCurColorTemperatureRB(sID, wbGain[sID].u16RGain, wbGain[sID].u16BGain);
			}
			colorTemp[sID] = WBInfo[sID]->u16ColorTemp;
		}
	}
#if defined(__CV180X__)
	//note: fix CV180X rtl bug
	//patch for not use global/bright tone node256
	//4095_Luma will map incorrectly on global tone & bright tone
	//we use awb Ggain make the pixel's max LumaValue descend to (4095-16)
	//Y = (0.41 * 4095 + 0.59 * G) <= 4079
	//G <= 4067
	//THRESHOLD = 4095 - 4067 = 28
#define THRESHOLD (30)
	CVI_FLOAT ratio = (CVI_FLOAT)(4095 - THRESHOLD) / 4095;
	pstAwbResult->au32WhiteBalanceGain[0] = wbGain[sID].u16RGain * ratio;
	pstAwbResult->au32WhiteBalanceGain[1] = wbGain[sID].u16GGain * ratio;
	pstAwbResult->au32WhiteBalanceGain[2] = wbGain[sID].u16GGain * ratio;
	pstAwbResult->au32WhiteBalanceGain[3] = wbGain[sID].u16BGain * ratio;
#else
	pstAwbResult->au32WhiteBalanceGain[0] = wbGain[sID].u16RGain;
	pstAwbResult->au32WhiteBalanceGain[1] = wbGain[sID].u16GGain;
	pstAwbResult->au32WhiteBalanceGain[2] = wbGain[sID].u16GGain;
	pstAwbResult->au32WhiteBalanceGain[3] = wbGain[sID].u16BGain;
#endif
	pstAwbResult->u32ColorTemp = colorTemp[sID];
	pstAwbResult->bStable = WBInfo[sID]->bStable;
	// Linear Mode	: use u8Saturation[0]
	// WDR mode@1835: use u8Saturation[0] for SE & LE
	// WDR mode@182x: use u8Saturation[0] for LE,u8Saturation[1] for SE
	pstAwbResult->u8Saturation[WB_WDR_LE] = WBInfo[sID]->u8Saturation[WB_WDR_LE];
	pstAwbResult->u8Saturation[WB_WDR_SE] = WBInfo[sID]->u8Saturation[WB_WDR_SE];
	pstAwbResult->u8Saturation[2] = WBInfo[sID]->u8Saturation[2];//reserve
	pstAwbResult->u8Saturation[3] = WBInfo[sID]->u8Saturation[3];//reserve
	pstAwbResult->u8AdjCASaturation = WBInfo[sID]->u8AdjCASaturation;
	pstAwbResult->u8AdjCASatLuma = WBInfo[sID]->u8AdjCASatLuma;
	if (AWB_GetDebugMode() == 3)
		printf("sID:%d R:%d G:%d B:%d CT:%d\n", sID, wbGain[sID].u16RGain,
		wbGain[sID].u16GGain, wbGain[sID].u16BGain, colorTemp[sID]);
}

static void AWB_GetStatisticData(VI_PIPE ViPipe, const ISP_AWB_INFO_S *pstAwbInfo)
{
	CVI_U16 i;
	CVI_U8 sID;

	sID = AWB_ViPipe2sID(ViPipe);

	for (i = 0; i < WBInfo[sID]->u8AwbMaxFrameNum; ++i) {
		WBInfo[sID]->pu16AwbStatistics[i][AWB_R] = pstAwbInfo->stAwbStat2[i].pau16ZoneAvgR;
		WBInfo[sID]->pu16AwbStatistics[i][AWB_G] = pstAwbInfo->stAwbStat2[i].pau16ZoneAvgG;
		WBInfo[sID]->pu16AwbStatistics[i][AWB_B] = pstAwbInfo->stAwbStat2[i].pau16ZoneAvgB;
	}
}

CVI_S32 awbRun(VI_PIPE ViPipe, const ISP_AWB_INFO_S *pstAwbInfo, ISP_AWB_RESULT_S *pstAwbResult, CVI_S32 s32Rsv)
{
	CVI_U8 sID;
	CVI_U32 periodT[AWB_SENSOR_NUM], processT[AWB_SENSOR_NUM];
	static struct timeval firstAwbTime[AWB_SENSOR_NUM], curAwbTime[AWB_SENSOR_NUM], preAwbTime[AWB_SENSOR_NUM],
		proAwbTime[AWB_SENSOR_NUM];
	static CVI_BOOL isFirstAwb[AWB_SENSOR_NUM];
	static CVI_U8 debugFrmCnt[AWB_SENSOR_NUM];

	sID = AWB_ViPipe2sID(ViPipe);

	if (!isFirstAwb[sID]) {
		isFirstAwb[sID] = 1;
		#ifndef ARCH_RTOS_CV181X // TODO@CV181X
		gettimeofday(&firstAwbTime[sID], NULL);
		#endif
		preAwbTime[sID].tv_sec = preAwbTime[sID].tv_usec = 0;
	}

	#ifndef ARCH_RTOS_CV181X // TODO@CV181X
	gettimeofday(&curAwbTime[sID], NULL);
	#endif
	periodT[sID] = AWB_GetMSTimeDiff(&preAwbTime[sID], &curAwbTime[sID]);
	preAwbTime[sID] = curAwbTime[sID];

	WBInfo[sID]->u32FrameID = pstAwbInfo->u32FrameCnt;

	AWB_CheckParamUpdateFlag(sID);

	if (isNeedUpdateCalib[sID]) {
		AWB_BuildCurve(sID, pstWbCalibration[sID], pstAwbMpiAttr[sID]);
		isNeedUpdateCalib[sID] = 0;
		isNeedUpdateAttr[sID] = 1;
	}

	if (isNeedUpdateAttr[sID]) {
		*stAwbAttrInfo[sID] = *pstAwbMpiAttr[sID];
		isNeedUpdateAttr[sID] = 0;

		AWB_SetDebugMode(pstAwbMpiAttr[sID]->u8DebugMode);
	}

	if (isNeedUpdateAttrEx[sID]) {
		*stAwbAttrInfoEx[sID] = *pstAwbMpiAttrEx[sID];
		isNeedUpdateAttrEx[sID] = 0;
		bNeedUpdateCT_OutDoor[sID] = 1;
	}

	if (WBInfo[sID]->u32FrameID % stAwbAttrInfo[sID]->u8AWBRunInterval != 0) {
		if (WBInfo[sID]->bEnableSmoothAWB && WBInfo[sID]->u8WBConvergeMode == WB_CONVERGED_NORMAL)
			AWB_PreviewConverge(sID, &WBInfo[sID]->stCurrentWB);
		AWB_SetWBGainToResult(sID, pstAwbResult, &WBInfo[sID]->stCurrentWB);
		return CVI_SUCCESS;
	}

	MEAUSURE_AWB_T(0);
	AWB_GetStatisticData(ViPipe, pstAwbInfo);
	WBInfo[sID]->stEnvInfo.s16LvX100 = pstAwbInfo->s16LVx100;
	WBInfo[sID]->stEnvInfo.u32ISONum = pstAwbInfo->u32IsoNum;
	WBInfo[sID]->stEnvInfo.fBVstep = pstAwbInfo->fBVstep;
	stFaceWB[sID] = pstAwbInfo->stSmartInfo;
	MEAUSURE_AWB_T(1);
	if (stAwbAttrInfo[sID]->stAuto.bEnable)
		AWB_RunAlgo(sID);
	MEAUSURE_AWB_T(2);

	#if CHECK_AWB_TIMING
	printf("TT1:%d\n", AWB_GetUSTimeDiff(&ttawb[0], &ttawb[1]));
	printf("TT2:%d\n", AWB_GetUSTimeDiff(&ttawb[1], &ttawb[2]));
	#endif

	AWB_SetWBGainToResult(sID, pstAwbResult, &WBInfo[sID]->stCurrentWB);
	AWB_SaveBootInfo(sID);
	AWB_SetInfo(sID, &WBInfo[sID]->stCurrentWB);
	//AWB_ShowDebugInfo(sID);
	AWB_SaveLog(sID);
	AWB_RunDetection(sID);

	if (!WBInfo[sID]->u32FirstStableTime)
		WBInfo[sID]->u32FirstStableTime = AWB_GetUSTimeDiff(&firstAwbTime[sID], &curAwbTime[sID]);

	#ifndef ARCH_RTOS_CV181X // TODO@CV181X
	gettimeofday(&proAwbTime[sID], NULL);
	#endif
	processT[sID] = AWB_GetMSTimeDiff(&curAwbTime[sID], &proAwbTime[sID]);

	if (AWB_GetDebugMode() == 1) {
		printf("Func : %s  rsv:%d\n", __func__, s32Rsv);
		printf("s32Handle:%u sId:%d Fc:%u\n", ViPipe, sID, WBInfo[sID]->u32FrameID);
		printf("AWB CTv:%d %d PTv:%d %d\n", (CVI_S32)curAwbTime[sID].tv_sec, (CVI_S32)curAwbTime[sID].tv_usec,
		       (CVI_S32)preAwbTime[sID].tv_sec, (CVI_S32)preAwbTime[sID].tv_usec);
		printf("PeriodT:%u ProT:%u\n", periodT[sID], processT[sID]);
		printf("WB Gain:%d %d\n", WBInfo[sID]->stCurrentWB.u16RGain, WBInfo[sID]->stCurrentWB.u16BGain);
		debugFrmCnt[sID]++;
		if (debugFrmCnt[sID] > 5) {
			debugFrmCnt[sID] = 0;
			AWB_SetDebugMode(0);
		}
	}

	return CVI_SUCCESS;
}


CVI_S32 awbCtrl(VI_PIPE ViPipe, CVI_U32 u32Cmd, CVI_VOID *pValue)
{
	switch (u32Cmd) {
	case ISP_WDR_MODE_SET:
		linearMode = *(CVI_U8 *)pValue;
		break;
	default:
		break;
	}

	UNUSED(ViPipe);

	return CVI_SUCCESS;
}


#define AWB_MAX_LOG_STRING_LEN    128
static CVI_U8	*pAWB_LogBuf[AWB_SENSOR_NUM];
static CVI_U32 AWB_LogTailIndex[AWB_SENSOR_NUM];

static void AWB_LogCreate(CVI_U8 sID)
{
	AWB_LogTailIndex[sID] = 0;
	AWB_Dump2File = 0;

	if (pAWB_LogBuf[sID] != CVI_NULL) {
		AWB_Free(sID, pAWB_LogBuf[sID]);
		pAWB_LogBuf[sID] = CVI_NULL;
	}
	pAWB_LogBuf[sID] = AWB_Malloc(sID, AWB_LOG_BUFF_SIZE);
	if (pAWB_LogBuf[sID] != CVI_NULL) {
		memset(pAWB_LogBuf[sID], 0, AWB_LOG_BUFF_SIZE);
	}
}
static void AWB_LogDestory(CVI_U8 sID)
{
	if (pAWB_LogBuf[sID] != CVI_NULL) {
		AWB_Free(sID, pAWB_LogBuf[sID]);
		pAWB_LogBuf[sID] = CVI_NULL;
	}
	AWB_LogTailIndex[sID] = 0;
}


CVI_S32 AWB_SetDumpLogPath(const char *szPath)
{
	if (strlen(szPath) >= MAX_AWB_LOG_PATH_LENGTH) {
		ISP_LOG_WARNING("Path length over %d\n", MAX_AWB_LOG_PATH_LENGTH);
		return CVI_FAILURE;
	}

	snprintf(awbDumpLogPath, MAX_AWB_LOG_PATH_LENGTH, "%s", szPath);
	return CVI_SUCCESS;
}

CVI_S32 AWB_SetDumpLogName(const char *szName)
{
	if (strlen(szName) >= MAX_AWB_LOG_PATH_LENGTH) {
		ISP_LOG_WARNING("log name over %d\n", MAX_AWB_LOG_PATH_LENGTH);
		return CVI_FAILURE;
	}

	snprintf(awbDumpLogName, MAX_AWB_LOG_PATH_LENGTH, "%s", szName);
	return CVI_SUCCESS;
}

static CVI_S32 AWB_DumpLog2Buff(CVI_U8 sID)
{
	CVI_U16 cnt_wait = 0;
	CVI_U16 i, maxInterval = 0;

	for (i = 0; i < AWB_SENSOR_NUM; i++) {
		if (stAwbAttrInfo[i] != NULL &&
			maxInterval < stAwbAttrInfo[i]->u8AWBRunInterval)
			maxInterval = stAwbAttrInfo[i]->u8AWBRunInterval;
	}
	AWB_LIMIT(maxInterval, 1, 0xFF);

	AWB_LogCreate(sID);
	AWB_Dump2File = sID + 1;
	while (cnt_wait < 5*maxInterval) {
		cnt_wait++;
		AWB_Delay1ms(33);//TODO@CLIFF
		if (AWB_Dump2File == 0)
			break;
	}

	if (AWB_Dump2File) {
		AWB_Dump2File = 0;
		AWB_LogDestory(sID);
		return CVI_FAILURE;
	}
	return CVI_SUCCESS;
}

#ifndef ARCH_RTOS_CV181X // TODO@CV181X
CVI_S32 AWB_DumpLog(CVI_U8 sID)
{
	FILE *fp;
	char fileName[MAX_LOG_FILENAME_LENGTH];
	static CVI_U16 fileCnt;

	AWB_DumpLog2Buff(sID);

	if (AWB_LogTailIndex[sID] && pAWB_LogBuf[sID] != CVI_NULL) {
		AWB_LogPrintf(sID, "End\n");
		if (strlen(awbDumpLogName)) {
			snprintf(fileName, MAX_LOG_FILENAME_LENGTH, "%s/%s_AwbLog.txt", awbDumpLogPath, awbDumpLogName);
		} else {
			snprintf(fileName, MAX_LOG_FILENAME_LENGTH, "%s/AwbLog%d.txt", awbDumpLogPath, fileCnt++);
		}
		fp = fopen(fileName, "w");
		if (fp == CVI_NULL) {
			printf("file:%s open fail!\n", fileName);
			AWB_LogDestory(sID);
			return CVI_FAILURE;
		}
		fwrite(pAWB_LogBuf[sID], 1, AWB_LogTailIndex[sID], fp);
		fclose(fp);
		printf("AWB logfile:%s save finish %d!\n", fileName, AWB_LogTailIndex[sID]);
		AWB_LogDestory(sID);
	}
	return CVI_SUCCESS;
}
#else
CVI_S32 AWB_DumpLog(CVI_U8 sID)
{
	UNUSED(sID);
	return CVI_SUCCESS;
}
#endif

void AWB_GetSnapLogBuf(CVI_U8 sID, CVI_U8 buf[], CVI_U32 bufSize)
{//only for cvi_helper.cpp
	CVI_U32 logSize;

	UNUSED(sID);

	AWB_DumpLog2Buff(sID);
	logSize = (bufSize < AWB_SNAP_LOG_BUFF_SIZE) ? bufSize : AWB_SNAP_LOG_BUFF_SIZE - 1;

	if (AWB_LogTailIndex[sID] && pAWB_LogBuf[sID] != CVI_NULL) {
		AWB_LogPrintf(sID, "End\n");
		memcpy(buf, pAWB_LogBuf[sID], logSize);
	}
	AWB_LogDestory(sID);
}

void AWB_GetDbgBinBuf(CVI_U8 sID, CVI_U8 buf[], CVI_U32 bufSize)
{
	CVI_U32 logSize;

	logSize = sizeof(s_AWB_DBG_S);
	AWB_SaveDbgBin(sID);
	if (logSize > bufSize) {
		ISP_LOG_ERR("Size is error, enlarge bufsize from %d to %d\n", bufSize, logSize);
		return;
	}
	if (pAwbDbg[sID] != NULL) {
		memcpy(buf, pAwbDbg[sID], logSize);
	}
}

CVI_U32 AWB_GetDbgBinSize(void)
{
	return sizeof(s_AWB_DBG_S);
}
#endif

static void AWB_SaveLog(CVI_U8 sID)
{
	CVI_U16 i;
#ifndef WBIN_SIM
	CVI_U16 j, k;
#endif
	SENSOR_ID sensorId;

	if (AWB_Dump2File != (sID + 1))
		return;
	sID = AWB_CheckSensorID(sID);

	isp_sensor_getId(sID, &sensorId);
	AWB_LogPrintf(sID, "fid:%d fnum:%d\n",  WBInfo[sID]->u32FrameID, WBInfo[sID]->u8AwbMaxFrameNum);
#ifndef WBIN_SIM
	for (k = 0; k < WBInfo[sID]->u8AwbMaxFrameNum; ++k) {
		for (i = 0; i < AWB_CHANNEL_NUM; ++i) {
			for (j = 0; j < WBInfo[sID]->u16WBRowSize * WBInfo[sID]->u16WBColumnSize; ++j) {
				AWB_LogPrintf(sID, "%d\t", WBInfo[sID]->pu16AwbStatistics[k][i][j]);
				if ((j + 1) % WBInfo[sID]->u16WBColumnSize == 0)
					AWB_LogPrintf(sID, "\n");
			}
			AWB_LogPrintf(sID, "\n\n");
		}
	}
#endif
	AWB_LogPrintf(sID, "Ver:%d.%d %d@%d\n", AWB_LIB_VER, AWB_LIB_SUBVER, AWB_DBG_VER, AWB_RELEASE_DATE);
	AWB_LogPrintf(sID, "AWB %d %d\n", WBInfo[sID]->stBalanceWB.u16RGain, WBInfo[sID]->stBalanceWB.u16BGain);
	AWB_LogPrintf(sID, "FWB %d %d\n", WBInfo[sID]->stFinalWB.u16RGain, WBInfo[sID]->stFinalWB.u16BGain);
	AWB_LogPrintf(sID, "CWB %d %d\n", WBInfo[sID]->stCurrentWB.u16RGain, WBInfo[sID]->stCurrentWB.u16BGain);
	AWB_LogPrintf(sID, "WDR_SEC:%d GC:%d GR:%d EC:%d TC:%d K:%d %d\n", stSampleInfo[sID]->u16WDRSECnt,
		stSampleInfo[sID]->u16GrayCnt, stSampleInfo[sID]->u8GrayRatio,
		stSampleInfo[sID]->u16EffectiveSampleCnt, stSampleInfo[sID]->u16TotalSampleCnt,
		WBInfo[sID]->u16ColorTemp, WBInfo[sID]->u16SEColorTemp);
	AWB_LogPrintf(sID, "LV:%d I:%d\n\n", WBInfo[sID]->stEnvInfo.s16LvX100, WBInfo[sID]->stEnvInfo.u32ISONum);
	AWB_LogPrintf(sID, "CAL\n");
	for (i = 0; i < AWB_CALIB_PTS_NUM; ++i) {
		AWB_LogPrintf(sID, "%d %d %d\n",
		WBInfo[sID]->stCalibWBGain[i].u16RGain,
		WBInfo[sID]->stCalibWBGain[i].u16BGain,
		WBInfo[sID]->u16CalibColorTemp[i]);
	}
	AWB_LogPrintf(sID, "RHL:%d %d\n", stBoundaryInfo[sID].u16RLowBound, stBoundaryInfo[sID].u16RHighBound);
	AWB_LogPrintf(sID, "BHL:%d %d\n", stBoundaryInfo[sID].u16BLowBound, stBoundaryInfo[sID].u16BHighBound);
	AWB_LogPrintf(sID, "isGW:%d\n",	WBInfo[sID]->bUseGrayWorld);
	AWB_LogPrintf(sID, "Sft\n");
	AWB_LogPrintf(sID, "%d %d\n", stCurveInfo[sID].u16LowTempTopRange, stCurveInfo[sID].u16LowTempBotRange);
	AWB_LogPrintf(sID, "%d %d\n", stCurveInfo[sID].u16MidTempTopRange, stCurveInfo[sID].u16MidTempBotRange);
	AWB_LogPrintf(sID, "%d %d\n", stCurveInfo[sID].u16CurveWhiteTopRange, stCurveInfo[sID].u16CurveWhiteBotRange);
	AWB_LogPrintf(sID, "%d %d\n", stCurveInfo[sID].u16HighTempTopRange, stCurveInfo[sID].u16HighTempBotRange);
	AWB_LogPrintf(sID, "R1:%d %d %d\n", stCurveInfo[sID].u16Region1_R, stCurveInfo[sID].u16Region2_R,
		stCurveInfo[sID].u16Region3_R);
	AWB_LogPrintf(sID, "ByP:%d\n", stAwbAttrInfo[sID]->bByPass);
	AWB_LogPrintf(sID, "OP:%d\n", stAwbAttrInfo[sID]->enOpType);
	AWB_LogPrintf(sID, "Int:%d\n", stAwbAttrInfo[sID]->u8AWBRunInterval);
	AWB_LogPrintf(sID, "dbg:%d\n", stAwbAttrInfo[sID]->u8DebugMode);
	AWB_LogPrintf(sID, "MWB:%d %d %d %d\n", stAwbAttrInfo[sID]->stManual.u16Rgain,
		stAwbAttrInfo[sID]->stManual.u16Grgain, stAwbAttrInfo[sID]->stManual.u16Gbgain,
		stAwbAttrInfo[sID]->stManual.u16Bgain);

	AWB_LogPrintf(sID, "aEn:%d spd:%d\n", stAwbAttrInfo[sID]->stAuto.bEnable, stAwbAttrInfo[sID]->stAuto.u16Speed);
	AWB_LogPrintf(sID, "RBstr:%d %d\n", stAwbAttrInfo[sID]->stAuto.u8RGStrength,
		stAwbAttrInfo[sID]->stAuto.u8BGStrength);
	AWB_LogPrintf(sID, "HLtmp:%d %d\n", stAwbAttrInfo[sID]->stAuto.u16HighColorTemp,
		stAwbAttrInfo[sID]->stAuto.u16LowColorTemp);
	AWB_LogPrintf(sID, "ShLim:%d %d\n", stAwbAttrInfo[sID]->stAuto.bShiftLimitEn,
		stAwbAttrInfo[sID]->stAuto.bNaturalCastEn);
	AWB_LogPrintf(sID, "Alg:%d\n", stAwbAttrInfo[sID]->enAlgType);
	AWB_LogPrintf(sID, "ShLimA:%d,%d,%d,%d,%d,%d,%d,%d\n", stAwbAttrInfo[sID]->stAuto.u16ShiftLimit[0],
		stAwbAttrInfo[sID]->stAuto.u16ShiftLimit[1], stAwbAttrInfo[sID]->stAuto.u16ShiftLimit[2],
		stAwbAttrInfo[sID]->stAuto.u16ShiftLimit[3], stAwbAttrInfo[sID]->stAuto.u16ShiftLimit[4],
		stAwbAttrInfo[sID]->stAuto.u16ShiftLimit[5], stAwbAttrInfo[sID]->stAuto.u16ShiftLimit[6],
		stAwbAttrInfo[sID]->stAuto.u16ShiftLimit[7]);
	for (i = 0; i < AWB_CT_BIN_NUM; i++) {
		AWB_LogPrintf(sID, "%d\t%d\t%d\n", stAwbAttrInfoEx[sID]->au16MultiCTBin[i],
			AWB_GetGainByColorTemperature(sID, stAwbAttrInfoEx[sID]->au16MultiCTBin[i]),
			stAwbAttrInfoEx[sID]->au16MultiCTWt[i]);
	}
	AWB_LogPrintf(sID, "sID:%d sns:%d\n", sID, sensorId);
	AWB_LogPrintf(sID, "Sat:%d %d %d %d\n", WBInfo[sID]->u8Saturation[0], WBInfo[sID]->u8Saturation[1],
		WBInfo[sID]->u8Saturation[2], WBInfo[sID]->u8Saturation[3]);
	AWB_LogPrintf(sID, "NC:%d\n", isNewCurve[sID]);
	AWB_Dump2File = 0;
}

#ifndef ARCH_RTOS_CV181X // TODO@CV181X
static void AWB_LogPrintf(CVI_U8 sID, const char *szFmt, ...)
{
#ifdef WBIN_SIM
	va_list vaPtr;
	CVI_S8 szBuffer[128];

	va_start(vaPtr, (const char *)szFmt);

	vsnprintf((char *)szBuffer, 128, (const char *)szFmt, vaPtr);
	va_end(vaPtr);
	printf("%s", szBuffer);
#else
	va_list vaPtr;
	CVI_S8 szBuffer[AWB_MAX_LOG_STRING_LEN];
	CVI_U32 bufLen;

	va_start(vaPtr, (const char *)szFmt);

	vsnprintf((char *)szBuffer, AWB_MAX_LOG_STRING_LEN, (const char *)szFmt, vaPtr);
	va_end(vaPtr);
	bufLen = strlen((char *)szBuffer);

	if (AWB_LogTailIndex[sID] + bufLen < AWB_LOG_BUFF_SIZE) {
		memcpy(pAWB_LogBuf[sID] + AWB_LogTailIndex[sID], szBuffer, bufLen);
		AWB_LogTailIndex[sID] += bufLen;
	}
#endif
}
#else
static void AWB_LogPrintf(CVI_U8 sID, const char *szFmt, ...)
{
	UNUSED(sID);
	UNUSED(szFmt);
}
#endif

static CVI_S8 AWB_DbgBinInit(CVI_U8 sID)
{
	CVI_U8 i;
	VI_PIPE ViPipe;
	ISP_STATISTICS_CFG_S stStatCfg;

	CVI_U16 imgW, imgH, numW, numH;

	if (pAwbDbg[sID] == NULL) {
		pAwbDbg[sID] = (s_AWB_DBG_S *)AWB_Malloc(sID, sizeof(s_AWB_DBG_S));
		if (pAwbDbg[sID] == NULL) {
			//ISP_LOG_ERR("pAwbDbg is NULL\n");
			return CVI_FAILURE;
		}
	}
	ViPipe = sID;

	CVI_ISP_GetStatisticsConfig(ViPipe, &stStatCfg);

	memset(pAwbDbg[sID], 0x00, sizeof(s_AWB_DBG_S));
	pAwbDbg[sID]->u32Date = AWB_RELEASE_DATE;
	pAwbDbg[sID]->u16AlgoVer = (AWB_LIB_VER<<8)+AWB_LIB_SUBVER;
	pAwbDbg[sID]->u16DbgVer = AWB_DBG_VER;
	pAwbDbg[sID]->u16MaxFrameNum = WBInfo[sID]->u8AwbMaxFrameNum;
	pAwbDbg[sID]->u32BinSize = sizeof(s_AWB_DBG_S);

	imgW = stStatCfg.stWBCfg.stCrop.u16W;
	imgH = stStatCfg.stWBCfg.stCrop.u16H;
	numW = stStatCfg.stWBCfg.u16ZoneCol;
	numH = stStatCfg.stWBCfg.u16ZoneRow;
	pAwbDbg[sID]->u16WinWnum = numW;
	pAwbDbg[sID]->u16WinHnum = numH;
	pAwbDbg[sID]->u16WinOffX = stStatCfg.stWBCfg.stCrop.u16X;
	pAwbDbg[sID]->u16WinOffY = stStatCfg.stWBCfg.stCrop.u16Y;
	if (numW && numH) {
		pAwbDbg[sID]->u16WinWsize = 2*numW*((imgW/numW)/2);
		pAwbDbg[sID]->u16WinHsize = 2*numH*((imgH/numH)/2);
	}

	for (i = 0; i < AWB_CALIB_PTS_NUM; ++i) {
		pAwbDbg[sID]->CalibRgain[i] = WBInfo[sID]->stCalibWBGain[i].u16RGain;
		pAwbDbg[sID]->CalibBgain[i] = WBInfo[sID]->stCalibWBGain[i].u16BGain;
		pAwbDbg[sID]->CalibTemp[i] = WBInfo[sID]->u16CalibColorTemp[i];
	}
	ISP_LOG_DEBUG("Dbg Size:%d\n", pAwbDbg[sID]->u32BinSize);
	return CVI_SUCCESS;
}

static void AWB_SaveDbgBin(CVI_U8 sID)
{
	CVI_U16 cnt_wait = 0;
	CVI_U16 maxInterval = 0;
	SENSOR_ID sensorId;

	if (AWB_DbgBinInit(sID) == CVI_FAILURE) {
		return;
	}

	isp_sensor_getId(sID, &sensorId);

	u8AwbDbgBinFlag[sID] = AWB_DBG_BIN_FLOW_TRIG;
	maxInterval = stAwbAttrInfo[sID]->u8AWBRunInterval;

	AWB_LIMIT(maxInterval, 1, 0xFF);
	while (cnt_wait < 5*maxInterval) {
		cnt_wait++;
		AWB_Delay1ms(80);//TODO@CLIFF
		if (u8AwbDbgBinFlag[sID] > AWB_DBG_BIN_FLOW_UPDATE) {//update data
			break;
		}
	}
	if (u8AwbDbgBinFlag[sID] > AWB_DBG_BIN_FLOW_UPDATE) {//SUCCESS
		//dgbXXX[1] will not be used
		pAwbDbg[sID]->dbgInfoAttr[0] = *stAwbAttrInfo[sID];
		//pAwbDbg[sID]->dbgInfoAttr[1] = *stAwbAttrInfo[1];
		pAwbDbg[sID]->dbgMPIAttr[0] = *pstAwbMpiAttr[sID];
		//pAwbDbg[sID]->dbgMPIAttr[1] = *pstAwbMpiAttr[1];
		pAwbDbg[sID]->dbgInfoAttrEx[0] = *stAwbAttrInfoEx[sID];
		//pAwbDbg[sID]->dbgInfoAttrEx[1] = *stAwbAttrInfoEx[1];
		pAwbDbg[sID]->dbgMPIAttrEx[0] = *pstAwbMpiAttrEx[sID];
		//pAwbDbg[sID]->dbgMPIAttrEx[1] = *pstAwbMpiAttrEx[1];
		pAwbDbg[sID]->calib_sts = u8CalibSts[sID];
		pAwbDbg[sID]->u16SensorId = sensorId;
		pAwbDbg[sID]->stFace = stFaceWB[sID];
	} else {
		//ISP_LOG_ERR("Err:No AWB Dbg Bin\n");
	}

	u8AwbDbgBinFlag[sID] = AWB_DBG_BIN_FLOW_INIT;
}

#ifndef ARCH_RTOS_CV181X // TODO@CV181X
void AWB_DumpDbgBin(CVI_U8 sID)
{
	FILE *fp;
	char fileName[MAX_LOG_FILENAME_LENGTH];

	static CVI_U16 fileCnt;

#ifdef WBIN_SIM
	sprintf(fileName, "pcTest%d.bin", fileCnt);
		pAwbDbg[sID]->dbgInfoAttr[0] = *stAwbAttrInfo[0];
		pAwbDbg[sID]->dbgInfoAttr[1] = *stAwbAttrInfo[1];
		pAwbDbg[sID]->dbgMPIAttr[0] = *pstAwbMpiAttr[0];
		pAwbDbg[sID]->dbgMPIAttr[1] = *pstAwbMpiAttr[1];
		pAwbDbg[sID]->dbgInfoAttrEx[0] = *stAwbAttrInfoEx[0];
		pAwbDbg[sID]->dbgInfoAttrEx[1] = *stAwbAttrInfoEx[1];
		pAwbDbg[sID]->dbgMPIAttrEx[0] = *pstAwbMpiAttrEx[0];
		pAwbDbg[sID]->dbgMPIAttrEx[1] = *pstAwbMpiAttrEx[1];
		pAwbDbg[sID]->calib_sts = u8CalibSts[sID];
		pAwbDbg[sID]->stFace = stFaceWB[sID];
		UNUSED(AWB_SaveDbgBin(sID));
#else
	if (strlen(awbDumpLogName)) {
		snprintf(fileName, MAX_LOG_FILENAME_LENGTH, "%s/%s.wbin", awbDumpLogPath, awbDumpLogName);
	} else {
		snprintf(fileName, MAX_LOG_FILENAME_LENGTH, "%s/AwbLog%d.wbin", awbDumpLogPath, fileCnt++);
	}
	AWB_SaveDbgBin(sID);
	if (pAwbDbg[sID] == NULL) {
		return;
	}
#endif
	fp = fopen(fileName, "wb");

	if (fp == CVI_NULL) {
		ISP_LOG_ERR("file:%s open fail!\n", fileName);
		return;
	}
	fwrite(pAwbDbg[sID], 1, pAwbDbg[sID]->u32BinSize, fp);
	fclose(fp);
	ISP_LOG_DEBUG("AWB DbgBin:%s save finish!\n", fileName);
}
#else
void AWB_DumpDbgBin(CVI_U8 sID)
{
	UNUSED(sID);
}
#endif


void AWB_GetAlgoVer(CVI_U16 *pVer, CVI_U16 *pSubVer)
{
	*pVer = AWB_LIB_VER;
	*pSubVer =  AWB_LIB_SUBVER;
}

#define ParaBuffer(Buffer, Row, Col) (*(Buffer + (Row) * (SizeSrc + 1) + (Col)))

static CVI_S32 ParalimitRow(CVI_FLOAT *Para, CVI_S32 SizeSrc, CVI_S32 Row)
{
	CVI_S32 i;
	CVI_FLOAT Max, Min, Temp;

	for (Max = AWB_ABS(ParaBuffer(Para, Row, 0)), Min = Max, i = SizeSrc; i; i--) {
		Temp = AWB_ABS(ParaBuffer(Para, Row, i));

		if (Max < Temp) {
			Max = Temp;
		}

		if (Min > Temp) {
			Min = Temp;
		}
	}

	Max = (Max + Min) * 0.000005;
	if (Max == 0)
		return 0;
	for (i = SizeSrc; i >= 0; i--) {
		ParaBuffer(Para, Row, i) /= Max;
	}

	return 0;
}

static CVI_S32 Paralimit(CVI_FLOAT *Para, CVI_S32 SizeSrc)
{
	CVI_S32 i;

	for (i = 0; i < SizeSrc; i++) {
		if (ParalimitRow(Para, SizeSrc, i)) {
			return -1;

		}

	}

	return 0;
}

static CVI_S32 ParaPreDealA(CVI_FLOAT *Para, CVI_S32 SizeSrc, CVI_S32 Size)
{
	CVI_S32 i, j;

	for (Size -= 1, i = 0; i < Size; i++) {
		for (j = 0; j < Size; j++) {
			ParaBuffer(Para, i, j) = ParaBuffer(Para, i, j) * ParaBuffer(Para, Size, Size) -
				ParaBuffer(Para, Size, j) * ParaBuffer(Para, i, Size);
		}

		ParaBuffer(Para, i, SizeSrc) = ParaBuffer(Para, i, SizeSrc) * ParaBuffer(Para, Size, Size) -
		ParaBuffer(Para, Size, SizeSrc) * ParaBuffer(Para, i, Size);
		ParaBuffer(Para, i, Size) = 0;
		ParalimitRow(Para, SizeSrc, i);
	}

	return 0;
}

static CVI_S32 ParaDealA(CVI_FLOAT *Para, CVI_S32 SizeSrc)
{
	CVI_S32 i;

	for (i = SizeSrc; i; i--) {
		if (ParaPreDealA(Para, SizeSrc, i)) {
			return -1;

		}

	}

	return 0;
}

static CVI_S32 ParaPreDealB(CVI_FLOAT *Para, CVI_S32 SizeSrc, CVI_S32 OffSet)
{
	CVI_S32 i, j;

	for (i = OffSet + 1; i < SizeSrc; i++) {
		for (j = OffSet + 1; j <= i; j++) {
			ParaBuffer(Para, i, j) *= ParaBuffer(Para, OffSet, OffSet);
		}

		ParaBuffer(Para, i, SizeSrc) = ParaBuffer(Para, i, SizeSrc) * ParaBuffer(Para, OffSet,
			 OffSet) - ParaBuffer(Para, i, OffSet) * ParaBuffer(Para, OffSet, SizeSrc);
		ParaBuffer(Para, i, OffSet) = 0;
		ParalimitRow(Para, SizeSrc, i);
	}

	return 0;
}

static CVI_S32 ParaDealB(CVI_FLOAT *Para, CVI_S32 SizeSrc)
{
	CVI_S32 i;

	for (i = 0; i < SizeSrc; i++) {
		if (ParaPreDealB(Para, SizeSrc, i)) {
			return -1;
		}

	}

	for (i = 0; i < SizeSrc; i++) {
		if (ParaBuffer(Para, i, i)) {
			ParaBuffer(Para, i, SizeSrc) /= ParaBuffer(Para, i, i);
			ParaBuffer(Para, i, i) = 1.0;
		}
	}

	return 0;
}

static CVI_S32 ParaDeal(CVI_FLOAT *Para, CVI_S32 SizeSrc)
{
	Paralimit(Para, SizeSrc);

	if (ParaDealA(Para, SizeSrc)) {
		return -1;

	}

	if (ParaDealB(Para, SizeSrc)) {
		return -1;

	}

	return 0;
}

static CVI_S32 GetParaBuffer(CVI_FLOAT *Para, const CVI_FLOAT *X, const CVI_FLOAT *Y, CVI_S32 Amount, CVI_S32 SizeSrc)
{
	CVI_S32 i, j;

	for (i = 0; i < SizeSrc; i++) {
		for (ParaBuffer(Para, 0, i) = 0, j = 0; j < Amount; j++) {
			ParaBuffer(Para, 0, i) += pow(*(X + j), 2 * (SizeSrc - 1) - i);
		}

	}

	for (i = 1; i < SizeSrc; i++) {
		for (ParaBuffer(Para, i, SizeSrc - 1) = 0, j = 0; j < Amount; j++) {
			ParaBuffer(Para, i, SizeSrc - 1) += pow(*(X + j), SizeSrc - 1 - i);
		}

	}

	for (i = 0; i < SizeSrc; i++) {
		for (ParaBuffer(Para, i, SizeSrc) = 0, j = 0; j < Amount; j++) {
			ParaBuffer(Para, i, SizeSrc) += (*(Y + j)) * pow(*(X + j), SizeSrc - 1 - i);
		}

	}

	for (i = 1; i < SizeSrc; i++) {
		for (j = 0; j < SizeSrc - 1; j++) {
			ParaBuffer(Para, i, j) = ParaBuffer(Para, i - 1, j + 1);
		}

	}

	return 0;
}

static CVI_S32 CalCurvePara(CVI_U8 sID,
	const CVI_FLOAT *BufferX, const CVI_FLOAT *BufferY, CVI_S32 Amount, CVI_S32 SizeSrc, CVI_FLOAT *ParaResK)
{
	CVI_FLOAT *ParaK = (CVI_FLOAT *) AWB_Malloc(sID, SizeSrc * (SizeSrc + 1) * sizeof(CVI_FLOAT));

	if (ParaK == NULL)
		return CVI_FAILURE;

	GetParaBuffer(ParaK, BufferX, BufferY, Amount, SizeSrc);
	ParaDeal(ParaK, SizeSrc);

	for (Amount = 0; Amount < SizeSrc; Amount++, ParaResK++) {
		*ParaResK = ParaBuffer(ParaK, Amount, SizeSrc);
	}

	AWB_Free(sID, ParaK);
	return CVI_SUCCESS;
}

static CVI_S32 AWB_BuildTempCurve(CVI_U8 sID, CVI_FLOAT *BufferX, CVI_FLOAT *BufferY, CVI_U8 Amount, CVI_FLOAT *ParaK)
{
	CVI_S32 ret;

	ret = CalCurvePara(sID, (const float *) BufferX, (const float *) BufferY, Amount, 3, (float *) ParaK);
	return ret;
}

void AWB_Delay1ms(CVI_U32 ms)
{
	CVI_U32 i;

	for (i = 0; i < ms; i++) {
#ifdef ARCH_RTOS_CV181X // TODO@CV181X
		vTaskDelay(pdMS_TO_TICKS(ms));
#else
		usleep(1000);
#endif
	}
}

static CVI_U32 AWB_GetUSTimeDiff(const struct timeval *before, const struct timeval *after)
{
	CVI_U32 CVI_timeDiff = 0;

	CVI_timeDiff = (after->tv_sec - before->tv_sec) * 1000000 + (after->tv_usec - before->tv_usec);

	return CVI_timeDiff;
}

static CVI_U32 AWB_GetMSTimeDiff(const struct timeval *before, const struct timeval *after)
{
	CVI_U32 CVI_timeDiff = 0;

	CVI_timeDiff = AWB_GetUSTimeDiff(before, after) / 1000;

	return CVI_timeDiff;
}

static void AWB_SetFaceArea(CVI_U8 sID)
{
	CVI_U8 i, maxIdx = 0;
	CVI_U32 faceSize, maxFaceSize = 0;
	CVI_U16 fdPosX, fdPosY, fdWidth, fdHeight, frameWidth, frameHeight;

	AwbFaceArea[sID].u8FDNum = stFaceWB[sID].u8Num;

	if (AwbFaceArea[sID].u8FDNum == 0) {
		return;
	}
	for (i = 0; i < stFaceWB[sID].u8Num; i++) {
		faceSize = stFaceWB[sID].u16Width[i] * stFaceWB[sID].u16Height[i];
		if (faceSize > maxFaceSize) {
			maxIdx = i;
			maxFaceSize = faceSize;
		}
	}
	fdPosX = stFaceWB[sID].u16PosX[maxIdx];
	fdPosY = stFaceWB[sID].u16PosY[maxIdx];
	fdWidth = stFaceWB[sID].u16Width[maxIdx];
	fdHeight = stFaceWB[sID].u16Height[maxIdx];
	frameWidth = stFaceWB[sID].u16FrameWidth;
	frameHeight = stFaceWB[sID].u16FrameHeight;
	if ((fdPosX + fdWidth > frameWidth) || (fdPosY + fdHeight > frameHeight) ||
		(!fdWidth) || (!fdHeight)) {
		AwbFaceArea[sID].u8FDNum = 0;
		return;
	}
	if (frameWidth == 0 || frameHeight == 0) {
		AwbFaceArea[sID].u8FDNum = 0;
		return;
	}
	AwbFaceArea[sID].u16StX = (fdPosX*WBInfo[sID]->u16WBColumnSize)/frameWidth;
	AwbFaceArea[sID].u16StY = (fdPosY*WBInfo[sID]->u16WBRowSize)/frameHeight;
	AwbFaceArea[sID].u16EndX = ((fdPosX+fdWidth)*WBInfo[sID]->u16WBColumnSize)/frameWidth;
	AwbFaceArea[sID].u16EndY = ((fdPosY+fdHeight)*WBInfo[sID]->u16WBRowSize)/frameHeight;
}

void AWB_GetGrayWorldWB(CVI_U8 sID, CVI_U16 *pRgain, CVI_U16 *pBgain)
{
#define WDR_LE 0

	CVI_U16 pixelR, pixelG, pixelB, G_Min, G_Max, idx;
	CVI_U16 RGain, BGain, x, y, u16EffectiveSampleCnt = 0;
	CVI_U64 u64TotalPixelSumR = 0, u64TotalPixelSumB = 0;

	sID = AWB_CheckSensorID(sID);
	if (stAwbAttrInfo[sID]->stAuto.bEnable) {
		*pRgain = WBInfo[sID]->stGrayWorldWB.u16RGain;
		*pBgain = WBInfo[sID]->stGrayWorldWB.u16BGain;
	} else {
		G_Min = 35;
		G_Max = 800;

		for (y = 0; y < WBInfo[sID]->u16WBRowSize; ++y) {
			for (x = 0; x < WBInfo[sID]->u16WBColumnSize; ++x) {
				idx = y * WBInfo[sID]->u16WBColumnSize + x;

				pixelR = WBInfo[sID]->pu16AwbStatistics[WDR_LE][AWB_R][idx];
				pixelG = WBInfo[sID]->pu16AwbStatistics[WDR_LE][AWB_G][idx];
				pixelB = WBInfo[sID]->pu16AwbStatistics[WDR_LE][AWB_B][idx];

				if (pixelR > 0 && pixelB > 0 && (pixelG > G_Min && pixelG < G_Max)) {
					RGain = pixelG * AWB_GAIN_BASE / pixelR;
					BGain = pixelG * AWB_GAIN_BASE / pixelB;
					u64TotalPixelSumR += RGain;
					u64TotalPixelSumB += BGain;
					u16EffectiveSampleCnt++;
				}
			}
		}
		*pRgain = u64TotalPixelSumR / u16EffectiveSampleCnt;
		*pBgain = u64TotalPixelSumB / u16EffectiveSampleCnt;
	}
}

void AWB_SetAwbSimMode(CVI_BOOL bMode)
{
	AwbSimMode = bMode;
}

CVI_BOOL AWB_GetAwbSimMode(void)
{
	return AwbSimMode;
}

static void AWB_RunDetection(CVI_U8 sID)
{
	#define DETECTION_MODE	110
	static time_t s_lLastTime;
	time_t lCurTime = time(NULL);

	if ((AWB_GetDebugMode() == DETECTION_MODE) && !WBInfo[sID]->bStable && (lCurTime - s_lLastTime > 5 * 60)) {
		AWB_SetAwbRunStatus(sID, true);
		s_lLastTime = lCurTime;
	}
}

void AWB_SetAwbRunStatus(CVI_U8 sID, CVI_BOOL bState)
{
	bAwbRunAbnormal[sID] = bState;
}

CVI_BOOL AWB_GetAwbRunStatus(CVI_U8 sID)
{
	return bAwbRunAbnormal[sID];
}

#ifdef WBIN_SIM
s_AWB_DBG_S *pc_AWB_DbgBinInit(CVI_U8 sID)
{
	AWB_DbgBinInit(sID);

	return pAwbDbg[sID];
}
void pc_AWB_RunAlgo(CVI_U8 sID)
{
	AWB_RunAlgo(sID);
	AWB_SetInfo(sID, &(WBInfo[sID]->stCurrentWB));
}

void pc_AWB_SaveLog(CVI_U8 sID)
{
	AWB_SaveLog(sID);
}

void pc_SetAWB_Dump2File(CVI_BOOL val)
{
	AWB_Dump2File = val;
}
sWBCurveInfo *pc_Get_stCurveInfo(CVI_U8 sID)
{
	return &stCurveInfo[sID];
}
void pc_Set_u8AwbDbgBinFlag(CVI_U8 val)
{
	u8AwbDbgBinFlag[0] = val;
}

sWBInfo *pc_Get_sWBInfo(CVI_U8 sID)
{
	return WBInfo[sID];
}
sWBCurveInfo *pc_Get_sWBCurveInfo(CVI_U8 sID)
{
	return &stCurveInfo[sID];
}
ISP_AWB_Calibration_Gain_S *pc_Get_stWbCalibration(CVI_U8 sID)
{
	return pstWbCalibration[sID];
}
ISP_WB_ATTR_S *pc_Get_stAwbMpiAttr(CVI_U8 sID)
{
	return pstAwbMpiAttr[sID];
}
ISP_WB_ATTR_S *pc_Get_stAwbAttrInfo(CVI_U8 sID)
{
	return stAwbAttrInfo[sID];
}

ISP_AWB_ATTR_EX_S *pc_Get_stAwbMpiAttrEx(CVI_U8 sID)
{
	return pstAwbMpiAttrEx[sID];
}
ISP_AWB_ATTR_EX_S *pc_Get_stAwbAttrInfoEx(CVI_U8 sID)
{
	return stAwbAttrInfoEx[sID];
}
SEXTRA_LIGHT_RB *pc_Get_stEXTRA_LIGHT_RB(void)
{
	return &sExLight;
}

#endif

