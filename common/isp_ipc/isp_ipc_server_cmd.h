/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2023. All rights reserved.
 *
 * File Name: isp_ipc_server_cmd.h
 * Description:
 *
 */

#ifndef _ISP_IPC_SERVER_CMD_H_
#define _ISP_IPC_SERVER_CMD_H_

#include "isp_comm_inc.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#ifdef ENABLE_ISP_IPC

typedef enum {
	ISP_IPC_INIT,
	ISP_IPC_EXIT,
	ISP_IPC_GET_AE_LOG_BUF_SIZE,
	ISP_IPC_GET_AE_LOG_BUF,
	ISP_IPC_GET_AE_BIN_BUF_SIZE,
	ISP_IPC_GET_AE_BIN_BUF,
	ISP_IPC_GET_AE_FRAME_ID,
	ISP_IPC_GET_AE_FPS,
	ISP_IPC_GET_AE_LVX100,
	ISP_IPC_SET_AE_RAW_DUMP_FRAME_ID,
	ISP_IPC_GET_AE_RAW_REPLAY_EXP_BUF,
	ISP_IPC_GET_AWB_LOG_BUF,
	ISP_IPC_GET_AWB_DBG_BIN_BUF_SIZE,
	ISP_IPC_GET_AWB_DBG_BIN_BUF,
	ISP_IPC_BIN_GET_TOTAL_LEN,
	ISP_IPC_BIN_EXPORT_BIN,
	ISP_IPC_BIN_GET_BIN_NAME,
	ISP_IPC_BIN_IMPORT_BIN,
	ISP_IPC_CMD_MAX
} ISP_IPC_CMD_E;

typedef struct {
	VI_PIPE ViPipe;
	CVI_U32 bufSize;
} ISP_IPC_GET_AE_LOG_BUF_ARGS_S;

typedef struct {
	VI_PIPE ViPipe;
	CVI_U32 bufSize;
} ISP_IPC_GET_AE_BIN_BUF_ARGS_S;

typedef struct {
	VI_PIPE ViPipe;
	CVI_U32 fid;
	CVI_U16 frmNum;
} ISP_IPC_SET_AE_RAW_DUMP_FRAME_ID_ARGS_S;

typedef struct {
	VI_PIPE ViPipe;
	CVI_U32 bufSize;
} ISP_IPC_GET_AE_RAW_REPLAY_EXP_BUF_ARGS_S;

typedef struct {
	CVI_U32 bufSize;
	CVI_U8 buf;
} ISP_IPC_GET_AE_RAW_REPLAY_EXP_BUF_RES_S;

typedef struct {
	VI_PIPE ViPipe;
	CVI_U32 bufSize;
} ISP_IPC_GET_AWB_LOG_BUF_ARGS_S;

typedef struct {
	VI_PIPE ViPipe;
	CVI_U32 bufSize;
} ISP_IPC_GET_AWB_DBG_BIN_BUF_ARGS_S;

void isp_ipc_server_cmd_init(void);

#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif


