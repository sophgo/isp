
#include "clog.h"
#include "clog_ringbuf.h"
#include "clog_producer.h"
#if CLOG_ENABLE_CONSUMER
#include "clog_consumer.h"
#endif
#include <stdlib.h>
#include <stdarg.h>

/* Global log context */
static struct {
	clog_ringbuf_t ringbuf;
	clog_producer_ctx_t *producer;
#if CLOG_ENABLE_CONSUMER
	clog_consumer_ctx_t *consumer;
#endif
	int initialized;
} g_clog_ctx = {
	.producer = NULL,
#if CLOG_ENABLE_CONSUMER
	.consumer = NULL,
#endif
	.initialized = 0
};

/*******************************************************************************/
/* Public APIs */
/*******************************************************************************/

int clog_init(const clog_config_t *config)
{
#if CLOG_ENABLE_CONSUMER
	clog_consumer_config_t consumer_cfg;
#endif

	if (g_clog_ctx.initialized) {
		return -1; /* Already initialized */
	}

	/* Initialize ring buffer */
	if (clog_ringbuf_init(&g_clog_ctx.ringbuf, config->ringbuf_size) != 0) {
		printf("ERROR: clog ring buffer init failed\n");
		return -1;
	}

	/* Create producer */
	g_clog_ctx.producer = clog_producer_create(&g_clog_ctx.ringbuf);
	if (!g_clog_ctx.producer) {
		clog_ringbuf_deinit(&g_clog_ctx.ringbuf);
		printf("ERROR: clog producer create failed\n");
		return -1;
	}

#if CLOG_ENABLE_CONSUMER
	/* Create and start consumer */
	consumer_cfg.mode = config->mode;
	consumer_cfg.file_path = config->file_path;
	consumer_cfg.file_split_size = config->file_split_size;
	consumer_cfg.tcp_port = config->tcp_port;

	g_clog_ctx.consumer = clog_consumer_create(&g_clog_ctx.ringbuf, &consumer_cfg);
	if (!g_clog_ctx.consumer) {
		clog_producer_destroy(g_clog_ctx.producer);
		clog_ringbuf_deinit(&g_clog_ctx.ringbuf);
		printf("ERROR: clog consumer create failed\n");
		return -1;
	}

	if (clog_consumer_start(g_clog_ctx.consumer) != 0) {
		clog_consumer_destroy(g_clog_ctx.consumer);
		clog_producer_destroy(g_clog_ctx.producer);
		clog_ringbuf_deinit(&g_clog_ctx.ringbuf);
		printf("ERROR: clog consumer start failed\n");
		return -1;
	}
#endif

	g_clog_ctx.initialized = 1;
	return 0;
}

int clog_deinit(void)
{
	if (!g_clog_ctx.initialized) {
		return -1;
	}

#if CLOG_ENABLE_CONSUMER
	/* Stop and cleanup consumer */
	if (g_clog_ctx.consumer) {
		clog_consumer_destroy(g_clog_ctx.consumer);
		g_clog_ctx.consumer = NULL;
	}
#endif

	/* Cleanup producer */
	if (g_clog_ctx.producer) {
		clog_producer_destroy(g_clog_ctx.producer);
		g_clog_ctx.producer = NULL;
	}

	/* Cleanup ring buffer */
	clog_ringbuf_deinit(&g_clog_ctx.ringbuf);

	g_clog_ctx.initialized = 0;
	return 0;
}

/*******************************************************************************/
/* Log output APIs (delegates to producer) */
/*******************************************************************************/

void clog_output(uint8_t level, const char *tag, const char *func,
				 const long line, const char *format, ...)
{
	va_list args;

	va_start(args, format);
	clog_producer_output_va(g_clog_ctx.producer, level, tag, func, line, format, args);
	va_end(args);
}

void clog_output_raw(const char *format, ...)
{
	va_list args;

	va_start(args, format);
	clog_producer_output_raw(g_clog_ctx.producer, format, args);
	va_end(args);
}
