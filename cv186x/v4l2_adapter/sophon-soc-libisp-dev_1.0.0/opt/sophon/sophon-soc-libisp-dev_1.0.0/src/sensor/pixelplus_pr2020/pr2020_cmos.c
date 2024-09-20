#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <syslog.h>
#include <errno.h>
#ifdef ARCH_CV182X
#include "cvi_type.h"
#include "cvi_comm_video.h"
#include <linux/cvi_vip_snsr.h>
#else
#include <cvi_type.h>
#include <cvi_comm_video.h>

#endif
#include "cvi_debug.h"
#include "cvi_comm_sns.h"
#include "cvi_sns_ctrl.h"
#include "cvi_isp.h"

#include "pr2020_cmos_ex.h"
#include "pr2020_cmos_param.h"

/* I2C slave address of Pr2020, SA0=0:0x5C*/
#define Pr2020_I2C_ADDR_1 0x5C
#define Pr2020_I2C_ADDR_2 0x5C
#define Pr2020_I2C_ADDR_IS_VALID(addr)      ((addr) == Pr2020_I2C_ADDR_1 || (addr) == Pr2020_I2C_ADDR_2)

/****************************************************************************
 * global variables                                                         *
 ****************************************************************************/
ISP_SNS_COMMBUS_U g_aunPr2020_BusInfo[VI_MAX_PIPE_NUM] = {
	[0] = { .s8I2cDev = 0},
	[1 ... VI_MAX_PIPE_NUM - 1] = { .s8I2cDev = -1}
};

ISP_SNS_COMMADDR_U g_aunPr2020_AddrInfo[VI_MAX_PIPE_NUM] = {
	[0] = { .s8I2cAddr = 0},
	[1 ... VI_MAX_PIPE_NUM - 1] = { .s8I2cAddr = -1}
};

ISP_SNS_STATE_S *g_pastPr2020[VI_MAX_PIPE_NUM] = {CVI_NULL};
SNS_COMBO_DEV_ATTR_S *g_pastPr2020ComboDevArray[VI_MAX_PIPE_NUM] = {CVI_NULL};

#define PR2020_SENSOR_GET_CTX(dev, pstCtx)   (pstCtx = g_pastPr2020[dev])
#define PR2020_SENSOR_SET_CTX(dev, pstCtx)   (g_pastPr2020[dev] = pstCtx)
#define PR2020_SENSOR_RESET_CTX(dev)         (g_pastPr2020[dev] = CVI_NULL)
#define Pr2020_SENSOR_SET_COMBO(dev, pstCtx)   (g_pastPr2020ComboDevArray[dev] = pstCtx)
#define Pr2020_SENSOR_GET_COMBO(dev, pstCtx)   (pstCtx = g_pastPr2020ComboDevArray[dev])
#define PR2020_RES_IS_720P(w, h)      ((w) <= 1280 && (h) <= 720)
#define PR2020_RES_IS_1080P(w, h)     ((w) <= 1920 && (h) <= 1080)
#define PR2020_ID 2020

/****************************************************************************
 * local variables and functions                                            *
 ****************************************************************************/
static CVI_S32 cmos_get_wdr_size(VI_PIPE ViPipe, ISP_SNS_ISP_INFO_S *pstIspCfg)
{
	const PR2020_MODE_S *pstMode = CVI_NULL;
	ISP_SNS_STATE_S *pstSnsState = CVI_NULL;

	PR2020_SENSOR_GET_CTX(ViPipe, pstSnsState);
	CMOS_CHECK_POINTER(pstSnsState);

	pstMode = &g_astPr2020_mode[pstSnsState->u8ImgMode];
	pstIspCfg->frm_num = 1;
	memcpy(&pstIspCfg->img_size[0], &pstMode->astImg[0], sizeof(ISP_WDR_SIZE_S));

	return CVI_SUCCESS;
}

static CVI_S32 cmos_get_sns_regs_info(VI_PIPE ViPipe, ISP_SNS_SYNC_INFO_S *pstSnsSyncInfo)
{
	ISP_SNS_STATE_S *pstSnsState = CVI_NULL;
	ISP_SNS_SYNC_INFO_S *pstCfg0 = CVI_NULL;

	CMOS_CHECK_POINTER(pstSnsSyncInfo);
	PR2020_SENSOR_GET_CTX(ViPipe, pstSnsState);
	CMOS_CHECK_POINTER(pstSnsState);

	pstCfg0 = &pstSnsState->astSyncInfo[0];
	cmos_get_wdr_size(ViPipe, &pstCfg0->ispCfg);
	memcpy(pstSnsSyncInfo, &pstSnsState->astSyncInfo[0], sizeof(ISP_SNS_SYNC_INFO_S));

	return CVI_SUCCESS;
}

static CVI_S32 cmos_set_image_mode(VI_PIPE ViPipe, ISP_CMOS_SENSOR_IMAGE_MODE_S *pstSensorImageMode)
{
	CVI_U8 u8SensorImageMode = 0;
	ISP_SNS_STATE_S *pstSnsState = CVI_NULL;

	CMOS_CHECK_POINTER(pstSensorImageMode);
	PR2020_SENSOR_GET_CTX(ViPipe, pstSnsState);
	CMOS_CHECK_POINTER(pstSnsState);

	u8SensorImageMode = pstSnsState->u8ImgMode;

	if (pstSensorImageMode->f32Fps <= 25) {
		if (PR2020_RES_IS_720P(pstSensorImageMode->u16Width, pstSensorImageMode->u16Height)) {
			u8SensorImageMode = PR2020_MODE_720P_25;
		} else if (PR2020_RES_IS_1080P(pstSensorImageMode->u16Width, pstSensorImageMode->u16Height)) {
			u8SensorImageMode = PR2020_MODE_1080P_25;
		} else {
			CVI_TRACE_SNS(CVI_DBG_ERR, "Not support! Width:%d, Height:%d, Fps:%f, WDRMode:%d\n",
				pstSensorImageMode->u16Width,
				pstSensorImageMode->u16Height,
				pstSensorImageMode->f32Fps,
				pstSnsState->enWDRMode);
			return CVI_FAILURE;
		}
	} else if (pstSensorImageMode->f32Fps <= 30) {
		if (PR2020_RES_IS_720P(pstSensorImageMode->u16Width, pstSensorImageMode->u16Height)) {
			u8SensorImageMode = PR2020_MODE_720P_30;
		} else if (PR2020_RES_IS_1080P(pstSensorImageMode->u16Width, pstSensorImageMode->u16Height)) {
			u8SensorImageMode = PR2020_MODE_1080P_30;
		} else {
			CVI_TRACE_SNS(CVI_DBG_ERR, "Not support! Width:%d, Height:%d, Fps:%f, WDRMode:%d\n",
				pstSensorImageMode->u16Width,
				pstSensorImageMode->u16Height,
				pstSensorImageMode->f32Fps,
				pstSnsState->enWDRMode);
			return CVI_FAILURE;
		}
	}

	if ((pstSnsState->bInit == CVI_TRUE) && (u8SensorImageMode == pstSnsState->u8ImgMode)) {
		/* Don't need to switch SensorImageMode */
		return CVI_FAILURE;
	}

	pstSnsState->u8ImgMode = u8SensorImageMode;

	return CVI_SUCCESS;
}

static CVI_VOID sensor_global_init(VI_PIPE ViPipe)
{
	ISP_SNS_STATE_S *pstSnsState = CVI_NULL;

	PR2020_SENSOR_GET_CTX(ViPipe, pstSnsState);
	CMOS_CHECK_POINTER_VOID(pstSnsState);

	pstSnsState->bInit = CVI_FALSE;
	pstSnsState->u8ImgMode = PR2020_MODE_1080P_25;
	pstSnsState->enWDRMode = WDR_MODE_NONE;
}

static CVI_S32 sensor_rx_attr(VI_PIPE ViPipe, SNS_COMBO_DEV_ATTR_S *pstRxAttr)
{
	ISP_SNS_STATE_S *pstSnsState = CVI_NULL;
	SNS_COMBO_DEV_ATTR_S *pstRxAttrSrc = CVI_NULL;

	PR2020_SENSOR_GET_CTX(ViPipe, pstSnsState);
	Pr2020_SENSOR_GET_COMBO(ViPipe, pstRxAttrSrc);
	CMOS_CHECK_POINTER(pstSnsState);
	CMOS_CHECK_POINTER(pstRxAttr);
	CMOS_CHECK_POINTER(pstRxAttrSrc);

	memcpy(pstRxAttr, pstRxAttrSrc, sizeof(*pstRxAttr));

	for (int i = 0; i < TTL_PIN_FUNC_NUM; i++) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "get rx funcid[%d] : %d\n", i, pstRxAttr->ttl_attr.func[i]);
	}
	pstRxAttr->img_size.width = g_astPr2020_mode[pstSnsState->u8ImgMode].astImg[0].stSnsSize.u32Width;
	pstRxAttr->img_size.height = g_astPr2020_mode[pstSnsState->u8ImgMode].astImg[0].stSnsSize.u32Height;

	pstRxAttrSrc = CVI_NULL;
	return CVI_SUCCESS;
}

static CVI_S32 sensor_patch_rx_attr(VI_PIPE ViPipe, RX_INIT_ATTR_S *pstRxInitAttr)
{
	int i;
	SNS_COMBO_DEV_ATTR_S *pstRxAttr = CVI_NULL;

	if (!g_pastPr2020ComboDevArray[ViPipe]) {
		pstRxAttr = malloc(sizeof(SNS_COMBO_DEV_ATTR_S));
	} else {
		Pr2020_SENSOR_GET_COMBO(ViPipe, pstRxAttr);
	}
	memcpy(pstRxAttr, &pr2020_rx_attr, sizeof(SNS_COMBO_DEV_ATTR_S));
	Pr2020_SENSOR_SET_COMBO(ViPipe, pstRxAttr);

	CMOS_CHECK_POINTER(pstRxInitAttr);

	if (pstRxInitAttr->stMclkAttr.bMclkEn)
		pstRxAttr->mclk.cam = pstRxInitAttr->stMclkAttr.u8Mclk;

	if (pstRxInitAttr->MipiDev >= VI_MAX_DEV_NUM + 3)
		return CVI_SUCCESS;

	pstRxAttr->devno = pstRxInitAttr->MipiDev;
	CVI_TRACE_SNS(CVI_DBG_ERR, "Sensor [%d] use Mipi Dev[%d]\n", ViPipe, pstRxInitAttr->MipiDev);
	if (pstRxAttr->input_mode == INPUT_MODE_BT656_9B) {
		for (i = 0; i < TTL_PIN_FUNC_NUM; i++) {
			CVI_TRACE_SNS(CVI_DBG_ERR, "Input funcid[%d] : %d\n", i, pstRxInitAttr->as16FuncId[i]);
			pstRxAttr->ttl_attr.func[i] = pstRxInitAttr->as16FuncId[i];
		}
	}
	pstRxAttr = CVI_NULL;

	return CVI_SUCCESS;
}

void pr2020_sensor_exit(VI_PIPE ViPipe)
{
	if (g_pastPr2020ComboDevArray[ViPipe]) {
		free(g_pastPr2020ComboDevArray[ViPipe]);
		g_pastPr2020ComboDevArray[ViPipe] = CVI_NULL;
	}
	pr2020_exit(ViPipe);
}

static CVI_S32 cmos_init_sensor_exp_function(ISP_SENSOR_EXP_FUNC_S *pstSensorExpFunc)
{
	CMOS_CHECK_POINTER(pstSensorExpFunc);

	memset(pstSensorExpFunc, 0, sizeof(ISP_SENSOR_EXP_FUNC_S));

	pstSensorExpFunc->pfn_cmos_sensor_init = pr2020_init;
	pstSensorExpFunc->pfn_cmos_sensor_exit = pr2020_sensor_exit;
	pstSensorExpFunc->pfn_cmos_sensor_global_init = sensor_global_init;
	pstSensorExpFunc->pfn_cmos_set_image_mode = cmos_set_image_mode;
	pstSensorExpFunc->pfn_cmos_get_sns_reg_info = cmos_get_sns_regs_info;

	return CVI_SUCCESS;
}

/****************************************************************************
 * callback structure                                                       *
 ****************************************************************************/
static CVI_VOID sensor_patch_i2c_addr(VI_PIPE ViPipe, CVI_S32 s32I2cAddr)
{
	if (Pr2020_I2C_ADDR_IS_VALID(s32I2cAddr))
		g_aunPr2020_AddrInfo[ViPipe].s8I2cAddr = s32I2cAddr;
	else {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C addr input error ,please check [0x%x]\n", s32I2cAddr);
		g_aunPr2020_AddrInfo[ViPipe].s8I2cAddr = Pr2020_I2C_ADDR_2;
	}
}

static CVI_S32 pr2020_set_bus_info(VI_PIPE ViPipe, ISP_SNS_COMMBUS_U unSNSBusInfo)
{
	g_aunPr2020_BusInfo[ViPipe].s8I2cDev = unSNSBusInfo.s8I2cDev;

	return CVI_SUCCESS;
}

static CVI_S32 sensor_ctx_init(VI_PIPE ViPipe)
{
	ISP_SNS_STATE_S *pastSnsStateCtx = CVI_NULL;

	PR2020_SENSOR_GET_CTX(ViPipe, pastSnsStateCtx);

	if (pastSnsStateCtx == CVI_NULL) {
		pastSnsStateCtx = (ISP_SNS_STATE_S *)malloc(sizeof(ISP_SNS_STATE_S));
		if (pastSnsStateCtx == CVI_NULL) {
			CVI_TRACE_SNS(CVI_DBG_ERR, "Isp[%d] SnsCtx malloc memory failed!\n", ViPipe);
			return -ENOMEM;
		}
	}

	memset(pastSnsStateCtx, 0, sizeof(ISP_SNS_STATE_S));
	PR2020_SENSOR_SET_CTX(ViPipe, pastSnsStateCtx);

	return CVI_SUCCESS;
}

static CVI_VOID sensor_ctx_exit(VI_PIPE ViPipe)
{
	ISP_SNS_STATE_S *pastSnsStateCtx = CVI_NULL;

	PR2020_SENSOR_GET_CTX(ViPipe, pastSnsStateCtx);
	SENSOR_FREE(pastSnsStateCtx);
	PR2020_SENSOR_RESET_CTX(ViPipe);
}

static CVI_S32 sensor_register_callback(VI_PIPE ViPipe, ALG_LIB_S *pstAeLib, ALG_LIB_S *pstAwbLib)
{
	(void) pstAeLib;
	(void) pstAwbLib;

	CVI_S32 s32Ret;
	ISP_SENSOR_REGISTER_S stIspRegister;
	ISP_SNS_ATTR_INFO_S   stSnsAttrInfo;

	s32Ret = sensor_ctx_init(ViPipe);

	if (s32Ret != CVI_SUCCESS)
		return CVI_FAILURE;

	stSnsAttrInfo.eSensorId = PR2020_ID;

	s32Ret  = cmos_init_sensor_exp_function(&stIspRegister.stSnsExp);
	s32Ret |= CVI_ISP_SensorRegCallBack(ViPipe, &stSnsAttrInfo, &stIspRegister);

	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "sensor register callback function failed!\n");
		return s32Ret;
	}

	return CVI_SUCCESS;
}

static CVI_S32 sensor_unregister_callback(VI_PIPE ViPipe, ALG_LIB_S *pstAeLib, ALG_LIB_S *pstAwbLib)
{
	(void) pstAeLib;
	(void) pstAwbLib;

	CVI_S32 s32Ret;

	s32Ret = CVI_ISP_SensorUnRegCallBack(ViPipe, PR2020_ID);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "sensor unregister callback function failed!\n");
		return s32Ret;
	}

	sensor_ctx_exit(ViPipe);

	return CVI_SUCCESS;
}

ISP_SNS_OBJ_S stSnsPR2020_Obj = {
	.pfnRegisterCallback    = sensor_register_callback,
	.pfnUnRegisterCallback  = sensor_unregister_callback,
	.pfnStandby             = CVI_NULL,
	.pfnRestart             = CVI_NULL,
	.pfnMirrorFlip          = CVI_NULL,
	.pfnWriteReg            = pr2020_write_register,
	.pfnReadReg             = pr2020_read_register,
	.pfnSetBusInfo          = pr2020_set_bus_info,
	.pfnSetInit             = CVI_NULL,
	.pfnPatchRxAttr         = sensor_patch_rx_attr,
	.pfnPatchI2cAddr        = sensor_patch_i2c_addr,
	.pfnGetRxAttr           = sensor_rx_attr,
	.pfnExpSensorCb         = cmos_init_sensor_exp_function,
	.pfnExpAeCb             = CVI_NULL,
	.pfnSnsProbe            = CVI_NULL,
};
