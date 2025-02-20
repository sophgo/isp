/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_dpc_ctrl.c
 * Description:
 *
 */

#include "cvi_sys.h"
#include "isp_main_local.h"
#include "isp_debug.h"
#include "isp_defines.h"
#include "cvi_comm_isp.h"
#include "isp_ioctl.h"
#include "cvi_vi.h"

#include "isp_proc_local.h"
#include "isp_tun_buf_ctrl.h"
#include "isp_interpolate.h"
#include "cvi_isp.h"
#include "dpcm_api.h"
#include <sys/time.h>

#include "isp_dpc_ctrl.h"
#include "isp_algo_dpc.h"
#include "isp_mgr_buf.h"

const struct isp_module_ctrl dpc_mod = {
	.init = isp_dpc_ctrl_init,
	.uninit = isp_dpc_ctrl_uninit,
	.suspend = isp_dpc_ctrl_suspend,
	.resume = isp_dpc_ctrl_resume,
	.ctrl = isp_dpc_ctrl_ctrl
};

static CVI_S32 isp_dpc_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_dpc_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
// static CVI_S32 isp_dpc_ctrl_process(VI_PIPE ViPipe);
static CVI_S32 isp_dpc_ctrl_postprocess(VI_PIPE ViPipe);

static CVI_S32 set_dpc_proc_info(VI_PIPE ViPipe);
static CVI_S32 isp_dpc_ctrl_check_dpc_dynamic_attr_valid(const ISP_DP_DYNAMIC_ATTR_S *pstDPCDynamicAttr);
static CVI_S32 isp_dpc_ctrl_check_dpc_static_attr_valid(const ISP_DP_STATIC_ATTR_S *pstDPStaticAttr);

static struct isp_dpc_ctrl_runtime  *_get_dpc_ctrl_runtime(VI_PIPE ViPipe);

CVI_S32 isp_dpc_ctrl_init(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	struct isp_dpc_ctrl_runtime *runtime = _get_dpc_ctrl_runtime(ViPipe);

	runtime->preprocess_updated = CVI_TRUE;
	runtime->process_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_FALSE;
	runtime->is_module_bypass = CVI_FALSE;

	return ret;
}

CVI_S32 isp_dpc_ctrl_uninit(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_dpc_ctrl_suspend(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_dpc_ctrl_resume(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_dpc_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_dpc_ctrl_runtime *runtime = _get_dpc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	switch (cmd) {
	case MOD_CMD_POST_EOF:
		isp_dpc_ctrl_post_eof(ViPipe, (ISP_ALGO_RESULT_S *)input);
		break;
	case MOD_CMD_SET_MODCTRL:
		runtime->is_module_bypass = ((ISP_MODULE_CTRL_U *)input)->bitBypassDpc;
		break;
	case MOD_CMD_GET_MODCTRL:
		((ISP_MODULE_CTRL_U *)input)->bitBypassDpc = runtime->is_module_bypass;
		break;
	default:
		break;
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_dpc_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_dpc_ctrl_preprocess(ViPipe, algoResult);
	if (ret != CVI_SUCCESS)
		return ret;

	#if 0
	ret = isp_dpc_ctrl_process(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;
	#endif

	ret = isp_dpc_ctrl_postprocess(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	set_dpc_proc_info(ViPipe);

	return ret;
}

static CVI_S32 isp_dpc_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_dpc_ctrl_runtime *runtime = _get_dpc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	const ISP_DP_DYNAMIC_ATTR_S *dpc_dynamic_attr = NULL;

	isp_dpc_ctrl_get_dpc_dynamic_attr(ViPipe, &dpc_dynamic_attr);

	CVI_BOOL is_preprocess_update = CVI_FALSE;
	CVI_U8 intvl = MAX(dpc_dynamic_attr->UpdateInterval, 1);

	is_preprocess_update = ((runtime->preprocess_updated) || ((algoResult->u32FrameIdx % intvl) == 0));

	// No need to update status
	if (is_preprocess_update == CVI_FALSE)
		return ret;

	runtime->preprocess_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_TRUE;

	// No need to update parameters if disable. Because its meaningless
	if (!dpc_dynamic_attr->Enable || runtime->is_module_bypass)
		return ret;

	if (dpc_dynamic_attr->enOpType == OP_TYPE_MANUAL) {
		#define MANUAL(_attr, _param) \
		runtime->_attr._param = _attr->stManual._param

		MANUAL(dpc_dynamic_attr, ClusterSize);
		MANUAL(dpc_dynamic_attr, BrightDefectToNormalPixRatio);
		MANUAL(dpc_dynamic_attr, DarkDefectToNormalPixRatio);
		MANUAL(dpc_dynamic_attr, FlatThreR);
		MANUAL(dpc_dynamic_attr, FlatThreG);
		MANUAL(dpc_dynamic_attr, FlatThreB);
		MANUAL(dpc_dynamic_attr, FlatThreMinG);
		MANUAL(dpc_dynamic_attr, FlatThreMinRB);

		#undef MANUAL
	} else {
		#define AUTO(_attr, _param, type) \
		runtime->_attr._param = INTERPOLATE_LINEAR(ViPipe, type, _attr->stAuto._param)

		AUTO(dpc_dynamic_attr, ClusterSize, INTPLT_POST_ISO);
		AUTO(dpc_dynamic_attr, BrightDefectToNormalPixRatio, INTPLT_POST_ISO);
		AUTO(dpc_dynamic_attr, DarkDefectToNormalPixRatio, INTPLT_POST_ISO);
		AUTO(dpc_dynamic_attr, FlatThreR, INTPLT_POST_ISO);
		AUTO(dpc_dynamic_attr, FlatThreG, INTPLT_POST_ISO);
		AUTO(dpc_dynamic_attr, FlatThreB, INTPLT_POST_ISO);
		AUTO(dpc_dynamic_attr, FlatThreMinG, INTPLT_POST_ISO);
		AUTO(dpc_dynamic_attr, FlatThreMinRB, INTPLT_POST_ISO);

		#undef AUTO
	}

	// ParamIn
	// runtime->dpc_param_in.BrightDefectToNormalPixRatio = runtime->dpc_dynamic_attr.BrightDefectToNormalPixRatio;
	// runtime->dpc_param_in.DarkDefectToNormalPixRatio = runtime->dpc_dynamic_attr.DarkDefectToNormalPixRatio;

	// ParamOut

	runtime->process_updated = CVI_TRUE;

	UNUSED(algoResult);

	return ret;
}

#if 0
static CVI_S32 isp_dpc_ctrl_process(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_dpc_ctrl_runtime *runtime = _get_dpc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (runtime->process_updated == CVI_FALSE)
		return ret;

	ret = isp_algo_dpc_main(
		(struct dpc_param_in *)&runtime->dpc_param_in,
		(struct dpc_param_out *)&runtime->dpc_param_out
	);

	runtime->process_updated = CVI_FALSE;

	return ret;
}
#endif

static CVI_S32 isp_dpc_ctrl_postprocess(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_dpc_ctrl_runtime *runtime = _get_dpc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	struct cvi_vip_isp_be_cfg *pre_be_addr = get_pre_be_tuning_buf_addr(ViPipe);
	CVI_U8 tun_idx = get_tuning_buf_idx(ViPipe);

	struct cvi_vip_isp_dpc_config *dpc_cfg[2] = {
		(struct cvi_vip_isp_dpc_config *)&(pre_be_addr->tun_cfg[tun_idx].dpc_cfg[0]),
		(struct cvi_vip_isp_dpc_config *)&(pre_be_addr->tun_cfg[tun_idx].dpc_cfg[1]),
	};

	const ISP_DP_DYNAMIC_ATTR_S *dpc_dynamic_attr = NULL;
	const ISP_DP_STATIC_ATTR_S *dpc_static_attr = NULL;

	isp_dpc_ctrl_get_dpc_dynamic_attr(ViPipe, &dpc_dynamic_attr);
	isp_dpc_ctrl_get_dpc_static_attr(ViPipe, &dpc_static_attr);

	CVI_BOOL is_postprocess_update = ((runtime->postprocess_updated == CVI_TRUE) || (IS_MULTI_CAM()));

	if (is_postprocess_update == CVI_FALSE) {
		for (CVI_U32 idx = 0 ; idx < 2 ; idx++) {
			dpc_cfg[idx]->inst = idx;
			dpc_cfg[idx]->update = 0;
		}
	} else {
		runtime->dpc_cfg.update = 1;
		// runtime->dpc_cfg.inst;
		runtime->dpc_cfg.enable = (dpc_dynamic_attr->Enable || dpc_static_attr->Enable)
									&& !runtime->is_module_bypass;
		runtime->dpc_cfg.staticbpc_enable = dpc_static_attr->Enable;
		runtime->dpc_cfg.dynamicbpc_enable = dpc_dynamic_attr->DynamicDPCEnable;
		runtime->dpc_cfg.cluster_size = runtime->dpc_dynamic_attr.ClusterSize;

		if (dpc_static_attr->Enable) {
			#define DPC_MAX_SIZE 4096
			CVI_U32 bp_cnt = 0;
			CVI_U32 brightMax = dpc_static_attr->BrightCount;
			CVI_U32 darkMax = dpc_static_attr->DarkCount;

			if (brightMax + darkMax > DPC_MAX_SIZE) {
				CVI_U16 total = brightMax + darkMax;

				brightMax =  brightMax * DPC_MAX_SIZE / total;
				darkMax =  darkMax * DPC_MAX_SIZE / total;
			}
			// bp_tbl = { (offsetx) | (offset_y << 13), ....}
			#define CONVERT_BP_FORMAT(bp) ((((bp) & 0x1FFF0000) >> (16-13)) + ((bp) & 0x00001FFF))

			for (CVI_U32 c = 0; c < brightMax; c++, bp_cnt++)
				runtime->dpc_cfg.bp_tbl[bp_cnt] = CONVERT_BP_FORMAT(dpc_static_attr->BrightTable[c]);
			for (CVI_U32 c = 0; c < darkMax; c++, bp_cnt++)
				runtime->dpc_cfg.bp_tbl[bp_cnt] = CONVERT_BP_FORMAT(dpc_static_attr->DarkTable[c]);
			runtime->dpc_cfg.bp_cnt = bp_cnt;

			#undef CONVERT_BP_FORMAT
		}

		struct cvi_isp_dpc_tun_cfg *cfg_dpc  = (struct cvi_isp_dpc_tun_cfg *)&runtime->dpc_cfg.dpc_cfg;

		//TODO@ST Check formula
		cfg_dpc->DPC_3.bits.DPC_R_BRIGHT_PIXEL_RATIO = runtime->dpc_dynamic_attr.BrightDefectToNormalPixRatio;
		cfg_dpc->DPC_3.bits.DPC_G_BRIGHT_PIXEL_RATIO = runtime->dpc_dynamic_attr.BrightDefectToNormalPixRatio;
		cfg_dpc->DPC_4.bits.DPC_B_BRIGHT_PIXEL_RATIO = runtime->dpc_dynamic_attr.BrightDefectToNormalPixRatio;
		cfg_dpc->DPC_4.bits.DPC_R_DARK_PIXEL_RATIO = runtime->dpc_dynamic_attr.DarkDefectToNormalPixRatio;
		cfg_dpc->DPC_5.bits.DPC_G_DARK_PIXEL_RATIO = runtime->dpc_dynamic_attr.DarkDefectToNormalPixRatio;
		cfg_dpc->DPC_5.bits.DPC_B_DARK_PIXEL_RATIO = runtime->dpc_dynamic_attr.DarkDefectToNormalPixRatio;
		cfg_dpc->DPC_6.bits.DPC_R_DARK_PIXEL_MINDIFF = 11;
		cfg_dpc->DPC_6.bits.DPC_G_DARK_PIXEL_MINDIFF = 11;
		cfg_dpc->DPC_6.bits.DPC_B_DARK_PIXEL_MINDIFF = 11;
		cfg_dpc->DPC_7.bits.DPC_R_BRIGHT_PIXEL_UPBOUD_RATIO = 236;
		cfg_dpc->DPC_7.bits.DPC_G_BRIGHT_PIXEL_UPBOUD_RATIO = 236;
		cfg_dpc->DPC_7.bits.DPC_B_BRIGHT_PIXEL_UPBOUD_RATIO = 236;
		cfg_dpc->DPC_8.bits.DPC_FLAT_THRE_MIN_RB = runtime->dpc_dynamic_attr.FlatThreMinRB;
		cfg_dpc->DPC_8.bits.DPC_FLAT_THRE_MIN_G = runtime->dpc_dynamic_attr.FlatThreMinG;
		cfg_dpc->DPC_9.bits.DPC_FLAT_THRE_R = runtime->dpc_dynamic_attr.FlatThreR;
		cfg_dpc->DPC_9.bits.DPC_FLAT_THRE_G = runtime->dpc_dynamic_attr.FlatThreG;
		cfg_dpc->DPC_9.bits.DPC_FLAT_THRE_B = runtime->dpc_dynamic_attr.FlatThreB;

		ISP_CTX_S *pstIspCtx = NULL;
		CVI_BOOL *map_pipe_to_enable;
		CVI_BOOL map_pipe_to_enable_sdr[2] = {1, 0};
		CVI_BOOL map_pipe_to_enable_wdr[2] = {1, 1};

		ISP_GET_CTX(ViPipe, pstIspCtx);
		if (IS_2to1_WDR_MODE(pstIspCtx->u8SnsWDRMode))
			map_pipe_to_enable = map_pipe_to_enable_wdr;
		else
			map_pipe_to_enable = map_pipe_to_enable_sdr;
		for (CVI_U32 idx = 0 ; idx < 2 ; idx++) {
			runtime->dpc_cfg.inst = idx;
			runtime->dpc_cfg.enable = runtime->dpc_cfg.enable && map_pipe_to_enable[idx];
			memcpy(dpc_cfg[idx], &(runtime->dpc_cfg), sizeof(struct cvi_vip_isp_dpc_config));
		}
	}

	runtime->postprocess_updated = CVI_FALSE;

	return ret;
}

static CVI_S32 set_dpc_proc_info(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;

	if (ISP_GET_PROC_ACTION(ViPipe, PROC_LEVEL_1)) {
		struct isp_dpc_ctrl_runtime *runtime = _get_dpc_ctrl_runtime(ViPipe);

		const ISP_DP_DYNAMIC_ATTR_S *dpc_dynamic_attr = NULL;
		ISP_DEBUGINFO_PROC_S *pProcST = NULL;

		if (runtime == CVI_NULL) {
			return CVI_FAILURE;
		}

		isp_dpc_ctrl_get_dpc_dynamic_attr(ViPipe, &dpc_dynamic_attr);
		ISP_GET_PROC_INFO(ViPipe, pProcST);

		//common
		pProcST->DPCEnable = dpc_dynamic_attr->Enable;
		pProcST->DPCisManualMode = dpc_dynamic_attr->enOpType;
		//manual or auto
		pProcST->DPCClusterSize = runtime->dpc_dynamic_attr.ClusterSize;
	}

	return ret;
}

static struct isp_dpc_ctrl_runtime  *_get_dpc_ctrl_runtime(VI_PIPE ViPipe)
{
	CVI_BOOL isVipipeValid = ((ViPipe >= 0) && (ViPipe < VI_MAX_PIPE_NUM));

	if (!isVipipeValid) {
		ISP_LOG_WARNING("Wrong ViPipe(%d)\n", ViPipe);
		return NULL;
	}

	struct isp_dpc_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_DPC, (CVI_VOID *) &shared_buffer);

	return &shared_buffer->runtime;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_dpc_ctrl_check_dpc_dynamic_attr_valid(const ISP_DP_DYNAMIC_ATTR_S *pstDPCDynamicAttr)
{
	CVI_S32 ret = CVI_SUCCESS;

	// CHECK_VALID_CONST(pstDPCDynamicAttr, Enable, CVI_FALSE, CVI_TRUE);
	CHECK_VALID_CONST(pstDPCDynamicAttr, enOpType, OP_TYPE_AUTO, OP_TYPE_MANUAL);
	// CHECK_VALID_CONST(pstDPCDynamicAttr, UpdateInterval, 0, 0xff);
	CHECK_VALID_AUTO_ISO_1D(pstDPCDynamicAttr, ClusterSize, 0x0, 0x3);

	return ret;
}

static CVI_S32 isp_dpc_ctrl_check_dpc_static_attr_valid(const ISP_DP_STATIC_ATTR_S *pstDPStaticAttr)
{
	CVI_S32 ret = CVI_SUCCESS;

	// CHECK_VALID_CONST(pstDPStaticAttr, Enable, CVI_FALSE, CVI_TRUE);
	CHECK_VALID_CONST(pstDPStaticAttr, BrightCount, 0x0, 0xFFF);
	CHECK_VALID_CONST(pstDPStaticAttr, DarkCount, 0x0, 0xFFF);
	CHECK_VALID_ARRAY_1D(pstDPStaticAttr, BrightTable, STATIC_DP_COUNT_MAX, 0x0, 0x1fff1fff);
	CHECK_VALID_ARRAY_1D(pstDPStaticAttr, DarkTable, STATIC_DP_COUNT_MAX, 0x0, 0x1fff1fff);

	return ret;
}

//-----------------------------------------------------------------------------
//  public functions, set or get param
//-----------------------------------------------------------------------------
CVI_S32 isp_dpc_ctrl_get_dpc_dynamic_attr(VI_PIPE ViPipe, const ISP_DP_DYNAMIC_ATTR_S **pstDPCDynamicAttr)
{
	if (pstDPCDynamicAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_dpc_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_DPC, (CVI_VOID *) &shared_buffer);

	*pstDPCDynamicAttr = &shared_buffer->stDPCDynamicAttr;

	return ret;
}

CVI_S32 isp_dpc_ctrl_set_dpc_dynamic_attr(VI_PIPE ViPipe, const ISP_DP_DYNAMIC_ATTR_S *pstDPCDynamicAttr)
{
	if (pstDPCDynamicAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_dpc_ctrl_runtime *runtime = _get_dpc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = isp_dpc_ctrl_check_dpc_dynamic_attr_valid(pstDPCDynamicAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	const ISP_DP_DYNAMIC_ATTR_S *p = CVI_NULL;

	isp_dpc_ctrl_get_dpc_dynamic_attr(ViPipe, &p);

	memcpy((CVI_VOID *) p, pstDPCDynamicAttr, sizeof(*pstDPCDynamicAttr));

	runtime->preprocess_updated = CVI_TRUE;

	return ret;
}

CVI_S32 isp_dpc_ctrl_get_dpc_static_attr(VI_PIPE ViPipe, const ISP_DP_STATIC_ATTR_S **pstDPStaticAttr)
{
	if (pstDPStaticAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_dpc_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_DPC, (CVI_VOID *) &shared_buffer);

	*pstDPStaticAttr = &shared_buffer->stDPStaticAttr;

	return ret;
}

CVI_S32 isp_dpc_ctrl_set_dpc_static_attr(VI_PIPE ViPipe, const ISP_DP_STATIC_ATTR_S *pstDPStaticAttr)
{
	if (pstDPStaticAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_dpc_ctrl_runtime *runtime = _get_dpc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = isp_dpc_ctrl_check_dpc_static_attr_valid(pstDPStaticAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	const ISP_DP_STATIC_ATTR_S *p = CVI_NULL;

	isp_dpc_ctrl_get_dpc_static_attr(ViPipe, &p);

	memcpy((CVI_VOID *) p, pstDPStaticAttr, sizeof(*pstDPStaticAttr));

	runtime->preprocess_updated = CVI_TRUE;

	return CVI_SUCCESS;
}
