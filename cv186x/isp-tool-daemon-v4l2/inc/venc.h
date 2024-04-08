#ifndef __VENC_H__
#define __VENC_H__

#include "utils.h"

int start_venc(RTSP_CFG *p_rtsp_cfg);
int stop_venc(RTSP_CFG *p_rtsp_cfg);

int get_stream(int chn, void *pstream);
int put_stream(int chn, void *pstream);

#endif

