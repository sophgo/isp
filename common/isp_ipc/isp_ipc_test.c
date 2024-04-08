#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/epoll.h>

#include "clog.h"
#include "isp_debug.h"
#include "isp_ipc_common.h"

#ifndef UNUSED
#define UNUSED(x) ((void)(x))
#endif

#include "isp_ipc_common.h"

#define ISP_IPC_TEST_ENABLE 0

#if ISP_IPC_TEST_ENABLE

typedef void (*ISP_IPC_TEST_FUN)(void);

typedef enum {
	ISP_IPC_SERVER_CMD_01,
	ISP_IPC_SERVER_CMD_02,
	ISP_IPC_SERVER_CMD_03,
	ISP_IPC_SERVER_CMD_04,
	ISP_IPC_SERVER_CMD_05,
	ISP_IPC_SERVER_CMD_MAX
} ISP_IPC_SERVER_CMD_E;

static uint32_t getTimeDiffus(const struct timeval *tv1, const struct timeval *tv2)
{
	return ((tv2->tv_sec - tv1->tv_sec) * 1000000 + (tv2->tv_usec - tv1->tv_usec));
}

static void client_send_cmd_01(void)
{
	char *arg = "how are you...";
	char res[64];
	ISP_IPC_MSG_S msg;

	msg.cmd = ISP_IPC_SERVER_CMD_01;
	msg.arg_len = strlen(arg);
	msg.res_len = 64;

	struct timeval tv1, tv2;

	gettimeofday(&tv1, NULL);
	client_send_cmd_to_server(&msg, (CVI_U8 *)arg, (CVI_U8 *)res);
	gettimeofday(&tv2, NULL);

	printf("cmd 1: %s, diff: %d\n", res, getTimeDiffus(&tv1, &tv2));
}

static CVI_S32 server_cmd_01(CVI_U8 *arg, CVI_U32 arg_len, CVI_U8 *res, CVI_U32 res_len)
{
	UNUSED(arg_len);

	printf("cmd 1: %s\n", arg);
	snprintf((char *)res, res_len, "%s", "i am fine...");
	return CVI_SUCCESS;
}

static void client_send_cmd_02(void)
{
	ISP_IPC_MSG_S msg;
	CVI_U32 test_len = 1024;

	CVI_U8 *arg = calloc(1, test_len);
	CVI_U8 *res = calloc(1, test_len);

	memset(arg, 0xAA, test_len);
	memset(res, 0x00, test_len);

	msg.cmd = ISP_IPC_SERVER_CMD_02;
	msg.arg_len = test_len;
	msg.res_len = test_len;

	struct timeval tv1, tv2;

	gettimeofday(&tv1, NULL);
	client_send_cmd_to_server(&msg, arg, res);
	gettimeofday(&tv2, NULL);

	printf("cmd 2 diff: %d\n", getTimeDiffus(&tv1, &tv2));

	if (memcmp(arg, res, test_len) == 0) {
		printf("cmd 2 test ok\n");
	} else {
		printf("cmd 2 test fail\n");
	}

	free(arg);
	free(res);
}

static CVI_S32 server_cmd_02(CVI_U8 *arg, CVI_U32 arg_len, CVI_U8 *res, CVI_U32 res_len)
{
	UNUSED(arg);
	UNUSED(arg_len);
	UNUSED(res);
	UNUSED(res_len);

	printf("run cmd 02\n");
	memcpy(res, arg, res_len);

	return CVI_SUCCESS;
}

static void client_send_cmd_03(void)
{
	ISP_IPC_MSG_S msg;
	CVI_U32 test_len = 4096;

	CVI_U8 *arg = calloc(1, test_len);
	CVI_U8 *res = calloc(1, test_len);

	memset(arg, 0x55, test_len);
	memset(res, 0x00, test_len);

	msg.cmd = ISP_IPC_SERVER_CMD_03;
	msg.arg_len = test_len;
	msg.res_len = test_len;

	struct timeval tv1, tv2;

	gettimeofday(&tv1, NULL);
	client_send_cmd_to_server(&msg, arg, res);
	gettimeofday(&tv2, NULL);

	printf("cmd 3 diff: %d\n", getTimeDiffus(&tv1, &tv2));

	if (memcmp(arg, res, test_len) == 0) {
		printf("cmd 3 test ok\n");
	} else {
		printf("cmd 3 test fail\n");
	}

	free(arg);
	free(res);
}

static CVI_S32 server_cmd_03(CVI_U8 *arg, CVI_U32 arg_len, CVI_U8 *res, CVI_U32 res_len)
{
	UNUSED(arg);
	UNUSED(arg_len);
	UNUSED(res);
	UNUSED(res_len);

	printf("run cmd 03\n");
	memcpy(res, arg, res_len);

	return CVI_SUCCESS;
}

static void client_send_cmd_04(void)
{
	ISP_IPC_MSG_S msg;
	CVI_U32 test_len = 1024 * 1024;

	CVI_U8 *arg = calloc(1, test_len);
	CVI_U8 *res = calloc(1, test_len);

	memset(arg, rand() % 256, test_len);
	memset(res, 0x00, test_len);

	msg.cmd = ISP_IPC_SERVER_CMD_04;
	msg.arg_len = test_len;
	msg.res_len = test_len;

	struct timeval tv1, tv2;

	gettimeofday(&tv1, NULL);
	client_send_cmd_to_server(&msg, arg, res);
	gettimeofday(&tv2, NULL);

	printf("cmd 4 diff: %d\n", getTimeDiffus(&tv1, &tv2));

	if (memcmp(arg, res, test_len) == 0) {
		printf("cmd 4 test ok\n");
	} else {
		printf("cmd 4 test fail\n");
	}

	free(arg);
	free(res);
}

static CVI_S32 server_cmd_04(CVI_U8 *arg, CVI_U32 arg_len, CVI_U8 *res, CVI_U32 res_len)
{
	UNUSED(arg);
	UNUSED(arg_len);
	UNUSED(res);
	UNUSED(res_len);

	printf("run cmd 04\n");
	memcpy(res, arg, res_len);

	return CVI_SUCCESS;
}

static void client_send_cmd_05(void)
{
	ISP_IPC_MSG_S msg;
	CVI_U32 test_len = rand() % 4096;

	CVI_U8 *arg = calloc(1, test_len);
	CVI_U8 *res = calloc(1, test_len);

	memset(arg, rand() % 256, test_len);
	memset(res, 0x00, test_len);

	msg.cmd = ISP_IPC_SERVER_CMD_05;
	msg.arg_len = test_len;
	msg.res_len = test_len;

	struct timeval tv1, tv2;

	gettimeofday(&tv1, NULL);
	client_send_cmd_to_server(&msg, arg, res);
	gettimeofday(&tv2, NULL);

	printf("cmd 5 diff: %d\n", getTimeDiffus(&tv1, &tv2));

	if (memcmp(arg, res, test_len) == 0) {
		printf("cmd 5 test ok\n");
	} else {
		printf("cmd 5 test fail\n");
	}

	free(arg);
	free(res);
}

static CVI_S32 server_cmd_05(CVI_U8 *arg, CVI_U32 arg_len, CVI_U8 *res, CVI_U32 res_len)
{
	UNUSED(arg);
	UNUSED(arg_len);
	UNUSED(res);
	UNUSED(res_len);

	printf("run cmd 05\n");
	memcpy(res, arg, res_len);

	return CVI_SUCCESS;
}

static ISP_IPC_CMD_ITEM_S server_cmd_list[] = {
	{ISP_IPC_SERVER_CMD_01, server_cmd_01},
	{ISP_IPC_SERVER_CMD_02, server_cmd_02},
	{ISP_IPC_SERVER_CMD_03, server_cmd_03},
	{ISP_IPC_SERVER_CMD_04, server_cmd_04},
	{ISP_IPC_SERVER_CMD_05, server_cmd_05},
};

void isp_ipc_server_reg_cmd(void)
{
	isp_ipc_reg_server_cmd(server_cmd_list, ISP_IPC_SERVER_CMD_MAX);
}

static void *test_thread(void *param)
{
	ISP_IPC_TEST_FUN test_fun = (ISP_IPC_TEST_FUN) param;

	srand((int) time(NULL));

	while (1) {
		uint32_t t = (uint32_t) rand();

		t = t % 20;
		usleep(t * 1000);
		test_fun();
	}

	return NULL;
}

void isp_ipc_test_client_to_server(void)
{
	pthread_t tid;

	pthread_create(&tid, NULL, test_thread, client_send_cmd_01);
	pthread_create(&tid, NULL, test_thread, client_send_cmd_02);
	pthread_create(&tid, NULL, test_thread, client_send_cmd_03);
	pthread_create(&tid, NULL, test_thread, client_send_cmd_04);
	pthread_create(&tid, NULL, test_thread, client_send_cmd_05);
}

#endif

