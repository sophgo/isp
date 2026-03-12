/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_dis.c
 * Description:
 *
 */

// sys
#include "stdlib.h"
#include "stdio.h"
#include "stdint.h"
#include "limits.h"
#include <sys/time.h> //for gettimeofday()

// cvitek
#include "cvi_type.h"
//#include "cvi_common.h"
#include "cvi_comm_isp.h"

// mw
#include "isp_algo_dis.h"
#include "isp_algo_debug.h"
#include "cvi_ae.h"

#ifdef DIS_PC_PLATFORM
#include "dis_platform.h"
#else
#ifdef __ARM_NEON__
#include "arm_neon.h"
#endif
#endif

//#define _PC_DIS_OFF

#define CHECK_DIS_TIMING (0)
#if CHECK_DIS_TIMING
static struct timeval ttdis[20];
static CVI_U32 timingCnt;
static CVI_U32 timingTol[20];
#define MEAUSURE_DIS_T(a) gettimeofday(&ttdis[a], NULL)
#else
#define MEAUSURE_DIS_T(a)
#endif

#ifdef DIS_PC_PLATFORM
FILE * fpDump;
#endif

static CVI_S16 mvx_candidate[DIS_MAX_WINDOW_X_NUM * DIS_MAX_WINDOW_Y_NUM];
static CVI_S16 mvy_candidate[DIS_MAX_WINDOW_X_NUM * DIS_MAX_WINDOW_Y_NUM];
static CVI_S8 dis_dir_x, dis_dir_y;
static CVI_U32 change_dir_x, change_dir_y;
static CVI_S8 stable_cnt[DIS_SENSOR_NUM][2][DIS_STB_Q_S];

static DIS_RUNTIME *pDisRunTime[DIS_SENSOR_NUM];
static CVI_BOOL isDisAlgoCreate[DIS_SENSOR_NUM];

//7KB
CVI_U32 SADBUF[(2 * MAX_MVX_JITTER_RANGE + 1)];
CVI_S32 CFRMBUF[(2 * MAX_MVX_JITTER_RANGE + 1)];

CVI_U32 accPrvX[XHIST_LENGTH + 1], accPrvY[YHIST_LENGTH + 1];
CVI_U32 accCurX[XHIST_LENGTH + 1], accCurY[YHIST_LENGTH + 1];

static CVI_VOID _get_hist_status(CVI_U16 histX[][DIS_MAX_WINDOW_X_NUM][XHIST_LENGTH],
	CVI_U16 histY[][DIS_MAX_WINDOW_X_NUM][YHIST_LENGTH], WINXY_DIS_HIST *curhist, REG_DIS greg_dis);
static CVI_VOID _update_histogram(DIS_RUNTIME *runtime);
static void _cal_mv_hist(WINXY_DIS_HIST *curhist, WINXY_DIS_HIST *prvhist,
	DIS_RGN_RAWMV rgn_curmv[][DIS_MAX_WINDOW_X_NUM], REG_DIS *greg_dis);
static CVI_U32 _pick_sort_find_med_mv(VI_PIPE ViPipe,
	DIS_RGN_RAWMV rgn_curMV[][DIS_MAX_WINDOW_X_NUM], MV_STRUCT *MV_BUF, CVI_S32 curpos_wrt, REG_DIS greg_dis);
static void _BubbleSort(CVI_S16 *arr, CVI_U8 buf_length);
static void _hist_find_mv_LV(CVI_S32 prvMV, DIS_RGN_RAWMV *rgn_curmv, DIS_HIST *prvhist, DIS_HIST *curhist,
	REG_DIS *greg_dis, CVI_S32 max_jitter_range, CVI_S32 MV_STEP, CVI_U32 SAD_STEP, DIS_DIR_TYPE_E mvtype,
	CVI_BOOL is1st);
static void _do_RgnMELV(DIS_RGN_RAWMV *baseMV, DIS_HIST *prvhist, DIS_HIST *curhist, DIS_RGN_RAWMV *rgn_curmv,
	REG_DIS *greg_dis, CVI_S32 xstep, CVI_S32 ystep, CVI_U32 SAD_STEP, CVI_S32 max_mvx_range, CVI_S32 max_mvy_range,
	CVI_BOOL is1st);
static CVI_S32 _check_freq(VI_PIPE ViPipe, CVI_FLOAT x, CVI_FLOAT y);

#if CHECK_DIS_TIMING
static CVI_U32 DIS_GetUSTimeDiff(const struct timeval *before, const struct timeval *after)
{
	CVI_U32 CVI_timeDiff = 0;

	CVI_timeDiff = (after->tv_sec - before->tv_sec) * 1000000 + (after->tv_usec - before->tv_usec);

	return CVI_timeDiff;
}
#endif


static DIS_RUNTIME *getDisRuntime(VI_PIPE ViPipe)
{
	CVI_BOOL isVipipeValid = ((ViPipe >= 0) && (ViPipe < DIS_SENSOR_NUM));

	if (!isVipipeValid) {
		printf("Wrong ViPipe(%d)\n", ViPipe);
		return NULL;
	}
	return pDisRunTime[ViPipe];
}



CVI_S32 disAlgoCreate(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	DIS_RUNTIME *runtime;
	CVI_BOOL isVipipeValid;

	if (isDisAlgoCreate[ViPipe]) {
		return ret;
	}

	isVipipeValid = ((ViPipe >= 0) && (ViPipe < DIS_SENSOR_NUM));
	if (!isVipipeValid) {
		return CVI_FAILURE;
	}

	if (pDisRunTime[ViPipe] == NULL) {
		pDisRunTime[ViPipe] = (DIS_RUNTIME *)malloc(sizeof(DIS_RUNTIME));
		if (pDisRunTime[ViPipe] == CVI_NULL) {
			return CVI_FAILURE;
		}

		isDisAlgoCreate[ViPipe] = 1;
	}

	runtime = getDisRuntime(ViPipe);
	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}


	memset(runtime, 0, sizeof(DIS_RUNTIME));

	runtime->curhist = &runtime->_cur;
	runtime->prvhist = &runtime->_prv;

	runtime->hist_ready_cnt = 2;
	runtime->dis_ready_cnt = 2;

	return ret;
}

CVI_S32 disAlgoDestroy(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	CVI_BOOL isVipipeValid = ((ViPipe >= 0) && (ViPipe < DIS_SENSOR_NUM));

	if (!isVipipeValid) {
		return CVI_FAILURE;
	}

	if (isDisAlgoCreate[ViPipe]) {
		isDisAlgoCreate[ViPipe] = 0;
		if (pDisRunTime[ViPipe]) {
			free(pDisRunTime[ViPipe]);
			pDisRunTime[ViPipe] = NULL;
		}
	}
	return ret;
}

CVI_S32 disMainInit(VI_PIPE ViPipe)
{
	ISP_ALGO_LOG_INFO("+\n");
	CVI_S32 ret = CVI_SUCCESS;

#ifdef DIS_PC_PLATFORM
	fpDump = fopen("result.txt", "w");
	printf("open result.txt\n");
#endif

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 disMainUnInit(VI_PIPE ViPipe)
{
	ISP_ALGO_LOG_INFO("+\n");
	CVI_S32 ret = CVI_SUCCESS;

#ifdef DIS_PC_PLATFORM
	fclose(fpDump);
	printf("close result.txt\n");
#endif
	UNUSED(ViPipe);
	return ret;
}

struct S_DIS_FLICKER stFlicker[2];

static void dis_check_flicker(VI_PIPE ViPipe, WINXY_DIS_HIST *curhist)
{
#define D_FLK_STB_L (256 - 10)
#define D_FLK_STB_H (256 + 10)
#define MAX_LAST_Q (5)
	CVI_U32 i, j, k, inxY, inxT;
	CVI_U32 total;
	CVI_U8 inxF = stFlicker[ViPipe].curInx;
	CVI_U32 *pLuma, *pLuma0;
	CVI_U32 ratioTab[DIS_MAX_WINDOW_Y_NUM * DIS_FLK_NUM_PER_Y];
	CVI_U32 ratioTab2[DIS_MAX_WINDOW_Y_NUM * DIS_FLK_NUM_PER_Y];
	CVI_U8 isStable, isStable2, isFlick;
	CVI_FLOAT ffps;

	CVI_ISP_QueryFps(ViPipe, &ffps);

	if (ffps == 25.0) {
		stFlicker[ViPipe].ena = 1;
	} else {
		stFlicker[ViPipe].ena = 0;
		stFlicker[ViPipe].curInx = 0;
		stFlicker[ViPipe].lastQ = 0;
		stFlicker[ViPipe].flickSts = 0;
	}
	if (stFlicker[ViPipe].ena == 0)
		return;
	pLuma = &stFlicker[ViPipe].flicker[inxF][0];
	for (i = 0; i < DIS_MAX_WINDOW_Y_NUM * DIS_FLK_NUM_PER_Y; i++) {
		pLuma[i] = 0;
	}

	for (i = 0; i < DIS_MAX_WINDOW_Y_NUM; i++) {
		inxY = i * DIS_FLK_NUM_PER_Y;
		for (k = 0; k < YHIST_LENGTH; k++) {
			total = 0;
			for (j = 0; j < DIS_MAX_WINDOW_X_NUM; j++) {
				total += curhist->wHist[i][j].yhist_d[k];
			}
			inxT = (k >> DIS_FLK_Y_SHT);
			if (inxT >= DIS_FLK_NUM_PER_Y)
				inxT = DIS_FLK_NUM_PER_Y - 1;
			pLuma[inxY + inxT] += total;
		}
	}
	inxF++;
	if (inxF == DIS_FLK_NUM_FRM) {
		inxF = 0;
		pLuma0 = &stFlicker[ViPipe].flicker[0][0];
		isStable = 1;
		//check frm[0] frm[5]
		for (i = 0; i < DIS_MAX_WINDOW_Y_NUM * DIS_FLK_NUM_PER_Y; i++) {
			if (pLuma0[i]) {
				ratioTab[i] = (pLuma[i] * 256) / pLuma0[i];
			} else {
				ratioTab[i] = 256;
			}
			if (ratioTab[i] < D_FLK_STB_L || ratioTab[i] > D_FLK_STB_H) {
				isStable = 0;
			}
		}
		isStable2 = 1;
		if (isStable) {//check frm[0] frm[1]
			pLuma = &stFlicker[ViPipe].flicker[1][0];
			for (i = 0; i < DIS_MAX_WINDOW_Y_NUM * DIS_FLK_NUM_PER_Y; i++) {
				if (pLuma0[i]) {
					ratioTab2[i] = (pLuma[i] * 256) / pLuma0[i];
				} else {
					ratioTab2[i] = 256;
				}
				if (ratioTab2[i] < D_FLK_STB_L || ratioTab2[i] > D_FLK_STB_H) {
					isStable2 = 0;
				}
			}
		}
		if (isStable == 1 && isStable2 == 0) {
			isFlick = 1;
			//stFlicker[ViPipe].flickSts = isFlick;
		} else {
			isFlick = 0;
		}
		if (isFlick) {
			stFlicker[ViPipe].lastQ++;
		} else {
			stFlicker[ViPipe].lastQ--;
		}
		if (stFlicker[ViPipe].lastQ < 0) {
			stFlicker[ViPipe].lastQ = 0;
			stFlicker[ViPipe].flickSts = 0;
		} else if (stFlicker[ViPipe].lastQ > MAX_LAST_Q) {
			stFlicker[ViPipe].lastQ = MAX_LAST_Q;
			stFlicker[ViPipe].flickSts = 1;
		}
	}
	stFlicker[ViPipe].curInx = inxF;

//	if (inxF == 0) {
//		printf("Ratio %d %d %d\n", isStable, isStable2, isFlick);
//		for (i = 0; i < DIS_MAX_WINDOW_Y_NUM * DIS_FLK_NUM_PER_Y; i++) {
//			printf("%d\t%d\n", ratioTab[i], ratioTab2[i]);
//		}
//	}
}

CVI_S32 disMain(VI_PIPE ViPipe, DIS_MAIN_INPUT_S *inPtr, DIS_MAIN_OUTPUT_S *outPtr)
{
	CVI_U32 me_status = 0;
	DIS_RUNTIME *runtime = getDisRuntime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}


	MEAUSURE_DIS_T(0);
	runtime->hist_ready_cnt = (inPtr->greg_dis.reg_sw_DIS_en == 0) ? 2 : MAX(runtime->hist_ready_cnt - 1, 0);
	runtime->dis_ready_cnt = (inPtr->greg_dis.reg_sw_DIS_en == 0) ? 2 : MAX(runtime->dis_ready_cnt - 1, 0);

	CVI_BOOL me_drop_hit = 0;
	// step-1 : get latest histogram: curhist: latest histogram:

	DIS_STATS_INFO *disStt = (DIS_STATS_INFO *) ISP_PTR_CAST_VOID(inPtr->disStt);

	_get_hist_status(disStt->histX, disStt->histY, runtime->curhist, inPtr->greg_dis);
	MEAUSURE_DIS_T(1);

	if (runtime->hist_ready_cnt != 0) {
		_update_histogram(runtime);
		runtime->curpos_exe   = 0;
		runtime->curpos_wrt   = 0;
		runtime->drop_end_cnt   = 0;
		outPtr->coordiate.xcor = 0;
		outPtr->coordiate.ycor = 0;
	} else {
		// step-2 : calculate this round mv:
		dis_check_flicker(ViPipe, runtime->curhist);
		_cal_mv_hist(runtime->curhist, runtime->prvhist, runtime->rgn_curMV, &inPtr->greg_dis);
		MEAUSURE_DIS_T(2);
		// step-3 : find the cur-pair mv & return DIS drop status.
		me_status = _pick_sort_find_med_mv(ViPipe, runtime->rgn_curMV, runtime->MV_BUF,
			runtime->curpos_wrt, inPtr->greg_dis);

		if (stFlicker[ViPipe].flickSts) {
			runtime->MV_BUF[runtime->curpos_wrt].x = 0;
			runtime->MV_BUF[runtime->curpos_wrt].y = 0;
		}
		MEAUSURE_DIS_T(3);
		CVI_BOOL mvxdrop_bigdiff = (me_status & 0x00F000) >> 12;
		CVI_BOOL mvxdrop_num_sml = (me_status & 0x000F00) >> 8;
		CVI_BOOL mvydrop_bigdiff = (me_status & 0x0000F0) >> 4;
		CVI_BOOL mvydrop_num_sml = (me_status & 0x00000F) >> 0;

		//=> step-3.1 : me_drop_hit: flag to trigget drop_end_cnt.
		me_drop_hit = ((mvxdrop_bigdiff || mvxdrop_num_sml) || (mvydrop_bigdiff || mvydrop_num_sml));
		runtime->drop_end_cnt = me_drop_hit ? 5 : MAX(0, runtime->drop_end_cnt - 1);

		// step 4 : DIS Is Ready to Go:
		if (runtime->dis_ready_cnt == 0) {
			_check_freq(
				ViPipe, runtime->MV_BUF[runtime->curpos_wrt].x, runtime->MV_BUF[runtime->curpos_wrt].y);

			if (mvydrop_bigdiff) {
				runtime->MV_BUF[runtime->curpos_wrt].y
					= runtime->MV_BUF[(runtime->curpos_wrt + QUEBUF_LENGTH - 1) % QUEBUF_LENGTH].y
					/ 2;
			}

			runtime->accsumX += runtime->MV_BUF[runtime->curpos_wrt].x;
			runtime->accsumY += runtime->MV_BUF[runtime->curpos_wrt].y;

			//move to center
#if 1
			if (runtime->accsumX > inPtr->greg_dis.reg_MARGIN_INI_R / 2) {
				runtime->accsumX--;
			} else if (runtime->accsumX < (0 - inPtr->greg_dis.reg_MARGIN_INI_L) / 2) {
				runtime->accsumX++;
			}
			if (runtime->accsumY > inPtr->greg_dis.reg_MARGIN_INI_D / 2) {
				runtime->accsumY--;
			} else if (runtime->accsumY < (0 - inPtr->greg_dis.reg_MARGIN_INI_U) / 2) {
				runtime->accsumY++;
			}
#endif
			if (runtime->accsumX < (0 - inPtr->greg_dis.reg_MARGIN_INI_L)) {
				runtime->accsumX = (0 - inPtr->greg_dis.reg_MARGIN_INI_L);
			}
			if (runtime->accsumX > (inPtr->greg_dis.reg_MARGIN_INI_R)) {
				runtime->accsumX = (inPtr->greg_dis.reg_MARGIN_INI_R);
			}
			if (runtime->accsumY < (0 - inPtr->greg_dis.reg_MARGIN_INI_U)) {
				runtime->accsumY = (0 - inPtr->greg_dis.reg_MARGIN_INI_U);
			}
			if (runtime->accsumY > (inPtr->greg_dis.reg_MARGIN_INI_D)) {
				runtime->accsumY = (inPtr->greg_dis.reg_MARGIN_INI_D);
			}
			runtime->accsum[runtime->curpos_wrt].x = runtime->accsumX;
			runtime->accsum[runtime->curpos_wrt].y = runtime->accsumY;

			outPtr->coordiate.xcor = runtime->accsum[runtime->curpos_wrt].x;
			outPtr->coordiate.ycor = runtime->accsum[runtime->curpos_wrt].y;
			runtime->curpos_exe = (runtime->curpos_exe + 1) % QUEBUF_LENGTH;
		} else {//(runtime->dis_ready_cnt == 0)
			//No offset
			outPtr->coordiate.xcor = 0;
			outPtr->coordiate.ycor = 0;
		}

		// update cur result to previous:
		_update_histogram(runtime);

		runtime->curpos_wrt = (runtime->curpos_wrt + 1) % QUEBUF_LENGTH;
	}

#if USE_CAM_VDO
	CVI_FLOAT fx, fy;
	CVI_S16 sX, sY;

	sX = pBinOut[binDataCnt].coordiate.xcor - inPtr->greg_dis.reg_MARGIN_INI_L;
	sY = pBinOut[binDataCnt].coordiate.ycor - inPtr->greg_dis.reg_MARGIN_INI_U;
#if 1
	printf("Ori\t%d\t%d\t", sX, sY);
	printf("%d\t%d\t", outPtr->coordiate.xcor, outPtr->coordiate.ycor);
#else
	static CVI_S16 prvsX, prvsY;
	static CVI_S16 prvoX, prvoY;

	printf("Ori\t%d\t%d\t", sX - prvsX, sY - prvsY);
	printf("%d\t%d\t", outPtr->coordiate.xcor - prvoX, outPtr->coordiate.ycor - prvoY);
	prvsX = sX;
	prvsY = sY;
	prvoX = outPtr->coordiate.xcor;
	prvoY = outPtr->coordiate.ycor;
#endif
	fx = inPtr->greg_dis.reg_MARGIN_INI_L - pBinOut[binDataCnt].coordiate.xcor;
	fy = inPtr->greg_dis.reg_MARGIN_INI_U - pBinOut[binDataCnt].coordiate.ycor;
	//printf("%d\n",binConfig.cropRatio);

#ifdef _PC_DIS_OFF
#else
	fx += outPtr->coordiate.xcor;
	fy += outPtr->coordiate.ycor;
#endif
	printf("f\t%0.1f\t%0.1f\t", fx, fy);

#if 0//1:keep the ori dis moving @cam.mp4,just crop
	fx = 0;
	fy = 0;
#endif

	outPtr->coordiate.xcor = fx;
	outPtr->coordiate.ycor = fy;
	binDataCnt++;
#endif

	outPtr->coordiate.xcor += inPtr->greg_dis.reg_MARGIN_INI_L;
	outPtr->coordiate.ycor += inPtr->greg_dis.reg_MARGIN_INI_U;

#ifdef DIS_PC_PLATFORM
	CVI_U8 ii, jj;

	printf(" %06x ", me_status);
	fprintf(fpDump, "%04d ", curInx);
	//	fprintf(fpDump, "(%d, %d) ", outPtr->coordiate.xcor, outPtr->coordiate.ycor);
	fprintf(fpDump, "%01d\t%01d\t%02d\t%02d\t%01d\t%02d\t", runtime->hist_ready_cnt, runtime->dis_ready_cnt,
		runtime->curpos_exe, runtime->curpos_wrt, me_drop_hit, runtime->drop_end_cnt);
	fprintf(fpDump, "%06x\t", me_status);
	fprintf(fpDump, "MV_(\t%.0f\t%.0f\t)",
		runtime->MV_BUF[(runtime->curpos_wrt + QUEBUF_LENGTH - 1) % QUEBUF_LENGTH].x,
		runtime->MV_BUF[(runtime->curpos_wrt + QUEBUF_LENGTH - 1) % QUEBUF_LENGTH].y);
	fprintf(fpDump, "GMV_(\t%.2f\t%.2f\t)",
		runtime->GMV[(runtime->curpos_exe + QUEBUF_LENGTH - 1) % QUEBUF_LENGTH].x,
		runtime->GMV[(runtime->curpos_exe + QUEBUF_LENGTH - 1) % QUEBUF_LENGTH].y);
	fprintf(fpDump, "OFF_(\t%.2f\t%.2f\t)\t",
		runtime->OFFSET[(runtime->curpos_exe + QUEBUF_LENGTH - 1) % QUEBUF_LENGTH].x,
		runtime->OFFSET[(runtime->curpos_exe + QUEBUF_LENGTH - 1) % QUEBUF_LENGTH].y);
	for (ii = 0; ii < DIS_MAX_WINDOW_Y_NUM; ii++) {
		for (jj = 0; jj < DIS_MAX_WINDOW_X_NUM; jj++) {
			fprintf(fpDump, "%d%d_(%d:%d%d, %d:%d%d)\t", ii, jj, runtime->rgn_curMV[ii][jj].x,
				runtime->rgn_curMV[ii][jj].xcfrmflag, runtime->rgn_curMV[ii][jj].xflatness,
				runtime->rgn_curMV[ii][jj].y, runtime->rgn_curMV[ii][jj].ycfrmflag,
				runtime->rgn_curMV[ii][jj].yflatness);
		}
	}
	fprintf(fpDump, "\n");
	if (curInx == 1) {
		//exit(0);
	}
#endif
	MEAUSURE_DIS_T(4);

#if CHECK_DIS_TIMING

	timingCnt++;
	if (timingCnt > 30) {
		timingTol[0] += DIS_GetUSTimeDiff(&ttdis[0], &ttdis[1]);
		timingTol[1] += DIS_GetUSTimeDiff(&ttdis[1], &ttdis[2]);
		timingTol[2] += DIS_GetUSTimeDiff(&ttdis[2], &ttdis[3]);
		timingTol[3] += DIS_GetUSTimeDiff(&ttdis[3], &ttdis[4]);
		printf("%d\n", timingCnt);
		printf("TT1:%d\n", timingTol[0] / (timingCnt - 30));
		printf("TT2:%d\n", timingTol[1] / (timingCnt - 30));
		printf("TT3:%d\n", timingTol[2] / (timingCnt - 30));
		printf("TT4:%d\n", timingTol[3] / (timingCnt - 30));
	}

#endif

	return 0;
}

static CVI_VOID _get_hist_status(CVI_U16 histX[][DIS_MAX_WINDOW_X_NUM][XHIST_LENGTH],
	CVI_U16 histY[][DIS_MAX_WINDOW_X_NUM][YHIST_LENGTH], WINXY_DIS_HIST *curhist, REG_DIS greg_dis)
{
	CVI_U8 i, j;
	CVI_U16 widx, hidx;
	//clear all buffers
	//memset(&curhist[0][0], 0, sizeof(DIS_HIST) * 9);
#ifndef __ARM_NEON__
	// normalize histogram status to smaller scale:
	for (j = 0; j < DIS_MAX_WINDOW_Y_NUM; j++) {
		for (i = 0; i < DIS_MAX_WINDOW_X_NUM; i++) {
			for (widx = 0; widx < greg_dis.reg_XHIST_LENGTH; widx++) {
				curhist->wHist[j][i].xhist_d[widx] = (histX[j][i][widx] >> greg_dis.reg_HIST_NORMX);
			}
			for (hidx = 0; hidx < greg_dis.reg_YHIST_LENGTH; hidx++) {
				curhist->wHist[j][i].yhist_d[hidx] = (histY[j][i][hidx] >> greg_dis.reg_HIST_NORMY);
			}
		}
	}
#else

	int16x8_t xshift = vdupq_n_s16(-greg_dis.reg_HIST_NORMX);
	int16x8_t yshift = vdupq_n_s16(-greg_dis.reg_HIST_NORMY);
	uint16x8_t inV;
	uint16x8_t normalizedValue;

	for (j = 0; j < DIS_MAX_WINDOW_Y_NUM; j++) {
		for (i = 0; i < DIS_MAX_WINDOW_X_NUM; i++) {

			for (widx = 0; widx < greg_dis.reg_XHIST_LENGTH; widx += 8) {
				inV = vld1q_u16(&histX[j][i][widx]);
				normalizedValue = vshlq_u16(inV, xshift);
				vst1q_u16(&curhist->wHist[j][i].xhist_d[widx], normalizedValue);
			}
			for (hidx = 0; hidx < greg_dis.reg_YHIST_LENGTH; hidx += 8) {
				inV = vld1q_u16(&histY[j][i][hidx]);
				normalizedValue = vshlq_u16(inV, yshift);
				vst1q_u16(&curhist->wHist[j][i].yhist_d[hidx], normalizedValue);
			}
		}
	}
#endif
}

static CVI_VOID _update_histogram(DIS_RUNTIME *runtime)
{
	//memcpy(dst, src, sizeof(DIS_HIST) * DIS_MAX_WINDOW_X_NUM * DIS_MAX_WINDOW_Y_NUM);
	WINXY_DIS_HIST *pTmp;

	pTmp = runtime->prvhist;
	runtime->prvhist = runtime->curhist;
	runtime->curhist = pTmp;
}

static void _cal_mv_hist(WINXY_DIS_HIST *curhist, WINXY_DIS_HIST *prvhist,
	DIS_RGN_RAWMV rgn_curmv[][DIS_MAX_WINDOW_X_NUM], REG_DIS *greg_dis)
{
	CVI_S32 xstep_lv1 = 8;
	CVI_S32 ystep_lv1 = 8;
	CVI_S32 xstep_lv2 = 1;
	CVI_S32 ystep_lv2 = 1;

	CVI_U32 SAD_STEP_LV1 = 2;
	CVI_U32 SAD_STEP_LV2 = 2;

#ifdef DIS_PC_PLATFORM
	dumpHist(curhist);
#endif
	// get 3x3 region raw mv resuts:
	for (CVI_S32 j = 0; j < DIS_MAX_WINDOW_Y_NUM; j++) {
		for (CVI_S32 i = 0; i < DIS_MAX_WINDOW_X_NUM; i++) {

			DIS_RGN_RAWMV rgn_init = {0};

			_do_RgnMELV(&rgn_init, &prvhist->wHist[j][i], &curhist->wHist[j][i], &rgn_curmv[j][i], greg_dis,
				xstep_lv1, ystep_lv1, SAD_STEP_LV1, greg_dis->reg_MAX_MVX_JITTER_RANGE,
				greg_dis->reg_MAX_MVY_JITTER_RANGE, 1);
			_do_RgnMELV(&rgn_curmv[j][i], &prvhist->wHist[j][i], &curhist->wHist[j][i], &rgn_curmv[j][i],
				greg_dis, xstep_lv2, ystep_lv2, SAD_STEP_LV2, (xstep_lv1 << 1), (ystep_lv1 << 1), 0);
		}
	}
}

static void _do_RgnMELV(DIS_RGN_RAWMV *baseMV, DIS_HIST *prvhist, DIS_HIST *curhist, DIS_RGN_RAWMV *rgn_curmv,
	REG_DIS *greg_dis, CVI_S32 xstep, CVI_S32 ystep, CVI_U32 SAD_STEP, CVI_S32 max_mvx_range, CVI_S32 max_mvy_range,
	CVI_BOOL is1st)
{
	//update region mv & confirm status
	CVI_S32 tmpVal = 0;

	_hist_find_mv_LV(
		baseMV->x, rgn_curmv, prvhist, curhist, greg_dis, max_mvx_range, xstep, SAD_STEP, DIS_DIR_X, is1st);
	_hist_find_mv_LV(
		baseMV->y, rgn_curmv, prvhist, curhist, greg_dis, max_mvy_range, ystep, SAD_STEP, DIS_DIR_Y, is1st);
	if (is1st == 0) {
		CVI_S32 xcfrmSAD = (rgn_curmv->xavgSAD - rgn_curmv->xbestSAD) / MAX(1, greg_dis->reg_cfrmsad_norm);
		CVI_S32 ycfrmSAD = (rgn_curmv->yavgSAD - rgn_curmv->ybestSAD) / MAX(1, greg_dis->reg_cfrmsad_norm);

		tmpVal = MAX(0, abs(rgn_curmv->y) - greg_dis->reg_cfrmgain_xcoring);
		CVI_S32 xcfrmMod_gain
			= MIN(greg_dis->reg_cfrmsad_max, tmpVal * (ycfrmSAD / MAX(1, greg_dis->reg_cfrmsad_norm)));

		tmpVal = MAX(0, abs(rgn_curmv->x) - greg_dis->reg_cfrmgain_ycoring);
		CVI_S32 ycfrmMod_gain
			= MIN(greg_dis->reg_cfrmsad_max, tmpVal * (xcfrmSAD / MAX(1, greg_dis->reg_cfrmsad_norm)));

		//by SAD to check confidence of mv results:
		CVI_S32 xcfrmSAD_mod = MAX(0,
			(rgn_curmv->xbestSAD / MAX(1, greg_dis->reg_cfrmsad_norm))
				- ((xcfrmMod_gain * MAX(0, xcfrmSAD - greg_dis->reg_cfrmsad_xcoring)) / 16)
				- (8 * xcfrmSAD >> 4));
		CVI_S32 ycfrmSAD_mod = MAX(0,
			(rgn_curmv->ybestSAD / MAX(1, greg_dis->reg_cfrmsad_norm))
				- ((ycfrmMod_gain * MAX(0, ycfrmSAD - greg_dis->reg_cfrmsad_ycoring)) / 16)
				- (8 * ycfrmSAD >> 4));

		rgn_curmv->xcfrmflag = (xcfrmSAD_mod <= greg_dis->reg_cfrmflag_xthd);
		rgn_curmv->ycfrmflag = (ycfrmSAD_mod <= greg_dis->reg_cfrmflag_ythd);
	}
}

static void _hist_find_mv_LV(CVI_S32 prvMV, DIS_RGN_RAWMV *rgn_curmv, DIS_HIST *prvhist, DIS_HIST *curhist,
	REG_DIS *greg_dis, CVI_S32 max_jitter_range, CVI_S32 MV_STEP, CVI_U32 SAD_STEP, DIS_DIR_TYPE_E mvtype,
	CVI_BOOL is1st)
{//per 12.5 us

	CVI_S32 hist_length, block_length;
	CVI_U32 hist_cplx_thd;
	CVI_U32 *pAccPrv, *pAccCur;
	CVI_U16 *curHistPtr, *preHistPtr;
	CVI_S32 i;
	CVI_BOOL isDoCplx;
	CVI_S32 minPrvMv, maxPrvMv;

	if (mvtype == DIS_DIR_X) {
		hist_length = greg_dis->reg_XHIST_LENGTH;
		block_length = greg_dis->reg_BLOCK_LENGTH_X;
		hist_cplx_thd = greg_dis->reg_xhist_cplx_thd;
		pAccPrv = accPrvX;
		pAccCur = accCurX;
	} else {
		hist_length = greg_dis->reg_YHIST_LENGTH;
		block_length = greg_dis->reg_BLOCK_LENGTH_Y;
		hist_cplx_thd = greg_dis->reg_yhist_cplx_thd;
		pAccPrv = accPrvY;
		pAccCur = accCurY;
	}

	CVI_S32 HALF_CNTS  = max_jitter_range / MV_STEP;

	// Get Initial Calculate SAD Position:
	CVI_S32 f2_init = (hist_length >> 1) - (block_length >> 1);//prv
	CVI_S32 f3_init;//cur
	CVI_U16 inxCur, inxPrv;
	CVI_S32 MV;
	CVI_U32 tmpSAD;
	CVI_S32 curHistVal, preHistVal, preHistLastVal;
	CVI_U32 f2_cplx;
	CVI_S32 SADdiff;
	CVI_S32 avgSAD_prv, avgSAD_cur;

	minPrvMv = HALF_CNTS * MV_STEP - f2_init;
	maxPrvMv = f2_init - HALF_CNTS * MV_STEP;
	prvMV = LIMIT_RANGE_DIS(prvMV, minPrvMv, maxPrvMv);
	f3_init = f2_init + prvMV;//cur

	//Step 1: cal avg(DC)-SAD for each mv: in order to calculate AC-SAD
	if (mvtype == DIS_DIR_X) {
		curHistPtr = curhist->xhist_d;
		preHistPtr = prvhist->xhist_d;
	} else {
		curHistPtr = curhist->yhist_d;
		preHistPtr = prvhist->yhist_d;
	}
	if (is1st) {
		pAccPrv[0] = 0;
		pAccCur[0] = 0;
		for (i = 1; i < hist_length + 1; i++) {
			pAccPrv[i] = pAccPrv[i - 1] + preHistPtr[i - 1];
			pAccCur[i] = pAccCur[i - 1] + curHistPtr[i - 1];
		}
	}

	// calculate AC-SAD  by introducing avgSAD:
	inxPrv = f2_init;
	avgSAD_prv = (pAccPrv[inxPrv + block_length] - pAccPrv[inxPrv]);
	f2_cplx = 0;
	isDoCplx = 1;
	for (CVI_S32 sridx = -HALF_CNTS; sridx <= HALF_CNTS; sridx++) {
		MV = sridx * MV_STEP;
		tmpSAD = 0;
		inxCur = f3_init + MV;
		// calculate complex value for each mv candidate:

		avgSAD_cur = (pAccCur[inxCur + block_length] - pAccCur[inxCur]);
		SADdiff = (avgSAD_cur - avgSAD_prv) / block_length;

		if (mvtype == DIS_DIR_X) {
			curHistPtr = curhist->xhist_d + inxCur;
			preHistPtr = prvhist->xhist_d + f2_init;
		} else {
			curHistPtr = curhist->yhist_d + inxCur;
			preHistPtr = prvhist->yhist_d + f2_init;
		}

		curHistVal = *(curHistPtr);
		curHistPtr += SAD_STEP;
		preHistVal = *(preHistPtr);
		preHistPtr += SAD_STEP;
		tmpSAD += abs((preHistVal - curHistVal + SADdiff));

		// sridx indicates shit value
		for (CVI_S32 idx = SAD_STEP; idx < block_length; idx += SAD_STEP) {
			curHistVal = *(curHistPtr);
			curHistPtr += SAD_STEP;

			preHistLastVal = preHistVal;
			preHistVal = *(preHistPtr);
			preHistPtr += SAD_STEP;
			if (isDoCplx) {
				f2_cplx += abs(preHistVal - preHistLastVal);
			}

			tmpSAD += abs((preHistVal - curHistVal + SADdiff));
		}
		isDoCplx = 0;

		// Save Raw SAD
		// More Modification on SADs later on...
		SADBUF[sridx + HALF_CNTS] = tmpSAD;
	}

	// Get SAD confirm value: SAD local min value & trend:
	// PS. SAD may has multiple candidate issue:
	// TODO@ST Check why not bit-true in 2nd round
	CVI_S32 maxSADval = -10000000;
	CVI_S32 minSADval = 0x7FFFFFFF;
	CVI_S32 avgSADval = 0, sad_C;
	//CVI_S32 inx_L, inx_R;
	//CVI_S32 sad_L, sad_R, sad_C;
	CVI_S32 bestIdx = 0;
	CVI_U32 cplxval = 0;

	for (CVI_S32 idx = 0; idx < 2 * HALF_CNTS + 1; idx++) {
		sad_C = SADBUF[idx];
		if (sad_C < minSADval) {
			minSADval = sad_C;
			bestIdx = idx;
		}
		if (sad_C > maxSADval) {
			maxSADval = sad_C;
		}
		avgSADval += sad_C;
	}
	avgSADval *= SAD_STEP;
	maxSADval *= SAD_STEP;
	minSADval *= SAD_STEP;
	avgSADval /= (2 * HALF_CNTS + 1);
	cplxval = SAD_STEP * f2_cplx;

	if (mvtype == DIS_DIR_X) {//mvx
		rgn_curmv->x = prvMV + (bestIdx - HALF_CNTS) * MV_STEP;

		// both are complex:
		if (cplxval < hist_cplx_thd) {
			rgn_curmv->xflatness = true;
		} else {
			rgn_curmv->xflatness = false;
		}
		rgn_curmv->xavgSAD  = avgSADval;
		rgn_curmv->xbestSAD = minSADval;
		rgn_curmv->xMaxSAD = maxSADval;
	} else {
		rgn_curmv->y = prvMV + (bestIdx - HALF_CNTS) * MV_STEP;

		// both are complex:
		if (cplxval < hist_cplx_thd) {
			rgn_curmv->yflatness = true;
		} else {
			rgn_curmv->yflatness = false;
		}
		rgn_curmv->yavgSAD  = avgSADval;
		rgn_curmv->ybestSAD = minSADval;
		rgn_curmv->yMaxSAD = maxSADval;
	}
}

static CVI_S16 _decide_final_mv(CVI_S16 *mv_candidate, CVI_U8 buf_length, CVI_U8 *me_drop_status, REG_DIS greg_dis)
{
#define FORCE_STB_SMALL_CNT (2)

	CVI_S16 mv = 0;
	CVI_S16 avgmv = 0;
	CVI_S16 medmv = 0;
	CVI_U8 idx, maxdif_idx, smallCnt = 0;
	CVI_BOOL mvdiffbig = false;
	CVI_BOOL buf_length_sml = false;
	CVI_S16 mvc;
	CVI_U32 tmp_maxval = 0;

	if (buf_length >= 3) {
		for (idx = 0; idx < buf_length; idx++) {
			mvc = mv_candidate[idx];
			avgmv += mvc;
			if (abs(mvc) < 2) {
				smallCnt++;
			}
		}

		avgmv /= buf_length;
		// sorintg from smal -> big:
		_BubbleSort(mv_candidate, buf_length);
		medmv = mv_candidate[buf_length / 2];

		// find mv variance skip 1 big err:
		maxdif_idx = 0;

		// 1. find max diff mv to medmv.
		for (idx = 0; idx < buf_length; idx++) {
			if (tmp_maxval < (CVI_U32)abs(medmv - mv_candidate[idx])) {
				tmp_maxval = abs(medmv - mv_candidate[idx]);
				maxdif_idx = idx;
			}
		}

		//2. Find 2nd max diff to medmv.
		tmp_maxval = 0;
		for (idx = 0; idx < buf_length; idx++) {
			if (idx != maxdif_idx) {
				if (tmp_maxval < (CVI_U32)abs(medmv - mv_candidate[idx])) {
					tmp_maxval = abs(medmv - mv_candidate[idx]);
				}
			}
		}

		if (tmp_maxval > MAX(greg_dis.reg_tmp_maxval_diff_thd,
			(greg_dis.reg_tmp_maxval_gain * abs(medmv)) / 32)) {
			mvdiffbig = true;
			mv = 0;
		} else {
			mv = medmv;
		}
		if (smallCnt >= FORCE_STB_SMALL_CNT) {
			mv = 0;
		}
	} else {
		// ME no useful result, return mv = 0;
		buf_length_sml = true;
		mv = 0;
	}

	*me_drop_status = (mvdiffbig << 4 | buf_length_sml);

	return mv;
}

static CVI_U32 _pick_sort_find_med_mv(VI_PIPE ViPipe,
	DIS_RGN_RAWMV rgn_curMV[][DIS_MAX_WINDOW_X_NUM], MV_STRUCT *MV_BUF, CVI_S32 curpos_wrt, REG_DIS greg_dis)
{
	CVI_U32 me_status;
	CVI_U8 xidx = 0;
	CVI_U8 yidx = 0;
	CVI_U8 i, j, stb_cnt;
	CVI_BOOL x_cur_flat, x_cur_cfrm, y_cur_flat, y_cur_cfrm;

	for (j = 0; j < DIS_MAX_WINDOW_Y_NUM; j++) {
		for (i = 0; i < DIS_MAX_WINDOW_X_NUM; i++) {
			x_cur_flat = rgn_curMV[j][i].xflatness;
			x_cur_cfrm = rgn_curMV[j][i].xcfrmflag;
			if (!x_cur_flat && x_cur_cfrm) {
				mvx_candidate[xidx] = rgn_curMV[j][i].x;
				xidx++;
			}

			y_cur_flat = rgn_curMV[j][i].yflatness;
			y_cur_cfrm = rgn_curMV[j][i].ycfrmflag;
			if (!y_cur_flat && y_cur_cfrm) {
				mvy_candidate[yidx] = rgn_curMV[j][i].y;
				yidx++;
			}
		}
	}

	// final mv rule:
	// 1. do not refer flatness region mv.
	// 2. do not refer un-confirm region mv.
	// 3. accpeted mv candidate diff >= thd, keep mv = ( 0, 0) this round.
	// 4. get histogram.

	MV_STRUCT mv;
	CVI_U8 mvx_drop_status, mvy_drop_status;

	mv.x = _decide_final_mv(mvx_candidate, xidx, &mvx_drop_status, greg_dis);
	mv.y = _decide_final_mv(mvy_candidate, yidx, &mvy_drop_status, greg_dis);
	for (j = 0; j < 2; j++) {
		for (i = DIS_STB_Q_S - 1; i > 0 ; i--) {
			stable_cnt[ViPipe][j][i] = stable_cnt[ViPipe][j][i - 1];
		}
		if (j == 0 && mv.x == 0) {
			stable_cnt[ViPipe][j][0] = 1;
		} else if (j == 1 && mv.y == 0) {
			stable_cnt[ViPipe][j][0] = 1;
		} else {
			stable_cnt[ViPipe][j][0] = 0;
		}
		stb_cnt = 0;
		for (i = 0 ; i < DIS_STB_Q_S; i++) {
			if (stable_cnt[ViPipe][j][i] == 1) {
				stb_cnt++;
			}
		}
		if (j == 0 && stb_cnt >= 3) {
			mv.x = 0;
		} else	if (j == 1 && stb_cnt >= 3) {
			mv.y = 0;
		}
	}

	#ifdef DIS_PC_PLATFORM
	printf("mv\t%.0f\t%.0f\t", mv.x, mv.y);
	#endif
	me_status = (xidx << 20 | yidx << 16 | mvx_drop_status << 8 | mvy_drop_status);

	MV_BUF[curpos_wrt] = mv;

	return me_status;
}


static void _BubbleSort(CVI_S16 *arr, CVI_U8 buf_length)
{
	CVI_S16 tmp;
	CVI_U8 i, j;

	for (i = buf_length - 1; i > 0; i--) {
		for (j = 0; j <= i - 1; j++) {
			if (arr[j] > arr[j + 1]) {
				tmp = arr[j];
				arr[j] = arr[j + 1];
				arr[j + 1] = tmp;
			}
		}
	}
}

static CVI_S32 _check_freq(VI_PIPE ViPipe, CVI_FLOAT x, CVI_FLOAT y)
{
#define HIGH_FREQ_THR (4)
#define HIGH_FREQ_RANGE_ADJ_GAIN (1.5)
#define HIGH_FREQ_MIN_STEP (4)
	CVI_U8 i, x_cnt, y_cnt, isHighFreqX = 0, isHighFreqY = 0;
	CVI_U32 tmpx;
	DIS_RUNTIME *runtime = getDisRuntime(ViPipe);
	CVI_FLOAT minX = 1024, minY = 1024, maxX = -1024, maxY = -1024;

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}


	change_dir_x = change_dir_x << 1;
	if (dis_dir_x <= 0) {
		if (x >= 1) {
			dis_dir_x = 1;
		}
	} else if (dis_dir_x == 1) {
		if (x <= -1) {
			dis_dir_x = -1;
			change_dir_x = change_dir_x | 0x1;
		}
	}

	change_dir_y = change_dir_y << 1;
	if (dis_dir_y <= 0) {
		if (y >= 1) {
			dis_dir_y = 1;
		}
	} else if (dis_dir_y == 1) {
		if (y <= -1) {
			dis_dir_y = -1;
			change_dir_y = change_dir_y | 0x1;
		}
	}

	tmpx = change_dir_x;
	x_cnt = 0;
	for (i = 0; i < 32; i++) {
		if (tmpx & 0x1) {
			x_cnt++;
		}
		tmpx = tmpx >> 1;
	}
	tmpx = change_dir_y;
	y_cnt = 0;
	for (i = 0; i < 32; i++) {
		if (tmpx & 0x1) {
			y_cnt++;
		}
		tmpx = tmpx >> 1;
	}

	if (x_cnt >= HIGH_FREQ_THR) {
		isHighFreqX = 1;
	}
	if (y_cnt >= HIGH_FREQ_THR) {
		isHighFreqY = 1;
	}

	if (isHighFreqX || isHighFreqY) {
		for (i = 0; i < QUEBUF_LENGTH; i++) {
			if (i != runtime->curpos_wrt) {
				if (minX > runtime->MV_BUF[i].x) {
					minX = runtime->MV_BUF[i].x;
				}
				if (maxX < runtime->MV_BUF[i].x) {
					maxX = runtime->MV_BUF[i].x;
				}
				if (minY > runtime->MV_BUF[i].y) {
					minY = runtime->MV_BUF[i].y;
				}
				if (maxY < runtime->MV_BUF[i].y) {
					maxY = runtime->MV_BUF[i].y;
				}
			}
		}
		if (isHighFreqX) {//maxX >= 1, minX <= -1
			maxX = maxX * HIGH_FREQ_RANGE_ADJ_GAIN;
			minX = minX * HIGH_FREQ_RANGE_ADJ_GAIN;
			maxX = MAX2(maxX, HIGH_FREQ_MIN_STEP);
			minX = MIN2(minX, -HIGH_FREQ_MIN_STEP);
			x = LIMIT_RANGE_DIS(x, minX, maxX);
			runtime->MV_BUF[runtime->curpos_wrt].x = x;
		}
		if (isHighFreqY) {
			maxY = maxY * HIGH_FREQ_RANGE_ADJ_GAIN;
			minY = minY * HIGH_FREQ_RANGE_ADJ_GAIN;
			maxY = MAX2(maxY, HIGH_FREQ_MIN_STEP);
			minY = MIN2(minY, -HIGH_FREQ_MIN_STEP);
			y = LIMIT_RANGE_DIS(y, minY, maxY);
			runtime->MV_BUF[runtime->curpos_wrt].y = y;
		}
	}

#if 0
	char s[36];

	itoa(change_dir_x, s, 2);
	printf("%s\t%d\t", s, x_cnt);
	itoa(change_dir_y, s, 2);
	printf("%s\t%d\t", s, y_cnt);
	printf("H:%d\t%d\t", isHighFreqX, isHighFreqY);
#endif
	return CVI_SUCCESS;
}
