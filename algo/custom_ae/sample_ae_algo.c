/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: sample_ae_algo.c
 * Description:
 *
 */

/**************************************************************************
 *						  H E A D E R	F I L E S
 **************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sample_ae_algo.h"

/**************************************************************************
 *							C O N S T A N T S
 **************************************************************************/
/**************************************************************************
 *								M A C R O S
 **************************************************************************/
#define SIMPLE_AE_STEP_BASE		(128)
#define SIMPLE_AE_STEP_SCALE	(4)//reduce it to speed AE

#define AE_LE_PERCENT_L	(10)
#define AE_LE_PERCENT_H	(90)
#define AE_SE_PERCENT_L	(60)
#define AE_SE_PERCENT_H	(90)

#define STABLE_THR	(8)

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

CVI_U16 lumaHist[AE_MAX_WDR_FRAME_NUM][HIST_BIN_SIZE];

CVI_U32 curExpTimeUs = 33333, curAgain = AE_GAIN_BASE;
CVI_U16 sample_ae_target = 46;//8 bits
CVI_U16 sample_ae_max_time = 33333;//33.33ms
CVI_U16 sample_ae_min_time = 100;//0.1ms
CVI_U16 sample_ae_max_gain = 16384;//
CVI_U16 sample_ae_min_gain = AE_GAIN_BASE;//



CVI_U16 hist_l[AE_MAX_WDR_FRAME_NUM];
CVI_U16 hist_h[AE_MAX_WDR_FRAME_NUM];

CVI_U16 curRgain = 1024, curBgain = 1024;

void SAMPLE_AE_Algo(SAMPLE_AE_ALGO_S *pInfo, SAMPLE_AE_CTRL_S *pCtr)
{
	CVI_U16 i, j, frmCnt;
	CVI_U16 luma;
	CVI_U32 totalG[AE_MAX_WDR_FRAME_NUM];
	CVI_U16 ratio;
	CVI_U16 sumCnt;
	CVI_U16 inx_L[AE_MAX_WDR_FRAME_NUM] = {0}, inx_H[AE_MAX_WDR_FRAME_NUM] = {0}, inx_Flag = 0;
	CVI_U16 totalCnt[AE_MAX_WDR_FRAME_NUM];
	CVI_U8 sample_ae_step;
	CVI_U8 frm_num = 1;
	CVI_U32 tR, tB;
	CVI_U8 isReachBound = 0;
	CVI_U8 speed_scale = SIMPLE_AE_STEP_SCALE;
	AEINFO_S *pAEINFO_S;
	CVI_U16 winNum;

	pAEINFO_S = (AEINFO_S *)&(pInfo->pstFEAeStat3->au16ZoneAvg[0]);
	memset(lumaHist, 0, sizeof(CVI_U8)*HIST_BIN_SIZE*AE_MAX_WDR_FRAME_NUM);

	totalG[AE_LE] = 0;
	totalG[AE_SE] = 0;
	totalCnt[AE_LE] = 0;
	totalCnt[AE_SE] = 0;

	for (i = 0; i < pInfo->winYNum; i++) {
		for (j = 0; j < pInfo->winXNum; j++) {
			luma = pAEINFO_S->au16ZoneAvg[i][j][1];//G
			if (luma > AE_MAX_LUMA)
				luma = AE_MAX_LUMA;
			lumaHist[AE_LE][luma>>2]++;
		}
	}
	winNum = pInfo->winXNum * pInfo->winYNum;
	//LE_LOW:10%	SE_LOW:60%
	hist_l[0] = (AE_LE_PERCENT_L * winNum)/100;
	hist_l[1] = (AE_SE_PERCENT_L * winNum)/100;
	//LE_HIGH:60%	SE_HIGH:90%
	hist_h[0] = (AE_LE_PERCENT_H * winNum)/100;
	hist_h[1] = (AE_SE_PERCENT_H * winNum)/100;
	for (frmCnt = 0; frmCnt < frm_num; frmCnt++) {
		sumCnt = 0;
		inx_Flag = 0;
		for (i = 0; i < HIST_BIN_SIZE; i++) {
			sumCnt += lumaHist[frmCnt][i];
			//printf("%d	%d	%d\n",i,lumaHist[0][i],sumCnt);
			if (inx_Flag == 0) {
				if (sumCnt > hist_l[frmCnt]) {
					inx_L[frmCnt] = i;
					inx_H[frmCnt] = i;//for i=255
					inx_Flag = 1;
				}
			} else if (inx_Flag == 1) {
				if (sumCnt > hist_h[frmCnt]) {
					inx_H[frmCnt] = i;
					inx_Flag = 2;
					break;
				}
			}
		}
		for (i = inx_L[frmCnt]; i <= inx_H[frmCnt]; i++) {
			totalG[frmCnt] += (lumaHist[frmCnt][i] * i);//Cnt * Luma(8 bit)
			totalCnt[frmCnt] += lumaHist[frmCnt][i];//Cnt
		}
	}
	//printf("inx_L %d %d,%d %d\n",hist_l[AE_LE],hist_h[AE_LE],inx_L[AE_LE],inx_H[AE_LE]);
	//printf("%d %d\n",totalG[AE_LE],totalCnt[AE_LE]);

	if (totalCnt[AE_LE]) {
		totalG[AE_LE] = totalG[AE_LE] / totalCnt[AE_LE];
	}
	ratio = (totalG[AE_LE] * SIMPLE_AE_STEP_BASE) / sample_ae_target;
	if (ratio > SIMPLE_AE_STEP_BASE) {
		sample_ae_step = (ratio - SIMPLE_AE_STEP_BASE)/speed_scale;
		//printf("ratio %d %d +%2d\n", ratio, totalG[0], sample_ae_step);
	} else {
		sample_ae_step = (SIMPLE_AE_STEP_BASE - ratio)/speed_scale;
		//printf("ratio %d %d -%2d\n", ratio, totalG[0], sample_ae_step);
	}

	if (ratio < SIMPLE_AE_STEP_BASE) {//too dark
		curExpTimeUs = curExpTimeUs * (sample_ae_step+SIMPLE_AE_STEP_BASE)/SIMPLE_AE_STEP_BASE;
		if (curExpTimeUs > sample_ae_max_time) {
			curExpTimeUs = sample_ae_max_time;
			curAgain = curAgain * (sample_ae_step+SIMPLE_AE_STEP_BASE)/SIMPLE_AE_STEP_BASE;
			if (curAgain > sample_ae_max_gain) {
				curAgain = sample_ae_max_gain;
				isReachBound = 1;
			}
		}
	} else {//too bright
		if (curAgain > sample_ae_min_gain) {
			curAgain = curAgain*SIMPLE_AE_STEP_BASE/(sample_ae_step+SIMPLE_AE_STEP_BASE);
			if (curAgain < sample_ae_min_gain)
				curAgain = sample_ae_min_gain;
		} else {
			if (curExpTimeUs > sample_ae_min_time) {
				curExpTimeUs = curExpTimeUs*SIMPLE_AE_STEP_BASE/(sample_ae_step+SIMPLE_AE_STEP_BASE);
				if (curExpTimeUs < sample_ae_min_time) {
					curExpTimeUs = sample_ae_min_time;
					isReachBound = 1;
				}
			}
		}
	}

#if 1	//Do simple AWB here
	frmCnt = 0;
	totalG[0] = 0;
	tR = 0;
	tB = 0;
	for (i = 0; i < pInfo->winYNum; i++) {
		for (j = 0; j < pInfo->winXNum; j++) {
			tR += pAEINFO_S->au16ZoneAvg[i][j][3];
			totalG[0] += pAEINFO_S->au16ZoneAvg[i][j][1];
			tB += pAEINFO_S->au16ZoneAvg[i][j][0];
		}
	}
	if (tR && tB) {
		curRgain = (totalG[0] << 10) / tR;
		curBgain = (totalG[0] << 10) / tB;
	}
	pCtr->rWb_Gain = curRgain;
	pCtr->bWb_Gain = curBgain;

	//printf("WB %d %d\n", pCtr->rWb_Gain, pCtr->bWb_Gain);
#endif
	pCtr->leExp = curExpTimeUs;
	pCtr->leGain = curAgain;
	pCtr->seExp = curExpTimeUs / 4;
	pCtr->seGain = curAgain;
	//printf("%d %d %d %d\n", pCtr->leExp, pCtr->leGain, pCtr->seExp, pCtr->seGain);


	if (sample_ae_step < STABLE_THR || isReachBound) {
		pCtr->bIsStable = 1;
	} else {
		pCtr->bIsStable = 0;
	}
	//printf("%d %d\n",pCtr->bIsStable,sample_ae_step);
}

#if 0
void SampleAE(void)
{
	SAMPLE_AE_ALGO_S tmpAeAlgoData;
	SAMPLE_AE_CTRL_S tmpAeCtrl;
	CVI_U16 i;

	//add test AE stat
	tmpAeAlgoData.frmNum = 2;
	for (i = 0; i < SIMPLE_AE_WIN_NUM; i++) {//17*15:255 windows
		tmpAeAlgoData.pR[AE_LE][i] = i*4;
		tmpAeAlgoData.pR[AE_SE][i] = i;
		tmpAeAlgoData.pG[AE_LE][i] = i*4;
		tmpAeAlgoData.pG[AE_SE][i] = i;
		tmpAeAlgoData.pB[AE_LE][i] = i*4;
		tmpAeAlgoData.pB[AE_SE][i] = i;
	}
	SAMPLE_AE_Algo(&tmpAeAlgoData, &tmpAeCtrl);
}
#endif

