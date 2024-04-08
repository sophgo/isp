/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2023. All rights reserved.
 *
 * File Name: isp_algo_rgbir.h
 * Description:
 *
 */

#ifndef _ISP_ALGO_RGBIR_H_
#define _ISP_ALGO_RGBIR_H_

#include "cvi_comm_isp.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

struct rgbir_param_in {
	//CVI_U32 rsv[32];
};

struct rgbir_param_out {

};

CVI_S32 isp_algo_rgbir_main(
	struct rgbir_param_in *rgbir_param_in, struct rgbir_param_out *rgbir_param_out);
CVI_S32 isp_algo_rgbir_init(void);
CVI_S32 isp_algo_rgbir_uninit(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_ALGO_RGBIR_H_
