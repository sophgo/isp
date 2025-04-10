
#ifndef _CLOG_H_
#define _CLOG_H_

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************/
#define CLOG_LINE_BUF_SIZE  (2 * 1024)

/* clog file cfg */
#define CLOG_FILE_PATH  "/mnt/sd/"
#define CLOG_FILE_MAX_PATH_LEN  64
#define CLOG_FILE_ENABLE_SINGLE_FILE 1
#define CLOG_FILE_SPLIT_SIZE (5 * 1024 * 1024)
#define CLOG_FILE_ASYNC_PINGPONG_BUF_SIZE (15 * 1024)

/*******************************************************************************/
#define CLOG_LVL_ASSERT                      0
#define CLOG_LVL_ERROR                       1
#define CLOG_LVL_WARN                        2
#define CLOG_LVL_INFO                        3
#define CLOG_LVL_DEBUG                       4
#define CLOG_LVL_VERBOSE                     5

#ifndef CLOG_OUTPUT_LVL
#define CLOG_OUTPUT_LVL CLOG_LVL_DEBUG
#endif

#if CLOG_OUTPUT_LVL >= CLOG_LVL_ASSERT
	#define clog_assert(tag, ...) \
			clog_output(CLOG_LVL_ASSERT, tag, __func__, __LINE__, __VA_ARGS__)
#else
	#define clog_assert(tag, ...)
#endif

#if CLOG_OUTPUT_LVL >= CLOG_LVL_ERROR
	#define clog_error(tag, ...) \
			clog_output(CLOG_LVL_ERROR, tag, __func__, __LINE__, __VA_ARGS__)
#else
	#define clog_error(tag, ...)
#endif

#if CLOG_OUTPUT_LVL >= CLOG_LVL_WARN
	#define clog_warn(tag, ...) \
			clog_output(CLOG_LVL_WARN, tag, __func__, __LINE__, __VA_ARGS__)
#else
	#define clog_warn(tag, ...)
#endif

#if CLOG_OUTPUT_LVL >= CLOG_LVL_INFO
	#define clog_info(tag, ...) \
			clog_output(CLOG_LVL_INFO, tag, __func__, __LINE__, __VA_ARGS__)
#else
	#define clog_info(tag, ...)
#endif

#if CLOG_OUTPUT_LVL >= CLOG_LVL_DEBUG
	#define clog_debug(tag, ...) \
			clog_output(CLOG_LVL_DEBUG, tag, __func__, __LINE__, __VA_ARGS__)
#else
	#define elog_debug(tag, ...)
#endif

#if CLOG_OUTPUT_LVL >= CLOG_LVL_VERBOSE
	#define clog_verbose(tag, ...) \
			clog_output(CLOG_LVL_VERBOSE, tag, __func__, __LINE__, __VA_ARGS__)
#else
	#define clog_verbose(tag, ...)
#endif

#ifndef CLOG_TAG
#define CLOG_TAG "isp"
#endif

#define clog_a(...)     clog_assert(CLOG_TAG, __VA_ARGS__)
#define clog_e(...)     clog_error(CLOG_TAG, __VA_ARGS__)
#define clog_w(...)     clog_warn(CLOG_TAG, __VA_ARGS__)
#define clog_i(...)     clog_info(CLOG_TAG, __VA_ARGS__)
#define clog_d(...)     clog_debug(CLOG_TAG, __VA_ARGS__)
#define clog_v(...)     clog_verbose(CLOG_TAG, __VA_ARGS__)

#define CLOG_ASSERT(EXPR)                        \
	{if (!(EXPR)) {                              \
		clog_a("assert", "%s\n", __func__);       \
	}}

int clog_file_enable(void);
int clog_file_disable(void);
void clog_output(uint8_t level, const char *tag, const char *func,
	const long line, const char *format, ...);
void clog_output_raw(const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif
