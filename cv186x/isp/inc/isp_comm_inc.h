/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: include/isp_comm_inc.h
 * Description:
 */

#ifndef __ISP_COMM_INC_H__
#define __ISP_COMM_INC_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#include "stddef.h"
#include "stdint.h"
#include <sys/time.h>

#include <cvi_common.h>
#include <cvi_comm_vi.h>
#include <cvi_comm_video.h>
#include <cvi_defines.h>
#include <cvi_math.h>
#include <vi_isp.h>
#include <vi_tun_cfg.h>
#ifdef V4L2_ISP_ENABLE
#include <vi_v4l2_uapi.h>
#else
#include <vi_uapi.h>
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* __CVI_COMM_INC_H__ */
