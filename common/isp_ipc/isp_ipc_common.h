/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2023. All rights reserved.
 *
 * File Name: isp_ipc_common.h
 * Description:
 *
 */

#ifndef _ISP_IPC_COMMON_H_
#define _ISP_IPC_COMMON_H_

#include "isp_comm_inc.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#ifdef ENABLE_ISP_IPC

typedef struct {
	CVI_S32 pid; // auto set
	CVI_U8 type; // auto set
	CVI_U8 id; // client to server auto set, server to client set by user
	CVI_U32 cmd; // set by user
	CVI_U32 arg_len; // set by user
	CVI_U32 res_len; // set by user
} ISP_IPC_MSG_S;

typedef CVI_S32 (*ISP_IPC_FUN)(CVI_U8 *arg, CVI_U32 arg_len, CVI_U8 *res, CVI_U32 res_len);

typedef struct {
	CVI_U32 cmd;
	ISP_IPC_FUN fun;
} ISP_IPC_CMD_ITEM_S;

// server call
CVI_S32 isp_ipc_reg_server_cmd(ISP_IPC_CMD_ITEM_S *cmd_list, CVI_U32 cmd_len);
// client call
CVI_S32 isp_ipc_reg_client_cmd(ISP_IPC_CMD_ITEM_S *cmd_list, CVI_U32 cmd_len);

CVI_S32 isp_ipc_server_init(void);
CVI_S32 isp_ipc_server_deinit(void);

CVI_S32 isp_ipc_client_init(void);
CVI_S32 isp_ipc_client_deinit(void);

// exec reg client cmd fun
CVI_S32 server_send_cmd_to_client(ISP_IPC_MSG_S *msg, CVI_U8 *arg, CVI_U8 *res);

// exec reg server cmd fun
CVI_S32 client_send_cmd_to_server(ISP_IPC_MSG_S *msg, CVI_U8 *arg, CVI_U8 *res);
CVI_S32 client_reg_server_exit_callback(ISP_IPC_FUN fun);

#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif

