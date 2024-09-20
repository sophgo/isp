
#ifndef __VI_H__
#define __VI_H__

#include <stdint.h>
#include "utils.h"

#define V4L2_REQ_BUFFER_NUM  (6)

typedef struct {
	int64_t id;
	int width;
	int height;
	void *vir_addr;
	uint64_t phy_addr;
	size_t length;
	int dma_buf_fd;
	void *v4l2_buffer;
} VideoBuffer;

#define VENC_MODULE_ID    (1 << 0)
#define VO_MODULE_ID      (1 << 1)
#define MODULE_COUNT_MAX  (2)

int start_vi(RTSP_CFG *p_rtsp_cfg);
int stop_vi(RTSP_CFG *p_rtsp_cfg);

int get_yuv_frame(int pipe, int moduleId, VideoBuffer *pbuf);
int put_yuv_frame(int pipe, int moduleId, VideoBuffer *pbuf);

#endif

