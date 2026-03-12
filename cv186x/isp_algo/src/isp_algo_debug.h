/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_debug.h
 * Description:
 *
 */

#ifndef _ISP_ALGO_DEBUG_H_
#define _ISP_ALGO_DEBUG_H_

#include <syslog.h>
#include "cvi_debug.h"

#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif

#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef MINMAX
#define MINMAX(a, b, c) ((a < b) ? b : (a > c) ? c : a)
#endif

#define ISP_ALGO_DEBUG_LEVEL			(4)
#define ISP_ALGO_EXPORT_LOG_TO_STDOUT	(0)

static const char ISP_ALGO_DEBUG_LEVEL_SYMBOL[] = {'e', 'A', 'C', 'E', 'W', 'N', 'I', 'D'};

#define ISP_ALGO_DEBUG(level, fmt, ...) do { \
	if (level <= ISP_ALGO_DEBUG_LEVEL) { \
		if (level <= LOG_NOTICE) { \
			CVI_TRACE(level, CVI_ID_ISP, "[%c] %s::%d : " fmt, \
				ISP_ALGO_DEBUG_LEVEL_SYMBOL[level], __func__, __LINE__, ##__VA_ARGS__); \
		} else { \
			CVI_TRACE(level, CVI_ID_ISP, "%s : " fmt, __func__, ##__VA_ARGS__); \
		} \
		if (ISP_ALGO_EXPORT_LOG_TO_STDOUT) { \
			printf("[%c] %s::%d : " fmt, \
				ISP_ALGO_DEBUG_LEVEL_SYMBOL[level], __func__, __LINE__, ##__VA_ARGS__); \
		} \
	} \
} while (0)

#define ISP_ALGO_LOG_EMERG(fmt, ...)	ISP_ALGO_DEBUG(LOG_EMERG, fmt, ##__VA_ARGS__)
#define ISP_ALGO_LOG_ALERT(fmt, ...)	ISP_ALGO_DEBUG(LOG_ALERT, fmt, ##__VA_ARGS__)
#define ISP_ALGO_LOG_CRIT(fmt, ...)		ISP_ALGO_DEBUG(LOG_CRIT, fmt, ##__VA_ARGS__)

#ifndef IGNORE_LOG_ERR
#define ISP_ALGO_LOG_ERR(fmt, ...)		ISP_ALGO_DEBUG(LOG_ERR, fmt, ##__VA_ARGS__)
#else
#define ISP_ALGO_LOG_ERR(fmt, ...)
#endif // IGNORE_LOG_ERR

#ifndef IGNORE_LOG_WARNING
#define ISP_ALGO_LOG_WARNING(fmt, ...)	ISP_ALGO_DEBUG(LOG_WARNING, fmt, ##__VA_ARGS__)
#else
#define ISP_ALGO_LOG_WARNING(fmt, ...)
#endif // IGNORE_LOG_WARNING

#ifndef IGNORE_LOG_NOTICE
#define ISP_ALGO_LOG_NOTICE(fmt, ...)	ISP_ALGO_DEBUG(LOG_NOTICE, fmt, ##__VA_ARGS__)
#else
#define ISP_ALGO_LOG_NOTICE(fmt, ...)
#endif // IGNORE_LOG_NOTICE

#ifndef IGNORE_LOG_INFO
#define ISP_ALGO_LOG_INFO(fmt, ...)		ISP_ALGO_DEBUG(LOG_INFO, fmt, ##__VA_ARGS__)
#else
#define ISP_ALGO_LOG_INFO(fmt, ...)
#endif // IGNORE_LOG_INFO

#ifndef IGNORE_LOG_DEBUG
#define ISP_ALGO_LOG_DEBUG(fmt, ...)	ISP_ALGO_DEBUG(LOG_DEBUG, fmt, ##__VA_ARGS__)
#else
#define ISP_ALGO_LOG_DEBUG(fmt, ...)
#endif // IGNORE_LOG_DEBUG

#endif // _ISP_ALGO_DEBUG_H_
