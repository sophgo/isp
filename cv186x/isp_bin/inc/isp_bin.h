/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_bin.h
 * Description:
 *
 */

#ifndef _ISP_BIN_H
#define _ISP_BIN_H

#include <cvi_common.h>
#include "cvi_bin.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

CVI_S32 isp_json_getParamFromJsonbuffer(const char *buffer, enum CVI_BIN_SECTION_ID id);
CVI_S32 isp_json_setParamToJsonbuffer(CVI_S8 **buffer, enum CVI_BIN_SECTION_ID id, CVI_S32 *len);
CVI_S32 header_bin_getBinSize(enum CVI_BIN_SECTION_ID id, CVI_U32 *binSize);
CVI_S32 header_bin_getBinParam(enum CVI_BIN_SECTION_ID id, CVI_U8 *addr, CVI_U32 binSize);
CVI_S32 header_bin_GetHeaderDescInfo(CVI_CHAR *pOutDesc, enum CVI_BIN_CREATMODE inCreatMode);

CVI_S32 isp_bin_getBinSize(enum CVI_BIN_SECTION_ID id, CVI_U32 *binSize);
CVI_S32 isp_bin_getBinParam(enum CVI_BIN_SECTION_ID id, CVI_U8 *addr, CVI_U32 binSize);
CVI_S32 header_bin_setBinParam(enum CVI_BIN_SECTION_ID id, FILE *fp);
CVI_S32 isp_bin_setBinParam(enum CVI_BIN_SECTION_ID id, FILE *fp);
CVI_S32 header_bin_setBinParambuf(enum CVI_BIN_SECTION_ID id, CVI_U8 *buffer);
CVI_S32 isp_bin_setBinParambuf(enum CVI_BIN_SECTION_ID id, CVI_U8 *buffer);

CVI_S32 isp_bin_setBinBypassParams(VI_PIPE ViPipe, ISP_BIN_BYPASS_U *ispBinBypass);
CVI_S32 isp_bin_getBinBypassParams(VI_PIPE ViPipe, ISP_BIN_BYPASS_U *ispBinBypass);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_BIN_H
