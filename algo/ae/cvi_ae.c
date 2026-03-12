/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: cvi_ae.c
 * Description:
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

//#include "cvi_ae_comm.h"
#include "cvi_isp.h"
#include "cvi_comm_3a.h"
#include "cvi_comm_sns.h"
#include "isp_comm_inc.h"
#include "aealgo.h"
#include "ae_iris.h"
#include "cvi_ae.h"
#include "cvi_awb.h"

#include "isp_main.h"

#include "isp_defines.h"
#include "isp_debug.h"
#include "isp_ipc.h"
#include "isp_proc.h"
#ifdef ENABLE_ISP_IPC
#include "isp_ipc_common.h"
#include "isp_mgr_buf.h"
#include "isp_ipc_server_cmd.h"
#endif

CVI_S32 CVI_AE_Register(VI_PIPE ViPipe, ALG_LIB_S *pstAeLib)
{
	ISP_LOG_DEBUG("%s\n", "+");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstAeLib == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 s32Ret = CVI_SUCCESS;
	ISP_AE_REGISTER_S aeAlgo;

	aeAlgo.stAeExpFunc.pfn_ae_init = aeInit;
	aeAlgo.stAeExpFunc.pfn_ae_run = aeRun;
	aeAlgo.stAeExpFunc.pfn_ae_ctrl = aeCtrl;
	aeAlgo.stAeExpFunc.pfn_ae_exit = aeExit;

	s32Ret = CVI_ISP_AELibRegCallBack(ViPipe, pstAeLib, &aeAlgo);

#ifndef ARCH_RTOS_CV181X // TODO@CV181X
	ISP_AE_DEBUG_FUNC_S aeDebug;

	aeDebug.pfn_ae_dumplog = AE_DumpLog;
	aeDebug.pfn_ae_setdumplogpath = AE_SetDumpLogPath;
	CVI_ISP_AELibRegInternalCallBack(ViPipe, &aeDebug);

	isp_proc_regAAAGetVerCb(ViPipe, ISP_AAA_TYPE_AE, AE_GetAlgoVer);
#endif

	return s32Ret;
}

CVI_S32 CVI_AE_UnRegister(VI_PIPE ViPipe, ALG_LIB_S *pstAeLib)
{
	ISP_LOG_DEBUG("%s\n", "+");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstAeLib == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 s32Ret = CVI_SUCCESS;

	s32Ret = CVI_ISP_AELibUnRegCallBack(ViPipe, pstAeLib);
#ifndef ARCH_RTOS_CV181X // TODO@CV181X
	CVI_ISP_AELibUnRegInternalCallBack(ViPipe);
	isp_proc_unRegAAAGetVerCb(ViPipe, ISP_AAA_TYPE_AE);
#endif

	return s32Ret;
}

CVI_S32 CVI_AE_SensorRegCallBack(VI_PIPE ViPipe, ALG_LIB_S *pstAeLib, ISP_SNS_ATTR_INFO_S *pstSnsAttrInfo,
				     AE_SENSOR_REGISTER_S *pstRegister)
{
	ISP_LOG_DEBUG("%s\n", "+");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstAeLib == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (pstSnsAttrInfo == CVI_NULL) {
		return CVI_FAILURE;
	}

	AE_CTX_S *pstAeCtx;
	CVI_S32 s32Ret = 0;

	pstAeCtx = AE_GET_CTX(ViPipe);
	memcpy((CVI_VOID *)(&(pstAeCtx->stSnsAttrInfo)), (CVI_VOID *)pstSnsAttrInfo, sizeof(ISP_SNS_ATTR_INFO_S));
	/*TODO. BindDev Seems not set to ViPipe.*/
	pstAeCtx->IspBindDev = ViPipe;
	if (pstRegister == CVI_NULL) {
		ISP_LOG_ERR("AE Register callback is NULL\n");
		return CVI_FAILURE;
	}
	memcpy(&(pstAeCtx->stSnsRegister), pstRegister, sizeof(AE_SENSOR_REGISTER_S));

	if (pstAeCtx->stSnsRegister.stAeExp.pfn_cmos_get_ae_default != CVI_NULL)
		pstAeCtx->stSnsRegister.stAeExp.pfn_cmos_get_ae_default(ViPipe, &pstAeCtx->stSnsDft);
	pstAeCtx->bSnsRegister = 1;
	return s32Ret;
}

CVI_S32 CVI_AE_SensorUnRegCallBack(VI_PIPE ViPipe, ALG_LIB_S *pstAeLib, SENSOR_ID SensorId)
{
	ISP_LOG_NOTICE("%s %d\n", "+", SensorId);
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstAeLib == CVI_NULL) {
		return CVI_FAILURE;
	}

	AE_CTX_S *pstAeCtx;
	/*TODO. Need check HI "s32handle" means.*/
	pstAeCtx = AE_GET_CTX(ViPipe);
	memset(&(pstAeCtx->stSnsRegister), 0, sizeof(AE_SENSOR_REGISTER_S));
	memset(&(pstAeCtx->stSnsDft), 0, sizeof(AE_SENSOR_DEFAULT_S));
	pstAeCtx->bSnsRegister = 0;

	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_SetExposureAttr(VI_PIPE ViPipe, const ISP_EXPOSURE_ATTR_S *pstExpAttr)
{
	ISP_LOG_DEBUG("%s\n", "+");
	if (pstExpAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (pstExpAttr->stAuto.stExpTimeRange.u32Max < pstExpAttr->stAuto.stExpTimeRange.u32Min) {
		return CVI_FAILURE;
	} else if (pstExpAttr->stAuto.stISONumRange.u32Max < pstExpAttr->stAuto.stISONumRange.u32Min) {
		return CVI_FAILURE;
	} else if (pstExpAttr->stAuto.stAGainRange.u32Max < pstExpAttr->stAuto.stAGainRange.u32Min) {
		return CVI_FAILURE;
	} else if (pstExpAttr->stAuto.stDGainRange.u32Max < pstExpAttr->stAuto.stDGainRange.u32Min) {
		return CVI_FAILURE;
	} else if (pstExpAttr->stAuto.stISPDGainRange.u32Max < pstExpAttr->stAuto.stISPDGainRange.u32Min) {
		return CVI_FAILURE;
	} else if (pstExpAttr->stAuto.stSysGainRange.u32Max < pstExpAttr->stAuto.stSysGainRange.u32Min) {
		return CVI_FAILURE;
	}

	CVI_U8 sID = AE_ViPipe2sID(ViPipe);

	return AE_SetExposureAttr(sID, pstExpAttr);
}

CVI_S32 CVI_ISP_GetExposureAttr(VI_PIPE ViPipe, ISP_EXPOSURE_ATTR_S *pstExpAttr)
{
	ISP_LOG_DEBUG("%s\n", "+");
	if (pstExpAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_U8 sID = AE_ViPipe2sID(ViPipe);

	return AE_GetExposureAttr(sID, pstExpAttr);
}

CVI_S32 CVI_ISP_QueryExposureInfo(VI_PIPE ViPipe, ISP_EXP_INFO_S *pstExpInfo)
{
	if (pstExpInfo == CVI_NULL) {
		return CVI_FAILURE;
	}


	CVI_U8 sID = AE_ViPipe2sID(ViPipe);

	return AE_GetExposureInfo(sID, pstExpInfo);
}

CVI_S32 CVI_ISP_SetWDRExposureAttr(VI_PIPE ViPipe, const ISP_WDR_EXPOSURE_ATTR_S *pstWDRExpAttr)
{
	CVI_S32 res = CVI_SUCCESS;

	ISP_LOG_DEBUG("%s\n", "+");
	if (pstWDRExpAttr == CVI_NULL) {
		return CVI_FAILURE;
	}


#if !SIMULATION
	CVI_U8 sID = AE_ViPipe2sID(ViPipe);

	res = AE_SetWDRExposureAttr(sID, pstWDRExpAttr);
#endif
	return res;
}

CVI_S32 CVI_ISP_GetWDRExposureAttr(VI_PIPE ViPipe, ISP_WDR_EXPOSURE_ATTR_S *pstWDRExpAttr)
{
	CVI_S32 res = CVI_SUCCESS;

	ISP_LOG_DEBUG("%s\n", "+");
	if (pstWDRExpAttr == CVI_NULL) {
		return CVI_FAILURE;
	}


#if !SIMULATION
	CVI_U8 sID = AE_ViPipe2sID(ViPipe);

	res = AE_GetWDRExposureAttr(sID, pstWDRExpAttr);
#endif

	return res;
}

CVI_S32 CVI_ISP_SetAERouteAttr(VI_PIPE ViPipe, const ISP_AE_ROUTE_S *pstAERouteAttr)
{
	ISP_LOG_DEBUG("%s\n", "+");
	if (pstAERouteAttr == CVI_NULL) {
		return CVI_FAILURE;
	}


	CVI_U8 sID = AE_ViPipe2sID(ViPipe);

	return AE_SetRouteAttr(sID, pstAERouteAttr);
}

CVI_S32 CVI_ISP_GetAERouteAttr(VI_PIPE ViPipe, ISP_AE_ROUTE_S *pstAERouteAttr)
{
	ISP_LOG_DEBUG("%s\n", "+");
	if (pstAERouteAttr == CVI_NULL) {
		return CVI_FAILURE;
	}


	CVI_U8 sID = AE_ViPipe2sID(ViPipe);

	return AE_GetRouteAttr(sID, pstAERouteAttr);
}

CVI_S32 CVI_ISP_SetAERouteAttrEx(VI_PIPE ViPipe, const ISP_AE_ROUTE_EX_S *pstAERouteAttrEx)
{
	ISP_LOG_DEBUG("%s\n", "+");
	if (pstAERouteAttrEx == CVI_NULL) {
		return CVI_FAILURE;
	}


	CVI_U8 sID = AE_ViPipe2sID(ViPipe);

	return AE_SetRouteAttrEx(sID, pstAERouteAttrEx);
}

CVI_S32 CVI_ISP_GetAERouteAttrEx(VI_PIPE ViPipe, ISP_AE_ROUTE_EX_S *pstAERouteAttrEx)
{
	ISP_LOG_DEBUG("%s\n", "+");
	if (pstAERouteAttrEx == CVI_NULL) {
		return CVI_FAILURE;
	}


	CVI_U8 sID = AE_ViPipe2sID(ViPipe);

	return AE_GetRouteAttrEx(sID, pstAERouteAttrEx);
}

CVI_S32 CVI_ISP_SetAERouteSFAttr(VI_PIPE ViPipe, const ISP_AE_ROUTE_S *pstAERouteSFAttr)
{
	ISP_LOG_DEBUG("%s\n", "+");
	if (pstAERouteSFAttr == CVI_NULL) {
		return CVI_FAILURE;
	}


	CVI_U8 sID = AE_ViPipe2sID(ViPipe);

	return AE_SetRouteSFAttr(sID, pstAERouteSFAttr);
}

CVI_S32 CVI_ISP_GetAERouteSFAttr(VI_PIPE ViPipe, ISP_AE_ROUTE_S *pstAERouteSFAttr)
{
	ISP_LOG_DEBUG("%s\n", "+");
	if (pstAERouteSFAttr == CVI_NULL) {
		return CVI_FAILURE;
	}


	CVI_U8 sID = AE_ViPipe2sID(ViPipe);

	return AE_GetRouteSFAttr(sID, pstAERouteSFAttr);
}

CVI_S32 CVI_ISP_SetAERouteSFAttrEx(VI_PIPE ViPipe, const ISP_AE_ROUTE_EX_S *pstAERouteSFAttrEx)
{
	ISP_LOG_DEBUG("%s\n", "+");
	if (pstAERouteSFAttrEx == CVI_NULL) {
		return CVI_FAILURE;
	}


	CVI_U8 sID = AE_ViPipe2sID(ViPipe);

	return AE_SetRouteSFAttrEx(sID, pstAERouteSFAttrEx);
}

CVI_S32 CVI_ISP_GetAERouteSFAttrEx(VI_PIPE ViPipe, ISP_AE_ROUTE_EX_S *pstAERouteSFAttrEx)
{
	ISP_LOG_DEBUG("%s\n", "+");
	if (pstAERouteSFAttrEx == CVI_NULL) {
		return CVI_FAILURE;
	}


	CVI_U8 sID = AE_ViPipe2sID(ViPipe);

	return AE_GetRouteSFAttrEx(sID, pstAERouteSFAttrEx);
}


CVI_S32 CVI_ISP_SetSmartExposureAttr(VI_PIPE ViPipe, const ISP_SMART_EXPOSURE_ATTR_S *pstSmartExpAttr)
{
	ISP_LOG_DEBUG("%s\n", "+");
	if (pstSmartExpAttr == CVI_NULL) {
		return CVI_FAILURE;
	}


	CVI_U8 sID = AE_ViPipe2sID(ViPipe);

	return AE_SetSmartExposureAttr(sID, pstSmartExpAttr);
}

CVI_S32 CVI_ISP_GetSmartExposureAttr(VI_PIPE ViPipe, ISP_SMART_EXPOSURE_ATTR_S *pstSmartExpAttr)
{
	ISP_LOG_DEBUG("%s\n", "+");
	if (pstSmartExpAttr == CVI_NULL) {
		return CVI_FAILURE;
	}


	CVI_U8 sID = AE_ViPipe2sID(ViPipe);

	return AE_GetSmartExposureAttr(sID, pstSmartExpAttr);
}



CVI_S32 CVI_ISP_AEBracketingStart(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");

	CVI_U8 sID = AE_ViPipe2sID(ViPipe);

	AE_BracketingStart(sID);
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_AEBracketingSetExpsoure(VI_PIPE ViPipe, CVI_S16 leEvX10, CVI_S16 seEvX10)
{
	ISP_LOG_DEBUG("+\n");

	CVI_U8 sID = AE_ViPipe2sID(ViPipe);
	CVI_S32 result;

	result = AE_BracketingSetExposure(sID, leEvX10, seEvX10);
	return result;
}

CVI_S32 CVI_ISP_AEBracketingSetSimple(CVI_BOOL bEnable)
{
	ISP_LOG_DEBUG("+\n");

	AE_BracketingSetSimple(bEnable);

	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_AEBracketingFinish(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");

	CVI_U8 sID = AE_ViPipe2sID(ViPipe);

	AE_BracketingSetExposureFinish(sID);
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_GetAELogBuf(VI_PIPE ViPipe, CVI_U8 *pBuf, CVI_U32 bufSize)
{
	if (pBuf == CVI_NULL) {
		return CVI_FAILURE;
	}
#ifdef ENABLE_ISP_IPC
	ISP_IPC_MSG_S msg;
	CVI_S32 ret = CVI_SUCCESS;

	if (isp_mgr_buf_is_client(ViPipe)) {

		ISP_IPC_GET_AE_LOG_BUF_ARGS_S args;

		args.ViPipe = ViPipe;
		args.bufSize = bufSize;

		memset(&msg, 0, sizeof(ISP_IPC_MSG_S));
		msg.cmd = ISP_IPC_GET_AE_LOG_BUF;
		msg.arg_len = sizeof(ISP_IPC_GET_AE_LOG_BUF_ARGS_S);
		msg.res_len = bufSize;

		ret = client_send_cmd_to_server(&msg, (CVI_U8 *) &args, pBuf);

		return ret;

	} else {
		return AE_GetLogBuf(ViPipe, pBuf, bufSize);
	}
#else
	return AE_GetLogBuf(ViPipe, pBuf, bufSize);
#endif
}


CVI_S32 CVI_ISP_GetAELogBufSize(VI_PIPE ViPipe, CVI_U32 *bufSize)
{
	if (bufSize == CVI_NULL) {
		return CVI_FAILURE;
	}
#ifdef ENABLE_ISP_IPC
	ISP_IPC_MSG_S msg;
	CVI_S32 ret = CVI_SUCCESS;

	if (isp_mgr_buf_is_client(ViPipe)) {

		memset(&msg, 0, sizeof(ISP_IPC_MSG_S));
		msg.cmd = ISP_IPC_GET_AE_LOG_BUF_SIZE;
		msg.arg_len = sizeof(VI_PIPE);
		msg.res_len = sizeof(CVI_U32);

		ret = client_send_cmd_to_server(&msg, (CVI_U8 *) &ViPipe, (CVI_U8 *) bufSize);

		return ret;

	} else {
		return AE_GetLogBufSize(ViPipe, bufSize);
	}
#else
	return AE_GetLogBufSize(ViPipe, bufSize);
#endif
}

CVI_S32 CVI_ISP_GetAEBinBuf(VI_PIPE ViPipe, CVI_U8 *pBuf, CVI_U32 bufSize)
{
	if (pBuf == CVI_NULL) {
		return CVI_FAILURE;
	}
#ifdef ENABLE_ISP_IPC
	ISP_IPC_MSG_S msg;
	CVI_S32 ret = CVI_SUCCESS;

	if (isp_mgr_buf_is_client(ViPipe)) {

		ISP_IPC_GET_AE_BIN_BUF_ARGS_S args;

		args.ViPipe = ViPipe;
		args.bufSize = bufSize;

		memset(&msg, 0, sizeof(ISP_IPC_MSG_S));
		msg.cmd = ISP_IPC_GET_AE_BIN_BUF;
		msg.arg_len = sizeof(ISP_IPC_GET_AE_BIN_BUF_ARGS_S);
		msg.res_len = bufSize;

		ret = client_send_cmd_to_server(&msg, (CVI_U8 *) &args, pBuf);

		return ret;

	} else {
		return AE_GetBinBuf(ViPipe, pBuf, bufSize);
	}
#else
	return AE_GetBinBuf(ViPipe, pBuf, bufSize);
#endif
}


CVI_S32 CVI_ISP_GetAEBinBufSize(VI_PIPE ViPipe, CVI_U32 *bufSize)
{
	if (bufSize == CVI_NULL) {
		return CVI_FAILURE;
	}
#ifdef ENABLE_ISP_IPC
	ISP_IPC_MSG_S msg;
	CVI_S32 ret = CVI_SUCCESS;

	if (isp_mgr_buf_is_client(ViPipe)) {

		memset(&msg, 0, sizeof(ISP_IPC_MSG_S));
		msg.cmd = ISP_IPC_GET_AE_BIN_BUF_SIZE;
		msg.arg_len = sizeof(VI_PIPE);
		msg.res_len = sizeof(CVI_U32);

		ret = client_send_cmd_to_server(&msg, (CVI_U8 *) &ViPipe, (CVI_U8 *) bufSize);

		return ret;

	} else {
		return AE_GetBinBufSize(ViPipe, bufSize);
	}
#else
	return AE_GetBinBufSize(ViPipe, bufSize);
#endif
}


CVI_S32 CVI_ISP_AESetRawDumpFrameID(VI_PIPE ViPipe, CVI_U32 fid, CVI_U16 frmNum)
{
	CVI_U8 sID = AE_ViPipe2sID(ViPipe);
#ifdef ENABLE_ISP_IPC
	ISP_IPC_MSG_S msg;
	CVI_S32 ret = CVI_SUCCESS;

	if (isp_mgr_buf_is_client(ViPipe)) {

		ISP_IPC_SET_AE_RAW_DUMP_FRAME_ID_ARGS_S args;

		args.ViPipe = ViPipe;
		args.fid = fid;
		args.frmNum = frmNum;

		memset(&msg, 0, sizeof(ISP_IPC_MSG_S));
		msg.cmd = ISP_IPC_SET_AE_RAW_DUMP_FRAME_ID;
		msg.arg_len = sizeof(ISP_IPC_SET_AE_RAW_DUMP_FRAME_ID_ARGS_S);
		msg.res_len = 0;

		ret = client_send_cmd_to_server(&msg, (CVI_U8 *) &args, NULL);

		return ret;

	} else {
		return AE_SetRawDumpFrameID(sID, fid, frmNum);
	}
#else
	return AE_SetRawDumpFrameID(sID, fid, frmNum);
#endif
}

CVI_S32 CVI_ISP_AEMallocRawReplayExpBuf(VI_PIPE ViPipe, CVI_U16 frmNum)
{
	CVI_U8 sID = AE_ViPipe2sID(ViPipe);

	return AE_MallocRawReplayExpBuf(sID, frmNum);
}

CVI_S32 CVI_ISP_AEGetRawReplayExpBuf(VI_PIPE ViPipe, CVI_U8 *buf, CVI_U32 *bufSize)
{
	if (buf == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_U8 sID = AE_ViPipe2sID(ViPipe);
#ifdef ENABLE_ISP_IPC
	ISP_IPC_MSG_S msg;
	CVI_S32 ret = CVI_SUCCESS;

	if (isp_mgr_buf_is_client(ViPipe)) {

		ISP_IPC_GET_AE_RAW_REPLAY_EXP_BUF_ARGS_S args;
		ISP_IPC_GET_AE_RAW_REPLAY_EXP_BUF_RES_S *res;

		args.ViPipe = ViPipe;
		args.bufSize = *bufSize;

		CVI_U32 tempbufSize = *bufSize + sizeof(CVI_U32);
		CVI_U8 *tempbuf = calloc(1, tempbufSize);

		memset(&msg, 0, sizeof(ISP_IPC_MSG_S));
		msg.cmd = ISP_IPC_GET_AE_RAW_REPLAY_EXP_BUF;
		msg.arg_len = sizeof(ISP_IPC_GET_AE_RAW_REPLAY_EXP_BUF_ARGS_S);
		msg.res_len = tempbufSize;

		ret = client_send_cmd_to_server(&msg, (CVI_U8 *) &args, tempbuf);

		res = (ISP_IPC_GET_AE_RAW_REPLAY_EXP_BUF_RES_S *) tempbuf;

		*bufSize = res->bufSize;

		memcpy(buf, &res->buf, *bufSize);

		free(tempbuf);

		return ret;

	} else {
		return AE_GetRawReplayExpBuf(sID, buf, bufSize);
	}
#else
	return AE_GetRawReplayExpBuf(sID, buf, bufSize);
#endif
}

void CVI_ISP_AESetRawReplayMode(VI_PIPE ViPipe, CVI_BOOL bMode)
{
	CVI_U8 sID = AE_ViPipe2sID(ViPipe);

	AE_SetRawReplayMode(sID, bMode);
}

CVI_S32 CVI_ISP_AESetRawReplayExposure(VI_PIPE ViPipe, const ISP_EXP_INFO_S *pstExpInfo)
{
	CVI_U8 sID = AE_ViPipe2sID(ViPipe);

	return AE_SetRawReplayExposure(sID, pstExpInfo);
}






#if 0
CVI_S32 CVI_ISP_SetFaceAeInfo(VI_PIPE ViPipe, const CVI_ISP_FACE_DETECT_INFO *pstFaceInfo)
{
	ISP_LOG_DEBUG("%s\n", "+");
	if (pstFaceInfo == CVI_NULL) {
		return CVI_FAILURE;
	}


	CVI_U8 sID = AE_ViPipe2sID(ViPipe);

	AE_SetFaceInfo(sID, pstFaceInfo);
	CVI_ISP_SetFaceAwbInfo(ViPipe, pstFaceInfo);
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_GetFaceAeInfo(VI_PIPE ViPipe, CVI_ISP_FACE_DETECT_INFO *pstFaceInfo)
{
	if (pstFaceInfo == CVI_NULL) {
		return CVI_FAILURE;
	}


	CVI_U8 sID = AE_ViPipe2sID(ViPipe);

	AE_GetFaceInfo(sID, pstFaceInfo);

	return CVI_SUCCESS;
}
#endif

CVI_S32 CVI_ISP_SetAEStatisticsConfig(VI_PIPE ViPipe, const ISP_AE_STATISTICS_CFG_S *pstAeStatCfg)
{
	ISP_LOG_DEBUG("%s\n", "+");
	if (pstAeStatCfg == CVI_NULL) {
		return CVI_FAILURE;
	}


	CVI_U8 sID = AE_ViPipe2sID(ViPipe);

	return AE_SetStatisticsConfig(sID, pstAeStatCfg);
}

CVI_S32 CVI_ISP_GetAEStatisticsConfig(VI_PIPE ViPipe, ISP_AE_STATISTICS_CFG_S *pstAeStatCfg)
{
	if (pstAeStatCfg == CVI_NULL) {
		return CVI_FAILURE;
	}


	CVI_U8 sID = AE_ViPipe2sID(ViPipe);

	return AE_GetStatisticsConfig(sID, pstAeStatCfg);
}

CVI_S32 CVI_ISP_SetAntiFlicker(VI_PIPE ViPipe, CVI_BOOL enable, CVI_U8 frequency)
{
	ISP_LOG_DEBUG("%s\n", "+");

	ISP_EXPOSURE_ATTR_S aeAttr;
	ISP_AE_ANTIFLICKER_FREQUENCE_E freq = AE_FREQUENCE_60HZ;
	CVI_S32 res;

	if (frequency == 50)
		freq = AE_FREQUENCE_50HZ;
	else if (frequency == 60)
		freq = AE_FREQUENCE_60HZ;

	res = CVI_ISP_GetExposureAttr(ViPipe, &aeAttr);

	if (res == CVI_SUCCESS) {
		aeAttr.stAuto.stAntiflicker.bEnable = enable;
		aeAttr.stAuto.stAntiflicker.enMode = ISP_ANTIFLICKER_AUTO_MODE;
		aeAttr.stAuto.stAntiflicker.enFrequency = freq;
		res = CVI_ISP_SetExposureAttr(ViPipe, &aeAttr);
	}

	return res;
}

CVI_S32 CVI_ISP_GetAntiFlicker(VI_PIPE ViPipe, CVI_BOOL *pEnable, CVI_U8 *pFrequency)
{
	if (pEnable == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (pFrequency == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 res;
	ISP_EXPOSURE_ATTR_S aeAttr;

	res = CVI_ISP_GetExposureAttr(ViPipe, &aeAttr);
	*pEnable = aeAttr.stAuto.stAntiflicker.bEnable;
	*pFrequency = (aeAttr.stAuto.stAntiflicker.enFrequency == AE_FREQUENCE_50HZ) ? 50 : 60;

	return res;
}

CVI_S32 CVI_ISP_SetIrisAttr(VI_PIPE ViPipe, const ISP_IRIS_ATTR_S *pstIrisAttr)
{
	CVI_S32 res;

	if (pstIrisAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_U8 sID = AE_ViPipe2sID(ViPipe);

	res = AE_SetIrisAttr(sID, pstIrisAttr);
	return res;
}

CVI_S32 CVI_ISP_GetIrisAttr(VI_PIPE ViPipe, ISP_IRIS_ATTR_S *pstIrisAttr)
{
	CVI_S32 res;

	if (pstIrisAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_U8 sID = AE_ViPipe2sID(ViPipe);

	res = AE_GetIrisAttr(sID, pstIrisAttr);
	return res;
}

CVI_S32 CVI_ISP_SetDcirisAttr(VI_PIPE ViPipe, const ISP_DCIRIS_ATTR_S *pstDcirisAttr)
{
	CVI_S32 res;

	if (pstDcirisAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_U8 sID = AE_ViPipe2sID(ViPipe);

	res = AE_SetDcIrisAttr(sID, pstDcirisAttr);
	return res;
}

CVI_S32 CVI_ISP_GetDcirisAttr(VI_PIPE ViPipe, ISP_DCIRIS_ATTR_S *pstDcirisAttr)
{
	CVI_S32 res;

	if (pstDcirisAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_U8 sID = AE_ViPipe2sID(ViPipe);

	res = AE_GetDcIrisAttr(sID, pstDcirisAttr);
	return res;
}

CVI_S32 CVI_ISP_SetDCIrisPwmPath(char *szPath)
{
	CVI_S32 res;

	res = AE_SetDCIrisPwmCtrlPath(szPath);
	return res;
}

CVI_S32 CVI_ISP_SetWDRLEOnly(VI_PIPE ViPipe, CVI_BOOL wdrLEOnly)
{
	ISP_LOG_DEBUG("+\n");

	CVI_U8 sID = AE_ViPipe2sID(ViPipe);

	AE_SetWDRLEOnly(sID, wdrLEOnly);

	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_GetFrameID(VI_PIPE ViPipe, CVI_U32 *frameID)
{
	CVI_U8 sID = AE_ViPipe2sID(ViPipe);
#ifdef ENABLE_ISP_IPC
	ISP_IPC_MSG_S msg;
	CVI_S32 ret = CVI_SUCCESS;

	if (isp_mgr_buf_is_client(ViPipe)) {

		memset(&msg, 0, sizeof(ISP_IPC_MSG_S));
		msg.cmd = ISP_IPC_GET_AE_FRAME_ID;
		msg.arg_len = sizeof(VI_PIPE);
		msg.res_len = sizeof(CVI_U32);

		ret = client_send_cmd_to_server(&msg, (CVI_U8 *) &ViPipe, (CVI_U8 *) frameID);

		return ret;

	} else {
		AE_GetFrameID(sID, frameID);
	}
#else
	AE_GetFrameID(sID, frameID);
#endif
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_SetAELogPath(const char *szPath)
{
	CVI_S32 result;

	result = AE_SetDumpLogPath(szPath);
	return result;
}

CVI_S32 CVI_ISP_GetAELogPath(char *szPath, CVI_U32 pathSize)
{
	CVI_S32 result;

	result = AE_GetDumpLogPath(szPath, pathSize);
	return result;
}


CVI_S32 CVI_ISP_SetAELogName(const char *szName)
{
	CVI_S32 result;

	result = AE_SetDumpLogName(szName);
	return result;
}

CVI_S32 CVI_ISP_GetAELogName(char *szName, CVI_U32 nameSize)
{
	CVI_S32 result;

	result = AE_GetDumpLogName(szName, nameSize);
	return result;
}

CVI_S32 CVI_ISP_QueryFps(VI_PIPE ViPipe, CVI_FLOAT *pFps)
{
	if (pFps == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_U8 sID = AE_ViPipe2sID(ViPipe);
#ifdef ENABLE_ISP_IPC
	ISP_IPC_MSG_S msg;
	CVI_S32 ret = CVI_SUCCESS;

	if (isp_mgr_buf_is_client(ViPipe)) {

		memset(&msg, 0, sizeof(ISP_IPC_MSG_S));
		msg.cmd = ISP_IPC_GET_AE_FPS;
		msg.arg_len = sizeof(VI_PIPE);
		msg.res_len = sizeof(CVI_FLOAT);

		ret = client_send_cmd_to_server(&msg, (CVI_U8 *) &ViPipe, (CVI_U8 *) pFps);

		return ret;

	} else {
		AE_GetFps(sID, pFps);
	}
#else
	AE_GetFps(sID, pFps);
#endif

	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_GetCurrentLvX100(VI_PIPE ViPipe, CVI_S16 *ps16Lv)
{
	CVI_U8 sID = AE_ViPipe2sID(ViPipe);
#ifdef ENABLE_ISP_IPC
	ISP_IPC_MSG_S msg;
	CVI_S32 ret = CVI_SUCCESS;

	if (isp_mgr_buf_is_client(ViPipe)) {

		memset(&msg, 0, sizeof(ISP_IPC_MSG_S));
		msg.cmd = ISP_IPC_GET_AE_LVX100;
		msg.arg_len = sizeof(VI_PIPE);
		msg.res_len = sizeof(CVI_S16);

		ret = client_send_cmd_to_server(&msg, (CVI_U8 *) &ViPipe, (CVI_U8 *) ps16Lv);

		return ret;

	} else {
		*ps16Lv = AE_GetCurrentLvX100(sID);
	}
#else
	*ps16Lv = AE_GetCurrentLvX100(sID);
#endif
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_SetFastBootExposure(VI_PIPE ViPipe, CVI_U32 expLine, CVI_U32 again, CVI_U32 dgain, CVI_U32 ispdgain)
{
	AE_SetFastBootExposure(ViPipe, expLine, again, dgain, ispdgain);

	return CVI_SUCCESS;
}

void CVI_AE_GenNewRaw(void *pDstOri, void *pSrcOri, CVI_U32 sizeBk, CVI_U32 mode, CVI_U32 w, CVI_U32 h, CVI_U32 blc)
{
	GenNewRaw(pDstOri, pSrcOri, sizeBk, mode, w, h, blc);
}

void CVI_AE_SetAeSimMode(CVI_BOOL bMode)
{
	AE_SetAeSimMode(bMode);
}

CVI_BOOL CVI_AE_IsAeSimMode(void)
{
	return AE_IsAeSimMode();
}

