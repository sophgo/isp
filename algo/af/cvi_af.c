/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: cvi_af.c
 * Description:
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <sys/time.h> //for gettimeofday()
#include <unistd.h> //for usleep()
#include "cvi_isp.h"
#include "cvi_comm_3a.h"
#include "isp_main.h"
#include "isp_debug.h"
#include "isp_proc.h"
#include "cvi_base.h"

#include "afalgo.h"


#define _SHOW_AF_INFO_	(0)

//need ENABLE_AF_LIB
//isp_3aLib_run(ViPipe, AAA_TYPE_AF);
//isp_defaults.c  pstIspCtx->ispProcCtrl.u32AFStatIntvl

ISP_FOCUS_ZONE_S stAFZ[AF_ZONE_ROW][AF_ZONE_COLUMN]; /*R; The zoned measure of contrast*/

CVI_S32 SAMPLE_AF_Init(CVI_S32 s32Handle, const ISP_AF_PARAM_S *pstAfParam)
{
	CVI_S32 s32Ret = CVI_SUCCESS;

	UNUSED(s32Handle);
	UNUSED(pstAfParam);
	printf("Func %s line %d\n", __func__, __LINE__);
	return s32Ret;
}
CVI_S32 SAMPLE_AF_Run(CVI_S32 s32Handle, const ISP_AF_INFO_S *pstAfInfo,
	ISP_AF_RESULT_S *pstAfResult, CVI_S32 s32Rsv)
{
	UNUSED(s32Handle);
	UNUSED(pstAfInfo);
	UNUSED(pstAfResult);
	UNUSED(s32Rsv);

	if (pstAfInfo == NULL) {
		return CVI_FAILURE;
	}
	if (pstAfInfo->pstAfStat == NULL) {
		return CVI_FAILURE;
	}

	memcpy(stAFZ, pstAfInfo->pstAfStat->stFEAFStat.stZoneMetrics, sizeof(ISP_FOCUS_ZONE_S) *
		AF_ZONE_ROW * AF_ZONE_ROW);

#if _SHOW_AF_INFO_
	printf("%d\n", pstAfInfo->u32FrameCnt);
	printf("%d %d %ld %ld\n", stAFZ[AF_ZONE_ROW/2][AF_ZONE_COLUMN/2].u16HlCnt,
		stAFZ[AF_ZONE_ROW/2][AF_ZONE_COLUMN/2].u32v0,
		stAFZ[AF_ZONE_ROW/2][AF_ZONE_COLUMN/2].u64h0,
		stAFZ[AF_ZONE_ROW/2][AF_ZONE_COLUMN/2].u64h1);
#endif
	return CVI_SUCCESS;
}

CVI_S32 SAMPLE_AF_Ctrl(CVI_S32 s32Handle, CVI_U32 u32Cmd, CVI_VOID *pValue)
{
	UNUSED(s32Handle);
	UNUSED(u32Cmd);
	UNUSED(pValue);

	//printf("Func %s line %d\n", __func__, __LINE__);
	return CVI_SUCCESS;
}
CVI_S32 SAMPLE_AF_Exit(CVI_S32 s32Handle)
{
	UNUSED(s32Handle);

	printf("Func %s line %d\n", __func__, __LINE__);
	return CVI_SUCCESS;
}

CVI_S32 CVI_AF_Register(VI_PIPE ViPipe, ALG_LIB_S *pstAfLib)
{
	ISP_AF_REGISTER_S stRegister;
	CVI_S32 s32Ret = CVI_SUCCESS;

	UNUSED(ViPipe);
	UNUSED(pstAfLib);

	printf("Func %s line %d\n", __func__, __LINE__);

	stRegister.stAfExpFunc.pfn_af_init = SAMPLE_AF_Init;//OK
	stRegister.stAfExpFunc.pfn_af_run = SAMPLE_AF_Run;
	stRegister.stAfExpFunc.pfn_af_ctrl = SAMPLE_AF_Ctrl;//OK
	stRegister.stAfExpFunc.pfn_af_exit = SAMPLE_AF_Exit;//OK
	s32Ret = CVI_ISP_AFLibRegCallBack(ViPipe, pstAfLib, &stRegister);

	if (s32Ret != CVI_SUCCESS) {
		printf("Cvi_af register failed!\n");
	}

	return s32Ret;
}

CVI_S32 CVI_AF_UnRegister(VI_PIPE ViPipe, ALG_LIB_S *pstAfLib)
{
	ISP_LOG_DEBUG("%s\n", "+");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstAfLib == CVI_NULL) {
		return CVI_FAILURE;
	}


	CVI_S32 s32Ret = CVI_SUCCESS;

	s32Ret = CVI_ISP_AFLibUnRegCallBack(ViPipe, pstAfLib);
	isp_proc_unRegAAAGetVerCb(ViPipe, ISP_AAA_TYPE_AF);

	return s32Ret;
}


