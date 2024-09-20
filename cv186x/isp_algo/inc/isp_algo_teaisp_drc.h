/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_teaisp_drc.h
 * Description:
 *
 */

#ifndef _ISP_ALGO_TEAISP_DRC_H_
#define _ISP_ALGO_TEAISP_DRC_H_

#include "cvi_comm_isp.h"
#include "isp_defines.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

struct teaisp_drc_param_in {
	ISP_OP_TYPE_E enOpType;
	ISP_U32_PTR pHist;
	CVI_U8 DarkCompensateRatio;
	CVI_U8 StrengthMin;
	CVI_U8 StrengthMax;
	CVI_U8 BrightThreshold;
	CVI_U8 SatuStrength;
};

struct teaisp_drc_param_out {
	CVI_U8 BrightThreshold;
	CVI_U8 SatuStrength;
};

CVI_S32 isp_algo_teaisp_drc_main(struct teaisp_drc_param_in *teaisp_drc_param_in, struct teaisp_drc_param_out *teaisp_drc_param_out);
CVI_S32 isp_algo_teaisp_drc_init(void);
CVI_S32 isp_algo_teaisp_drc_uninit(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_ALGO_TEAISP_DRC_H_
