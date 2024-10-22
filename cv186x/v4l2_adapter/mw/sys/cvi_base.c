#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <sys/queue.h>
#include <pthread.h>
#include <inttypes.h>
#include <math.h>

#include <getopt.h>		/* getopt_long() */

#include <fcntl.h>		/* low-level i/o */
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/wait.h>

#include <base_uapi.h>
#include <sys/prctl.h>

#include "cvi_base.h"
#include "cvi_buffer.h"
#include <cvi_errno.h>

static CVI_S32 base_fd = -1;
static int dev_isp[VI_MAX_PIPE_NUM];

int get_v4l2_fd(int pipe)
{
	if (pipe >= VI_MAX_PIPE_NUM) {
		return -1;
	}
	return dev_isp[pipe];
}

int open_device(const char *dev_name, int *fd)
{
	struct stat st;

	*fd = open(dev_name, O_RDWR | O_NONBLOCK | O_CLOEXEC, 0);
	if (-1 == *fd) {
		fprintf(stderr, "Cannot open '%s': %d, %s\n", dev_name, errno,
			strerror(errno));
		return -1;
	}

	if (-1 == fstat(*fd, &st)) {
		close(*fd);
		fprintf(stderr, "Cannot identify '%s': %d, %s\n", dev_name,
			errno, strerror(errno));
		return -1;
	}

	if (!S_ISCHR(st.st_mode)) {
		close(*fd);
		fprintf(stderr, "%s is no device\n", dev_name);
		return -ENODEV;
	}
	return 0;
}

CVI_S32 close_device(CVI_S32 *fd)
{
	if (*fd == -1)
		return -1;

	if (-1 == close(*fd)) {
		fprintf(stderr, "%s: fd(%d) failure\n", __func__, *fd);
		return -1;
	}

	*fd = -1;

	return CVI_SUCCESS;
}

int CVI_ISP_V4L2_SetFd(int pipe, int fd)
{
	if (pipe >= VI_MAX_PIPE_NUM || fd <= 0) {
		return -1;
	}

	dev_isp[pipe] = fd;

	return 0;
}

int CVI_ISP_V4L2_GetFd(int pipe, int *pfd)
{
	int ret = 0;

	if (pipe >= VI_MAX_PIPE_NUM || pfd == NULL) {
		return -1;
	}

	if (dev_isp[pipe] > 0) {
		*pfd = dev_isp[pipe];
		ret = 0;
	} else {
		*pfd = -1;
		ret = -1;
	}

	return ret;
}

int get_vpss_fd(void)
{
	return 0;
}

CVI_S32 base_dev_open(CVI_VOID)
{
	if (base_fd != -1) {
		CVI_TRACE_SYS(CVI_DBG_DEBUG, "base dev has already opened\n");
		return CVI_SUCCESS;
	}

	if (open_device(BASE_DEV_NAME, &base_fd) != 0) {
		printf("%s: %s, base open failed.\n", __FILE__, __func__);
		base_fd = -1;
		return CVI_ERR_SYS_NOTREADY;
	}
	return CVI_SUCCESS;
}

CVI_S32 base_dev_close(CVI_VOID)
{
	if (base_fd == -1) {
		CVI_TRACE_SYS(CVI_DBG_INFO, "base dev is not opened\n");
		return CVI_SUCCESS;
	}

	close_device(&base_fd);
	base_fd = -1;
	return CVI_SUCCESS;
}

CVI_S32 get_base_fd(CVI_VOID)
{
	if (base_fd == -1)
		base_dev_open();
	return base_fd;
}
