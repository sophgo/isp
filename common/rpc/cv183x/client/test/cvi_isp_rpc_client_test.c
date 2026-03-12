#include "cvi_isp.h"
#include "cvi_ae.h"
#include "cvi_awb.h"

#define LOGOUT(fmt, arg...) printf("%s,%d: " fmt, __func__, __LINE__, ##arg)

#define AE_LOG_BUF_SIZE (256 * 1024)
#define AE_PATH_BUF_SIZE 64

static union {
	ISP_PUB_ATTR_S stPubAttr;
	ISP_MOD_PARAM_S stModParam;
	ISP_CTRL_PARAM_S stIspCtrlParam;
	ISP_MODULE_CTRL_U unModCtrl;
	ISP_COLOR_TONE_ATTR_S stWBGAttr;
	ISP_DEHAZE_ATTR_S stDehazeAttr;
	ISP_GAMMA_ATTR_S stGammaAttr;
	ISP_AUTO_GAMMA_ATTR_S stAutoGammaAttr;
	ISP_STATISTICS_CFG_S stStatCfg;
	ISP_NR_ATTR_S stNRAttr;
	ISP_NR_FILTER_ATTR_S stNRFilterAttr;
	ISP_RLSC_ATTR_S stRLSCAttr;
	ISP_YNR_ATTR_S stYNRAttr;
	ISP_YNR_MOTION_NR_ATTR_S stYNRMotionNRAttr;
	ISP_YNR_FILTER_ATTR_S stYNRFilterAttr;
	ISP_CNR_ATTR_S stCNRAttr;
	ISP_CAC_ATTR_S stCACAttr;
	ISP_DCI_ATTR_S stDCIAttr;
	ISP_MESH_SHADING_ATTR_S stMeshShadingAttr;
	ISP_MESH_SHADING_GAIN_LUT_ATTR_S stMeshShadingGainLutAttr;
	ISP_RADIAL_SHADING_ATTR_S stRadialShadingAttr;
	ISP_RADIAL_SHADING_GAIN_LUT_ATTR_S stRadialShadingGainLutAttr;
	ISP_TNR_ATTR_S stTNRAttr;
	ISP_TNR_NOISE_MODEL_ATTR_S stTNRNoiseModelAttr;
	ISP_TNR_LUMA_MOTION_ATTR_S stTNRLumaMotionAttr;
	ISP_TNR_GHOST_ATTR_S stTNRGhostAttr;
	ISP_TNR_MT_PRT_ATTR_S stTNRMtPrtAttr;
	ISP_CLUT_ATTR_S stCLutAttr;
#ifdef ARCH_CV183X
	ISP_CLUT_LUT_S stClutLUT;
#endif
	ISP_BLACK_LEVEL_ATTR_S stBlackLevelAttr;
	ISP_DEMOSAIC_ATTR_S stDemosaicAttr;
	ISP_DEMOSAIC_DEMOIRE_ATTR_S stDemosaicDemoireAttr;
	ISP_DEMOSAIC_FILTER_ATTR_S stDemosaicFilterAttr;
#ifdef ARCH_CV183X
	ISP_DEMOSAIC_EE_ATTR_S stDemosaicEEAttr;
#endif
	ISP_SATURATION_ATTR_S stSaturationAttr;
	ISP_CCM_ATTR_S stCCMAttr;
#ifdef ARCH_CV183X
	ISP_HSV_ATTR_S stHSVAttr;
#endif
	ISP_DP_DYNAMIC_ATTR_S stDPCDynamicAttr;
	ISP_DP_STATIC_ATTR_S stDPStaticAttr;
	ISP_DP_CALIB_ATTR_S stDPCalibAttr;
	ISP_CROSSTALK_ATTR_S stCrosstalkAttr;
	ISP_AE_STATISTICS_S stAeStat;
	ISP_WB_STATISTICS_S stWBStat;
	ISP_AF_STATISTICS_S stAfStat;
	ISP_AWB_LightBox_Gain_S stAWBLightBoxGain;
	ISP_FSWDR_ATTR_S stFSWDRAttr;
	ISP_DRC_ATTR_S stDRCAttr;
	ISP_SHARPEN_ATTR_S stSharpenAttr;
	ISP_CMOS_NOISE_CALIBRATION_S stNoiseProfileAttr;
	ISP_YCONTRAST_ATTR_S stYContrastAttr;
	ISP_DCF_INFO_S stIspDCF;
	ISP_INNER_STATE_INFO_S stInnerStateInfo;
	ISP_MONO_ATTR_S stMonoAttr;
	ISP_DIS_ATTR_S stDisAttr;
	ISP_DIS_CONFIG_S stDisConfig;
	ISP_VC_ATTR_S stVCAttr;
	ISP_SMART_INFO_S stSmartInfoAttr;
#ifdef ARCH_CV182X
	ISP_TNR_MOTION_ADAPT_ATTR_S stMotionAdaptAttr;
	ISP_CLUT_SATURATION_ATTR_S stClutSaturationAttr;
	ISP_GAMMA_ATTR_S stGammaCurve;
	ISP_PRESHARPEN_ATTR_S stPreSharpenAttr;
	ISP_RGBCAC_ATTR_S stRGBCACAttr;
	ISP_CNR_MOTION_NR_ATTR_S stCNRMotionNRAttr;
	ISP_CCM_SATURATION_ATTR_S stCCMSaturationAttr;
	ISP_CA_ATTR_S stCAAttr;
#endif
	ISP_EXPOSURE_ATTR_S stExpAttr;
	ISP_EXP_INFO_S stExpInfo;
	ISP_WDR_EXPOSURE_ATTR_S stWDRExpAttr;
	ISP_AE_ROUTE_S stAERouteAttr;
	ISP_AE_ROUTE_EX_S stAERouteAttrEx;
	ISP_AE_STATISTICS_CFG_S stAeStatisticsCfg;
	ISP_SMART_EXPOSURE_ATTR_S stSmartExpAttr;
	ISP_WB_ATTR_S stWBAttr;
	ISP_AWB_ATTR_EX_S stAWBAttrEx;
	ISP_WB_INFO_S stWBInfo;
	ISP_AWB_Calibration_Gain_S stWBCalib;
	ISP_AWB_Calibration_Gain_S_EX stWBCalibEx;
	ISP_WB_STATISTICS_CFG_S stAwbWinCfg;
	ISP_WB_CURVE_S shWBCurve;
}  data,setData;


#define RPC_RW_TEST(__TYPE, __NAME, __GET_FUN,__SET_FUN,__DBG) ({              \
		result = __SET_FUN(ViPipe, &data.__NAME);                              \
		if(result != CVI_SUCCESS){printf("%s write test fail \n",__DBG);}     \
		result = __GET_FUN(ViPipe, &setData.__NAME);                           \
		if(result == CVI_SUCCESS){                                             \
			if(memcmp(&setData.__NAME,&data.__NAME,sizeof(__TYPE)) == 0 ){     \
				printf("%s rw test success\n",__DBG);                          \
			}else{                                                             \
				printf("%s rw test fail\n",__DBG);                             \
			}                                                                  \
		}else{                                                                 \
			printf("%s read test fail \n",__DBG);                              \
		}})                                                                    \

#define PRINT_RW_RESULT(result,__DBG) ({                     \
		if(result != CVI_SUCCESS){                             \
			printf("%s read test fail\n",__DBG);               \
		}else{                                                 \
			printf("%s read test success\n",__DBG);            \
		}})                                                    \

static void rw_test(VI_PIPE ViPipe)
{
	CVI_S32 result;
	memset(&data.stPubAttr, 0, sizeof(ISP_PUB_ATTR_S));
	CVI_ISP_GetPubAttr(ViPipe, &data.stPubAttr);
	data.stPubAttr.f32FrameRate -= 1;
	RPC_RW_TEST(ISP_PUB_ATTR_S,stPubAttr,CVI_ISP_GetPubAttr,CVI_ISP_SetPubAttr,"PubAttr");

	memset(&data.stModParam, 0, sizeof(ISP_MOD_PARAM_S));
	CVI_ISP_GetModParam(&data.stModParam);
	//data.stModParam.u32IntBotHalf += 1;
	result = CVI_ISP_SetModParam(&data.stModParam);
	if(result != CVI_SUCCESS){
		printf("ModParam write test fail");
	}
	result = CVI_ISP_GetModParam(&setData.stModParam);
	if(result != CVI_SUCCESS){
		printf("ModParam write test fail");
	}else{
		if(memcmp(&setData.stModParam,&data.stModParam,sizeof(ISP_MOD_PARAM_S)) == 0 ){
			printf("ModParam rw test success\n");
		}else{
			printf("ModParam rw test fail\n");
		}
	}

	memset(&data.stIspCtrlParam, 0, sizeof(ISP_CTRL_PARAM_S));
	CVI_ISP_GetCtrlParam(ViPipe, &data.stIspCtrlParam);
	data.stIspCtrlParam.u32PwmNumber += 1;
	RPC_RW_TEST(ISP_CTRL_PARAM_S,stIspCtrlParam,CVI_ISP_GetCtrlParam,CVI_ISP_SetCtrlParam,"IspCtrlParam");

	ISP_FMW_STATE_E enFmwState;
	enFmwState = ISP_FMW_STATE_FREEZE;
	CVI_ISP_SetFMWState(ViPipe, enFmwState);
	CVI_ISP_GetFMWState(ViPipe, &enFmwState);
	if(enFmwState == ISP_FMW_STATE_FREEZE){
		printf("FMWState rw test success\n");
	}else{
		printf("FMWState rw test fail\n");
	}


	memset(&data.unModCtrl, 0, sizeof(ISP_MODULE_CTRL_U));
	CVI_ISP_GetModuleControl(ViPipe, &data.unModCtrl);
	data.unModCtrl.u64Key += 1;
	RPC_RW_TEST(ISP_MODULE_CTRL_U,unModCtrl,CVI_ISP_GetModuleControl,CVI_ISP_SetModuleControl,"ModCtrl");

	memset(&data.stWBGAttr, 0, sizeof(ISP_COLOR_TONE_ATTR_S));
	CVI_ISP_GetColorToneAttr(ViPipe, &data.stWBGAttr);
	data.stWBGAttr.wbg_enable = (data.stWBGAttr.wbg_enable == 1 ? 0 : 1);
	RPC_RW_TEST(ISP_COLOR_TONE_ATTR_S,stWBGAttr,CVI_ISP_GetColorToneAttr,CVI_ISP_SetColorToneAttr,"WBGAttr");

	memset(&data.stDehazeAttr, 0, sizeof(ISP_DEHAZE_ATTR_S));
	CVI_ISP_GetDehazeAttr(ViPipe, &data.stDehazeAttr);
	data.stDehazeAttr.Enable = (data.stDehazeAttr.Enable == 1 ? 0 : 1);
	RPC_RW_TEST(ISP_DEHAZE_ATTR_S,stDehazeAttr,CVI_ISP_GetDehazeAttr,CVI_ISP_SetDehazeAttr,"DehazeAttr");

	memset(&data, 0, sizeof(ISP_GAMMA_ATTR_S));
	CVI_ISP_GetGammaAttr(ViPipe, &data.stGammaAttr);
	data.stGammaAttr.Enable = (data.stGammaAttr.Enable == 1 ? 0 : 1);
	RPC_RW_TEST(ISP_GAMMA_ATTR_S,stGammaAttr,CVI_ISP_GetGammaAttr,CVI_ISP_SetGammaAttr,"GammaAttr");

	memset(&data, 0, sizeof(data));
	CVI_ISP_GetAutoGammaAttr(ViPipe, &data.stAutoGammaAttr);
	//data.stAutoGammaAttr.GammaTabNum += 1;
	RPC_RW_TEST(ISP_AUTO_GAMMA_ATTR_S,stAutoGammaAttr,CVI_ISP_GetAutoGammaAttr,CVI_ISP_SetAutoGammaAttr,"AutoGammaAttr");

	memset(&data, 0, sizeof(data));
	CVI_ISP_GetStatisticsConfig(ViPipe, &data.stStatCfg);
	data.stStatCfg.unKey.u64Key +=1;
	RPC_RW_TEST(ISP_STATISTICS_CFG_S,stStatCfg,CVI_ISP_GetStatisticsConfig,CVI_ISP_SetStatisticsConfig,"StatCfg");

	memset(&data, 0, sizeof(data));
	CVI_ISP_GetNRAttr(ViPipe, &data.stNRAttr);
	data.stNRAttr.Enable = (data.stNRAttr.Enable == 1 ? 0 : 1);
	RPC_RW_TEST(ISP_NR_ATTR_S,stNRAttr,CVI_ISP_GetNRAttr,CVI_ISP_SetNRAttr,"NRAttr");

	memset(&data, 0, sizeof(data));
	CVI_ISP_GetNRFilterAttr(ViPipe, &data.stNRFilterAttr);
	data.stNRFilterAttr.TuningMode += 1;
	RPC_RW_TEST(ISP_NR_FILTER_ATTR_S,stNRFilterAttr,CVI_ISP_GetNRFilterAttr,CVI_ISP_SetNRFilterAttr,"NRFilterAttr");

	memset(&data, 0, sizeof(data));
	CVI_ISP_GetRLSCAttr(ViPipe, &data.stRLSCAttr);
	data.stRLSCAttr.RlscEnable = (data.stRLSCAttr.RlscEnable == 1 ? 0 : 1);
	RPC_RW_TEST(ISP_RLSC_ATTR_S,stRLSCAttr,CVI_ISP_GetRLSCAttr,CVI_ISP_SetRLSCAttr,"RLSCAttr");

	memset(&data, 0, sizeof(data));
	CVI_ISP_GetYNRAttr(ViPipe, &data.stYNRAttr);
	data.stYNRAttr.Enable = (data.stYNRAttr.Enable == 1 ? 0 : 1);
	RPC_RW_TEST(ISP_YNR_ATTR_S,stYNRAttr,CVI_ISP_GetYNRAttr,CVI_ISP_SetYNRAttr,"YNRAttr");

	memset(&data, 0, sizeof(data));
	CVI_ISP_GetYNRMotionNRAttr(ViPipe, &data.stYNRMotionNRAttr);
	data.stYNRMotionNRAttr.enOpType = OP_TYPE_MANUAL;
	RPC_RW_TEST(ISP_YNR_MOTION_NR_ATTR_S,stYNRMotionNRAttr,CVI_ISP_GetYNRMotionNRAttr,CVI_ISP_SetYNRMotionNRAttr,"YNRMotionNRAttr");

	memset(&data, 0, sizeof(data));
	CVI_ISP_GetYNRFilterAttr(ViPipe, &data.stYNRFilterAttr);
	data.stYNRFilterAttr.enOpType = OP_TYPE_MANUAL;
	RPC_RW_TEST(ISP_YNR_FILTER_ATTR_S,stYNRFilterAttr,CVI_ISP_GetYNRFilterAttr,CVI_ISP_SetYNRFilterAttr,"YNRFilterAttr");

	memset(&data, 0, sizeof(data));
	CVI_ISP_GetCNRAttr(ViPipe, &data.stCNRAttr);
	data.stCNRAttr.Enable = (data.stCNRAttr.Enable == 1 ? 0 : 1);
	RPC_RW_TEST(ISP_CNR_ATTR_S,stCNRAttr,CVI_ISP_GetCNRAttr,CVI_ISP_SetCNRAttr,"CNRAttr");

	memset(&data, 0, sizeof(data));
	CVI_ISP_GetCACAttr(ViPipe, &data.stCACAttr);
	data.stCACAttr.Enable = (data.stCACAttr.Enable == 1 ? 0 : 1);
	RPC_RW_TEST(ISP_CAC_ATTR_S,stCACAttr,CVI_ISP_GetCACAttr,CVI_ISP_SetCACAttr,"CACAttr");

	memset(&data, 0, sizeof(data));
	CVI_ISP_GetDCIAttr(ViPipe, &data.stDCIAttr);
	data.stDCIAttr.Enable = (data.stDCIAttr.Enable == 1 ? 0 : 1);
	RPC_RW_TEST(ISP_DCI_ATTR_S,stDCIAttr,CVI_ISP_GetDCIAttr,CVI_ISP_SetDCIAttr,"DCIAttr");

	memset(&data, 0, sizeof(data));
	CVI_ISP_GetMeshShadingAttr(ViPipe, &data.stMeshShadingAttr);
	data.stMeshShadingAttr.Enable = (data.stMeshShadingAttr.Enable == 1 ? 0 : 1);
	RPC_RW_TEST(ISP_MESH_SHADING_ATTR_S,stMeshShadingAttr,CVI_ISP_GetMeshShadingAttr,CVI_ISP_SetMeshShadingAttr,"MeshShadingAttr");

	memset(&data, 0, sizeof(data));
	CVI_ISP_GetMeshShadingGainLutAttr(ViPipe, &data.stMeshShadingGainLutAttr);
	data.stMeshShadingGainLutAttr.Size += 1;
	RPC_RW_TEST(ISP_MESH_SHADING_GAIN_LUT_ATTR_S,stMeshShadingGainLutAttr,CVI_ISP_GetMeshShadingGainLutAttr,CVI_ISP_SetMeshShadingGainLutAttr,"MeshShadingGainLutAttr");

	memset(&data, 0, sizeof(data));
	CVI_ISP_GetRadialShadingAttr(ViPipe, &data.stRadialShadingAttr);
	data.stRadialShadingAttr.Enable = (data.stRadialShadingAttr.Enable == 1 ? 0 : 1);
	RPC_RW_TEST(ISP_RADIAL_SHADING_ATTR_S,stRadialShadingAttr,CVI_ISP_GetRadialShadingAttr,CVI_ISP_SetRadialShadingAttr,"RadialShadingAttr");

	memset(&data, 0, sizeof(data));
	CVI_ISP_GetRadialShadingGainLutAttr(ViPipe, &data.stRadialShadingGainLutAttr);
	data.stRadialShadingGainLutAttr.Size += 1;
	RPC_RW_TEST(ISP_RADIAL_SHADING_GAIN_LUT_ATTR_S,stRadialShadingGainLutAttr,CVI_ISP_GetRadialShadingGainLutAttr,CVI_ISP_SetRadialShadingGainLutAttr,"RadialShadingGainLutAttr");

	memset(&data, 0, sizeof(data));
	CVI_ISP_GetTNRAttr(ViPipe, &data.stTNRAttr);
	data.stTNRAttr.Enable = (data.stTNRAttr.Enable == 1 ? 0 : 1);
	RPC_RW_TEST(ISP_TNR_ATTR_S,stTNRAttr,CVI_ISP_GetTNRAttr,CVI_ISP_SetTNRAttr,"TNRAttr");

	memset(&data, 0, sizeof(data));
	CVI_ISP_GetTNRNoiseModelAttr(ViPipe, &data.stTNRNoiseModelAttr);
	data.stTNRNoiseModelAttr.enOpType = OP_TYPE_MANUAL;
	RPC_RW_TEST(ISP_TNR_NOISE_MODEL_ATTR_S,stTNRNoiseModelAttr,CVI_ISP_GetTNRNoiseModelAttr,CVI_ISP_SetTNRNoiseModelAttr,"TNRNoiseModelAttr");

	memset(&data, 0, sizeof(data));
	CVI_ISP_GetTNRLumaMotionAttr(ViPipe, &data.stTNRLumaMotionAttr);
	data.stTNRLumaMotionAttr.enOpType = OP_TYPE_MANUAL;
	RPC_RW_TEST(ISP_TNR_LUMA_MOTION_ATTR_S,stTNRLumaMotionAttr,CVI_ISP_GetTNRLumaMotionAttr,CVI_ISP_SetTNRLumaMotionAttr,"TNRLumaMotionAttr");

	memset(&data, 0, sizeof(data));
	CVI_ISP_GetTNRGhostAttr(ViPipe, &data.stTNRGhostAttr);
	data.stTNRGhostAttr.enOpType = OP_TYPE_MANUAL;
	RPC_RW_TEST(ISP_TNR_GHOST_ATTR_S,stTNRGhostAttr,CVI_ISP_GetTNRGhostAttr,CVI_ISP_SetTNRGhostAttr,"TNRGhostAttr");

	memset(&data, 0, sizeof(data));
	CVI_ISP_GetTNRMtPrtAttr(ViPipe, &data.stTNRMtPrtAttr);
	data.stTNRMtPrtAttr.LowMtPrtEn = (data.stTNRMtPrtAttr.LowMtPrtEn == 1 ? 0 : 1);
	RPC_RW_TEST(ISP_TNR_MT_PRT_ATTR_S,stTNRMtPrtAttr,CVI_ISP_GetTNRMtPrtAttr,CVI_ISP_SetTNRMtPrtAttr,"TNRMtPrtAttr");

	memset(&data, 0, sizeof(data));
	CVI_ISP_GetClutAttr(ViPipe, &data.stCLutAttr);
	data.stCLutAttr.Enable = (data.stCLutAttr.Enable == 1 ? 0 : 1);
	RPC_RW_TEST(ISP_CLUT_ATTR_S,stCLutAttr,CVI_ISP_GetClutAttr,CVI_ISP_SetClutAttr,"CLutAttr");

	memset(&data, 0, sizeof(data));
	CVI_ISP_GetBlackLevelAttr(ViPipe, &data.stBlackLevelAttr);
	data.stBlackLevelAttr.Enable = (data.stBlackLevelAttr.Enable  == 1 ? 0 : 1);
	RPC_RW_TEST(ISP_BLACK_LEVEL_ATTR_S,stBlackLevelAttr,CVI_ISP_GetBlackLevelAttr,CVI_ISP_SetBlackLevelAttr,"BlackLevelAttr");

	memset(&data, 0, sizeof(data));
	CVI_ISP_GetDemosaicAttr(ViPipe, &data.stDemosaicAttr);
	data.stDemosaicAttr.Enable = (data.stDemosaicAttr.Enable  == 1 ? 0 : 1);
	RPC_RW_TEST(ISP_DEMOSAIC_ATTR_S,stDemosaicAttr,CVI_ISP_GetDemosaicAttr,CVI_ISP_SetDemosaicAttr,"DemosaicAttr");

	memset(&data, 0, sizeof(data));
	CVI_ISP_GetDemosaicDemoireAttr(ViPipe, &data.stDemosaicDemoireAttr);
	data.stDemosaicDemoireAttr.enOpType = OP_TYPE_MANUAL;
	RPC_RW_TEST(ISP_DEMOSAIC_DEMOIRE_ATTR_S,stDemosaicDemoireAttr,CVI_ISP_GetDemosaicDemoireAttr,CVI_ISP_SetDemosaicDemoireAttr,"DemosaicDemoireAttr");

	memset(&data, 0, sizeof(data));
	CVI_ISP_GetDemosaicFilterAttr(ViPipe, &data.stDemosaicFilterAttr);
	data.stDemosaicFilterAttr.enOpType = OP_TYPE_MANUAL;
	RPC_RW_TEST(ISP_DEMOSAIC_FILTER_ATTR_S,stDemosaicFilterAttr,CVI_ISP_GetDemosaicFilterAttr,CVI_ISP_SetDemosaicFilterAttr,"DemosaicFilterAttr");
#ifdef ARCH_CV183X
	memset(&data, 0, sizeof(data));
	CVI_ISP_GetDemosaicEEAttr(ViPipe, &data.stDemosaicEEAttr);
	data.stDemosaicEEAttr.LumaTunedCoringEn = (data.stDemosaicEEAttr.LumaTunedCoringEn  == 1 ? 0 : 1);
	RPC_RW_TEST(ISP_DEMOSAIC_EE_ATTR_S,stDemosaicEEAttr,CVI_ISP_GetDemosaicEEAttr,CVI_ISP_SetDemosaicEEAttr,"DemosaicEEAttr");
#endif
	memset(&data, 0, sizeof(data));
	CVI_ISP_GetSaturationAttr(ViPipe, &data.stSaturationAttr);
	data.stSaturationAttr.enOpType = OP_TYPE_MANUAL;
	RPC_RW_TEST(ISP_SATURATION_ATTR_S,stSaturationAttr,CVI_ISP_GetSaturationAttr,CVI_ISP_SetSaturationAttr,"SaturationAttr");

	memset(&data, 0, sizeof(data));
	CVI_ISP_GetCCMAttr(ViPipe, &data.stCCMAttr);
	data.stCCMAttr.Enable = (data.stCCMAttr.Enable  == 1 ? 0 : 1);
	RPC_RW_TEST(ISP_CCM_ATTR_S,stCCMAttr,CVI_ISP_GetCCMAttr,CVI_ISP_SetCCMAttr,"CCMAttr");
#ifdef ARCH_CV183X
	memset(&data, 0, sizeof(data));
	CVI_ISP_GetHSVAttr(ViPipe, &data.stHSVAttr);
	data.stHSVAttr.stHbyHAttr.Enable = (data.stHSVAttr.stHbyHAttr.Enable  == 1 ? 0 : 1);
	RPC_RW_TEST(ISP_HSV_ATTR_S,stHSVAttr,CVI_ISP_GetHSVAttr,CVI_ISP_SetHSVAttr,"HSVAttr");
#endif
	memset(&data, 0, sizeof(data));
	CVI_ISP_GetDPDynamicAttr(ViPipe, &data.stDPCDynamicAttr);
	data.stDPCDynamicAttr.Enable = (data.stDPCDynamicAttr.Enable == 1 ? 0 : 1);
	RPC_RW_TEST(ISP_DP_DYNAMIC_ATTR_S,stDPCDynamicAttr,CVI_ISP_GetDPDynamicAttr,CVI_ISP_SetDPDynamicAttr,"DPCDynamicAttr");

	memset(&data, 0, sizeof(data));
	CVI_ISP_GetDPStaticAttr(ViPipe, &data.stDPStaticAttr);
	data.stDPStaticAttr.Enable = (data.stDPStaticAttr.Enable == 1 ? 0 : 1);
	RPC_RW_TEST(ISP_DP_STATIC_ATTR_S,stDPStaticAttr,CVI_ISP_GetDPStaticAttr,CVI_ISP_SetDPStaticAttr,"DPStaticAttr");

	memset(&data, 0, sizeof(data));
	CVI_ISP_GetDPCalibrate(ViPipe, &data.stDPCalibAttr);
	//data.stDPCalibAttr.EnableDetect = (data.stDPCalibAttr.EnableDetect == 1 ? 0 : 1);
	RPC_RW_TEST(ISP_DP_CALIB_ATTR_S,stDPCalibAttr,CVI_ISP_GetDPCalibrate,CVI_ISP_SetDPCalibrate,"DPCalibAttr");

	memset(&data, 0, sizeof(data));
	CVI_ISP_GetCrosstalkAttr(ViPipe, &data.stCrosstalkAttr);
	data.stCrosstalkAttr.Enable = (data.stCrosstalkAttr.Enable == 1 ? 0 : 1);
	RPC_RW_TEST(ISP_CROSSTALK_ATTR_S,stCrosstalkAttr,CVI_ISP_GetCrosstalkAttr,CVI_ISP_SetCrosstalkAttr,"CrosstalkAttr");

	memset(&data, 0, sizeof(data));
	result = CVI_ISP_GetAEStatistics(ViPipe, &data.stAeStat);
	PRINT_RW_RESULT(result,"AeStat");

	memset(&data, 0, sizeof(data));
	result = CVI_ISP_GetWBStatistics(ViPipe, &data.stWBStat);
	PRINT_RW_RESULT(result,"WBStat");

	memset(&data, 0, sizeof(data));
	result = CVI_ISP_GetFocusStatistics(ViPipe, &data.stAfStat);
	PRINT_RW_RESULT(result,"AfStat");

	memset(&data, 0, sizeof(data));
	result = CVI_ISP_GetLightboxGain(ViPipe, &data.stAWBLightBoxGain);
	PRINT_RW_RESULT(result,"AWBLightBoxGain");

	memset(&data, 0, sizeof(data));
	CVI_ISP_GetFSWDRAttr(ViPipe, &data.stFSWDRAttr);
	data.stFSWDRAttr.Enable = (data.stFSWDRAttr.Enable == 1 ? 0 : 1);
	RPC_RW_TEST(ISP_FSWDR_ATTR_S,stFSWDRAttr,CVI_ISP_GetFSWDRAttr,CVI_ISP_SetFSWDRAttr,"FSWDRAttr");

	memset(&data, 0, sizeof(data));
	CVI_ISP_GetDRCAttr(ViPipe, &data.stDRCAttr);
	data.stDRCAttr.SatEnable = (data.stDRCAttr.SatEnable == 1 ? 0 : 1);
	RPC_RW_TEST(ISP_DRC_ATTR_S,stDRCAttr,CVI_ISP_GetDRCAttr,CVI_ISP_SetDRCAttr,"DRCAttr");

	memset(&data, 0, sizeof(data));
	CVI_ISP_GetSharpenAttr(ViPipe, &data.stSharpenAttr);
	data.stSharpenAttr.Enable = (data.stSharpenAttr.Enable == 1 ? 0 : 1);
	RPC_RW_TEST(ISP_SHARPEN_ATTR_S,stSharpenAttr,CVI_ISP_GetSharpenAttr,CVI_ISP_SetSharpenAttr,"SharpenAttr");

	memset(&data, 0, sizeof(data));
	CVI_ISP_GetNoiseProfileAttr(ViPipe, &data.stNoiseProfileAttr);
	RPC_RW_TEST(ISP_CMOS_NOISE_CALIBRATION_S,stNoiseProfileAttr,CVI_ISP_GetNoiseProfileAttr,CVI_ISP_SetNoiseProfileAttr,"NoiseProfileAttr");

	memset(&data, 0, sizeof(data));
	CVI_ISP_GetYContrastAttr(ViPipe, &data.stYContrastAttr);
	data.stYContrastAttr.Enable = (data.stYContrastAttr.Enable == 1 ? 0 : 1);
	RPC_RW_TEST(ISP_YCONTRAST_ATTR_S,stYContrastAttr,CVI_ISP_GetYContrastAttr,CVI_ISP_SetYContrastAttr,"YContrastAttr");
#ifdef ARCH_CV183X
	memset(&data, 0, sizeof(data));
	CVI_ISP_GetDCFInfo(ViPipe, &data.stIspDCF);
	data.stIspDCF.stIspDCFConstInfo.u8SceneType =2;
	RPC_RW_TEST(ISP_DCF_INFO_S,stIspDCF,CVI_ISP_GetDCFInfo,CVI_ISP_SetDCFInfo,"IspDCF");
#endif
	memset(&data, 0, sizeof(data));
	result = CVI_ISP_QueryInnerStateInfo(ViPipe, &data.stInnerStateInfo);
	PRINT_RW_RESULT(result,"InnerStateInfo");

	memset(&data, 0, sizeof(data));
	CVI_ISP_GetMonoAttr(ViPipe, &data.stMonoAttr);
	data.stMonoAttr.Enable = (data.stMonoAttr.Enable == 1 ? 0 : 1);
	RPC_RW_TEST(ISP_MONO_ATTR_S,stMonoAttr,CVI_ISP_GetMonoAttr,CVI_ISP_SetMonoAttr,"MonoAttr");

	memset(&data, 0, sizeof(data));
	CVI_ISP_GetDisAttr(ViPipe, &data.stDisAttr);
	data.stDisAttr.enable = (data.stDisAttr.enable == 1 ? 0 : 1);
	RPC_RW_TEST(ISP_DIS_ATTR_S,stDisAttr,CVI_ISP_GetDisAttr,CVI_ISP_SetDisAttr,"DisAttr");

	memset(&data, 0, sizeof(data));
	CVI_ISP_GetDisConfig(ViPipe, &data.stDisConfig);
	data.stDisConfig.cropRatio += 1;
	RPC_RW_TEST(ISP_DIS_CONFIG_S,stDisConfig,CVI_ISP_GetDisConfig,CVI_ISP_SetDisConfig,"DisConfig");

	memset(&data, 0, sizeof(data));
	CVI_ISP_GetExposureAttr(ViPipe, &data.stExpAttr);
	data.stExpAttr.bByPass = (data.stExpAttr.bByPass == 1 ? 0 : 1);
	RPC_RW_TEST(ISP_EXPOSURE_ATTR_S,stExpAttr,CVI_ISP_GetExposureAttr,CVI_ISP_SetExposureAttr,"ExpAttr");

	memset(&data, 0, sizeof(data));
	result = CVI_ISP_QueryExposureInfo(ViPipe, &data.stExpInfo);
	PRINT_RW_RESULT(result,"ExpInfo");

	memset(&data, 0, sizeof(data));
	CVI_ISP_GetWDRExposureAttr(ViPipe, &data.stWDRExpAttr);
	data.stWDRExpAttr.u16Speed += 1;
	RPC_RW_TEST(ISP_WDR_EXPOSURE_ATTR_S,stWDRExpAttr,CVI_ISP_GetWDRExposureAttr,CVI_ISP_SetWDRExposureAttr,"WDRExpAttr");

	memset(&data, 0, sizeof(data));
	CVI_ISP_GetAERouteAttr(ViPipe, &data.stAERouteAttr);
	data.stAERouteAttr.u32TotalNum += 1;
	RPC_RW_TEST(ISP_AE_ROUTE_S,stAERouteAttr,CVI_ISP_GetAERouteAttr,CVI_ISP_SetAERouteAttr,"AERouteAttr");

	memset(&data, 0, sizeof(data));
	CVI_ISP_GetAERouteAttrEx(ViPipe, &data.stAERouteAttrEx);
	data.stAERouteAttrEx.u32TotalNum +=1;
	RPC_RW_TEST(ISP_AE_ROUTE_EX_S,stAERouteAttrEx,CVI_ISP_GetAERouteAttrEx,CVI_ISP_SetAERouteAttrEx,"AERouteAttrEx");

	memset(&data, 0, sizeof(data));
	CVI_ISP_GetAEStatisticsConfig(ViPipe, &data.stAeStatisticsCfg);
	data.stAeStatisticsCfg.bHisStatisticsEnable = (data.stAeStatisticsCfg.bHisStatisticsEnable == 1 ? 0 : 1);
	RPC_RW_TEST(ISP_AE_STATISTICS_CFG_S,stAeStatisticsCfg,CVI_ISP_GetAEStatisticsConfig,CVI_ISP_SetAEStatisticsConfig,"AeStatisticsCfg");

	memset(&data, 0, sizeof(data));
	CVI_ISP_GetWBAttr(ViPipe, &data.stWBAttr);
	data.stWBAttr.u8DebugMode = (data.stWBAttr.u8DebugMode == 1 ? 0 : 1);
	RPC_RW_TEST(ISP_WB_ATTR_S,stWBAttr,CVI_ISP_GetWBAttr,CVI_ISP_SetWBAttr,"WBAttr");

	memset(&data, 0, sizeof(data));
	CVI_ISP_GetAWBAttrEx(ViPipe, &data.stAWBAttrEx);
	data.stAWBAttrEx.bExtraLightEn = (data.stAWBAttrEx.bExtraLightEn == 1 ? 0 : 1);
	RPC_RW_TEST(ISP_AWB_ATTR_EX_S,stAWBAttrEx,CVI_ISP_GetAWBAttrEx,CVI_ISP_SetAWBAttrEx,"AWBAttrEx");

	memset(&data, 0, sizeof(data));
	result = CVI_ISP_QueryWBInfo(ViPipe, &data.stWBInfo);
	PRINT_RW_RESULT(result,"WBInfo");

	memset(&data, 0, sizeof(data));
	CVI_ISP_GetWBCalibration(ViPipe, &data.stWBCalib);
	RPC_RW_TEST(ISP_AWB_Calibration_Gain_S,stWBCalib,CVI_ISP_GetWBCalibration,CVI_ISP_SetWBCalibration,"WBCalib");

	memset(&data, 0, sizeof(data));
	CVI_ISP_GetWBCalibrationEx(ViPipe, &data.stWBCalibEx);
	RPC_RW_TEST(ISP_AWB_Calibration_Gain_S_EX,stWBCalibEx,CVI_ISP_GetWBCalibrationEx,CVI_ISP_SetWBCalibrationEx,"WBCalibEx");

	memset(&data, 0, sizeof(data));
	CVI_ISP_GetAWBWinStatictics(ViPipe, &data.stAwbWinCfg);
	data.stAwbWinCfg.u16BlackLevel +=1;
	RPC_RW_TEST(ISP_WB_STATISTICS_CFG_S,stAwbWinCfg,CVI_ISP_GetAWBWinStatictics,CVI_ISP_SetAWBWinStatictics,"AwbWinCfg");

	memset(&data, 0, sizeof(data));
	result = CVI_ISP_GetAWBCurve(ViPipe, &data.shWBCurve);
	PRINT_RW_RESULT(result,"WBCurve");

	memset(&data, 0, sizeof(data));
	CVI_ISP_GetVCAttr(ViPipe,&data.stVCAttr);
	data.stVCAttr.MotionThreshold +=1;
	RPC_RW_TEST(ISP_VC_ATTR_S,stVCAttr,CVI_ISP_GetVCAttr,CVI_ISP_SetVCAttr,"VCAttr");

#ifdef ARCH_CV182X
	memset(&data, 0, sizeof(data));
	CVI_ISP_GetTNRMotionAdaptAttr(ViPipe,&data.stMotionAdaptAttr);
	RPC_RW_TEST(ISP_TNR_MOTION_ADAPT_ATTR_S,stMotionAdaptAttr,CVI_ISP_GetTNRMotionAdaptAttr,CVI_ISP_SetTNRMotionAdaptAttr,"MotionAdaptAttr");

	memset(&data, 0, sizeof(data));
	CVI_ISP_GetClutSaturationAttr(ViPipe,&data.stClutSaturationAttr);
	data.stClutSaturationAttr.Enable = (data.stClutSaturationAttr.Enable == 1 ? 0 : 1);
	RPC_RW_TEST(ISP_CLUT_SATURATION_ATTR_S,stClutSaturationAttr,CVI_ISP_GetClutSaturationAttr,CVI_ISP_SetClutSaturationAttr,"ClutSaturationAttr");

	result = CVI_ISP_GetGammaCurveByType(ViPipe,&data.stGammaAttr,ISP_GAMMA_CURVE_DEFAULT);
	PRINT_RW_RESULT(result,"GammaAttr");

	memset(&data, 0, sizeof(data));
	CVI_ISP_GetPreSharpenAttr(ViPipe,&data.stPreSharpenAttr);
	data.stPreSharpenAttr.Enable = (data.stPreSharpenAttr.Enable == 1 ? 0 : 1);
	RPC_RW_TEST(ISP_PRESHARPEN_ATTR_S,stPreSharpenAttr,CVI_ISP_GetPreSharpenAttr,CVI_ISP_SetPreSharpenAttr,"PreSharpenAttr");

	memset(&data, 0, sizeof(data));
	CVI_ISP_GetRGBCACAttr(ViPipe,&data.stRGBCACAttr);
	data.stRGBCACAttr.Enable = (data.stRGBCACAttr.Enable == 1 ? 0 : 1);
	RPC_RW_TEST(ISP_RGBCAC_ATTR_S,stRGBCACAttr,CVI_ISP_GetRGBCACAttr,CVI_ISP_SetRGBCACAttr,"RGBCACAttr");

	memset(&data, 0, sizeof(data));
	CVI_ISP_GetCNRMotionNRAttr(ViPipe,&data.stCNRMotionNRAttr);
	data.stCNRMotionNRAttr.MotionCnrEnable = (data.stCNRMotionNRAttr.MotionCnrEnable == 1 ? 0 : 1);
	RPC_RW_TEST(ISP_CNR_MOTION_NR_ATTR_S,stCNRMotionNRAttr,CVI_ISP_GetCNRMotionNRAttr,CVI_ISP_SetCNRMotionNRAttr,"CNRMotionNRAttr");

	memset(&data, 0, sizeof(data));
	CVI_ISP_GetCCMSaturationAttr(ViPipe,&data.stCCMSaturationAttr);
	RPC_RW_TEST(ISP_CCM_SATURATION_ATTR_S,stCCMSaturationAttr,CVI_ISP_GetCCMSaturationAttr,CVI_ISP_SetCCMSaturationAttr,"CCMSaturationAttr");

	memset(&data, 0, sizeof(data));
	CVI_ISP_GetCAAttr(ViPipe,&data.stCAAttr);
	data.stCAAttr.Enable = (data.stCAAttr.Enable == 1 ? 0 : 1);
	RPC_RW_TEST(ISP_CA_ATTR_S,stCAAttr,CVI_ISP_GetCAAttr,CVI_ISP_SetCAAttr,"CAAttr");
#endif

}

static void other_test(VI_PIPE ViPipe)
{
	CVI_S32 ret = 0;

	ret = ret;

	CVI_U32 regAddr = 0xA057004,getValue;

	CVI_ISP_GetRegister(ViPipe,regAddr,&getValue);
	getValue += 5;
	CVI_ISP_SetRegister(ViPipe,regAddr, getValue);
	CVI_ISP_GetRegister(ViPipe,regAddr,&getValue);
	printf("CVI_ISP_GetRegister 0x%x= %d \n",regAddr,getValue);

	//CVI_ISP_GetVDTimeOut(VI_PIPE ViPipe, ISP_VD_TYPE_E enIspVDType, CVI_U32 u32MilliSec);

#ifdef ARCH_CV183X
	ret = CVI_ISP_GetBinStatus(ViPipe);
	printf("CVI_ISP_GetBinStatus ret =%d \n",ret);
#endif

	FILE *fp = NULL;

	fp = fopen("/mnt/data/dumpRegister2File.txt", "w");

	ret = CVI_ISP_DumpHwRegisterToFile(ViPipe, fp);

	fclose(fp);


	fp = fopen("/mnt/data/dumpFrameRawInfo2File.txt", "w");

	ret = CVI_ISP_DumpFrameRawInfoToFile(ViPipe, fp);

	fclose(fp);

	CVI_ISP_AEBracketingStart(ViPipe);
//	CVI_ISP_AEBracketingSetExpsoure(ViPipe, CVI_S16 leEvX10, CVI_S16 seEvX10);
	CVI_ISP_AEBracketingFinish(ViPipe);


	CVI_BOOL enable;
	CVI_U8 frequency;

	ret = CVI_ISP_GetAntiFlicker(ViPipe, &enable, &frequency);
	printf("GetAntiFlicker enable =%d frequency=%d \n",enable, frequency);

	ret = CVI_ISP_SetAntiFlicker(ViPipe, enable, frequency);
	printf("SetAntiFlicker return =%d  \n",ret);

	ret = CVI_ISP_SetWDRLEOnly(ViPipe,enable);
	printf("CVI_ISP_SetWDRLEOnly ret =%d \n",ret);
	CVI_U32 frameID;

	ret = CVI_ISP_GetFrameID(ViPipe, &frameID);
	printf("CVI_ISP_GetFrameID frameID=%d\n",frameID);


	char szPath[AE_PATH_BUF_SIZE] = {0};
	char szName[AE_PATH_BUF_SIZE] = {0};
	CVI_ISP_SetAELogPath("/mnt/");
	CVI_ISP_GetAELogPath(szPath, AE_PATH_BUF_SIZE);
	CVI_ISP_SetAELogName("AE.log");
	CVI_ISP_GetAELogName(szName, AE_PATH_BUF_SIZE);
	printf("Get AE log  path =[%s] name = [%s]\n", szPath, szName);
	CVI_FLOAT pFps;

	ret = CVI_ISP_QueryFps(ViPipe, &pFps);
	printf("CVI_ISP_QueryFps pFps=%f \n",pFps);

#if 1
	CVI_U32 aeLogSize;
	CVI_ISP_GetAELogBufSize(ViPipe,&aeLogSize);
	printf("CVI_ISP_GetAELogBufSize =%d \n",aeLogSize);

	static CVI_U8 buf[AE_LOG_BUF_SIZE] = {0};
	memset(buf, 0, AE_LOG_BUF_SIZE);
	ret = CVI_ISP_GetAELogBuf(ViPipe, buf, AE_LOG_BUF_SIZE);
	fp = fopen("/mnt/data/aelog.txt", "w");
	fwrite(buf, sizeof(CVI_U8), strlen((char *)buf), fp);
	fclose(fp);


	memset(buf, 0, AE_LOG_BUF_SIZE);

	ret = CVI_ISP_GetAWBSnapLogBuf(ViPipe, buf, AE_LOG_BUF_SIZE);

	fp = fopen("/mnt/data/awbsnaplog.txt", "w");

	fwrite(buf, sizeof(CVI_U8), strlen((char *)buf), fp);

	fclose(fp);


	memset(buf, 0, AE_LOG_BUF_SIZE);

	ret = CVI_ISP_GetAWBDbgBinBuf(ViPipe, buf, AE_LOG_BUF_SIZE);

	fp = fopen("/mnt/data/awbdbgbin.bin", "w");

	fwrite(buf, sizeof(CVI_U8), AE_LOG_BUF_SIZE, fp);

	fclose(fp);


	ret = CVI_ISP_GetAWBDbgBinSize();
	printf("CVI_ISP_GetAWBDbgBinSize ret =%d \n",ret);

//CVI_S32 CVI_ISP_SetAWBLogPath(const char *szPath);
//CVI_S32 CVI_ISP_SetAWBLogName(const char *szName);
//CVI_S32 CVI_ISP_SetFaceAwbInfo(VI_PIPE ViPipe, const CVI_ISP_FACE_DETECT_INFO *pstFaceInfo);
#endif
}

#include <stdlib.h>
#include <unistd.h>
#include "cvi_ispd.h"
#include "cvi_bin.h"

#define WEBSOCKET_PORT	5566

static void print_usage(void)
{
	printf("1. read and write test, ./rpc_test 0 vipipe\n");
	printf("\t ex: ./rpc_test 0 0\n");
	printf("2. run isp daemon.\n");
	printf("\t ex: ./prc_test 1\n");
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		print_usage();
		return -1;
	}

	if (atoi(argv[1]) == 0) {

		if (argc < 3) {
			print_usage();
			return -1;
		}

		if (atoi(argv[2]) >= 4) {
			print_usage();
			return -1;
		}

		rw_test(atoi(argv[2]));
		other_test(atoi(argv[2]));

	} else if(atoi(argv[1]) == 1) {

		isp_daemon_init(WEBSOCKET_PORT);

		while (1) {
			sleep(1);
		}
	}

	return 0;
}

#include "cvi_vi.h"
#include "cvi_vpss.h"

int i2cInit(uint8_t i2c_bus, uint8_t slave_addr)
{
	LOGOUT("%d,%d\n", i2c_bus, slave_addr);
	return 0;
}

void i2cExit(void)
{
	LOGOUT("\n");
}

int i2cRead(uint16_t addr, unsigned int addr_nbytes, uint8_t *buffer,
	    unsigned int data_nbytes)
{
	LOGOUT("%d,%d,%p,%d\n", addr, addr_nbytes, buffer, data_nbytes);
	return 0;
}

int i2cWrite(uint16_t addr, unsigned int addr_nbytes, uint8_t *buffer,
	     unsigned int data_nbytes)
{
	LOGOUT("%d,%d,%p,%d\n", addr, addr_nbytes, buffer, data_nbytes);
	return 0;
}

uint32_t regRead(uint32_t addr)
{
	LOGOUT("%d\n", addr);
	return 0x00;
}

void regWrite(uint32_t addr, uint32_t data)
{
	LOGOUT("%d,%d\n", addr, data);
}

void regWriteMask(uint32_t addr, uint32_t data, uint32_t mask)
{
	LOGOUT("%d,%d,%d\n", addr, data, mask);
}

void *CVI_SYS_Mmap(CVI_U64 u64PhyAddr, CVI_U32 u32Size)
{
	LOGOUT("%lld,%d\n", u64PhyAddr, u32Size);
	return NULL;
}

CVI_S32 CVI_SYS_Munmap(void *pVirAddr, CVI_U32 u32Size)
{
	LOGOUT("%p,%d\n", pVirAddr, u32Size);
	return CVI_SUCCESS;
}

CVI_S32 CVI_SYS_IonInvalidateCache(CVI_U64 u64PhyAddr, CVI_VOID *pVirAddr, CVI_U32 u32Len)
{
	CVI_S32 ret = CVI_SUCCESS;

	LOGOUT("%lld,%p,%d\n", u64PhyAddr, pVirAddr, u32Len);
	return ret;
}

CVI_S32 CVI_SYS_Bind(const MMF_CHN_S *pstSrcChn, const MMF_CHN_S *pstDestChn)
{
	CVI_S32 ret = CVI_SUCCESS;

	LOGOUT("%p,%p\n", pstSrcChn, pstDestChn);
	return ret;
}

CVI_S32 CVI_SYS_UnBind(const MMF_CHN_S *pstSrcChn, const MMF_CHN_S *pstDestChn)
{
	CVI_S32 ret = CVI_SUCCESS;

	LOGOUT("%p,%p\n", pstSrcChn, pstDestChn);
	return ret;
}


CVI_S32 CVI_VPSS_SetGrpProcAmp(VPSS_GRP VpssGrp, PROC_AMP_E type, CVI_S32 value)
{
	LOGOUT("%d,%d,%d\n", VpssGrp, type, value);
	return CVI_SUCCESS;
}

CVI_S32 CVI_VPSS_GetGrpProcAmp(VPSS_GRP VpssGrp, PROC_AMP_E type, CVI_S32 *value)
{
	LOGOUT("%d,%d,%p\n", VpssGrp, type, value);
	return CVI_SUCCESS;
}

CVI_S32 CVI_VPSS_GetChnLDCAttr(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VPSS_LDC_ATTR_S *pstLDCAttr)
{
	LOGOUT("%d,%d,%p\n", VpssGrp, VpssChn, pstLDCAttr);
	return CVI_SUCCESS;
}

CVI_S32 CVI_VPSS_SetChnLDCAttr(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, const VPSS_LDC_ATTR_S *pstLDCAttr)
{
	LOGOUT("%d,%d,%p\n", VpssGrp, VpssChn, pstLDCAttr);
	return CVI_SUCCESS;
}


CVI_S32 CVI_VI_SetChnLDCAttr(VI_PIPE ViPipe, VI_CHN ViChn, const VI_LDC_ATTR_S *pstLDCAttr)
{
	LOGOUT("%d,%d,%p\n", ViPipe, ViChn, pstLDCAttr);
	return CVI_SUCCESS;
}

CVI_S32 CVI_VI_GetChnLDCAttr(VI_PIPE ViPipe, VI_CHN ViChn, VI_LDC_ATTR_S *pstLDCAttr)
{
	LOGOUT("%d,%d,%p\n", ViPipe, ViChn, pstLDCAttr);
	return CVI_SUCCESS;
}

CVI_S32 CVI_VI_GetPipeFrame(VI_PIPE ViPipe, VIDEO_FRAME_INFO_S *pstVideoFrame, CVI_S32 s32MilliSec)
{
	LOGOUT("%d,%p,%d\n", ViPipe, pstVideoFrame, s32MilliSec);
	return CVI_SUCCESS;
}

CVI_S32 CVI_VI_ReleasePipeFrame(VI_PIPE ViPipe, const VIDEO_FRAME_INFO_S *pstVideoFrame)
{
	LOGOUT("%d,%p\n", ViPipe, pstVideoFrame);
	return CVI_SUCCESS;
}

CVI_S32 CVI_VI_GetChnFrame(VI_PIPE ViPipe, VI_CHN ViChn, VIDEO_FRAME_INFO_S *pstFrameInfo, CVI_S32 s32MilliSec)
{
	LOGOUT("%d,%d,%p,%d\n", ViPipe, ViChn, pstFrameInfo, s32MilliSec);
	return CVI_SUCCESS;
}

CVI_S32 CVI_VI_ReleaseChnFrame(VI_PIPE ViPipe, VI_CHN ViChn, const VIDEO_FRAME_INFO_S *pstFrameInfo)
{
	LOGOUT("%d,%d,%p\n", ViPipe, ViChn, pstFrameInfo);
	return CVI_SUCCESS;
}

CVI_S32 CVI_VI_GetChnCrop(VI_PIPE ViPipe, VI_CHN ViChn, VI_CROP_INFO_S  *pstCropInfo)
{
	LOGOUT("%d,%d,%p\n", ViPipe, ViChn, pstCropInfo);
	return CVI_SUCCESS;
}

CVI_S32 CVI_VI_SetPipeDumpAttr(VI_PIPE ViPipe, const VI_DUMP_ATTR_S *pstDumpAttr)
{
	LOGOUT("%d,%p\n", ViPipe, pstDumpAttr);
	return CVI_SUCCESS;
}

CVI_S32 CVI_BIN_LoadParamFromBin(enum CVI_BIN_SECTION_ID id, CVI_U8 *buf)
{
	CVI_S32 ret = CVI_SUCCESS;

	LOGOUT("%d,%p\n", id, buf);
	return ret;
}

CVI_S32 CVI_BIN_GetBinName(CVI_CHAR *binName)
{
	LOGOUT("%p\n", binName);
	return CVI_SUCCESS;
}

CVI_S32 CVI_BIN_SaveParamToBin(FILE *fp, CVI_BIN_EXTRA_S *extraInfo)
{
	CVI_S32 ret = CVI_SUCCESS;

	LOGOUT("%p,%p\n", fp, extraInfo);
	return ret;
}


