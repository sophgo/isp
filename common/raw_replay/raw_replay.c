/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: raw_replay.c
 * Description:
 *
 */

#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <semaphore.h>
#include "inttypes.h"

#include "cvi_ae.h"
#include "cvi_awb.h"
#include "cvi_base.h"

#include "../sample/common/sample_comm.h"
#include "3A_internal.h"
#include "raw_replay.h"
#include "teaisp_raw_test.h"

#if defined(__GNUC__) && defined(__riscv)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation" /* Or  "-Wformat-overflow"  */
#endif

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MINMAX(a, min, max) (MIN(MAX((a), (min)), (max)))

#define _BUFF_LEN  128
#define LOGOUT(fmt, arg...) printf("[RAW_REPLAY] %s,%d: " fmt, __func__, __LINE__, ##arg)

#define ABORT_IF(EXP)                  \
	do {                               \
		if (EXP) {                     \
			LOGOUT("abort: "#EXP"\n"); \
			abort();                   \
		}                              \
	} while (0)

#define RETURN_FAILURE_IF(EXP)         \
	do {                               \
		if (EXP) {                     \
			LOGOUT("fail: "#EXP"\n");  \
			return CVI_FAILURE;        \
		}                              \
	} while (0)

#define GOTO_FAIL_IF(EXP, LABEL)       \
	do {                               \
		if (EXP) {                     \
			LOGOUT("fail: "#EXP"\n");  \
			goto LABEL;                \
		}                              \
	} while (0)

#define WARN_IF(EXP)                   \
	do {                               \
		if (EXP) {                     \
			LOGOUT("warn: "#EXP"\n");  \
		}                              \
	} while (0)

#define ERROR_IF(EXP)                  \
	do {                               \
		if (EXP) {                     \
			LOGOUT("error: "#EXP"\n"); \
		}                              \
	} while (0)

#define MAX_GET_YUV_TIMEOUT (1000)
#define SEC_TO_USEC 1000000

typedef struct {
	CVI_BOOL bEnable;
	CVI_U8 src;
	CVI_S32 pipeOrGrp;
	CVI_S32 chn;
	VIDEO_FRAME_INFO_S videoFrame;
	CVI_S32 yuvIndex;
	sem_t sem;
	pthread_t getYuvTid;
	CVI_BOOL bGetYuvThreadEnabled;
} GET_YUV_CTX_S;

typedef struct {
	VI_PIPE ViPipe;
	RAW_REPLAY_INFO *pRawHeader;
	CVI_U32 u32TotalFrame;
	CVI_U32 u32RawFrameSize;
	RECT_S stRoiRect;
	CVI_U32 u32RoiTotalFrame;
	CVI_U32 u32RoiRawFrameSize;

	VB_POOL PoolID;
	VB_BLK blk;
	CVI_U32 u32BlkSize;
	CVI_U64 u64PhyAddr[2];
	CVI_U8 *pu8VirAddr[2];
	CVI_U64 u64RoiPhyAddr[2];
	CVI_U8 *pu8RoiVirAddr[2];

	pthread_t rawReplayTid;
	CVI_BOOL bRawReplayThreadEnabled;
	CVI_BOOL bRawReplayThreadSuspend;

	CVI_BOOL bUseDMA;
	CVI_CDMA_2D_S stDMAParam[2];
	CVI_U32 u32CtrlFlag;
	CVI_BOOL bSuspend;

	CVI_BOOL isRawReplayReady;
	VI_DEV_TIMING_ATTR_S timingAttr;

	GET_YUV_CTX_S stGetYuvCtx;
} RAW_REPLAY_CTX_S;

static RAW_REPLAY_CTX_S *pRawReplayCtx;

static void get_current_awb_info(ISP_MWB_ATTR_S *pstMwbAttr);
static void update_awb_config(const ISP_MWB_ATTR_S *pstMwbAttr, ISP_OP_TYPE_E eType);
static void get_current_ae_info(ISP_EXP_INFO_S *pstExpInfo);
static void update_ae_config(const ISP_EXP_INFO_S *pstExpInfo, ISP_OP_TYPE_E eType);
static void apply_raw_header(ISP_MWB_ATTR_S *pstMwbAttr, ISP_EXP_INFO_S *pstExpInfo, CVI_U32 index);

static void raw_replay_use_dma(ISP_MWB_ATTR_S *pstMwbAttr, ISP_EXP_INFO_S *pstExpInfo,
				VIDEO_FRAME_INFO_S *pstVideoFrame);
static void raw_replay_use_vb(ISP_MWB_ATTR_S *pstMwbAttr, ISP_EXP_INFO_S *pstExpInfo,
				VIDEO_FRAME_INFO_S *pstVideoFrame);
static void raw_replay_singlefrmae(ISP_MWB_ATTR_S *pstMwbAttr, ISP_EXP_INFO_S *pstExpInfo,
				VIDEO_FRAME_INFO_S *pstVideoFrame);
static void *get_yuv_thread(void *param);
static void *raw_replay_thread(void *param);
static void get_raw_replay_yuv_by_index(CVI_S32 index);

static CVI_S32 raw_replay_init(CVI_VOID)
{
	ABORT_IF(pRawReplayCtx != NULL);

	pRawReplayCtx = calloc(1, sizeof(RAW_REPLAY_CTX_S));
	ABORT_IF(pRawReplayCtx == NULL);

	memset(pRawReplayCtx, 0, sizeof(RAW_REPLAY_CTX_S));

	pRawReplayCtx->bRawReplayThreadEnabled = false;
	pRawReplayCtx->bRawReplayThreadSuspend = false;

	pRawReplayCtx->u32RoiTotalFrame = 0;

	pRawReplayCtx->bUseDMA = CVI_FALSE;

	pRawReplayCtx->u32CtrlFlag = 0;

	pRawReplayCtx->isRawReplayReady = false;

	pRawReplayCtx->stGetYuvCtx.bEnable = CVI_FALSE;
	pRawReplayCtx->stGetYuvCtx.yuvIndex = -1;
	sem_init(&pRawReplayCtx->stGetYuvCtx.sem, 0, 0);

	pRawReplayCtx->timingAttr.bEnable = CVI_FALSE;
	pRawReplayCtx->timingAttr.s32FrmRate = 25;

	return CVI_SUCCESS;
}

static CVI_S32 raw_replay_deinit(CVI_VOID)
{
	sem_destroy(&pRawReplayCtx->stGetYuvCtx.sem);
	if (pRawReplayCtx != NULL) {
		free(pRawReplayCtx);
		pRawReplayCtx = NULL;
	}

	return CVI_SUCCESS;
}

void raw_replay_ctrl(CVI_U32 flag)
{
	ABORT_IF(pRawReplayCtx == NULL);

	pRawReplayCtx->u32CtrlFlag = flag;
}

#define ION_TOTAL_MEM "/sys/kernel/debug/ion/cvi_vpp_heap_dump/total_mem"
#define ION_ALLOC_MEM "/sys/kernel/debug/ion/cvi_vpp_heap_dump/alloc_mem"
#define ION_SUMMARY   "/sys/kernel/debug/ion/cvi_vpp_heap_dump/summary"

static void show_ion_debug_info(void)
{
	system("cat "ION_SUMMARY);

	LOGOUT("ION total mem:\n");
	system("cat "ION_TOTAL_MEM);

	LOGOUT("ION alloc mem:\n");
	system("cat "ION_ALLOC_MEM);

	LOGOUT("raw replay frame number = (ION total mem - ION alloc mem) / rawFrameSize\n\n");
}

CVI_S32 set_raw_replay_data(const CVI_VOID *header, const CVI_VOID *data,
							CVI_U32 totalFrame, CVI_U32 curFrame, CVI_U32 rawFrameSize)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	RAW_REPLAY_INFO *pRawInfo = (RAW_REPLAY_INFO *) header;

#ifdef ENABLE_TEAISP_RAW_REPLAY_TEST
	teaisp_raw_inference_test(header, (CVI_VOID *) data, totalFrame, curFrame, rawFrameSize);
#endif

	if (curFrame == 0) {

		if (pRawReplayCtx == NULL) {
			raw_replay_init();
		} else {
			if (pRawReplayCtx->bRawReplayThreadEnabled) {

				pRawReplayCtx->bRawReplayThreadSuspend = true;

				while (pRawReplayCtx->isRawReplayReady) {
					usleep(10 * 1000);
				}

				LOGOUT("suspend raw replay thread...\n");

				if (pRawReplayCtx->pRawHeader != NULL) {
					free(pRawReplayCtx->pRawHeader);
					pRawReplayCtx->pRawHeader = NULL;
				}

				s32Ret = CVI_SYS_Munmap(pRawReplayCtx->pu8VirAddr[0],
					pRawReplayCtx->u32BlkSize);
				ERROR_IF(s32Ret != CVI_SUCCESS);

				s32Ret = CVI_VB_ReleaseBlock(pRawReplayCtx->blk);
				ERROR_IF(s32Ret != CVI_SUCCESS);

				s32Ret = CVI_VB_DestroyPool(pRawReplayCtx->PoolID);
				ERROR_IF(s32Ret != CVI_SUCCESS);
				pRawReplayCtx->PoolID = 0;
				pRawReplayCtx->u32RoiTotalFrame = 0;
			}
		}

		LOGOUT("totalFrame: %d, rawFrameSize: %d, %d,%d, roiFrameSize: %d, %d,%d,%d,%d,%d\n",
			totalFrame, rawFrameSize, pRawInfo->width, pRawInfo->height,
			pRawInfo->roiFrameSize,
			pRawInfo->roiFrameNum,
			pRawInfo->stRoiRect.s32X, pRawInfo->stRoiRect.s32Y,
			pRawInfo->stRoiRect.u32Width, pRawInfo->stRoiRect.u32Height);

		pRawReplayCtx->u32TotalFrame = totalFrame;
		pRawReplayCtx->u32RawFrameSize = rawFrameSize;

		if (pRawInfo->roiFrameNum != 0) {
			pRawReplayCtx->u32RoiTotalFrame = pRawInfo->roiFrameNum;
			pRawReplayCtx->stRoiRect = pRawInfo->stRoiRect;
			pRawReplayCtx->u32RoiRawFrameSize = pRawInfo->roiFrameSize;
		}

		if (pRawReplayCtx->pRawHeader == NULL) {
			pRawReplayCtx->pRawHeader = (RAW_REPLAY_INFO *) calloc(
										totalFrame,
										sizeof(RAW_REPLAY_INFO));
		}

		ABORT_IF(pRawReplayCtx->pRawHeader == NULL);

		VB_POOL_CONFIG_S cfg;
		CVI_U64 tmpPhyAddr = 0;

		cfg.u32BlkSize = (pRawReplayCtx->u32TotalFrame - pRawReplayCtx->u32RoiTotalFrame) * \
						 pRawReplayCtx->u32RawFrameSize;
		cfg.u32BlkCnt = 1;
		cfg.enRemapMode = VB_REMAP_MODE_CACHED;

		snprintf(cfg.acName, MAX_VB_POOL_NAME_LEN, "%s", "raw_replay_vb");

		if (pRawReplayCtx->u32RoiTotalFrame != 0) {
			cfg.u32BlkSize += pRawReplayCtx->u32RoiTotalFrame * pRawReplayCtx->u32RoiRawFrameSize;
		}

		pRawReplayCtx->u32BlkSize = cfg.u32BlkSize;

		if (pRawReplayCtx->PoolID == 0) {
			pRawReplayCtx->PoolID = CVI_VB_CreatePool(&cfg);
			if (pRawReplayCtx->PoolID == VB_INVALID_POOLID) {
				show_ion_debug_info();
			}
			ABORT_IF(pRawReplayCtx->PoolID == VB_INVALID_POOLID);

			pRawReplayCtx->blk = CVI_VB_GetBlock(pRawReplayCtx->PoolID, cfg.u32BlkSize);
			tmpPhyAddr = CVI_VB_Handle2PhysAddr(pRawReplayCtx->blk);
			pRawReplayCtx->u64PhyAddr[0] = tmpPhyAddr;
			pRawReplayCtx->pu8VirAddr[0] = (CVI_U8 *) CVI_SYS_MmapCache(tmpPhyAddr, cfg.u32BlkSize);
			if (pRawInfo->enWDR || pRawInfo->pixFormat) {
				pRawReplayCtx->u64PhyAddr[1] = pRawReplayCtx->u64PhyAddr[0] + pRawReplayCtx->u32BlkSize / 2;
				pRawReplayCtx->pu8VirAddr[1] = pRawReplayCtx->pu8VirAddr[0] + pRawReplayCtx->u32BlkSize / 2;
			}

			LOGOUT("create vb pool cnt: %d, blksize: %d phyAddr: %"PRId64"\n", 1,
				cfg.u32BlkSize, pRawReplayCtx->u64PhyAddr[0]);
		}

		if (pRawReplayCtx->u32RoiTotalFrame != 0) {
			pRawReplayCtx->u64RoiPhyAddr[0] = pRawReplayCtx->u64PhyAddr[0] + pRawReplayCtx->u32RawFrameSize;
			pRawReplayCtx->pu8RoiVirAddr[0] = pRawReplayCtx->pu8VirAddr[0] + pRawReplayCtx->u32RawFrameSize;
			if (pRawInfo->enWDR) {
				pRawReplayCtx->u64RoiPhyAddr[0] = pRawReplayCtx->u64PhyAddr[0] + pRawReplayCtx->u32RawFrameSize / 2;
				pRawReplayCtx->pu8RoiVirAddr[0] = pRawReplayCtx->pu8VirAddr[0] + pRawReplayCtx->u32RawFrameSize / 2;
				pRawReplayCtx->u64RoiPhyAddr[1] = pRawReplayCtx->u64PhyAddr[1] + pRawReplayCtx->u32RawFrameSize / 2;
				pRawReplayCtx->pu8RoiVirAddr[1] = pRawReplayCtx->pu8VirAddr[1] + pRawReplayCtx->u32RawFrameSize / 2;
			}
		}

		if (pRawReplayCtx->u32RoiTotalFrame != 0 &&
			pRawReplayCtx->u32RoiRawFrameSize != pRawReplayCtx->u32RawFrameSize) {

			pRawReplayCtx->bUseDMA = CVI_TRUE;
			CVI_U32 heightSrc = pRawInfo->stRoiRect.u32Height;
			CVI_U32 strideBytesSrc = pRawInfo->roiFrameSize / heightSrc;
			CVI_U32 strideBytesDst =
				pRawReplayCtx->u32RawFrameSize / pRawInfo->height;
			if (pRawInfo->enWDR) {
				strideBytesSrc = strideBytesSrc / 2;
				strideBytesDst = strideBytesDst / 2;
			}

			pRawReplayCtx->stDMAParam[0].u16StrideSrc = strideBytesSrc;
			pRawReplayCtx->stDMAParam[0].u16StrideDst = strideBytesDst;
			pRawReplayCtx->stDMAParam[0].u16Height = heightSrc;
			pRawReplayCtx->stDMAParam[0].u16Width = strideBytesSrc;
			pRawReplayCtx->stDMAParam[0].u64PhyAddrSrc = pRawReplayCtx->u64RoiPhyAddr[0];
			pRawReplayCtx->stDMAParam[0].u64PhyAddrDst = pRawReplayCtx->u64PhyAddr[0] +
				strideBytesDst * pRawInfo->stRoiRect.s32Y + pRawInfo->stRoiRect.s32X / 2 * 3;
			if (pRawInfo->enWDR) {
				pRawReplayCtx->stDMAParam[1].u16StrideSrc = strideBytesSrc;
				pRawReplayCtx->stDMAParam[1].u16StrideDst = strideBytesDst;
				pRawReplayCtx->stDMAParam[1].u16Height = heightSrc;
				pRawReplayCtx->stDMAParam[1].u16Width = strideBytesSrc;
				pRawReplayCtx->stDMAParam[1].u64PhyAddrSrc = pRawReplayCtx->u64RoiPhyAddr[1];
				pRawReplayCtx->stDMAParam[1].u64PhyAddrDst = pRawReplayCtx->u64PhyAddr[1] +
					strideBytesDst * pRawInfo->stRoiRect.s32Y + pRawInfo->stRoiRect.s32X / 2 * 3;
			}

			LOGOUT("stride_src: %d, stride_dst: %d, h: %d, w_bytes: %d\n",
				strideBytesSrc, strideBytesDst, heightSrc, strideBytesSrc);
		} else {
			pRawReplayCtx->bUseDMA = CVI_FALSE;
		}
	}

	if (pRawReplayCtx == NULL) {
		LOGOUT("ERROR, please set frame 0\n");
		return CVI_FAILURE;
	}

	LOGOUT("TotalFrame:%d, CurFrame:%d\n", pRawReplayCtx->u32TotalFrame, curFrame);

	if (curFrame < pRawReplayCtx->u32TotalFrame) {
		memcpy((uint8_t *)pRawReplayCtx->pRawHeader + curFrame * sizeof(RAW_REPLAY_INFO),
				header, sizeof(RAW_REPLAY_INFO));

		if (pRawReplayCtx->u32RoiTotalFrame == 0) {
			if (pRawInfo->enWDR || pRawInfo->pixFormat) {
				memcpy(pRawReplayCtx->pu8VirAddr[0] + curFrame * pRawReplayCtx->u32RawFrameSize / 2,
					data, pRawReplayCtx->u32RawFrameSize / 2);
				memcpy(pRawReplayCtx->pu8VirAddr[1] + curFrame * pRawReplayCtx->u32RawFrameSize / 2,
					data + pRawReplayCtx->u32RawFrameSize / 2, pRawReplayCtx->u32RawFrameSize / 2);
			} else {
				memcpy(pRawReplayCtx->pu8VirAddr[0] + curFrame * pRawReplayCtx->u32RawFrameSize,
					data, pRawReplayCtx->u32RawFrameSize);
			}
		} else {
			if (curFrame == 0) {
				if (pRawInfo->enWDR) {
					memcpy(pRawReplayCtx->pu8VirAddr[0], data, pRawReplayCtx->u32RawFrameSize / 2);
					memcpy(pRawReplayCtx->pu8VirAddr[1], data + pRawReplayCtx->u32RawFrameSize / 2,
							pRawReplayCtx->u32RawFrameSize / 2);
				} else {
					memcpy(pRawReplayCtx->pu8VirAddr[0], data, pRawReplayCtx->u32RawFrameSize);
				}
			} else {
				if (pRawInfo->enWDR) {
					memcpy(pRawReplayCtx->pu8RoiVirAddr[0] + (curFrame - 1) * pRawReplayCtx->u32RoiRawFrameSize / 2,
						data, pRawReplayCtx->u32RoiRawFrameSize / 2);
					memcpy(pRawReplayCtx->pu8RoiVirAddr[1] + (curFrame - 1) * pRawReplayCtx->u32RoiRawFrameSize / 2,
						data + pRawReplayCtx->u32RoiRawFrameSize / 2, pRawReplayCtx->u32RoiRawFrameSize / 2);
				} else {
					memcpy(pRawReplayCtx->pu8RoiVirAddr[0] + (curFrame - 1) * pRawReplayCtx->u32RoiRawFrameSize,
						data, pRawReplayCtx->u32RoiRawFrameSize);
				}
			}
		}
	} else {
		return CVI_FAILURE;
	}

	return CVI_SUCCESS;
}

CVI_S32 start_raw_replay(VI_PIPE ViPipe)
{
	RETURN_FAILURE_IF(pRawReplayCtx == NULL);

	CVI_ISP_AESetRawReplayMode(0, CVI_TRUE);

	if (!pRawReplayCtx->bRawReplayThreadEnabled) {

		pRawReplayCtx->ViPipe = ViPipe;

		pRawReplayCtx->bRawReplayThreadEnabled = CVI_TRUE;

		RETURN_FAILURE_IF(pthread_create(&pRawReplayCtx->rawReplayTid,
				NULL, raw_replay_thread, NULL) != 0);
	} else if (pRawReplayCtx->bRawReplayThreadSuspend) {
		pRawReplayCtx->bRawReplayThreadSuspend = false;
	}

	return CVI_SUCCESS;
}

CVI_S32 stop_raw_replay(void)
{
	CVI_S32 s32Ret = CVI_SUCCESS;

	CVI_ISP_AESetRawReplayMode(0, CVI_FALSE);

	if (pRawReplayCtx == NULL)
		return CVI_SUCCESS;

	if (pRawReplayCtx->bRawReplayThreadEnabled) {

		pRawReplayCtx->bRawReplayThreadSuspend = false;
		pRawReplayCtx->bRawReplayThreadEnabled = false;

		pthread_join(pRawReplayCtx->rawReplayTid, NULL);

		if (pRawReplayCtx->pRawHeader != NULL) {
			free(pRawReplayCtx->pRawHeader);
			pRawReplayCtx->pRawHeader = NULL;
		}

		s32Ret = CVI_SYS_Munmap(pRawReplayCtx->pu8VirAddr[0],
			pRawReplayCtx->u32BlkSize);
		ERROR_IF(s32Ret != CVI_SUCCESS);

		s32Ret = CVI_VB_ReleaseBlock(pRawReplayCtx->blk);
		ERROR_IF(s32Ret != CVI_SUCCESS);

		s32Ret = CVI_VB_DestroyPool(pRawReplayCtx->PoolID);
		ERROR_IF(s32Ret != CVI_SUCCESS);

		raw_replay_deinit();
	}

	return CVI_SUCCESS;
}

static void get_current_awb_info(ISP_MWB_ATTR_S *pstMwbAttr)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	ISP_WB_INFO_S stWBInfo;

	memset(&stWBInfo, 0, sizeof(ISP_WB_INFO_S));

	s32Ret = CVI_ISP_QueryWBInfo(pRawReplayCtx->ViPipe, &stWBInfo);
	ERROR_IF(s32Ret != CVI_SUCCESS);

	pstMwbAttr->u16Rgain = stWBInfo.u16Rgain;
	pstMwbAttr->u16Grgain = stWBInfo.u16Grgain;
	pstMwbAttr->u16Gbgain = stWBInfo.u16Gbgain;
	pstMwbAttr->u16Bgain = stWBInfo.u16Bgain;
}

static void update_awb_config(const ISP_MWB_ATTR_S *pstMwbAttr, ISP_OP_TYPE_E eType)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	ISP_WB_ATTR_S stWbAttr;

	if ((pRawReplayCtx->u32CtrlFlag & DISABLE_AWB_UPDATE_CTRL) != 0) {
		return;
	}

	memset(&stWbAttr, 0, sizeof(ISP_WB_ATTR_S));

	s32Ret = CVI_ISP_GetWBAttr(pRawReplayCtx->ViPipe, &stWbAttr);
	ERROR_IF(s32Ret != CVI_SUCCESS);

	if (eType == OP_TYPE_MANUAL) {
		stWbAttr.enOpType = OP_TYPE_MANUAL;
		stWbAttr.stManual = *pstMwbAttr;
	} else {
		stWbAttr.enOpType = OP_TYPE_AUTO;
	}

	s32Ret = CVI_ISP_SetWBAttr(pRawReplayCtx->ViPipe, &stWbAttr);
	ERROR_IF(s32Ret != CVI_SUCCESS);
}

static void get_current_ae_info(ISP_EXP_INFO_S *pstExpInfo)
{
	CVI_S32 s32Ret = CVI_SUCCESS;

	s32Ret = CVI_ISP_QueryExposureInfo(pRawReplayCtx->ViPipe, pstExpInfo);
	ERROR_IF(s32Ret != CVI_SUCCESS);
}

static void update_ae_config(const ISP_EXP_INFO_S *pstExpInfo, ISP_OP_TYPE_E eType)
{
	CVI_S32 s32Ret = CVI_SUCCESS;

	ISP_EXPOSURE_ATTR_S stAEAttr;
	ISP_WDR_EXPOSURE_ATTR_S stWdrExpAttr;

	memset(&stAEAttr, 0, sizeof(ISP_EXPOSURE_ATTR_S));
	memset(&stWdrExpAttr, 0, sizeof(ISP_WDR_EXPOSURE_ATTR_S));

	s32Ret = CVI_ISP_GetExposureAttr(pRawReplayCtx->ViPipe, &stAEAttr);
	ERROR_IF(s32Ret != CVI_SUCCESS);

	s32Ret = CVI_ISP_GetWDRExposureAttr(pRawReplayCtx->ViPipe, &stWdrExpAttr);
	ERROR_IF(s32Ret != CVI_SUCCESS);

	if (eType == OP_TYPE_MANUAL) {
		stAEAttr.enOpType = OP_TYPE_MANUAL;

		stAEAttr.stManual.enExpTimeOpType = OP_TYPE_MANUAL;
		stAEAttr.stManual.enAGainOpType = OP_TYPE_MANUAL;
		stAEAttr.stManual.enDGainOpType = OP_TYPE_MANUAL;
		stAEAttr.stManual.enISPDGainOpType = OP_TYPE_MANUAL;
		stAEAttr.stManual.enGainType = AE_TYPE_GAIN;

		stAEAttr.stManual.u32ExpTime = pstExpInfo->u32ExpTime;
		stAEAttr.stManual.u32AGain = pstExpInfo->u32AGain;
		stAEAttr.stManual.u32DGain = pstExpInfo->u32DGain;
		stAEAttr.stManual.u32ISPDGain = pstExpInfo->u32ISPDGain;

		if (pRawReplayCtx->u32TotalFrame == 1) {
			stWdrExpAttr.enExpRatioType = OP_TYPE_MANUAL;

			for (uint32_t i = 0; i < WDR_EXP_RATIO_NUM; i++) {
				stWdrExpAttr.au32ExpRatio[i] = pstExpInfo->u32WDRExpRatio;
			}
		}

		memcpy((CVI_U8 *)&stAEAttr.stAuto.au32Reserve[0],
			(CVI_U8 *)&pstExpInfo->fLightValue,
			sizeof(CVI_FLOAT));
	} else {
		stAEAttr.enOpType = OP_TYPE_AUTO;

		stAEAttr.stManual.enExpTimeOpType = OP_TYPE_AUTO;
		stAEAttr.stManual.enGainType = AE_TYPE_GAIN;
		stAEAttr.stManual.enISONumOpType = OP_TYPE_AUTO;

		stWdrExpAttr.enExpRatioType = OP_TYPE_AUTO;
	}

	s32Ret = CVI_ISP_SetExposureAttr(pRawReplayCtx->ViPipe, &stAEAttr);
	ERROR_IF(s32Ret != CVI_SUCCESS);

	s32Ret = CVI_ISP_SetWDRExposureAttr(pRawReplayCtx->ViPipe, &stWdrExpAttr);
	ERROR_IF(s32Ret != CVI_SUCCESS);

	if (eType == OP_TYPE_MANUAL)
		CVI_ISP_AESetRawReplayExposure(pRawReplayCtx->ViPipe, pstExpInfo);
}

static void apply_raw_header(ISP_MWB_ATTR_S *pstMwbAttr, ISP_EXP_INFO_S *pstExpInfo, CVI_U32 index)
{
	pstMwbAttr->u16Rgain = pRawReplayCtx->pRawHeader[index].WB_RGain;
	pstMwbAttr->u16Grgain = pstMwbAttr->u16Gbgain = pRawReplayCtx->pRawHeader[index].WB_GGain;
	pstMwbAttr->u16Bgain = pRawReplayCtx->pRawHeader[index].WB_BGain;

	pstExpInfo->u32ExpTime = pRawReplayCtx->pRawHeader[index].longExposure;
	pstExpInfo->u32AGain = pRawReplayCtx->pRawHeader[index].exposureAGain;
	pstExpInfo->u32DGain = pRawReplayCtx->pRawHeader[index].exposureDGain;
	pstExpInfo->u32ISPDGain = pRawReplayCtx->pRawHeader[index].ispDGain;

	pstExpInfo->u32ShortExpTime = pRawReplayCtx->pRawHeader[index].shortExposure;
	pstExpInfo->u32AGainSF = pRawReplayCtx->pRawHeader[index].AGainSF;
	pstExpInfo->u32DGainSF = pRawReplayCtx->pRawHeader[index].DGainSF;
	pstExpInfo->u32ISPDGainSF = pRawReplayCtx->pRawHeader[index].ispDGainSF;

	pstExpInfo->u32ISO = pRawReplayCtx->pRawHeader[index].ISO;
	pstExpInfo->fLightValue = pRawReplayCtx->pRawHeader[index].lightValue;

	pstExpInfo->u32WDRExpRatio = pRawReplayCtx->pRawHeader[index].exposureRatio;
}

static void raw_replay_use_dma(ISP_MWB_ATTR_S *pstMwbAttr, ISP_EXP_INFO_S *pstExpInfo,
				VIDEO_FRAME_INFO_S *pstVideoFrame)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	CVI_S32 idx = 0, sig = 1;
	CVI_U64 addrLe = 0, addrSe = 0;
	VI_PIPE PipeId[] = {0};
	const VIDEO_FRAME_INFO_S *pstVideoFrameInfo[1];
	CVI_U32 frameTime = 0;
	CVI_U32 frameRate = pRawReplayCtx->timingAttr.s32FrmRate;
	struct timeval t1, t2;

	pstVideoFrameInfo[0] = pstVideoFrame;
	if (!pRawReplayCtx->timingAttr.bEnable) {
		frameRate = frameRate > 0 ? frameRate : 25;
		frameTime = SEC_TO_USEC / frameRate;
	}

	if (!pRawReplayCtx->pRawHeader[0].pixFormat) {
		apply_raw_header(pstMwbAttr, pstExpInfo, 0);
		update_ae_config(pstExpInfo, OP_TYPE_MANUAL);
		update_awb_config(pstMwbAttr, OP_TYPE_MANUAL);
	}
	s32Ret = CVI_VI_SendPipeRaw(1, PipeId, pstVideoFrameInfo, 0);
	ERROR_IF(s32Ret != CVI_SUCCESS);

	while (pRawReplayCtx->bRawReplayThreadEnabled && !pRawReplayCtx->bRawReplayThreadSuspend) {
		get_raw_replay_yuv_by_index(idx + 1);
		CVI_ISP_GetVDTimeOut(0, ISP_VD_FE_END, 0);
		while (pRawReplayCtx->stGetYuvCtx.bEnable) {
			usleep(10 * 1000);
		}
		gettimeofday(&t2, NULL);

		if (pRawReplayCtx->pRawHeader[0].enWDR) {
			addrLe = pRawReplayCtx->u64RoiPhyAddr[0] + idx * pRawReplayCtx->u32RoiRawFrameSize / 2;
			pRawReplayCtx->stDMAParam[0].u64PhyAddrSrc = addrLe;
			s32Ret = CVI_SYS_CDMACopy2D(&pRawReplayCtx->stDMAParam[0]);
			ERROR_IF(s32Ret != CVI_SUCCESS);
			addrSe = pRawReplayCtx->u64RoiPhyAddr[1] + idx * pRawReplayCtx->u32RoiRawFrameSize / 2;
			pRawReplayCtx->stDMAParam[1].u64PhyAddrSrc = addrSe;
			s32Ret = CVI_SYS_CDMACopy2D(&pRawReplayCtx->stDMAParam[1]);
			ERROR_IF(s32Ret != CVI_SUCCESS);

		} else {
			addrLe = pRawReplayCtx->u64RoiPhyAddr[0] + idx * pRawReplayCtx->u32RoiRawFrameSize;
			pRawReplayCtx->stDMAParam[0].u64PhyAddrSrc = addrLe;
			s32Ret = CVI_SYS_CDMACopy2D(&pRawReplayCtx->stDMAParam[0]);
			ERROR_IF(s32Ret != CVI_SUCCESS);
		}

		if (!pRawReplayCtx->pRawHeader[0].pixFormat) {
			apply_raw_header(pstMwbAttr, pstExpInfo, idx + 1);
			update_ae_config(pstExpInfo, OP_TYPE_MANUAL);
		}

		if ((idx + sig >= (CVI_S32)pRawReplayCtx->u32RoiTotalFrame) || (idx + sig < 0)) {
			sig = -sig;
		}
		idx += sig;

		if (!pRawReplayCtx->timingAttr.bEnable) {
			CVI_U64 interval = (t2.tv_sec - t1.tv_sec) * SEC_TO_USEC + (t2.tv_usec - t1.tv_usec);

			if (frameTime > interval)
				usleep(frameTime - interval);
		}

		gettimeofday(&t1, NULL);
		s32Ret = CVI_VI_SendPipeRaw(1, PipeId, pstVideoFrameInfo, 0);
		ERROR_IF(s32Ret != CVI_SUCCESS);
	}
}

static void raw_replay_use_vb(ISP_MWB_ATTR_S *pstMwbAttr, ISP_EXP_INFO_S *pstExpInfo,
				VIDEO_FRAME_INFO_S *pstVideoFrame)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	VI_PIPE PipeId[] = {0};
	CVI_S32 idx = 0, sig = 1;
	CVI_U64 addrLe = 0, addrSe = 0;
	const VIDEO_FRAME_INFO_S *pstVideoFrameInfo[1];
	CVI_U32 frameTime = 0;
	CVI_U32 frameRate = pRawReplayCtx->timingAttr.s32FrmRate;
	struct timeval t1, t2;

	addrLe = pRawReplayCtx->u64PhyAddr[0];
	addrSe = pRawReplayCtx->u64PhyAddr[1];
	pstVideoFrameInfo[0] = pstVideoFrame;
	if (!pRawReplayCtx->timingAttr.bEnable) {
		frameRate = frameRate > 0 ? frameRate : 25;
		frameTime = SEC_TO_USEC / frameRate;
	}

	if (!pRawReplayCtx->pRawHeader[0].pixFormat) {
		apply_raw_header(pstMwbAttr, pstExpInfo, 0);
		update_awb_config(pstMwbAttr, OP_TYPE_MANUAL);
	}

	while (pRawReplayCtx->bRawReplayThreadEnabled && !pRawReplayCtx->bRawReplayThreadSuspend) {
		if (pRawReplayCtx->pRawHeader[0].enWDR  || pRawReplayCtx->pRawHeader[0].pixFormat) {
			pstVideoFrame->stVFrame.u64PhyAddr[0] = addrLe +
				idx * pRawReplayCtx->u32RawFrameSize / 2;
			pstVideoFrame->stVFrame.u64PhyAddr[1] = addrSe +
				idx * pRawReplayCtx->u32RawFrameSize / 2;
		} else {
			pstVideoFrame->stVFrame.u64PhyAddr[0] = addrLe +
				idx * pRawReplayCtx->u32RawFrameSize;
		}

		if (!pRawReplayCtx->pRawHeader[0].pixFormat) {
			apply_raw_header(pstMwbAttr, pstExpInfo, idx);
			update_ae_config(pstExpInfo, OP_TYPE_MANUAL);
		}

		gettimeofday(&t1, NULL);

		s32Ret = CVI_VI_SendPipeRaw(1, PipeId, pstVideoFrameInfo, 0);
		ERROR_IF(s32Ret != CVI_SUCCESS);

		get_raw_replay_yuv_by_index(idx);

		CVI_ISP_GetVDTimeOut(0, ISP_VD_FE_END, 0);
		while (pRawReplayCtx->stGetYuvCtx.bEnable) {
			usleep(10 * 1000);
		}

		gettimeofday(&t2, NULL);

		if (!pRawReplayCtx->timingAttr.bEnable) {
			CVI_U64 interval = (t2.tv_sec - t1.tv_sec) * SEC_TO_USEC + (t2.tv_usec - t1.tv_usec);

			if (frameTime > interval)
				usleep(frameTime - interval);
		}

		if ((idx + sig >= (CVI_S32)pRawReplayCtx->u32TotalFrame) || (idx + sig < 0)) {
			sig = -sig;
		}
		idx += sig;
	}
}

static void raw_replay_singlefrmae(ISP_MWB_ATTR_S *pstMwbAttr, ISP_EXP_INFO_S *pstExpInfo,
				VIDEO_FRAME_INFO_S *pstVideoFrame)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	VI_PIPE PipeId[] = {0};
	const VIDEO_FRAME_INFO_S *pstVideoFrameInfo[1];
	CVI_U32 frameTime = 0;
	CVI_U32 frameRate = pRawReplayCtx->timingAttr.s32FrmRate;
	struct timeval t1, t2;

	pstVideoFrameInfo[0] = pstVideoFrame;
	if (!pRawReplayCtx->timingAttr.bEnable) {
		frameRate = frameRate > 0 ? frameRate : 25;
		frameTime = SEC_TO_USEC / frameRate;
	}

	if (!pRawReplayCtx->pRawHeader[0].pixFormat) {
		apply_raw_header(pstMwbAttr, pstExpInfo, 0);
		update_awb_config(pstMwbAttr, OP_TYPE_MANUAL);
	}

	while (pRawReplayCtx->bRawReplayThreadEnabled && !pRawReplayCtx->bRawReplayThreadSuspend) {
		if (!pRawReplayCtx->pRawHeader[0].pixFormat) {
			update_ae_config(pstExpInfo, OP_TYPE_MANUAL);
		}

		gettimeofday(&t1, NULL);

		s32Ret = CVI_VI_SendPipeRaw(1, PipeId, pstVideoFrameInfo, 0);
		ERROR_IF(s32Ret != CVI_SUCCESS);
		CVI_ISP_GetVDTimeOut(0, ISP_VD_FE_END, 0);

		gettimeofday(&t2, NULL);

		if (!pRawReplayCtx->timingAttr.bEnable) {
			CVI_U64 interval = (t2.tv_sec - t1.tv_sec) * SEC_TO_USEC + (t2.tv_usec - t1.tv_usec);

			if (frameTime > interval)
				usleep(frameTime - interval);
		}
	}
}

CVI_BOOL is_raw_replay_ready(void)
{
	if (pRawReplayCtx != NULL) {
		return pRawReplayCtx->isRawReplayReady;
	} else {
		return false;
	}
}

static void update_video_frame(VIDEO_FRAME_INFO_S *stVideoFrame)
{
	CVI_U8 mode = pRawReplayCtx->pRawHeader[0].enWDR;

	LOGOUT("wdrmode: %d, width: %d, height: %d\n", mode, pRawReplayCtx->pRawHeader[0].width,
		pRawReplayCtx->pRawHeader[0].height);

	if (mode) {
		stVideoFrame->stVFrame.u32Width = pRawReplayCtx->pRawHeader[0].width >> 1;
	} else {
		stVideoFrame->stVFrame.u32Width = pRawReplayCtx->pRawHeader[0].width;
	}

	stVideoFrame->stVFrame.u32Height = pRawReplayCtx->pRawHeader[0].height;

	stVideoFrame->stVFrame.s16OffsetLeft = stVideoFrame->stVFrame.s16OffsetTop =
		stVideoFrame->stVFrame.s16OffsetRight = stVideoFrame->stVFrame.s16OffsetBottom = 0;

	stVideoFrame->stVFrame.enBayerFormat = (BAYER_FORMAT_E) pRawReplayCtx->pRawHeader[0].bayerID;
	stVideoFrame->stVFrame.enPixelFormat = 0;

	if (mode) {
		stVideoFrame->stVFrame.enDynamicRange = DYNAMIC_RANGE_HDR10;
		stVideoFrame->stVFrame.u64PhyAddr[0] = pRawReplayCtx->u64PhyAddr[0];
		stVideoFrame->stVFrame.u64PhyAddr[1] = pRawReplayCtx->u64PhyAddr[1];
	} else if (pRawReplayCtx->pRawHeader[0].pixFormat) {
		stVideoFrame->stVFrame.enDynamicRange = DYNAMIC_RANGE_SDR10;
		stVideoFrame->stVFrame.enPixelFormat = pRawReplayCtx->pRawHeader[0].pixFormat;
		stVideoFrame->stVFrame.u64PhyAddr[0] = pRawReplayCtx->u64PhyAddr[0];
		stVideoFrame->stVFrame.u64PhyAddr[1] = pRawReplayCtx->u64PhyAddr[1];
	} else {
		stVideoFrame->stVFrame.enDynamicRange = DYNAMIC_RANGE_SDR10;
		stVideoFrame->stVFrame.u64PhyAddr[0] = pRawReplayCtx->u64PhyAddr[0];
	}
}

static void *get_yuv_thread(void *param)
{
	CVI_S32 s32Ret = CVI_SUCCESS;

	UNUSED(param);

	while (pRawReplayCtx->stGetYuvCtx.bGetYuvThreadEnabled) {
		if (pRawReplayCtx->stGetYuvCtx.bEnable) {
			LOGOUT("get yuv by index: %d\n", pRawReplayCtx->stGetYuvCtx.yuvIndex);

			memset(&pRawReplayCtx->stGetYuvCtx.videoFrame, 0, sizeof(VIDEO_FRAME_INFO_S));

			if (pRawReplayCtx->stGetYuvCtx.src == 0) {
				s32Ret = CVI_VI_GetChnFrame(pRawReplayCtx->stGetYuvCtx.pipeOrGrp,
					pRawReplayCtx->stGetYuvCtx.chn,
					&pRawReplayCtx->stGetYuvCtx.videoFrame,
					MAX_GET_YUV_TIMEOUT);
			} else {
				s32Ret = CVI_VPSS_GetChnFrame(pRawReplayCtx->stGetYuvCtx.pipeOrGrp,
					pRawReplayCtx->stGetYuvCtx.chn,
					&pRawReplayCtx->stGetYuvCtx.videoFrame,
					MAX_GET_YUV_TIMEOUT);
			}

			if (s32Ret != CVI_SUCCESS) {
				LOGOUT("get yuv failed with: %#x!\n", s32Ret);
			}

			pRawReplayCtx->stGetYuvCtx.yuvIndex = -1;
			sem_post(&pRawReplayCtx->stGetYuvCtx.sem);
			pRawReplayCtx->stGetYuvCtx.bEnable = CVI_FALSE;
		}
		usleep(20 * 1000);
	}

	return NULL;
}

static void *raw_replay_thread(void *param)
{
	VIDEO_FRAME_INFO_S stVideoFrame;
	CVI_S32 s32Ret = CVI_SUCCESS;
	ISP_MWB_ATTR_S stMWBAttr;
	ISP_EXP_INFO_S stExpInfo;
	VI_DEV_TIMING_ATTR_S stTimingAttr;
	param = param;

	memset(&stMWBAttr, 0, sizeof(ISP_MWB_ATTR_S));
	memset(&stExpInfo, 0, sizeof(ISP_EXP_INFO_S));
	get_current_awb_info(&stMWBAttr);
	get_current_ae_info(&stExpInfo);
	LOGOUT("wbRGain:%d,wbGGain:%d,wbBGain:%d,exptime:%d,iso:%d,expratio:%d,LV:%f\n",
		pRawReplayCtx->pRawHeader[0].WB_RGain,
		pRawReplayCtx->pRawHeader[0].WB_GGain, pRawReplayCtx->pRawHeader[0].WB_BGain,
		pRawReplayCtx->pRawHeader[0].longExposure, pRawReplayCtx->pRawHeader[0].ISO,
		pRawReplayCtx->pRawHeader[0].exposureRatio, pRawReplayCtx->pRawHeader[0].lightValue);

	s32Ret = CVI_VI_GetDevTimingAttr(0, &stTimingAttr);
	if (s32Ret == CVI_SUCCESS) {
		pRawReplayCtx->timingAttr.bEnable = stTimingAttr.bEnable;
		pRawReplayCtx->timingAttr.s32FrmRate = stTimingAttr.s32FrmRate;
	}
	LOGOUT("\n\nstart raw replay, mode %d (0 manu, 1 auto), framerate %d...\n\n",
		pRawReplayCtx->timingAttr.bEnable, pRawReplayCtx->timingAttr.s32FrmRate);

	pRawReplayCtx->stGetYuvCtx.bGetYuvThreadEnabled = CVI_TRUE;
	pthread_create(&pRawReplayCtx->stGetYuvCtx.getYuvTid, NULL, get_yuv_thread, NULL);

	while (pRawReplayCtx->bRawReplayThreadEnabled) {
		update_video_frame(&stVideoFrame);

		pRawReplayCtx->isRawReplayReady = true;
		if (pRawReplayCtx->u32TotalFrame > 1) {
			if (pRawReplayCtx->bUseDMA) {
				raw_replay_use_dma(&stMWBAttr, &stExpInfo, &stVideoFrame);
			} else {
				raw_replay_use_vb(&stMWBAttr, &stExpInfo, &stVideoFrame);
			}
		} else {
			raw_replay_singlefrmae(&stMWBAttr, &stExpInfo, &stVideoFrame);
		}
		pRawReplayCtx->isRawReplayReady = false;

		while (pRawReplayCtx->bRawReplayThreadSuspend) {
			usleep(500 * 1000);
		}
	}

	pRawReplayCtx->stGetYuvCtx.bGetYuvThreadEnabled = CVI_FALSE;
	pthread_join(pRawReplayCtx->stGetYuvCtx.getYuvTid, NULL);

	LOGOUT("/*** raw replay therad end ***/\n");

	return NULL;
}

static void get_raw_replay_yuv_by_index(CVI_S32 index)
{
	if (index != pRawReplayCtx->stGetYuvCtx.yuvIndex) {
		return;
	}

	pRawReplayCtx->stGetYuvCtx.bEnable = CVI_TRUE;
}

CVI_S32 get_raw_replay_yuv(CVI_U8 yuvIndex, CVI_U8 src, CVI_S32 pipeOrGrp, CVI_S32 chn,
							VIDEO_FRAME_INFO_S *pstFrameInfo)
{
	if (pRawReplayCtx == NULL || pstFrameInfo == NULL) {
		return CVI_FAILURE;
	}

	if (yuvIndex <= 0) {
		LOGOUT("Single frame mode don't support!\n");
		return CVI_FAILURE;
	}

	pRawReplayCtx->stGetYuvCtx.src = src;
	pRawReplayCtx->stGetYuvCtx.pipeOrGrp = pipeOrGrp;
	pRawReplayCtx->stGetYuvCtx.chn = chn;
	pRawReplayCtx->stGetYuvCtx.yuvIndex = yuvIndex;
	memset(&pRawReplayCtx->stGetYuvCtx.videoFrame, 0, sizeof(VIDEO_FRAME_INFO_S));

	LOGOUT("get raw replay yuv+++, src: %d, pipeOrGrp: %d, chn: %d, yuvIndex: %d\n",
		src, pipeOrGrp, chn, yuvIndex);
	sem_wait(&pRawReplayCtx->stGetYuvCtx.sem);
	*pstFrameInfo = pRawReplayCtx->stGetYuvCtx.videoFrame;
	LOGOUT("get raw replay yuv---\n");

	return CVI_SUCCESS;
}

#if defined(__GNUC__) && defined(__riscv)
#pragma GCC diagnostic pop
#endif
