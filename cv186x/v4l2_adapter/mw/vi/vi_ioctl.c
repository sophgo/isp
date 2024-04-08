#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

#include <fcntl.h>		/* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>

#include <linux/cvi_comm_video.h>
#include <linux/cvi_comm_vo.h>

#include <linux/vi_uapi.h>

#define UNUSED(x) ((void)(x))

int vi_sdk_disable_dis(int fd, int pipe)
{
	UNUSED(fd);
	UNUSED(pipe);
	return 0;
}

int vi_sdk_enable_dis(int fd, int pipe)
{
	UNUSED(fd);
	UNUSED(pipe);
	return 0;
}

int vi_sdk_get_pipe_attr(int fd, int pipe, VI_PIPE_ATTR_S *pstPipeAttr)
{
	UNUSED(pipe);
	struct v4l2_ext_controls ecs;
	struct v4l2_ext_control ec;

	memset(&ecs, 0, sizeof(ecs));

	if (fd > 0) {
		ec.id = VI_IOCTL_GET_PIPE_ATTR;
		ec.ptr = pstPipeAttr;
		ecs.count = 1;
		ecs.controls = &ec;
		if (ioctl(fd, VIDIOC_G_EXT_CTRLS, &ecs) < 0) {
			printf("get ext ctrls fail!\n");
			return -1;
		}
	}
	return 0;
}

int vi_sdk_set_pipe_attr(int fd, int pipe, VI_PIPE_ATTR_S *pstPipeAttr)
{
	UNUSED(pipe);
	struct v4l2_ext_controls ecs;
	struct v4l2_ext_control ec;

	memset(&ecs, 0, sizeof(ecs));

	if (fd > 0) {
		ec.id = VI_IOCTL_SET_PIPE_ATTR;
		ec.ptr = pstPipeAttr;
		ecs.count = 1;
		ecs.controls = &ec;
		if (ioctl(fd, VIDIOC_S_EXT_CTRLS, &ecs) < 0) {
			printf("set ext ctrls fail!\n");
			return -1;
		}
	}
	return 0;
}

int vi_sdk_get_pipe_dump_attr(int fd, int pipe, VI_DUMP_ATTR_S *pstDumpAttr)
{
	UNUSED(fd);
	UNUSED(pipe);
	UNUSED(pstDumpAttr);
	return 0;
}

int vi_sdk_set_pipe_dump_attr(int fd, int pipe, VI_DUMP_ATTR_S *pstDumpAttr)
{
	struct v4l2_ext_controls ecs;
	struct v4l2_ext_control ec;
	int rc = 0;

	UNUSED(pipe);
	memset(&ecs, 0, sizeof(ecs));

	ec.id = VI_IOCTL_SET_PIPE_DUMP_ATTR;
	ec.ptr = pstDumpAttr;
	ecs.count = 1;
	ecs.controls = &ec;
	rc = ioctl(fd, VIDIOC_S_EXT_CTRLS, &ecs);
	if (rc < 0) {
		fprintf(stderr, "VIDIOC_G_EXT_CTRLS - %s NG, %s\n", __func__, strerror(errno));
		return -1;
	}

	return 0;
}

int vi_sdk_set_bypass_frm(int fd, CVI_U32 snr_num, CVI_U8 bypass_num)
{
	UNUSED(snr_num);
	struct v4l2_ext_controls ecs;
	struct v4l2_ext_control ec;

	memset(&ecs, 0, sizeof(ecs));

	if (fd > 0) {
		ec.id = VI_IOCTL_SET_BYPASS_FRM;
		ec.value = bypass_num;
		ecs.count = 1;
		ecs.controls = &ec;
		if (ioctl(fd, VIDIOC_S_EXT_CTRLS, &ecs) < 0) {
			printf("set the bypass frame fail!\n");
			return -1;
		}
	}
	return 0;
}

int vi_sdk_get_chn_frame(int fd, int pipe, int chn, VIDEO_FRAME_INFO_S *pstFrameInfo, CVI_S32 s32MilliSec)
{
	struct v4l2_ext_controls ecs;
	struct v4l2_ext_control ec;
	int rc = 0;

	UNUSED(pipe);
	UNUSED(chn);
	UNUSED(s32MilliSec);
	memset(&ecs, 0, sizeof(ecs));

	ec.id = VI_IOCTL_GET_CHN_FRAME;
	ec.ptr = pstFrameInfo;
	ecs.count = 1;
	ecs.controls = &ec;
	rc = ioctl(fd, VIDIOC_G_EXT_CTRLS, &ecs);
	if (rc < 0) {
		fprintf(stderr, "VIDIOC_G_EXT_CTRLS - %s NG, %s\n", __func__, strerror(errno));
		return -1;
	}

	return 0;
}

int vi_sdk_release_chn_frame(int fd, int pipe, int chn, VIDEO_FRAME_INFO_S *pstFrameInfo)
{
	struct v4l2_ext_controls ecs;
	struct v4l2_ext_control ec;
	int rc = 0;

	UNUSED(pipe);
	UNUSED(chn);
	memset(&ecs, 0, sizeof(ecs));

	ec.id = VI_IOCTL_RELEASE_CHN_FRAME;
	ec.ptr = pstFrameInfo;
	ecs.count = 1;
	ecs.controls = &ec;
	rc = ioctl(fd, VIDIOC_S_EXT_CTRLS, &ecs);
	if (rc < 0) {
		fprintf(stderr, "VIDIOC_S_EXT_CTRLS - %s NG, %s\n", __func__, strerror(errno));
		return -1;
	}

	return 0;
}

int vi_sdk_get_pipe_frame(int fd, int pipe, VIDEO_FRAME_INFO_S *pstFrameInfo, CVI_S32 s32MilliSec)
{
	struct v4l2_ext_controls ecs;
	struct v4l2_ext_control ec;
	int rc = 0;

	UNUSED(pipe);
	UNUSED(s32MilliSec);
	memset(&ecs, 0, sizeof(ecs));

	ec.id = VI_IOCTL_GET_PIPE_FRAME;
	ec.ptr = pstFrameInfo;
	ecs.count = 1;
	ecs.controls = &ec;
	rc = ioctl(fd, VIDIOC_G_EXT_CTRLS, &ecs);
	if (rc < 0) {
		fprintf(stderr, "VIDIOC_G_EXT_CTRLS - %s NG, %s\n", __func__, strerror(errno));
		return -1;
	}

	return 0;
}

int vi_sdk_release_pipe_frame(int fd, int pipe, VIDEO_FRAME_INFO_S *pstFrameInfo)
{
	struct v4l2_ext_controls ecs;
	struct v4l2_ext_control ec;
	int rc = 0;

	UNUSED(pipe);
	memset(&ecs, 0, sizeof(ecs));

	ec.id = VI_IOCTL_RELEASE_PIPE_FRAME;
	ec.ptr = pstFrameInfo;
	ecs.count = 1;
	ecs.controls = &ec;
	rc = ioctl(fd, VIDIOC_S_EXT_CTRLS, &ecs);
	if (rc < 0) {
		fprintf(stderr, "VIDIOC_S_EXT_CTRLS - %s NG, %s\n", __func__, strerror(errno));
		return -1;
	}

	return 0;
}

int vi_get_ip_dump_list(int fd, void *ip_info_list)
{
	struct v4l2_ext_controls ecs;
	struct v4l2_ext_control ec;
	int rc = 0;

	memset(&ecs, 0, sizeof(ecs));

	ec.id = VI_IOCTL_GET_IP_INFO;
	ec.ptr = ip_info_list;
	ecs.count = 1;
	ecs.controls = &ec;
	rc = ioctl(fd, VIDIOC_G_EXT_CTRLS, &ecs);
	if (rc < 0) {
		fprintf(stderr, "VIDIOC_G_EXT_CTRLS - %s NG, %s\n", __func__, strerror(errno));
		return -1;
	}

	return 0;
}

int vi_get_tun_addr(int fd, void *tun_buf_info)
{
	struct v4l2_ext_controls ecs;
	struct v4l2_ext_control ec;
	int rc = 0;

	memset(&ecs, 0, sizeof(ecs));

	ec.id = VI_IOCTL_GET_TUN_ADDR;
	ec.ptr = tun_buf_info;
	ecs.count = 1;
	ecs.controls = &ec;
	rc = ioctl(fd, VIDIOC_G_EXT_CTRLS, &ecs);
	if (rc < 0) {
		fprintf(stderr, "VIDIOC_G_EXT_CTRLS - %s NG, %s\n", __func__, strerror(errno));
		return -1;
	}

	return 0;
}

int vi_sdk_set_dev_attr(int fd, int dev, VI_DEV_ATTR_S *pstDevAttr)
{
	UNUSED(dev);

	struct v4l2_ext_controls ecs;
	struct v4l2_ext_control ec;

	memset(&ecs, 0, sizeof(ecs));

	if (fd > 0) {
		ec.id = VI_IOCTL_SET_DEV_ATTR;
		ec.ptr = pstDevAttr;
		ecs.count = 1;
		ecs.controls = &ec;
		if (ioctl(fd, VIDIOC_S_EXT_CTRLS, &ecs) < 0) {
			printf("set ext ctrls fail!\n");
			return -1;
		}
	}

	return 0;
}

int vi_sdk_get_dev_attr(int fd, int dev, VI_DEV_ATTR_S *pstDevAttr)
{
	UNUSED(dev);

	struct v4l2_ext_controls ecs;
	struct v4l2_ext_control ec;

	memset(&ecs, 0, sizeof(ecs));

	if (fd > 0) {
		ec.id = VI_IOCTL_GET_DEV_ATTR;
		ec.ptr = pstDevAttr;
		ecs.count = 1;
		ecs.controls = &ec;
		if (ioctl(fd, VIDIOC_G_EXT_CTRLS, &ecs) < 0) {
			printf("get ext ctrls fail!\n");
			return -1;
		}
	}

	return 0;
}