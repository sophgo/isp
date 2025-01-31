/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2023. All rights reserved.
 *
 * File Name: isp_rgbir_ctrl.h
 * Description:
 *
 */

#ifndef _ISP_RGBIR_CTRL_H_
#define _ISP_RGBIR_CTRL_H_

#include "cvi_comm_isp.h"
#include "isp_feature_ctrl.h"
#include "isp_algo_rgbir.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

struct isp_rgbir_ctrl_runtime {
	struct rgbir_param_in rgbir_param_in;
	struct rgbir_param_out rgbir_param_out;

	ISP_RGBIR_MANUAL_ATTR_S rgbir_attr;

	CVI_BOOL preprocess_updated;
	CVI_BOOL process_updated;
	CVI_BOOL postprocess_updated;
	CVI_BOOL is_module_bypass;
};

CVI_S32 isp_rgbir_ctrl_init(VI_PIPE ViPipe);
CVI_S32 isp_rgbir_ctrl_uninit(VI_PIPE ViPipe);
CVI_S32 isp_rgbir_ctrl_suspend(VI_PIPE ViPipe);
CVI_S32 isp_rgbir_ctrl_resume(VI_PIPE ViPipe);
CVI_S32 isp_rgbir_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input);

CVI_S32 isp_rgbir_ctrl_get_rgbir_attr(VI_PIPE ViPipe, const ISP_RGBIR_ATTR_S **pstRgbirAttr);
CVI_S32 isp_rgbir_ctrl_set_rgbir_attr(VI_PIPE ViPipe, const ISP_RGBIR_ATTR_S *pstRgbirAttr);

extern const struct isp_module_ctrl rgbir_mod;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_RGBIR_CTRL_H_
