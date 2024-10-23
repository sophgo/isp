/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2023. All rights reserved.
 *
 * File Name: isp_ipc_sdk_wrap.h
 * Description:
 *
 */

#ifndef _ISP_IPC_SDK_WRAP_H_
#define _ISP_IPC_SDK_WRAP_H_

#include "isp_comm_inc.h"
#include "cvi_bin.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#ifdef ENABLE_ISP_IPC

CVI_U32 CVI_BIN_GetBinTotalLenWrap(void);
CVI_U32 CVI_BIN_GetSingleISPBinLenWrap(enum CVI_BIN_SECTION_ID id);
CVI_S32 CVI_BIN_ExportBinDataWrap(CVI_U8 *pu8Buffer, CVI_U32 u32DataLength);
CVI_S32 CVI_BIN_ExportSingleISPBinDataWrap(enum CVI_BIN_SECTION_ID id, CVI_U8 *pu8Buffer, CVI_U32 u32DataLength);
CVI_S32 CVI_BIN_GetBinNameWrap(CVI_CHAR *binName);
CVI_S32 CVI_BIN_ImportBinDataWrap(CVI_U8 *pu8Buffer, CVI_U32 u32DataLength);
CVI_S32 CVI_BIN_LoadParamFromBinExWrap(enum CVI_BIN_SECTION_ID id, CVI_U8 *pu8Buffer, CVI_U32 u32DataLength);

#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif


