#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <errno.h>

#include "vpss_ioctl.h"

#define UNUSED(x) ((void)(x))

CVI_S32 vpss_get_all_proc_amp(CVI_S32 fd, struct vpss_all_proc_amp_cfg *cfg)
{
	UNUSED(fd);
	for (int i = 0; i < VPSS_MAX_GRP_NUM; ++i) {
		for (int j = 0; j < PROC_AMP_MAX; ++j) {
			cfg->proc_amp[i][j] = 50;
		}
	}

	return 0;
}
