
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>

#include "vi.h"
#include "venc.h"
#include "rtsp.h"

#include "cvi_rtsp/rtsp.h"
#include "cvi_isp.h"
#include "clog.h"
#include "isp_debug.h"
#include "cvi_venc.h"

#define UNUSED(x) ((void)(x))

typedef struct {
	int rtspThreadEnable;
	pthread_t tid;
	CVI_RTSP_SESSION *pSession;
} __RtspCtx_S;

static CVI_RTSP_CTX *serverCtx;
static __RtspCtx_S RtspCtx[VI_MAX_PIPE_NUM];

static void rtsp_connect(const char *ip, void *arg)
{
	(void) arg;
	printf("rtsp connect: %s\n", ip);
}

static void disconnect(const char *ip, void *arg)
{
	(void) arg;
	printf("rtsp disconnect: %s\n", ip);
}

static int rtsp_session_setup(int chn, int type)
{
	int ret = 0;
	CVI_RTSP_SESSION_ATTR attr;

	memset(&attr, 0, sizeof(CVI_RTSP_SESSION_ATTR));

	attr.video.codec = (CVI_RTSP_VIDEO_CODEC)type;
	sprintf(attr.name, "stream%d", chn);

	ret = CVI_RTSP_CreateSession(serverCtx, &attr, &RtspCtx[chn].pSession);
	if (ret != 0) {
		ISP_LOG_ERR("CVI_RTSP_CreateSession failed with %#x\n", ret);
		return -1;
	}

	return 0;
}

int send_to_rtsp(int chn, VENC_STREAM_S *pstStream)
{
	CVI_S32 ret = CVI_SUCCESS;
	VENC_PACK_S *ppack;
	CVI_RTSP_DATA data;

	if (pstStream->u32PackCount >= CVI_RTSP_DATA_MAX_BLOCK) {
		ISP_LOG_ERR("out of max block count, %d\n", pstStream->u32PackCount);
		return -1;
	}

	memset(&data, 0, sizeof(CVI_RTSP_DATA));

	data.blockCnt = pstStream->u32PackCount;
	for (CVI_U32 i = 0; i < pstStream->u32PackCount; i++) {
		ppack = &pstStream->pstPack[i];
		data.dataPtr[i] = ppack->pu8Addr + ppack->u32Offset;
		data.dataLen[i] = ppack->u32Len - ppack->u32Offset;
	}

#define __DUMP_RTSP_FRAME_MAX  (500)

	static FILE *fp = NULL;
	static int dump_count = 0;

	// TODO dump rtsp support multi chn
	if (access("/tmp/isp_dump_rtsp", F_OK) == 0) {

		if (fp == NULL) {
			fp = fopen("/tmp/isp_dump_rtsp.h265", "wb"); // TODO
			dump_count = 0;
		}

		dump_count++;

		if (dump_count > __DUMP_RTSP_FRAME_MAX) {
			fclose(fp);
			fp = NULL;
			dump_count = 0;
			system("rm /tmp/isp_dump_rtsp");
			printf("isp dump rtsp success, saved in /tmp/isp_dump_rtsp.h265\n");
		}
	}

	if (fp != NULL) {
		for (CVI_U32 i = 0; i < pstStream->u32PackCount; i++) {
			ppack = &pstStream->pstPack[i];
			fwrite(ppack->pu8Addr + ppack->u32Offset,
				ppack->u32Len - ppack->u32Offset, 1, fp);
		}
	}

	ret = CVI_RTSP_WriteFrame(serverCtx, RtspCtx[chn].pSession->video, &data);
	if (ret != CVI_SUCCESS) {
		ISP_LOG_ERR("CVI_RTSP_WriteFrame failed\n");
		return -1;
	}

	return 0;
}

static void *rtsp_thread(void *param)
{
	int chn = (int) (uint64_t) param;

	CVI_S32 ret = CVI_SUCCESS;
	VENC_CHN_ATTR_S stChnAttr;

	memset(&stChnAttr, 0, sizeof(stChnAttr));
	ret = CVI_VENC_GetChnAttr(chn, &stChnAttr);
	if (ret != CVI_SUCCESS) {
		ISP_LOG_ERR("rtsp thread get venc chan attr fail...%d\n", chn);
		return NULL;
	}

	printf("start rtsp thread, chn: %d, %d\n", chn, stChnAttr.stVencAttr.enType);

	switch (stChnAttr.stVencAttr.enType) {
	case PT_H264:
		ret = rtsp_session_setup(chn, RTSP_VIDEO_H264);
		break;

	case PT_H265:
		ret = rtsp_session_setup(chn, RTSP_VIDEO_H265);
		break;
	default:
		ISP_LOG_ERR("type:%d not support\n", stChnAttr.stVencAttr.enType);
		return NULL;
	}

	if (ret < 0) {
		return NULL;
	}

	VENC_STREAM_S stStream;

	memset(&stStream, 0, sizeof(VENC_STREAM_S));

	while (RtspCtx[chn].rtspThreadEnable) {

		if (get_stream(chn, &stStream) < 0) {
			continue;
		}

		send_to_rtsp(chn, &stStream);

		put_stream(chn, &stStream);
	}

	CVI_RTSP_DestroySession(serverCtx, RtspCtx[chn].pSession);

	return NULL;
}

static int rtsp_start_session(int chn)
{
	memset(&RtspCtx[chn], 0, sizeof(__RtspCtx_S));

	RtspCtx[chn].rtspThreadEnable = 1;
	pthread_create(&RtspCtx[chn].tid, NULL, rtsp_thread, (void *) (uint64_t) chn);

	return 0;
}

static int rtsp_stop_session(int chn)
{
	RtspCtx[chn].rtspThreadEnable = 0;
	pthread_join(RtspCtx[chn].tid, NULL);

	return 0;
}

int start_rtsp(RTSP_CFG *p_rtsp_cfg)
{
	int num = p_rtsp_cfg->dev_num;

	if (num > VI_MAX_PIPE_NUM || num < 1) {
		return -1;
	}

	CVI_RTSP_CONFIG config;

	memset(&config, 0, sizeof(CVI_RTSP_CONFIG));

	config.port = p_rtsp_cfg->rtsp_port;

	if (CVI_RTSP_Create(&serverCtx, &config) < 0) {
		ISP_LOG_ERR("create rtsp server fail...\n");
		return -1;
	}

	CVI_RTSP_SetOutPckBuf_MaxSize(p_rtsp_cfg->rtsp_max_buf_size);

	CVI_RTSP_STATE_LISTENER listener;

	memset(&listener, 0, sizeof(CVI_RTSP_STATE_LISTENER));

	listener.onConnect = rtsp_connect;
	listener.argConn = serverCtx;
	listener.onDisconnect = disconnect;
	CVI_RTSP_SetListener(serverCtx, &listener);

	if (CVI_RTSP_Start(serverCtx) < 0) {
		ISP_LOG_ERR("fail to rtsp start\n");
		return -1;
	}

	for (int i = 0; i < num; i++) {
		rtsp_start_session(i);
	}

	return 0;
}

int stop_rtsp(RTSP_CFG *p_rtsp_cfg)
{
	int num = p_rtsp_cfg->dev_num;

	if (num > VI_MAX_PIPE_NUM) {
		return -1;
	}

	for (int i = 0; i < num; i++) {
		rtsp_stop_session(i);
	}

	CVI_RTSP_Stop(serverCtx);

	CVI_RTSP_Destroy(&serverCtx);

	return 0;
}

