
#include <string.h>
#include "isp_ipc_server_cmd.h"
#include "isp_ipc_common.h"
#include "isp_mgr_buf.h"
#include "3A_internal.h"

#include "cvi_ae.h"
#include "cvi_awb.h"
#include "cvi_bin.h"

#ifdef ENABLE_ISP_IPC

#ifndef UNUSED
#define UNUSED(x) ((void)(x))
#endif

static CVI_S32 server_cmd_init(CVI_U8 *arg, CVI_U32 arg_len, CVI_U8 *res, CVI_U32 res_len)
{
	VI_PIPE ViPipe;
	CVI_U64 u64PhyAddr = 0x00;
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(res_len);
	UNUSED(arg_len);

	memcpy(&ViPipe, arg, sizeof(VI_PIPE));

	ret = isp_mgr_buf_get_shared_buf_paddr(ViPipe, &u64PhyAddr);

	memcpy(res, &u64PhyAddr, sizeof(CVI_U64));

	return ret;
}

static CVI_S32 server_cmd_exit(CVI_U8 *arg, CVI_U32 arg_len, CVI_U8 *res, CVI_U32 res_len)
{
	UNUSED(arg);
	UNUSED(arg_len);
	UNUSED(res);
	UNUSED(res_len);

	return CVI_SUCCESS;
}

static CVI_S32 server_cmd_get_ae_log_buf_size(CVI_U8 *arg, CVI_U32 arg_len, CVI_U8 *res, CVI_U32 res_len)
{
	CVI_S32 ret = CVI_SUCCESS;
	VI_PIPE ViPipe;
	CVI_U32 bufSize;

	UNUSED(arg_len);

	memcpy(&ViPipe, arg, sizeof(VI_PIPE));

	ret = CVI_ISP_GetAELogBufSize(ViPipe, &bufSize);

	memcpy(res, &bufSize, res_len);

	return ret;
}

static CVI_S32 server_cmd_get_ae_log_buf(CVI_U8 *arg, CVI_U32 arg_len, CVI_U8 *res, CVI_U32 res_len)
{
	CVI_S32 ret = CVI_SUCCESS;
	ISP_IPC_GET_AE_LOG_BUF_ARGS_S *args;

	UNUSED(arg_len);

	args = (ISP_IPC_GET_AE_LOG_BUF_ARGS_S *) arg;

	ret = CVI_ISP_GetAELogBuf(args->ViPipe, res, res_len);

	return ret;
}

static CVI_S32 server_cmd_get_ae_bin_buf_size(CVI_U8 *arg, CVI_U32 arg_len, CVI_U8 *res, CVI_U32 res_len)
{
	CVI_S32 ret = CVI_SUCCESS;
	VI_PIPE ViPipe;
	CVI_U32 bufSize;

	UNUSED(arg_len);

	memcpy(&ViPipe, arg, sizeof(VI_PIPE));

	ret = CVI_ISP_GetAEBinBufSize(ViPipe, &bufSize);

	memcpy(res, &bufSize, res_len);

	return ret;
}

static CVI_S32 server_cmd_get_ae_bin_buf(CVI_U8 *arg, CVI_U32 arg_len, CVI_U8 *res, CVI_U32 res_len)
{
	CVI_S32 ret = CVI_SUCCESS;
	ISP_IPC_GET_AE_BIN_BUF_ARGS_S *args;

	UNUSED(arg);
	UNUSED(arg_len);

	args = (ISP_IPC_GET_AE_BIN_BUF_ARGS_S *) arg;

	ret = CVI_ISP_GetAEBinBuf(args->ViPipe, res, res_len);

	return ret;
}

static CVI_S32 server_cmd_get_ae_frame_id(CVI_U8 *arg, CVI_U32 arg_len, CVI_U8 *res, CVI_U32 res_len)
{
	CVI_S32 ret = CVI_SUCCESS;
	VI_PIPE ViPipe;
	CVI_U32 frameID;

	UNUSED(arg_len);

	memcpy(&ViPipe, arg, sizeof(VI_PIPE));

	ret = CVI_ISP_GetFrameID(ViPipe, &frameID);

	memcpy(res, &frameID, res_len);

	return ret;
}

static CVI_S32 server_cmd_get_ae_fps(CVI_U8 *arg, CVI_U32 arg_len, CVI_U8 *res, CVI_U32 res_len)
{
	CVI_S32 ret = CVI_SUCCESS;
	VI_PIPE ViPipe;
	CVI_FLOAT fps;

	UNUSED(arg_len);

	memcpy(&ViPipe, arg, sizeof(VI_PIPE));

	ret = CVI_ISP_QueryFps(ViPipe, &fps);

	memcpy(res, &fps, res_len);

	return ret;
}

static CVI_S32 server_cmd_get_ae_lvx100(CVI_U8 *arg, CVI_U32 arg_len, CVI_U8 *res, CVI_U32 res_len)
{
	CVI_S32 ret = CVI_SUCCESS;
	VI_PIPE ViPipe;
	CVI_S16 lv;

	UNUSED(arg_len);

	memcpy(&ViPipe, arg, sizeof(VI_PIPE));

	ret = CVI_ISP_GetCurrentLvX100(ViPipe, &lv);

	memcpy(res, &lv, res_len);

	return ret;
}

static CVI_S32 server_cmd_set_ae_raw_dump_frame_id(CVI_U8 *arg, CVI_U32 arg_len, CVI_U8 *res, CVI_U32 res_len)
{
	CVI_S32 ret = CVI_SUCCESS;
	ISP_IPC_SET_AE_RAW_DUMP_FRAME_ID_ARGS_S *args;

	UNUSED(arg_len);
	UNUSED(res);
	UNUSED(res_len);

	args = (ISP_IPC_SET_AE_RAW_DUMP_FRAME_ID_ARGS_S *) arg;

	ret = CVI_ISP_AESetRawDumpFrameID(args->ViPipe, args->fid, args->frmNum);

	return ret;
}

static CVI_S32 server_cmd_get_ae_raw_replay_exp_buf(CVI_U8 *arg, CVI_U32 arg_len, CVI_U8 *res, CVI_U32 res_len)
{
	CVI_S32 ret = CVI_SUCCESS;
	ISP_IPC_GET_AE_RAW_REPLAY_EXP_BUF_ARGS_S *args;
	ISP_IPC_GET_AE_RAW_REPLAY_EXP_BUF_RES_S *ress;

	UNUSED(arg_len);
	UNUSED(res_len);

	args = (ISP_IPC_GET_AE_RAW_REPLAY_EXP_BUF_ARGS_S *) arg;
	ress = (ISP_IPC_GET_AE_RAW_REPLAY_EXP_BUF_RES_S *) res;

	ret = CVI_ISP_AEGetRawReplayExpBuf(args->ViPipe, &ress->buf, &args->bufSize);

	ress->bufSize = args->bufSize;

	return ret;
}

static CVI_S32 server_cmd_get_awb_log_buf(CVI_U8 *arg, CVI_U32 arg_len, CVI_U8 *res, CVI_U32 res_len)
{
	CVI_S32 ret = CVI_SUCCESS;
	ISP_IPC_GET_AWB_LOG_BUF_ARGS_S *args;

	UNUSED(arg_len);

	args = (ISP_IPC_GET_AWB_LOG_BUF_ARGS_S *) arg;

	ret = CVI_ISP_GetAWBSnapLogBuf(args->ViPipe, res, res_len);

	return ret;
}

static CVI_S32 server_cmd_get_awb_dbg_bin_buf_size(CVI_U8 *arg, CVI_U32 arg_len, CVI_U8 *res, CVI_U32 res_len)
{
	CVI_S32 ret = CVI_SUCCESS;
	CVI_U32 bufSize;

	UNUSED(arg);
	UNUSED(arg_len);

	bufSize = CVI_ISP_GetAWBDbgBinSize();

	memcpy(res, &bufSize, res_len);

	return ret;
}

static CVI_S32 server_cmd_get_awb_dbg_bin_buf(CVI_U8 *arg, CVI_U32 arg_len, CVI_U8 *res, CVI_U32 res_len)
{
	CVI_S32 ret = CVI_SUCCESS;
	ISP_IPC_GET_AWB_DBG_BIN_BUF_ARGS_S *args;

	UNUSED(arg_len);

	args = (ISP_IPC_GET_AWB_DBG_BIN_BUF_ARGS_S *) arg;

	ret = CVI_ISP_GetAWBDbgBinBuf(args->ViPipe, res, res_len);

	return ret;
}

static CVI_S32 server_cmd_bin_get_total_len(CVI_U8 *arg, CVI_U32 arg_len, CVI_U8 *res, CVI_U32 res_len)
{
	CVI_U32 binSize = 0;

	UNUSED(arg);
	UNUSED(arg_len);

	binSize = CVI_BIN_GetBinTotalLen();

	memcpy(res, &binSize, res_len);

	return CVI_SUCCESS;
}

static CVI_S32 server_cmd_bin_export_bin(CVI_U8 *arg, CVI_U32 arg_len, CVI_U8 *res, CVI_U32 res_len)
{
	CVI_S32 s32Ret = CVI_SUCCESS;

	UNUSED(arg);
	UNUSED(arg_len);

	s32Ret = CVI_BIN_ExportBinData(res, res_len);

	return s32Ret;
}

static CVI_S32 server_cmd_bin_get_bin_name(CVI_U8 *arg, CVI_U32 arg_len, CVI_U8 *res, CVI_U32 res_len)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	char szBinFileName[128] = {'\0'};

	UNUSED(arg);
	UNUSED(arg_len);

	s32Ret = CVI_BIN_GetBinName(szBinFileName);

	memcpy(res, szBinFileName, res_len);

	return s32Ret;
}

static CVI_S32 server_cmd_bin_import_bin(CVI_U8 *arg, CVI_U32 arg_len, CVI_U8 *res, CVI_U32 res_len)
{
	CVI_S32 s32Ret = CVI_SUCCESS;

	UNUSED(res);
	UNUSED(res_len);

	s32Ret = CVI_BIN_ImportBinData(arg, arg_len);

	return s32Ret;
}

static ISP_IPC_CMD_ITEM_S server_cmd_list[] = {
	{ISP_IPC_INIT, server_cmd_init},
	{ISP_IPC_EXIT, server_cmd_exit},
	{ISP_IPC_GET_AE_LOG_BUF_SIZE, server_cmd_get_ae_log_buf_size},
	{ISP_IPC_GET_AE_LOG_BUF, server_cmd_get_ae_log_buf},
	{ISP_IPC_GET_AE_BIN_BUF_SIZE, server_cmd_get_ae_bin_buf_size},
	{ISP_IPC_GET_AE_BIN_BUF, server_cmd_get_ae_bin_buf},
	{ISP_IPC_GET_AE_FRAME_ID, server_cmd_get_ae_frame_id},
	{ISP_IPC_GET_AE_FPS, server_cmd_get_ae_fps},
	{ISP_IPC_GET_AE_LVX100, server_cmd_get_ae_lvx100},
	{ISP_IPC_SET_AE_RAW_DUMP_FRAME_ID, server_cmd_set_ae_raw_dump_frame_id},
	{ISP_IPC_GET_AE_RAW_REPLAY_EXP_BUF, server_cmd_get_ae_raw_replay_exp_buf},
	{ISP_IPC_GET_AWB_LOG_BUF, server_cmd_get_awb_log_buf},
	{ISP_IPC_GET_AWB_DBG_BIN_BUF_SIZE, server_cmd_get_awb_dbg_bin_buf_size},
	{ISP_IPC_GET_AWB_DBG_BIN_BUF, server_cmd_get_awb_dbg_bin_buf},
	{ISP_IPC_BIN_GET_TOTAL_LEN, server_cmd_bin_get_total_len},
	{ISP_IPC_BIN_EXPORT_BIN, server_cmd_bin_export_bin},
	{ISP_IPC_BIN_GET_BIN_NAME, server_cmd_bin_get_bin_name},
	{ISP_IPC_BIN_IMPORT_BIN, server_cmd_bin_import_bin},
};

void isp_ipc_server_cmd_init(void)
{
	isp_ipc_reg_server_cmd(server_cmd_list, ISP_IPC_CMD_MAX);
}

#endif

