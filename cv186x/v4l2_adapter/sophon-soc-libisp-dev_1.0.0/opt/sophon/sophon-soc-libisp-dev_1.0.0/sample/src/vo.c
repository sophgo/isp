
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>

#include <sys/mman.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <poll.h>

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm_fourcc.h>
#include <linux/videodev2.h>

#include "clog.h"
#include "isp_debug.h"
#include "cvi_isp.h"

#include "vi.h"
#include "vo.h"
#include "cvi_vo.h"
#include "cvi_isp_v4l2.h"

#define VO_DEV_PATH "/dev/dri/card0"

#define UNUSED(x) ((void)(x))

#define MODULE_ID (VO_MODULE_ID)

typedef struct {
	int dev_fd;
	int vi_pipe;
	uint32_t crtc_id;
	uint32_t conn_id;
	drmModeModeInfo modeInfo;
	drmModeRes *res;
	bool bHotplugThreadRun;
	pthread_t hotplugThreadTid;
	bool bDisplayThreadRun;
	pthread_t displayThreadTid;
	VideoBuffer vbuf;
	int drmHandle[V4L2_REQ_BUFFER_NUM];
} __VoCtx_S;

static __VoCtx_S voCtx;

#ifndef __USE_MEMCPY

static int add_fb(VideoBuffer *pvbuf)
{
	struct v4l2_buffer *pv4l2buf;

	pv4l2buf = (struct v4l2_buffer *) pvbuf->v4l2_buffer;

	if (pv4l2buf->index >= V4L2_REQ_BUFFER_NUM) {
		ISP_LOG_ERR("out of range, %d, %d\n", V4L2_REQ_BUFFER_NUM, pv4l2buf->index);
		return -1;
	}

	if (voCtx.drmHandle[pv4l2buf->index] < 0) {
		int ret;
		uint32_t handles[4];
		uint32_t pitches[4];
		uint32_t offsets[4];
		uint32_t drmHandle;

		ret = drmPrimeFDToHandle(voCtx.dev_fd, pvbuf->dma_buf_fd, &drmHandle);
		if (ret) {
			ISP_LOG_ERR("drmPrimeFDToHandle error, %s\n", strerror(errno));
			return -1;
		}

		// NV21
		handles[0] = drmHandle;
		pitches[0] = pvbuf->width;
		offsets[0] = 0;

		handles[1] = drmHandle;
		pitches[1] = pvbuf->width;
		offsets[1] = pvbuf->width * pvbuf->height;

		handles[2] = 0;
		pitches[2] = 0;
		offsets[2] = 0;

		handles[3] = 0;
		pitches[3] = 0;
		offsets[3] = 0;

		ret = drmModeAddFB2(voCtx.dev_fd, pvbuf->width, pvbuf->height, DRM_FORMAT_NV21,
			handles, pitches, offsets, &drmHandle, 0);
		if (ret) {
			ISP_LOG_ERR("drmModeAddFB2 error, %s\n", strerror(errno));
			return -1;
		}

		voCtx.drmHandle[pv4l2buf->index] = drmHandle;
	}

	return pv4l2buf->index;
}

static void modeset_page_flip_handler(int fd, uint32_t frame,
				    uint32_t sec, uint32_t usec,
				    void *data)
{
	(void)frame;
	(void)sec;
	(void)usec;

	int index;
	VideoBuffer vbuf;
	uint32_t crtc_id = *(uint32_t *)data;

	get_yuv_frame(voCtx.vi_pipe, MODULE_ID, &vbuf);

	index = add_fb(&vbuf);
	if (index < 0) {
		put_yuv_frame(voCtx.vi_pipe, MODULE_ID, &vbuf);
		return;
	}

	drmModePageFlip(fd, crtc_id, voCtx.drmHandle[index],
			DRM_MODE_PAGE_FLIP_EVENT, data);

	put_yuv_frame(voCtx.vi_pipe, MODULE_ID, &voCtx.vbuf);
	voCtx.vbuf = vbuf;
}

static void *display_thread(void *param)
{
	UNUSED(param);

	int index;
	uint32_t crtc_id = voCtx.crtc_id;
	uint32_t conn_id = voCtx.conn_id;

	for (int i = 0; i < V4L2_REQ_BUFFER_NUM; i++) {
		voCtx.drmHandle[i] = -1;
	}

	get_yuv_frame(voCtx.vi_pipe, MODULE_ID, &voCtx.vbuf);

	index = add_fb(&voCtx.vbuf);
	if (index < 0) {
		put_yuv_frame(voCtx.vi_pipe, MODULE_ID, &voCtx.vbuf);
		return NULL;
	}

	drmModeSetCrtc(voCtx.dev_fd, crtc_id, voCtx.drmHandle[index],
		0, 0, &conn_id, 1, &voCtx.modeInfo);

	drmModePageFlip(voCtx.dev_fd, crtc_id, voCtx.drmHandle[index],
			DRM_MODE_PAGE_FLIP_EVENT, &crtc_id);

	drmEventContext ev = {};

	ev.version = DRM_EVENT_CONTEXT_VERSION;
	ev.page_flip_handler = modeset_page_flip_handler;

	while (voCtx.bDisplayThreadRun) {
		drmHandleEvent(voCtx.dev_fd, &ev);
	}

	for (int i = 0; i < V4L2_REQ_BUFFER_NUM; i++) {
		if (voCtx.drmHandle[i] >= 0) {
			drmModeRmFB(voCtx.dev_fd, voCtx.drmHandle[i]);

			struct drm_gem_close args = {0};

			args.handle = voCtx.drmHandle[i];
			drmIoctl(voCtx.dev_fd, DRM_IOCTL_GEM_CLOSE, &args);
		}
	}

	put_yuv_frame(voCtx.vi_pipe, MODULE_ID, &voCtx.vbuf);

	return NULL;
}
#else

#define __MAX_DUMB (2)

static void *dumb_ptr[__MAX_DUMB];
static size_t dumb_size[__MAX_DUMB];
static size_t dumb_pitch[__MAX_DUMB];
static uint32_t dumb_handle[__MAX_DUMB];
static uint32_t dumb_fb_id[__MAX_DUMB];

static void modeset_page_flip_handler(int fd, uint32_t frame,
				    uint32_t sec, uint32_t usec,
				    void *data)
{
	(void)frame;
	(void)sec;
	(void)usec;

	static int i = 0;
	uint32_t crtc_id = *(uint32_t *)data;

	i ^= 1;

	VideoBuffer vbuf;

	get_yuv_frame(voCtx.vi_pipe, MODULE_ID, &vbuf);

	memcpy(dumb_ptr[i], vbuf.vir_addr, dumb_size[i]); // !!!

	put_yuv_frame(voCtx.vi_pipe, MODULE_ID, &vbuf);

	drmModePageFlip(fd, crtc_id, dumb_fb_id[i],
			DRM_MODE_PAGE_FLIP_EVENT, data);
}

static void *display_thread(void *param)
{
	int ret;
	uint32_t width, height;
	ISP_PUB_ATTR_S stPubAttr;

	UNUSED(param);

	memset(&stPubAttr, 0, sizeof(ISP_PUB_ATTR_S));
	CVI_ISP_GetPubAttr(voCtx.vi_pipe, &stPubAttr);

	width = stPubAttr.stWndRect.u32Width;
	height = stPubAttr.stWndRect.u32Height;

	// create dumb
	for (int i = 0; i < __MAX_DUMB; i++) {
		struct drm_mode_create_dumb arg;

		memset(&arg, 0, sizeof(arg));
		arg.bpp = 8; // NV21
		arg.width = width;
		arg.height = height * 3 / 2; // NV21

		ret = drmIoctl(voCtx.dev_fd, DRM_IOCTL_MODE_CREATE_DUMB, &arg);
		if (ret) {
			ISP_LOG_ERR("failed to create dumb buffer: %s\n", strerror(errno));
			goto fail;
		}

		dumb_handle[i] = arg.handle;
		dumb_size[i] = arg.size;
		dumb_pitch[i] = arg.pitch;

		printf("create dumb: %d, %d, %ld, %ld\n", i, dumb_handle[i],
			dumb_size[i], dumb_pitch[i]);
	}

	// mmap
	for (int i = 0; i < __MAX_DUMB; i++) {
		struct drm_mode_map_dumb arg;

		memset(&arg, 0, sizeof(arg));

		arg.handle = dumb_handle[i];
		ret = drmIoctl(voCtx.dev_fd, DRM_IOCTL_MODE_MAP_DUMB, &arg);
		if (ret) {
			ISP_LOG_ERR("failed to map dumb buffer: %s\n", strerror(errno));
			goto fail;
		}

		dumb_ptr[i] = mmap(0, dumb_size[i], PROT_READ | PROT_WRITE, MAP_SHARED,
		       voCtx.dev_fd, arg.offset);

		if (dumb_ptr[i] == MAP_FAILED) {
			ISP_LOG_ERR("mmap failed!!!\n");
			dumb_ptr[i] = NULL;
			goto fail;
		}

		memset(dumb_ptr[i], 0, dumb_size[i]);

		printf("mmap: %d, %p\n", i, dumb_ptr[i]);
	}

	// add fb
	uint32_t handles[4];
	uint32_t pitches[4];
	uint32_t offsets[4];

	for (int i = 0; i < __MAX_DUMB; i++) {
		handles[0] = dumb_handle[i];
		pitches[0] = dumb_pitch[i];
		offsets[0] = 0;

		handles[1] = dumb_handle[i];
		pitches[1] = dumb_pitch[i];
		offsets[1] = dumb_pitch[i] * height;

		ret = drmModeAddFB2(voCtx.dev_fd, width, height, DRM_FORMAT_NV21, handles, pitches,
			offsets, &dumb_fb_id[i], 0);
		if (ret) {
			printf("drmModeAddFB2 failed, %s\n", strerror(errno));
			goto fail;
		}
	}

	uint32_t crtc_id = voCtx.crtc_id;
	uint32_t conn_id = voCtx.conn_id;

	drmModeSetCrtc(voCtx.dev_fd, crtc_id, dumb_fb_id[0],
			0, 0, &conn_id, 1, &voCtx.modeInfo);

	drmModePageFlip(voCtx.dev_fd, crtc_id, dumb_fb_id[0],
			DRM_MODE_PAGE_FLIP_EVENT, &crtc_id);

	drmEventContext ev = {};

	ev.version = DRM_EVENT_CONTEXT_VERSION;
	ev.page_flip_handler = modeset_page_flip_handler;

	while (voCtx.bDisplayThreadRun) {
		drmHandleEvent(voCtx.dev_fd, &ev);
	}

fail:

	for (int i = 0; i < __MAX_DUMB; i++) {
		if (dumb_fb_id[i] > 0) {
			drmModeRmFB(voCtx.dev_fd, dumb_fb_id[i]);
		}

		if (dumb_ptr[i]) {
			munmap(dumb_ptr[i], dumb_size[i]);
		}

		if (dumb_handle[i] > 0) {
			struct drm_mode_destroy_dumb arg;

			memset(&arg, 0, sizeof(arg));
			arg.handle = dumb_handle[i];

			drmIoctl(voCtx.dev_fd, DRM_IOCTL_MODE_DESTROY_DUMB, &arg);
		}
	}

	return NULL;
}
#endif

static void update_mode_info(void)
{
	ISP_PUB_ATTR_S stPubAttr;
	uint32_t conn_id = voCtx.conn_id;
	drmModeConnector *conn;
	int modeIndex;

	conn = drmModeGetConnector(voCtx.dev_fd, conn_id);
	if (conn == NULL) {
		return;
	}

	if (voCtx.bDisplayThreadRun) {
		voCtx.bDisplayThreadRun = false;
		printf("wait display thread finish...\n");
		pthread_join(voCtx.displayThreadTid, NULL);
	}

	if (conn->count_modes <= 0) {
		drmModeFreeConnector(conn);
		printf("pull out, return....\n");
		return;
	}

	memset(&stPubAttr, 0, sizeof(ISP_PUB_ATTR_S));
	CVI_ISP_GetPubAttr(voCtx.vi_pipe, &stPubAttr);

	for (modeIndex = 0; modeIndex < conn->count_modes; modeIndex++) {
		printf("connector modes info, type: %d, vrefresh: %d, clock: %d, h: %d, v: %d\n",
			conn->modes[modeIndex].type,
			conn->modes[modeIndex].vrefresh,
			conn->modes[modeIndex].clock,
			conn->modes[modeIndex].hdisplay,
			conn->modes[modeIndex].vdisplay);

		if (conn->modes[modeIndex].hdisplay == stPubAttr.stWndRect.u32Width &&
			conn->modes[modeIndex].vdisplay == stPubAttr.stWndRect.u32Height &&
			conn->modes[modeIndex].vrefresh <= 30) {
			break;
		}
	}

	if (modeIndex >= conn->count_modes) {
		CVI_BOOL bFind = false;
		CVI_U32 findIdx = 0;

		for (modeIndex = 0; modeIndex < conn->count_modes; modeIndex++) {
			if (conn->modes[modeIndex].hdisplay < stPubAttr.stWndRect.u32Width &&
				conn->modes[modeIndex].vdisplay < stPubAttr.stWndRect.u32Height) {
				if (bFind) {
					if (conn->modes[modeIndex].hdisplay > conn->modes[findIdx].hdisplay &&
						conn->modes[modeIndex].vdisplay > conn->modes[findIdx].vdisplay) {
						findIdx = modeIndex;
					}
				} else {
					bFind = true;
					findIdx = modeIndex;
				}
			}
		}
		modeIndex = findIdx;
	}

	voCtx.modeInfo = conn->modes[modeIndex];

	printf("start display thread...\n");

	voCtx.bDisplayThreadRun = true;
	pthread_create(&voCtx.displayThreadTid, NULL, display_thread, NULL);

	drmModeFreeConnector(conn);
}

static void *hotplug_monitor_thread(void *param)
{
#define __HDMI_HOTPLUG_MSG "card0"

	struct sockaddr_nl nladdr;
	int sz = 64 * 1024;
	int on = 1;

	UNUSED(param);

	memset(&nladdr, 0, sizeof(nladdr));

	nladdr.nl_family = AF_NETLINK;
	nladdr.nl_pid = getpid();
	nladdr.nl_groups = 1;

	int fd = -1;

	char recvbuf[sz];
	int recvlen = 0;
	struct pollfd fds[1];

	memset(recvbuf, 0, sz);
	memset(fds, 0, sizeof(fds));

	int ret = -1;

	fd = socket(PF_NETLINK, SOCK_DGRAM | SOCK_CLOEXEC, NETLINK_KOBJECT_UEVENT);
	if (fd < 0) {
		ISP_LOG_ERR("Unable to create uevent socket\n");
		return (void *) -1;
	}

	if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &sz, sizeof(sz)) < 0) {
		ISP_LOG_ERR("Unable to set uevent socket SO_RCVBUF/SO_RCVBUFFORCE option\n");
		goto ERROR;
	}

	if (setsockopt(fd, SOL_SOCKET, SO_PASSCRED, &on, sizeof(on)) < 0) {
		ISP_LOG_ERR("Unable to set uevent socket SO_PASSCRED option\n");
		goto ERROR;
	}

	if (bind(fd, (struct sockaddr *)&nladdr, sizeof(nladdr)) < 0) {
		ISP_LOG_ERR("Unable to bind uevent socket\n");
		goto ERROR;
	}

	fds[0].fd = fd;
	fds[0].events = POLLIN | POLLERR | POLLHUP;

	voCtx.bHotplugThreadRun = true;

	printf("hotplug thread start...\n");

	while (voCtx.bHotplugThreadRun) {

		ret = poll(fds, 1, (1 * 500));
		if (ret <= 0) {
			continue;
		}

		if (fds[0].revents & POLLIN) {
			recvlen = recv(fd, &recvbuf, sz, 0);
			if (recvlen <= 0) {
				continue;
			}
			//printf("%s\n", recvbuf);
			if (NULL != strstr(recvbuf, __HDMI_HOTPLUG_MSG)) {
				update_mode_info();
			}
		}
	}

	printf("hotplug thread end...\n");

ERROR:
	close(fd);

	return NULL;
}

int start_vo(RTSP_CFG *p_rtsp_cfg)
{
	UNUSED(p_rtsp_cfg);

	uint32_t crtc_id = VO_HDMI_CRTC_ID;
	uint32_t conn_id = VO_HDMI_CONN_ID;

	memset(&voCtx, 0, sizeof(__VoCtx_S));

	voCtx.vi_pipe = -1;

	for (int i = 0; i < p_rtsp_cfg->dev_num; i++) {
		if (p_rtsp_cfg->pa_video_src_cfg &&
			p_rtsp_cfg->pa_video_src_cfg[i].enable_hdmi) {
			voCtx.vi_pipe = i;
			break;
		}
	}

	if (voCtx.vi_pipe < 0) { // not find
		return 0;
	}

	voCtx.crtc_id = crtc_id;
	voCtx.conn_id = conn_id;

	voCtx.dev_fd = open(VO_DEV_PATH, O_RDWR | O_CLOEXEC);
	if (voCtx.dev_fd <= 0) {
		ISP_LOG_ERR("open %s failed!!!\n", VO_DEV_PATH);
		goto failed;
	}

	CVI_ISP_V4L2_SetVoFd(voCtx.dev_fd);

	voCtx.res = drmModeGetResources(voCtx.dev_fd);
	if (voCtx.res == NULL) {
		ISP_LOG_ERR("drmModeGetResources failed!!!\n");
		goto failed;
	}

	for (int i = 0; i < voCtx.res->count_crtcs; i++) {
		if (crtc_id == voCtx.res->crtcs[i])
			break;
		if (i == voCtx.res->count_crtcs - 1) {
			ISP_LOG_ERR("not find crtc-%u\n", crtc_id);
			goto failed;
		}
	}

	for (int i = 0; i < voCtx.res->count_connectors; i++) {
		if (conn_id == voCtx.res->connectors[i])
			break;
		if (i == voCtx.res->count_connectors - 1) {
			ISP_LOG_ERR("not find connector-%u\n", conn_id);
			goto failed;
		}
	}

	update_mode_info();
	pthread_create(&voCtx.hotplugThreadTid, NULL, hotplug_monitor_thread, NULL);

	return 0;

failed:

	if (voCtx.res) {
		drmModeFreeResources(voCtx.res);
		voCtx.res = NULL;
	}

	if (voCtx.dev_fd > 0) {
		close(voCtx.dev_fd);
		voCtx.dev_fd = -1;
	}

	CVI_ISP_V4L2_SetVoFd(-1);

	return -1;
}

int stop_vo(RTSP_CFG *p_rtsp_cfg)
{
	UNUSED(p_rtsp_cfg);

	if (voCtx.vi_pipe < 0) {
		return 0;
	}

	// set default vo gamma
	VO_GAMMA_INFO_S vo_gamma;

	vo_gamma.enable = 1;
	vo_gamma.osd_apply = 0;

	CVI_U32 default_gamma_table[VO_GAMMA_NODENUM] = {
		0,   3,   7,   11,  15,  19,  23,  27,
		31,  35,  39,  43,  47,  51,  55,  59,
		63,  67,  71,  75,  79,  83,  87,  91,
		95,  99,  103, 107, 111, 115, 119, 123,
		127, 131, 135, 139, 143, 147, 151, 155,
		159, 163, 167, 171, 175, 179, 183, 187,
		191, 195, 199, 203, 207, 211, 215, 219,
		223, 227, 231, 235, 239, 243, 247, 251,
		255
	};

	memcpy(vo_gamma.value, default_gamma_table, VO_GAMMA_NODENUM * sizeof(CVI_U32));

	CVI_VO_SetGammaInfo(&vo_gamma);

	if (voCtx.bDisplayThreadRun) {
		voCtx.bDisplayThreadRun = false;
		printf("wait display thread finish...\n");
		pthread_join(voCtx.displayThreadTid, NULL);
	}

	voCtx.bHotplugThreadRun = false;
	printf("wait hotplug thread finish...\n");
	pthread_join(voCtx.hotplugThreadTid, NULL);

	if (voCtx.res) {
		drmModeFreeResources(voCtx.res);
		voCtx.res = NULL;
	}

	if (voCtx.dev_fd > 0) {
		close(voCtx.dev_fd);
		voCtx.dev_fd = -1;
	}

	CVI_ISP_V4L2_SetVoFd(-1);

	return 0;
}

