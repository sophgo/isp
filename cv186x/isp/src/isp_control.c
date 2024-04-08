#include "cvi_base.h"

#include "isp_control.h"
#include "isp_debug.h"
#include "isp_ioctl.h"
#include "isp_3a.h"

#include "isp_comm_inc.h"
#ifndef V4L2_ISP_ENABLE
extern CVI_S32 get_vi_fd(void);
extern CVI_S32 vi_is_closed(CVI_VOID);
CVI_S32 isp_control_set_fd_info(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	ISP_CTX_S *pstIspCtx = NULL;

	CVI_S32	fd = get_vi_fd();

	ISP_GET_CTX(ViPipe, pstIspCtx);

	pstIspCtx->ispDevFd = fd;

	if (vi_is_closed()) {
		ISP_LOG_ERR("Pipe(%d) state(0) incorrect\n", ViPipe);
		return -EBADF;
	}

	return ret;
}
#else
extern int get_v4l2_fd(int pipe);
CVI_S32 isp_control_set_fd_info(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	ISP_CTX_S *pstIspCtx = NULL;

	int fd = get_v4l2_fd(ViPipe);

	ISP_GET_CTX(ViPipe, pstIspCtx);
	pstIspCtx->ispDevFd = fd;

	if (fd < 0) {
		ISP_LOG_ERR("pipe: %d, wrong fd: %d\n", ViPipe, fd);
		return -EBADF;
	}

	return ret;
}
#endif

CVI_S32 isp_control_set_scene_info(VI_PIPE ViPipe)
{
	CVI_U32 scene = 0;
	ISP_CTX_S *pstIspCtx = NULL;

	ISP_GET_CTX(ViPipe, pstIspCtx);

	G_EXT_CTRLS_VALUE(VI_IOCTL_GET_SCENE_INFO, 0, &scene);

	pstIspCtx->scene = scene;

	return CVI_SUCCESS;
}

CVI_S32 isp_control_get_scene_info(VI_PIPE ViPipe, enum ISP_SCENE_INFO *scene)
{
	ISP_CTX_S *pstIspCtx = NULL;

	ISP_GET_CTX(ViPipe, pstIspCtx);

	*scene = pstIspCtx->scene;

	return CVI_SUCCESS;
}
