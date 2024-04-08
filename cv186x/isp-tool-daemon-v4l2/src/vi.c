
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <dlfcn.h>

#include "cvi_isp.h"
#include "cvi_base.h"
#include "cvi_sys.h"
#include "clog.h"
#include "isp_debug.h"

#include "vi.h"
#include "cvi_isp_v4l2.h"


#include "bmcv_api_ext_c.h"

#ifndef UNUSED
#define UNUSED(x) ((void)(x))
#endif

//#ifdef ISP_LOG_ERR
//#undef ISP_LOG_ERR
//#define ISP_LOG_ERR printf
//#endif
//
//#ifdef ISP_LOG_INFO
//#undef ISP_LOG_INFO
//#define ISP_LOG_INFO printf
//#endif

#define REQ_BUFFER_NUM V4L2_REQ_BUFFER_NUM
#define VBUF_LIST_MAX (MODULE_COUNT_MAX + 1)
#define TPU_DEV_NUM_MAX (2)

#define MAX_IMAGE_CHANNEL 4
typedef struct {
	bm_handle_t     handle;
	bm_device_mem_t data[MAX_IMAGE_CHANNEL];
}DIS_PRIVATE;

typedef struct {
	unsigned char bcrop;
	struct dis_info info;
	uint64_t frame_phy_addr;
	bm_image dst_image;
}DIS_INFO;

static DIS_INFO g_disInfo[VI_MAX_PIPE_NUM];

typedef struct {
	VideoBuffer *pvbf;
	int moduleId;
	int ref;
} __Vbuf_S;

typedef struct {
	int vi_fd;
	int64_t curr_index;
	struct v4l2_buffer vbuf[REQ_BUFFER_NUM];
	VideoBuffer framebuf[REQ_BUFFER_NUM];
	__Vbuf_S vbufList[VBUF_LIST_MAX];
	pthread_mutex_t lock;
} __ViCtx_S;

static __ViCtx_S ViCtx[VI_MAX_PIPE_NUM];

static void get_dis_info(struct dis_info *dis_i)
{
	unsigned char min_pipe = 0;
	if (dis_i) {
		if (dis_i->sensor_num >= min_pipe && dis_i->sensor_num < VI_MAX_PIPE_NUM) {
			memcpy(&g_disInfo[dis_i->sensor_num].info, dis_i, sizeof(struct dis_info));
		}
	}
}

static int dis_info_proc(int pipe, VideoBuffer *pbuf)
{
	bm_handle_t handle;
	bm_image src;
	bmcv_rect_t rect;
	bm_device_mem_t dev_mem[MAX_IMAGE_CHANNEL];
	ISP_DIS_ATTR_S stDisAttr;
	unsigned char delayCnt = 0;

	unsigned int image_w = pbuf->width;
	unsigned int image_h = pbuf->height;
	CVI_ISP_GetDisAttr(pipe, &stDisAttr);
	struct v4l2_buffer *pv4l2 = pbuf->v4l2_buffer;

	if (stDisAttr.enable || stDisAttr.stillCrop) {
		while (g_disInfo[pipe].info.frm_num < pv4l2->sequence) {
			if (delayCnt++ > 4) break;
			usleep(5 * 1000);
		}

		rect.start_x = g_disInfo[pipe].info.dis_i.start_x;
		rect.start_y = g_disInfo[pipe].info.dis_i.start_y;
		rect.crop_w = g_disInfo[pipe].info.dis_i.end_x - g_disInfo[pipe].info.dis_i.start_x;
		rect.crop_h = g_disInfo[pipe].info.dis_i.end_y - g_disInfo[pipe].info.dis_i.start_y;

		g_disInfo[pipe].bcrop = 1;
		bm_dev_request(&handle, 0);
		bm_image_create(handle, image_h, image_w, FORMAT_NV21, DATA_TYPE_EXT_1N_BYTE, &src, NULL);
		bm_image_alloc_dev_mem(src, 1);
		g_disInfo[pipe].frame_phy_addr = pbuf->phy_addr;

		DIS_PRIVATE *pimg = (DIS_PRIVATE*)src.image_private;
		pimg->data[0].u.device.device_addr = pbuf->phy_addr;
		pimg->data[1].u.device.device_addr = pbuf->phy_addr + pimg->data[0].size;

		bm_image_create(handle, image_h, image_w, FORMAT_NV21, DATA_TYPE_EXT_1N_BYTE,
						&g_disInfo[pipe].dst_image, NULL);
		bm_image_alloc_dev_mem(g_disInfo[pipe].dst_image, 1);
		bmcv_image_vpp_convert(handle, 1, src, &g_disInfo[pipe].dst_image, &rect, BMCV_INTER_LINEAR);
		bm_image_get_device_mem(g_disInfo[pipe].dst_image,dev_mem);
		pbuf->phy_addr = dev_mem[0].u.device.device_addr;

		bm_image_destroy(&src);
		bm_dev_free(handle);
	}

	return 0;
}

static int destroy_dis_image(int pipe, VideoBuffer *pbuf)
{
	if (g_disInfo[pipe].bcrop == 1) {
		pbuf->phy_addr = g_disInfo[pipe].frame_phy_addr;
		bm_image_destroy(&g_disInfo[pipe].dst_image);
		g_disInfo[pipe].bcrop = 0;
	}

	return 0;
}

static int request_buffer(int pipe)
{
	struct v4l2_format format;
	struct v4l2_requestbuffers reqbuf;
	int fd = ViCtx[pipe].vi_fd;
	int i, width, height;

	format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl(fd, VIDIOC_G_FMT, &format) < 0) {
		ISP_LOG_ERR("get format fail!\n");
		return -1;
	}

	width = format.fmt.pix.width;
	height = format.fmt.pix.height;

	reqbuf.count = REQ_BUFFER_NUM;
	reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	reqbuf.memory = V4L2_MEMORY_MMAP;
	//reqbuf.memory = V4L2_MEMORY_DMABUF;
	if (ioctl(fd, VIDIOC_REQBUFS, &reqbuf) < 0) {
		ISP_LOG_ERR("request buffer fail !\n");
		return -1;
	}

	for (i = 0; i < REQ_BUFFER_NUM; i++) {
		ViCtx[pipe].vbuf[i].type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		ViCtx[pipe].vbuf[i].memory = V4L2_MEMORY_MMAP;
		//ViCtx[pipe].vbuf[i].memory = V4L2_MEMORY_DMABUF;
		ViCtx[pipe].vbuf[i].index = i;

		if (ioctl(fd, VIDIOC_QUERYBUF, &ViCtx[pipe].vbuf[i]) < 0) {
			ISP_LOG_ERR("query buffer fail !\n");
			return -1;
		}

		ViCtx[pipe].framebuf[i].id = -1;
		ViCtx[pipe].framebuf[i].width = width;
		ViCtx[pipe].framebuf[i].height = height;
		ViCtx[pipe].framebuf[i].length = ViCtx[pipe].vbuf[i].length;
		ViCtx[pipe].framebuf[i].vir_addr = mmap(NULL, ViCtx[pipe].vbuf[i].length,
			PROT_READ | PROT_WRITE, MAP_SHARED, fd, ViCtx[pipe].vbuf[i].m.offset);

		if (MAP_FAILED == ViCtx[pipe].framebuf[i].vir_addr) {
			ISP_LOG_ERR("mmap fail !\n");
			return -1;
		}

		ISP_LOG_ERR("vbuf[%d]: size: %dx%d addr=%p, len=%ld, offset=%d\n",
			i, width, height, ViCtx[pipe].framebuf[i].vir_addr,
			ViCtx[pipe].framebuf[i].length, ViCtx[pipe].vbuf[i].m.offset);

		if (ioctl(fd, VIDIOC_QBUF, &ViCtx[pipe].vbuf[i]) < 0) {
			ISP_LOG_ERR("qbuf fail !\n");
			return -1;
		}

		ViCtx[pipe].framebuf[i].v4l2_buffer = (void *) &ViCtx[pipe].vbuf[i];
	}

	ViCtx[pipe].curr_index = 0;

	return 0;
}

static int streamon(int pipe)
{
	enum v4l2_buf_type type;
	struct v4l2_ext_controls val;
	struct v4l2_ext_control control;
	uint64_t phy_addr[REQ_BUFFER_NUM];
	struct v4l2_exportbuffer expbuf;

	int fd = ViCtx[pipe].vi_fd;

	// stream on
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl(fd, VIDIOC_STREAMON, &type) < 0) {
		ISP_LOG_ERR("stream on fail !\n");
		return -1;
	}

	// get phy_addr
	memset(&val, 0, sizeof(struct v4l2_ext_controls));
	control.id = VI_IOCTL_GET_V4L2_BUF_PHY_ADDR;
	control.ptr = phy_addr;
	val.count = 1;
	val.controls = &control;
	if (ioctl(fd, VIDIOC_G_EXT_CTRLS, &val) < 0) {
		ISP_LOG_ERR("get v4l2 buffer phy_addr fail\n");
		return -1;
	}

	memset(&expbuf, 0, sizeof(struct v4l2_exportbuffer));

	for (int i = 0; i < REQ_BUFFER_NUM; i++) {
		ISP_LOG_ERR("get v4l2 buffer phy_addr:0x%lx\n", phy_addr[i]);
		ViCtx[pipe].framebuf[i].phy_addr = phy_addr[i];

		expbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		expbuf.index = i;

		if (ioctl(fd, VIDIOC_EXPBUF, &expbuf) == -1) { // TODO:mason.zou
			ISP_LOG_ERR("expbuf fail !\n");
			return -1;
		}

		ISP_LOG_ERR("expbuf: %d\n", expbuf.fd);
		ViCtx[pipe].framebuf[i].dma_buf_fd = expbuf.fd;
	}

	return 0;
}

static int streamoff(int pipe)
{
	enum v4l2_buf_type type;

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (ioctl(ViCtx[pipe].vi_fd, VIDIOC_STREAMOFF, &type) < 0) {
		ISP_LOG_ERR("stream off fail !\n");
		return -1;
	}

	return 0;
}

static int set_wdr_mode(int pipe, int is_wdr_mode)
{
	int fd = ViCtx[pipe].vi_fd;
	struct v4l2_ext_controls val;
	struct v4l2_ext_control control;

	memset(&val, 0, sizeof(struct v4l2_ext_controls));

	if(fd > 0) {
		// test set ext ctrl
		control.id = VI_IOCTL_HDR;
		control.value = is_wdr_mode;
		val.count = 1;
		val.controls = &control;
		if(ioctl(fd, VIDIOC_S_EXT_CTRLS, &val) < 0) {
			ISP_LOG_ERR("pipe: %d, set wdr: %d fail !\n", pipe, is_wdr_mode);
			return -1;
		} else {
			printf("pipe: %d, set wdr: %d success\n", pipe, is_wdr_mode);
		}
	}

	return 0;
}

static void load_bnr_model(int pipe, char *model_path)
{
	FILE *fp = fopen(model_path, "r");

	if (fp == NULL) {
		ISP_LOG_ERR("open model path failed, %s\n", model_path);
		return;
	}

	char line[1024];
	char *token;
	char *rest;

	TEAISP_BNR_MODEL_INFO_S stModelInfo;

	while (fgets(line, sizeof(line), fp) != NULL) {

		printf("load model, list, %s", line);

		rest = line;
		token = strtok_r(rest, " ", &rest);

		memset(&stModelInfo, 0, sizeof(TEAISP_BNR_MODEL_INFO_S));
		snprintf(stModelInfo.path, TEAISP_MODEL_PATH_LEN, "%s", token);

		printf("load model, path, %s\n", token);

		token = strtok_r(rest, " ", &rest);

		int iso = atoi(token);

		stModelInfo.enterISO = iso;
		stModelInfo.tolerance = iso * 10 / 100;

		printf("load model, iso, %d, %d\n", stModelInfo.enterISO, stModelInfo.tolerance);
		CVI_TEAISP_BNR_SetModel(pipe, &stModelInfo);
	}

	fclose(fp);
}

typedef CVI_S32 (*TEAISP_INIT_FUN)(int pipe, int maxDev);

static int init_teaisp_bnr(int pipe, char *path)
{
	static TEAISP_INIT_FUN teaisp_init;

	if (teaisp_init == NULL) {
		void *teaisp_dl = dlopen("libteaisp.so", RTLD_LAZY);

		if (!teaisp_dl) {
			ISP_LOG_ERR("dlopen libteaisp.so fail, %s\n", dlerror());
			return -1;
		}

		dlerror();

		teaisp_init = (TEAISP_INIT_FUN) dlsym(teaisp_dl, "CVI_TEAISP_Init");
		if (teaisp_init == NULL) {
			ISP_LOG_ERR("load symbol CVI_TEAISP_Init fail, %s\n", dlerror());
			return -1;
		}

		// load bmrt
		teaisp_dl = dlopen("libbmrt.so", RTLD_LAZY);
		if (!teaisp_dl) {
			ISP_LOG_ERR("dlopen libbmrt.so fail, %s\n", dlerror());
			return -1;
		}

		teaisp_dl = dlopen("libbmlib.so", RTLD_LAZY);
		if (!teaisp_dl) {
			ISP_LOG_ERR("dlopen libbmlib.so fail, %s\n", dlerror());
			return -1;
		}
	}

	teaisp_init(pipe, TPU_DEV_NUM_MAX);
	load_bnr_model(pipe, path);

	return 0;
}

int start_vi(RTSP_CFG *p_rtsp_cfg)
{
	int pipe_num = p_rtsp_cfg->dev_num;
	char devicename[64];

	if (pipe_num > VI_MAX_PIPE_NUM || pipe_num < 1) {
		return -1;
	}

	// open fd and run isp
	for (int pipe = 0; pipe < pipe_num; ++pipe) {
		if (ViCtx[pipe].vi_fd > 0) {
			continue;
		}
		memset(&ViCtx[pipe], 0, sizeof(__ViCtx_S));

		sprintf(devicename, "/dev/video%d", pipe);

		ViCtx[pipe].vi_fd = open(devicename, O_RDWR | O_NONBLOCK);

		if (ViCtx[pipe].vi_fd < 0) {
			ISP_LOG_ERR("open %s fail\n", devicename);
			return -1;
		}

		pthread_mutex_init(&ViCtx[pipe].lock, NULL);
		set_wdr_mode(pipe, p_rtsp_cfg->pa_video_src_cfg[pipe].is_wdr_mode);
		CVI_ISP_V4L2_Init(pipe, ViCtx[pipe].vi_fd);

		if (p_rtsp_cfg->pa_video_src_cfg[pipe].enable_teaisp_bnr) {
			CVI_TEAISP_SetMode(pipe, TEAISP_RAW_MODE);
			init_teaisp_bnr(pipe, p_rtsp_cfg->pa_video_src_cfg[pipe].bnr_model_list);
		}
	}
	CVI_ISP_SetDISInfoCallback(get_dis_info);

	// request buffer and stream on
	for (int pipe = 0; pipe < pipe_num; ++pipe) {
		if (request_buffer(pipe) != 0) {
			ISP_LOG_ERR("pipe %d request buffer fail!\n", pipe);
			return -1;
		}
		if (streamon(pipe) != 0) {
			ISP_LOG_ERR("pipe %d stream on fail!\n", pipe);
			return -1;
		}
	}

	for (int pipe = 0; pipe < VI_MAX_PIPE_NUM; pipe++) {
		for (int i = 0; i < VBUF_LIST_MAX; i++) {
			memset(&ViCtx[pipe].vbufList[i], 0, sizeof(__Vbuf_S));
		}
	}

	return 0;
}

int stop_vi(RTSP_CFG *p_rtsp_cfg)
{
	int pipe_num = p_rtsp_cfg->dev_num;

	if (pipe_num > VI_MAX_PIPE_NUM || pipe_num < 1) {
		return -1;
	}

	for (int pipe = 0; pipe < pipe_num; ++pipe) {
		if (ViCtx[pipe].vi_fd <= 0) {
			continue;
		}

		CVI_ISP_V4L2_Exit(pipe);
		streamoff(pipe);
		pthread_mutex_destroy(&ViCtx[pipe].lock);
		close(ViCtx[pipe].vi_fd);
		ViCtx[pipe].vi_fd = -1;
	}

	return 0;
}

int get_yuv_frame(int pipe, int moduleId, VideoBuffer *pbuf)
{
	int ret = 0;
	int fd = ViCtx[pipe].vi_fd;
	struct v4l2_buffer temp_buf;
	int emptyBufIndex;

	if (pipe >= VI_MAX_PIPE_NUM || pbuf == NULL) {
		return -1;
	}

	ISP_LOG_INFO("P:%d,M:%d+++\n", pipe, moduleId);

	pthread_mutex_lock(&ViCtx[pipe].lock);

	emptyBufIndex = -1;

	for (int i = 0; i < VBUF_LIST_MAX; i++) {

		if (emptyBufIndex < 0 && ViCtx[pipe].vbufList[i].ref == 0) {
			emptyBufIndex = i;
		}

		if (ViCtx[pipe].vbufList[i].pvbf &&
			((ViCtx[pipe].vbufList[i].moduleId & moduleId) == 0x0)) {
			*pbuf = *ViCtx[pipe].vbufList[i].pvbf;
			ViCtx[pipe].vbufList[i].moduleId |=  moduleId;
			ViCtx[pipe].vbufList[i].ref++;
			ret = 0;
			goto exit_get_yuv_frame;
		}
	}

	if (emptyBufIndex < 0) {
		ISP_LOG_ERR("pipe %d vbufList not free, return...\n", pipe);
		ret = -1;
		goto exit_get_yuv_frame;
	}

	temp_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	temp_buf.memory = V4L2_MEMORY_MMAP;
	//temp_buf.memory = V4L2_MEMORY_DMABUF;

	if (ioctl(fd, VIDIOC_DQBUF, &temp_buf) < 0) {
		ISP_LOG_ERR("VIDIOC_DQBUF failed at frame(%ld) !\n", ViCtx[pipe].curr_index);
		ret = -1;
		goto exit_get_yuv_frame;
	}

	if (temp_buf.index >= REQ_BUFFER_NUM) {
		ISP_LOG_ERR("VIDIOC_DQBUF buffer index: %d out of REQ_BUFFER_NUM!\n", temp_buf.index);
		ret = -1;
		goto exit_get_yuv_frame;
	}

	ViCtx[pipe].vbuf[temp_buf.index] = temp_buf;
	ViCtx[pipe].framebuf[temp_buf.index].id = ViCtx[pipe].curr_index++;
	ViCtx[pipe].framebuf[temp_buf.index].v4l2_buffer = &ViCtx[pipe].vbuf[temp_buf.index];
	*pbuf = ViCtx[pipe].framebuf[temp_buf.index];

	dis_info_proc(pipe, pbuf);
	ViCtx[pipe].vbufList[emptyBufIndex].pvbf = &ViCtx[pipe].framebuf[temp_buf.index];
	ViCtx[pipe].vbufList[emptyBufIndex].moduleId = moduleId;
	ViCtx[pipe].vbufList[emptyBufIndex].ref = 1;

exit_get_yuv_frame:
	pthread_mutex_unlock(&ViCtx[pipe].lock);

	ISP_LOG_INFO("P:%d,M:%d,ret:%d---\n", pipe, moduleId, ret);

	return ret;
}

int put_yuv_frame(int pipe, int moduleId, VideoBuffer *pbuf)
{
	int ret = 0;
	int fd = ViCtx[pipe].vi_fd;
	struct v4l2_buffer *ptemp_buf;
	int putBufIndex = -1;

	if (pipe >= VI_MAX_PIPE_NUM || pbuf == NULL) {
		return -1;
	}

	if (pbuf->id < 0) {
		return -1;
	}

	ISP_LOG_INFO("P:%d,M:%d+++\n", pipe, moduleId);

	pthread_mutex_lock(&ViCtx[pipe].lock);
	destroy_dis_image(pipe, pbuf);
	for (int i = 0; i < VBUF_LIST_MAX; i++) {

		if (ViCtx[pipe].vbufList[i].pvbf &&
			ViCtx[pipe].vbufList[i].pvbf->phy_addr == pbuf->phy_addr) {

			putBufIndex = i;

			if ((ViCtx[pipe].vbufList[i].moduleId & moduleId) == 0x0) {
				ISP_LOG_ERR("module not find...\n");
				ret = -1;
				goto exit_put_yuv_frame;
			}

			if (ViCtx[pipe].vbufList[i].ref > 0) {
				ViCtx[pipe].vbufList[i].ref--;
			} else {
				ISP_LOG_ERR("vbuf ref error...\n");
				ret = -1;
				goto exit_put_yuv_frame;
			}

			if (ViCtx[pipe].vbufList[i].ref > 0) {
				ret = 0;
				goto exit_put_yuv_frame;
			} else {
				break;
			}
		}
	}

	if (putBufIndex < 0) {
		ISP_LOG_ERR("pipe: %d not find vbuf...\n", pipe);
		ret = -1;
		goto exit_put_yuv_frame;
	}

	ptemp_buf = (struct v4l2_buffer *) pbuf->v4l2_buffer;

	if (ioctl(fd, VIDIOC_QBUF, ptemp_buf) < 0) {
		ISP_LOG_ERR("VIDIOC_QBUF failed at frame(%ld)\n", ViCtx[pipe].curr_index);
		ret = -1;
		goto exit_put_yuv_frame;
	}

	memset(&ViCtx[pipe].vbufList[putBufIndex], 0, sizeof(__Vbuf_S));

exit_put_yuv_frame:
	pthread_mutex_unlock(&ViCtx[pipe].lock);

	ISP_LOG_INFO("P:%d,M:%d,ret:%d---\n", pipe, moduleId, ret);

	return ret;
}

