
#ifndef __CVI_ISP_V4L2_H__
#define __CVI_ISP_V4L2_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

int CVI_ISP_V4L2_SetFd(int pipe, int fd);
int CVI_ISP_V4L2_GetFd(int pipe, int *pfd);

int CVI_ISP_V4L2_SetVoFd(int fd);
int CVI_ISP_V4L2_GetVoFd(int *pfd);

int CVI_ISP_V4L2_Init(int pipe, int fd);
int CVI_ISP_V4L2_Exit(int pipe);

CVI_VOID CVI_ISP_SetDISInfoCallback(CVI_VOID *pcallbackFunc);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif

