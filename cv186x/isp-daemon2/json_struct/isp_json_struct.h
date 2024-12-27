/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_json_struct.h
 * Description:
 *
 */

#ifndef _ISP_JSON_STRUCT_H
#define _ISP_JSON_STRUCT_H

#include "cvi_json_struct_comm.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

void ISP_EXP_INFO_S_JSON(int r_w_flag, JSON *j, char *key, ISP_EXP_INFO_S *data);
void ISP_WB_INFO_S_JSON(int r_w_flag, JSON *j, char *key, ISP_WB_INFO_S *data);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_JSON_STRUCT_H
