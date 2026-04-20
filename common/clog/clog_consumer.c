#define _GNU_SOURCE
#include "clog.h"
#if CLOG_ENABLE_CONSUMER
#include "clog_consumer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/time.h>

#define CLOG_DEFAULT_FILE_PATH  "/tmp"
#define CLOG_DEFAULT_FILE_SPLIT_SIZE (1 * 1024 * 1024) /* 1MB */
#define CLOG_DEFAULT_TCP_PORT  5568

/* Consumer context */
struct clog_consumer_ctx {
	/* Ring buffer reference */
	clog_ringbuf_t *ringbuf;

	/* Thread control */
	pthread_t thread;
	volatile int running;

	/* Output mode */
	clog_output_mode_t mode;

	/* File output */
	FILE *fp;
	char file_path[256];
	uint32_t file_split_size;
	uint32_t file_size;
	uint32_t file_count;

	/* TCP output (server mode) */
	int tcp_listen_fd;
	int tcp_client_fd;
	uint16_t tcp_port;
};

/*******************************************************************************/
/* File output functions */
/*******************************************************************************/

static int clog_file_open(clog_consumer_ctx_t *ctx)
{
	char path_buf[512];

	if (ctx->fp) {
		fclose(ctx->fp);
		ctx->fp = NULL;
	}

	snprintf(path_buf, sizeof(path_buf), "%s/isplog-%d-%02d.txt",
		ctx->file_path, (int) getpid(), (int) ctx->file_count);

	ctx->fp = fopen(path_buf, "w");
	if (ctx->fp == NULL) {
		printf("ERROR: clog cannot open: %s\n", path_buf);
		return -1;
	}

	ctx->file_count++;
	return 0;
}

static void clog_file_close(clog_consumer_ctx_t *ctx)
{
	if (ctx->fp) {
		fclose(ctx->fp);
		ctx->fp = NULL;
	}
}

static int clog_file_write(clog_consumer_ctx_t *ctx, const char *data, uint32_t len)
{
	if (ctx->fp == NULL) {
		if (clog_file_open(ctx) != 0) {
			return -1;
		}
	}

	fwrite(data, len, 1, ctx->fp);
	fflush(ctx->fp);

	ctx->file_size += len;

	/* Check if need to split file */
	if (ctx->file_size >= ctx->file_split_size) {
		clog_file_open(ctx);
		ctx->file_size = 0;
	}

	return 0;
}

/*******************************************************************************/
/* TCP output functions (Server Mode) */
/*******************************************************************************/

static int clog_tcp_server_init(clog_consumer_ctx_t *ctx)
{
	struct sockaddr_in server_addr;
	int reuse = 1;

	if (ctx->tcp_listen_fd >= 0) {
		close(ctx->tcp_listen_fd);
		ctx->tcp_listen_fd = -1;
	}

	ctx->tcp_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (ctx->tcp_listen_fd < 0) {
		printf("ERROR: socket create failed: %s\n", strerror(errno));
		return -1;
	}

	/* Enable address reuse */
	if (setsockopt(ctx->tcp_listen_fd, SOL_SOCKET, SO_REUSEADDR,
				   &reuse, sizeof(reuse)) < 0) {
		printf("WARN: setsockopt SO_REUSEADDR failed: %s\n", strerror(errno));
	}

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(ctx->tcp_port);

	if (bind(ctx->tcp_listen_fd, (struct sockaddr *)&server_addr,
			 sizeof(server_addr)) < 0) {
		printf("ERROR: bind failed on port %d: %s\n",
			   ctx->tcp_port, strerror(errno));
		close(ctx->tcp_listen_fd);
		ctx->tcp_listen_fd = -1;
		return -1;
	}

	if (listen(ctx->tcp_listen_fd, 1) < 0) {
		printf("ERROR: listen failed: %s\n", strerror(errno));
		close(ctx->tcp_listen_fd);
		ctx->tcp_listen_fd = -1;
		return -1;
	}

	printf("INFO: clog TCP server listening on port %d\n", ctx->tcp_port);
	return 0;
}

static int clog_tcp_accept_client(clog_consumer_ctx_t *ctx)
{
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);
	int client_fd;
	struct timeval tv;

	if (ctx->tcp_listen_fd < 0) {
		if (clog_tcp_server_init(ctx) != 0) {
			return -1;
		}
	}

	/* Set non-blocking timeout for accept */
	tv.tv_sec = 0;
	tv.tv_usec = 100000;  /* 100ms timeout */
	setsockopt(ctx->tcp_listen_fd, SOL_SOCKET, SO_RCVTIMEO,
			   &tv, sizeof(tv));

	client_fd = accept(ctx->tcp_listen_fd,
					   (struct sockaddr *)&client_addr, &client_len);
	if (client_fd < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			/* No client waiting, not an error */
			return -1;
		}
		printf("WARN: accept failed: %s\n", strerror(errno));
		return -1;
	}

	/* Close old client if exists */
	if (ctx->tcp_client_fd >= 0) {
		close(ctx->tcp_client_fd);
	}

	ctx->tcp_client_fd = client_fd;
	printf("INFO: clog client connected from %s:%d\n",
		   inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

	return 0;
}

static void clog_tcp_close(clog_consumer_ctx_t *ctx)
{
	if (ctx->tcp_client_fd >= 0) {
		close(ctx->tcp_client_fd);
		ctx->tcp_client_fd = -1;
	}
	if (ctx->tcp_listen_fd >= 0) {
		close(ctx->tcp_listen_fd);
		ctx->tcp_listen_fd = -1;
	}
}

static int clog_tcp_write(clog_consumer_ctx_t *ctx, const char *data, uint32_t len)
{
	int ret;

	/* Try to accept client if not connected */
	if (ctx->tcp_client_fd < 0) {
		clog_tcp_accept_client(ctx);
		/* If still no client, just drop the log */
		if (ctx->tcp_client_fd < 0) {
			return -1;
		}
	}

	ret = send(ctx->tcp_client_fd, data, len, MSG_NOSIGNAL);
	if (ret < 0) {
		/* Client disconnected, close and wait for new connection */
		printf("WARN: send failed, client disconnected: %s\n", strerror(errno));
		close(ctx->tcp_client_fd);
		ctx->tcp_client_fd = -1;
		return -1;
	}

	return 0;
}

/*******************************************************************************/
/* Consumer processing */
/*******************************************************************************/

int clog_consumer_process_one(clog_consumer_ctx_t *ctx)
{
	const char *data;
	uint32_t len;
	int ret;

	/* Try to read from ring buffer (zero-copy) */
	ret = clog_ringbuf_read_zerocopy(ctx->ringbuf, &data, &len);
	if (ret != 0) {
		return -1;  /* No data available */
	}

	/* Output based on mode */
	switch (ctx->mode) {
	case CLOG_OUTPUT_FILE:
		clog_file_write(ctx, data, len);
		break;
	case CLOG_OUTPUT_TCP:
		clog_tcp_write(ctx, data, len);
		break;
	}

	/* Commit read after processing */
	clog_ringbuf_read_commit(ctx->ringbuf, len);

	return 0;
}

static void *clog_consumer_thread(void *param)
{
	clog_consumer_ctx_t *ctx = (clog_consumer_ctx_t *)param;

	pthread_setname_np(pthread_self(), "clog");

	while (ctx->running) {
		if (clog_consumer_process_one(ctx) != 0) {
			/* No data, sleep a bit */
			usleep(10 * 1000); /* 10ms */
		}
	}

	return NULL;
}

/*******************************************************************************/
/* Consumer lifecycle */
/*******************************************************************************/

clog_consumer_ctx_t *clog_consumer_create(clog_ringbuf_t *ringbuf,
										  const clog_consumer_config_t *config)
{
	clog_consumer_ctx_t *ctx;

	if (!ringbuf || !config) {
		return NULL;
	}

	ctx = (clog_consumer_ctx_t *)calloc(1, sizeof(clog_consumer_ctx_t));
	if (!ctx) {
		return NULL;
	}

	/* Initialize context */
	ctx->ringbuf = ringbuf;
	ctx->mode = config->mode;
	ctx->running = 0;
	ctx->tcp_listen_fd = -1;
	ctx->tcp_client_fd = -1;

	/* Setup output mode */
	switch (config->mode) {
	case CLOG_OUTPUT_FILE:
		strncpy(ctx->file_path,
				config->file_path ? config->file_path : CLOG_DEFAULT_FILE_PATH,
				sizeof(ctx->file_path) - 1);
		ctx->file_split_size = config->file_split_size > 0 ?
							   config->file_split_size : CLOG_DEFAULT_FILE_SPLIT_SIZE;
		ctx->file_size = 0;
		ctx->file_count = 0;
		ctx->fp = NULL;
		break;

	case CLOG_OUTPUT_TCP:
		ctx->tcp_port = config->tcp_port > 0 ? config->tcp_port : CLOG_DEFAULT_TCP_PORT;
		/* Initialize TCP server (listening socket) */
		if (clog_tcp_server_init(ctx) != 0) {
			printf("WARN: Failed to initialize TCP server, will retry later\n");
		}
		break;

	default:
		break;
	}

	return ctx;
}

int clog_consumer_start(clog_consumer_ctx_t *ctx)
{
	if (!ctx || ctx->running) {
		return -1;
	}

	ctx->running = 1;
	if (pthread_create(&ctx->thread, NULL, clog_consumer_thread, ctx) != 0) {
		ctx->running = 0;
		return -1;
	}

	return 0;
}

void clog_consumer_destroy(clog_consumer_ctx_t *ctx)
{
	if (!ctx) {
		return;
	}

	/* Stop thread if running */
	if (ctx->running) {
		ctx->running = 0;
		pthread_join(ctx->thread, NULL);
	}

	/* Cleanup resources */
	if (ctx->mode == CLOG_OUTPUT_FILE) {
		clog_file_close(ctx);
	} else if (ctx->mode == CLOG_OUTPUT_TCP) {
		clog_tcp_close(ctx);
	}

	free(ctx);
}
#endif /* CLOG_ENABLE_CONSUMER */
