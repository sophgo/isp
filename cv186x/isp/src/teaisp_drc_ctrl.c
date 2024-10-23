/*
 * Copyright (C) Cvitek Co., Ltd. 2023-2024. All rights reserved.
 *
 * File Name: teaisp_drc_ctrl.c
 * Description:
 *
 */

#include "cvi_type.h"
#include "isp_main_local.h"
#include "isp_debug.h"
#include "isp_defines.h"
#include "cvi_comm_isp.h"
#include "isp_ioctl.h"

#include "isp_proc_local.h"
#include "isp_tun_buf_ctrl.h"
#include "isp_interpolate.h"
#include "cvi_isp.h"

#include "isp_mgr_buf.h"
#include "teaisp_drc_ctrl.h"

const struct isp_module_ctrl teaisp_drc_mod = {
	.init = teaisp_drc_ctrl_init,
	.uninit = teaisp_drc_ctrl_uninit,
	.suspend = teaisp_drc_ctrl_suspend,
	.resume = teaisp_drc_ctrl_resume,
	.ctrl = teaisp_drc_ctrl_ctrl
};

static CVI_S32 teaisp_drc_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 teaisp_drc_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 teaisp_drc_ctrl_process(VI_PIPE ViPipe);
static CVI_S32 teaisp_drc_ctrl_postprocess(VI_PIPE ViPipe);
static CVI_S32 set_drc_proc_info(VI_PIPE ViPipe);

static struct teaisp_drc_ctrl_runtime  *_get_drc_ctrl_runtime(VI_PIPE ViPipe);
static CVI_S32 isp_drc_ctrl_check_drc_attr_valid(const TEAISP_DRC_ATTR_S *pstDrcAttr);

static const CVI_FLOAT global_luma_map[][4] = {
	{-0.1752, 0.7538, 0.411, 0.008768},
	{-0.1639, 0.8268, 0.3295, 0.006203},
	{-0.1395, 0.876, 0.2579, 0.004557},
	{-0.1032, 0.9031, 0.1958, 0.003557},
	{-0.05628, 0.9099, 0.143, 0.002997},
	{0, 0.8985, 0.09883, 0.002718},
	{0.06443, 0.8705, 0.06281, 0.002602},
	{0.1359, 0.8279, 0.03432, 0.002561},
	{0.2133, 0.7723, 0.01274, 0.002531},
	{0.2957, 0.7054, 0.002521, 0.002465},
	{0.3823, 0.6286, 0.01206, 0.002331}
};

static const CVI_FLOAT dark_threshold_map[][6] = {
	{0.0, 0.0, 0.0, 0.0, 0.0, 1.0},
	{0.0, 0.0, 0.0, 0.0, 1.0, 0.0},
	{0.0, 0.0, 0.0, 1.0, 0.0, 0.0},
	{0.0, 0.0, 1.0, 0.0, 0.0, 0.0},
	{0.0, 1.0, 0.0, 0.0, 0.0, 0.0},
	{1.0, 0.0, 0.0, 0.0, 0.0, 0.0}
};

static const CVI_FLOAT bright_threshold_map[][6] = {
	{0.0, 0.0, 0.0, 0.0, 0.0, 1.0},
	{0.0, 0.0, 0.0, 0.0, 1.0, 0.0},
	{0.0, 0.0, 0.0, 1.0, 0.0, 0.0},
	{0.0, 0.0, 1.0, 0.0, 0.0, 0.0},
	{0.0, 1.0, 0.0, 0.0, 0.0, 0.0},
	{1.0, 0.0, 0.0, 0.0, 0.0, 0.0}
};

CVI_S32 teaisp_drc_ctrl_init(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	struct teaisp_drc_ctrl_runtime *runtime = _get_drc_ctrl_runtime(ViPipe);

	runtime->preprocess_updated = CVI_TRUE;
	runtime->process_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_FALSE;
	runtime->is_module_bypass = CVI_FALSE;

	return ret;
}

CVI_S32 teaisp_drc_ctrl_uninit(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 teaisp_drc_ctrl_suspend(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 teaisp_drc_ctrl_resume(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 teaisp_drc_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct teaisp_drc_ctrl_runtime *runtime = _get_drc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	switch (cmd) {
	case MOD_CMD_POST_EOF:
		teaisp_drc_ctrl_post_eof(ViPipe, (ISP_ALGO_RESULT_S *)input);
		break;
	case MOD_CMD_SET_MODCTRL:
		runtime->is_module_bypass = ((ISP_MODULE_CTRL_U *)input)->bitBypassAiDrc;
		break;
	case MOD_CMD_GET_MODCTRL:
		((ISP_MODULE_CTRL_U *)input)->bitBypassAiDrc = runtime->is_module_bypass;
		break;
	default:
		break;
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 teaisp_drc_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;

	ret = teaisp_drc_ctrl_preprocess(ViPipe, algoResult);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = teaisp_drc_ctrl_process(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = teaisp_drc_ctrl_postprocess(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	set_drc_proc_info(ViPipe);

	return ret;
}

static CVI_S32 teaisp_drc_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct teaisp_drc_ctrl_runtime *runtime = _get_drc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	const TEAISP_DRC_ATTR_S *drc_attr = NULL;

	teaisp_drc_ctrl_get_drc_attr(ViPipe, &drc_attr);

	CVI_BOOL is_preprocess_update = CVI_FALSE;
	CVI_U8 intvl = MAX(drc_attr->UpdateInterval, 1);

	is_preprocess_update = ((runtime->preprocess_updated) || ((algoResult->u32FrameIdx % intvl) == 0));

	// No need to update status
	if (is_preprocess_update == CVI_FALSE)
		return ret;

	runtime->preprocess_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_TRUE;

	// No need to update parameters if disable. Because its meaningless
	if (!drc_attr->enable || runtime->is_module_bypass) {
		return ret;
	}

	if (drc_attr->enOpType == OP_TYPE_MANUAL) {
		#define MANUAL(_attr, _param) \
		runtime->_attr._param = _attr->stManual._param

		MANUAL(drc_attr, DarkStrength);

		#undef MANUAL
	} else {
		#define AUTO(_attr, _param, type) \
		runtime->_attr._param = INTERPOLATE_LINEAR(ViPipe, type, _attr->stAuto._param)

		AUTO(drc_attr, DarkStrength, INTPLT_POST_ISO);

		#undef AUTO

	}

	// ParamIn

	CVI_U32 *p_hist256_val;

	ISP_AE_STATISTICS_COMPAT_S *p_ae_sts = NULL;

	isp_sts_ctrl_get_ae_sts(ViPipe, &p_ae_sts);

	if (p_ae_sts == NULL) {
		return CVI_FAILURE;
	}

	p_hist256_val = p_ae_sts->aeStat1[0].au32HistogramMemArray[0];

	runtime->teaisp_drc_param_in.enOpType = drc_attr->enOpType;
	runtime->teaisp_drc_param_in.pHist = ISP_PTR_CAST_PTR(p_hist256_val);
	runtime->teaisp_drc_param_in.DarkCompensateRatio = drc_attr->stAuto.DarkCompensateRatio;
	runtime->teaisp_drc_param_in.StrengthMin = drc_attr->stAuto.StrengthMin;
	runtime->teaisp_drc_param_in.StrengthMax = drc_attr->stAuto.StrengthMax;
	runtime->teaisp_drc_param_in.BrightThreshold = drc_attr->stManual.BrightThreshold;
	runtime->teaisp_drc_param_in.SatuStrength = drc_attr->stManual.SatuStrength;

	runtime->process_updated = CVI_TRUE;

	return ret;
}

static CVI_S32 teaisp_drc_ctrl_process(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct teaisp_drc_ctrl_runtime *runtime = _get_drc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (runtime->process_updated == CVI_FALSE)
		return ret;

	isp_algo_teaisp_drc_main(
		(struct teaisp_drc_param_in *)&runtime->teaisp_drc_param_in,
		(struct teaisp_drc_param_out *)&runtime->teaisp_drc_param_out
	);

	runtime->process_updated = CVI_FALSE;

	return ret;
}

static CVI_S32 teaisp_drc_ctrl_postprocess(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct teaisp_drc_ctrl_runtime *runtime = _get_drc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	const TEAISP_DRC_ATTR_S * drc_attr = NULL;

	teaisp_drc_ctrl_get_drc_attr(ViPipe, &drc_attr);

	TEAISP_DRC_PARAM_UPDATE_CALLBACK callback = (TEAISP_DRC_PARAM_UPDATE_CALLBACK) runtime->callback;

	CVI_BOOL is_postprocess_update = (runtime->postprocess_updated == CVI_TRUE);

	if (callback == NULL) {
		is_postprocess_update = CVI_FALSE;
	}

	if (is_postprocess_update == CVI_FALSE) {

	} else {

		CVI_FLOAT *temp = runtime->param;

		*temp++ = (CVI_FLOAT) drc_attr->enable;

		*temp++ = (CVI_FLOAT) ((CVI_FLOAT) drc_attr->DeflickStrength / 128.0);
		*temp++ = (CVI_FLOAT) ((CVI_FLOAT) drc_attr->DetailStrength / 128.0);

		for (int i = 0; i < 4; i++) {
			*temp++ = global_luma_map[drc_attr->GlobalLuma - 15][i];
		}

		*temp++ = (CVI_FLOAT) ((CVI_FLOAT) runtime->drc_attr.DarkStrength / 128.0);
		*temp++ = (CVI_FLOAT) ((CVI_FLOAT) drc_attr->BrightStrength / 128.0);
		*temp++ = (CVI_FLOAT) ((CVI_FLOAT) runtime->teaisp_drc_param_out.SatuStrength / 128.0);
		*temp++ = (CVI_FLOAT) ((CVI_FLOAT) drc_attr->MaxvStrength / 128.0);

		for (int i = 0; i < 6; i++) {
			*temp++ = dark_threshold_map[drc_attr->DarkThreshold][i];
		}

		for (int i = 0; i < 6; i++) {
			*temp++ = bright_threshold_map[runtime->teaisp_drc_param_out.BrightThreshold][i];
		}

		//printf("AIDRC PARAM:");
		//for (int i = 0; i < (int)((temp - runtime->param)); i++) {
		//	printf(" %f,", runtime->param[i]);
		//}
		//printf("\n");

		callback(ViPipe, runtime->param);
	}

	runtime->postprocess_updated = CVI_FALSE;

	return ret;
}

static CVI_S32 set_drc_proc_info(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

static struct teaisp_drc_ctrl_runtime  *_get_drc_ctrl_runtime(VI_PIPE ViPipe)
{
	CVI_BOOL isVipipeValid = ((ViPipe >= 0) && (ViPipe < VI_MAX_PIPE_NUM));

	if (!isVipipeValid) {
		ISP_LOG_WARNING("Wrong ViPipe(%d)\n", ViPipe);
		return NULL;
	}

	struct teaisp_drc_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_TEAISP_DRC, (CVI_VOID *) &shared_buffer);

	return &shared_buffer->runtime;
}

static CVI_S32 isp_drc_ctrl_check_drc_attr_valid(const TEAISP_DRC_ATTR_S *pstDrcAttr)
{
	// only check for StrengthMin and StrengthMax, making sure StrengthMin < StrengthMax.
	CVI_S32 ret = CVI_SUCCESS;
	int StrengthMin = (int)pstDrcAttr->stAuto.StrengthMin;
	int StrengthMax = (int)pstDrcAttr->stAuto.StrengthMax;

	if (StrengthMax - StrengthMin <= 0) {
		ISP_VALUE_CHECK_ERR_LOG_V(STRING(pstDrcAttr->stAuto.StrengthMin), pstDrcAttr->stAuto.StrengthMin);
		ISP_VALUE_CHECK_ERR_LOG_V(STRING(pstDrcAttr->stAuto.StrengthMax), pstDrcAttr->stAuto.StrengthMax);
		ret = CVI_FAILURE_ILLEGAL_PARAM;
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  public functions, set or get param
//-----------------------------------------------------------------------------
CVI_S32 teaisp_drc_ctrl_reg_callback(VI_PIPE ViPipe, TEAISP_DRC_PARAM_UPDATE_CALLBACK callback)
{
	struct teaisp_drc_ctrl_runtime *runtime = _get_drc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	runtime->callback = (ISP_VOID_PTR) callback;

	return CVI_SUCCESS;
}

CVI_S32 teaisp_drc_ctrl_get_drc_attr(VI_PIPE ViPipe, const TEAISP_DRC_ATTR_S **pstDrcAttr)
{
	if (pstDrcAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	struct teaisp_drc_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_TEAISP_DRC, (CVI_VOID *) &shared_buffer);

	*pstDrcAttr = &shared_buffer->stDrcAttr;

	return ret;
}

CVI_S32 teaisp_drc_ctrl_set_drc_attr(VI_PIPE ViPipe, const TEAISP_DRC_ATTR_S *pstDrcAttr)
{
	if (pstDrcAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct teaisp_drc_ctrl_runtime *runtime = _get_drc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = isp_drc_ctrl_check_drc_attr_valid(pstDrcAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	const TEAISP_DRC_ATTR_S *p = CVI_NULL;

	teaisp_drc_ctrl_get_drc_attr(ViPipe, &p);
	memcpy((CVI_VOID *)p, pstDrcAttr, sizeof(*pstDrcAttr));

	runtime->preprocess_updated = CVI_TRUE;

	return ret;
}

