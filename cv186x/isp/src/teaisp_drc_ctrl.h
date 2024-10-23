/*
 * Copyright (C) Cvitek Co., Ltd. 2023-2024. All rights reserved.
 *
 * File Name: teaisp_drc_ctrl.h
 * Description:
 *
 */

#ifndef _TEAISP_DRC_CTRL_H_
#define _TEAISP_DRC_CTRL_H_

#include "isp_feature_ctrl.h"
#include "isp_algo_teaisp_drc.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

struct teaisp_drc_ctrl_runtime {
	struct teaisp_drc_param_in teaisp_drc_param_in;
	struct teaisp_drc_param_out teaisp_drc_param_out;
	TEAISP_DRC_MANUAL_ATTR_S drc_attr;

	CVI_BOOL preprocess_updated;
	CVI_BOOL process_updated;
	CVI_BOOL postprocess_updated;
	CVI_BOOL is_module_bypass;

	CVI_FLOAT param[TEAISP_DRC_PARAM_LENGHT];
	ISP_VOID_PTR callback;
};

CVI_S32 teaisp_drc_ctrl_init(VI_PIPE ViPipe);
CVI_S32 teaisp_drc_ctrl_uninit(VI_PIPE ViPipe);
CVI_S32 teaisp_drc_ctrl_suspend(VI_PIPE ViPipe);
CVI_S32 teaisp_drc_ctrl_resume(VI_PIPE ViPipe);
CVI_S32 teaisp_drc_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input);

CVI_S32 teaisp_drc_ctrl_reg_callback(VI_PIPE ViPipe, TEAISP_DRC_PARAM_UPDATE_CALLBACK callback);
CVI_S32 teaisp_drc_ctrl_get_drc_attr(VI_PIPE ViPipe, const TEAISP_DRC_ATTR_S **pstDrcAttr);
CVI_S32 teaisp_drc_ctrl_set_drc_attr(VI_PIPE ViPipe, const TEAISP_DRC_ATTR_S *pstDrcAttr);

extern const struct isp_module_ctrl teaisp_drc_mod;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_BNR_CTRL_H_

