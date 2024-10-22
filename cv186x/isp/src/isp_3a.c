/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_3a.c
 * Description:
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>

#include "isp_main_local.h"
#include "isp_debug.h"
#include "isp_defines.h"
#include "cvi_ae.h"
#include "cvi_awb.h"
#include "isp_3a.h"
#include "isp_proc_local.h"
#include "isp_sts_ctrl.h"
#include "cvi_isp.h"

typedef struct _STITCH_CALIBRATION_INFO {
	CVI_BOOL bIsMainPipe;
	CVI_U16 centerLuma;
	CVI_U16 rGain;
	CVI_U16 bGain;
} STITCH_CALIBRATION_INFO;

ISP_LIB_INFO_S aeAlgoLibReg[VI_MAX_PIPE_NUM * MAX_REGISTER_ALG_LIB_NUM] = { 0 };
ISP_LIB_INFO_S awbAlgoLibReg[VI_MAX_PIPE_NUM * MAX_REGISTER_ALG_LIB_NUM] = { 0 };
ISP_LIB_INFO_S afAlgoLibReg[VI_MAX_PIPE_NUM * MAX_REGISTER_ALG_LIB_NUM] = { 0 };

ISP_LIB_INFO_S *pAlgoLibReg[AAA_TYPE_MAX] = {
	[AAA_TYPE_AE] = aeAlgoLibReg,
	[AAA_TYPE_AWB] = awbAlgoLibReg,
	[AAA_TYPE_AF] = afAlgoLibReg};

// ISP_AA_STAT_INFO_S aaStatInfoBuf[VI_MAX_PIPE_NUM];
ISP_AWB_INFO_S awbAlgoInfo[VI_MAX_PIPE_NUM];
ISP_AE_INFO_S aeAlgoInfo[VI_MAX_PIPE_NUM];
ISP_AF_INFO_S afAlgoInfo[VI_MAX_PIPE_NUM];

ISP_SMART_INFO_S smartInfo[VI_MAX_PIPE_NUM] = {0};
STITCH_CALIBRATION_INFO stitchCalibInfo[VI_MAX_PIPE_NUM] = {0};
#define DIV_0_TO_1(a)           ((0 == (a)) ? 1 : (a))

static CVI_U8 gSmartAeTimeOut;
static CVI_U32 gSmartAeFrameCnt[VI_MAX_PIPE_NUM] = {0};
struct timeval t2, t1;

void isp_reg_lib_dump(void)
{
	CVI_U32 i, j, k;
	ISP_LIB_INFO_S *pAlgo;

	for (i = 0; i < VI_MAX_PIPE_NUM; i++) {
		for (j = 0; j < MAX_REGISTER_ALG_LIB_NUM; j++) {
			for (k = 0; k < AAA_TYPE_MAX; k++) {
				pAlgo = pAlgoLibReg[k];
				if (pAlgo != NULL) {
					ISP_LOG_DEBUG("pAlgo 0x%p used %d LibName : %s\n", (void *)pAlgo,
						pAlgo[i * MAX_REGISTER_ALG_LIB_NUM + j].used,
						pAlgo[i * MAX_REGISTER_ALG_LIB_NUM + j].libInfo.acLibName);
				}
			}
		}
	}
}

CVI_S32 isp_3aLib_find(VI_PIPE ViPipe, const ALG_LIB_S *pstAlgoLib, AAA_LIB_TYPE_E type)
{
	CVI_S32 ret = ISP_3ALIB_FIND_FAIL, i;
	ISP_LIB_INFO_S *pAlgo = pAlgoLibReg[type];

	for (i = 0; i < MAX_REGISTER_ALG_LIB_NUM; i++) {
		if (strcmp((const char *)(pstAlgoLib->acLibName),
				(const char *)(pAlgo[ViPipe * MAX_REGISTER_ALG_LIB_NUM + i].libInfo.acLibName)) == 0) {
			ret = i;
			break;
		}
	}
	return ret;
}

CVI_S32 isp_3aLib_reg(VI_PIPE ViPipe, ALG_LIB_S *pstLib, CVI_VOID *pstRegister, AAA_LIB_TYPE_E type)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstRegister == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (pstLib == CVI_NULL) {
		return CVI_FAILURE;
	}

	ISP_LIB_INFO_S *pAlgo = pAlgoLibReg[type];
	CVI_U32 i;

	for (i = 0; i < MAX_REGISTER_ALG_LIB_NUM; i++) {
		if (pAlgo[ViPipe * MAX_REGISTER_ALG_LIB_NUM + i].used == 0) {
			memcpy(&(pAlgo[ViPipe * MAX_REGISTER_ALG_LIB_NUM + i].libInfo), pstLib, sizeof(ALG_LIB_S));

			if (type == AAA_TYPE_AE) {
				memcpy(&(pAlgo[ViPipe * MAX_REGISTER_ALG_LIB_NUM + i].algoFunc.aeFunc), pstRegister,
					sizeof(ISP_AE_EXP_FUNC_S));
			} else if (type == AAA_TYPE_AWB) {
				memcpy(&(pAlgo[ViPipe * MAX_REGISTER_ALG_LIB_NUM + i].algoFunc.awbFunc), pstRegister,
					sizeof(ISP_AWB_EXP_FUNC_S));
			} else if (type == AAA_TYPE_AF) {
				memcpy(&(pAlgo[ViPipe * MAX_REGISTER_ALG_LIB_NUM + i].algoFunc.afFunc), pstRegister,
					sizeof(ISP_AF_EXP_FUNC_S));
			}

			pAlgo[ViPipe * MAX_REGISTER_ALG_LIB_NUM + i].used = 1;
			break;
		}
	}

	if (i == MAX_REGISTER_ALG_LIB_NUM) {
		ISP_LOG_ERR("ALG Lib reg is already full\n");
		return CVI_FAILURE;
	}
	return CVI_SUCCESS;
}

CVI_S32 isp_3aLib_unreg(VI_PIPE ViPipe, ALG_LIB_S *pstLib, AAA_LIB_TYPE_E type)
{
	ISP_LIB_INFO_S *pAlgo = pAlgoLibReg[type];
	CVI_U32 i;

	for (i = 0; i < MAX_REGISTER_ALG_LIB_NUM; i++) {
		if (pAlgo[ViPipe * MAX_REGISTER_ALG_LIB_NUM + i].used == 1) {
			if (strcmp((const CVI_CHAR *)(pstLib->acLibName),
				(const CVI_CHAR *)(pAlgo[ViPipe * MAX_REGISTER_ALG_LIB_NUM + i].libInfo.acLibName))
				== 0) {
				memset(&(pAlgo[ViPipe * MAX_REGISTER_ALG_LIB_NUM + i].libInfo), 0, sizeof(ALG_LIB_S));

				if (type == AAA_TYPE_AE) {
					memset(&(pAlgo[ViPipe * MAX_REGISTER_ALG_LIB_NUM + i].algoFunc.aeFunc), 0,
						sizeof(ISP_AE_EXP_FUNC_S));
				} else if (type == AAA_TYPE_AWB) {
					memset(&(pAlgo[ViPipe * MAX_REGISTER_ALG_LIB_NUM + i].algoFunc.awbFunc), 0,
						sizeof(ISP_AWB_EXP_FUNC_S));
				} else if (type == AAA_TYPE_AF) {
					memset(&(pAlgo[ViPipe * MAX_REGISTER_ALG_LIB_NUM + i].algoFunc.afFunc), 0,
						sizeof(ISP_AF_EXP_FUNC_S));
				}

				pAlgo[ViPipe * MAX_REGISTER_ALG_LIB_NUM + i].used = 0;
				break;
			}
		}
	}

	if (i == MAX_REGISTER_ALG_LIB_NUM) {
		ISP_LOG_WARNING("type %d can't find compatible lib\n", type);
		return CVI_FAILURE;
	}
	return CVI_SUCCESS;
}

CVI_S32 isp_3aLib_get(VI_PIPE ViPipe, ALG_LIB_S *pstAlgoLib, AAA_LIB_TYPE_E type)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstAlgoLib == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	CVI_S32 activeIdx = 0;
	ISP_CTX_S *pstIspCtx = NULL;
	ISP_LIB_INFO_S *pAlgo = pAlgoLibReg[type];

	ISP_GET_CTX(ViPipe, pstIspCtx);

	activeIdx = pstIspCtx->activeLibIdx[type];

	// When not find active index. Show warning message but return OK.
	if (activeIdx == -1) {
		ISP_LOG_WARNING("type %d can't get compatible index\n", type);
	} else {
		memcpy(pstAlgoLib, &(pAlgo[ViPipe * MAX_REGISTER_ALG_LIB_NUM + activeIdx].libInfo), sizeof(ALG_LIB_S));
	}
	return ret;
}

CVI_S32 isp_3aLib_init(VI_PIPE ViPipe, AAA_LIB_TYPE_E type)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	ISP_LIB_INFO_S *pAlgo;
	ISP_CTX_S *pstIspCtx = NULL;
	CVI_S32 activeIdx = 0;
	ISP_AWB_PARAM_S awbInitParam;
	ISP_AE_PARAM_S aeInitParam;
	ISP_AF_PARAM_S afInitParam;

	ISP_GET_CTX(ViPipe, pstIspCtx);
	aeInitParam.enBayer = pstIspCtx->enBayer;
	aeInitParam.f32Fps = pstIspCtx->stSnsImageMode.f32Fps;
	afInitParam.u8WDRMode = awbInitParam.u8WDRMode = aeInitParam.u8WDRMode = pstIspCtx->u8SnsWDRMode;
	aeInitParam.aeSEWinConfig.winXNum = aeInitParam.aeLEWinConfig[0].winXNum = AE_ZONE_COLUMN;
	aeInitParam.aeSEWinConfig.winYNum = aeInitParam.aeLEWinConfig[0].winYNum = AE_ZONE_ROW;
	memcpy(&aeInitParam.stStitchAttr, &pstIspCtx->stStitchAttr, sizeof(ISP_STITCH_ATTR_S));
	awbInitParam.u16AWBWidth = pstIspCtx->stSysRect.u32Width;
	awbInitParam.u16AWBHeight = pstIspCtx->stSysRect.u32Height;

	awbInitParam.u8AWBZoneRow = pstIspCtx->stsCfgInfo.stWBCfg.u16ZoneRow;
	awbInitParam.u8AWBZoneCol = pstIspCtx->stsCfgInfo.stWBCfg.u16ZoneCol;
	awbInitParam.u8AWBZoneBin = 1;
	awbInitParam.SensorId = ViPipe;
	memcpy(&awbInitParam.stStitchAttr, &pstIspCtx->stStitchAttr, sizeof(ISP_STITCH_ATTR_S));
	afInitParam.SensorId = ViPipe;

	activeIdx = pstIspCtx->activeLibIdx[type];
	pAlgo = pAlgoLibReg[type];
	if (activeIdx == -1) {
		// AF not registered now. Don't print error message now.
		if (type != AAA_TYPE_AF) {
			ISP_LOG_WARNING("type %d not registered\n", type);
		}
		return CVI_FAILURE;
	}
	if (pAlgo[ViPipe * MAX_REGISTER_ALG_LIB_NUM + activeIdx].used != 1) {
		if (type != AAA_TYPE_AF) {
			ISP_LOG_ERR("type %d activeIdx is not used\n", type);
		}
		return CVI_FAILURE;
	}

	if (type == AAA_TYPE_AE) {
		if (pAlgo[ViPipe * MAX_REGISTER_ALG_LIB_NUM + activeIdx].algoFunc.aeFunc.pfn_ae_init != NULL) {
			pAlgo[ViPipe * MAX_REGISTER_ALG_LIB_NUM + activeIdx].algoFunc.aeFunc.pfn_ae_init(
				ViPipe, &aeInitParam);
		} else {
			ISP_LOG_ERR("Active AE lib didn't initial\n");
			return CVI_FAILURE;
		}
	} else if (type == AAA_TYPE_AWB) {
		if (pAlgo[ViPipe * MAX_REGISTER_ALG_LIB_NUM + activeIdx].algoFunc.awbFunc.pfn_awb_init != NULL) {
			pAlgo[ViPipe * MAX_REGISTER_ALG_LIB_NUM + activeIdx].algoFunc.awbFunc.pfn_awb_init(
				ViPipe, &awbInitParam);
		} else {
			ISP_LOG_ERR("Active AWB lib didn't initial\n");
			return CVI_FAILURE;
		}
	} else if (type == AAA_TYPE_AF) {
		if (pAlgo[ViPipe * MAX_REGISTER_ALG_LIB_NUM + activeIdx].algoFunc.afFunc.pfn_af_init != NULL) {
			pAlgo[ViPipe * MAX_REGISTER_ALG_LIB_NUM + activeIdx].algoFunc.afFunc.pfn_af_init(
				ViPipe, &afInitParam);
		} else {
			ISP_LOG_ERR("Active AF lib didn't initial\n");
			return CVI_FAILURE;
		}
	}

	return CVI_SUCCESS;
}

CVI_S32 isp_3aLib_Ctrl(VI_PIPE ViPipe, AAA_LIB_TYPE_E type)
{
	ISP_LIB_INFO_S *pAlgo;
	ISP_CTX_S *pstIspCtx = NULL;
	CVI_S32 activeIdx = 0;

	ISP_GET_CTX(ViPipe, pstIspCtx);

	activeIdx = pstIspCtx->activeLibIdx[type];
	pAlgo = pAlgoLibReg[type];
	if (activeIdx == -1) {
		if (type != AAA_TYPE_AF) {
			ISP_LOG_WARNING("type %d not registered\n", type);
		}
		return CVI_FAILURE;
	}
	if (pAlgo[ViPipe * MAX_REGISTER_ALG_LIB_NUM + activeIdx].used != 1) {
		if (type != AAA_TYPE_AF) {
			ISP_LOG_ERR("type %d activeIdx is not used\n", type);
		}
		return CVI_FAILURE;
	}

	if (type == AAA_TYPE_AE) {
		if (pAlgo[ViPipe * MAX_REGISTER_ALG_LIB_NUM + activeIdx].algoFunc.aeFunc.pfn_ae_ctrl != NULL) {
			pAlgo[ViPipe * MAX_REGISTER_ALG_LIB_NUM + activeIdx].algoFunc.aeFunc.pfn_ae_ctrl(
				ViPipe, ISP_WDR_MODE_SET, (CVI_VOID *)(&pstIspCtx->wdrLinearMode));
		} else {
			ISP_LOG_ERR("Active AE lib didn't initial or support ctrl func\n");
			return CVI_FAILURE;
		}
	} else if (type == AAA_TYPE_AWB) {
		if (pAlgo[ViPipe * MAX_REGISTER_ALG_LIB_NUM + activeIdx].algoFunc.awbFunc.pfn_awb_ctrl != NULL) {
			pAlgo[ViPipe * MAX_REGISTER_ALG_LIB_NUM + activeIdx].algoFunc.awbFunc.pfn_awb_ctrl(
				ViPipe, ISP_WDR_MODE_SET, (CVI_VOID *)(&pstIspCtx->wdrLinearMode));
		} else {
			ISP_LOG_ERR("Active AWB lib didn't initial or support ctrl func\n");
			return CVI_FAILURE;
		}
	} else if (type == AAA_TYPE_AF) {
		if (pAlgo[ViPipe * MAX_REGISTER_ALG_LIB_NUM + activeIdx].algoFunc.afFunc.pfn_af_ctrl != NULL) {
			pAlgo[ViPipe * MAX_REGISTER_ALG_LIB_NUM + activeIdx].algoFunc.afFunc.pfn_af_ctrl(
				ViPipe, ISP_WDR_MODE_SET, (CVI_VOID *)(&pstIspCtx->wdrLinearMode));
		} else {
			ISP_LOG_ERR("Active AF lib didn't initial or support ctrl func\n");
			return CVI_FAILURE;
		}
	}

	return CVI_SUCCESS;
}

static CVI_S32 isp_3aLib_updateAEAlgoInfo(VI_PIPE ViPipe)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	CVI_S32 ret = CVI_SUCCESS;

	ISP_AE_INFO_S *pstAEAlgoInfo = NULL;
	ISP_AE_STATISTICS_COMPAT_S *ae_sts;

	pstAEAlgoInfo = &(aeAlgoInfo[ViPipe]);
	isp_sts_ctrl_get_ae_sts(ViPipe, &ae_sts);

	for (CVI_U32 j = 0; j < AE_MAX_NUM; j++) {
		pstAEAlgoInfo->pstFEAeStat1[j] = &(ae_sts->aeStat1[j]);
		pstAEAlgoInfo->pstFEAeStat2[j] = &(ae_sts->aeStat2[j]);
		pstAEAlgoInfo->pstFEAeStat3[j] = &(ae_sts->aeStat3[j]);
	}

	return ret;
}

static CVI_S32 isp_3aLib_updateAWBAlgoInfo(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	ISP_CTX_S *pstIspCtx = NULL;
	ISP_AWB_INFO_S *pstAWBAlgoInfo = NULL;

	ISP_GET_CTX(ViPipe, pstIspCtx);
	pstAWBAlgoInfo = &(awbAlgoInfo[ViPipe]);

	pstAWBAlgoInfo->u32IsoNum = pstIspCtx->stAeResult.u32Iso;
	pstAWBAlgoInfo->s16LVx100 = pstIspCtx->stAeResult.s16CurrentLV;
	pstAWBAlgoInfo->fBVstep = pstIspCtx->stAeResult.fBvStep;

	ISP_WB_STATISTICS_S *awb_sts;

	ret = isp_sts_ctrl_get_awb_sts(ViPipe, ISP_CHANNEL_LE, &awb_sts);
	if (ret != CVI_SUCCESS) {
		ISP_LOG_ERR("awb le sts not ready\n");
		ret = CVI_FAILURE;
	}
	pstAWBAlgoInfo->stAwbStat2[ISP_CHANNEL_LE].pau16ZoneAvgR = awb_sts->au16ZoneAvgR;
	pstAWBAlgoInfo->stAwbStat2[ISP_CHANNEL_LE].pau16ZoneAvgG = awb_sts->au16ZoneAvgG;
	pstAWBAlgoInfo->stAwbStat2[ISP_CHANNEL_LE].pau16ZoneAvgB = awb_sts->au16ZoneAvgB;
	pstAWBAlgoInfo->stAwbStat2[ISP_CHANNEL_LE].pau16ZoneCount = awb_sts->au16ZoneCountAll;

	ret = isp_sts_ctrl_get_awb_sts(ViPipe, ISP_CHANNEL_SE, &awb_sts);
	if (ret != CVI_SUCCESS) {
		ISP_LOG_ERR("awb se sts not ready\n");
		ret = CVI_FAILURE;
	}
	pstAWBAlgoInfo->stAwbStat2[ISP_CHANNEL_SE].pau16ZoneAvgR = awb_sts->au16ZoneAvgR;
	pstAWBAlgoInfo->stAwbStat2[ISP_CHANNEL_SE].pau16ZoneAvgG = awb_sts->au16ZoneAvgG;
	pstAWBAlgoInfo->stAwbStat2[ISP_CHANNEL_SE].pau16ZoneAvgB = awb_sts->au16ZoneAvgB;
	pstAWBAlgoInfo->stAwbStat2[ISP_CHANNEL_SE].pau16ZoneCount = awb_sts->au16ZoneCountAll;

	return ret;
}

static CVI_S32 isp_3aLib_updateAFAlgoInfo(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	ISP_AF_INFO_S *pstAFAlgoInfo = NULL;

	pstAFAlgoInfo = &(afAlgoInfo[ViPipe]);

	ret = isp_sts_ctrl_get_af_sts(ViPipe, &(pstAFAlgoInfo->pstAfStat));
	if (ret != CVI_SUCCESS) {
		ISP_LOG_ERR("awb le sts not ready\n");
		ret = CVI_FAILURE;
	}

	return ret;
}

CVI_S32 isp_3aLib_smart_info_set(VI_PIPE ViPipe, const ISP_SMART_INFO_S *pstSmartInfo, CVI_U8 TimeOut)
{
	ISP_CTX_S *pstIspCtx = NULL;
	int index = 0;

	if (pstSmartInfo == CVI_NULL) {
		return CVI_FAILURE;
	}

	ISP_GET_CTX(ViPipe, pstIspCtx);
	__sync_fetch_and_and(&gSmartAeTimeOut, 0);
	__sync_fetch_and_add(&gSmartAeTimeOut, TimeOut);
	gSmartAeFrameCnt[ViPipe] = pstIspCtx->frameCnt;
	if (pstSmartInfo->stROI[0].u8Num == 0 && pstSmartInfo->stROI[1].u8Num != 0) {
		index = 1;
	}
	memcpy(&smartInfo[ViPipe], pstSmartInfo, sizeof(ISP_SMART_INFO_S));
	memcpy(&aeAlgoInfo[ViPipe].stSmartInfo, &pstSmartInfo->stROI[index], sizeof(ISP_SMART_ROI_S));
	memcpy(&awbAlgoInfo[ViPipe].stSmartInfo, &pstSmartInfo->stROI[index], sizeof(ISP_SMART_ROI_S));
	return CVI_SUCCESS;
}

CVI_VOID isp_smartROI_info_update(VI_PIPE ViPipe)
{
	ISP_CTX_S *pstIspCtx = NULL;

	ISP_GET_CTX(ViPipe, pstIspCtx);
	if (aeAlgoInfo[ViPipe].stSmartInfo.bEnable) {
		if (gSmartAeTimeOut != 0 && pstIspCtx->frameCnt - gSmartAeFrameCnt[ViPipe] > gSmartAeTimeOut) {
			__sync_fetch_and_and(&aeAlgoInfo[ViPipe].stSmartInfo.u8Num, 0);
			__sync_fetch_and_and(&awbAlgoInfo[ViPipe].stSmartInfo.u8Num, 0);
			__sync_fetch_and_and(&gSmartAeTimeOut, 0);
		}
	}
}

CVI_S32 isp_3aLib_smart_info_get(VI_PIPE ViPipe, ISP_SMART_INFO_S *pstSmartInfo)
{
	if (pstSmartInfo == CVI_NULL) {
		return CVI_FAILURE;
	}

	memcpy(pstSmartInfo, &smartInfo[ViPipe], sizeof(ISP_SMART_INFO_S));
	return CVI_SUCCESS;
}

CVI_S32 isp_3aLib_stitch_calibration(VI_PIPE ViPipe, ISP_STITCH_ATTR_S *pAttr, CVI_U8 chnNum)
{
#define STITCH_GAIN_BASE 1024
#define AE_CHANNEL_GR (1)
#define AE_CHANNEL_GB (2)
#define VALUE_MIN	  (20)
#define VALUE_MAX	  (900)

	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM) ||
		(chnNum > VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d or chnNum %d value error\n", ViPipe, chnNum);
		return -ENODEV;
	}

	if (pAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	ISP_AE_STATISTICS_COMPAT_S * ae_sts;
	ISP_WB_STATISTICS_S *awb_sts;
	ISP_FE_AE_STAT_3_S aeLumaStat;
	ISP_WB_STATISTICS_S wbStat;
	CVI_U32 GValue, i, row, column, idx, centerCnt = 0, centerG = 0;
	CVI_U16 RGain = 0, BGain = 0, pixelR, pixelG, pixelB;
	CVI_U8 centerRowStart, centerRowEnd, centerColumnStart, centerColumnEnd;
	CVI_U64 u64TotalPixelSumR = 0, u64TotalPixelSumB = 0;
	VI_PIPE maxPipe = 0;

	isp_sts_ctrl_get_ae_sts(ViPipe, &ae_sts);
	isp_sts_ctrl_get_awb_sts(ViPipe, ISP_CHANNEL_LE, &awb_sts);

	stitchCalibInfo[ViPipe].bIsMainPipe = pAttr->bMainPipe;

	memcpy(&aeLumaStat, &ae_sts->aeStat3[0], sizeof(ISP_FE_AE_STAT_3_S));
	memcpy(&wbStat, awb_sts, sizeof(ISP_WB_STATISTICS_S));

	centerRowStart = AE_ZONE_ROW / 2 - AE_ZONE_ROW / 4;
	centerRowEnd = AE_ZONE_ROW / 2 + AE_ZONE_ROW / 4;
	centerColumnStart = AE_ZONE_COLUMN / 2 - AE_ZONE_COLUMN / 4;
	centerColumnEnd = AE_ZONE_COLUMN / 2 + AE_ZONE_COLUMN / 4;

	for (row = centerRowStart; row < centerRowEnd; row++) {
		for (column = centerColumnStart; column < centerColumnEnd; column++) {
			GValue = (aeLumaStat.au16ZoneAvg[ISP_CHANNEL_LE][row][column][AE_CHANNEL_GR] +
					aeLumaStat.au16ZoneAvg[ISP_CHANNEL_LE][row][column][AE_CHANNEL_GB]) / 2;
			if (GValue > VALUE_MIN && GValue < VALUE_MAX) {
				centerCnt++;
				centerG += GValue;
			}
		}
	}
	stitchCalibInfo[ViPipe].centerLuma = centerG / centerCnt;

	centerRowStart = AWB_ZONE_ORIG_ROW / 2 - AWB_ZONE_ORIG_ROW / 4;
	centerRowEnd = AWB_ZONE_ORIG_ROW / 2 + AWB_ZONE_ORIG_ROW / 4;
	centerColumnStart = AWB_ZONE_ORIG_COLUMN / 2 - AWB_ZONE_ORIG_COLUMN / 4;
	centerColumnEnd = AWB_ZONE_ORIG_COLUMN / 2 + AWB_ZONE_ORIG_COLUMN / 4;

	for (row = centerRowStart; row < centerRowEnd; row++) {
		for (column = centerColumnStart; column < centerColumnEnd; column++) {
			idx = column * AWB_ZONE_ORIG_COLUMN + row;
			pixelR = wbStat.au16ZoneAvgR[idx];
			pixelG = wbStat.au16ZoneAvgG[idx];
			pixelB = wbStat.au16ZoneAvgB[idx];

			if ((pixelR > VALUE_MIN && pixelR < VALUE_MAX) &&
				(pixelG > VALUE_MIN && pixelG < VALUE_MAX) &&
				(pixelB > VALUE_MIN && pixelB < VALUE_MAX)) {
				RGain = pixelG * STITCH_GAIN_BASE / pixelR;
				BGain = pixelG * STITCH_GAIN_BASE / pixelB;
				u64TotalPixelSumR += RGain;
				u64TotalPixelSumB += BGain;
				centerCnt++;
			}
		}
	}

	stitchCalibInfo[ViPipe].rGain = u64TotalPixelSumR / centerCnt;
	stitchCalibInfo[ViPipe].bGain = u64TotalPixelSumB / centerCnt;

	for (i = 1; i < chnNum; i++) {
		if (stitchCalibInfo[i].centerLuma > stitchCalibInfo[maxPipe].centerLuma) {
			maxPipe = i;
		}
	}

	for (i = 0; i < chnNum; i++) {
		if(stitchCalibInfo[i].centerLuma != 0) {
			pAttr[i].u32CalibLumaRatio = stitchCalibInfo[maxPipe].centerLuma * STITCH_GAIN_BASE/
										stitchCalibInfo[i].centerLuma;
			pAttr[i].u32CalibRGainRatio = stitchCalibInfo[i].rGain * STITCH_GAIN_BASE/
										stitchCalibInfo[maxPipe].rGain;
			pAttr[i].u32CalibBGainRatio = stitchCalibInfo[i].bGain * STITCH_GAIN_BASE/
										stitchCalibInfo[maxPipe].bGain;
		} else {
			pAttr[i].u32CalibLumaRatio = STITCH_GAIN_BASE;
			pAttr[i].u32CalibRGainRatio = STITCH_GAIN_BASE;
			pAttr[i].u32CalibBGainRatio = STITCH_GAIN_BASE;
		}
	}

	return CVI_SUCCESS;
}

CVI_S32 isp_3aLib_stitch_ctrl(VI_PIPE ViPipe, AAA_LIB_TYPE_E type)
{
#define AE_LE (0)

	ISP_CTX_S *pstIspCtx = NULL;
	static ISP_FE_AE_STAT_3_S preFEAeStat;
	static CVI_U16 preFEAwbStatR[ISP_CHANNEL_MAX_NUM][AWB_ZONE_NUM];
	static CVI_U16 preFEAwbStatG[ISP_CHANNEL_MAX_NUM][AWB_ZONE_NUM];
	static CVI_U16 preFEAwbStatB[ISP_CHANNEL_MAX_NUM][AWB_ZONE_NUM];
	static CVI_U8 preAeSave[VI_MAX_DEV_NUM], preAwbSave[VI_MAX_DEV_NUM];
	ISP_FE_AE_STAT_3_S *tmpFEAeStat;
	ISP_AWB_STAT_RESULT_S *tmpFEAwbStat;
	CVI_U8 row, column, i, chn, columnStart, baseNum, chnNum = 4;
	CVI_U16 columnIdx = 0;

	ISP_GET_CTX(ViPipe, pstIspCtx);
	CVI_U8 ispChnMax = (pstIspCtx->u8SnsWDRMode) ? 2 : 1;

	if (!pstIspCtx->stStitchAttr.enable) {
		return CVI_SUCCESS;
	}

	baseNum = pstIspCtx->stStitchAttr.bMainPipe ? 0 : 1;
	if (type == AAA_TYPE_AE) {
		preAeSave[ViPipe] = 1;
		columnStart = pstIspCtx->stStitchAttr.bMainPipe ? 0 : AE_ZONE_COLUMN / 2; // !!!
		tmpFEAeStat = aeAlgoInfo[ViPipe].pstFEAeStat3[0];
		for (i = 0; i < ispChnMax; ++i) {
			for (row = 0; row < AE_ZONE_ROW; row++) {
				for (column = 0; column < AE_ZONE_COLUMN / 2; column++) {
					for (chn = 0; chn < chnNum; chn++) {
						preFEAeStat.au16ZoneAvg[i][row][column + columnStart][chn] =
						(tmpFEAeStat[0].au16ZoneAvg[i][row][2 * column + baseNum][chn] +
						tmpFEAeStat[0].au16ZoneAvg[i][row][2 * column + 1 + baseNum][chn]) / 2;
					}
				}
			}
		}
		if (preAeSave[0] == 1 && preAeSave[1] == 1) {
			memcpy(aeAlgoInfo[ViPipe].pstFEAeStat3[0], &preFEAeStat, sizeof(ISP_FE_AE_STAT_3_S));
		}
	} else if (type == AAA_TYPE_AWB) {
		preAwbSave[ViPipe] = 1;
		for (i = 0; i < ispChnMax; ++i) {
			tmpFEAwbStat = &awbAlgoInfo[ViPipe].stAwbStat2[i];
			for (row = 0; row < AWB_ZONE_ORIG_ROW; row++) {
				for (column = 0; column < AWB_ZONE_ORIG_COLUMN / 2; column++) {
					columnIdx = row * AWB_ZONE_ORIG_COLUMN +  2 * column + baseNum;
					preFEAwbStatR[i][columnIdx] = tmpFEAwbStat->pau16ZoneAvgR[columnIdx];
					preFEAwbStatG[i][columnIdx] = tmpFEAwbStat->pau16ZoneAvgG[columnIdx];
					preFEAwbStatB[i][columnIdx] = tmpFEAwbStat->pau16ZoneAvgB[columnIdx];
				}
			}
			if (preAwbSave[0] == 1 && preAwbSave[1] == 1) {
				memcpy(awbAlgoInfo[ViPipe].stAwbStat2[i].pau16ZoneAvgR, preFEAwbStatR[i],
					AWB_ZONE_NUM * sizeof(CVI_U16));
				memcpy(awbAlgoInfo[ViPipe].stAwbStat2[i].pau16ZoneAvgG, preFEAwbStatG[i],
					AWB_ZONE_NUM * sizeof(CVI_U16));
				memcpy(awbAlgoInfo[ViPipe].stAwbStat2[i].pau16ZoneAvgB, preFEAwbStatB[i],
					AWB_ZONE_NUM * sizeof(CVI_U16));
			}
		}
	}
	return CVI_SUCCESS;
}

CVI_S32 isp_3aLib_run(VI_PIPE ViPipe, AAA_LIB_TYPE_E type)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	ISP_LIB_INFO_S *pAlgo;
	ISP_CTX_S *pstIspCtx = NULL;
	CVI_S32 activeIdx = 0;
	CVI_S32 rsv = 0;
	ISP_AE_RESULT_S stAeResult;
	ISP_AWB_RESULT_S stAwbResult;
	ISP_AF_RESULT_S stAfResult;
	CVI_U8 bMainPipeOk;
	VI_PIPE mainPipe = VI_MAX_PIPE_NUM, i;

	ISP_GET_CTX(ViPipe, pstIspCtx);

	activeIdx = pstIspCtx->activeLibIdx[type];
	pAlgo = pAlgoLibReg[type];

	if (activeIdx == -1) {
		// AF not registered now. Don't print error message now.
		if (type != AAA_TYPE_AF) {
			ISP_LOG_WARNING("type %d not registered\n", type);
		}
		return CVI_FAILURE;
	}
	if (pAlgo[ViPipe * MAX_REGISTER_ALG_LIB_NUM + activeIdx].used != 1) {
		// AF not registered now. Don't print error message now.
		if (type != AAA_TYPE_AF) {
			ISP_LOG_ERR("type %d activeIdx is not used\n", type);
		}
		return CVI_FAILURE;
	}
	if (type == AAA_TYPE_AE) {
		isp_3aLib_updateAEAlgoInfo(ViPipe);
	} else if (type == AAA_TYPE_AWB) {
		isp_3aLib_updateAWBAlgoInfo(ViPipe);
	}

	isp_3aLib_Ctrl(ViPipe, type);
	isp_smartROI_info_update(ViPipe);
	isp_3aLib_stitch_ctrl(ViPipe, type);

	if (pstIspCtx->stStitchAttr.enable && !pstIspCtx->stStitchAttr.bMainPipe) {
		for (i = 0; i < VI_MAX_PIPE_NUM; i++) {
			if (g_astIspCtx[i]->stStitchAttr.bMainPipe) {
				mainPipe = i;
				break;
			}
		}
		if (mainPipe != VI_MAX_PIPE_NUM) {
			bMainPipeOk = 1;
		}
	}

	if (type == AAA_TYPE_AE) {
		// isp_3aLib_updateAEAlgoInfo(ViPipe);

		if (pAlgo[ViPipe * MAX_REGISTER_ALG_LIB_NUM + activeIdx].algoFunc.aeFunc.pfn_ae_run != NULL) {
			pAlgo[ViPipe * MAX_REGISTER_ALG_LIB_NUM + activeIdx].algoFunc.aeFunc.pfn_ae_run(
				ViPipe, &aeAlgoInfo[ViPipe], &stAeResult, rsv);
			//isp_snsSync_cfg_set(ViPipe);
			if (pstIspCtx->stStitchAttr.enable && !pstIspCtx->stStitchAttr.bMainPipe && bMainPipeOk) {
				memcpy(&(pstIspCtx->stAeResult), &(g_astIspCtx[mainPipe]->stAeResult),
							sizeof(ISP_AE_RESULT_S));
				if (pstIspCtx->stStitchAttr.CalibEnable) {
					ISP_AE_RESULT_S *tmp = &pstIspCtx->stAeResult;

					tmp->u32IspDgain = stAeResult.u32IspDgain *
						pstIspCtx->stStitchAttr.u32CalibLumaRatio / 1024;
					tmp->u32IspDgainSF = stAeResult.u32IspDgainSF *
						pstIspCtx->stStitchAttr.u32CalibLumaRatio / 1024;
				}
			} else {
				memcpy(&(pstIspCtx->stAeResult), &stAeResult, sizeof(ISP_AE_RESULT_S));
			}
		} else {
			ISP_LOG_ERR("Active AE lib didn't initial or support run func\n");

			return CVI_FAILURE;
		}

		/*TODO@CF. Open BLC here first for AE updaet dgain. Maybe check BLC at param_flush.*/
	} else if (type == AAA_TYPE_AWB) {

		if (pAlgo[ViPipe * MAX_REGISTER_ALG_LIB_NUM + activeIdx].algoFunc.awbFunc.pfn_awb_run != NULL) {
			// gettimeofday(&t1, NULL);
			// if (isp_3aLib_updateAWBAlgoInfo(ViPipe) == CVI_SUCCESS)
			pAlgo[ViPipe * MAX_REGISTER_ALG_LIB_NUM + activeIdx].algoFunc.awbFunc.pfn_awb_run(
					ViPipe, &awbAlgoInfo[ViPipe], &stAwbResult, rsv);
			if (pstIspCtx->stStitchAttr.enable && !pstIspCtx->stStitchAttr.bMainPipe && bMainPipeOk) {
				memcpy(&(pstIspCtx->stAwbResult), &(g_astIspCtx[mainPipe]->stAwbResult),
						sizeof(ISP_AWB_RESULT_S));
				if (pstIspCtx->stStitchAttr.CalibEnable) {
					ISP_AWB_RESULT_S *tmp = &pstIspCtx->stAwbResult;

					tmp->au32WhiteBalanceGain[0] = stAwbResult.au32WhiteBalanceGain[0] *
						pstIspCtx->stStitchAttr.u32CalibRGainRatio/ 1024;
					tmp->au32WhiteBalanceGain[3] = stAwbResult.au32WhiteBalanceGain[3] *
						pstIspCtx->stStitchAttr.u32CalibBGainRatio/ 1024;
				}
			} else {
				memcpy(&(pstIspCtx->stAwbResult), &stAwbResult, sizeof(ISP_AWB_RESULT_S));
			}

		} else {
			ISP_LOG_ERR("Active AWB lib didn't initial or support run func\n");

			return CVI_FAILURE;
		}
	} else if (type == AAA_TYPE_AF) {
		isp_3aLib_updateAFAlgoInfo(ViPipe);

		if (pAlgo[ViPipe * MAX_REGISTER_ALG_LIB_NUM + activeIdx].algoFunc.afFunc.pfn_af_run != NULL) {
			pAlgo[ViPipe * MAX_REGISTER_ALG_LIB_NUM + activeIdx].algoFunc.afFunc.pfn_af_run(
				ViPipe, &afAlgoInfo[ViPipe], &stAfResult, rsv);

		} else {
			ISP_LOG_ERR("Active AF lib didn't initial or support run func\n");

			return CVI_FAILURE;
		}
	}

	return CVI_SUCCESS;
}

CVI_S32 isp_3aLib_exit(VI_PIPE ViPipe, AAA_LIB_TYPE_E type)
{
	ISP_LIB_INFO_S *pAlgo;
	ISP_CTX_S *pstIspCtx = NULL;
	CVI_S32 activeIdx = 0;

	ISP_GET_CTX(ViPipe, pstIspCtx);

	activeIdx = pstIspCtx->activeLibIdx[type];
	pAlgo = pAlgoLibReg[type];

	if (activeIdx == -1) {
		if (type != AAA_TYPE_AF) {
			ISP_LOG_WARNING("type %d not registered\n", type);
		}
		return CVI_FAILURE;
	}

	if (type == AAA_TYPE_AE) {
		if (pAlgo[ViPipe * MAX_REGISTER_ALG_LIB_NUM + activeIdx].algoFunc.aeFunc.pfn_ae_exit != NULL)
			pAlgo[ViPipe * MAX_REGISTER_ALG_LIB_NUM + activeIdx].algoFunc.aeFunc.pfn_ae_exit(ViPipe);
		else {
			ISP_LOG_ERR("type %d registered ae lib can't init\n", type);
			return -EPERM;
		}
	} else if (type == AAA_TYPE_AWB) {
		if (pAlgo[ViPipe * MAX_REGISTER_ALG_LIB_NUM + activeIdx].algoFunc.awbFunc.pfn_awb_exit != NULL)
			pAlgo[ViPipe * MAX_REGISTER_ALG_LIB_NUM + activeIdx].algoFunc.awbFunc.pfn_awb_exit(ViPipe);
		else {
			ISP_LOG_ERR("type %d registered awb lib can't init\n", type);
			return -EPERM;
		}
	}
	return CVI_SUCCESS;
}

CVI_S32 isp_3a_frameCnt_set(VI_PIPE ViPipe, CVI_U32 frameCnt)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	awbAlgoInfo[ViPipe].u32FrameCnt = frameCnt;
	aeAlgoInfo[ViPipe].u32FrameCnt = frameCnt;
	afAlgoInfo[ViPipe].u32FrameCnt = frameCnt;
	return CVI_SUCCESS;
}

CVI_S32 isp_3a_update_proc_info(VI_PIPE ViPipe)
{
	ISP_EXP_INFO_S stExpInfo;
	ISP_WB_INFO_S stWBInfo;

	// AE
	CVI_ISP_QueryExposureInfo(ViPipe, &stExpInfo);
	isp_proc_collectAeInfo(ViPipe, &stExpInfo);

	// AWB
	CVI_ISP_QueryWBInfo(ViPipe, &stWBInfo);
	isp_proc_collectAwbInfo(ViPipe, &stWBInfo);

	// AF
	return CVI_SUCCESS;
}
