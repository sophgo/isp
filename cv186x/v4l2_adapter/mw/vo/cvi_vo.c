#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/param.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <inttypes.h>
#include <math.h>
#include <sys/prctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm_fourcc.h>
#include "cvi_sys.h"
#include <cvi_comm_vo.h>
#include "cvi_buffer.h"

#include "cvi_base.h"
#include "cvi_vo.h"
#include <vo_uapi.h>

#define UNUSED(x) ((void)(x))

#define CRTC_ID 40
#define CONNECTOR_ID 44
#define BIT8_TO_BIT16(x) ((float)x / 255.0 * 65535.0)
#define BIT16_TO_BIT8(x) ((float)x / 65535.0 * 255.0)
#define VO_BIN_GUARDMAGIC 0x12345678

static int vo_fd = -1;

static VO_BIN_INFO_S vo_bin_info[VO_MAX_DEV_NUM] = {
	{
		.gamma_info = {
			.s32VoDev = 0,
			.enable = CVI_FALSE,
			.osd_apply = CVI_FALSE,
			.value = {
				0,   3,   7,   11,  15,  19,  23,  27,
				31,  35,  39,  43,  47,  51,  55,  59,
				63,  67,  71,  75,  79,  83,  87,  91,
				95,  99,  103, 107, 111, 115, 119, 123,
				127, 131, 135, 139, 143, 147, 151, 155,
				159, 163, 167, 171, 175, 179, 183, 187,
				191, 195, 199, 203, 207, 211, 215, 219,
				223, 227, 231, 235, 239, 243, 247, 251,
				255
			}
		},
		.guard_magic = VO_BIN_GUARDMAGIC
	},
	{
		.gamma_info = {
			.s32VoDev = 1,
			.enable = CVI_FALSE,
			.osd_apply = CVI_FALSE,
			.value = {
				0,   3,   7,   11,  15,  19,  23,  27,
				31,  35,  39,  43,  47,  51,  55,  59,
				63,  67,  71,  75,  79,  83,  87,  91,
				95,  99,  103, 107, 111, 115, 119, 123,
				127, 131, 135, 139, 143, 147, 151, 155,
				159, 163, 167, 171, 175, 179, 183, 187,
				191, 195, 199, 203, 207, 211, 215, 219,
				223, 227, 231, 235, 239, 243, 247, 251,
				255
			}
		},
		.guard_magic = VO_BIN_GUARDMAGIC
	}
};

CVI_S32 CVI_VO_SetGammaInfo(VO_GAMMA_INFO_S *pinfo)
{
	int fd;
	int ret = 0;
	drmModeRes *res;
	uint32_t crtc_id = CRTC_ID;
	uint32_t conn_id = CONNECTOR_ID;
	uint16_t disp_gamma_data[VO_GAMMA_NODENUM];

	fd = vo_fd;

	if (fd < 0) {
		printf("wrong vo fd!\n");
		return -1;
	}

	res = drmModeGetResources(fd);
	for (int i = 0; i < res->count_crtcs; i++) {
		if(crtc_id == res->crtcs[i])
			break;
		if(i == res->count_crtcs -1) {
			printf("not find crtc-%u\n", crtc_id);
			return -1;
		}
	}

	for (int i = 0; i < res->count_connectors; i++) {
		if(conn_id == res->connectors[i])
			break;
		if(i == res->count_connectors -1) {
			printf("not find connector-%u\n", conn_id);
			return -1;
		}
	}

	vo_bin_info[0].gamma_info.enable = pinfo->enable;
	vo_bin_info[0].gamma_info.osd_apply = pinfo->osd_apply;

	if (!pinfo->enable)
		return 0;

	for (uint32_t i = 0; i < VO_GAMMA_NODENUM; i++)
		disp_gamma_data[i] = BIT8_TO_BIT16(pinfo->value[i]);

	ret = drmModeCrtcSetGamma(fd, crtc_id, VO_GAMMA_NODENUM,
		disp_gamma_data, disp_gamma_data, disp_gamma_data);

	if (ret != 0) {
		printf("set the vo gamma fail!\n");
	}

	return ret;
}

CVI_S32 CVI_VO_GetGammaInfo(VO_GAMMA_INFO_S *pinfo)
{
	int fd;
	int ret = 0;
	drmModeRes *res;
	uint32_t crtc_id = CRTC_ID;
	uint32_t conn_id = CONNECTOR_ID;
	uint16_t disp_gamma_data[VO_GAMMA_NODENUM];

	fd = vo_fd;

	if (fd < 0) {
		printf("wrong vo fd!\n");
		return -1;
	}

	res = drmModeGetResources(fd);
	for (int i = 0; i < res->count_crtcs; i++) {
		if(crtc_id == res->crtcs[i])
			break;
		if(i == res->count_crtcs -1) {
			printf("not find crtc-%u\n", crtc_id);
			return -1;
		}
	}

	for (int i = 0; i < res->count_connectors; i++) {
		if(conn_id == res->connectors[i])
			break;
		if(i == res->count_connectors -1) {
			printf("not find connector-%u\n", conn_id);
			return -1;
		}
	}

	pinfo->enable = vo_bin_info[0].gamma_info.enable;
	pinfo->osd_apply = vo_bin_info[0].gamma_info.osd_apply;

	ret = drmModeCrtcGetGamma(fd, crtc_id, VO_GAMMA_NODENUM,
		disp_gamma_data, disp_gamma_data, disp_gamma_data);

	if (ret != 0) {
		printf("fail to get the vo gamma!\n");
	}

	for (uint32_t i = 0; i < VO_GAMMA_NODENUM; i++)
		pinfo->value[i] = BIT16_TO_BIT8(disp_gamma_data[i]);

	return ret;
}

VO_BIN_INFO_S *get_vo_bin_info_addr(void)
{
	return vo_bin_info;
}

CVI_U32 get_vo_bin_guardmagic_code(void)
{
	return VO_BIN_GUARDMAGIC;
}

int CVI_ISP_V4L2_SetVoFd(int fd)
{
	vo_fd = fd;

	return 0;

}

int CVI_ISP_V4L2_GetVoFd(int *pfd)
{
	*pfd = vo_fd;

	return 0;
}
