/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_cnr_ctrl.h
 * Description:
 *
 */

#ifndef _ISP_CNR_CTRL_H_
#define _ISP_CNR_CTRL_H_

#include "cvi_comm_isp.h"
#include "isp_feature_ctrl.h"
#include "isp_algo_cnr.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

struct isp_cnr_ctrl_runtime {
	struct cnr_param_in cnr_param_in;
	struct cnr_param_out cnr_param_out;

	ISP_CNR_MANUAL_ATTR_S cnr_attr;
	ISP_CNR_MOTION_NR_MANUAL_ATTR_S cnr_motion_attr;

	CVI_BOOL preprocess_updated;
	CVI_BOOL process_updated;
	CVI_BOOL postprocess_updated;
	CVI_BOOL is_module_bypass;
};

CVI_S32 isp_cnr_ctrl_init(VI_PIPE ViPipe);
CVI_S32 isp_cnr_ctrl_uninit(VI_PIPE ViPipe);
CVI_S32 isp_cnr_ctrl_suspend(VI_PIPE ViPipe);
CVI_S32 isp_cnr_ctrl_resume(VI_PIPE ViPipe);
CVI_S32 isp_cnr_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input);

CVI_S32 isp_cnr_ctrl_get_cnr_attr(VI_PIPE ViPipe, const ISP_CNR_ATTR_S **pstCNRAttr);
CVI_S32 isp_cnr_ctrl_set_cnr_attr(VI_PIPE ViPipe, const ISP_CNR_ATTR_S *pstCNRAttr);
CVI_S32 isp_cnr_ctrl_get_cnr_motion_attr(VI_PIPE ViPipe, const ISP_CNR_MOTION_NR_ATTR_S **pstCNRMotionNRAttr);
CVI_S32 isp_cnr_ctrl_set_cnr_motion_attr(VI_PIPE ViPipe, const ISP_CNR_MOTION_NR_ATTR_S *pstCNRMotionNRAttr);

extern const struct isp_module_ctrl cnr_mod;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_CNR_CTRL_H_
