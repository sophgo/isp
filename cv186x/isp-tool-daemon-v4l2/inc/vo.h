
#ifndef __VO_H__
#define __VO_H__

#include <stdint.h>
#include "utils.h"

#define VO_HDMI_CRTC_ID (40)
#define VO_HDMI_CONN_ID (44)

int start_vo(RTSP_CFG *p_rtsp_cfg);
int stop_vo(RTSP_CFG *p_rtsp_cfg);

#endif

