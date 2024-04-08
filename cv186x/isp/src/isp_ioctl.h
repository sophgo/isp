/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_ioctl.h
 * Description:
 *
 */

#ifndef _ISP_IOCTL_H_
#define _ISP_IOCTL_H_

#include <sys/ioctl.h>
// #include <sys/time.h>

// #include <linux/vi_uapi.h>
// #include <linux/vi_isp.h>

#include "isp_defines.h"
#include "isp_main_local.h"

#ifdef V4L2_ISP_ENABLE
#include <linux/videodev2.h>
#endif
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#ifndef V4L2_ISP_ENABLE

struct isp_ioctl_param {
	VI_PIPE ViPipe;
	CVI_U32 id;
	CVI_VOID *ptr;
	CVI_U32 ptr_size;
	CVI_U32 val;

	CVI_U32 *out_val;
};

#define S_EXT_CTRLS_PTR(_id, _ptr)\
	do {\
		ISP_CTX_S *pstIspCtx = NULL;\
		ISP_GET_CTX(ViPipe, pstIspCtx);\
		struct vi_ext_control ec1;\
		memset(&ec1, 0, sizeof(ec1));\
		ec1.id = _id;\
		ec1.ptr = (void *)_ptr;\
		ec1.sdk_cfg.pipe = ViPipe;\
		if (ioctl(pstIspCtx->ispDevFd, VI_IOC_S_CTRL, &ec1) < 0) {\
			ISP_IOCTL_ERR(pstIspCtx->ispDevFd, ec1);\
			return -1;\
		} \
	} while (0)

#define S_EXT_CTRLS_VALUE(_id, in, out)\
	do {\
		ISP_CTX_S *pstIspCtx = NULL;\
		ISP_GET_CTX(ViPipe, pstIspCtx);\
		struct vi_ext_control ec1;\
		memset(&ec1, 0, sizeof(ec1));\
		CVI_U32 val = in;\
		CVI_U32 *ptr = out;\
		ec1.id = _id;\
		ec1.value = val;\
		ec1.sdk_cfg.pipe = ViPipe;\
		if (ioctl(pstIspCtx->ispDevFd, VI_IOC_S_CTRL, &ec1) < 0) {\
			ISP_IOCTL_ERR(pstIspCtx->ispDevFd, ec1);\
			return -1;\
		} \
		if (ptr != CVI_NULL)\
			*ptr = ec1.value;\
	} while (0)

#define G_EXT_CTRLS_PTR(_id, _ptr)\
	do {\
		ISP_CTX_S *pstIspCtx = NULL;\
		ISP_GET_CTX(ViPipe, pstIspCtx);\
		struct vi_ext_control ec1;\
		memset(&ec1, 0, sizeof(ec1));\
		ec1.id = _id;\
		ec1.ptr = (void *)_ptr;\
		ec1.sdk_cfg.pipe = ViPipe;\
		if (ioctl(pstIspCtx->ispDevFd, VI_IOC_G_CTRL, &ec1) < 0) {\
			ISP_IOCTL_ERR(pstIspCtx->ispDevFd, ec1);\
			return -1;\
		} \
	} while (0)

#define G_EXT_CTRLS_VALUE(_id, in, out)\
	do {\
		ISP_CTX_S *pstIspCtx = NULL;\
		ISP_GET_CTX(ViPipe, pstIspCtx);\
		struct vi_ext_control ec1;\
		memset(&ec1, 0, sizeof(ec1));\
		CVI_U32 val = in;\
		CVI_U32 *ptr = out;\
		ec1.id = _id;\
		ec1.value = val;\
		ec1.sdk_cfg.pipe = ViPipe;\
		if (ioctl(pstIspCtx->ispDevFd, VI_IOC_G_CTRL, &ec1) < 0) {\
			ISP_IOCTL_ERR(pstIspCtx->ispDevFd, ec1);\
			return -1;\
		} \
		if (ptr != CVI_NULL)\
			*ptr = ec1.value;\
	} while (0)

#else

#define S_EXT_CTRLS_PTR(_id, _ptr)\
	do {\
		ISP_CTX_S *pstIspCtx = NULL;\
		ISP_GET_CTX(ViPipe, pstIspCtx);\
		struct v4l2_ext_controls ecs;\
		struct v4l2_ext_control ec;\
		memset(&ec, 0, sizeof(ec));\
		memset(&ecs, 0, sizeof(ecs));\
		ec.id = _id;\
		ec.ptr = (void *)_ptr;\
		ecs.count = 1;\
		ecs.controls = &ec;\
		if (ioctl(pstIspCtx->ispDevFd, VIDIOC_S_EXT_CTRLS, &ecs) < 0) {\
			ISP_IOCTL_V4L2_ERR(pstIspCtx->ispDevFd, ecs);\
			return -1;\
		} \
	} while (0)

#define G_EXT_CTRLS_PTR(_id, _ptr)\
	do {\
		ISP_CTX_S *pstIspCtx = NULL;\
		ISP_GET_CTX(ViPipe, pstIspCtx);\
		struct v4l2_ext_controls ecs;\
		struct v4l2_ext_control ec;\
		memset(&ec, 0, sizeof(ec));\
		memset(&ecs, 0, sizeof(ecs));\
		ec.id = _id;\
		ec.ptr = (void *)_ptr;\
		ecs.count = 1;\
		ecs.controls = &ec;\
		if (ioctl(pstIspCtx->ispDevFd, VIDIOC_G_EXT_CTRLS, &ecs) < 0) {\
			ISP_IOCTL_V4L2_ERR(pstIspCtx->ispDevFd, ecs);\
			return -1;\
		} \
	} while (0)

#define S_EXT_CTRLS_VALUE(_id, in, out)\
	do {\
		ISP_CTX_S *pstIspCtx = NULL;\
		ISP_GET_CTX(ViPipe, pstIspCtx);\
		CVI_U32 val = in;\
		CVI_U32 *ptr = out;\
		struct v4l2_ext_controls ecs;\
		struct v4l2_ext_control ec;\
		memset(&ec, 0, sizeof(ec));\
		memset(&ecs, 0, sizeof(ecs));\
		ec.id = _id;\
		ec.value = val;\
		ecs.count = 1;\
		ecs.controls = &ec;\
		if (ioctl(pstIspCtx->ispDevFd, VIDIOC_S_EXT_CTRLS, &ecs) < 0) {\
			ISP_IOCTL_V4L2_ERR(pstIspCtx->ispDevFd, ecs);\
			return -1;\
		} \
		if (ptr != CVI_NULL)\
			*ptr = ecs.controls->value;\
	} while (0)

#define G_EXT_CTRLS_VALUE(_id, in, out)\
	do {\
		ISP_CTX_S *pstIspCtx = NULL;\
		ISP_GET_CTX(ViPipe, pstIspCtx);\
		CVI_U32 val = in;\
		CVI_U32 *ptr = out;\
		struct v4l2_ext_controls ecs;\
		struct v4l2_ext_control ec;\
		memset(&ec, 0, sizeof(ec));\
		memset(&ecs, 0, sizeof(ecs));\
		ec.id = _id;\
		ec.value = val;\
		ecs.count = 1;\
		ecs.controls = &ec;\
		if (ioctl(pstIspCtx->ispDevFd, VIDIOC_G_EXT_CTRLS, &ecs) < 0) {\
			ISP_IOCTL_V4L2_ERR(pstIspCtx->ispDevFd, ecs);\
			return -1;\
		} \
		if (ptr != CVI_NULL)\
			*ptr = ecs.controls->value;\
	} while (0)

#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_IOCTL_H_
