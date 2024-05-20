/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2023. All rights reserved.
 *
 * File Name: isp_rgbir_ctrl.c
 * Description:
 *
 */

#include "isp_main_local.h"
#include "isp_debug.h"
#include "isp_defines.h"
#include "cvi_comm_isp.h"
#include "isp_ioctl.h"

#include "isp_proc_local.h"
#include "isp_tun_buf_ctrl.h"
#include "isp_interpolate.h"
#include "cvi_isp.h"

#include "isp_rgbir_ctrl.h"
#include "isp_mgr_buf.h"

const struct isp_module_ctrl rgbir_mod = {
	.init = isp_rgbir_ctrl_init,
	.uninit = isp_rgbir_ctrl_uninit,
	.suspend = isp_rgbir_ctrl_suspend,
	.resume = isp_rgbir_ctrl_resume,
	.ctrl = isp_rgbir_ctrl_ctrl
};

static CVI_S32 isp_rgbir_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_rgbir_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
// static CVI_S32 isp_rgbir_ctrl_process(VI_PIPE ViPipe);
static CVI_S32 isp_rgbir_ctrl_postprocess(VI_PIPE ViPipe);
static CVI_S32 set_rgbir_proc_info(VI_PIPE ViPipe);
static CVI_S32 isp_rgbir_ctrl_check_rgbir_attr_valid(const ISP_RGBIR_ATTR_S *pstRgbirAttr);

static struct isp_rgbir_ctrl_runtime  *_get_rgbir_ctrl_runtime(VI_PIPE ViPipe);

CVI_S32 isp_rgbir_ctrl_init(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_rgbir_ctrl_runtime *runtime = _get_rgbir_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	runtime->preprocess_updated = CVI_TRUE;
	runtime->process_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_FALSE;
	runtime->is_module_bypass = CVI_FALSE;

	return ret;
}

CVI_S32 isp_rgbir_ctrl_uninit(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_rgbir_ctrl_suspend(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_rgbir_ctrl_resume(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_rgbir_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_rgbir_ctrl_runtime *runtime = _get_rgbir_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	switch (cmd) {
	case MOD_CMD_POST_EOF:
		isp_rgbir_ctrl_post_eof(ViPipe, (ISP_ALGO_RESULT_S *)input);
		break;
	case MOD_CMD_SET_MODCTRL:
		runtime->is_module_bypass = ((ISP_MODULE_CTRL_U *)input)->bitBypassRgbir;
		break;
	case MOD_CMD_GET_MODCTRL:
		((ISP_MODULE_CTRL_U *)input)->bitBypassRgbir = runtime->is_module_bypass;
		break;
	default:
		break;
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_rgbir_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_rgbir_ctrl_preprocess(ViPipe, algoResult);
	if (ret != CVI_SUCCESS)
		return ret;

	//ret = isp_rgbir_ctrl_process(ViPipe);
	//if (ret != CVI_SUCCESS)
	//	return ret;

	ret = isp_rgbir_ctrl_postprocess(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	set_rgbir_proc_info(ViPipe);

	return ret;
}

static CVI_S32 isp_rgbir_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_rgbir_ctrl_runtime *runtime = _get_rgbir_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	const ISP_RGBIR_ATTR_S *rgbir_attr = NULL;

	isp_rgbir_ctrl_get_rgbir_attr(ViPipe, &rgbir_attr);

	CVI_BOOL is_preprocess_update = CVI_FALSE;
	CVI_U8 intvl = MAX(rgbir_attr->UpdateInterval, 1);

	is_preprocess_update = ((runtime->preprocess_updated) || ((algoResult->u32FrameIdx % intvl) == 0));

	// No need to update status
	if (is_preprocess_update == CVI_FALSE)
		return ret;

	runtime->preprocess_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_TRUE;

	// No need to update parameters if disable. Because its meaningless
	if (!rgbir_attr->Enable || runtime->is_module_bypass)
		return ret;

	if (rgbir_attr->enOpType == OP_TYPE_MANUAL) {
		#define MANUAL(_attr, _param) \
		runtime->_attr._param = _attr->stManual._param

		MANUAL(rgbir_attr, u16RecGbrGain);
		MANUAL(rgbir_attr, u16RecGbrOffset);
		MANUAL(rgbir_attr, u16RecGirGain);
		MANUAL(rgbir_attr, u16RecGirOffset);
		MANUAL(rgbir_attr, u16RecRgGain);
		MANUAL(rgbir_attr, u16RecRgOffset);
		MANUAL(rgbir_attr, u16RecBgGain);
		MANUAL(rgbir_attr, u16RecBgOffset);

		#undef MANUAL
	} else {
		#define AUTO(_attr, _param, type) \
		runtime->_attr._param = INTERPOLATE_LINEAR(ViPipe, type, _attr->stAuto._param)

		AUTO(rgbir_attr, u16RecGbrGain, INTPLT_POST_ISO);
		AUTO(rgbir_attr, u16RecGbrOffset, INTPLT_POST_ISO);
		AUTO(rgbir_attr, u16RecGirGain, INTPLT_POST_ISO);
		AUTO(rgbir_attr, u16RecGirOffset, INTPLT_POST_ISO);
		AUTO(rgbir_attr, u16RecRgGain, INTPLT_POST_ISO);
		AUTO(rgbir_attr, u16RecRgOffset, INTPLT_POST_ISO);
		AUTO(rgbir_attr, u16RecBgGain, INTPLT_POST_ISO);
		AUTO(rgbir_attr, u16RecBgOffset, INTPLT_POST_ISO);

		#undef AUTO
	}

	// ParamIn

	// ParamOut

	runtime->process_updated = CVI_TRUE;

	UNUSED(algoResult);

	return ret;
}

//static CVI_S32 isp_rgbir_ctrl_process(VI_PIPE ViPipe)
//{
//	CVI_S32 ret = CVI_SUCCESS;
//	struct isp_rgbir_ctrl_runtime *runtime = _get_rgbir_ctrl_runtime(ViPipe);
//
//	if (runtime == CVI_NULL) {
//		return CVI_FAILURE;
//	}
//
//	if (runtime->process_updated == CVI_FALSE)
//		return ret;
//
//	ret = isp_algo_rgbir_main(
//		(struct rgbir_param_in *)&runtime->rgbir_param_in,
//		(struct rgbir_param_out *)&runtime->rgbir_param_out
//	);
//
//	runtime->process_updated = CVI_FALSE;
//
//	return ret;
//}

static CVI_S32 isp_rgbir_ctrl_postprocess(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_rgbir_ctrl_runtime *runtime = _get_rgbir_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	struct cvi_vip_isp_be_cfg *pre_be_addr = get_pre_be_tuning_buf_addr(ViPipe);
	CVI_U8 tun_idx = get_tuning_buf_idx(ViPipe);

	struct cvi_vip_isp_rgbir_config *rgbir_cfg_0 = {
		(struct cvi_vip_isp_rgbir_config *)&(pre_be_addr->tun_cfg[tun_idx].rgbir_cfg[0]),
	};

	const ISP_RGBIR_ATTR_S *rgbir_attr = NULL;
	ISP_CTX_S *pstIspCtx = NULL;

	ISP_GET_CTX(ViPipe, pstIspCtx);
	isp_rgbir_ctrl_get_rgbir_attr(ViPipe, &rgbir_attr);

	CVI_BOOL is_postprocess_update = ((runtime->postprocess_updated == CVI_TRUE) || (IS_MULTI_CAM()));
	CVI_BOOL isRgbIrSns = (pstIspCtx->enBayer >= BAYER_GRGBI && pstIspCtx->enBayer <= BAYER_IGBGR) ? CVI_TRUE : CVI_FALSE;

	rgbir_cfg_0->inst = 0;

	if (is_postprocess_update == CVI_FALSE) {
		rgbir_cfg_0->update = 0;
	} else {
		CVI_BOOL enable = rgbir_attr->Enable && !runtime->is_module_bypass;

		rgbir_cfg_0->update = 1;
		if (isRgbIrSns) {
			rgbir_cfg_0->enable = 1;
			if (enable) {
				rgbir_cfg_0->comp_enable = rgbir_attr->bLumaCompEnable;
			} else {
				rgbir_cfg_0->comp_enable = 0;
			}
		} else {
			rgbir_cfg_0->enable = 0;
			rgbir_cfg_0->comp_enable = 0;
		}

		struct cvi_isp_rgbir_tun_cfg *rgbir_tun_cfg = &rgbir_cfg_0->rgbir_cfg;

		if (isRgbIrSns && !enable) {
			rgbir_tun_cfg->GAIN_OFFSET_1.bits.RGBIR2RGGB_REC_GBR_GAIN = 0;
			rgbir_tun_cfg->GAIN_OFFSET_1.bits.RGBIR2RGGB_REC_GBR_OFFSET = 0;
			rgbir_tun_cfg->GAIN_OFFSET_2.bits.RGBIR2RGGB_REC_GIR_GAIN = 0;
			rgbir_tun_cfg->GAIN_OFFSET_2.bits.RGBIR2RGGB_REC_GIR_OFFSET = 0;
			rgbir_tun_cfg->GAIN_OFFSET_3.bits.RGBIR2RGGB_REC_RG_GAIN = 0;
			rgbir_tun_cfg->GAIN_OFFSET_3.bits.RGBIR2RGGB_REC_RG_OFFSET = 0;
			rgbir_tun_cfg->GAIN_OFFSET_4.bits.RGBIR2RGGB_REC_BG_GAIN = 0;
			rgbir_tun_cfg->GAIN_OFFSET_4.bits.RGBIR2RGGB_REC_BG_OFFSET = 0;
		} else {
			rgbir_tun_cfg->GAIN_OFFSET_1.bits.RGBIR2RGGB_REC_GBR_GAIN = runtime->rgbir_attr.u16RecGbrGain;
			rgbir_tun_cfg->GAIN_OFFSET_1.bits.RGBIR2RGGB_REC_GBR_OFFSET = runtime->rgbir_attr.u16RecGbrOffset;
			rgbir_tun_cfg->GAIN_OFFSET_2.bits.RGBIR2RGGB_REC_GIR_GAIN = runtime->rgbir_attr.u16RecGirGain;
			rgbir_tun_cfg->GAIN_OFFSET_2.bits.RGBIR2RGGB_REC_GIR_OFFSET = runtime->rgbir_attr.u16RecGirOffset;
			rgbir_tun_cfg->GAIN_OFFSET_3.bits.RGBIR2RGGB_REC_RG_GAIN = runtime->rgbir_attr.u16RecRgGain;
			rgbir_tun_cfg->GAIN_OFFSET_3.bits.RGBIR2RGGB_REC_RG_OFFSET = runtime->rgbir_attr.u16RecRgOffset;
			rgbir_tun_cfg->GAIN_OFFSET_4.bits.RGBIR2RGGB_REC_BG_GAIN = runtime->rgbir_attr.u16RecBgGain;
			rgbir_tun_cfg->GAIN_OFFSET_4.bits.RGBIR2RGGB_REC_BG_OFFSET = runtime->rgbir_attr.u16RecBgOffset;
		}
		rgbir_tun_cfg->COMP_GAIN.bits.RGBIR2RGGB_G_COMP_GAIN = rgbir_attr->u8GCompGain;
		rgbir_tun_cfg->COMP_GAIN.bits.RGBIR2RGGB_R_COMP_GAIN = rgbir_attr->u8RCompGain;
		rgbir_tun_cfg->COMP_GAIN.bits.RGBIR2RGGB_B_COMP_GAIN = rgbir_attr->u8BCompGain;
	}

	struct cvi_vip_isp_rgbir_config *rgbir_cfg_1 = {
		&(pre_be_addr->tun_cfg[tun_idx].rgbir_cfg[1]),
	};

	if (IS_2to1_WDR_MODE(pstIspCtx->u8SnsWDRMode)) {
		if (is_postprocess_update == CVI_FALSE) {
			rgbir_cfg_1->update = 0;
		} else {
			memcpy(rgbir_cfg_1, rgbir_cfg_0, sizeof(struct cvi_vip_isp_rgbir_config));
		}
	} else {
		rgbir_cfg_1->enable = 0;
		rgbir_cfg_1->update = 0;
	}

	rgbir_cfg_1->inst = 1;

	runtime->postprocess_updated = CVI_FALSE;

	return ret;
}

static CVI_S32 set_rgbir_proc_info(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

static struct isp_rgbir_ctrl_runtime  *_get_rgbir_ctrl_runtime(VI_PIPE ViPipe)
{
	CVI_BOOL isVipipeValid = ((ViPipe >= 0) && (ViPipe < VI_MAX_PIPE_NUM));

	if (!isVipipeValid) {
		ISP_LOG_WARNING("Wrong ViPipe(%d)\n", ViPipe);
		return NULL;
	}

	struct isp_rgbir_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_RGBIR, (CVI_VOID *) &shared_buffer);

	return &shared_buffer->runtime;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_rgbir_ctrl_check_rgbir_attr_valid(const ISP_RGBIR_ATTR_S *pstRgbirAttr)
{
	CVI_S32 ret = CVI_SUCCESS;

	// CHECK_VALID_CONST(pstRgbirAttr, Enable, CVI_FALSE, CVI_TRUE);
	CHECK_VALID_CONST(pstRgbirAttr, enOpType, OP_TYPE_AUTO, OP_TYPE_MANUAL);
	// CHECK_VALID_CONST(pstRgbirAttr, UpdateInterval, 0, 0xff);
	CHECK_VALID_CONST(pstRgbirAttr, enIrBayerFormat, BAYER_GRGBI, BAYER_IGBGR);
	CHECK_VALID_CONST(pstRgbirAttr, u8RCompGain, 0x0, 0xF);
	CHECK_VALID_CONST(pstRgbirAttr, u8GCompGain, 0x0, 0xF);
	CHECK_VALID_CONST(pstRgbirAttr, u8BCompGain, 0x0, 0xF);
	CHECK_VALID_CONST(pstRgbirAttr, u8DebugMode, 0x0, 0xF);
	CHECK_VALID_AUTO_ISO_1D(pstRgbirAttr, u16RecGbrGain, 0x0, 0xFFF);
	CHECK_VALID_AUTO_ISO_1D(pstRgbirAttr, u16RecGbrOffset, 0x0, 0xFFF);
	CHECK_VALID_AUTO_ISO_1D(pstRgbirAttr, u16RecGirGain, 0x0, 0xFFF);
	CHECK_VALID_AUTO_ISO_1D(pstRgbirAttr, u16RecGirOffset, 0x0, 0xFFF);
	CHECK_VALID_AUTO_ISO_1D(pstRgbirAttr, u16RecRgGain, 0x0, 0xFFF);
	CHECK_VALID_AUTO_ISO_1D(pstRgbirAttr, u16RecRgOffset, 0x0, 0xFFF);
	CHECK_VALID_AUTO_ISO_1D(pstRgbirAttr, u16RecBgGain, 0x0, 0xFFF);
	CHECK_VALID_AUTO_ISO_1D(pstRgbirAttr, u16RecBgOffset, 0x0, 0xFFF);

	return ret;
}
//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//  public functions, set or get param
//-----------------------------------------------------------------------------
CVI_S32 isp_rgbir_ctrl_get_rgbir_attr(VI_PIPE ViPipe, const ISP_RGBIR_ATTR_S **pstRgbirAttr)
{
	if (pstRgbirAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	struct isp_rgbir_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_RGBIR, (CVI_VOID *) &shared_buffer);
	*pstRgbirAttr = &shared_buffer->stRgbirAttr;

	return ret;
}

CVI_S32 isp_rgbir_ctrl_set_rgbir_attr(VI_PIPE ViPipe, const ISP_RGBIR_ATTR_S *pstRgbirAttr)
{
	if (pstRgbirAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_rgbir_ctrl_runtime *runtime = _get_rgbir_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = isp_rgbir_ctrl_check_rgbir_attr_valid(pstRgbirAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	ISP_CTX_S *pstIspCtx = NULL;

	ISP_GET_CTX(ViPipe, pstIspCtx);

	CVI_BOOL isRgbIrSns = (pstIspCtx->enBayer >= BAYER_GRGBI && pstIspCtx->enBayer <= BAYER_IGBGR) ? CVI_TRUE : CVI_FALSE;

	if (!isRgbIrSns && pstRgbirAttr->Enable) {
		ISP_LOG_ERR("Sensor(%d) isn't rgbir sensor, cann't enable rgbir module!\n", ViPipe);
		return CVI_FAILURE;
	}

	const ISP_RGBIR_ATTR_S *p = CVI_NULL;

	isp_rgbir_ctrl_get_rgbir_attr(ViPipe, &p);
	memcpy((void *)p, pstRgbirAttr, sizeof(*pstRgbirAttr));

	runtime->preprocess_updated = CVI_TRUE;

	return ret;
}

