#ifndef _RAW_REPLAY_OFFLINE_H_
#define _RAW_REPLAY_OFFLINE_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#include "cvi_comm_isp.h"

CVI_S32 raw_replay_offline_init(char *boardPath);
void raw_replay_offline_uninit(void);
CVI_S32 start_raw_replay_offline(VI_PIPE ViPipe);
CVI_S32 stop_raw_replay_offline(void);
void replay_one_frame(VI_PIPE ViPipe, CVI_U32 curFrame, ISP_MWB_ATTR_S *stMWBAttr, ISP_EXP_INFO_S *stExpInfo);
CVI_U32 get_numFrames(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _RAW_REPLAY_OFFLINE_H_
