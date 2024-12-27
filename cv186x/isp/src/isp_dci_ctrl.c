/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_dci_ctrl.c
 * Description:
 *
 */

#include <math.h>

#include "isp_main_local.h"
#include "isp_debug.h"
#include "isp_defines.h"
#include "cvi_comm_isp.h"
#include "isp_ioctl.h"

#include "isp_proc_local.h"
#include "isp_tun_buf_ctrl.h"
#include "isp_interpolate.h"
#include "isp_dci_ctrl.h"
#include "isp_mgr_buf.h"

const struct isp_module_ctrl dci_mod = {
	.init = isp_dci_ctrl_init,
	.uninit = isp_dci_ctrl_uninit,
	.suspend = isp_dci_ctrl_suspend,
	.resume = isp_dci_ctrl_resume,
	.ctrl = isp_dci_ctrl_ctrl
};

static CVI_S32 isp_dci_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_dci_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_dci_ctrl_process(VI_PIPE ViPipe);
static CVI_S32 isp_dci_ctrl_postprocess(VI_PIPE ViPipe);

static CVI_S32 set_dci_proc_info(VI_PIPE ViPipe);
static CVI_S32 isp_dci_ctrl_check_dci_attr_valid(const ISP_DCI_ATTR_S *pstdciAttr);

static struct isp_dci_ctrl_runtime  *_get_dci_ctrl_runtime(VI_PIPE ViPipe);

CVI_S32 isp_dci_ctrl_init(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	struct isp_dci_ctrl_runtime *runtime = _get_dci_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	memset(runtime->cur_lut, 0, sizeof(CVI_U16) * DCI_BINS_NUM);
	memset(runtime->pre_lut, 0, sizeof(CVI_U32) * DCI_BINS_NUM);
	memset(runtime->out_lut, 0, sizeof(CVI_U16) * DCI_BINS_NUM);
	memset(&runtime->dciStatsInfoBuf, 0, sizeof(ISP_DCI_STATISTICS_S));
	runtime->reset_iir = CVI_TRUE;

	runtime->preprocess_updated = CVI_TRUE;
	runtime->process_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_FALSE;
	runtime->is_module_bypass = CVI_FALSE;

	isp_algo_dci_init();

	return ret;
}

CVI_S32 isp_dci_ctrl_uninit(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	isp_algo_dci_uninit();

	return ret;
}

CVI_S32 isp_dci_ctrl_suspend(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_dci_ctrl_resume(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_dci_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_dci_ctrl_runtime *runtime = _get_dci_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	switch (cmd) {
	case MOD_CMD_POST_EOF:
		isp_dci_ctrl_post_eof(ViPipe, (ISP_ALGO_RESULT_S *)input);
		break;
	case MOD_CMD_SET_MODCTRL:
		runtime->is_module_bypass = ((ISP_MODULE_CTRL_U *)input)->bitBypassDci;
		break;
	case MOD_CMD_GET_MODCTRL:
		((ISP_MODULE_CTRL_U *)input)->bitBypassDci = runtime->is_module_bypass;
		break;
	default:
		break;
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_dci_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_dci_ctrl_preprocess(ViPipe, algoResult);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = isp_dci_ctrl_process(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = isp_dci_ctrl_postprocess(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	set_dci_proc_info(ViPipe);

	return ret;
}

static CVI_S32 isp_dci_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_dci_ctrl_runtime *runtime = _get_dci_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	const ISP_DCI_ATTR_S *dci_attr = NULL;

	isp_dci_ctrl_get_dci_attr(ViPipe, &dci_attr);

	CVI_U32 speed = dci_attr->Speed;
	CVI_BOOL checkSkipDCIGenCurveStep = CVI_TRUE;

	if (dci_attr->Enable && !runtime->is_module_bypass) {
		if (runtime->reset_iir) {
			speed = 0;
			checkSkipDCIGenCurveStep = CVI_FALSE;
			runtime->reset_iir = CVI_FALSE;
		}
	} else {
		runtime->reset_iir = CVI_TRUE;
	}

	CVI_BOOL is_preprocess_update = CVI_FALSE;
	CVI_U8 intvl = MAX(dci_attr->UpdateInterval, 1);
	CVI_U8 frame_delay = (intvl < speed) ? 1 : (intvl / MAX(speed, 1));

	is_preprocess_update = ((runtime->preprocess_updated) || ((algoResult->u32FrameIdx % frame_delay) == 0));

	// No need to update status
	if (is_preprocess_update == CVI_FALSE)
		return ret;

	runtime->preprocess_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_TRUE;

	// No need to update parameters if disable. Because its meaningless
	if (!dci_attr->Enable || runtime->is_module_bypass)
		return ret;


	// ParamIn
	ISP_DCI_STATISTICS_S *dciStatsInfo;

	isp_sts_ctrl_get_dci_sts(ViPipe, &dciStatsInfo);

	runtime->dci_param_in.pHist = ISP_PTR_CAST_PTR(dciStatsInfo->hist);
	runtime->dci_param_in.curLut = ISP_PTR_CAST_PTR(runtime->cur_lut);
	runtime->dci_param_in.preLut = ISP_PTR_CAST_PTR(runtime->pre_lut);
	runtime->dci_param_in.mode = dci_attr->CurveMode;
	runtime->dci_param_in.method = dci_attr->Method;
	runtime->dci_param_in.speed = speed;
	runtime->dci_param_in.strength = dci_attr->DciStrength;
	runtime->dci_param_in.bUpdateCurve = CVI_TRUE;

	// ParamOut
	runtime->dci_param_out.outLut = ISP_PTR_CAST_PTR(runtime->out_lut);

	if (checkSkipDCIGenCurveStep) {
		CVI_U32 threshold = (0xFF - dci_attr->Sensitivity) << 4;
		CVI_U32 diff;

		runtime->dci_param_in.bUpdateCurve = CVI_FALSE;
		for (CVI_U32 i = 0 ; i < DCI_BINS_NUM; i++) {
			if (runtime->dciStatsInfoBuf.hist[i] >= dciStatsInfo->hist[i])
				diff = runtime->dciStatsInfoBuf.hist[i] - dciStatsInfo->hist[i];
			else
				diff = dciStatsInfo->hist[i] - runtime->dciStatsInfoBuf.hist[i];

			if (diff > threshold) {
				memcpy(&runtime->dciStatsInfoBuf, dciStatsInfo, sizeof(*dciStatsInfo));
				runtime->dci_param_in.bUpdateCurve = CVI_TRUE;
				break;
			}
		}
	} else {
		memcpy(&runtime->dciStatsInfoBuf, dciStatsInfo, sizeof(*dciStatsInfo));
	}

	runtime->process_updated = CVI_TRUE;

	return ret;
}

static CVI_S32 isp_dci_ctrl_process(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_dci_ctrl_runtime *runtime = _get_dci_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (runtime->process_updated == CVI_FALSE)
		return ret;

	ret = isp_algo_dci_main(
		(struct dci_param_in *)&runtime->dci_param_in,
		(struct dci_param_out *)&runtime->dci_param_out
	);

	runtime->process_updated = CVI_FALSE;

	return ret;
}

static CVI_S32 isp_dci_ctrl_postprocess(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_dci_ctrl_runtime *runtime = _get_dci_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	struct cvi_vip_isp_post_cfg *post_addr = get_post_tuning_buf_addr(ViPipe);
	CVI_U8 tun_idx = get_tuning_buf_idx(ViPipe);

	struct cvi_vip_isp_dci_config *dci_cfg =
		(struct cvi_vip_isp_dci_config *)&(post_addr->tun_cfg[tun_idx].dci_cfg);

	const ISP_DCI_ATTR_S *dci_attr = NULL;

	isp_dci_ctrl_get_dci_attr(ViPipe, &dci_attr);

	CVI_BOOL is_postprocess_update = ((runtime->postprocess_updated == CVI_TRUE) || (IS_MULTI_CAM()));

	if (is_postprocess_update == CVI_FALSE) {
		dci_cfg->update = 0;
	} else {
		dci_cfg->update = 1;
		dci_cfg->enable = dci_attr->Enable && !runtime->is_module_bypass;
		dci_cfg->map_enable = dci_attr->Enable && !runtime->is_module_bypass;
		dci_cfg->demo_mode = dci_attr->TuningMode;
		if (dci_attr->CurveMode) {
			memcpy(dci_cfg->map_lut, dci_attr->DciGammaCurve, sizeof(CVI_U16) * DCI_BINS_NUM);
		} else {
			memcpy(dci_cfg->map_lut, runtime->out_lut, sizeof(CVI_U16) * DCI_BINS_NUM);
		}

		dci_cfg->per1sample_enable = 1;
		dci_cfg->hist_enable = 1;
	}

	runtime->postprocess_updated = CVI_FALSE;

	return ret;
}

static CVI_S32 set_dci_proc_info(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;

	if (ISP_GET_PROC_ACTION(ViPipe, PROC_LEVEL_1)) {
		struct isp_dci_ctrl_runtime *runtime = _get_dci_ctrl_runtime(ViPipe);

		const ISP_DCI_ATTR_S *dci_attr = NULL;
		ISP_DEBUGINFO_PROC_S *pProcST = NULL;

		if (runtime == CVI_NULL) {
			return CVI_FAILURE;
		}

		isp_dci_ctrl_get_dci_attr(ViPipe, &dci_attr);
		ISP_GET_PROC_INFO(ViPipe, pProcST);

		//common
		pProcST->DCIEnable = dci_attr->Enable;
		pProcST->DCIDciStrength = dci_attr->DciStrength;
	}

	return ret;
}

static struct isp_dci_ctrl_runtime *_get_dci_ctrl_runtime(VI_PIPE ViPipe)
{
	CVI_BOOL isVipipeValid = ((ViPipe >= 0) && (ViPipe < VI_MAX_PIPE_NUM));

	if (!isVipipeValid) {
		ISP_LOG_WARNING("Wrong ViPipe(%d)\n", ViPipe);
		return NULL;
	}

	struct isp_dci_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_DCI, (CVI_VOID *) &shared_buffer);

	return &shared_buffer->runtime;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_dci_ctrl_check_dci_attr_valid(const ISP_DCI_ATTR_S *pstDCIAttr)
{
	CVI_S32 ret = CVI_SUCCESS;

	CHECK_VALID_CONST(pstDCIAttr, Method, 0x0, 0x1);
	CHECK_VALID_CONST(pstDCIAttr, DciStrength, 0x0, 0x2000);
	CHECK_VALID_CONST(pstDCIAttr, DciGamma, 0x0, 0x1F);
	CHECK_VALID_CONST(pstDCIAttr, DciOffset, 0x1, 0xf);
	CHECK_VALID_CONST(pstDCIAttr, DciContrast, 0x0, 0x3);
	CHECK_VALID_ARRAY_1D(pstDCIAttr, DciGammaCurve, DCI_BINS_NUM, 0x0, 0x3FF);

	return ret;
}

//-----------------------------------------------------------------------------
//  public functions, set or get param
//-----------------------------------------------------------------------------
CVI_S32 isp_dci_ctrl_get_dci_attr(VI_PIPE ViPipe, const ISP_DCI_ATTR_S **pstDCIAttr)
{
	if (pstDCIAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_dci_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_DCI, (CVI_VOID *) &shared_buffer);

	*pstDCIAttr = &shared_buffer->stDciAttr;

	return ret;
}

CVI_S32 isp_dci_ctrl_set_dci_attr(VI_PIPE ViPipe, const ISP_DCI_ATTR_S *pstDCIAttr)
{
	if (pstDCIAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_dci_ctrl_runtime *runtime = _get_dci_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = isp_dci_ctrl_check_dci_attr_valid(pstDCIAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	const ISP_DCI_ATTR_S *p = CVI_NULL;

	isp_dci_ctrl_get_dci_attr(ViPipe, &p);

	memcpy((CVI_VOID *) p, pstDCIAttr, sizeof(*pstDCIAttr));

	runtime->preprocess_updated = CVI_TRUE;

	return ret;
}


