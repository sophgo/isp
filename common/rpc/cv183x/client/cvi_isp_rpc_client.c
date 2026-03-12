/*
 * client
 */

#include "cvi_isp.h"
#include "cvi_ae.h"
#include "cvi_awb.h"
#include "isp_rpc.h"

//#include "isp_debug.h"

//#pragma GCC diagnostic push
//#pragma GCC diagnostic ignored "-Wunused-variable"
//#pragma GCC diagnostic ignored "-Wunused-parameter"
//#pragma GCC diagnostic ignored "-Wreturn-type"
//#pragma GCC diagnostic ignored "-Wuninitialized"

#define LOGOUT(fmt, arg...) printf("%s,%d: " fmt, __func__, __LINE__, ##arg)
//#define LOGOUT(fmt, arg...) ISP_DEBUG(LOG_DEBUG, "%s,%d: " fmt, __func__, __LINE__, ##arg)

#define ABORT() ({                                           \
		LOGOUT("%s:%d, abort...\n", __FILE__, __LINE__);     \
		abort();})

#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif

static char *host = "127.0.0.1";

#define CREATE_CLNT() ({                                                \
		clnt = clnt_create (host, ISP_RPC, ISP_RPC_VERSION, "tcp");     \
		if (clnt == NULL) {                                             \
			clnt_pcreateerror (host);                                   \
			ABORT();                                                    \
		}})

#define DESTROY_CLNT() ({                                               \
		clnt_destroy (clnt);})                                          \

#define SET_ATTR(__TYPE, __NAME, __FUN) ({                              \
		CLIENT *clnt;                                                   \
		CVI_S32 ret = CVI_FAILURE;                                      \
		RPC_CVI_S32 *result;                                            \
		RPC_ISP_SET_ATTR_DATA_S stAttrData;                             \
		clnt = clnt_create (host, ISP_RPC, ISP_RPC_VERSION, "tcp");     \
		if (clnt == NULL) {                                             \
			clnt_pcreateerror (host);                                   \
			ABORT();                                                    \
		}                                                               \
		stAttrData.data.data_len = sizeof(__TYPE);                      \
		stAttrData.data.data_val = (uint8_t *) __NAME;                  \
		result = __FUN(ViPipe, stAttrData, clnt);                       \
		if (result == (RPC_CVI_S32 *) NULL) {                           \
			clnt_perror (clnt, "call failed");                          \
			LOGOUT("isp rpc set attr error!!!\n");                      \
		} else {                                                        \
			ret = *result;                                              \
		}                                                               \
		clnt_destroy (clnt);                                            \
		return ret;})

#define GET_ATTR(__TYPE, __NAME, __FUN) ({                              \
		CLIENT *clnt;                                                   \
		CVI_S32 ret = CVI_FAILURE;                                      \
		RPC_ISP_GET_ATTR_DATA_S *result;                                \
		__TYPE data;                                                    \
		memset(&data, 0, sizeof(__TYPE));                               \
		clnt = clnt_create (host, ISP_RPC, ISP_RPC_VERSION, "tcp");     \
		if (clnt == NULL) {                                             \
			clnt_pcreateerror (host);                                   \
			ABORT();                                                    \
		}                                                               \
		result = __FUN(ViPipe, clnt);                                   \
		if (result == (RPC_ISP_GET_ATTR_DATA_S*) NULL) {                \
			clnt_perror(clnt, "call failed");                           \
			LOGOUT("isp rpc get attr error!!!\n");                      \
			ret = CVI_FAILURE;                                          \
		} else {                                                        \
			ret = result->ret;                                          \
			if (sizeof(__TYPE) == result->data.data_len) {              \
				memcpy(__NAME, result->data.data_val, sizeof(__TYPE));  \
			} else {                                                    \
				LOGOUT("isp rpc get attr error!!!\n");                  \
				ret = CVI_FAILURE;                                      \
			}                                                           \
		}                                                               \
		clnt_destroy (clnt);                                            \
		return ret;})


CVI_S32 CVI_ISP_SetPubAttr(VI_PIPE ViPipe, const ISP_PUB_ATTR_S *pstPubAttr)
{
	SET_ATTR(ISP_PUB_ATTR_S, pstPubAttr, rpc_cvi_isp_setpubattr_1);
}

CVI_S32 CVI_ISP_GetPubAttr(VI_PIPE ViPipe, ISP_PUB_ATTR_S *pstPubAttr)
{
	GET_ATTR(ISP_PUB_ATTR_S, pstPubAttr, rpc_cvi_isp_getpubattr_1);
}

CVI_S32 CVI_ISP_SetModParam(const ISP_MOD_PARAM_S *pstModParam)
{
	CVI_S32 ret = CVI_FAILURE;
	RPC_CVI_S32 *result;
	RPC_ISP_SET_ATTR_DATA_S stAttrData;
	CLIENT *clnt;

	CREATE_CLNT();

	stAttrData.data.data_len = sizeof(ISP_MOD_PARAM_S);
	stAttrData.data.data_val = (uint8_t *) pstModParam;

	result = rpc_cvi_isp_setmodparam_1(stAttrData, clnt);

	if(result == (RPC_CVI_S32 *) NULL) {
		clnt_perror (clnt, "call failed");
		LOGOUT("isp rpc set attr error!!!");
	} else {
		ret = *result;
	}

	DESTROY_CLNT();

	return ret;
}

CVI_S32 CVI_ISP_GetModParam(ISP_MOD_PARAM_S *pstModParam)
{
	CVI_S32 ret = CVI_FAILURE;
	RPC_ISP_GET_ATTR_DATA_S *result;
	CLIENT *clnt;

	CREATE_CLNT();

	result = rpc_cvi_isp_getmodparam_1(clnt);
	if (result == (RPC_ISP_GET_ATTR_DATA_S *) NULL) {
		clnt_perror (clnt, "call failed");
		ret = CVI_FAILURE;
	} else {
		ret = result->ret;
		memcpy(pstModParam, result->data.data_val, sizeof(ISP_MOD_PARAM_S));
	}

	DESTROY_CLNT();

	return ret;
}

CVI_S32 CVI_ISP_SetCtrlParam(VI_PIPE ViPipe,
				const ISP_CTRL_PARAM_S *pstIspCtrlParam)
{
	SET_ATTR(ISP_CTRL_PARAM_S, pstIspCtrlParam, rpc_cvi_isp_setctrlparam_1);
}

CVI_S32 CVI_ISP_GetCtrlParam(VI_PIPE ViPipe, ISP_CTRL_PARAM_S *pstIspCtrlParam)
{
	GET_ATTR(ISP_CTRL_PARAM_S, pstIspCtrlParam, rpc_cvi_isp_getctrlparam_1);
}

CVI_S32 CVI_ISP_SetFMWState(VI_PIPE ViPipe, const ISP_FMW_STATE_E enState)
{
	CVI_S32 ret;
	RPC_CVI_S32  *result;
	CLIENT *clnt;

	CREATE_CLNT();

	result = rpc_cvi_isp_setfmwstate_1(ViPipe, enState, clnt);
	if (result == (RPC_CVI_S32 *) NULL) {
		clnt_perror (clnt, "call failed");
		ret = CVI_FAILURE;
	} else {
		ret = *result;
	}

	DESTROY_CLNT();

	return ret;
}

CVI_S32 CVI_ISP_GetFMWState(VI_PIPE ViPipe, ISP_FMW_STATE_E *penState)
{
	CVI_S32 ret = CVI_SUCCESS;
	uint32_t  *result;
	CLIENT *clnt;

	CREATE_CLNT();

	result = rpc_cvi_isp_getfmwstate_1(ViPipe, clnt);
	if (result == (uint32_t *) NULL) {
		clnt_perror (clnt, "call failed");
		ret = CVI_FAILURE;
	} else {
		*penState = *result;
	}

	DESTROY_CLNT();

	return ret;
}

CVI_S32 CVI_ISP_SetModuleControl(VI_PIPE ViPipe, const ISP_MODULE_CTRL_U *punModCtrl)
{
	SET_ATTR(ISP_MODULE_CTRL_U, punModCtrl, rpc_cvi_isp_setmodulecontrol_1);
}

CVI_S32 CVI_ISP_GetModuleControl(VI_PIPE ViPipe, ISP_MODULE_CTRL_U *punModCtrl)
{
	GET_ATTR(ISP_MODULE_CTRL_U, punModCtrl, rpc_cvi_isp_getmodulecontrol_1);
}

CVI_S32 CVI_ISP_SetRegister(VI_PIPE ViPipe, CVI_U32 u32Addr, CVI_U32 u32Value)
{
	CVI_S32 ret;
	RPC_CVI_S32  *result;
	CLIENT *clnt;

	CREATE_CLNT();

	result = rpc_cvi_isp_setregister_1(ViPipe, u32Addr, u32Value, clnt);
	if (result == (RPC_CVI_S32 *) NULL) {
		clnt_perror (clnt, "call failed");
		ret = CVI_FAILURE;
	} else {
		ret = *result;
	}

	DESTROY_CLNT();

	return ret;
}

CVI_S32 CVI_ISP_GetRegister(VI_PIPE ViPipe, CVI_U32 u32Addr, CVI_U32 *pu32Value)
{
	CVI_S32 ret = CVI_SUCCESS;
	uint32_t  *result;
	CLIENT *clnt;

	CREATE_CLNT();

	result = rpc_cvi_isp_getregister_1(ViPipe, u32Addr, clnt);
	if (result == (uint32_t *) NULL) {
		clnt_perror (clnt, "call failed");
		ret = CVI_FAILURE;
	} else {
		*pu32Value = *result;
	}

	DESTROY_CLNT();

	return ret;
}

CVI_S32 CVI_ISP_SetColorToneAttr(VI_PIPE ViPipe,
				const ISP_COLOR_TONE_ATTR_S *pstWBGAttr)
{
	SET_ATTR(ISP_COLOR_TONE_ATTR_S, pstWBGAttr, rpc_cvi_isp_setcolortoneattr_1);
}

CVI_S32 CVI_ISP_GetColorToneAttr(VI_PIPE ViPipe, ISP_COLOR_TONE_ATTR_S *pstWBGAttr)
{
	GET_ATTR(ISP_COLOR_TONE_ATTR_S, pstWBGAttr, rpc_cvi_isp_getcolortoneattr_1);
}

CVI_S32 CVI_ISP_SetDehazeAttr(VI_PIPE ViPipe,
				const ISP_DEHAZE_ATTR_S *pstDehazeAttr)
{
	SET_ATTR(ISP_DEHAZE_ATTR_S, pstDehazeAttr, rpc_cvi_isp_setdehazeattr_1);
}

CVI_S32 CVI_ISP_GetDehazeAttr(VI_PIPE ViPipe, ISP_DEHAZE_ATTR_S *pstDehazeAttr)
{
	GET_ATTR(ISP_DEHAZE_ATTR_S, pstDehazeAttr, rpc_cvi_isp_getdehazeattr_1);
}

CVI_S32 CVI_ISP_SetGammaAttr(VI_PIPE ViPipe, const ISP_GAMMA_ATTR_S *pstGammaAttr)
{
	SET_ATTR(ISP_GAMMA_ATTR_S, pstGammaAttr, rpc_cvi_isp_setgammaattr_1);
}

CVI_S32 CVI_ISP_GetGammaAttr(VI_PIPE ViPipe, ISP_GAMMA_ATTR_S *pstGammaAttr)
{
	GET_ATTR(ISP_GAMMA_ATTR_S, pstGammaAttr, rpc_cvi_isp_getgammaattr_1);
}

CVI_S32 CVI_ISP_SetGammaCurveType(VI_PIPE ViPipe, const ISP_GAMMA_CURVE_TYPE_E curveType)
{
	CVI_S32 ret;
	RPC_CVI_S32  *result;
	CLIENT *clnt;

	CREATE_CLNT();

	result = rpc_cvi_isp_setgammacurvetype_1(ViPipe, curveType, clnt);
	if (result == (RPC_CVI_S32 *) NULL) {
		clnt_perror (clnt, "call failed");
		ret = CVI_FAILURE;
	} else {
		ret = *result;
	}

	DESTROY_CLNT();

	return ret;
}

CVI_S32 CVI_ISP_SetAutoGammaAttr(VI_PIPE ViPipe, const ISP_AUTO_GAMMA_ATTR_S *pstGammaAttr)
{
	SET_ATTR(ISP_AUTO_GAMMA_ATTR_S, pstGammaAttr, rpc_cvi_isp_setautogammaattr_1);
}

CVI_S32 CVI_ISP_GetAutoGammaAttr(VI_PIPE ViPipe, ISP_AUTO_GAMMA_ATTR_S *pstGammaAttr)
{
	GET_ATTR(ISP_AUTO_GAMMA_ATTR_S, pstGammaAttr, rpc_cvi_isp_getautogammaattr_1);
}

CVI_S32 CVI_ISP_SetStatisticsConfig(VI_PIPE ViPipe, const ISP_STATISTICS_CFG_S *pstStatCfg)
{
	SET_ATTR(ISP_STATISTICS_CFG_S, pstStatCfg, rpc_cvi_isp_setstatisticsconfig_1);
}

CVI_S32 CVI_ISP_GetStatisticsConfig(VI_PIPE ViPipe, ISP_STATISTICS_CFG_S *pstStatCfg)
{
	GET_ATTR(ISP_STATISTICS_CFG_S, pstStatCfg, rpc_cvi_isp_getstatisticsconfig_1);
}

CVI_S32 CVI_ISP_SetNRAttr(VI_PIPE ViPipe, const ISP_NR_ATTR_S *pstNRAttr)
{
	SET_ATTR(ISP_NR_ATTR_S, pstNRAttr, rpc_cvi_isp_setnrattr_1);
}

CVI_S32 CVI_ISP_GetNRAttr(VI_PIPE ViPipe, ISP_NR_ATTR_S *pstNRAttr)
{
	GET_ATTR(ISP_NR_ATTR_S, pstNRAttr, rpc_cvi_isp_getnrattr_1);
}

CVI_S32 CVI_ISP_SetNRFilterAttr(VI_PIPE ViPipe, const ISP_NR_FILTER_ATTR_S *pstNRFilterAttr)
{
	SET_ATTR(ISP_NR_FILTER_ATTR_S, pstNRFilterAttr, rpc_cvi_isp_setnrfilterattr_1);
}

CVI_S32 CVI_ISP_GetNRFilterAttr(VI_PIPE ViPipe, ISP_NR_FILTER_ATTR_S *pstNRFilterAttr)
{
	GET_ATTR(ISP_NR_FILTER_ATTR_S, pstNRFilterAttr, rpc_cvi_isp_getnrfilterattr_1);
}

CVI_S32 CVI_ISP_SetRLSCAttr(VI_PIPE ViPipe, const ISP_RLSC_ATTR_S *pstRLSCAttr)
{
	SET_ATTR(ISP_RLSC_ATTR_S, pstRLSCAttr, rpc_cvi_isp_setrlscattr_1);
}

CVI_S32 CVI_ISP_GetRLSCAttr(VI_PIPE ViPipe, ISP_RLSC_ATTR_S *pstRLSCAttr)
{
	GET_ATTR(ISP_RLSC_ATTR_S, pstRLSCAttr, rpc_cvi_isp_getrlscattr_1);
}

CVI_S32 CVI_ISP_SetYNRAttr(VI_PIPE ViPipe, const ISP_YNR_ATTR_S *pstYNRAttr)
{
	SET_ATTR(ISP_YNR_ATTR_S, pstYNRAttr, rpc_cvi_isp_setynrattr_1);
}

CVI_S32 CVI_ISP_GetYNRAttr(VI_PIPE ViPipe, ISP_YNR_ATTR_S *pstYNRAttr)
{
	GET_ATTR(ISP_YNR_ATTR_S, pstYNRAttr, rpc_cvi_isp_getynrattr_1);
}

CVI_S32 CVI_ISP_SetYNRMotionNRAttr(VI_PIPE ViPipe, const ISP_YNR_MOTION_NR_ATTR_S *pstYNRMotionNRAttr)
{
	SET_ATTR(ISP_YNR_MOTION_NR_ATTR_S, pstYNRMotionNRAttr, rpc_cvi_isp_setynrmotionnrattr_1);
}

CVI_S32 CVI_ISP_GetYNRMotionNRAttr(VI_PIPE ViPipe, ISP_YNR_MOTION_NR_ATTR_S *pstYNRMotionNRAttr)
{
	GET_ATTR(ISP_YNR_MOTION_NR_ATTR_S, pstYNRMotionNRAttr, rpc_cvi_isp_getynrmotionnrattr_1);
}

CVI_S32 CVI_ISP_SetYNRFilterAttr(VI_PIPE ViPipe, const ISP_YNR_FILTER_ATTR_S *pstYNRFilterAttr)
{
	SET_ATTR(ISP_YNR_FILTER_ATTR_S, pstYNRFilterAttr, rpc_cvi_isp_setynrfilterattr_1);
}

CVI_S32 CVI_ISP_GetYNRFilterAttr(VI_PIPE ViPipe, ISP_YNR_FILTER_ATTR_S *pstYNRFilterAttr)
{
	GET_ATTR(ISP_YNR_FILTER_ATTR_S, pstYNRFilterAttr, rpc_cvi_isp_getynrfilterattr_1);
}

CVI_S32 CVI_ISP_SetCNRAttr(VI_PIPE ViPipe, const ISP_CNR_ATTR_S *pstCNRAttr)
{
	SET_ATTR(ISP_CNR_ATTR_S, pstCNRAttr, rpc_cvi_isp_setcnrattr_1);
}

CVI_S32 CVI_ISP_GetCNRAttr(VI_PIPE ViPipe, ISP_CNR_ATTR_S *pstCNRAttr)
{
	GET_ATTR(ISP_CNR_ATTR_S, pstCNRAttr, rpc_cvi_isp_getcnrattr_1);
}

CVI_S32 CVI_ISP_SetCACAttr(VI_PIPE ViPipe, const ISP_CAC_ATTR_S *pstCACAttr)
{
	SET_ATTR(ISP_CAC_ATTR_S, pstCACAttr, rpc_cvi_isp_setcacattr_1);
}

CVI_S32 CVI_ISP_GetCACAttr(VI_PIPE ViPipe, ISP_CAC_ATTR_S *pstCACAttr)
{
	GET_ATTR(ISP_CAC_ATTR_S, pstCACAttr, rpc_cvi_isp_getcacattr_1);
}

CVI_S32 CVI_ISP_SetDCIAttr(VI_PIPE ViPipe, const ISP_DCI_ATTR_S *pstDCIAttr)
{
	SET_ATTR(ISP_DCI_ATTR_S, pstDCIAttr, rpc_cvi_isp_setdciattr_1);
}

CVI_S32 CVI_ISP_GetDCIAttr(VI_PIPE ViPipe, ISP_DCI_ATTR_S *pstDCIAttr)
{
	GET_ATTR(ISP_DCI_ATTR_S, pstDCIAttr, rpc_cvi_isp_getdciattr_1);
}

CVI_S32 CVI_ISP_SetMeshShadingAttr(VI_PIPE ViPipe, const ISP_MESH_SHADING_ATTR_S *pstMeshShadingAttr)
{
	SET_ATTR(ISP_MESH_SHADING_ATTR_S, pstMeshShadingAttr, rpc_cvi_isp_setmeshshadingattr_1);
}

CVI_S32 CVI_ISP_GetMeshShadingAttr(VI_PIPE ViPipe, ISP_MESH_SHADING_ATTR_S *pstMeshShadingAttr)
{
	GET_ATTR(ISP_MESH_SHADING_ATTR_S, pstMeshShadingAttr, rpc_cvi_isp_getmeshshadingattr_1);
}

CVI_S32 CVI_ISP_SetMeshShadingGainLutAttr(VI_PIPE ViPipe,
				const ISP_MESH_SHADING_GAIN_LUT_ATTR_S *pstMeshShadingGainLutAttr)
{
	SET_ATTR(ISP_MESH_SHADING_GAIN_LUT_ATTR_S, pstMeshShadingGainLutAttr, rpc_cvi_isp_setmeshshadinggainlutattr_1);
}

CVI_S32 CVI_ISP_GetMeshShadingGainLutAttr(VI_PIPE ViPipe,
				ISP_MESH_SHADING_GAIN_LUT_ATTR_S *pstMeshShadingGainLutAttr)
{
	GET_ATTR(ISP_MESH_SHADING_GAIN_LUT_ATTR_S, pstMeshShadingGainLutAttr, rpc_cvi_isp_getmeshshadinggainlutattr_1);
}

CVI_S32 CVI_ISP_SetRadialShadingAttr(VI_PIPE ViPipe, const ISP_RADIAL_SHADING_ATTR_S *pstRadialShadingAttr)
{
	SET_ATTR(ISP_RADIAL_SHADING_ATTR_S, pstRadialShadingAttr, rpc_cvi_isp_setradialshadingattr_1);
}

CVI_S32 CVI_ISP_GetRadialShadingAttr(VI_PIPE ViPipe, ISP_RADIAL_SHADING_ATTR_S *pstRadialShadingAttr)
{
	GET_ATTR(ISP_RADIAL_SHADING_ATTR_S, pstRadialShadingAttr, rpc_cvi_isp_getradialshadingattr_1);
}

CVI_S32 CVI_ISP_SetRadialShadingGainLutAttr(VI_PIPE ViPipe,
				const ISP_RADIAL_SHADING_GAIN_LUT_ATTR_S *pstRadialShadingGainLutAttr)
{
	SET_ATTR(ISP_RADIAL_SHADING_GAIN_LUT_ATTR_S, pstRadialShadingGainLutAttr, rpc_cvi_isp_setradialshadinggainlutattr_1);
}

CVI_S32 CVI_ISP_GetRadialShadingGainLutAttr(VI_PIPE ViPipe,
				ISP_RADIAL_SHADING_GAIN_LUT_ATTR_S *pstRadialShadingGainLutAttr)
{
	GET_ATTR(ISP_RADIAL_SHADING_GAIN_LUT_ATTR_S, pstRadialShadingGainLutAttr, rpc_cvi_isp_getradialshadinggainlutattr_1);
}

CVI_S32 CVI_ISP_SetTNRAttr(VI_PIPE ViPipe, const ISP_TNR_ATTR_S *pstTNRAttr)
{
	SET_ATTR(ISP_TNR_ATTR_S, pstTNRAttr, rpc_cvi_isp_settnrattr_1);
}

CVI_S32 CVI_ISP_GetTNRAttr(VI_PIPE ViPipe, ISP_TNR_ATTR_S *pstTNRAttr)
{
	GET_ATTR(ISP_TNR_ATTR_S, pstTNRAttr, rpc_cvi_isp_gettnrattr_1);
}

CVI_S32 CVI_ISP_SetTNRNoiseModelAttr(VI_PIPE ViPipe, const ISP_TNR_NOISE_MODEL_ATTR_S *pstTNRNoiseModelAttr)
{
	SET_ATTR(ISP_TNR_NOISE_MODEL_ATTR_S, pstTNRNoiseModelAttr, rpc_cvi_isp_settnrnoisemodelattr_1);
}

CVI_S32 CVI_ISP_GetTNRNoiseModelAttr(VI_PIPE ViPipe, ISP_TNR_NOISE_MODEL_ATTR_S *pstTNRNoiseModelAttr)
{
	GET_ATTR(ISP_TNR_NOISE_MODEL_ATTR_S, pstTNRNoiseModelAttr, rpc_cvi_isp_gettnrnoisemodelattr_1);
}

CVI_S32 CVI_ISP_SetTNRLumaMotionAttr(VI_PIPE ViPipe, const ISP_TNR_LUMA_MOTION_ATTR_S *pstTNRLumaMotionAttr)
{
	SET_ATTR(ISP_TNR_LUMA_MOTION_ATTR_S, pstTNRLumaMotionAttr, rpc_cvi_isp_settnrlumamotionattr_1);
}

CVI_S32 CVI_ISP_GetTNRLumaMotionAttr(VI_PIPE ViPipe, ISP_TNR_LUMA_MOTION_ATTR_S *pstTNRLumaMotionAttr)
{
	GET_ATTR(ISP_TNR_LUMA_MOTION_ATTR_S, pstTNRLumaMotionAttr, rpc_cvi_isp_gettnrlumamotionattr_1);
}

CVI_S32 CVI_ISP_SetTNRGhostAttr(VI_PIPE ViPipe, const ISP_TNR_GHOST_ATTR_S *pstTNRGhostAttr)
{
	SET_ATTR(ISP_TNR_GHOST_ATTR_S, pstTNRGhostAttr, rpc_cvi_isp_settnrghostattr_1);
}

CVI_S32 CVI_ISP_GetTNRGhostAttr(VI_PIPE ViPipe, ISP_TNR_GHOST_ATTR_S *pstTNRGhostAttr)
{
	GET_ATTR(ISP_TNR_GHOST_ATTR_S, pstTNRGhostAttr, rpc_cvi_isp_gettnrghostattr_1);
}

CVI_S32 CVI_ISP_SetTNRMtPrtAttr(VI_PIPE ViPipe, const ISP_TNR_MT_PRT_ATTR_S *pstTNRMtPrtAttr)
{
	SET_ATTR(ISP_TNR_MT_PRT_ATTR_S, pstTNRMtPrtAttr, rpc_cvi_isp_settnrmtprtattr_1);
}

CVI_S32 CVI_ISP_GetTNRMtPrtAttr(VI_PIPE ViPipe, ISP_TNR_MT_PRT_ATTR_S *pstTNRMtPrtAttr)
{
	GET_ATTR(ISP_TNR_MT_PRT_ATTR_S, pstTNRMtPrtAttr, rpc_cvi_isp_gettnrmtprtattr_1);
}

CVI_S32 CVI_ISP_SetClutAttr(VI_PIPE ViPipe, const ISP_CLUT_ATTR_S *pstClutAttr)
{
	SET_ATTR(ISP_CLUT_ATTR_S, pstClutAttr, rpc_cvi_isp_setclutattr_1);
}

CVI_S32 CVI_ISP_GetClutAttr(VI_PIPE ViPipe, ISP_CLUT_ATTR_S *pstClutAttr)
{
	GET_ATTR(ISP_CLUT_ATTR_S, pstClutAttr, rpc_cvi_isp_getclutattr_1);
}

#ifdef ARCH_CV183X

CVI_S32 CVI_ISP_SetClutCoeff(VI_PIPE ViPipe, const ISP_CLUT_LUT_S *pstClutLUT)
{
	SET_ATTR(ISP_CLUT_LUT_S, pstClutLUT, rpc_cvi_isp_setclutcoeff_1);
}

CVI_S32 CVI_ISP_GetClutCoeff(VI_PIPE ViPipe, ISP_CLUT_LUT_S *pstClutLUT)
{
	GET_ATTR(ISP_CLUT_LUT_S, pstClutLUT, rpc_cvi_isp_getclutcoeff_1);
}
#endif
CVI_S32 CVI_ISP_SetBlackLevelAttr(VI_PIPE ViPipe, const ISP_BLACK_LEVEL_ATTR_S *pstBlackLevelAttr)
{
	SET_ATTR(ISP_BLACK_LEVEL_ATTR_S, pstBlackLevelAttr, rpc_cvi_isp_setblacklevelattr_1);
}

CVI_S32 CVI_ISP_GetBlackLevelAttr(VI_PIPE ViPipe, ISP_BLACK_LEVEL_ATTR_S *pstBlackLevelAttr)
{
	GET_ATTR(ISP_BLACK_LEVEL_ATTR_S, pstBlackLevelAttr, rpc_cvi_isp_getblacklevelattr_1);
}

CVI_S32 CVI_ISP_SetDemosaicAttr(VI_PIPE ViPipe, const ISP_DEMOSAIC_ATTR_S *pstDemosaicAttr)
{
	SET_ATTR(ISP_DEMOSAIC_ATTR_S, pstDemosaicAttr, rpc_cvi_isp_setdemosaicattr_1);
}

CVI_S32 CVI_ISP_GetDemosaicAttr(VI_PIPE ViPipe, ISP_DEMOSAIC_ATTR_S *pstDemosaicAttr)
{
	GET_ATTR(ISP_DEMOSAIC_ATTR_S, pstDemosaicAttr, rpc_cvi_isp_getdemosaicattr_1);
}

CVI_S32 CVI_ISP_SetDemosaicDemoireAttr(VI_PIPE ViPipe, const ISP_DEMOSAIC_DEMOIRE_ATTR_S *pstDemosaicDemoireAttr)
{
	SET_ATTR(ISP_DEMOSAIC_DEMOIRE_ATTR_S, pstDemosaicDemoireAttr, rpc_cvi_isp_setdemosaicdemoireattr_1);
}

CVI_S32 CVI_ISP_GetDemosaicDemoireAttr(VI_PIPE ViPipe, ISP_DEMOSAIC_DEMOIRE_ATTR_S *pstDemosaicDemoireAttr)
{
	GET_ATTR(ISP_DEMOSAIC_DEMOIRE_ATTR_S, pstDemosaicDemoireAttr, rpc_cvi_isp_getdemosaicdemoireattr_1);
}

CVI_S32 CVI_ISP_SetDemosaicFilterAttr(VI_PIPE ViPipe, const ISP_DEMOSAIC_FILTER_ATTR_S *pstDemosaicFilterAttr)
{
	SET_ATTR(ISP_DEMOSAIC_FILTER_ATTR_S, pstDemosaicFilterAttr, rpc_cvi_isp_setdemosaicfilterattr_1);
}

CVI_S32 CVI_ISP_GetDemosaicFilterAttr(VI_PIPE ViPipe, ISP_DEMOSAIC_FILTER_ATTR_S *pstDemosaicFilterAttr)
{
	GET_ATTR(ISP_DEMOSAIC_FILTER_ATTR_S, pstDemosaicFilterAttr, rpc_cvi_isp_getdemosaicfilterattr_1);
}

#ifdef ARCH_CV183X
CVI_S32 CVI_ISP_SetDemosaicEEAttr(VI_PIPE ViPipe, const ISP_DEMOSAIC_EE_ATTR_S *pstDemosaicEEAttr)
{
	SET_ATTR(ISP_DEMOSAIC_EE_ATTR_S, pstDemosaicEEAttr, rpc_cvi_isp_setdemosaiceeattr_1);
}

CVI_S32 CVI_ISP_GetDemosaicEEAttr(VI_PIPE ViPipe, ISP_DEMOSAIC_EE_ATTR_S *pstDemosaicEEAttr)
{
	GET_ATTR(ISP_DEMOSAIC_EE_ATTR_S, pstDemosaicEEAttr, rpc_cvi_isp_getdemosaiceeattr_1);
}
#endif
CVI_S32 CVI_ISP_SetSaturationAttr(VI_PIPE ViPipe, const ISP_SATURATION_ATTR_S *pstSaturationAttr)
{
	SET_ATTR(ISP_SATURATION_ATTR_S, pstSaturationAttr, rpc_cvi_isp_setsaturationattr_1);
}

CVI_S32 CVI_ISP_GetSaturationAttr(VI_PIPE ViPipe, ISP_SATURATION_ATTR_S *pstSaturationAttr)
{
	GET_ATTR(ISP_SATURATION_ATTR_S, pstSaturationAttr, rpc_cvi_isp_getsaturationattr_1);
}

CVI_S32 CVI_ISP_SetCCMAttr(VI_PIPE ViPipe, const ISP_CCM_ATTR_S *pstCCMAttr)
{
	SET_ATTR(ISP_CCM_ATTR_S, pstCCMAttr, rpc_cvi_isp_setccmattr_1);
}

CVI_S32 CVI_ISP_GetCCMAttr(VI_PIPE ViPipe, ISP_CCM_ATTR_S *pstCCMAttr)
{
	GET_ATTR(ISP_CCM_ATTR_S, pstCCMAttr, rpc_cvi_isp_getccmattr_1);
}

#ifdef ARCH_CV183X
CVI_S32 CVI_ISP_SetHSVAttr(VI_PIPE ViPipe, const ISP_HSV_ATTR_S *pstHSVAttr)
{
	SET_ATTR(ISP_HSV_ATTR_S, pstHSVAttr, rpc_cvi_isp_sethsvattr_1);
}

CVI_S32 CVI_ISP_GetHSVAttr(VI_PIPE ViPipe, ISP_HSV_ATTR_S *pstHSVAttr)
{
	GET_ATTR(ISP_HSV_ATTR_S, pstHSVAttr, rpc_cvi_isp_gethsvattr_1);
}
#endif
CVI_S32 CVI_ISP_SetDPDynamicAttr(VI_PIPE ViPipe, const ISP_DP_DYNAMIC_ATTR_S *pstDPCDynamicAttr)
{
	SET_ATTR(ISP_DP_DYNAMIC_ATTR_S, pstDPCDynamicAttr, rpc_cvi_isp_setdpdynamicattr_1);
}

CVI_S32 CVI_ISP_GetDPDynamicAttr(VI_PIPE ViPipe, ISP_DP_DYNAMIC_ATTR_S *pstDPCDynamicAttr)
{
	GET_ATTR(ISP_DP_DYNAMIC_ATTR_S, pstDPCDynamicAttr, rpc_cvi_isp_getdpdynamicattr_1);
}

CVI_S32 CVI_ISP_SetDPStaticAttr(VI_PIPE ViPipe, const ISP_DP_STATIC_ATTR_S *pstDPStaticAttr)
{
	SET_ATTR(ISP_DP_STATIC_ATTR_S, pstDPStaticAttr, rpc_cvi_isp_setdpstaticattr_1);
}

CVI_S32 CVI_ISP_GetDPStaticAttr(VI_PIPE ViPipe, ISP_DP_STATIC_ATTR_S *pstDPStaticAttr)
{
	GET_ATTR(ISP_DP_STATIC_ATTR_S, pstDPStaticAttr, rpc_cvi_isp_getdpstaticattr_1);
}

CVI_S32 CVI_ISP_SetDPCalibrate(VI_PIPE ViPipe, const ISP_DP_CALIB_ATTR_S *pstDPCalibAttr)
{
	SET_ATTR(ISP_DP_CALIB_ATTR_S, pstDPCalibAttr, rpc_cvi_isp_setdpcalibrate_1);
}

CVI_S32 CVI_ISP_GetDPCalibrate(VI_PIPE ViPipe, ISP_DP_CALIB_ATTR_S *pstDPCalibAttr)
{
	GET_ATTR(ISP_DP_CALIB_ATTR_S, pstDPCalibAttr, rpc_cvi_isp_getdpcalibrate_1);
}

CVI_S32 CVI_ISP_SetCrosstalkAttr(VI_PIPE ViPipe, const ISP_CROSSTALK_ATTR_S *pstCrosstalkAttr)
{
	SET_ATTR(ISP_CROSSTALK_ATTR_S, pstCrosstalkAttr, rpc_cvi_isp_setcrosstalkattr_1);
}

CVI_S32 CVI_ISP_GetCrosstalkAttr(VI_PIPE ViPipe, ISP_CROSSTALK_ATTR_S *pstCrosstalkAttr)
{
	GET_ATTR(ISP_CROSSTALK_ATTR_S, pstCrosstalkAttr, rpc_cvi_isp_getcrosstalkattr_1);
}

CVI_S32 CVI_ISP_GetAEStatistics(VI_PIPE ViPipe, ISP_AE_STATISTICS_S *pstAeStat)
{
	GET_ATTR(ISP_AE_STATISTICS_S, pstAeStat, rpc_cvi_isp_getaestatistics_1);
}

CVI_S32 CVI_ISP_GetWBStatistics(VI_PIPE ViPipe, ISP_WB_STATISTICS_S *pstWBStat)
{
	GET_ATTR(ISP_WB_STATISTICS_S, pstWBStat, rpc_cvi_isp_getwbstatistics_1);
}

CVI_S32 CVI_ISP_GetFocusStatistics(VI_PIPE ViPipe, ISP_AF_STATISTICS_S *pstAfStat)
{
	GET_ATTR(ISP_AF_STATISTICS_S, pstAfStat, rpc_cvi_isp_getfocusstatistics_1);
}

CVI_S32 CVI_ISP_GetLightboxGain(VI_PIPE ViPipe, ISP_AWB_LightBox_Gain_S *pstAWBLightBoxGain)
{
	GET_ATTR(ISP_AWB_LightBox_Gain_S, pstAWBLightBoxGain, rpc_cvi_isp_getlightboxgain_1);
}

CVI_S32 CVI_ISP_SetFSWDRAttr(VI_PIPE ViPipe, const ISP_FSWDR_ATTR_S *pstFSWDRAttr)
{
	SET_ATTR(ISP_FSWDR_ATTR_S, pstFSWDRAttr, rpc_cvi_isp_setfswdrattr_1);
}

CVI_S32 CVI_ISP_GetFSWDRAttr(VI_PIPE ViPipe, ISP_FSWDR_ATTR_S *pstFSWDRAttr)
{
	GET_ATTR(ISP_FSWDR_ATTR_S, pstFSWDRAttr, rpc_cvi_isp_getfswdrattr_1);
}

CVI_S32 CVI_ISP_SetDRCAttr(VI_PIPE ViPipe, const ISP_DRC_ATTR_S *pstDRCAttr)
{
	SET_ATTR(ISP_DRC_ATTR_S, pstDRCAttr, rpc_cvi_isp_setdrcattr_1);
}

CVI_S32 CVI_ISP_GetDRCAttr(VI_PIPE ViPipe, ISP_DRC_ATTR_S *pstDRCAttr)
{
	GET_ATTR(ISP_DRC_ATTR_S, pstDRCAttr, rpc_cvi_isp_getdrcattr_1);
}

CVI_S32 CVI_ISP_SetSharpenAttr(VI_PIPE ViPipe, const ISP_SHARPEN_ATTR_S *pstSharpenAttr)
{
	SET_ATTR(ISP_SHARPEN_ATTR_S, pstSharpenAttr, rpc_cvi_isp_setsharpenattr_1);
}

CVI_S32 CVI_ISP_GetSharpenAttr(VI_PIPE ViPipe, ISP_SHARPEN_ATTR_S *pstSharpenAttr)
{
	GET_ATTR(ISP_SHARPEN_ATTR_S, pstSharpenAttr, rpc_cvi_isp_getsharpenattr_1);
}

CVI_S32 CVI_ISP_SetNoiseProfileAttr(VI_PIPE ViPipe, const ISP_CMOS_NOISE_CALIBRATION_S *pstNoiseProfileAttr)
{
	SET_ATTR(ISP_CMOS_NOISE_CALIBRATION_S, pstNoiseProfileAttr, rpc_cvi_isp_setnoiseprofileattr_1);
}

CVI_S32 CVI_ISP_GetNoiseProfileAttr(VI_PIPE ViPipe, ISP_CMOS_NOISE_CALIBRATION_S *pstNoiseProfileAttr)
{
	GET_ATTR(ISP_CMOS_NOISE_CALIBRATION_S, pstNoiseProfileAttr, rpc_cvi_isp_getnoiseprofileattr_1);
}

CVI_S32 CVI_ISP_SetYContrastAttr(VI_PIPE ViPipe, const ISP_YCONTRAST_ATTR_S *pstYContrastAttr)
{
	SET_ATTR(ISP_YCONTRAST_ATTR_S, pstYContrastAttr, rpc_cvi_isp_setycontrastattr_1);
}

CVI_S32 CVI_ISP_GetYContrastAttr(VI_PIPE ViPipe, ISP_YCONTRAST_ATTR_S *pstYContrastAttr)
{
	GET_ATTR(ISP_YCONTRAST_ATTR_S, pstYContrastAttr, rpc_cvi_isp_getycontrastattr_1);
}

CVI_S32 CVI_ISP_SetDCFInfo(VI_PIPE ViPipe, const ISP_DCF_INFO_S *pstIspDCF)
{
	SET_ATTR(ISP_DCF_INFO_S, pstIspDCF, rpc_cvi_isp_setdcfinfo_1);
}

CVI_S32 CVI_ISP_GetDCFInfo(VI_PIPE ViPipe, ISP_DCF_INFO_S *pstIspDCF)
{
	GET_ATTR(ISP_DCF_INFO_S, pstIspDCF, rpc_cvi_isp_getdcfinfo_1);
}

CVI_S32 CVI_ISP_QueryInnerStateInfo(VI_PIPE ViPipe, ISP_INNER_STATE_INFO_S *pstInnerStateInfo)
{
	GET_ATTR(ISP_INNER_STATE_INFO_S, pstInnerStateInfo, rpc_cvi_isp_queryinnerstateinfo_1);
}

CVI_S32 CVI_ISP_GetVDTimeOut(VI_PIPE ViPipe, ISP_VD_TYPE_E enIspVDType, CVI_U32 u32MilliSec)
{
	CVI_S32 ret = CVI_SUCCESS;
	CVI_S32 *result;
	CLIENT *clnt;

	CREATE_CLNT();

	result = rpc_cvi_isp_getvdtimeout_1(ViPipe, enIspVDType, u32MilliSec, clnt);
	if (result == (CVI_S32 *) NULL) {
		clnt_perror (clnt, "call failed");
		ret = CVI_FAILURE;
	} else {
		ret = *result;
	}

	DESTROY_CLNT();

	return ret;
}

CVI_S32 CVI_ISP_SetMonoAttr(VI_PIPE ViPipe, const ISP_MONO_ATTR_S *pstMonoAttr)
{
	SET_ATTR(ISP_MONO_ATTR_S, pstMonoAttr, rpc_cvi_isp_setmonoattr_1);
}

CVI_S32 CVI_ISP_GetMonoAttr(VI_PIPE ViPipe, ISP_MONO_ATTR_S *pstMonoAttr)
{
	GET_ATTR(ISP_MONO_ATTR_S, pstMonoAttr, rpc_cvi_isp_getmonoattr_1);
}

CVI_S32 CVI_ISP_SetDisAttr(VI_PIPE ViPipe, const ISP_DIS_ATTR_S *pstDisAttr)
{
	SET_ATTR(ISP_DIS_ATTR_S, pstDisAttr, rpc_cvi_isp_setdisattr_1);
}

CVI_S32 CVI_ISP_GetDisAttr(VI_PIPE ViPipe, ISP_DIS_ATTR_S *pstDisAttr)
{
	GET_ATTR(ISP_DIS_ATTR_S, pstDisAttr, rpc_cvi_isp_getdisattr_1);
}

CVI_S32 CVI_ISP_SetDisConfig(VI_PIPE ViPipe, const ISP_DIS_CONFIG_S *pstDisConfig)
{
	SET_ATTR(ISP_DIS_CONFIG_S, pstDisConfig, rpc_cvi_isp_setdisconfig_1);
}

CVI_S32 CVI_ISP_GetDisConfig(VI_PIPE ViPipe, ISP_DIS_CONFIG_S *pstDisConfig)
{
	GET_ATTR(ISP_DIS_CONFIG_S, pstDisConfig, rpc_cvi_isp_getdisconfig_1);
}

CVI_S32 CVI_ISP_GetBinStatus(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	CVI_S32 *result;
	CLIENT *clnt;

	CREATE_CLNT();

	result = rpc_cvi_isp_getbinstatus_1(ViPipe, clnt);
	if (result == (CVI_S32 *) NULL) {
		clnt_perror (clnt, "call failed");
		ret = CVI_FAILURE;
	} else {
		ret = *result;
	}

	DESTROY_CLNT();

	return ret;
}

CVI_S32 CVI_ISP_SetVCAttr(VI_PIPE ViPipe, const ISP_VC_ATTR_S *pstVCAttr)
{
	SET_ATTR(ISP_VC_ATTR_S, pstVCAttr, rpc_cvi_isp_setvcattr_1);
}

CVI_S32 CVI_ISP_GetVCAttr(VI_PIPE ViPipe, ISP_VC_ATTR_S *pstVCAttr)
{
	GET_ATTR(ISP_VC_ATTR_S, pstVCAttr, rpc_cvi_isp_getvcattr_1);
}

#define __FREAD_BUFFER_SIZE  64
#define __TEMP_FILE "/tmp/ispRpcTempFile"

CVI_S32 CVI_ISP_DumpHwRegisterToFile(VI_PIPE ViPipe, FILE *fp)
{
	CVI_S32 ret;
	RPC_CVI_S32  *result;
	CLIENT *clnt;

	FILE *fpTemp = NULL;
	size_t size = 0;
	uint8_t buffer[__FREAD_BUFFER_SIZE] = {0};

	CREATE_CLNT();

	result = rpc_cvi_isp_dumphwregistertofile_1(ViPipe, (char *)__TEMP_FILE, clnt);
	if (result == (RPC_CVI_S32 *) NULL) {
		clnt_perror (clnt, "call failed");
		ret = CVI_FAILURE;
	} else {
		ret = *result;
	}

	DESTROY_CLNT();

	fpTemp = fopen(__TEMP_FILE, "r");

	if (fpTemp != NULL) {
		do {
			size = fread(buffer, sizeof(uint8_t), __FREAD_BUFFER_SIZE, fpTemp);

			if (size > 0) {
				fwrite(buffer, sizeof(uint8_t), size, fp);
			}

		} while (size > 0);

		fclose(fpTemp);
	}

	return ret;
}

CVI_S32 CVI_ISP_DumpFrameRawInfoToFile(VI_PIPE ViPipe, FILE *fp)
{
	CVI_S32 ret;
	RPC_CVI_S32  *result;
	CLIENT *clnt;

	FILE *fpTemp = NULL;
	size_t size = 0;
	uint8_t buffer[__FREAD_BUFFER_SIZE] = {0};

	CREATE_CLNT();

	result = rpc_cvi_isp_dumpframerawinfotofile_1(ViPipe, (char *)__TEMP_FILE, clnt);
	if (result == (RPC_CVI_S32 *) NULL) {
		clnt_perror (clnt, "call failed");
		ret = CVI_FAILURE;
	} else {
		ret = *result;
	}

	DESTROY_CLNT();

	fpTemp = fopen(__TEMP_FILE, "r");

	if (fpTemp != NULL) {
		do {
			size = fread(buffer, sizeof(uint8_t), __FREAD_BUFFER_SIZE, fpTemp);

			if (size > 0) {
				fwrite(buffer, sizeof(uint8_t), size, fp);
			}

		} while (size > 0);

		fclose(fpTemp);
	}

	return ret;
}

CVI_S32 CVI_ISP_IrAutoRunOnce(ISP_DEV IspDev, ISP_IR_AUTO_ATTR_S *pstIrAttr)
{
	CLIENT *clnt;
	CVI_S32 ret = CVI_FAILURE;
	RPC_ISP_GET_ATTR_DATA_S *result;
	RPC_ISP_SET_ATTR_DATA_S stAttrData;
	clnt = clnt_create (host, ISP_RPC, ISP_RPC_VERSION, "tcp");
	if (clnt == NULL) {
		clnt_pcreateerror (host);
		ABORT();
	}
	stAttrData.data.data_len = sizeof(ISP_IR_AUTO_ATTR_S);
	stAttrData.data.data_val = (uint8_t *) pstIrAttr;
	result = rpc_cvi_isp_irautorunonce_1(IspDev, stAttrData, clnt);
	if (result == (RPC_ISP_GET_ATTR_DATA_S *) NULL) {
		clnt_perror (clnt, "call failed");
		LOGOUT("isp rpc set attr error!!!\n");
	} else {
		ret = result->ret;
		if (result->data.data_len == sizeof(ISP_IR_AUTO_ATTR_S)) {
			memcpy(pstIrAttr, (ISP_IR_AUTO_ATTR_S *)result->data.data_val, sizeof(ISP_IR_AUTO_ATTR_S));
		}else {
			LOGOUT("isp rpc get attr error!!!\n");
			ret = CVI_FAILURE;
		}
	}
	clnt_destroy (clnt);
	return ret;
}

CVI_S32 CVI_ISP_SetSmartInfo(VI_PIPE ViPipe, const ISP_SMART_INFO_S *pstSmartInfo, CVI_U8 TimeOut)
{
	CLIENT *clnt;
	CVI_S32 ret = CVI_FAILURE;
	RPC_CVI_S32 *result;
	RPC_ISP_SET_ATTR_DATA_S stAttrData;
	clnt = clnt_create (host, ISP_RPC, ISP_RPC_VERSION, "tcp");
	if (clnt == NULL) {
		clnt_pcreateerror (host);
		ABORT();
	}
	stAttrData.data.data_len = sizeof(ISP_IR_AUTO_ATTR_S);
	stAttrData.data.data_val = (uint8_t *) pstSmartInfo;
	result = rpc_cvi_isp_setsmartinfo_1(ViPipe, stAttrData, TimeOut, clnt);
	if (result == (RPC_CVI_S32 *) NULL) {
		clnt_perror (clnt, "call failed");
		LOGOUT("isp rpc set attr error!!!\n");
	} else {
		ret = *result;
	}
	clnt_destroy (clnt);

	return ret;
}

CVI_S32 CVI_ISP_GetSmartInfo(VI_PIPE ViPipe, ISP_SMART_INFO_S *pstSmartInfo)
{
	GET_ATTR(ISP_SMART_INFO_S, pstSmartInfo, rpc_cvi_isp_getsmartinfo_1);
}

#ifdef ARCH_CV182X
CVI_S32 CVI_ISP_SetTNRMotionAdaptAttr(VI_PIPE ViPipe, const ISP_TNR_MOTION_ADAPT_ATTR_S *pstTNRMotionAdaptAttr)
{
	SET_ATTR(ISP_TNR_MOTION_ADAPT_ATTR_S, pstTNRMotionAdaptAttr, rpc_cvi_isp_settnrmotionadaptattr_1);
}

CVI_S32 CVI_ISP_GetTNRMotionAdaptAttr(VI_PIPE ViPipe, ISP_TNR_MOTION_ADAPT_ATTR_S *pstTNRMotionAdaptAttr)
{
	GET_ATTR(ISP_TNR_MOTION_ADAPT_ATTR_S, pstTNRMotionAdaptAttr, rpc_cvi_isp_gettnrmotionadaptattr_1);
}

CVI_S32 CVI_ISP_SetClutSaturationAttr(VI_PIPE ViPipe, const ISP_CLUT_SATURATION_ATTR_S *pstClutSaturationAttr)
{
	SET_ATTR(ISP_CLUT_SATURATION_ATTR_S, pstClutSaturationAttr, rpc_cvi_isp_setclutsaturationattr_1);
}

CVI_S32 CVI_ISP_GetClutSaturationAttr(VI_PIPE ViPipe, ISP_CLUT_SATURATION_ATTR_S *pstClutSaturationAttr)
{
	GET_ATTR(ISP_CLUT_SATURATION_ATTR_S, pstClutSaturationAttr, rpc_cvi_isp_getclutsaturationattr_1);
}

CVI_S32 CVI_ISP_GetGammaCurveByType(VI_PIPE ViPipe,ISP_GAMMA_ATTR_S *pstGammaAttr, const ISP_GAMMA_CURVE_TYPE_E curveType)
{
	CVI_S32 ret;
	RPC_ISP_GET_ATTR_DATA_S  *result;
	CLIENT *clnt;

	CREATE_CLNT();

	result = rpc_cvi_isp_getgammacurvebytype_1(ViPipe,curveType, clnt);
	if (result == (RPC_ISP_GET_ATTR_DATA_S *) NULL) {
		clnt_perror (clnt, "call failed");
		ret = CVI_FAILURE;
	} else {
		ret = result->ret;
		if (sizeof(ISP_GAMMA_ATTR_S) == result->data.data_len) {
			memcpy(pstGammaAttr, result->data.data_val, sizeof(ISP_GAMMA_ATTR_S));
		} else {
			LOGOUT("isp rpc get attr error!!!\n");
			ret = CVI_FAILURE;
		}
	}

	DESTROY_CLNT();

	return ret;
}

CVI_S32 CVI_ISP_SetPreSharpenAttr(VI_PIPE ViPipe, const ISP_PRESHARPEN_ATTR_S *pstPreSharpenAttr)
{
	SET_ATTR(ISP_PRESHARPEN_ATTR_S, pstPreSharpenAttr, rpc_cvi_isp_setpresharpenattr_1);
}

CVI_S32 CVI_ISP_GetPreSharpenAttr(VI_PIPE ViPipe, ISP_PRESHARPEN_ATTR_S *pstPreSharpenAttr)
{
	GET_ATTR(ISP_PRESHARPEN_ATTR_S, pstPreSharpenAttr, rpc_cvi_isp_getpresharpenattr_1);
}

CVI_S32 CVI_ISP_SetRGBCACAttr(VI_PIPE ViPipe, const ISP_RGBCAC_ATTR_S *pstRGBCACAttr)
{
	SET_ATTR(ISP_RGBCAC_ATTR_S, pstRGBCACAttr, rpc_cvi_isp_setrgbcacattr_1);
}

CVI_S32 CVI_ISP_GetRGBCACAttr(VI_PIPE ViPipe, ISP_RGBCAC_ATTR_S *pstRGBCACAttr)
{
	GET_ATTR(ISP_RGBCAC_ATTR_S, pstRGBCACAttr, rpc_cvi_isp_getrgbcacattr_1);
}

CVI_S32 CVI_ISP_SetCNRMotionNRAttr(VI_PIPE ViPipe, const ISP_CNR_MOTION_NR_ATTR_S *pstCNRMotionNRAttr)
{
	SET_ATTR(ISP_CNR_MOTION_NR_ATTR_S, pstCNRMotionNRAttr, rpc_cvi_isp_setcnrmotionnrattr_1);
}

CVI_S32 CVI_ISP_GetCNRMotionNRAttr(VI_PIPE ViPipe, ISP_CNR_MOTION_NR_ATTR_S *pstCNRMotionNRAttr)
{
	GET_ATTR(ISP_CNR_MOTION_NR_ATTR_S, pstCNRMotionNRAttr, rpc_cvi_isp_getcnrmotionnrattr_1);
}

CVI_S32 CVI_ISP_SetCCMSaturationAttr(VI_PIPE ViPipe, const ISP_CCM_SATURATION_ATTR_S *pstCCMSaturationAttr)
{
	SET_ATTR(ISP_CCM_SATURATION_ATTR_S, pstCCMSaturationAttr, rpc_cvi_isp_setccmsaturationattr_1);
}

CVI_S32 CVI_ISP_GetCCMSaturationAttr(VI_PIPE ViPipe, ISP_CCM_SATURATION_ATTR_S *pstCCMSaturationAttr)
{
	GET_ATTR(ISP_CCM_SATURATION_ATTR_S, pstCCMSaturationAttr, rpc_cvi_isp_getccmsaturationattr_1);
}

CVI_S32 CVI_ISP_SetCAAttr(VI_PIPE ViPipe, const ISP_CA_ATTR_S *pstCAAttr)
{
	SET_ATTR(ISP_CA_ATTR_S, pstCAAttr, rpc_cvi_isp_setcaattr_1);
}

CVI_S32 CVI_ISP_GetCAAttr(VI_PIPE ViPipe, ISP_CA_ATTR_S *pstCAAttr)
{
	GET_ATTR(ISP_CA_ATTR_S, pstCAAttr, rpc_cvi_isp_getcaattr_1);
}

CVI_S32 CVI_ISP_SetCSCAttr(VI_PIPE ViPipe, const ISP_CSC_ATTR_S *pstCscAttr)
{
	SET_ATTR(ISP_CSC_ATTR_S, pstCscAttr, rpc_cvi_isp_setcscattr_1);
}

CVI_S32 CVI_ISP_GetCSCAttr(VI_PIPE ViPipe, ISP_CSC_ATTR_S *pstCscAttr)
{
	GET_ATTR(ISP_CSC_ATTR_S, pstCscAttr, rpc_cvi_isp_getcscattr_1);
}

#endif

CVI_S32 CVI_ISP_SetExposureAttr(VI_PIPE ViPipe, const ISP_EXPOSURE_ATTR_S *pstExpAttr)
{
	SET_ATTR(ISP_EXPOSURE_ATTR_S, pstExpAttr, rpc_cvi_isp_setexposureattr_1);
}

CVI_S32 CVI_ISP_GetExposureAttr(VI_PIPE ViPipe, ISP_EXPOSURE_ATTR_S *pstExpAttr)
{
	GET_ATTR(ISP_EXPOSURE_ATTR_S, pstExpAttr, rpc_cvi_isp_getexposureattr_1);
}

CVI_S32 CVI_ISP_QueryExposureInfo(VI_PIPE ViPipe, ISP_EXP_INFO_S *pstExpInfo)
{
	GET_ATTR(ISP_EXP_INFO_S, pstExpInfo, rpc_cvi_isp_queryexposureinfo_1);
}

CVI_S32 CVI_ISP_SetWDRExposureAttr(VI_PIPE ViPipe, const ISP_WDR_EXPOSURE_ATTR_S *pstWDRExpAttr)
{
	SET_ATTR(ISP_WDR_EXPOSURE_ATTR_S, pstWDRExpAttr, rpc_cvi_isp_setwdrexposureattr_1);
}

CVI_S32 CVI_ISP_GetWDRExposureAttr(VI_PIPE ViPipe, ISP_WDR_EXPOSURE_ATTR_S *pstWDRExpAttr)
{
	GET_ATTR(ISP_WDR_EXPOSURE_ATTR_S, pstWDRExpAttr, rpc_cvi_isp_getwdrexposureattr_1);
}

CVI_S32 CVI_ISP_SetAERouteAttr(VI_PIPE ViPipe, const ISP_AE_ROUTE_S *pstAERouteAttr)
{
	SET_ATTR(ISP_AE_ROUTE_S, pstAERouteAttr, rpc_cvi_isp_setaerouteattr_1);
}

CVI_S32 CVI_ISP_GetAERouteAttr(VI_PIPE ViPipe, ISP_AE_ROUTE_S *pstAERouteAttr)
{
	GET_ATTR(ISP_AE_ROUTE_S, pstAERouteAttr, rpc_cvi_isp_getaerouteattr_1);
}

CVI_S32 CVI_ISP_SetAERouteAttrEx(VI_PIPE ViPipe, const ISP_AE_ROUTE_EX_S *pstAERouteAttr)
{
	SET_ATTR(ISP_AE_ROUTE_EX_S, pstAERouteAttr, rpc_cvi_isp_setaerouteattrex_1);
}

CVI_S32 CVI_ISP_GetAERouteAttrEx(VI_PIPE ViPipe, ISP_AE_ROUTE_EX_S *pstAERouteAttr)
{
	GET_ATTR(ISP_AE_ROUTE_EX_S, pstAERouteAttr, rpc_cvi_isp_getaerouteattrex_1);
}

CVI_S32 CVI_ISP_AEBracketingStart(VI_PIPE ViPipe)
{
	CVI_S32 ret;
	RPC_CVI_S32  *result;
	CLIENT *clnt;

	CREATE_CLNT();

	result = rpc_cvi_isp_aebracketingstart_1(ViPipe, clnt);
	if (result == (RPC_CVI_S32 *) NULL) {
		clnt_perror (clnt, "call failed");
		ret = CVI_FAILURE;
	} else {
		ret = *result;
	}

	DESTROY_CLNT();

	return ret;
}

CVI_S32 CVI_ISP_AEBracketingSetExpsoure(VI_PIPE ViPipe, CVI_S16 leEvX10, CVI_S16 seEvX10)
{
	CVI_S32 ret;
	RPC_CVI_S32  *result;
	CLIENT *clnt;

	CREATE_CLNT();

	result = rpc_cvi_isp_aebracketingsetexpsoure_1(ViPipe, leEvX10, seEvX10, clnt);
	if (result == (RPC_CVI_S32 *) NULL) {
		clnt_perror (clnt, "call failed");
		ret = CVI_FAILURE;
	} else {
		ret = *result;
	}

	DESTROY_CLNT();

	return ret;
}

CVI_S32 CVI_ISP_AEBracketingFinish(VI_PIPE ViPipe)
{
	CVI_S32 ret;
	RPC_CVI_S32  *result;
	CLIENT *clnt;

	CREATE_CLNT();

	result = rpc_cvi_isp_aebracketingfinish_1(ViPipe, clnt);
	if (result == (RPC_CVI_S32 *) NULL) {
		clnt_perror (clnt, "call failed");
		ret = CVI_FAILURE;
	} else {
		ret = *result;
	}

	DESTROY_CLNT();

	return ret;
}

CVI_S32 CVI_ISP_AEBracketingSetSimple(CVI_BOOL bEnable)
{
	CVI_S32 ret;
	RPC_CVI_S32  *result;
	CLIENT *clnt;

	CREATE_CLNT();

	result = rpc_cvi_isp_aebracketingsetsimple_1(bEnable, clnt);
	if (result == (RPC_CVI_S32 *) NULL) {
		clnt_perror (clnt, "call failed");
		ret = CVI_FAILURE;
	} else {
		ret = *result;
	}

	DESTROY_CLNT();

	return ret;
}

CVI_S32 CVI_ISP_SetAEStatisticsConfig(VI_PIPE ViPipe, const ISP_AE_STATISTICS_CFG_S *pstAeStatCfg)
{
	SET_ATTR(ISP_AE_STATISTICS_CFG_S, pstAeStatCfg, rpc_cvi_isp_setaestatisticsconfig_1);
}

CVI_S32 CVI_ISP_GetAEStatisticsConfig(VI_PIPE ViPipe, ISP_AE_STATISTICS_CFG_S *pstAeStatCfg)
{
	GET_ATTR(ISP_AE_STATISTICS_CFG_S, pstAeStatCfg, rpc_cvi_isp_getaestatisticsconfig_1);
}

CVI_S32 CVI_ISP_SetAntiFlicker(VI_PIPE ViPipe, CVI_BOOL enable, CVI_U8 frequency)
{
	CVI_S32 ret;
	RPC_CVI_S32  *result;
	CLIENT *clnt;

	CREATE_CLNT();

	result = rpc_cvi_isp_setantiflicker_1(ViPipe, enable, frequency, clnt);
	if (result == (RPC_CVI_S32 *) NULL) {
		clnt_perror (clnt, "call failed");
		ret = CVI_FAILURE;
	} else {
		ret = *result;
	}

	DESTROY_CLNT();

	return ret;
}

CVI_S32 CVI_ISP_GetAntiFlicker(VI_PIPE ViPipe, CVI_BOOL *pEnable, CVI_U8 *pFrequency)
{
	CVI_S32 ret = CVI_SUCCESS;
	uint32_t  *result;
	CLIENT *clnt;

	CREATE_CLNT();

	result = rpc_cvi_isp_getantiflicker_1(ViPipe, clnt);
	if (result == (uint32_t *) NULL) {
		clnt_perror (clnt, "call failed");
		ret = CVI_FAILURE;
	} else {
		*pEnable = (*result >> 8) & 0xFF;
		*pFrequency = (*result) & 0xFF;
	}

	DESTROY_CLNT();

	return ret;
}

CVI_S32 CVI_ISP_SetWDRLEOnly(VI_PIPE ViPipe, CVI_BOOL wdrLEOnly)
{
	CVI_S32 ret;
	RPC_CVI_S32  *result;
	CLIENT *clnt;

	CREATE_CLNT();

	result = rpc_cvi_isp_setwdrleonly_1(ViPipe, wdrLEOnly, clnt);
	if (result == (RPC_CVI_S32 *) NULL) {
		clnt_perror (clnt, "call failed");
		ret = CVI_FAILURE;
	} else {
		ret = *result;
	}

	DESTROY_CLNT();

	return ret;
}

CVI_S32 CVI_ISP_GetFrameID(VI_PIPE ViPipe, CVI_U32 *frameID)
{
	CVI_S32 ret = CVI_SUCCESS;
	uint32_t  *result;
	CLIENT *clnt;

	CREATE_CLNT();

	result = rpc_cvi_isp_getframeid_1(ViPipe, clnt);
	if (result == (uint32_t *) NULL) {
		clnt_perror (clnt, "call failed");
		ret = CVI_FAILURE;
	} else {
		*frameID = *result;
	}

	DESTROY_CLNT();

	return ret;
}

CVI_S32 CVI_ISP_SetAELogPath(const char *szPath)
{
	CVI_S32 ret;
	RPC_CVI_S32  *result;
	CLIENT *clnt;

	CREATE_CLNT();

	result = rpc_cvi_isp_setaelogpath_1((char *)szPath, clnt);
	if (result == (RPC_CVI_S32 *) NULL) {
		clnt_perror (clnt, "call failed");
		ret = CVI_FAILURE;
	} else {
		ret = *result;
	}

	DESTROY_CLNT();

	return ret;
}

CVI_S32 CVI_ISP_GetAELogPath(char *szPath, CVI_U32 pathSize)
{
	CVI_S32 ret;
	RPC_ISP_GET_ATTR_DATA_S  *result;
	CLIENT *clnt;

	CREATE_CLNT();

	result = rpc_cvi_isp_getaelogpath_1(pathSize,clnt);
	if (result == (RPC_ISP_GET_ATTR_DATA_S *) NULL) {
		clnt_perror (clnt, "call failed");
		ret = CVI_FAILURE;
	} else {
		if (result->ret == CVI_SUCCESS) {
			strcpy(szPath, (char *)result->data.data_val);
		} else {
			printf("path size is too small \n");
			ret = CVI_FAILURE;
		}
	}

	DESTROY_CLNT();

	return ret;
}

CVI_S32 CVI_ISP_SetAELogName(const char *szName)
{
	CVI_S32 ret;
	RPC_CVI_S32  *result;
	CLIENT *clnt;

	CREATE_CLNT();

	result = rpc_cvi_isp_setaelogname_1((char *)szName, clnt);
	if (result == (RPC_CVI_S32 *) NULL) {
		clnt_perror (clnt, "call failed");
		ret = CVI_FAILURE;
	} else {
		ret = *result;
	}

	DESTROY_CLNT();

	return ret;
}

CVI_S32 CVI_ISP_GetAELogName(char *szName, CVI_U32 nameSize)
{
	CVI_S32 ret;
	RPC_ISP_GET_ATTR_DATA_S  *result;
	CLIENT *clnt;

	CREATE_CLNT();

	result = rpc_cvi_isp_getaelogname_1(nameSize,clnt);
	if (result == (RPC_ISP_GET_ATTR_DATA_S *) NULL) {
		clnt_perror (clnt, "call failed");
		ret = CVI_FAILURE;
	} else {
		if (result->ret == CVI_SUCCESS) {
			strcpy(szName, (char *)result->data.data_val);
		} else {
			printf("name size is too small \n");
			ret = CVI_FAILURE;
		}
	}

	DESTROY_CLNT();

	return ret;
}

CVI_S32 CVI_ISP_QueryFps(VI_PIPE ViPipe, CVI_FLOAT *pFps)
{
	CVI_S32 ret = CVI_SUCCESS;
	CVI_FLOAT *result;
	CLIENT *clnt;

	CREATE_CLNT();

	result = rpc_cvi_isp_queryfps_1(ViPipe, clnt);
	if (result == (CVI_FLOAT *) NULL) {
		clnt_perror (clnt, "call failed");
		ret = CVI_FAILURE;
	} else {
		*pFps = *result;
	}

	DESTROY_CLNT();

	return ret;
}

CVI_S32 CVI_ISP_GetAELogBuf(VI_PIPE ViPipe, CVI_U8 *buf, CVI_U32 bufSize)
{
	CVI_S32 ret = CVI_SUCCESS;
	RPC_ISP_GET_ATTR_DATA_S *result;
	CLIENT *clnt;
	CVI_U32 size = 0x00;

	CREATE_CLNT();

	result = rpc_cvi_isp_getaelogbuf_1(ViPipe, clnt);
	if (result == (RPC_ISP_GET_ATTR_DATA_S *) NULL) {
		clnt_perror (clnt, "call failed");
		ret = CVI_FAILURE;
	} else {
		ret = result->ret;
		size = bufSize > result->data.data_len ? result->data.data_len : bufSize;
		memcpy(buf, result->data.data_val, size);
	}

	DESTROY_CLNT();

	return ret;
}

CVI_S32 CVI_ISP_GetAELogBufSize(VI_PIPE ViPipe, CVI_U32 *bufSize)
{
	CVI_S32 ret = CVI_SUCCESS;
	RPC_ISP_GET_ATTR_DATA_S *result;
	CLIENT *clnt;

	CREATE_CLNT();

	result = rpc_cvi_isp_getaelogbufsize_1(ViPipe, clnt);
	if (result == (RPC_ISP_GET_ATTR_DATA_S *) NULL) {
		clnt_perror (clnt, "call failed");
		ret = CVI_FAILURE;
	} else {
		ret = result->ret;
		*bufSize = *(CVI_U32 *)result->data.data_val;
	}

	DESTROY_CLNT();

	return ret;
}

CVI_S32 CVI_ISP_GetAEBinBuf(VI_PIPE ViPipe, CVI_U8 *pBuf, CVI_U32 bufSize)
{
	CVI_S32 ret = CVI_SUCCESS;
	RPC_ISP_GET_ATTR_DATA_S *result;
	CLIENT *clnt;
	CVI_U32 size = 0x00;

	CREATE_CLNT();

	result = rpc_cvi_isp_getaebinbuf_1(ViPipe, clnt);
	if (result == (RPC_ISP_GET_ATTR_DATA_S *) NULL) {
		clnt_perror (clnt, "call failed");
		ret = CVI_FAILURE;
	} else {
		ret = result->ret;
		size = bufSize > result->data.data_len ? result->data.data_len : bufSize;
		memcpy(pBuf, result->data.data_val, size);
	}

	DESTROY_CLNT();

	return ret;
}

CVI_S32 CVI_ISP_GetAEBinBufSize(VI_PIPE ViPipe, CVI_U32 *bufSize)
{
	CVI_S32 ret = CVI_SUCCESS;
	RPC_ISP_GET_ATTR_DATA_S *result;
	CLIENT *clnt;

	CREATE_CLNT();

	result = rpc_cvi_isp_getaebinbufsize_1(ViPipe, clnt);
	if (result == (RPC_ISP_GET_ATTR_DATA_S *) NULL) {
		clnt_perror (clnt, "call failed");
		ret = CVI_FAILURE;
	} else {
		ret = result->ret;
		*bufSize = *(CVI_U32 *)result->data.data_val;
	}

	DESTROY_CLNT();

	return ret;
}

CVI_S32 CVI_ISP_GetCurrentLvX100(VI_PIPE ViPipe, CVI_S16 *ps16Lv)
{
	CVI_S32 ret = CVI_SUCCESS;
	RPC_ISP_GET_ATTR_DATA_S *result;
	CLIENT *clnt;

	CREATE_CLNT();

	result = rpc_cvi_isp_getcurrentlvx100_1(ViPipe, clnt);
	if (result == (RPC_ISP_GET_ATTR_DATA_S *) NULL) {
		clnt_perror (clnt, "call failed");
		ret = CVI_FAILURE;
	} else {
		ret = result->ret;
		*ps16Lv = *(CVI_S16 *)result->data.data_val;
	}

	DESTROY_CLNT();

	return ret;
}

CVI_S32 CVI_ISP_SetFastBootExposure(VI_PIPE ViPipe, CVI_U32 expLine, CVI_U32 again, CVI_U32 dgain, CVI_U32 ispdgain)
{
	CVI_S32 ret = CVI_SUCCESS;
	RPC_CVI_S32 *result;
	CLIENT *clnt;

	CREATE_CLNT();

	result = rpc_cvi_isp_setfastbootexposure_1(ViPipe, expLine, again, dgain, ispdgain, clnt);
	if (result == (RPC_CVI_S32 *) NULL) {
		clnt_perror (clnt, "call failed");
		ret = CVI_FAILURE;
	} else {
		ret = *result;
	}

	DESTROY_CLNT();

	return ret;
}

CVI_S32 CVI_ISP_SetSmartExposureAttr(VI_PIPE ViPipe, const ISP_SMART_EXPOSURE_ATTR_S *pstSmartExpAttr)
{
	SET_ATTR(ISP_SMART_EXPOSURE_ATTR_S, pstSmartExpAttr, rpc_cvi_isp_setsmartexposureattr_1);

}

CVI_S32 CVI_ISP_GetSmartExposureAttr(VI_PIPE ViPipe, ISP_SMART_EXPOSURE_ATTR_S *pstSmartExpAttr)
{
	GET_ATTR(ISP_SMART_EXPOSURE_ATTR_S, pstSmartExpAttr, rpc_cvi_isp_getsmartexposureattr_1);
}

CVI_S32 CVI_ISP_SetWBAttr(VI_PIPE ViPipe, const ISP_WB_ATTR_S *pstWBAttr)
{
	SET_ATTR(ISP_WB_ATTR_S, pstWBAttr, rpc_cvi_isp_setwbattr_1);
}

CVI_S32 CVI_ISP_GetWBAttr(VI_PIPE ViPipe, ISP_WB_ATTR_S *pstWBAttr)
{
	GET_ATTR(ISP_WB_ATTR_S, pstWBAttr, rpc_cvi_isp_getwbattr_1);
}

CVI_S32 CVI_ISP_SetAWBAttrEx(VI_PIPE ViPipe, const ISP_AWB_ATTR_EX_S *pstAWBAttrEx)
{
	SET_ATTR(ISP_AWB_ATTR_EX_S, pstAWBAttrEx, rpc_cvi_isp_setawbattrex_1);
}

CVI_S32 CVI_ISP_GetAWBAttrEx(VI_PIPE ViPipe, ISP_AWB_ATTR_EX_S *pstAWBAttrEx)
{
	GET_ATTR(ISP_AWB_ATTR_EX_S, pstAWBAttrEx, rpc_cvi_isp_getawbattrex_1);
}

CVI_S32 CVI_ISP_QueryWBInfo(VI_PIPE ViPipe, ISP_WB_INFO_S *pstWBInfo)
{
	GET_ATTR(ISP_WB_INFO_S, pstWBInfo, rpc_cvi_isp_querywbinfo_1);
}

CVI_S32 CVI_ISP_SetWBCalibration(VI_PIPE ViPipe, const ISP_AWB_Calibration_Gain_S *pstWBCalib)
{
	SET_ATTR(ISP_AWB_Calibration_Gain_S, pstWBCalib, rpc_cvi_isp_setwbcalibration_1);
}

CVI_S32 CVI_ISP_GetWBCalibration(VI_PIPE ViPipe, ISP_AWB_Calibration_Gain_S *pstWBCalib)
{
	GET_ATTR(ISP_AWB_Calibration_Gain_S, pstWBCalib, rpc_cvi_isp_getwbcalibration_1);
}

CVI_S32 CVI_ISP_SetWBCalibrationEx(VI_PIPE ViPipe, const ISP_AWB_Calibration_Gain_S_EX *pstWBCalib)
{
	SET_ATTR(ISP_AWB_Calibration_Gain_S_EX, pstWBCalib, rpc_cvi_isp_setwbcalibrationex_1);
}

CVI_S32 CVI_ISP_GetWBCalibrationEx(VI_PIPE ViPipe, ISP_AWB_Calibration_Gain_S_EX *pstWBCalib)
{
	GET_ATTR(ISP_AWB_Calibration_Gain_S_EX, pstWBCalib, rpc_cvi_isp_getwbcalibrationex_1);
}

CVI_S32 CVI_ISP_SetAWBWinStatictics(VI_PIPE ViPipe, const ISP_WB_STATISTICS_CFG_S *pstAwbWinCfg)
{
	SET_ATTR(ISP_WB_STATISTICS_CFG_S, pstAwbWinCfg, rpc_cvi_isp_setawbwinstatictics_1);
}

CVI_S32 CVI_ISP_GetAWBWinStatictics(VI_PIPE ViPipe, ISP_WB_STATISTICS_CFG_S *pstAwbWinCfg)
{
	GET_ATTR(ISP_WB_STATISTICS_CFG_S, pstAwbWinCfg, rpc_cvi_isp_getawbwinstatictics_1);
}

CVI_S32 CVI_ISP_GetAWBSnapLogBuf(VI_PIPE ViPipe, CVI_U8 *buf, CVI_U32 bufSize)
{
	CVI_S32 ret = CVI_SUCCESS;
	RPC_ISP_GET_ATTR_DATA_S *result;
	CLIENT *clnt;
	CVI_U32 size = 0x00;

	CREATE_CLNT();

	result = rpc_cvi_isp_getawbsnaplogbuf_1(ViPipe, clnt);
	if (result == (RPC_ISP_GET_ATTR_DATA_S *) NULL) {
		clnt_perror (clnt, "call failed");
		ret = CVI_FAILURE;
	} else {
		ret = result->ret;
		size = bufSize > result->data.data_len ? result->data.data_len : bufSize;
		memcpy(buf, result->data.data_val, size);
	}

	DESTROY_CLNT();

	return ret;
}

CVI_S32 CVI_ISP_GetAWBDbgBinBuf(VI_PIPE ViPipe, CVI_U8 *buf, CVI_U32 bufSize)
{
	CVI_S32 ret = CVI_SUCCESS;
	RPC_ISP_GET_ATTR_DATA_S *result;
	CLIENT *clnt;
	CVI_U32 size = 0x00;

	CREATE_CLNT();

	result = rpc_cvi_isp_getawbdbgbinbuf_1(ViPipe, clnt);
	if (result == (RPC_ISP_GET_ATTR_DATA_S *) NULL) {
		clnt_perror (clnt, "call failed");
		ret = CVI_FAILURE;
	} else {
		ret = result->ret;
		size = bufSize > result->data.data_len ? result->data.data_len : bufSize;
		memcpy(buf, result->data.data_val, size);
	}

	DESTROY_CLNT();

	return ret;
}

CVI_S32 CVI_ISP_GetAWBDbgBinSize(void)
{
	CVI_S32 ret = CVI_SUCCESS;
	CVI_S32 *result;
	CLIENT *clnt;

	CREATE_CLNT();

	result = rpc_cvi_isp_getawbdbgbinsize_1(clnt);
	if (result == (CVI_S32 *) NULL) {
		clnt_perror (clnt, "call failed");
		ret = CVI_FAILURE;
	} else {
		ret = *result;
	}

	DESTROY_CLNT();

	return ret;
}

CVI_S32 CVI_ISP_GetAWBCurve(VI_PIPE ViPipe, ISP_WB_CURVE_S *pshWBCurve)
{
	GET_ATTR(ISP_WB_CURVE_S, pshWBCurve, rpc_cvi_isp_getawbcurve_1);
}

CVI_S32 CVI_ISP_SetAWBLogPath(const char *szPath)
{
	CVI_S32 ret;
	RPC_CVI_S32  *result;
	CLIENT *clnt;

	CREATE_CLNT();

	result = rpc_cvi_isp_setawblogpath_1((char *)szPath, clnt);
	if (result == (RPC_CVI_S32 *) NULL) {
		clnt_perror (clnt, "call failed");
		ret = CVI_FAILURE;
	} else {
		ret = *result;
	}

	DESTROY_CLNT();

	return ret;
}

CVI_S32 CVI_ISP_SetAWBLogName(const char *szName)
{
	CVI_S32 ret;
	RPC_CVI_S32  *result;
	CLIENT *clnt;

	CREATE_CLNT();

	result = rpc_cvi_isp_setawblogname_1((char *)szName, clnt);
	if (result == (RPC_CVI_S32 *) NULL) {
		clnt_perror (clnt, "call failed");
		ret = CVI_FAILURE;
	} else {
		ret = *result;
	}

	DESTROY_CLNT();

	return ret;
}

CVI_BOOL CVI_AE_IsAeSimMode(void)
{
	LOGOUT("UNUSED !!!");
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_GetRepoInfo(CVI_CHAR *gerritid, CVI_CHAR *commitid)
{
	LOGOUT("UNUSED !!!");
	UNUSED(gerritid);
	UNUSED(commitid);
	return CVI_SUCCESS;
}

//#pragma GCC diagnostic pop

