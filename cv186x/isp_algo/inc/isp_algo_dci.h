/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_dci.h
 * Description:
 *
 */

#ifndef _ISP_ALGO_DCI_H_
#define _ISP_ALGO_DCI_H_

#include "cvi_comm_isp.h"
#include "isp_defines.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

struct dci_param_in {
	ISP_U32_PTR pHist;
	ISP_U16_PTR curLut;
	ISP_U32_PTR preLut;
	ISP_DCI_CURVE_MODE_E mode;
	CVI_U8 method;
	CVI_U8 speed;
	CVI_U16 strength;
	CVI_BOOL bUpdateCurve;
};

struct dci_param_out {
	ISP_U16_PTR outLut;
};

CVI_S32 isp_algo_dci_main(
	struct dci_param_in *dci_param_in, struct dci_param_out *dci_param_out);
CVI_S32 isp_algo_dci_init(void);
CVI_S32 isp_algo_dci_uninit(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_ALGO_DCI_H_
