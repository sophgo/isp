
#ifndef AAA_PC_PLATFORM
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#ifndef ARCH_RTOS_CV181X // TODO@CV181X
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <sys/time.h> //for gettimeofday()
#include <unistd.h> //for usleep()
#endif

#include "isp_debug.h"

#ifdef ARCH_RTOS_CV181X
#include "malloc.h"
#endif

#include "ae_buf.h"
#endif

#if defined(__CV181X__) || defined(__CV180X__) || defined(__CV186X__)
#include "isp_mgr_buf.h"
#else
static CVI_U32 u32AEParamUpdateFlag[AE_SENSOR_NUM];
#endif

#if defined(__GNUC__) && defined(__riscv)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation=" /* Or  "-Wformat-overflow"  */
#endif

#include "ae_project_param.h"
#include "ae_debug.h"

SAE_BOOT_INFO * AeBootInfo[AE_SENSOR_NUM];

ISP_EXP_INFO_S *pstAeMpiExpInfo[AE_SENSOR_NUM];

ISP_EXPOSURE_ATTR_S *pstAeMpiExposureAttr[AE_SENSOR_NUM];
ISP_EXPOSURE_ATTR_S *pstAeExposureAttrInfo[AE_SENSOR_NUM];

ISP_WDR_EXPOSURE_ATTR_S *pstAeMpiWDRExposureAttr[AE_SENSOR_NUM];
ISP_WDR_EXPOSURE_ATTR_S *pstAeWDRExposureAttrInfo[AE_SENSOR_NUM];

ISP_AE_ROUTE_S *pstAeMpiRoute[AE_SENSOR_NUM][AE_MAX_WDR_FRAME_NUM];
ISP_AE_ROUTE_S *pstAeRouteInfo[AE_SENSOR_NUM][AE_MAX_WDR_FRAME_NUM];

ISP_AE_ROUTE_EX_S *pstAeMpiRouteEx[AE_SENSOR_NUM][AE_MAX_WDR_FRAME_NUM];
ISP_AE_ROUTE_EX_S *pstAeRouteExInfo[AE_SENSOR_NUM][AE_MAX_WDR_FRAME_NUM];

ISP_SMART_EXPOSURE_ATTR_S *pstAeMpiSmartExposureAttr[AE_SENSOR_NUM];
ISP_SMART_EXPOSURE_ATTR_S *pstAeSmartExposureAttrInfo[AE_SENSOR_NUM];

ISP_AE_STATISTICS_CFG_S	*pstAeStatisticsCfg[AE_SENSOR_NUM];
ISP_AE_STATISTICS_CFG_S *pstAeStatisticsCfgInfo[AE_SENSOR_NUM];

ISP_IRIS_ATTR_S	*pstAeIrisAttr[AE_SENSOR_NUM];
ISP_IRIS_ATTR_S *pstAeIrisAttrInfo[AE_SENSOR_NUM];

ISP_DCIRIS_ATTR_S *pstAeDcIrisAttr[AE_SENSOR_NUM];
ISP_DCIRIS_ATTR_S *pstAeDcIrisAttrInfo[AE_SENSOR_NUM];

char aeDumpLogPath[MAX_AE_LOG_PATH_LENGTH] = "/var/log";
char aeDumpLogName[MAX_AE_LOG_PATH_LENGTH] = "";

CVI_U8	u8MallocCnt[AE_SENSOR_NUM];

s_AE_DBG_S *pstAeDbg;

#ifndef ARCH_RTOS_CV181X // TODO@CV181X
void *AE_Malloc(CVI_U8 sID, size_t size)
{
	void *ptr = malloc(size);

	if (ptr == CVI_NULL) {
		return CVI_NULL;
	}
	sID = AE_CheckSensorID(sID);
	memset(ptr, 0, size);
	u8MallocCnt[sID]++;
	//printf("ae u8MallocCnt[sID] cnt:%d\n", u8MallocCnt[sID]);
	return ptr;
}
#else
void *AE_Malloc(CVI_U8 sID, size_t size)
{
	void *ptr = pvPortMalloc(size);

	if (ptr == CVI_NULL) {
		return CVI_NULL;
	}
	sID = AE_CheckSensorID(sID);
	memset(ptr, 0, size);
	u8MallocCnt[sID]++;
	//printf("ae u8MallocCnt[sID] cnt:%d\n", u8MallocCnt[sID]);
	return ptr;
}
#endif

#ifndef ARCH_RTOS_CV181X // TODO@CV181X
void AE_Free(CVI_U8 sID, void *ptr)
{
	if (ptr != CVI_NULL) {
		free(ptr);
		sID = AE_CheckSensorID(sID);
		u8MallocCnt[sID] = (u8MallocCnt[sID] >= 1) ? u8MallocCnt[sID] - 1 : 0;
		//printf("ae free cnt:%d\n", u8MallocCnt[sID]);
	}
}
#else
void AE_Free(CVI_U8 sID, void *ptr)
{
	if (ptr != CVI_NULL) {
		vPortFree(ptr);
		sID = AE_CheckSensorID(sID);
		u8MallocCnt[sID] = (u8MallocCnt[sID] >= 1) ? u8MallocCnt[sID] - 1 : 0;
		//printf("ae free cnt:%d\n", u8MallocCnt[sID]);
	}
}
#endif

void AE_BootBufCreate(CVI_U8 sID)
{
	sID = AE_CheckSensorID(sID);

	if (AeBootInfo[sID] == CVI_NULL) {
		AeBootInfo[sID] = (SAE_BOOT_INFO *)AE_Malloc(sID, sizeof(SAE_BOOT_INFO));
		if (AeBootInfo[sID] == CVI_NULL) {
			ISP_LOG_ERR("%d %s\n", sID, "AeBootInfo malloc fail!");
			return;
		}
	}
}

void AE_BootBufDestroy(CVI_U8 sID)
{
	sID = AE_CheckSensorID(sID);

	if (AeBootInfo[sID]) {
		AE_Free(sID, AeBootInfo[sID]);
		AeBootInfo[sID] = CVI_NULL;
	}
}

#define AE_DBG_LOG
#define AE_MAX_LOG_STRING_LEN    128

CVI_U8	*AE_LogBuf = CVI_NULL;
CVI_U32	AE_LogBufSize;
CVI_U32 AE_LogHeadIndex, AE_LogTailIndex;
CVI_BOOL AE_CircleEnable;
CVI_U8	*AE_SnapLogBuf = CVI_NULL;
CVI_U32	AE_PreviewLogBufSize, AE_SnapLogBufSize;
CVI_U32 AE_SnapLogTailIndex;
CVI_BOOL	AE_Dump2File;
CVI_U8	AE_LogBufLevel;


void AE_LogBufCreate(CVI_U8 sID)
{

#define AE_LOG_BUFF_LEVEL_0_SIZE	(50*1024)
#define AE_LOG_BUFF_LEVEL_1_SIZE	(38*1024)
#define AE_LOG_BUFF_LEVEL_2_SIZE	(25*1024)

#define AE_SNAP_LOG_BUFF_SIZE	(22*1024)

	CVI_U8 maxFrameNum = 0, i;
	CVI_U32 bufSize = 0, snapBufSize = 0;
	CVI_U32 bufferLevelSize[3] = {
		AE_LOG_BUFF_LEVEL_0_SIZE,
		AE_LOG_BUFF_LEVEL_1_SIZE,
		AE_LOG_BUFF_LEVEL_2_SIZE,
	};

	if (AE_LogBuf == CVI_NULL) {
		for (i = 0; i < u8SensorNum; ++i) {
			maxFrameNum += AeInfo[i].u8AeMaxFrameNum;
		}

		for (i = 0 ; i < 3; ++i) {
			bufSize = maxFrameNum * bufferLevelSize[i];
			AE_LogBuf = (CVI_U8 *)AE_Malloc(sID, bufSize);
			if (AE_LogBuf) {
				AE_LogBufLevel = i;
				break;
			}
		}
		if (AE_LogBuf == CVI_NULL) {
			ISP_LOG_WARNING("printf AE Log buff alloc fail!\n");
			return;
		}
		snapBufSize = (maxFrameNum + 1) * AE_SNAP_LOG_BUFF_SIZE;
		//printf("log buff:%d level:%d create!\n", bufSize, AE_LogBufLevel);

		AE_LogBufSize = bufSize;
		AE_SnapLogBufSize = snapBufSize;
		AE_PreviewLogBufSize = (AE_LogBufSize >= AE_SnapLogBufSize) ?
				AE_LogBufSize - AE_SnapLogBufSize : 0;
		AE_LogInit();
	}
}

void AE_LogBufDestroy(CVI_U8 sID)
{
	if (AE_LogBuf) {
		AE_Free(sID, AE_LogBuf);
		AE_LogBufSize = 0;
		AE_LogBuf = CVI_NULL;
		//printf("aeLog buff destroy!\n");
	}

	if (pstAeDbg) {
		AE_Free(sID, pstAeDbg);
		pstAeDbg = CVI_NULL;
	}
}

CVI_BOOL AE_GetLogBufSize(CVI_U8 sID, CVI_U32 *bufSize)
{
	UNUSED(sID);

	*bufSize = AE_LogBufSize;
	return CVI_SUCCESS;
}

CVI_BOOL AE_GetBinBufSize(CVI_U8 sID, CVI_U32 *bufSize)
{
	UNUSED(sID);

	*bufSize = sizeof(s_AE_DBG_S);
	return CVI_SUCCESS;
}

void AE_LogInit(void)
{
	if (AE_LogBuf) {
		memset(AE_LogBuf, 0, AE_LogBufSize);
		if (AE_LogBufSize > AE_SnapLogBufSize)
			AE_SnapLogBuf = (AE_LogBuf + AE_LogBufSize - AE_SnapLogBufSize);
	}
	AE_LogHeadIndex = AE_LogTailIndex = 0;
	AE_CircleEnable = AE_Dump2File = 0;
}

#ifdef ENABLE_AE_DEBUG_LOG_TO_FILE
void AE_LogToFile(char *log, int length)
{
#define AE_LOG_SPLIT_SIZE 0x100000 // 1M

	static CVI_BOOL bEnable;

	static FILE *fp;

	static int logSize;

	CVI_U8 debugMode = AE_GetDebugMode(0);

	if (debugMode == AE_DEBUG_ENABLE_LOG_TO_FILE) {
		bEnable = CVI_TRUE;
	} else if (debugMode == AE_DEBUG_DISABLE_LOG_TO_FILE) {

		bEnable = CVI_FALSE;

		if (fp != CVI_NULL) {

			AE_SnapLogInit();
			for (CVI_U8 sID = 0; sID < u8SensorNum; ++sID) {
				AE_SaveSnapLog(sID);
			}
			AE_SaveParameterSetting();

			fwrite(AE_SnapLogBuf, 1, AE_SnapLogTailIndex, fp);

			fclose(fp);
			fp = CVI_NULL;
			logSize = 0;
		}
	}

	if (bEnable) {

		if (fp == CVI_NULL) {

			char path[MAX_AE_LOG_PATH_LENGTH];
			struct tm tm;
			struct timespec time;

			clock_gettime(CLOCK_REALTIME, &time);
			localtime_r(&time.tv_sec, &tm);

			snprintf(path, MAX_AE_LOG_PATH_LENGTH, "%s/aelog-%d-%d-%d_%d-%d-%d.txt",
				aeDumpLogPath,
				tm.tm_year + 1900,
				tm.tm_mon + 1,
				tm.tm_mday,
				tm.tm_hour,
				tm.tm_min,
				tm.tm_sec);

			fp = fopen(path, "w");

			if (fp == CVI_NULL) {
				printf("ERROR, aelog open fail, %s\n", path);
			}
		}

		if (fp != CVI_NULL && logSize <= AE_LOG_SPLIT_SIZE) {
			fwrite(log, 1, length, fp);
			logSize += length;
		}

		if (logSize > AE_LOG_SPLIT_SIZE) {

			AE_SnapLogInit();
			for (CVI_U8 sID = 0; sID < u8SensorNum; ++sID) {
				AE_SaveSnapLog(sID);
			}
			AE_SaveParameterSetting();

			fwrite(AE_SnapLogBuf, 1, AE_SnapLogTailIndex, fp);

			fclose(fp);
			fp = CVI_NULL;
			logSize = 0;
		}
	}
}
#endif

#ifndef ARCH_RTOS_CV181X // TODO@CV181X
void AE_LogPrintf(const char *szFmt, ...)
{
#ifdef AE_DBG_LOG
	va_list vaPtr;
	CVI_S8 szBuffer[AE_MAX_LOG_STRING_LEN];
	CVI_U32 bufLen, newBuflen;

	if (AE_LogBuf == CVI_NULL)
		return;

	va_start(vaPtr, (const char *)szFmt);

	vsnprintf((char *)szBuffer, AE_MAX_LOG_STRING_LEN, (const char *)szFmt, vaPtr);
	va_end(vaPtr);
	bufLen = strlen((char *)szBuffer);

#ifdef ENABLE_AE_DEBUG_LOG_TO_FILE
	AE_LogToFile((char *) szBuffer, bufLen);
#endif

	if (AE_LogTailIndex + bufLen >= AE_PreviewLogBufSize) {
		newBuflen = AE_PreviewLogBufSize - AE_LogTailIndex;
		memcpy(AE_LogBuf + AE_LogTailIndex, szBuffer, newBuflen);
		bufLen -= newBuflen;
		memcpy(AE_LogBuf, szBuffer + newBuflen, bufLen);
		AE_LogTailIndex = bufLen;
		AE_LogHeadIndex = (AE_LogTailIndex + 1) % AE_PreviewLogBufSize;
		AE_CircleEnable = 1;
	} else {
		memcpy(AE_LogBuf + AE_LogTailIndex, szBuffer, bufLen);
		AE_LogTailIndex += bufLen;
		if (AE_CircleEnable)
			AE_LogHeadIndex = (AE_LogTailIndex + 1) % AE_PreviewLogBufSize;
	}
#endif
}
#else
void AE_LogPrintf(const char *szFmt, ...)
{
	UNUSED(szFmt);
}
#endif

void AE_SnapLogInit(void)
{
	if (AE_SnapLogBuf != CVI_NULL) {
		memset(AE_SnapLogBuf, 0, AE_SnapLogBufSize);
	}

	AE_SnapLogTailIndex = 0;
}

void AE_SnapLogPrintf(const char *szFmt, ...)
{
	if (AE_SnapLogBuf == CVI_NULL) {
		return;
	}

	va_list vaPtr;
	CVI_S8 szBuffer[AE_MAX_LOG_STRING_LEN];
	CVI_U32 buflen;

	va_start(vaPtr, (const char *)szFmt);
	//vsprintf((char *)szBuffer, (const char *)szFmt, vaPtr);
	vsnprintf((char *)szBuffer, AE_MAX_LOG_STRING_LEN, (const char *)szFmt, vaPtr);
	va_end(vaPtr);
#ifndef AAA_PC_PLATFORM
	#if CHECK_AE_SIM
	UNUSED(buflen);
	printf("%s", szBuffer);
	#else
	buflen = strlen((char *)szBuffer);
	if (AE_SnapLogTailIndex + buflen >= AE_SnapLogBufSize)
		return;
	memcpy(AE_SnapLogBuf + AE_SnapLogTailIndex, szBuffer, buflen);
	AE_SnapLogTailIndex += buflen;
	#endif
#else
	printf("%s", szBuffer);
#endif

}

static void AE_GetSensorExpInfo(CVI_U8 sID, SENSOR_EXP_INFO *SensorExpInfo)
{
	SensorExpInfo->u32LinesPer500ms = AeInfo[sID].u32LinePer500ms;
	SensorExpInfo->u32FrameLine = AeInfo[sID].u32FrameLine;
	SensorExpInfo->u32MaxExpLine = AeInfo[sID].u32ExpLineMax;
	SensorExpInfo->u32MinExpLine = AeInfo[sID].u32ExpLineMin;
	SensorExpInfo->f32Accuracy = AeInfo[sID].fExpTimeAccu;
	SensorExpInfo->f32Offset = AeInfo[sID].fExpTimeOffset;
	SensorExpInfo->u32MaxAgain = AeInfo[sID].u32AGainMax;
	SensorExpInfo->u32MinAgain = AeInfo[sID].u32AGainMin;
	SensorExpInfo->u32MaxDgain = AeInfo[sID].u32DGainMax;
	SensorExpInfo->u32MinDgain = AeInfo[sID].u32DGainMin;
	SensorExpInfo->u8AERunInterval = AeInfo[sID].u8SensorRunInterval;
	SensorExpInfo->f32MaxFps = AeInfo[sID].fMaxFps;
	SensorExpInfo->f32MinFps = AeInfo[sID].fMinFps;
	SensorExpInfo->u8SensorPeriod = AeInfo[sID].u8SensorPeriod;
	SensorExpInfo->enBlcType = AeInfo[sID].enBlcType;
	SensorExpInfo->u8DGainAccuType = AeInfo[sID].u8DGainAccuType;
}

static void AE_SaveDbgBin(void)
{
	CVI_U8 i;

	if (pstAeDbg == CVI_NULL) {
		pstAeDbg = (s_AE_DBG_S *)AE_Malloc(0, sizeof(s_AE_DBG_S));
	}

	if (pstAeDbg == CVI_NULL) {
		ISP_LOG_ERR("AE Debug Bin memory alloc fail!\n");
		return;
	}

	pstAeDbg->u8AlgoVer = AE_LIB_VER;
	pstAeDbg->u8AlgoSubVer = AE_LIB_SUBVER;
	pstAeDbg->u8DbgVer = AE_DBG_BIN_VER;
	//pstAeDbg->u32Date = AE_RELEASE_DATE;
	pstAeDbg->u32BinSize = sizeof(s_AE_DBG_S);

	for (i = 0; i < u8SensorNum; i++) {
		if(pstAeMpiExposureAttr[i] != CVI_NULL) {
			pstAeDbg->stExpAtt[i] = *pstAeMpiExposureAttr[i];
			pstAeDbg->stWdrAtt[i] = *pstAeMpiWDRExposureAttr[i];
			pstAeDbg->stRoute[i][AE_LE] = *pstAeMpiRoute[i][AE_LE];
			pstAeDbg->stRouteEx[i][AE_LE] = *pstAeMpiRouteEx[i][AE_LE];
			pstAeDbg->stSmartAtr[i] = *pstAeMpiSmartExposureAttr[i];
			pstAeDbg->stStatCfg[i] = *pstAeStatisticsCfg[i];
			pstAeDbg->stSmartExpInfo[i] = AeFaceDetect[i].stSmartInfo;
			AE_GetSensorExpInfo(i, &pstAeDbg->stSensorExpInfo[i]);
		}
	}
}

#ifndef ARCH_RTOS_CV181X // TODO@CV181X
CVI_S32 AE_DumpLog(void)
{
	FILE *fp;
	char fileName[MAX_AE_LOG_FILENAME_LENGTH];

	static CVI_U16 fileCnt;

	CVI_U16 stringLength = strlen(aeDumpLogPath);

	if (AE_LogBuf == CVI_NULL) {
		AE_MemoryAlloc(0, AE_LOG_ITEM);
	}

	AE_Dump2File = 1;
	AE_Delay1ms(2);
	//length = path length(128) + filename length(128)
	if (AE_LogTailIndex != CVI_NULL) {
		if (stringLength > MAX_AE_LOG_FILENAME_LENGTH) {
			ISP_LOG_ERR("string length:%d over range %d\n", stringLength,
				MAX_AE_LOG_FILENAME_LENGTH);
			return CVI_FAILURE;
		}
		if (strlen(aeDumpLogName)) {
			snprintf(fileName, MAX_AE_LOG_FILENAME_LENGTH, "%s/%s_AeLog.txt", aeDumpLogPath, aeDumpLogName);
		} else {
			snprintf(fileName, MAX_AE_LOG_FILENAME_LENGTH, "%s/AeLog%d.txt", aeDumpLogPath, fileCnt);
		}
		fp = fopen(fileName, "w");
		if (fp == CVI_NULL) {
			printf("file:%s open fail!\n", fileName);
			return CVI_FAILURE;
		}
		fprintf(fp, "AeLib v:%d sub:%d\n", AE_LIB_VER, AE_LIB_SUBVER);
		if (!AE_CircleEnable) {
			fwrite(AE_LogBuf, 1, AE_LogTailIndex - AE_LogHeadIndex, fp);
		} else {
			fwrite(AE_LogBuf + AE_LogHeadIndex, 1, AE_PreviewLogBufSize - AE_LogHeadIndex, fp);
			fwrite(AE_LogBuf, 1, AE_LogTailIndex, fp);
		}
		//AE_DumpParameterSetting(fp);
		AE_SnapLogInit();
		for (CVI_U8 sID = 0; sID < u8SensorNum; ++sID)
			AE_SaveSnapLog(sID);
		AE_SaveParameterSetting();
		fwrite(AE_SnapLogBuf, 1, AE_SnapLogTailIndex, fp);
		fclose(fp);
		AE_LogInit();
		printf("AE logfile:%s save finish!\n", fileName);
		if (strlen(aeDumpLogName)) {
			snprintf(fileName, MAX_AE_LOG_FILENAME_LENGTH, "%s/%s_AeLog.bin", aeDumpLogPath, aeDumpLogName);
		} else {
			snprintf(fileName, MAX_AE_LOG_FILENAME_LENGTH, "%s/AeLog%d.bin", aeDumpLogPath, fileCnt);
		}

		fp = fopen(fileName, "w");
		if (fp == CVI_NULL) {
			printf("file:%s open fail!\n", fileName);
			return CVI_FAILURE;
		}
		AE_SaveDbgBin();

		if (pstAeDbg) {
			//Don't Free pstAeDbg
			printf("AE DbgBin:%s save finish %zu!\n", fileName, sizeof(s_AE_DBG_S));
			fwrite(pstAeDbg, 1, sizeof(s_AE_DBG_S), fp);
		}

		fclose(fp);
		fileCnt++;
	} else {
		AE_Dump2File = 0;
	}

	return CVI_SUCCESS;
}
#else
CVI_S32 AE_DumpLog(void)
{
	return CVI_SUCCESS;
}
#endif

CVI_BOOL AE_GetBinBuf(CVI_U8 sID, CVI_U8 buf[], CVI_U32 bufSize)
{
	CVI_U32 binSize;

	UNUSED(sID);
	binSize = sizeof(s_AE_DBG_S);
	if (bufSize < binSize) {
		ISP_LOG_ERR("Size is error, enlarge bufsize from %d to %d\n", bufSize, binSize);
		return CVI_FAILURE;
	}
	AE_SaveDbgBin();
	if (pstAeDbg != CVI_NULL) {
		memcpy(buf, pstAeDbg, binSize);
	}
	return CVI_SUCCESS;
}

CVI_BOOL AE_GetLogBuf(CVI_U8 sID, CVI_U8 buf[], CVI_U32 bufSize)
{
	CVI_U32 logSize;

	if (!AE_LogBuf) {
		AE_MemoryAlloc(sID, AE_LOG_ITEM);
	}
	AE_GetLogBufSize(sID, &logSize);
	AE_Dump2File = 1;
	AE_Delay1ms(2);
	AE_SnapLogInit();
	AE_SaveSnapLog(sID);
	AE_SaveParameterSetting();
	logSize = AAA_MIN(bufSize, logSize);
	memcpy(buf, AE_LogBuf, logSize);
	AE_Dump2File = 0;
	return CVI_SUCCESS;
}

#ifndef ARCH_RTOS_CV181X // TODO@CV181X
CVI_BOOL AE_DumpBootLog(CVI_U8 sID)
{
	FILE *fp;
	char fileName[MAX_AE_LOG_PATH_LENGTH];
	CVI_U8 i, j;
	CVI_U32 time, timeLine, ISONum;
	AE_GAIN bootGain;

	if (!AeBootInfo[sID]) {
		ISP_LOG_ERR("\n\nNULL bootLog buffer!\n");
		return CVI_FAILURE;
	}

	snprintf(fileName, MAX_AE_LOG_PATH_LENGTH, "%s/AeBootLog.txt", aeDumpLogPath);
	fp = fopen(fileName, "w");
	if (fp == CVI_NULL) {
		printf("file:%s open fail!\n", fileName);
		return CVI_FAILURE;
	}

	for (j = 0; j < AE_BOOT_MAX_FRAME; ++j) {
		for (i = 0; i < AeInfo[sID].u8AeMaxFrameNum; ++i) {
			AE_GetExpTimeByEntry(sID, AeBootInfo[sID]->stApex[i][j].s16TVEntry, &time);
			AE_GetExposureTimeLine(sID, time, &timeLine);
			AE_GetExpGainByEntry(sID, AeBootInfo[sID]->stApex[i][j].s16SVEntry, &bootGain);
			AE_GetISONumByEntry(sID, AeBootInfo[sID]->stApex[i][j].s16SVEntry, &ISONum);
			fprintf(fp, "fid(%d,%d):%d L:%d ATL:%d Bv:%d Bs:%d LV:%d CM:%d Stb:%d\n",
				sID, i, AeBootInfo[sID]->u8FrmID[j], AeBootInfo[sID]->u16FrmLuma[i][j],
				AeBootInfo[sID]->u16AdjustTargetLuma[i][j],
				AeBootInfo[sID]->stApex[i][j].s16BVEntry, AeBootInfo[sID]->s16FrmBvStep[i][j],
				AeBootInfo[sID]->s16LvX100[j], AeBootInfo[sID]->bFrmConvergeMode[i][j],
				AeBootInfo[sID]->bStable[i][j]);
			fprintf(fp, "\tTv:%d(%u %u) Sv:%d(%d %d %d) ISO:%u\n",
				AeBootInfo[sID]->stApex[i][j].s16TVEntry, time, timeLine,
				AeBootInfo[sID]->stApex[i][j].s16SVEntry, bootGain.u32AGain,
				bootGain.u32DGain, bootGain.u32ISPDGain, ISONum);
		}
		fprintf(fp, "\n");
	}

	fclose(fp);
	printf("AE bootLogfile:%s save finish!\n", fileName);
	return CVI_SUCCESS;
}
#else
CVI_BOOL AE_DumpBootLog(CVI_U8 sID)
{
	UNUSED(sID);
	return CVI_SUCCESS;
}
#endif

CVI_S32 AE_SetDumpLogPath(const char *szPath)
{
	if (strlen(szPath) >= MAX_AE_LOG_PATH_LENGTH) {
		ISP_LOG_WARNING("Path length over %d\n", MAX_AE_LOG_PATH_LENGTH);
		return CVI_FAILURE;
	}
	snprintf(aeDumpLogPath, MAX_AE_LOG_PATH_LENGTH, "%s", szPath);
	return CVI_SUCCESS;
}

CVI_S32 AE_GetDumpLogPath(char *szPath, CVI_U32 pathSize)
{
	CVI_U8 logPathSize = strlen(aeDumpLogPath);

	if (pathSize <= logPathSize) {
		ISP_LOG_WARNING("path size smaller than %d\n", logPathSize);
		return CVI_FAILURE;
	}

	strcpy(szPath, aeDumpLogPath);
	return CVI_SUCCESS;
}

CVI_S32 AE_SetDumpLogName(const char *szName)
{
	if (strlen(szName) >= MAX_AE_LOG_PATH_LENGTH) {
		ISP_LOG_WARNING("log name over %d\n", MAX_AE_LOG_PATH_LENGTH);
		return CVI_FAILURE;
	}
	snprintf(aeDumpLogName, MAX_AE_LOG_PATH_LENGTH, "%s", szName);
	return CVI_SUCCESS;
}

CVI_S32 AE_GetDumpLogName(char *szName, CVI_U32 nameSize)
{
	CVI_U8 logNameSize = strlen(aeDumpLogName);

	if (nameSize <= logNameSize) {
		ISP_LOG_WARNING("log name size smaller than %d\n", logNameSize);
		return CVI_FAILURE;
	}
	strcpy(szName, aeDumpLogName);
	return CVI_SUCCESS;
}

CVI_U8 AE_IrisParameterBufCreate(CVI_U8 sID)
{
	CVI_U8 errorCode = 0;

#if defined(__CV181X__) || defined(__CV180X__) || defined(__CV186X__)
	struct isp_3a_shared_buffer *p = CVI_NULL;

	isp_mgr_buf_get_3a_addr(sID, (CVI_VOID **) &p);
#endif

#if defined(__CV181X__) || defined(__CV180X__) || defined(__CV186X__)
	pstAeIrisAttr[sID] = &p->stIrisAttr;
#else
	if (pstAeIrisAttr[sID] == CVI_NULL) {
		pstAeIrisAttr[sID] =
			(ISP_IRIS_ATTR_S *)AE_Malloc(sID, sizeof(ISP_IRIS_ATTR_S));
		if (pstAeIrisAttr[sID] == CVI_NULL) {
			return 11;
		}
	}
#endif

	if (pstAeIrisAttrInfo[sID] == CVI_NULL) {
		pstAeIrisAttrInfo[sID] =
			(ISP_IRIS_ATTR_S *)AE_Malloc(sID, sizeof(ISP_IRIS_ATTR_S));
		if (pstAeIrisAttrInfo[sID] == CVI_NULL) {
			return 12;
		}
	}

#if defined(__CV181X__) || defined(__CV180X__) || defined(__CV186X__)
	pstAeDcIrisAttr[sID] = &p->stDcirisAttr;
#else
	if (pstAeDcIrisAttr[sID] == CVI_NULL) {
		pstAeDcIrisAttr[sID] =
			(ISP_DCIRIS_ATTR_S *)AE_Malloc(sID, sizeof(ISP_DCIRIS_ATTR_S));
		if (pstAeDcIrisAttr[sID] == CVI_NULL) {
			return 13;
		}
	}
#endif

	if (pstAeDcIrisAttrInfo[sID] == CVI_NULL) {
		pstAeDcIrisAttrInfo[sID] =
			(ISP_DCIRIS_ATTR_S *)AE_Malloc(sID, sizeof(ISP_DCIRIS_ATTR_S));
		if (pstAeDcIrisAttrInfo[sID] == CVI_NULL) {
			return 14;
		}
	}
	return errorCode;
}

void AE_IrisParameterBufDestroy(CVI_U8 sID)
{
#if defined(__CV181X__) || defined(__CV180X__) || defined(__CV186X__)
	pstAeIrisAttr[sID] = CVI_NULL;
#else
	if (pstAeIrisAttr[sID]) {
		AE_Free(sID, pstAeIrisAttr[sID]);
		pstAeIrisAttr[sID] = CVI_NULL;
	}
#endif

	if (pstAeIrisAttrInfo[sID]) {
		AE_Free(sID, pstAeIrisAttrInfo[sID]);
		pstAeIrisAttrInfo[sID] = CVI_NULL;
	}

#if defined(__CV181X__) || defined(__CV180X__) || defined(__CV186X__)
	pstAeDcIrisAttr[sID] = CVI_NULL;
#else
	if (pstAeDcIrisAttr[sID]) {
		AE_Free(sID, pstAeDcIrisAttr[sID]);
		pstAeDcIrisAttr[sID] = CVI_NULL;
	}
#endif

	if (pstAeDcIrisAttrInfo[sID]) {
		AE_Free(sID, pstAeDcIrisAttrInfo[sID]);
		pstAeDcIrisAttrInfo[sID] = CVI_NULL;
	}
}

void AE_ToolParameterBufDestroy(CVI_U8 sID)
{
	CVI_U8 i;

	sID = AE_CheckSensorID(sID);

#if defined(__CV181X__) || defined(__CV180X__) || defined(__CV186X__)
	pstAeMpiExpInfo[sID] = CVI_NULL;
#else
	if (pstAeMpiExpInfo[sID]) {
		AE_Free(sID, pstAeMpiExpInfo[sID]);
		pstAeMpiExpInfo[sID] = CVI_NULL;
	}
#endif

#if defined(__CV181X__) || defined(__CV180X__) || defined(__CV186X__)
	pstAeMpiExposureAttr[sID] = CVI_NULL;
#else
	if (pstAeMpiExposureAttr[sID]) {
		AE_Free(sID, pstAeMpiExposureAttr[sID]);
		pstAeMpiExposureAttr[sID] = CVI_NULL;
	}
#endif

	if (pstAeExposureAttrInfo[sID]) {
		AE_Free(sID, pstAeExposureAttrInfo[sID]);
		pstAeExposureAttrInfo[sID] = CVI_NULL;
	}

#if defined(__CV181X__) || defined(__CV180X__) || defined(__CV186X__)
	pstAeMpiWDRExposureAttr[sID] = CVI_NULL;
#else
	if (pstAeMpiWDRExposureAttr[sID]) {
		AE_Free(sID, pstAeMpiWDRExposureAttr[sID]);
		pstAeMpiWDRExposureAttr[sID] = CVI_NULL;
	}
#endif

	if (pstAeWDRExposureAttrInfo[sID]) {
		AE_Free(sID, pstAeWDRExposureAttrInfo[sID]);
		pstAeWDRExposureAttrInfo[sID] = CVI_NULL;
	}

#if defined(__CV181X__) || defined(__CV180X__) || defined(__CV186X__)
	for (i = 0; i < AeInfo[sID].u8AeMaxFrameNum; ++i) {

		pstAeMpiRoute[sID][i] = CVI_NULL;
		pstAeMpiRouteEx[sID][i] = CVI_NULL;

		if (pstAeRouteInfo[sID][i]) {
			AE_Free(sID, pstAeRouteInfo[sID][i]);
			pstAeRouteInfo[sID][i] = CVI_NULL;
		}

		if (pstAeRouteExInfo[sID][i]) {
			AE_Free(sID, pstAeRouteExInfo[sID][i]);
			pstAeRouteExInfo[sID][i] = CVI_NULL;
		}
	}
#else
	for (i = 0; i < AeInfo[sID].u8AeMaxFrameNum; ++i) {
		if (pstAeMpiRoute[sID][i]) {
			AE_Free(sID, pstAeMpiRoute[sID][i]);
			pstAeMpiRoute[sID][i] = CVI_NULL;
		}

		if (pstAeRouteInfo[sID][i]) {
			AE_Free(sID, pstAeRouteInfo[sID][i]);
			pstAeRouteInfo[sID][i] = CVI_NULL;
		}

		if (pstAeMpiRouteEx[sID][i]) {
			AE_Free(sID, pstAeMpiRouteEx[sID][i]);
			pstAeMpiRouteEx[sID][i] = CVI_NULL;
		}

		if (pstAeRouteExInfo[sID][i]) {
			AE_Free(sID, pstAeRouteExInfo[sID][i]);
			pstAeRouteExInfo[sID][i] = CVI_NULL;
		}
	}
#endif

#if defined(__CV181X__) || defined(__CV180X__) || defined(__CV186X__)
	pstAeMpiSmartExposureAttr[sID] = CVI_NULL;
#else
	if (pstAeMpiSmartExposureAttr[sID]) {
		AE_Free(sID, pstAeMpiSmartExposureAttr[sID]);
		pstAeMpiSmartExposureAttr[sID] = CVI_NULL;
	}
#endif

	if (pstAeSmartExposureAttrInfo[sID]) {
		AE_Free(sID, pstAeSmartExposureAttrInfo[sID]);
		pstAeSmartExposureAttrInfo[sID] = CVI_NULL;
	}

#if defined(__CV181X__) || defined(__CV180X__) || defined(__CV186X__)
	pstAeStatisticsCfg[sID] = CVI_NULL;
#else
	if (pstAeStatisticsCfg[sID]) {
		AE_Free(sID, pstAeStatisticsCfg[sID]);
		pstAeStatisticsCfg[sID] = CVI_NULL;
	}
#endif

	if (pstAeStatisticsCfgInfo[sID]) {
		AE_Free(sID, pstAeStatisticsCfgInfo[sID]);
		pstAeStatisticsCfgInfo[sID] = CVI_NULL;
	}

	AE_IrisParameterBufDestroy(sID);
}


void AE_ToolParameterBufCreate(CVI_U8 sID)
{
	CVI_U8 errorCode = 0;
	CVI_U8 i;

	sID = AE_CheckSensorID(sID);

#if defined(__CV181X__) || defined(__CV180X__) || defined(__CV186X__)

	struct isp_3a_shared_buffer *p = CVI_NULL;

	isp_mgr_buf_get_3a_addr(sID, (CVI_VOID **) &p);
#endif

#if defined(__CV181X__) || defined(__CV180X__) || defined(__CV186X__)
	pstAeMpiExpInfo[sID] = &p->stExpInfo;
#else
	if (pstAeMpiExpInfo[sID] == CVI_NULL) {
		pstAeMpiExpInfo[sID] =
			(ISP_EXP_INFO_S *)AE_Malloc(sID, sizeof(ISP_EXP_INFO_S));
		if (pstAeMpiExpInfo[sID] == CVI_NULL) {
			errorCode = 1;
			goto EXIT;
		}
	}
#endif

#if defined(__CV181X__) || defined(__CV180X__) || defined(__CV186X__)
	pstAeMpiExposureAttr[sID] = &p->stExpAttr;
#else
	if (pstAeMpiExposureAttr[sID] == CVI_NULL) {
		pstAeMpiExposureAttr[sID] =
			(ISP_EXPOSURE_ATTR_S *)AE_Malloc(sID, sizeof(ISP_EXPOSURE_ATTR_S));
		if (pstAeMpiExposureAttr[sID] == CVI_NULL) {
			errorCode = 1;
			goto EXIT;
		}
	}
#endif

	if (pstAeExposureAttrInfo[sID] == CVI_NULL) {
		pstAeExposureAttrInfo[sID] =
			(ISP_EXPOSURE_ATTR_S *)AE_Malloc(sID, sizeof(ISP_EXPOSURE_ATTR_S));
		if (pstAeExposureAttrInfo[sID] == CVI_NULL) {
			errorCode = 2;
			goto EXIT;
		}
	}

#if defined(__CV181X__) || defined(__CV180X__) || defined(__CV186X__)
	pstAeMpiWDRExposureAttr[sID] = &p->stWDRExpAttr;
#else
	if (pstAeMpiWDRExposureAttr[sID] == CVI_NULL) {
		pstAeMpiWDRExposureAttr[sID] =
			(ISP_WDR_EXPOSURE_ATTR_S *)AE_Malloc(sID, sizeof(ISP_WDR_EXPOSURE_ATTR_S));
		if (pstAeMpiWDRExposureAttr[sID] == CVI_NULL) {
			errorCode = 3;
			goto EXIT;
		}
	}
#endif

	if (pstAeWDRExposureAttrInfo[sID] == CVI_NULL) {
		pstAeWDRExposureAttrInfo[sID] =
			(ISP_WDR_EXPOSURE_ATTR_S *)AE_Malloc(sID, sizeof(ISP_WDR_EXPOSURE_ATTR_S));
		if (pstAeWDRExposureAttrInfo[sID] == CVI_NULL) {
			errorCode = 4;
			goto EXIT;
		}
	}

#if defined(__CV181X__) || defined(__CV180X__) || defined(__CV186X__)
	pstAeMpiRoute[sID][AE_LE] = &p->stAERouteAttr;
	pstAeMpiRouteEx[sID][AE_LE] = &p->stAERouteAttrEx;

	if (AeInfo[sID].u8AeMaxFrameNum > 1) {
		pstAeMpiRoute[sID][AE_SE] = &p->stAERouteSFAttr;
		pstAeMpiRouteEx[sID][AE_SE] = &p->stAERouteSFAttrEx;
	}

	for (i = 0; i < AeInfo[sID].u8AeMaxFrameNum; ++i) {
		if (pstAeRouteInfo[sID][i] == CVI_NULL) {
			pstAeRouteInfo[sID][i] =
				(ISP_AE_ROUTE_S *)AE_Malloc(sID, sizeof(ISP_AE_ROUTE_S));
			if (pstAeRouteInfo[sID][i] == CVI_NULL) {
				errorCode = 6;
				goto EXIT;
			}
		}

		if (pstAeRouteExInfo[sID][i] == CVI_NULL) {
			pstAeRouteExInfo[sID][i] =
				(ISP_AE_ROUTE_EX_S *)AE_Malloc(sID, sizeof(ISP_AE_ROUTE_EX_S));
			if (pstAeRouteExInfo[sID][i] == CVI_NULL) {
				errorCode = 8;
				goto EXIT;
			}
		}
	}
#else
	for (i = 0; i < AeInfo[sID].u8AeMaxFrameNum; ++i) {
		if (pstAeMpiRoute[sID][i] == CVI_NULL) {
			pstAeMpiRoute[sID][i] =
				(ISP_AE_ROUTE_S *)AE_Malloc(sID, sizeof(ISP_AE_ROUTE_S));
			if (pstAeMpiRoute[sID][i] == CVI_NULL) {
				errorCode = 5;
				goto EXIT;
			}
		}

		if (pstAeRouteInfo[sID][i] == CVI_NULL) {
			pstAeRouteInfo[sID][i] =
				(ISP_AE_ROUTE_S *)AE_Malloc(sID, sizeof(ISP_AE_ROUTE_S));
			if (pstAeRouteInfo[sID][i] == CVI_NULL) {
				errorCode = 6;
				goto EXIT;
			}
		}

		if (pstAeMpiRouteEx[sID][i] == CVI_NULL) {
			pstAeMpiRouteEx[sID][i] =
				(ISP_AE_ROUTE_EX_S *)AE_Malloc(sID, sizeof(ISP_AE_ROUTE_EX_S));
			if (pstAeMpiRouteEx[sID][i] == CVI_NULL) {
				errorCode = 7;
				goto EXIT;
			}
		}

		if (pstAeRouteExInfo[sID][i] == CVI_NULL) {
			pstAeRouteExInfo[sID][i] =
				(ISP_AE_ROUTE_EX_S *)AE_Malloc(sID, sizeof(ISP_AE_ROUTE_EX_S));
			if (pstAeRouteExInfo[sID][i] == CVI_NULL) {
				errorCode = 8;
				goto EXIT;
			}
		}
	}
#endif

#if defined(__CV181X__) || defined(__CV180X__) || defined(__CV186X__)
	pstAeMpiSmartExposureAttr[sID] = &p->stSmartExpAttr;
#else
	if (pstAeMpiSmartExposureAttr[sID] == CVI_NULL) {
		pstAeMpiSmartExposureAttr[sID] =
			(ISP_SMART_EXPOSURE_ATTR_S *)AE_Malloc(sID, sizeof(ISP_SMART_EXPOSURE_ATTR_S));
		if (pstAeMpiSmartExposureAttr[sID] == CVI_NULL) {
			errorCode = 9;
			goto EXIT;
		}
	}
#endif

	if (pstAeSmartExposureAttrInfo[sID] == CVI_NULL) {
		pstAeSmartExposureAttrInfo[sID] =
			(ISP_SMART_EXPOSURE_ATTR_S *)AE_Malloc(sID, sizeof(ISP_SMART_EXPOSURE_ATTR_S));
		if (pstAeSmartExposureAttrInfo[sID] == CVI_NULL) {
			errorCode = 10;
			goto EXIT;
		}
	}

#if defined(__CV181X__) || defined(__CV180X__) || defined(__CV186X__)
	pstAeStatisticsCfg[sID] = &p->stAeStatCfg;
#else
	if (pstAeStatisticsCfg[sID] == CVI_NULL) {
		pstAeStatisticsCfg[sID] =
			(ISP_AE_STATISTICS_CFG_S *)AE_Malloc(sID, sizeof(ISP_AE_STATISTICS_CFG_S));
		if (pstAeStatisticsCfg[sID] == CVI_NULL) {
			errorCode = 11;
			goto EXIT;
		}
	}
#endif

	if (pstAeStatisticsCfgInfo[sID] == CVI_NULL) {
		pstAeStatisticsCfgInfo[sID] =
			(ISP_AE_STATISTICS_CFG_S *)AE_Malloc(sID, sizeof(ISP_AE_STATISTICS_CFG_S));
		if (pstAeStatisticsCfgInfo[sID] == CVI_NULL) {
			errorCode = 12;
			goto EXIT;
		}
	}

	errorCode = AE_IrisParameterBufCreate(sID);
	if (errorCode) {
		goto EXIT;
	}

EXIT:
	if (errorCode)
		ISP_LOG_ERR("%s Error:%d\n", "AeToolParam malloc fail!", errorCode);
}

void AE_SetParamUpdateFlag(CVI_U8 sID, AE_PARAMETER_UPDATE flag)
{
#if defined(__CV181X__) || defined(__CV180X__) || defined(__CV186X__)
	struct isp_3a_shared_buffer *p = CVI_NULL;

	isp_mgr_buf_get_3a_addr(sID, (CVI_VOID **) &p);
	p->u32AEParamUpdateFlag |= (0x01 << flag);
#else
	u32AEParamUpdateFlag[sID] |= (0x01 << flag);
#endif
}

void AE_CheckParamUpdateFlag(CVI_U8 sID)
{
	CVI_U32 flag = 0;

#if defined(__CV181X__) || defined(__CV180X__) || defined(__CV186X__)
	struct isp_3a_shared_buffer *p = CVI_NULL;

	isp_mgr_buf_get_3a_addr(sID, (CVI_VOID **) &p);
	flag = p->u32AEParamUpdateFlag;
	if (flag != 0x00) {
		p->u32AEParamUpdateFlag = 0x00;
	}
#else
	flag = u32AEParamUpdateFlag[sID];
	if (flag != 0x00) {
		u32AEParamUpdateFlag[sID] = 0x00;
	}
#endif

#define __CHECK_PARAMETER_FLAG(__FLAG) {\
	if (flag & (0x01 << __FLAG)) {\
		AeInfo[sID].bParameterUpdate[__FLAG] = 1;\
	} \
}

	if (flag != 0x00) {
		__CHECK_PARAMETER_FLAG(AE_EXPOSURE_ATTR_UPDATE);
		__CHECK_PARAMETER_FLAG(AE_WDR_EXPOSURE_ATTR_UPDATE);
		__CHECK_PARAMETER_FLAG(AE_ROUTE_UPDATE);
		__CHECK_PARAMETER_FLAG(AE_ROUTE_EX_UPDATE);
		__CHECK_PARAMETER_FLAG(AE_STATISTICS_CONFIG_UPDATE);
		__CHECK_PARAMETER_FLAG(AE_SMART_EXPOSURE_ATTR_UPDATE);
		__CHECK_PARAMETER_FLAG(AE_IRIS_ATTR_UPDATE);
		__CHECK_PARAMETER_FLAG(AE_DC_IRIS_ATTR_UPDATE);
		__CHECK_PARAMETER_FLAG(AE_ROUTE_SF_UPDATE);
		__CHECK_PARAMETER_FLAG(AE_ROUTE_SF_EX_UPDATE);
	}
}

void AE_MemoryAlloc(CVI_U8 sID, AE_MEMORY_ITEM item)
{
	if (item == AE_BOOT_ITEM) {
		AE_BootBufCreate(sID);
	} else if (item == AE_LOG_ITEM) {
		AE_LogBufCreate(0);
	} else if (item == AE_TOOL_PARAMETER_ITEM) {
		AE_ToolParameterBufCreate(sID);
	}
}

void AE_MemoryFree(CVI_U8 sID, AE_MEMORY_ITEM item)
{
	UNUSED(sID);
	if (item == AE_BOOT_ITEM) {
		AE_BootBufDestroy(sID);
	} else if (item == AE_LOG_ITEM) {
		AE_LogBufDestroy(0);
	} else if (item == AE_TOOL_PARAMETER_ITEM) {
		AE_ToolParameterBufDestroy(sID);
	}
}

#ifdef ARCH_RTOS_CV181X // TODO: mason.zou
#include "isp_mgr_buf.h"
void AE_RtosBufInit(CVI_U8 sID)
{
	struct isp_3a_shared_buffer *p = CVI_NULL;

	isp_mgr_buf_get_3a_addr(sID, (CVI_VOID **) &p);

	pstAeMpiExpInfo[sID] = &p->stExpInfo;
	pstAeMpiExposureAttr[sID] = &p->stExpAttr;
	pstAeMpiWDRExposureAttr[sID] = &p->stWDRExpAttr;
	pstAeMpiRoute[sID][AE_LE] = &p->stAERouteAttr;
	pstAeMpiRouteEx[sID][AE_LE] = &p->stAERouteAttrEx;
	pstAeMpiSmartExposureAttr[sID] = &p->stSmartExpAttr;
	pstAeStatisticsCfg[sID] = &p->stAeStatCfg;
	pstAeIrisAttr[sID] = &p->stIrisAttr;
	pstAeDcIrisAttr[sID] = &p->stDcirisAttr;
}
#endif

#ifdef ENABLE_ISP_IPC
#include "isp_mgr_buf.h"
void AE_BufInit(CVI_U8 sID)
{
	struct isp_3a_shared_buffer *p = CVI_NULL;

	isp_mgr_buf_get_3a_addr(sID, (CVI_VOID **) &p);

	pstAeMpiExpInfo[sID] = &p->stExpInfo;
	pstAeMpiExposureAttr[sID] = &p->stExpAttr;
	pstAeMpiWDRExposureAttr[sID] = &p->stWDRExpAttr;
	pstAeMpiRoute[sID][AE_LE] = &p->stAERouteAttr;
	pstAeMpiRouteEx[sID][AE_LE] = &p->stAERouteAttrEx;
	pstAeMpiSmartExposureAttr[sID] = &p->stSmartExpAttr;
	pstAeStatisticsCfg[sID] = &p->stAeStatCfg;
	pstAeIrisAttr[sID] = &p->stIrisAttr;
	pstAeDcIrisAttr[sID] = &p->stDcirisAttr;
}
#endif

#if defined(__GNUC__) && defined(__riscv)
#pragma GCC diagnostic pop
#endif
