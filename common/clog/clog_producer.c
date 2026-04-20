#include "clog_producer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#include <sys/syscall.h>
#define gettid() syscall(SYS_gettid)

#ifndef UNUSED
#define UNUSED(x) ((void)(x))
#endif

#define CLOG_TEMP_BUF_SIZE 128

/* Producer context */
struct clog_producer_ctx {
	clog_ringbuf_t *ringbuf;  /* NULL for console-only mode */
};

/*******************************************************************************/
/* Helper functions */
/*******************************************************************************/

static int clog_get_time(char *time_str, size_t size)
{
	struct timeval tv;
	struct tm cur_tm;

	gettimeofday(&tv, NULL);
	localtime_r(&tv.tv_sec, &cur_tm);

	snprintf(time_str, size, "%02d%02d %02d:%02d:%02d.%03d ",
		cur_tm.tm_mon + 1,
		cur_tm.tm_mday,
		cur_tm.tm_hour,
		cur_tm.tm_min,
		cur_tm.tm_sec,
		(int)(tv.tv_usec / 1000));

	return 0;
}

static const char *clog_get_tid(char *tid_str, size_t size)
{
	snprintf(tid_str, size, "%04ld ", gettid());
	return tid_str;
}

static const char * const level_output_info[] = {
	[CLOG_LVL_ASSERT]  = "A ",
	[CLOG_LVL_ERROR]   = "E ",
	[CLOG_LVL_WARN]    = "W ",
	[CLOG_LVL_INFO]    = "I ",
	[CLOG_LVL_DEBUG]   = "D ",
	[CLOG_LVL_VERBOSE] = "V ",
};

static size_t clog_strcpy(size_t cur_len, char *dst, const char *src)
{
	const char *src_old = src;

	while (*src != 0) {
		if (cur_len++ < CLOG_LINE_BUF_SIZE) {
			*dst++ = *src++;
		} else {
			break;
		}
	}

	return src - src_old;
}

/*******************************************************************************/
/* Producer lifecycle */
/*******************************************************************************/

clog_producer_ctx_t *clog_producer_create(clog_ringbuf_t *ringbuf)
{
	clog_producer_ctx_t *ctx;

	ctx = (clog_producer_ctx_t *)calloc(1, sizeof(clog_producer_ctx_t));
	if (!ctx) {
		return NULL;
	}

	ctx->ringbuf = ringbuf;
	return ctx;
}

void clog_producer_destroy(clog_producer_ctx_t *ctx)
{
	if (ctx) {
		free(ctx);
	}
}

/*******************************************************************************/
/* Producer APIs (thread-safe) */
/*******************************************************************************/

void clog_producer_output_va(clog_producer_ctx_t *ctx, uint8_t level,
							 const char *tag, const char *func, const long line,
							 const char *format, va_list args)
{
	char temp_buf[CLOG_TEMP_BUF_SIZE];
	char log_buf[CLOG_LINE_BUF_SIZE];
	size_t log_len = 0;
	int fmt_result;

	assert(level <= CLOG_LVL_VERBOSE);

	/* Build log line in local buffer */
	clog_get_time(temp_buf, CLOG_TEMP_BUF_SIZE);
	log_len += clog_strcpy(log_len, log_buf + log_len, temp_buf);

	clog_get_tid(temp_buf, CLOG_TEMP_BUF_SIZE);
	log_len += clog_strcpy(log_len, log_buf + log_len, temp_buf);

	log_len += clog_strcpy(log_len, log_buf + log_len, level_output_info[level]);
	log_len += clog_strcpy(log_len, log_buf + log_len, tag);
	log_len += clog_strcpy(log_len, log_buf + log_len, " ");
	log_len += clog_strcpy(log_len, log_buf + log_len, func);
	log_len += clog_strcpy(log_len, log_buf + log_len, ":");

	snprintf(temp_buf, CLOG_TEMP_BUF_SIZE, "%ld ", line);
	log_len += clog_strcpy(log_len, log_buf + log_len, temp_buf);

	fmt_result = vsnprintf(log_buf + log_len, CLOG_LINE_BUF_SIZE - log_len, format, args);
	log_len += fmt_result;

	if (ctx != NULL && ctx->ringbuf) {
		/* Try to write to ring buffer */
		if (clog_ringbuf_write(ctx->ringbuf, log_buf, log_len) != 0) {
			printf("ERROR: log ringbuf full, dropping log\n");
		}

		if (level <= CLOG_LVL_ERROR) {
			printf("%s", log_buf);
		}
	} else {
		/* No ring buffer, output to console */
		printf("%s", log_buf);
	}

	if (level == CLOG_LVL_ASSERT) {
		assert(0);
	}
}

void clog_producer_output(clog_producer_ctx_t *ctx, uint8_t level,
						  const char *tag, const char *func, const long line,
						  const char *format, ...)
{
	va_list args;

	va_start(args, format);
	clog_producer_output_va(ctx, level, tag, func, line, format, args);
	va_end(args);
}

void clog_producer_output_raw(clog_producer_ctx_t *ctx,
							   const char *format, ...)
{
	char log_buf[CLOG_LINE_BUF_SIZE];
	va_list args;
	int fmt_result;

	va_start(args, format);

	/* Build log in local buffer */
	fmt_result = vsnprintf(log_buf, CLOG_LINE_BUF_SIZE, format, args);
	va_end(args);

	if (ctx != NULL && ctx->ringbuf) {
		/* Try to write to ring buffer, drop if full */
		clog_ringbuf_write(ctx->ringbuf, log_buf, fmt_result);
	} else {
		/* No ring buffer, output to console */
		printf("%s", log_buf);
	}
}
