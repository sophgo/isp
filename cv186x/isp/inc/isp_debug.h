/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_debug.h
 * Description:
 *
 */

#ifndef _ISP_DEBUG_H_
#define _ISP_DEBUG_H_

#include <stdio.h>
#include "isp_comm_inc.h"
#include "clog.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

void CVI_DEBUG_SetDebugLevel(int level);
CVI_S32 isp_dbg_dumpFrameRawInfoToFile(VI_PIPE ViPipe, FILE *fp);

void isp_dbg_init(void);
void isp_dbg_deinit(void);
CVI_U32 isp_dbg_get_time_diff_us(const struct timeval *pre, const struct timeval *cur);

extern int g_isp_debug_level;

#define ISP_DEBUG(level, ...) do { \
	if (level <= g_isp_debug_level) { \
		clog_output(level, CLOG_TAG, __func__, __LINE__, __VA_ARGS__); \
	} \
} while (0)

#define IGNORE_LOG_DEBUG
//#define IGNORE_LOG_INFO
//#define IGNORE_LOG_NOTICE
//#define IGNORE_LOG_WARNING
//#define IGNORE_LOG_ERR
//#define IGNORE_LOG_RAW

#define ISP_LOG_ASSERT(EXPR) CLOG_ASSERT(EXPR)
#ifndef IGNORE_LOG_ERR
#define ISP_LOG_ERR(...) ISP_DEBUG(CLOG_LVL_ERROR, __VA_ARGS__)
#else
#define ISP_LOG_ERR(...)
#endif // IGNORE_LOG_ERR
#ifndef IGNORE_LOG_WARNING
#define ISP_LOG_WARNING(...) ISP_DEBUG(CLOG_LVL_WARN, __VA_ARGS__)
#else
#define ISP_LOG_WARNING(...)
#endif // IGNORE_LOG_WARNING
#ifndef IGNORE_LOG_NOTICE
#define ISP_LOG_NOTICE(...) ISP_DEBUG(CLOG_LVL_WARN, __VA_ARGS__)
#else
#define ISP_LOG_NOTICE(...)
#endif // IGNORE_LOG_NOTICE
#ifndef IGNORE_LOG_INFO
#define ISP_LOG_INFO(...) ISP_DEBUG(CLOG_LVL_INFO, __VA_ARGS__)
#else
#define ISP_LOG_INFO(...)
#endif // IGNORE_LOG_INFO
#ifndef IGNORE_LOG_DEBUG
#define ISP_LOG_DEBUG(...) ISP_DEBUG(CLOG_LVL_DEBUG, __VA_ARGS__)
#else
#define ISP_LOG_DEBUG(...)
#endif // IGNORE_LOG_DEBUG
#ifndef IGNORE_LOG_RAW
#define ISP_LOG_RAW(...) clog_output_raw(__VA_ARGS__)
#else
#define ISP_LOG_RAW(...)
#endif // IGNORE_LOG_RAW

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif //_ISP_DEBUG_H_
