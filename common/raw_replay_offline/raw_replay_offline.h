#ifndef _RAW_REPLAY_OFFLINE_H_
#define _RAW_REPLAY_OFFLINE_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

CVI_S32 raw_replay_offline_init(char *boardPath);
void raw_replay_offline_uninit(void);
CVI_S32 start_raw_replay_offline(VI_PIPE ViPipe);
CVI_S32 stop_raw_replay_offline(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _RAW_REPLAY_OFFLINE_H_
