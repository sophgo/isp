
#include <stdlib.h>
#include <stdio.h>
//#include <conio.h>
#include <stdarg.h>
#include <string.h>

#include <windows.h>
#include <sys/time.h>//for gettimeofday()
#include <unistd.h>
#include <sys/stat.h>

#include "cvi_comm_video.h"
#include "cvi_isp.h"

#include "cvi_comm_isp.h"
#include "cvi_comm_3a.h"
#include "cvi_common.h"
#include "isp_main.h"
#include "isp_debug.h"
#include "cvi_awb.h"
#ifdef AAA_PC_PLATFORM
#include "awb_platform.h"
#endif
#include "awbalgo.h"

#include "awb_platform.h"
#include "misc.h"
#include "bmp.h"
#include "simple_jpg.h"

#include "ext_misc.h"

#define ABS(x) ((x) >= 0 ? (x) : (-(x)))
#define DIV_0_TO_1(a)   ((0 == (a)) ? 1 : (a))

//for dummy ==>
CVI_S32 *log_levels;
char *log_name[8];
int gExportToStdout;
int gDebugLevel = LOG_INFO;
//for dummy <==

SENSOR_ID sensorId;
char exe_dir[1024];//awbalgo.exe directory
s_AWB_DBG_S *pInputDbg;
ISP_AWB_Calibration_Gain_S calibWB;

sWBInfo *pWBInfo[AWB_SENSOR_NUM];
sWBCurveInfo *pstCurveInfo[AWB_SENSOR_NUM];

ISP_AWB_Calibration_Gain_S *pstWbCalibration[AWB_SENSOR_NUM];
ISP_WB_ATTR_S *pstAwbMpiAttr[AWB_SENSOR_NUM], *pstAwbAttrInfo[AWB_SENSOR_NUM];
ISP_AWB_ATTR_EX_S *pstAwbMpiAttrEx[AWB_SENSOR_NUM], *pstAwbAttrInfoEx[AWB_SENSOR_NUM];
s_AWB_DBG_S *pPcAwbDbg;
SEXTRA_LIGHT_RB *psExLight;

void getAWB_dataPtr(void)
{
	pWBInfo[0] = pc_Get_sWBInfo(0);
	pWBInfo[1] = pc_Get_sWBInfo(1);
	pstCurveInfo[0] = pc_Get_stCurveInfo(0);
	pstCurveInfo[1] = pc_Get_stCurveInfo(1);
	pstWbCalibration[0] = pc_Get_stWbCalibration(0);
	pstWbCalibration[1] = pc_Get_stWbCalibration(1);
	pstAwbMpiAttr[0] = pc_Get_stAwbMpiAttr(0);
	pstAwbMpiAttr[1] = pc_Get_stAwbMpiAttr(1);
	pstAwbAttrInfo[0] = pc_Get_stAwbAttrInfo(0);
	pstAwbAttrInfo[1] = pc_Get_stAwbAttrInfo(1);
	pstAwbMpiAttrEx[0] = pc_Get_stAwbMpiAttrEx(0);
	pstAwbMpiAttrEx[1] = pc_Get_stAwbMpiAttrEx(1);
	pstAwbAttrInfoEx[0] = pc_Get_stAwbAttrInfoEx(0);
	pstAwbAttrInfoEx[1] = pc_Get_stAwbAttrInfoEx(1);
	psExLight = pc_Get_stEXTRA_LIGHT_RB();
}

CVI_S32 CVI_ISP_GetStatisticsConfig(VI_PIPE ViPipe, ISP_STATISTICS_CFG_S *pstStatCfg)
{
	pstStatCfg->stWBCfg.u16ZoneCol = pInputDbg->u16WinWnum;
	pstStatCfg->stWBCfg.u16ZoneRow = pInputDbg->u16WinHnum;
	pstStatCfg->stWBCfg.stCrop.u16X = pInputDbg->u16WinOffX;
	pstStatCfg->stWBCfg.stCrop.u16Y = pInputDbg->u16WinOffY;
	pstStatCfg->stWBCfg.stCrop.u16W = pInputDbg->u16WinWsize;
	pstStatCfg->stWBCfg.stCrop.u16H = pInputDbg->u16WinHsize;

	return CVI_SUCCESS;
}
CVI_S32 CVI_ISP_SetStatisticsConfig(VI_PIPE ViPipe, const ISP_STATISTICS_CFG_S *pstStatCfg)
{
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_SetWBCalibration(VI_PIPE ViPipe, const ISP_AWB_Calibration_Gain_S *pstWBCalib)
{
	ISP_WB_ATTR_S tmpAwbAttr;
	CVI_U8 sID = AWB_ViPipe2sID(ViPipe);

	AWB_GetAttr(sID, &tmpAwbAttr);
	AWB_SetCalibration(sID, pstWBCalib);
	AWB_BuildCurve(sID, pstWBCalib, &tmpAwbAttr);
	AWB_SetAttr(sID, &tmpAwbAttr);

	return CVI_SUCCESS;
}

void AWB_GetPCStatisticData(CVI_U8 sID, CVI_BOOL show)
{
	CVI_U16 i, j, k;
	CVI_U32 inx;
	CVI_U32 frameNum;
	CVI_U16 row, col;

	frameNum = pInputDbg->u16MaxFrameNum;
	col = pInputDbg->u16WinWnum;
	row = pInputDbg->u16WinHnum;
	printf("FrmCnt :%d W:%d H:%d\n", frameNum, col, row);

	pWBInfo[sID]->u8AwbMaxFrameNum = frameNum;
	for (k = 0; k < frameNum; k++) {
		pWBInfo[sID]->pu16AwbStatistics[k][AWB_R] = (CVI_U16 *)malloc(col * row * sizeof(CVI_U16));
		pWBInfo[sID]->pu16AwbStatistics[k][AWB_G] = (CVI_U16 *)malloc(col * row * sizeof(CVI_U16));
		pWBInfo[sID]->pu16AwbStatistics[k][AWB_B] = (CVI_U16 *)malloc(col * row * sizeof(CVI_U16));
		for (i = 0; i < row; ++i) {
			for (j = 0; j < col; ++j) {
				inx = i * col + j;
				pWBInfo[sID]->pu16AwbStatistics[k][AWB_R][inx] = pInputDbg->u16P_R[k][inx];
				pWBInfo[sID]->pu16AwbStatistics[k][AWB_G][inx] = pInputDbg->u16P_G[k][inx];
				pWBInfo[sID]->pu16AwbStatistics[k][AWB_B][inx] = pInputDbg->u16P_B[k][inx];
			}
		}
	}
	for (i = 0; i < AWB_CALIB_PTS_NUM; ++i) {
		calibWB.u16AvgRgain[i] = pInputDbg->CalibRgain[i];
		calibWB.u16AvgBgain[i] = pInputDbg->CalibBgain[i];
		calibWB.u16ColorTemperature[i] = pInputDbg->CalibTemp[i];
	}

	pWBInfo[sID]->stEnvInfo.u32ISONum = pInputDbg->u32ISONum;
	pWBInfo[sID]->stEnvInfo.s16LvX100 = pInputDbg->s16LvX100;

	if (show) {
		for (k = 0; k < frameNum; k++) {
			for (i = 0; i < row; ++i) {
				for (j = 0; j < col; ++j) {
					inx = i * col + j;
					printf("%d\t", pWBInfo[sID]->pu16AwbStatistics[k][AWB_R][inx]);
				}
				printf("\n");
			}
			printf("\n\n");
			for (i = 0; i < row; ++i) {
				for (j = 0; j < col; ++j) {
					inx = i * col + j;
					printf("%d\t", pWBInfo[sID]->pu16AwbStatistics[k][AWB_G][inx]);
				}
				printf("\n");
			}
			printf("\n\n");
			for (i = 0; i < row; ++i) {
				for (j = 0; j < col; ++j) {
					inx = i * col + j;
					printf("%d\t", pWBInfo[sID]->pu16AwbStatistics[k][AWB_B][inx]);
				}
				printf("\n");
			}
			printf("\n\n");
		}
	}
}

void ShowDbgBinInfo(s_AWB_DBG_S *ptmp)
{
	int i;
	int tmon, tday, thour, tmin;

	tmon = ptmp->u32Date / 10000;
	tday = tmon % 100;
	tmon = tmon / 100;
	thour = ptmp->u32Date % 10000;
	tmin = thour % 100;
	thour = thour / 100;

	printf("======== Bin info Start========\n");
	printf("AlgoVer :%d.%d DbgVer:%d\n", ptmp->u16AlgoVer / 256, ptmp->u16AlgoVer % 256, ptmp->u16DbgVer);
	printf("ReleaseDate:%d/%d %d:%d\n", tmon, tday, thour, tmin);
	printf("FrmCnt :%d W:%d H:%d\n", ptmp->u16MaxFrameNum, ptmp->u16WinWnum, ptmp->u16WinHnum);
	printf("LV:%d ISO:%d\n", ptmp->s16LvX100, ptmp->u32ISONum);
	for (i = 0; i < AWB_CALIB_PTS_NUM; i++) {
		printf("Calib :%d %d %d\n", ptmp->CalibRgain[i], ptmp->CalibBgain[i], ptmp->CalibTemp[i]);
	}
	printf("AWB :%d %d\n", ptmp->u16BalanceR, ptmp->u16BalanceB);
	printf("GrayCnt :%d\n", ptmp->u16GrayCnt);
	printf("calib_sts:%d\n", ptmp->calib_sts);
	printf("======== Bin info End  ========\n");
}

CVI_U32 AE_GetISONum(CVI_U8 sID)
{
	return pWBInfo[sID]->stEnvInfo.u32ISONum;
}

CVI_S16 AE_GetCurrentLvX100(CVI_U8 sID)
{
	return pWBInfo[sID]->stEnvInfo.s16LvX100;
}
void AWB_SetDebugMode(CVI_U8 mode)
{
	UNUSED(mode);
}
void AWB_ShowInfoList(void)
{
}

#ifdef AAA_PC_PLATFORM
CVI_S32 AWB_DumpLog(void)
{
}
#endif
CVI_U8 AWB_GetDebugMode(void)
{
}

CVI_S32 isp_sensor_getId(VI_PIPE ViPipe, SENSOR_ID *pSensorId)
{
	*pSensorId = sensorId;
	UNUSED(ViPipe);
	return 0;
}

int loadDbgBin(char *fname)
{
	FILE *dbgBin;

	pInputDbg = (s_AWB_DBG_S *)malloc(sizeof(s_AWB_DBG_S));
	dbgBin = fopen(fname, "rb");
	fread(pInputDbg, sizeof(s_AWB_DBG_S), 1, dbgBin);
	fclose(dbgBin);
	printf("BinSize:%d ,s_AWB_DBG_S :%ld\n", pInputDbg->u32BinSize, sizeof(s_AWB_DBG_S));
	if (pInputDbg->u32BinSize != sizeof(s_AWB_DBG_S)) {
		printf("fname size %d is not match s_AWB_DBG_S:%ld\n", pInputDbg->u32BinSize, sizeof(s_AWB_DBG_S));
		return -1;
	}
	sensorId = pInputDbg->u16SensorId;
	{
		int i;

		for (i = 0; i < 6; i++) {
			//pcprintf("Cur%d\t%d\n",i,pInputDbg->dbgMPIAttr[0].stAuto.as32CurvePara[i]);
		}
	}
#if 0
	dbgBin = fopen("Attr.bin", "wb");
	fwrite(pInputDbg->dbgMPIAttr, sizeof(ISP_WB_ATTR_S), 1, dbgBin);
	fclose(dbgBin);
	dbgBin = fopen("AttrEx.bin", "wb");
	fwrite(pInputDbg->dbgMPIAttrEx, sizeof(ISP_AWB_ATTR_EX_S), 1, dbgBin);
	fclose(dbgBin);

	dbgBin = fopen("AttrInfo.bin", "wb");
	fwrite(pInputDbg->dbgInfoAttr, sizeof(ISP_WB_ATTR_S), 1, dbgBin);
	fclose(dbgBin);
	dbgBin = fopen("AttrInfoEx.bin", "wb");
	fwrite(pInputDbg->dbgInfoAttrEx, sizeof(ISP_AWB_ATTR_EX_S), 1, dbgBin);
	fclose(dbgBin);

#endif

	return 0;
}

const CVI_CHAR *CVI_SYS_GetModName(MOD_ID_E id)
{
}
void SaveSimResult(char *binname, struct AWB_JUDGE_ST *stJudge)
{
	FILE *outTxt;
	char fname[1024 + 32];
	CVI_FLOAT fRgainerr = 0.0;
	CVI_FLOAT fBgainerr = 0.0;

	fRgainerr = ABS(pPcAwbDbg->u16BalanceR-stJudge->u16IdealR)/(float)DIV_0_TO_1(stJudge->u16IdealR);
	fBgainerr = ABS(pPcAwbDbg->u16BalanceB-stJudge->u16IdealB)/(float)DIV_0_TO_1(stJudge->u16IdealB);

	sprintf(fname, "%s/%s", exe_dir, binname);
	outTxt = fopen(fname, "wb");
	fprintf(outTxt, "Ideal    :%d %d\n", stJudge->u16IdealR, stJudge->u16IdealB);
	fprintf(outTxt, "Acutal   :%d %d\n", pPcAwbDbg->u16BalanceR, pPcAwbDbg->u16BalanceB);
	fprintf(outTxt, "Threshold:%f %f\n", stJudge->fIdealRThresh, stJudge->fIdealBThresh);
	if ((fRgainerr <= stJudge->fIdealRThresh) &&
		(fBgainerr <= stJudge->fIdealBThresh))
		fprintf(outTxt, "%s\n", "PASS");
	else
		fprintf(outTxt, "%s\n", "FAIL");
	fclose(outTxt);
}

#ifdef AAA_LINUX_PLATFORM
void MessageBox(int v1, char *str1, char *str2, int v2)
{
	char tmpstr[128];
	int slen, i;

	sprintf(tmpstr, "* %s : %s *\n", str2, str1);
	slen = strlen(tmpstr) - 1;
	for (i = 0; i < slen; i++) {
		ColorPrintf(RED_COLOR, "*");
	}
	printf("\n");
	ColorPrintf(RED_COLOR, "%s", tmpstr);
	for (i = 0; i < slen; i++) {
		ColorPrintf(RED_COLOR, "*");
	}
	printf("\n");

	getchar();
}

#endif
void special_warning(void)
{
	CVI_U8 sID = 0;

	if (pstAwbAttrInfo[sID]->stAuto.u8RGStrength != 128) {
		MessageBox(0, "u8RGStrength", "Warning", MB_OK);
	}
	if (pstAwbAttrInfo[sID]->stAuto.u8BGStrength != 128) {
		MessageBox(0, "u8BGStrength", "Warning", MB_OK);
	}
	if (pstAwbAttrInfo[sID]->enOpType != OP_TYPE_AUTO) {
		MessageBox(0, "enOpType", "Warning", MB_OK);
	}
	if (pstAwbAttrInfo[sID]->stAuto.u16ZoneSel == 0 || pstAwbAttrInfo[sID]->stAuto.u16ZoneSel == 255) {
		MessageBox(0, "u16ZoneSel", "Warning", MB_OK);
	}
}

int AWB_DLL_Main(struct AWB_JUDGE_ST *pstJudge, char *wbinName, char *jsonName, char *outputPath)
{
	const CVI_U8 sID = 0;
	int i, j, slen;
	char moduleName[1024];
	char pqName[1024];
	ISP_AWB_PARAM_S stAwbParam;
#ifdef AAA_PC_PLATFORM
	GetModuleFileNameA(NULL, moduleName, 1024);
	printf("%s\n", moduleName);
	strcpy(exe_dir, moduleName);
	slen = strlen(exe_dir);
	for (i = slen - 1; i > 0; i--) {
		if (exe_dir[i] == '\\') {
			exe_dir[i] = 0;
			break;
		}
	}
#else
	char tmpWbinName[1024];
	char *pWbinName = tmpWbinName;
	char subDir[1024];
	char *p = NULL;

	strcpy(tmpWbinName, wbinName);
	strsep(&pWbinName, "/");
	do {
		if (pWbinName == NULL)
			break;
		p = strsep(&pWbinName, "/");
	} while (p != NULL);
	pWbinName = strsep(&p, ".");
	printf("%s\n", pWbinName);
	memset(subDir, 0, 1024);
	printf("%s\n", pWbinName);
	if (outputPath != NULL)
		sprintf(subDir, "%s/%s", outputPath, pWbinName);
	else
		sprintf(subDir, "./output/%s", pWbinName);
	if (access(subDir, 0) != 0) {
		mkdir("./output", 0777);
		mkdir(subDir, 0777);
	}
	sprintf(exe_dir, "%s", subDir);
#endif
	printf("Module dir=%s=\n", exe_dir);

	getAWB_dataPtr();

	ColorPrintf(GREEN_COLOR, "loadDbgBin\n");
	loadDbgBin(wbinName);//pInputDbg ,original grom CAM

	ColorPrintf(GREEN_COLOR, "AWB_GetPCStatisticData\n");
	AWB_GetPCStatisticData(sID, 0);

	ColorPrintf(GREEN_COLOR, "awbInit\n");
	if (pWBInfo[sID]->u8AwbMaxFrameNum > 1) {//from AWB_GetPCStatisticData
		stAwbParam.u8WDRMode = 1;
	} else {
		stAwbParam.u8WDRMode = 0;
	}
	stAwbParam.u8AWBZoneCol = pInputDbg->u16WinWnum;
	stAwbParam.u8AWBZoneRow = pInputDbg->u16WinHnum;
	awbInit(sID, &stAwbParam);

	pWBInfo[sID]->u16WBColumnSize = pInputDbg->u16WinWnum;
	pWBInfo[sID]->u16WBRowSize = pInputDbg->u16WinHnum;

	pWBInfo[sID]->stBalanceWB.u16RGain = pInputDbg->u16BalanceR;
	pWBInfo[sID]->stBalanceWB.u16BGain = pInputDbg->u16BalanceB;
	//Apply PQ tool para by AwbLog0.bin ,current Cam setting
	memcpy(pstAwbMpiAttr[0], &pInputDbg->dbgMPIAttr[0], sizeof(ISP_WB_ATTR_S));
	memcpy(pstAwbMpiAttr[1], &pInputDbg->dbgMPIAttr[1], sizeof(ISP_WB_ATTR_S));
	memcpy(pstAwbMpiAttrEx[0], &pInputDbg->dbgMPIAttrEx[0], sizeof(ISP_AWB_ATTR_EX_S));
	memcpy(pstAwbMpiAttrEx[1], &pInputDbg->dbgMPIAttrEx[1], sizeof(ISP_AWB_ATTR_EX_S));
	memcpy(pstAwbAttrInfo[0], &pInputDbg->dbgInfoAttr[0], sizeof(ISP_WB_ATTR_S));
	memcpy(pstAwbAttrInfo[1], &pInputDbg->dbgInfoAttr[1], sizeof(ISP_WB_ATTR_S));
	memcpy(pstAwbAttrInfoEx[0], &pInputDbg->dbgInfoAttrEx[0], sizeof(ISP_AWB_ATTR_EX_S));
	memcpy(pstAwbAttrInfoEx[1], &pInputDbg->dbgInfoAttrEx[1], sizeof(ISP_AWB_ATTR_EX_S));

	pstAwbMpiAttr[0]->u8DebugMode = 0;
	pstAwbMpiAttr[1]->u8DebugMode = 0;

#ifdef AAA_PC_PLATFORM
	sprintf(pqName, "%s\\pq.json", exe_dir);
#else
	if (jsonName != NULL)
		sprintf(pqName, "%s", jsonName);
#endif
	getAttrFromJson(pqName, 1);
	//____________________________________________________________
	// Test new function here

#if 0
	pstAwbMpiAttrEx[sID]->stLightInfo[0].u16WhiteRgain = 1440;
	pstAwbMpiAttrEx[sID]->stLightInfo[0].u16WhiteBgain = 3000;
	pstAwbMpiAttrEx[sID]->stLightInfo[0].u16ExpQuant = 6;
	pstAwbMpiAttrEx[sID]->stLightInfo[0].u8LightStatus = AWB_EXTRA_LS_ADD;//AWB_EXTRA_LS_ADD,AWB_EXTRA_LS_REMOVE
	pstAwbMpiAttrEx[sID]->stLightInfo[0].u8Radius = 16;

	pstAwbMpiAttrEx[sID]->stLightInfo[1].u16WhiteRgain = 1300;
	pstAwbMpiAttrEx[sID]->stLightInfo[1].u16WhiteBgain = 3700;
	pstAwbMpiAttrEx[sID]->stLightInfo[1].u16ExpQuant = 6;
	pstAwbMpiAttrEx[sID]->stLightInfo[1].u8LightStatus = AWB_EXTRA_LS_ADD;//AWB_EXTRA_LS_ADD,AWB_EXTRA_LS_REMOVE
	pstAwbMpiAttrEx[sID]->stLightInfo[1].u8Radius = 8;
#endif
#if 0
	pstAwbMpiAttrEx[sID]->stLightInfo[3].u16WhiteRgain = 1100;
	pstAwbMpiAttrEx[sID]->stLightInfo[3].u16WhiteBgain = 4095;
	pstAwbMpiAttrEx[sID]->stLightInfo[3].u16ExpQuant = 6;
	pstAwbMpiAttrEx[sID]->stLightInfo[3].u8LightStatus = AWB_EXTRA_LS_ADD;//AWB_EXTRA_LS_ADD,AWB_EXTRA_LS_REMOVE
	pstAwbMpiAttrEx[sID]->stLightInfo[3].u8Radius = 10;

	//pstAwbMpiAttr[sID]->stAuto.bShiftLimitEn= 0;
	pstAwbMpiAttr[sID]->stAuto.u16LowColorTemp = 2500;
#endif

#if 0
	pstAwbMpiAttrEx[sID]->stInOrOut.bEnable = 1;
	pstAwbMpiAttrEx[sID]->stInOrOut.enOpType = OP_TYPE_AUTO;//OP_TYPE_MANUAL;
	pstAwbMpiAttrEx[sID]->stInOrOut.enOutdoorStatus = AWB_OUTDOOR_MODE;//outdoor
	pstAwbMpiAttrEx[sID]->stInOrOut.u32OutThresh = 14;//10000
	pstAwbMpiAttrEx[sID]->stInOrOut.u16LowStart = 5000;//5000
	pstAwbMpiAttrEx[sID]->stInOrOut.u16LowStop = 4500;//4500
	pstAwbMpiAttrEx[sID]->stInOrOut.u16HighStart = 6000;//6000
	pstAwbMpiAttrEx[sID]->stInOrOut.u16HighStop = 7200;//7200
	pstAwbMpiAttrEx[sID]->stInOrOut.bGreenEnhanceEn = 0;//green enhance
	pstAwbMpiAttrEx[sID]->stInOrOut.u8OutShiftLimit = 32;//green enhance
#endif

#if 0
	//low bot,low top ---- high bot,high Top
	CVI_U16 u16ShiftLimit[AWB_CURVE_BOUND_NUM] = { 240, 240, 240, 240, 240, 240, 240, 240 };

	for (i = 0; i < AWB_CURVE_BOUND_NUM; i++) {
		pstAwbMpiAttr[sID]->stAuto.u16ShiftLimit[i] = u16ShiftLimit[i];
	}
	pstAwbMpiAttrEx[sID]->stInOrOut.u16HighStart = 7800;
	pstAwbMpiAttrEx[sID]->stInOrOut.u16HighStop = 9000;
#endif

#if 0
	pstAwbMpiAttrEx[sID]->au16MultiCTWt[0] = 128;
	pstAwbMpiAttrEx[sID]->au16MultiCTWt[1] = 128;
	pstAwbMpiAttrEx[sID]->au16MultiCTWt[2] = 128;
#endif

#if 0
	int x, y;
	//printf("pstAwbMpiAttr[sID]->stAuto.bAWBZoneWtEn %d\n",pstAwbMpiAttr[sID]->stAuto.bAWBZoneWtEn);
	//pstAwbMpiAttr[sID]->stAuto.bAWBZoneWtEn = 1;
	for (y = 0; y < AWB_ZONE_WT_H; y++) {
		for (x = 0; x < AWB_ZONE_WT_W; x++) {
			//pstAwbMpiAttr[sID]->stAuto.au8ZoneWt[x+y*AWB_ZONE_WT_W] = AWB_ZONE_WT_DEF;
			//if (x<4 || x >27)
			//	pstAwbMpiAttr[sID]->stAuto.au8ZoneWt[x+y*AWB_ZONE_WT_W] = 0;
			printf("%d\t", pstAwbMpiAttr[sID]->stAuto.au8ZoneWt[x + y * AWB_ZONE_WT_W]);
		}
		printf("\n");
	}
#endif
	//pstAwbMpiAttr[sID]->stAuto.u16ZoneSel = 32;
	//pstAwbMpiAttr[sID]->stAuto.bNaturalCastEn = 1;
#if 0
	pstAwbMpiAttr[sID]->stAuto.stCbCrTrack.bEnable = 1;
	pstAwbMpiAttr[sID]->stAuto.stCbCrTrack.au16CrMax[0] = 1050;
#endif
//Apply PQ tool para <==
#if 0
	calibWB.u16AvgRgain[0] = 1606;
	calibWB.u16AvgBgain[0] = 3212;
	calibWB.u16ColorTemperature[0] = 2850;
	calibWB.u16AvgRgain[1] = 1879;
	calibWB.u16AvgBgain[1] = 2717;
	calibWB.u16ColorTemperature[1] = 3900;
	calibWB.u16AvgRgain[2] = 2741;
	calibWB.u16AvgBgain[2] = 1743;
	calibWB.u16ColorTemperature[2] = 6500;
#endif

#if 0
	pstAwbMpiAttrEx[sID]->stSky.u8Mode = AWB_SKY_REMOVE;
	pstAwbMpiAttrEx[sID]->stSky.u8ThrLv = 14;//12
	pstAwbMpiAttrEx[sID]->stSky.u16Rgain = 2700;//2700
	pstAwbMpiAttrEx[sID]->stSky.u16Bgain = 1200;//1200
	pstAwbMpiAttrEx[sID]->stSky.u16MapRgain = 1677;
	pstAwbMpiAttrEx[sID]->stSky.u16MapBgain = 1964;
	pstAwbMpiAttrEx[sID]->stSky.u8Radius = 8;
#endif

#if 0
	pstAwbMpiAttrEx[sID]->stCtLv.u8Mode = 1;
	pstAwbMpiAttrEx[sID]->stCtLv.s8ThrLv[0] = 1;
	pstAwbMpiAttrEx[sID]->stCtLv.s8ThrLv[1] = 5;
	pstAwbMpiAttrEx[sID]->stCtLv.s8ThrLv[2] = 9;
	pstAwbMpiAttrEx[sID]->stCtLv.s8ThrLv[3] = 13;
	for (i = 0; i < AWB_CT_LV_NUM; i++) {
		for (j = 0; j < AWB_CT_BIN_NUM; j++) {
			pstAwbMpiAttrEx[sID]->stCtLv.au16MultiCTWt[i][j] = 50 * j + 200 * i;
			printf("w:\t%d\n", pstAwbMpiAttrEx[sID]->stCtLv.au16MultiCTWt[i][j]);
		}
		printf("\n");
	}
	pWBInfo[sID]->stEnvInfo.s16LvX100 = 1150;
#endif
	CVI_ISP_SetWBCalibration(0, &calibWB);

	ColorPrintf(GREEN_COLOR, "AWB_DbgBinInit\n");
	pPcAwbDbg = pc_AWB_DbgBinInit(sID);//pAwbDbg for AWB_RunAlgo
	pc_Set_u8AwbDbgBinFlag(AWB_DBG_BIN_FLOW_UPDATE);

	ColorPrintf(GREEN_COLOR, "AWB_RunAlgo\n");
	memcpy(pstAwbAttrInfo[sID], pstAwbMpiAttr[sID], sizeof(ISP_WB_ATTR_S));
	memcpy(pstAwbAttrInfoEx[sID], pstAwbMpiAttrEx[sID], sizeof(ISP_AWB_ATTR_EX_S));
	special_warning();
	pc_AWB_RunAlgo(sID);

	ColorPrintf(GREEN_COLOR, "AWB_SaveLog, <=== PC AWB Algo\n");
	pc_SetAWB_Dump2File(1);
	pc_AWB_SaveLog(sID);

	ColorPrintf(GREEN_COLOR, "ShowDbgBinInfo ,%s <===Original\n", wbinName);
	ShowDbgBinInfo(pInputDbg);
	//AWB_ShowColorTemperature(sID);

	////for AwbAlgo result
	//ColorPrintf(GREEN_COLOR, "Save result Bin: PcTest0.bin\n");
	//AWB_DumpDbgBin(sID);//Generate new bin ,save pAwbDbg to PcTest0.bin for testing

	ColorPrintf(GREEN_COLOR, "ShowDbgBinInfo, <=== PC Algo\n");
	BMP_DrawDbginfo(pPcAwbDbg, (char *)"WB.JPG");
	//BMP_DrawDbginfo(pInputDbg, (char *)"WB_Ori.JPG");

	SaveSimResult((char *)"WB.txt", pstJudge);

	//SaveAwbInfo();
	if (pInputDbg) {
		free(pInputDbg);
	}
	if (pPcAwbDbg) {
		free(pPcAwbDbg);
	}

	printf("Finish\n");

	return 0;
}

CVI_U8 *BmpImg;
#define BMP_W (600)
#define BMP_H (600)
#define BMP_VW (1200)//R/G Max 1.2
#define BMP_VH (1200)//B/G Max 1.2
#define ONE_BASE (1000)//1.0 ==>1000

void BMP_ImgInit(void)
{
	int i;

	if (BmpImg != NULL) {
		free(BmpImg);
		BmpImg = NULL;
	}
	BmpImg = (CVI_U8 *)malloc(3 * BMP_W * BMP_H);//3MB for 1024x1024
	memset(BmpImg, 0xff, 3 * BMP_W * BMP_H);

	for (i = 1; i < 8; i++) {//draw 0.5 x GRID for reference
		//BMP_line(0,BMP_VH*i/8,BMP_VW-1,BMP_VH*i/8,64,64,64);
		//BMP_line(BMP_VW*i/8,0,BMP_VW*i/8,BMP_VH-1,64,64,64);
	}

	for (i = 1; i < 5; i++) {//draw 0.25 x GRID for reference
		BMP_dashline(0, ONE_BASE * i / 4, BMP_VW - 1, ONE_BASE * i / 4, 128, 128, 128);
		BMP_dashline(ONE_BASE * i / 4, 0, ONE_BASE * i / 4, BMP_VH - 1, 128, 128, 128);
	}
}

void BMP_ImgSave(char *fname)
{
	char bmpfname[1024 + 32];
#ifdef AAA_PC_PLATFORM
	sprintf(bmpfname, "%s\\%s", exe_dir, fname);
#else
	sprintf(bmpfname, "%s/%s", exe_dir, fname);
#endif
	//pcprintf("bmp:=%s=\n", bmpfname);

	BMP *pBMPtmp = bmpCreate(BMP_W, BMP_H);

	memcpy(pBMPtmp->data, BmpImg, BMP_W * BMP_H * 3);
	//bmpSave(pBMPtmp,"WB.BMP");
	SaveBmp2Jpg(pBMPtmp, bmpfname);
	pBMPtmp = bmpDestory(pBMPtmp);
	free(BmpImg);
	BmpImg = NULL;
}

void BMP_Setpixel(int x, int y, int col_r, int col_g, int col_b)
{
//input 4096x4096 domain BMP_VW x BMP_VH
// 0,0 is at left_bottom
#define PIXEL_SIZE (1)//depend on BMP_W
	unsigned char *p;
	int i, j;

	if (x > BMP_VW - 1) {
		return;
	}
	if (y > BMP_VH - 1) {
		return;
	}

	x = x * BMP_W / BMP_VW;
	y = y * BMP_H / BMP_VH;

	y = BMP_H - y;
	if (x >= (BMP_W - PIXEL_SIZE) || y >= (BMP_H - PIXEL_SIZE)) {
		return;
	}
	if (x < 0 || y < 0) {
		return;
	}

	for (i = 0; i < PIXEL_SIZE; i++) {
		for (j = 0; j < PIXEL_SIZE; j++) {
			p = BmpImg + ((y + i) * BMP_W + x + j) * 3;
			p[0] = col_b;
			p[1] = col_g;
			p[2] = col_r;
		}
	}
}

void BMP_line(int x0, int y0, int x1, int y1, int r, int g, int b)

{
	int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
	int dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
	int err = (dx > dy ? dx : -dy) / 2;

	while (x0 != x1 || y0 != y1) {
		int e2 = err;

		BMP_Setpixel(x0, y0, r, g, b);
		if (e2 > -dx) {
			err -= dy;
			x0 += sx;
		}
		if (e2 < dy) {
			err += dx;
			y0 += sy;
		}
	}
}

void BMP_dashline(int x0, int y0, int x1, int y1, int r, int g, int b)

{
	int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
	int dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
	int err = (dx > dy ? dx : -dy) / 2;
	int skip = 0;

	while (x0 != x1 || y0 != y1) {
		int e2 = err;

		skip++;
		if (skip % 8 == 0) {
			BMP_Setpixel(x0, y0, r, g, b);
		}
		if (e2 > -dx) {
			err -= dy;
			x0 += sx;
		}
		if (e2 < dy) {
			err += dx;
			y0 += sy;
		}
	}
}

void BMP_circle(unsigned int x0, unsigned int y0, unsigned int radius, int r, int g, int b)
{
	int f = 1 - radius;
	int ddF_x = 0;
	int ddF_y = -2 * radius;
	int x = 0;
	int y = radius;

	BMP_Setpixel(x0, y0 + radius, r, g, b);
	BMP_Setpixel(x0, y0 - radius, r, g, b);
	BMP_Setpixel(x0 + radius, y0, r, g, b);
	BMP_Setpixel(x0 - radius, y0, r, g, b);

	while (x < y) {
		if (f >= 0) {
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x + 1;
		BMP_Setpixel(x0 + x, y0 + y, r, g, b);
		BMP_Setpixel(x0 - x, y0 + y, r, g, b);
		BMP_Setpixel(x0 + x, y0 - y, r, g, b);
		BMP_Setpixel(x0 - x, y0 - y, r, g, b);
		BMP_Setpixel(x0 + y, y0 + x, r, g, b);
		BMP_Setpixel(x0 - y, y0 + x, r, g, b);
		BMP_Setpixel(x0 + y, y0 - x, r, g, b);
		BMP_Setpixel(x0 - y, y0 - x, r, g, b);
	}
}

void BMP_circle_fill(unsigned int x0, unsigned int y0, unsigned int radius, int r, int g, int b)
{
	int f = 1 - radius;
	int ddF_x = 0;
	int ddF_y = -2 * radius;
	int x = 0;
	int y = radius;

	BMP_Setpixel(x0, y0 + radius, r, g, b);
	BMP_Setpixel(x0, y0 - radius, r, g, b);
	BMP_Setpixel(x0 + radius, y0, r, g, b);
	BMP_Setpixel(x0 - radius, y0, r, g, b);

	while (x < y) {
		if (f >= 0) {
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x + 1;

		BMP_line(x0 + x, y0 + y, x0 - x, y0 + y, r, g, b);
		BMP_line(x0 + x, y0 - y, x0 - x, y0 - y, r, g, b);
		BMP_line(x0 + y, y0 + x, x0 - y, y0 + x, r, g, b);
		BMP_line(x0 + y, y0 - x, x0 - y, y0 - x, r, g, b);
	}
}

void BMP_DrawDbginfo(s_AWB_DBG_S *ptmp, char *fname)
{
	//R/G & B/G base
	int i, j, k;
	int winSize;
	CVI_U16 RG_ratio, BG_ratio;
	CVI_U16 x1, y1, x2, y2;
	CVI_U16 GoldenR, GoldenB;
	CVI_U16 CalibR, CalibB;

	//ColorPrintf(GREEN_COLOR, "%s\n", fname);

	ShowDbgBinInfo(ptmp);

	winSize = ptmp->u16WinWnum * ptmp->u16WinHnum;
	BMP_ImgInit();

	for (k = 0; k < ptmp->u16MaxFrameNum; k++) {
		for (i = 0; i < winSize; i++) {
			if (ptmp->u16P_type[k][i] && ptmp->u16P_G[k][i]) {
				RG_ratio = ptmp->u16P_R[k][i] * ONE_BASE / ptmp->u16P_G[k][i];
				BG_ratio = ptmp->u16P_B[k][i] * ONE_BASE / ptmp->u16P_G[k][i];

				if (ptmp->u16P_type[k][i] == AWB_DBG_PT_OUT_CURVE) {
					BMP_Setpixel(RG_ratio, BG_ratio, 0, 0, 0);
				} else if (ptmp->u16P_type[k][i] == AWB_DBG_PT_IN_CURVE) {
					//BMP_Setpixel(RG_ratio,BG_ratio,0,2,186);
					BMP_circle_fill(RG_ratio, BG_ratio, 3, 0, 2, 186);
				}
			}
		}
	}
	for (i = 1; i < 256; i++) {
		if (ptmp->u16CurveR[i] == 0) {
			break;
		}
		//printf("%d\t%d\t%d\t%d\n", ptmp->u16CurveR[i], ptmp->u16CurveB[i], ptmp->u16CurveB_Top[i],
		//	ptmp->u16CurveB_Bot[i]);
		//Rgan:G/R*AWB_GAIN_BASE, to R/G*ONE_BASE
		if (ptmp->u16CurveR[i - 1] && ptmp->u16CurveB[i - 1] && ptmp->u16CurveB_Top[i - 1]
			&& ptmp->u16CurveB_Bot[i - 1] && ptmp->u16CurveR[i] && ptmp->u16CurveB[i]
			&& ptmp->u16CurveB_Top[i] && ptmp->u16CurveB_Bot[i]) {
			BMP_line((ONE_BASE * AWB_GAIN_BASE) / ptmp->u16CurveR[i - 1],
				(ONE_BASE * AWB_GAIN_BASE) / ptmp->u16CurveB[i - 1],
				(ONE_BASE * AWB_GAIN_BASE) / ptmp->u16CurveR[i],
				(ONE_BASE * AWB_GAIN_BASE) / ptmp->u16CurveB[i], 192, 0, 0);
			BMP_line((ONE_BASE * AWB_GAIN_BASE) / ptmp->u16CurveR[i - 1],
				(ONE_BASE * AWB_GAIN_BASE) / ptmp->u16CurveB_Top[i - 1],
				(ONE_BASE * AWB_GAIN_BASE) / ptmp->u16CurveR[i],
				(ONE_BASE * AWB_GAIN_BASE) / ptmp->u16CurveB_Top[i], 3, 196, 0);
			BMP_line((ONE_BASE * AWB_GAIN_BASE) / ptmp->u16CurveR[i - 1],
				(ONE_BASE * AWB_GAIN_BASE) / ptmp->u16CurveB_Bot[i - 1],
				(ONE_BASE * AWB_GAIN_BASE) / ptmp->u16CurveR[i],
				(ONE_BASE * AWB_GAIN_BASE) / ptmp->u16CurveB_Bot[i], 3, 196, 0);
		}
	}
	//Rgan:G/R*AWB_GAIN_BASE, to R/G*ONE_BASE

	GoldenR = ptmp->dbgInfoAttr[0].stAuto.au16StaticWB[0];
	GoldenB = ptmp->dbgInfoAttr[0].stAuto.au16StaticWB[3];
	if (!GoldenR || !GoldenB) {
		GoldenR = 1024;
		GoldenB = 1024;
	}
	if (GoldenR == 1024) {
		GoldenR = ptmp->CalibRgain[1];
		GoldenB = ptmp->CalibBgain[1];
	}
	for (i = 0; i < AWB_CALIB_PTS_NUM; i++) {
		CalibR = GoldenR * ptmp->CalibRgain[i] / ptmp->CalibRgain[1];
		CalibB = GoldenB * ptmp->CalibBgain[i] / ptmp->CalibBgain[1];
		BMP_circle((ONE_BASE * AWB_GAIN_BASE) / CalibR, (ONE_BASE * AWB_GAIN_BASE) / CalibB, 15, 192, 0, 0);
	}
	BMP_circle_fill((ONE_BASE * AWB_GAIN_BASE) / ptmp->u16BalanceR, (ONE_BASE * AWB_GAIN_BASE) / ptmp->u16BalanceB,
		15, 131, 0, 255);


	RG_ratio = ptmp->dbgInfoAttrEx[0].u16CurveLLimit * ONE_BASE / 1024;
	BG_ratio = ptmp->dbgInfoAttrEx[0].u16CurveLLimit * ONE_BASE / 1024;
	BMP_line(0, BG_ratio, RG_ratio, BG_ratio, 0, 0, 0);
	BMP_line(RG_ratio, 0, RG_ratio, BG_ratio, 0, 0, 0);
	RG_ratio = ptmp->dbgInfoAttrEx[0].u16CurveRLimit * ONE_BASE / 1024;
	BG_ratio = ptmp->dbgInfoAttrEx[0].u16CurveRLimit * ONE_BASE / 1024;
	BMP_line(RG_ratio, BG_ratio, RG_ratio, BMP_VH - 1, 0, 0, 0);
	BMP_line(RG_ratio, BG_ratio, BMP_VW - 1, BG_ratio, 0, 0, 0);



	printf("1R:%d 2R:%d 3R:%d\n", pstCurveInfo[0]->u16Region1_R, pstCurveInfo[0]->u16Region2_R,
		pstCurveInfo[0]->u16Region3_R);
	RG_ratio = (ONE_BASE * AWB_GAIN_BASE) / pstCurveInfo[0]->u16Region1_R;
	BMP_line(RG_ratio, 0, RG_ratio, BMP_VH - 1, 255, 255, 0);
	RG_ratio = (ONE_BASE * AWB_GAIN_BASE) / pstCurveInfo[0]->u16Region2_R;
	BMP_line(RG_ratio, 0, RG_ratio, BMP_VH - 1, 255, 255, 0);
	RG_ratio = (ONE_BASE * AWB_GAIN_BASE) / pstCurveInfo[0]->u16Region3_R;
	BMP_line(RG_ratio, 0, RG_ratio, BMP_VH - 1, 255, 255, 0);



#if 0
	if (ptmp->dbgInfoAttrEx[0].bExtraLightEn == 1) {
		for (j = 0; j < AWB_LS_NUM; j++) {
			if (stAwbAttrInfoEx[sID].stLightInfo[j].u8LightStatus != AWB_EXTRA_LS_DONT_CARE) {
				u16Radius = stAwbAttrInfoEx[sID].stLightInfo[j].u8Radius;
				ext_ls_r_max[sID][j] = (AWB_GAIN_BASE * AWB_GAIN_SMALL_BASE)
						/ stAwbAttrInfoEx[sID].stLightInfo[j].u16WhiteRgain
					- u16Radius;
				ext_ls_r_max[sID][j] = (AWB_GAIN_BASE * AWB_GAIN_SMALL_BASE) / ext_ls_r_max[sID][j];
				ext_ls_r_min[sID][j] = (AWB_GAIN_BASE * AWB_GAIN_SMALL_BASE)
						/ stAwbAttrInfoEx[sID].stLightInfo[j].u16WhiteRgain
					+ u16Radius;
				ext_ls_r_min[sID][j] = (AWB_GAIN_BASE * AWB_GAIN_SMALL_BASE) / ext_ls_r_min[sID][j];
				ext_ls_b_max[sID][j] = (AWB_GAIN_BASE * AWB_GAIN_SMALL_BASE)
						/ stAwbAttrInfoEx[sID].stLightInfo[j].u16WhiteBgain
					- u16Radius;
				ext_ls_b_max[sID][j] = (AWB_GAIN_BASE * AWB_GAIN_SMALL_BASE) / ext_ls_b_max[sID][j];
				ext_ls_b_min[sID][j] = (AWB_GAIN_BASE * AWB_GAIN_SMALL_BASE)
						/ stAwbAttrInfoEx[sID].stLightInfo[j].u16WhiteBgain
					+ u16Radius;
				ext_ls_b_min[sID][j] = (AWB_GAIN_BASE * AWB_GAIN_SMALL_BASE) / ext_ls_b_min[sID][j];
			}
		}
	}
#endif

	if (ptmp->dbgInfoAttrEx[0].bExtraLightEn == 1) {
		for (j = 0; j < AWB_LS_NUM; j++) {
			if (ptmp->dbgInfoAttrEx[0].stLightInfo[j].u8LightStatus != AWB_EXTRA_LS_DONT_CARE) {
				if (psExLight->ext_ls_r_max[0][j] && psExLight->ext_ls_b_max[0][j]
					&& psExLight->ext_ls_r_min[0][j] && psExLight->ext_ls_b_min[0][j]) {
					x1 = (ONE_BASE * AWB_GAIN_BASE) / psExLight->ext_ls_r_max[0][j];
					y1 = (ONE_BASE * AWB_GAIN_BASE) / psExLight->ext_ls_b_max[0][j];
					x2 = (ONE_BASE * AWB_GAIN_BASE) / psExLight->ext_ls_r_min[0][j];
					y2 = (ONE_BASE * AWB_GAIN_BASE) / psExLight->ext_ls_b_min[0][j];
					if (ptmp->dbgInfoAttrEx[0].stLightInfo[j].u8LightStatus == AWB_EXTRA_LS_ADD) {
						BMP_line(x1, y1, x1, y2, 0, 255, 0);
						BMP_line(x1, y2, x2, y2, 0, 255, 0);
						BMP_line(x2, y2, x2, y1, 0, 255, 0);
						BMP_line(x2, y1, x1, y1, 0, 255, 0);
					} else {
						BMP_line(x1, y1, x1, y2, 255, 0, 0);
						BMP_line(x1, y2, x2, y2, 255, 0, 0);
						BMP_line(x2, y2, x2, y1, 255, 0, 0);
						BMP_line(x2, y1, x1, y1, 255, 0, 0);
					}
					//printf("%d:%d %d %d %d\n",j,x1,y1,x2,y2);
				}
			}
		}
	}


	BMP_ImgSave(fname);
}

void SaveAwbInfo(void)
{
#if 0
	CVI_U16 sID = 0, i, j, k;
	FILE *outtxt;
	char fname[1024];

	sprintf(fname, "%s\\wbinfo.txt", exe_dir);

	outtxt = fopen(fname, "wb");

	fprintf(outtxt, "fid:%d wdr:%d\n", pWBInfo[sID]->u32FrameID, pWBInfo[sID]->u8AwbMaxFrameNum);

	for (k = 0; k < pWBInfo[sID]->u8AwbMaxFrameNum; ++k) {
		for (i = 0; i < AWB_CHANNEL_NUM; ++i) {
			for (j = 0; j < AWB_TOTAL_SIZE; ++j) {
				fprintf(outtxt, "%d\t", pWBInfo[sID]->pu16AwbStatistics[k][i][j]);
				if ((j + 1) % pWBInfo[sID]->u16WBColumnSize == 0) {
					fprintf(outtxt, "\n");
				}
			}
			fprintf(outtxt, "\n\n");
		}
	}
	fclose(outtxt);
#endif
}

void AWB_DLL_GetVer(CVI_U16 *pVer, CVI_U16 *pSubVer)
{
	AWB_GetAlgoVer(pVer, pSubVer);
}
