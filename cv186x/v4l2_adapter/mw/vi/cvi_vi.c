#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/select.h>
#include <inttypes.h>

#include "cvi_buffer.h"
#include "cvi_base.h"
#include "cvi_vi.h"
#include "cvi_vb.h"
#include "cvi_sys.h"
#include "vi_ioctl.h"
#include "cvi_sns_ctrl.h"
#include "cvi_isp_v4l2.h"
#include <linux/cvi_vi_ctx.h>

#define UNUSED(x) ((void)(x))

extern int get_v4l2_fd(int pipe);
extern CVI_S32 dump_register(VI_PIPE ViPipe, FILE *fp, VI_DUMP_REGISTER_TABLE_S *pstRegTbl);
CVI_VOID (*getDisInfoCallback)(struct dis_info *pDisInfo);

/* dpc */
CVI_VOID CVI_VI_SetMotionLV(struct mlv_info mlevel_i)
{
	UNUSED(mlevel_i);
}

CVI_S32 CVI_VI_GetPipeFrame(VI_PIPE ViPipe, VIDEO_FRAME_INFO_S *pstFrameInfo, CVI_S32 s32MilliSec)
{
	CVI_S32 s32Ret = CVI_SUCCESS;

	if (ViPipe < 0 || ViPipe > VI_MAX_PIPE_NUM - 1) {
		CVI_TRACE_VI(CVI_DBG_ERR, " invalid dev-id(%d)\n", ViPipe);
		return CVI_ERR_VI_INVALID_DEVID;
	}

	int fd = get_v4l2_fd(ViPipe);

	if (fd < 0) {
		CVI_TRACE_VI(CVI_DBG_ERR, "get_vi_fd open failed\n");
		return CVI_ERR_VI_BUSY;
	}

	s32Ret = vi_sdk_get_pipe_frame(fd, ViPipe, pstFrameInfo, s32MilliSec);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_VI(CVI_DBG_ERR, "vi_sdk_get_pipe_frame ioctl failed. errno 0x%x\n", s32Ret);
		return s32Ret;
	}

	return CVI_SUCCESS;
}

CVI_S32 CVI_VI_ReleasePipeFrame(VI_PIPE ViPipe, const VIDEO_FRAME_INFO_S *pstFrameInfo)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	VIDEO_FRAME_INFO_S stFrameInfo;

	if (ViPipe < 0 || ViPipe > VI_MAX_PIPE_NUM - 1) {
		CVI_TRACE_VI(CVI_DBG_ERR, " invalid dev-id(%d)\n", ViPipe);
		return CVI_ERR_VI_INVALID_DEVID;
	}

	int fd = get_v4l2_fd(ViPipe);

	if (fd < 0) {
		CVI_TRACE_VI(CVI_DBG_ERR, "get_vi_fd open failed\n");
		return CVI_ERR_VI_BUSY;
	}

	stFrameInfo = *pstFrameInfo;
	s32Ret = vi_sdk_release_pipe_frame(fd, ViPipe, &stFrameInfo);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_VI(CVI_DBG_ERR, "vi_sdk_release_pipe_frame ioctl failed. errno 0x%x\n", s32Ret);
		return s32Ret;
	}

	return CVI_SUCCESS;
}

CVI_VOID CVI_ISP_SetDISInfoCallback(CVI_VOID *pcallbackFunc)
{
	if (pcallbackFunc) {
		getDisInfoCallback = pcallbackFunc;
	}
}

/* dis */
CVI_VOID CVI_VI_SET_DIS_INFO(struct dis_info dis_i)
{
	if (getDisInfoCallback) {
		getDisInfoCallback(&dis_i);
	}
}

CVI_S32 CVI_VI_SetPipeDumpAttr(VI_PIPE ViPipe, const VI_DUMP_ATTR_S *pstDumpAttr)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	VI_DUMP_ATTR_S stDumpAttr;

	if (ViPipe < 0 || ViPipe > VI_MAX_PIPE_NUM - 1) {
		CVI_TRACE_VI(CVI_DBG_ERR, " invalid dev-id(%d)\n", ViPipe);
		return CVI_ERR_VI_INVALID_DEVID;
	}

	int fd = get_v4l2_fd(ViPipe);

	if (fd < 0) {
		CVI_TRACE_VI(CVI_DBG_ERR, "get_vi_fd open failed\n");
		return CVI_ERR_VI_BUSY;
	}

	stDumpAttr = *pstDumpAttr;
	s32Ret = vi_sdk_set_pipe_dump_attr(fd, ViPipe, &stDumpAttr);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_VI(CVI_DBG_ERR, "vi_sdk_set_pipe_dump_attr ioctl failed. errno 0x%x\n", s32Ret);
		return s32Ret;
	}

	return CVI_SUCCESS;
}

CVI_S32 CVI_VI_GetPipeDumpAttr(VI_PIPE ViPipe, VI_DUMP_ATTR_S *pstDumpAttr)
{
	UNUSED(ViPipe);
	UNUSED(pstDumpAttr);
	return 0;
}

/* freeze */
CVI_S32 CVI_VI_SetBypassFrm(CVI_U32 snr_num, CVI_U8 bypass_num)
{

	CVI_S32 s32Ret = CVI_SUCCESS;

	if (snr_num > VI_MAX_PIPE_NUM - 1) {
		CVI_TRACE_VI(CVI_DBG_ERR, " invalid dev-id(%d)\n", snr_num);
		return CVI_ERR_VI_INVALID_DEVID;
	}

	int fd = get_v4l2_fd(snr_num);

	s32Ret = vi_sdk_set_bypass_frm(fd, snr_num, bypass_num);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_VI(CVI_DBG_ERR, "vi_sdk_set_bypass_frm ioctl failed errno 0x%x\n", s32Ret);
		return s32Ret;
	}

	return CVI_SUCCESS;
}

CVI_S32 CVI_VI_GetPipeAttr(VI_PIPE ViPipe, VI_PIPE_ATTR_S *pstPipeAttr)
{
	CVI_S32 s32Ret = CVI_SUCCESS;

	if (ViPipe > VI_MAX_PIPE_NUM - 1) {
		CVI_TRACE_VI(CVI_DBG_ERR, " invalid dev-id(%d)\n", ViPipe);
		return CVI_ERR_VI_INVALID_DEVID;
	}

	int fd = get_v4l2_fd(ViPipe);

	if (fd < 0) {
		CVI_TRACE_VI(CVI_DBG_ERR, "get_vi_fd open failed\n");
		return CVI_ERR_VI_BUSY;
	}

	s32Ret = vi_sdk_get_pipe_attr(fd, ViPipe, pstPipeAttr);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_VI(CVI_DBG_ERR, "vi_sdk_get_pipe_attr ioctl failed. errno 0x%x\n", s32Ret);
		return s32Ret;
	}

	return CVI_SUCCESS;
}

/* others */
CVI_S32 CVI_VI_GetDevNum(CVI_U32 *devNum)
{
	int working_max_pipe = 0;
	int v4l2_fd = -1;

	for (int pipe = 0; pipe < VI_MAX_PIPE_NUM; ++pipe) {
		if (CVI_ISP_V4L2_GetFd(pipe, &v4l2_fd) == 0) {
			working_max_pipe = pipe;
		}
	}

	*devNum = working_max_pipe + 1;

	return 0;
}

CVI_S32 CVI_VI_SetDevAttr(VI_DEV ViDev, const VI_DEV_ATTR_S *pstDevAttr)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	int ViPipe = ViDev;
	VI_DEV_ATTR_S devAttr = *pstDevAttr;

	if (ViPipe > VI_MAX_PIPE_NUM - 1) {
		CVI_TRACE_VI(CVI_DBG_ERR, " invalid dev-id(%d)\n", ViPipe);
		return CVI_ERR_VI_INVALID_DEVID;
	}

	int fd = get_v4l2_fd(ViPipe);

	if (fd < 0) {
		CVI_TRACE_VI(CVI_DBG_ERR, "get_vi_fd open failed\n");
		return CVI_ERR_VI_BUSY;
	}

	s32Ret = vi_sdk_set_dev_attr(fd, ViPipe, &devAttr);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_VI(CVI_DBG_ERR, "vi_sdk_set_dev_attr ioctl failed. errno 0x%x\n", s32Ret);
		return s32Ret;
	}

	return CVI_SUCCESS;
}

CVI_S32 CVI_VI_QueryDevStatus(VI_PIPE ViPipe)
{
	CVI_S32 v4l2_fd = -1;
	CVI_S32 s32Ret = CVI_FAILURE;

	if (ViPipe > VI_MAX_PIPE_NUM - 1) {
		CVI_TRACE_VI(CVI_DBG_ERR, " invalid dev-id(%d)\n", ViPipe);
		return CVI_ERR_VI_INVALID_DEVID;
	}

	CVI_ISP_V4L2_GetFd(ViPipe, &v4l2_fd);
	if (v4l2_fd > 0) {
		s32Ret = CVI_SUCCESS;
	}

	return s32Ret;
}

CVI_S32 CVI_VI_GetDevAttr(VI_DEV ViDev, VI_DEV_ATTR_S *pstDevAttr)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	int ViPipe = ViDev;

	if (ViPipe > VI_MAX_PIPE_NUM - 1) {
		CVI_TRACE_VI(CVI_DBG_ERR, " invalid dev-id(%d)\n", ViPipe);
		return CVI_ERR_VI_INVALID_DEVID;
	}

	int fd = get_v4l2_fd(ViPipe);

	if (fd < 0) {
		CVI_TRACE_VI(CVI_DBG_ERR, "get_vi_fd open failed\n");
		return CVI_ERR_VI_BUSY;
	}

	s32Ret = vi_sdk_get_dev_attr(fd, ViPipe, pstDevAttr);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_VI(CVI_DBG_ERR, "vi_sdk_get_dev_attr ioctl failed. errno 0x%x\n", s32Ret);
		return s32Ret;
	}

	return CVI_SUCCESS;
}

CVI_S32 CVI_VI_SetDevTimingAttr(VI_DEV ViDev, const VI_DEV_TIMING_ATTR_S *pstTimingAttr)
{
	UNUSED(ViDev);
	UNUSED(pstTimingAttr);
	return 0;
}

CVI_S32 CVI_VI_CreatePipe(VI_PIPE ViPipe, const VI_PIPE_ATTR_S *pstPipeAttr)
{
	UNUSED(ViPipe);
	UNUSED(pstPipeAttr);
	return 0;
}

CVI_S32 CVI_VI_StartPipe(VI_PIPE ViPipe)
{
	UNUSED(ViPipe);
	return 0;
}

CVI_S32 CVI_VI_StartSmoothRawDump(const VI_SMOOTH_RAW_DUMP_INFO_S *pstDumpInfo)
{
	UNUSED(pstDumpInfo);
	return 0;
}

CVI_S32 CVI_VI_StopSmoothRawDump(const VI_SMOOTH_RAW_DUMP_INFO_S *pstDumpInfo)
{
	UNUSED(pstDumpInfo);
	return 0;
}

CVI_S32 CVI_VI_GetSmoothRawDump(VI_PIPE ViPipe, VIDEO_FRAME_INFO_S *pstVideoFrame, CVI_S32 s32MilliSec)
{
	UNUSED(ViPipe);
	UNUSED(pstVideoFrame);
	UNUSED(s32MilliSec);
	return 0;
}

CVI_S32 CVI_VI_PutSmoothRawDump(VI_PIPE ViPipe, const VIDEO_FRAME_INFO_S *pstVideoFrame)
{
	UNUSED(ViPipe);
	UNUSED(pstVideoFrame);
	return 0;
}

CVI_S32 CVI_VI_SetPipeFrameSource(VI_PIPE ViPipe, const VI_PIPE_FRAME_SOURCE_E enSource)
{
	UNUSED(ViPipe);
	UNUSED(enSource);
	return 0;
}

CVI_S32 CVI_VI_GetPipeFrameSource(VI_PIPE ViPipe, VI_PIPE_FRAME_SOURCE_E *penSource)
{
	UNUSED(ViPipe);
	UNUSED(penSource);
	return 0;
}

CVI_S32 CVI_VI_SendPipeRaw(CVI_U32 u32PipeNum, VI_PIPE PipeId[], const VIDEO_FRAME_INFO_S *pstVideoFrame[],
			   CVI_S32 s32MilliSec)
{
	UNUSED(u32PipeNum);
	UNUSED(PipeId);
	UNUSED(pstVideoFrame);
	UNUSED(s32MilliSec);
	return 0;
}

CVI_S32 CVI_VI_CloseFd(void)
{
	return CVI_SUCCESS;
}

CVI_S32 CVI_VI_GetChnAttr(VI_PIPE ViPipe, VI_CHN ViChn, VI_CHN_ATTR_S *pstChnAttr)
{
	UNUSED(ViPipe);
	UNUSED(ViChn);
	UNUSED(pstChnAttr);
	return 0;
}

CVI_S32 CVI_VI_GetChnCrop(VI_PIPE ViPipe, VI_CHN ViChn, VI_CROP_INFO_S  *pstCropInfo)
{
	UNUSED(ViPipe);
	UNUSED(ViChn);
	pstCropInfo->bEnable = CVI_FALSE;
	return 0;
}

CVI_S32 CVI_VI_GetChnFrame(VI_PIPE ViPipe, VI_CHN ViChn, VIDEO_FRAME_INFO_S *pstFrameInfo, CVI_S32 s32MilliSec)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	ViPipe = ViChn;

	if (ViPipe < 0 || ViPipe > VI_MAX_PIPE_NUM - 1) {
		CVI_TRACE_VI(CVI_DBG_ERR, " invalid dev-id(%d)\n", ViPipe);
		return CVI_ERR_VI_INVALID_DEVID;
	}

	int fd = get_v4l2_fd(ViPipe);

	if (fd < 0) {
		CVI_TRACE_VI(CVI_DBG_ERR, "get_vi_fd open failed\n");
		return CVI_ERR_VI_BUSY;
	}

	s32Ret = vi_sdk_get_chn_frame(fd, ViPipe, ViChn, pstFrameInfo, s32MilliSec);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_VI(CVI_DBG_ERR, "vi_sdk_get_chn_frame ioctl failed. errno 0x%x\n", s32Ret);
		return s32Ret;
	}

	return CVI_SUCCESS;
}

CVI_S32 CVI_VI_ReleaseChnFrame(VI_PIPE ViPipe, VI_CHN ViChn, const VIDEO_FRAME_INFO_S *pstFrameInfo)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	VIDEO_FRAME_INFO_S stFrameInfo;
	ViPipe = ViChn;

	if (ViPipe < 0 || ViPipe > VI_MAX_PIPE_NUM - 1) {
		CVI_TRACE_VI(CVI_DBG_ERR, " invalid dev-id(%d)\n", ViPipe);
		return CVI_ERR_VI_INVALID_DEVID;
	}

	int fd = get_v4l2_fd(ViPipe);

	if (fd < 0) {
		CVI_TRACE_VI(CVI_DBG_ERR, "get_vi_fd open failed\n");
		return CVI_ERR_VI_BUSY;
	}

	stFrameInfo = *pstFrameInfo;
	s32Ret = vi_sdk_release_chn_frame(fd, ViPipe, ViChn, &stFrameInfo);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_VI(CVI_DBG_ERR, "vi_sdk_release_chn_frame ioctl failed. errno 0x%x\n", s32Ret);
		return s32Ret;
	}

	return CVI_SUCCESS;
}

CVI_S32 CVI_VI_SetChnLDCAttr(VI_PIPE ViPipe, VI_CHN ViChn, const VI_LDC_ATTR_S *pstLDCAttr)
{
	UNUSED(ViPipe);
	UNUSED(ViChn);
	UNUSED(pstLDCAttr);
	return 0;
}

CVI_S32 CVI_VI_GetChnLDCAttr(VI_PIPE ViPipe, VI_CHN ViChn, VI_LDC_ATTR_S *pstLDCAttr)
{
	UNUSED(ViPipe);
	UNUSED(ViChn);
	UNUSED(pstLDCAttr);
	return 0;
}

CVI_S32 CVI_VI_DumpHwRegisterToFile(VI_PIPE ViPipe, FILE *fp, VI_DUMP_REGISTER_TABLE_S *pstRegTbl)
{
	CVI_S32 s32Ret = CVI_SUCCESS;

	if (ViPipe < 0 || ViPipe > VI_MAX_PIPE_NUM - 1) {
		CVI_TRACE_VI(CVI_DBG_ERR, " invalid dev-id(%d)\n", ViPipe);
		return CVI_ERR_VI_INVALID_DEVID;
	}

	if (fp == NULL || pstRegTbl == NULL) {
		CVI_TRACE_VI(CVI_DBG_ERR, " Invalid null pointer\n");
		return CVI_ERR_VI_INVALID_NULL_PTR;
	}

	s32Ret = dump_register(ViPipe, fp, pstRegTbl);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_VI(CVI_DBG_ERR, "dump_register fail\n");
		return s32Ret;
	}

	return CVI_SUCCESS;
}
