/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: ae_detect_flicker.c
 * Description:
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "ae_project_param.h"
#include "aealgo.h"

#include "cvi_ae.h"
#include "ae_detect_flicker.h"


#ifndef AAA_PC_PLATFORM

#ifndef _Windows
#ifndef ARCH_RTOS_CV181X // TODO@CV181X
#include <sys/time.h>//for gettimeofday()
#include <unistd.h>//for usleep()
#endif

#include "isp_main.h"
#include "ae_debug.h"
#include "isp_debug.h"
#else
#include "Unit1.h"
#endif

#endif



//#define CHECK_LOG

#define VAR_THR_120 (14)// TODO,USE PQ parameter later.
#define VAR_THR_60 (20)
#define VAR_THR_30 (20)

S_AE_DET_FLICKER sAeDetFlicker[AE_SENSOR_NUM];

#ifdef CHECK_LOG
#define DetFDebug(fmt, args...) printf(fmt, ##args)
#else
#define DetFDebug(fmt, args...)
#endif

// flow :
//	1.AE_EnableFlickerDetect
//	2. AE_DoFlickDetect per frame
//	3. check sAeDetFlicker[sID].FlickerStatus

void AE_EnableFlickerDetect(CVI_U8 sID, CVI_U8 ena)
{
	SAE_INFO *pInfo;

	if (sAeDetFlicker[sID].bIsEnable != ena) {
		AE_GetCurrentInfo(sID, &pInfo);
		sAeDetFlicker[sID].fps = pInfo->fFps;
#ifdef CHECK_LOG
		sAeDetFlicker[sID].bIsDbgWrite = 1;
		sAeDetFlicker[sID].bIsDbgLog = 1;
#endif
		sAeDetFlicker[sID].inx = 0;
		sAeDetFlicker[sID].bIsEnable = ena;
		DetFDebug("Ena Flicker Det %d %f %d\n", ena, pInfo->fFps, sAeDetFlicker[sID].fps);
	}
}

CVI_U8 isFlicker(CVI_U8 sID, CVI_S16 exp_t, CVI_U8 idx)
{
#define LUMA_STABLE_THR (8)
#define DRAK_THR (340)// 17* 20

	CVI_U8 ret = 0;
	CVI_U8 i, j;
	CVI_U8 isStable50 = 1;
	CVI_U32 maxV, minV;
	CVI_U32 var_thr;
	CVI_U16 darkcnt = 0;

	CVI_U32 stbvarMax = 0, stbvarMin = 0xffff;
	CVI_U8 inx0, inxStbSize;
	CVI_U32 frameR[5][FLICK_V_BIN], diff[5];
	CVI_U32 *p32StabFrm;

	inx0 = idx;
	if (sAeDetFlicker[sID].fps == 25) {
		inxStbSize = 5;//(5)*40ms = 200ms,stable for 60Hz light
	} else {
		inxStbSize = 3;//(3)*33.3ms = 100ms stable for 50Hz light
	}
	p32StabFrm = &frameR[inxStbSize - 1][0];

	for (j = 0; j < inxStbSize; j++) {
		for (i = 0; i < FLICK_V_BIN; i++) {
			if (sAeDetFlicker[sID].VLuma[inx0][i] > DRAK_THR) {
				frameR[j][i] = (sAeDetFlicker[sID].VLuma[inx0 + 1 + j][i] * 256) /
					sAeDetFlicker[sID].VLuma[inx0][i];
			} else {
				frameR[j][i] = 256;
			}
		}
	}
	for (i = 0; i < FLICK_V_BIN; i++) {
		if (sAeDetFlicker[sID].VLuma[inx0][i] < DRAK_THR) {
			darkcnt++;
		}
		if (p32StabFrm[i] > stbvarMax)
			stbvarMax = p32StabFrm[i];
		if (p32StabFrm[i] < stbvarMin)
			stbvarMin = p32StabFrm[i];
	}
	if (darkcnt > 2) {
		isStable50 = 0;
	}
	if ((stbvarMax - stbvarMin) > LUMA_STABLE_THR) {
		isStable50 = 0;
	}
#ifndef ARCH_RTOS_CV181X // TODO@CV181X
	if (sAeDetFlicker[sID].bIsDbgWrite) {
		fprintf(sAeDetFlicker[sID].fidDbg, "\nidx:%d %d\n", inx0, inxStbSize);
		for (i = 0; i < FLICK_V_BIN; i++) {
			for (j = 0; j < inxStbSize; j++) {
				fprintf(sAeDetFlicker[sID].fidDbg, "%d\t", frameR[j][i]);
			}
			fprintf(sAeDetFlicker[sID].fidDbg, "\n");
		}
		fprintf(sAeDetFlicker[sID].fidDbg, "isStable50:%d dark:%d %d %d\n",
			isStable50, darkcnt, stbvarMax, stbvarMin);
		DetFDebug("isStable50:%d dark:%d\n", isStable50, darkcnt);
	}
#endif

	if ((!isStable50) && (!sAeDetFlicker[sID].bIsDbgWrite)) {
		return 0;
	}
	if (exp_t == EVTT_ENTRY_1_120SEC || exp_t == EVTT_ENTRY_1_100SEC) {
		var_thr = VAR_THR_120;
	} else if (exp_t == EVTT_ENTRY_1_60SEC || exp_t == EVTT_ENTRY_1_50SEC) {
		var_thr = VAR_THR_60;
	} else {
		var_thr = VAR_THR_30;
	}

	for (j = 0; j < inxStbSize - 1; j++) {
		maxV = 0;
		minV = 0xFFFF;
		for (i = 0; i < FLICK_V_BIN; i++) {
			frameR[j][i] = frameR[j][i] * 256 / p32StabFrm[i];
			if (frameR[j][i] > maxV) {
				maxV = frameR[j][i];
			}
			if (frameR[j][i] < minV) {
				minV = frameR[j][i];
			}
		}
		diff[j] = maxV - minV;
		if (diff[j] > var_thr) {
			ret = 1;
		}
	}

#ifndef ARCH_RTOS_CV181X // TODO@CV181X
	if (sAeDetFlicker[sID].bIsDbgWrite) {
		for (i = 0; i < FLICK_V_BIN; i++) {
			for (j = 0; j < inxStbSize - 1; j++) {
				fprintf(sAeDetFlicker[sID].fidDbg, "%d\t", frameR[j][i]);
			}
			fprintf(sAeDetFlicker[sID].fidDbg, "\n");
		}
		fprintf(sAeDetFlicker[sID].fidDbg, "%d\t%d\t%d\t%d\n", diff[0], diff[1], diff[2], diff[3]);
		fprintf(sAeDetFlicker[sID].fidDbg, "var_thr:%d exp_t:%d\n", var_thr, exp_t);
	}
	if (sAeDetFlicker[sID].bIsDbgLog) {
		printf("%d,dif %d %d %d %d,exp %d\n", idx, diff[0], diff[1], diff[2], diff[3], exp_t);
	}
#endif

	return ret;
}

void CalcFlick(CVI_U8 sID, CVI_S16 exp_t)
{
	CVI_U32 i;
	CVI_U8 isF[3] = { 0 };

#ifndef ARCH_RTOS_CV181X // TODO@CV181X
	if (sAeDetFlicker[sID].bIsDbgWrite) {
		DetFDebug("Write AE win ..\n");
#ifndef _Windows
		sAeDetFlicker[sID].fidDbg = fopen("/var/log/flicker.txt", "wb");
#else
		sAeDetFlicker[sID].fidDbg = fopen("flicker.txt", "wb");
#endif
		fprintf(sAeDetFlicker[sID].fidDbg, "fps:%d\n", sAeDetFlicker[sID].fps);
		for (i = 0; i < 8; i++) {
			fprintf(sAeDetFlicker[sID].fidDbg, "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
				sAeDetFlicker[sID].VLuma[0][i], sAeDetFlicker[sID].VLuma[1][i],
				sAeDetFlicker[sID].VLuma[2][i], sAeDetFlicker[sID].VLuma[3][i],
				sAeDetFlicker[sID].VLuma[4][i], sAeDetFlicker[sID].VLuma[5][i],
				sAeDetFlicker[sID].VLuma[6][i], sAeDetFlicker[sID].VLuma[7][i]);
		}
		fprintf(sAeDetFlicker[sID].fidDbg, "\n");
	}
#endif
	for (i = 0; i < 3; i++) {
		isF[i] = isFlicker(sID, exp_t, i);
		if (isF[i] == 0 && sAeDetFlicker[sID].bIsDbgWrite == 0) {
			break;
		}
	}

	if ((isF[0] == 1) && (isF[1] == 1) && (isF[2] == 1)) {
		sAeDetFlicker[sID].bIsEnable = 0;// stop detect
		sAeDetFlicker[sID].FlickerStatus = FCLICKER_YES;
	}
	if (sAeDetFlicker[sID].bIsDbgLog) {
		DetFDebug("Flicker\n");
	}
#ifndef ARCH_RTOS_CV181X // TODO@CV181X
	if (sAeDetFlicker[sID].bIsDbgWrite) {
		sAeDetFlicker[sID].bIsDbgWrite = 0;
		fprintf(sAeDetFlicker[sID].fidDbg, "F0:%d F1:%d F2:%d\n", isF[0], isF[1], isF[2]);
		fclose(sAeDetFlicker[sID].fidDbg);
		DetFDebug("Write AE ..OK\n");
	}
#endif
}

void AE_DoFlickDetect(CVI_U8 sID, CVI_S16 expinx)
{
	CVI_U32 i, j, inx;
	CVI_U32 sumY[FLICK_V_BIN] = { 0 }, cntY[FLICK_V_BIN] = { 0 };
	SAE_INFO *pInfo;

	AE_GetCurrentInfo(sID, &pInfo);

	if (sAeDetFlicker[sID].bIsEnable == 0) {
		sAeDetFlicker[sID].inx = 0;
		return;
	}
	if (sAeDetFlicker[sID].fps != 25 && sAeDetFlicker[sID].fps != 30) {
		return;
	}

	if (sAeDetFlicker[sID].fps == 30) {
		if (expinx != EVTT_ENTRY_1_120SEC && expinx != EVTT_ENTRY_1_60SEC && expinx != EVTT_ENTRY_1_30SEC) {
			sAeDetFlicker[sID].inx = 0;
			return;
		}
	} else if (sAeDetFlicker[sID].fps == 25) {
		if (expinx != EVTT_ENTRY_1_100SEC && expinx != EVTT_ENTRY_1_50SEC && expinx != EVTT_ENTRY_1_25SEC) {
			sAeDetFlicker[sID].inx = 0;
			return;
		}
	} else {
		return;
	}

	for (i = 0; i < AE_ZONE_ROW; i++) {// H:15
		inx = i / 2;
		if (inx < FLICK_V_BIN) {
			for (j = 0; j < AE_ZONE_COLUMN; j++) {// W:17
				sumY[inx] += pInfo->pu16AeStatistics[AE_LE][i][j][1];// G
			}
			cntY[inx]++;
		}
	}
	for (i = 0; i < FLICK_V_BIN; i++) {// VLuma = Ori *17
		if (cntY[i]) {
			sAeDetFlicker[sID].VLuma[sAeDetFlicker[sID].inx][i] = sumY[i] / cntY[i];
		}
	}
	sAeDetFlicker[sID].inx++;
	if (sAeDetFlicker[sID].inx >= FLICK_FRAME_NEED) {
		CalcFlick(sID, expinx);
		sAeDetFlicker[sID].inx = 0;
	}
}
