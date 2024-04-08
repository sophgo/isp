/*
 * Copyright (C) Cvitek Co., Ltd. 2013-2024. All rights reserved.
 *
 * File Name: teaisp_raw_test.h
 * Description:
 *
 */

#ifndef _TEAISP_RAW_TEST_H_
#define _TEAISP_RAW_TEST_H_

#include "cvi_type.h"
#include "isp_comm_inc.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

//#define ENABLE_TEAISP_RAW_REPLAY_TEST 1

CVI_S32 teaisp_raw_inference_test(const CVI_VOID *header, CVI_VOID *data,
							CVI_U32 totalFrame, CVI_U32 curFrame, CVI_U32 rawFrameSize);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _RAW_REPLAY_H_

