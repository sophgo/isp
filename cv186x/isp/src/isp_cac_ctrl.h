/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_cac_ctrl.h
 * Description:
 *
 */

#ifndef _ISP_CAC_CTRL_H_
#define _ISP_CAC_CTRL_H_

#include "cvi_comm_isp.h"
#include "isp_feature_ctrl.h"
#include "isp_algo_cac.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

struct isp_cac_ctrl_runtime {
	struct cac_param_in cac_param_in;
	struct cac_param_out cac_param_out;

	ISP_CAC_MANUAL_ATTR_S cac_attr;

	CVI_BOOL preprocess_updated;
	CVI_BOOL process_updated;
	CVI_BOOL postprocess_updated;
	CVI_BOOL is_module_bypass;
};

CVI_S32 isp_cac_ctrl_init(VI_PIPE ViPipe);
CVI_S32 isp_cac_ctrl_uninit(VI_PIPE ViPipe);
CVI_S32 isp_cac_ctrl_suspend(VI_PIPE ViPipe);
CVI_S32 isp_cac_ctrl_resume(VI_PIPE ViPipe);
CVI_S32 isp_cac_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input);

CVI_S32 isp_cac_ctrl_get_cac_attr(VI_PIPE ViPipe, const ISP_CAC_ATTR_S **pstCACAttr);
CVI_S32 isp_cac_ctrl_set_cac_attr(VI_PIPE ViPipe, const ISP_CAC_ATTR_S *pstCACAttr);

extern const struct isp_module_ctrl cac_mod;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_CAC_CTRL_H_
