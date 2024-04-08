#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/param.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <math.h>
#include <inttypes.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/mman.h>

#include "cvi_buffer.h"
#include "cvi_base.h"
#include "cvi_vpss.h"
#include "cvi_sys.h"

#define UNUSED(x) ((void)(x))

static VPSS_BIN_DATA vpss_bin_data[VPSS_MAX_GRP_NUM];
static CVI_BOOL g_bLoadBinDone = CVI_FALSE;

CVI_VOID set_loadbin_state(CVI_BOOL done)
{
	g_bLoadBinDone = done;
}

VPSS_BIN_DATA *get_vpssbindata_addr(void)
{
	return vpss_bin_data;
}

CVI_S32 CVI_VPSS_GetGrpProcAmp(VPSS_GRP VpssGrp, PROC_AMP_E type, CVI_S32 *value)
{
	UNUSED(VpssGrp);
	UNUSED(type);
	UNUSED(value);
	return 0;
}

CVI_S32 CVI_VPSS_SetGrpProcAmp(VPSS_GRP VpssGrp, PROC_AMP_E type, const CVI_S32 value)
{
	UNUSED(VpssGrp);
	UNUSED(type);
	UNUSED(value);
	return 0;
}

CVI_S32 CVI_VPSS_GetChnCrop(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VPSS_CROP_INFO_S *pstCropInfo)
{
	UNUSED(VpssGrp);
	UNUSED(VpssChn);
	UNUSED(pstCropInfo);
	return 0;
}

CVI_S32 CVI_VPSS_GetChnFrame(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VIDEO_FRAME_INFO_S *pstFrameInfo,
			     CVI_S32 s32MilliSec)
{
	UNUSED(VpssGrp);
	UNUSED(VpssChn);
	UNUSED(pstFrameInfo);
	UNUSED(s32MilliSec);
	return 0;
}

CVI_S32 CVI_VPSS_ReleaseChnFrame(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, const VIDEO_FRAME_INFO_S *pstVideoFrame)
{
	UNUSED(VpssGrp);
	UNUSED(VpssChn);
	UNUSED(pstVideoFrame);
	return 0;
}

CVI_S32 CVI_VPSS_SetChnLDCAttr(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, const VPSS_LDC_ATTR_S *pstLDCAttr)
{
	UNUSED(VpssGrp);
	UNUSED(VpssChn);
	UNUSED(pstLDCAttr);
	return 0;
}

CVI_S32 CVI_VPSS_GetChnLDCAttr(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VPSS_LDC_ATTR_S *pstLDCAttr)
{
	UNUSED(VpssGrp);
	UNUSED(VpssChn);
	UNUSED(pstLDCAttr);
	return 0;
}

CVI_S32 CVI_VPSS_SetGrpParamfromBin(VPSS_GRP VpssGrp, CVI_U8 scene)
{
	UNUSED(VpssGrp);
	UNUSED(scene);
	return 0;
}

