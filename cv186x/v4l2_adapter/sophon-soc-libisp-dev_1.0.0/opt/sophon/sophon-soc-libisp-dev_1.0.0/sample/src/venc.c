
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>

#include "vi.h"
#include "venc.h"
#include "cvi_isp.h"
#include "clog.h"
#include "isp_debug.h"
#include "cvi_sys.h"
#include "cvi_venc.h"

#define UNUSED(x) ((void)(x))

#define MODULE_ID (VENC_MODULE_ID)
#define SEND_FRAME_MILLSEC 20000
#define GET_FRAME_MILLSEC 2000

typedef struct {
	bool bufEnFlag;
	VideoBuffer buf;
	VIDEO_FRAME_INFO_S stFrame;
} __VencCtx_S;

static __VencCtx_S VencCtx[VI_MAX_PIPE_NUM];

static int get_video_frame(int chn)
{
	int ySize = 0;

	if (VencCtx[chn].bufEnFlag) {
		return 0;
	}

	if (get_yuv_frame(chn, MODULE_ID, &VencCtx[chn].buf) < 0) {
		return -1;
	}

	// TODO dump support multi chn
	if (access("/tmp/isp_dump_yuv", F_OK) == 0) {

		system("rm /tmp/isp_dump_yuv");

		FILE *fp = fopen("/tmp/isp_dump.yuv", "wb");

		if (fp) {

			void *vir_addr = CVI_SYS_Mmap(VencCtx[chn].buf.phy_addr, VencCtx[chn].buf.length);

			fwrite(vir_addr, VencCtx[chn].buf.length, 1, fp);
			fclose(fp);
			printf("isp dump yuv success, saved in /tmp/isp_dump.yuv\n");

			CVI_SYS_Munmap(vir_addr, VencCtx[chn].buf.length);
		}
	}

	//VencCtx[chn].stFrame.stVFrame.s32FrameIdx = VencCtx[chn].buf.id;
	//VencCtx[chn].stFrame.stVFrame.s32FrameIdx = -1;
	VencCtx[chn].stFrame.stVFrame.s32FrameIdx = 0; // !!!
	VencCtx[chn].stFrame.stVFrame.u32Width = VencCtx[chn].buf.width;
	VencCtx[chn].stFrame.stVFrame.u32Height = VencCtx[chn].buf.height;

	VencCtx[chn].stFrame.stVFrame.enPixelFormat = PIXEL_FORMAT_NV21; // !!!
	VencCtx[chn].stFrame.stVFrame.u32Stride[0] = VencCtx[chn].buf.width;
	VencCtx[chn].stFrame.stVFrame.u32Stride[1] = VencCtx[chn].buf.width;
	VencCtx[chn].stFrame.stVFrame.u32Stride[2] = 0;

	ySize = ALIGN(VencCtx[chn].buf.width, 32) * VencCtx[chn].buf.height;

	VencCtx[chn].stFrame.stVFrame.u64PhyAddr[0] = VencCtx[chn].buf.phy_addr;
	VencCtx[chn].stFrame.stVFrame.u64PhyAddr[1] =
		VencCtx[chn].stFrame.stVFrame.u64PhyAddr[0] + ySize;
	VencCtx[chn].stFrame.stVFrame.u64PhyAddr[2] = 0;

	VencCtx[chn].stFrame.stVFrame.pu8VirAddr[0] = VencCtx[chn].buf.vir_addr;
	VencCtx[chn].stFrame.stVFrame.pu8VirAddr[1] =
		VencCtx[chn].stFrame.stVFrame.pu8VirAddr[0] + ySize;
	VencCtx[chn].stFrame.stVFrame.pu8VirAddr[2] = 0;

	VencCtx[chn].stFrame.stVFrame.u32Length[0] = ySize;
	VencCtx[chn].stFrame.stVFrame.u32Length[1] = ySize / 2;
	VencCtx[chn].stFrame.stVFrame.u32Length[2] = 0;

	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	VencCtx[chn].stFrame.stVFrame.u64PTS = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;

	VencCtx[chn].bufEnFlag = true;

	return 0;
}

static int put_video_frame(int chn)
{
	int ret = 0;

	if (VencCtx[chn].bufEnFlag) {
		ret = put_yuv_frame(chn, MODULE_ID, &VencCtx[chn].buf);
		VencCtx[chn].bufEnFlag = false;
	}

	return ret;
}

static int venc_init(int chn, RTSP_CFG *p_rtsp_cfg)
{
	CVI_S32 ret = CVI_SUCCESS;
	ISP_PUB_ATTR_S stPubAttr;
	VENC_CHN_ATTR_S stChnAttr;
	VENC_RC_PARAM_S stRcParam;
	VENC_RECV_PIC_PARAM_S stRecvParam;
	// rtsp cfg
	GOP_MODE st_gop_mode = p_rtsp_cfg->pa_video_src_cfg[chn].st_vc_cfg.st_gop_mode;
	RC_ATTR st_rc_attr = p_rtsp_cfg->pa_video_src_cfg[chn].st_vc_cfg.st_rc_attr;
	RC_PARAM st_rc_param = p_rtsp_cfg->pa_video_src_cfg[chn].st_vc_cfg.st_rc_param;

	memset(&VencCtx[chn], 0, sizeof(__VencCtx_S));

	memset(&stPubAttr, 0, sizeof(ISP_PUB_ATTR_S));
	CVI_ISP_GetPubAttr(chn, &stPubAttr);

	if ((stPubAttr.stWndRect.u32Width % 32) != 0) {
		ISP_LOG_ERR("error, venc width must be aligned to 32...\n");
		return -1;
	}

	memset(&stChnAttr, 0, sizeof(VENC_CHN_ATTR_S));

	if (strcmp(p_rtsp_cfg->pa_video_src_cfg[chn].codec, "264") == 0) {
		stChnAttr.stVencAttr.enType = PT_H264;
	} else if (strcmp(p_rtsp_cfg->pa_video_src_cfg[chn].codec, "265") == 0) {
		stChnAttr.stVencAttr.enType = PT_H265;
	} else if (strcmp(p_rtsp_cfg->pa_video_src_cfg[chn].codec, "mjpeg") == 0)  {
		stChnAttr.stVencAttr.enType = PT_MJPEG;
	} else {
		ISP_LOG_ERR("codec: %s not support!\n", p_rtsp_cfg->pa_video_src_cfg[chn].codec);
		return -1;
	}

	stChnAttr.stVencAttr.u32MaxPicWidth = stPubAttr.stWndRect.u32Width;
	stChnAttr.stVencAttr.u32MaxPicHeight = stPubAttr.stWndRect.u32Height;
	stChnAttr.stVencAttr.u32PicWidth = stPubAttr.stWndRect.u32Width;
	stChnAttr.stVencAttr.u32PicHeight = stPubAttr.stWndRect.u32Height;
	stChnAttr.stVencAttr.u32BufSize = 2 * 1024 * 1024; // TODO
	stChnAttr.stVencAttr.u32CmdQueueDepth = 3;
	stChnAttr.stVencAttr.enEncMode = VENC_MODE_RECOMMEND;

	switch (stChnAttr.stVencAttr.enType) {
	case PT_H264:
		stChnAttr.stVencAttr.stAttrH264e.bRcnRefShareBuf = CVI_FALSE;
		stChnAttr.stVencAttr.stAttrH264e.bSingleLumaBuf = CVI_FALSE;
		stChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
		stChnAttr.stRcAttr.stH264Cbr.u32Gop = st_rc_attr.Gop;
		stChnAttr.stRcAttr.stH264Cbr.u32StatTime = st_rc_attr.StatTime;
		stChnAttr.stRcAttr.stH264Cbr.u32SrcFrameRate = st_rc_attr.SrcFrmRate;
		stChnAttr.stRcAttr.stH264Cbr.fr32DstFrameRate = st_rc_attr.DstFrmRate;
		stChnAttr.stRcAttr.stH264Cbr.u32BitRate = st_rc_attr.BitRate;
		stChnAttr.stRcAttr.stH264Cbr.bVariFpsEn = st_rc_attr.VariableFPS;
		break;
	case PT_H265:
		stChnAttr.stVencAttr.stAttrH265e.bRcnRefShareBuf = CVI_FALSE;
		stChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H265CBR;
		stChnAttr.stRcAttr.stH265Cbr.u32Gop = st_rc_attr.Gop;
		stChnAttr.stRcAttr.stH265Cbr.u32StatTime = st_rc_attr.StatTime;
		stChnAttr.stRcAttr.stH265Cbr.u32SrcFrameRate = st_rc_attr.SrcFrmRate;
		stChnAttr.stRcAttr.stH265Cbr.fr32DstFrameRate = st_rc_attr.DstFrmRate;
		stChnAttr.stRcAttr.stH265Cbr.u32BitRate = st_rc_attr.BitRate;
		stChnAttr.stRcAttr.stH265Cbr.bVariFpsEn = st_rc_attr.VariableFPS;
		break;
	case PT_MJPEG:
		stChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_MJPEGCBR;
		stChnAttr.stRcAttr.stMjpegCbr.u32StatTime = st_rc_attr.StatTime;
		stChnAttr.stRcAttr.stMjpegCbr.u32SrcFrameRate = st_rc_attr.SrcFrmRate;
		stChnAttr.stRcAttr.stMjpegCbr.fr32DstFrameRate = st_rc_attr.DstFrmRate;
		stChnAttr.stRcAttr.stMjpegCbr.u32BitRate = st_rc_attr.BitRate;
		stChnAttr.stRcAttr.stMjpegCbr.bVariFpsEn = st_rc_attr.VariableFPS;
		break;
	default:
		break;
	}

	stChnAttr.stGopAttr.enGopMode = st_gop_mode.GopMode;
	stChnAttr.stGopAttr.stNormalP.s32IPQpDelta = st_gop_mode.IPQpDelta;
	//stChnAttr.stGopExAttr.u32GopPreset = GOP_PRESET_IDX_IPPPP;
	stChnAttr.stGopExAttr.u32GopPreset = GOP_PRESET_IDX_IPP_SINGLE;

	ret = CVI_VENC_CreateChn(chn, &stChnAttr);
	if (ret != CVI_SUCCESS) {
		ISP_LOG_ERR("CVI_VENC_CreateChn FAIL: 0x%x\n", ret);
		return -1;
	}

	ret =  CVI_VENC_GetRcParam(chn, &stRcParam);
	if (ret != CVI_SUCCESS) {
		ISP_LOG_ERR("CVI_VENC_GetRcParam FAIL: 0x%x\n", ret);
		return -1;
	}

	stRcParam.s32FirstFrameStartQp = st_rc_param.FirstFrameStartQp;
	stRcParam.s32InitialDelay = st_rc_param.InitialDelay;
	stRcParam.u32ThrdLv = st_rc_param.ThrdLv;

	switch (stChnAttr.stVencAttr.enType) {
	case PT_H264:
		stRcParam.stParamH264Cbr.bQpMapEn = CVI_FALSE;
		stRcParam.stParamH264Cbr.u32MaxIprop = CVI_H26X_MAX_I_PROP_MAX;
		stRcParam.stParamH264Cbr.u32MinIprop = CVI_H26X_MAX_I_PROP_MIN;
		stRcParam.stParamH264Cbr.u32MaxIQp = st_rc_param.MaxIQp;
		stRcParam.stParamH264Cbr.u32MinIQp = st_rc_param.MinIQp;
		stRcParam.stParamH264Cbr.u32MaxQp = st_rc_param.MaxQp;
		stRcParam.stParamH264Cbr.u32MinQp = st_rc_param.MinQp;
		break;
	case PT_H265:
		stRcParam.stParamH265Cbr.bQpMapEn = CVI_FALSE;
		// stRcParam.stParamH265Cbr.s32MaxReEncodeTimes = attr.s32MaxReEncodeTimes;
		stRcParam.stParamH265Cbr.u32MaxIprop = CVI_H26X_MAX_I_PROP_MAX;
		stRcParam.stParamH265Cbr.u32MinIprop = CVI_H26X_MAX_I_PROP_MIN;
		stRcParam.stParamH265Cbr.u32MaxIQp = st_rc_param.MaxIQp;
		stRcParam.stParamH265Cbr.u32MinIQp = st_rc_param.MinIQp;
		stRcParam.stParamH265Cbr.u32MaxQp = st_rc_param.MaxQp;
		stRcParam.stParamH265Cbr.u32MinQp = st_rc_param.MinQp;
		break;
	case PT_MJPEG:
		// TODO
		break;
	default:
		break;
	}

	ret = CVI_VENC_SetRcParam(chn, &stRcParam);
	if (ret != CVI_SUCCESS) {
		ISP_LOG_ERR("CVI_VENC_SetRcParam FAIL: 0x%x\n", ret);
		return -1;
	}

	memset(&stRecvParam, 0, sizeof(VENC_RECV_PIC_PARAM_S));

	stRecvParam.s32RecvPicNum = -1;
	ret = CVI_VENC_StartRecvFrame(chn, &stRecvParam);
	if (ret != CVI_SUCCESS) {
		ISP_LOG_ERR("CVI_VENC_StartRecvFrame FAIL: 0x%x\n", ret);
		return -1;
	}

	return 0;
}

static int venc_deinit(int chn, RTSP_CFG *p_rtsp_cfg)
{
	UNUSED(p_rtsp_cfg);
	CVI_S32 ret = CVI_SUCCESS;

	if (get_video_frame(chn) < 0) {
		ISP_LOG_ERR("venc chn %d, venc deinit get video frame fail...\n", chn);
	}

	VencCtx[chn].stFrame.stVFrame.bSrcEnd = CVI_TRUE;

	int send_fail_count = 1;

send_again:
	ret = CVI_VENC_SendFrame(chn, &VencCtx[chn].stFrame, SEND_FRAME_MILLSEC);
	if (ret != CVI_SUCCESS) {
		ISP_LOG_ERR("venc chn %d, send frame fail times: %d\n", chn, send_fail_count);
		usleep(1000 * 1000);
		send_fail_count++;
		if (send_fail_count < 10) {
			goto send_again;
		}
	}

	put_video_frame(chn);

	ret = CVI_VENC_StopRecvFrame(chn);
	if (ret != CVI_SUCCESS) {
		ISP_LOG_ERR("venc chn: %d, stop recv frame fail...\n", chn);
	}

	ret = CVI_VENC_DestroyChn(chn);
	if (ret != CVI_SUCCESS) {
		ISP_LOG_ERR("venc chn: %d, destroy chn fail...\n", chn);
	}

	return ret;
}

int start_venc(RTSP_CFG *p_rtsp_cfg)
{
	int num = p_rtsp_cfg->dev_num;

	if (num >= VI_MAX_PIPE_NUM) {
		return -1;
	}

	for (int i = 0; i < num; i++) {
		venc_init(i, p_rtsp_cfg);
	}

	return 0;
}

int stop_venc(RTSP_CFG *p_rtsp_cfg)
{
	int num = p_rtsp_cfg->dev_num;

	if (num >= VI_MAX_PIPE_NUM) {
		return -1;
	}

	for (int i = 0; i < num; i++) {
		venc_deinit(i, p_rtsp_cfg);
	}

	return 0;
}

int get_stream(int chn, void *pstream)
{
	if (chn >= VI_MAX_PIPE_NUM) {
		return -1;
	}

	CVI_S32 ret = CVI_SUCCESS;
	VENC_CHN_STATUS_S stStat;
	VENC_STREAM_S *pstStream = (VENC_STREAM_S *) pstream;

	if (get_video_frame(chn) < 0) {
		ISP_LOG_ERR("venc chn %d, get video frame fail...\n", chn);
		return -1;
	}

	int send_fail_count = 1;

send_again:
	ret = CVI_VENC_SendFrame(chn, &VencCtx[chn].stFrame, SEND_FRAME_MILLSEC);
	if (ret != CVI_SUCCESS) {
		ISP_LOG_ERR("venc chn %d, send frame fail times: %d\n", chn, send_fail_count);
		usleep(500 * 1000);
		send_fail_count++;
		if (send_fail_count < 10) {
			goto send_again;
		} else {
			return -1;
		}
	}

	memset(&stStat, 0, sizeof(VENC_CHN_STATUS_S));
	ret = CVI_VENC_QueryStatus(chn, &stStat);
	if (ret != CVI_SUCCESS) {
		ISP_LOG_ERR("venc chn: %d, query status fail...\n", chn);
		return -1;
	}

	if (!stStat.u32CurPacks) {
		ISP_LOG_ERR("venc chn: %d, u32CurPacks == 0, try again...\n", chn);
		return -1;
	}

	if (pstStream->pstPack != NULL) {
		free(pstStream->pstPack);
		pstStream->pstPack = NULL;
	}

	memset(pstStream, 0, sizeof(VENC_STREAM_S));
	pstStream->pstPack = (VENC_PACK_S *) malloc(sizeof(VENC_PACK_S) * stStat.u32CurPacks);

	int get_fail_count = 1;

get_again:
	ret = CVI_VENC_GetStream(chn, pstStream, GET_FRAME_MILLSEC);
	if (ret != CVI_SUCCESS) {
		ISP_LOG_ERR("venc chn: %d, get stream fail times: %d\n", chn, get_fail_count);
		usleep(500 * 1000);
		get_fail_count++;
		if (get_fail_count < 10) {
			goto get_again;
		} else {
			return -1;
		}
	}

	return 0;
}

int put_stream(int chn, void *pstream)
{
	if (chn >= VI_MAX_PIPE_NUM) {
		return -1;
	}

	CVI_S32 ret = CVI_SUCCESS;
	VENC_STREAM_S *pstStream = (VENC_STREAM_S *) pstream;

	ret = CVI_VENC_ReleaseStream(chn, pstStream);
	if (ret != CVI_SUCCESS) {
		ISP_LOG_ERR("venc chn: %d, release stream fail...\n", chn);
	}

	if (pstStream->pstPack != NULL) {
		free(pstStream->pstPack);
		pstStream->pstPack = NULL;
	}

	put_video_frame(chn);

	return ret;
}

