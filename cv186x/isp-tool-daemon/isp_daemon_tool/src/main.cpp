/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: main.cpp
 * Description:
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <memory>
#include <csignal>
#include <syslog.h>
#include <unistd.h>

#include "cvi_streamer.hpp"
#include "isp_tool_daemon_comm.hpp"

extern "C" {
#include "cvi_ispd2.h"
#include "raw_dump.h"
#include "raw_dump_internal.h"
#include "uart_utils.h"
#include <string.h>
#include "raw_replay_offline.h"
}

using std::shared_ptr;
using cvitek::system::CVIStreamer;

#include <signal.h>     /* for signal */
#include <execinfo.h>   /* for backtrace() */

#define BACKTRACE_SIZE   16

static void dump(void)
{
	int j, nptrs;
	void *buffer[BACKTRACE_SIZE];
	char **strings;

	nptrs = backtrace(buffer, BACKTRACE_SIZE);

	printf("backtrace() returned %d addresses\n", nptrs);

	strings = backtrace_symbols(buffer, nptrs);
	if (strings == NULL) {
		printf("backtrace_symbols\n");
		exit(EXIT_FAILURE);

	}

	for (j = 0; j < nptrs; j++)
		printf("  [%02d] %s\n", j, strings[j]);

	free(strings);
}

static void signal_handler(int signo)
{

#if 0
	char buff[64] = {0x00};

	sprintf(buff,"cat /proc/%d/maps", getpid());

	system((const char*) buff);
#endif

	printf("\n\n=========>>>catch signal %d <<<=========\n", signo);

	printf("Dump stack start...\n");
	dump();
	printf("Dump stack end...\n");

	signal(signo, SIG_DFL);
	raise(signo);
}

static void RegisterBacktraceHandler(void)
{
	signal(SIGSEGV, signal_handler);
}

static CVI_U32 gLoopRun;

static void signalHandler(int iSigNo)
{
	switch (iSigNo) {
		case SIGINT:
		case SIGTERM:
			gLoopRun = 0;
			break;

		default:
			break;
	}
}

static void initSignal()
{
	struct sigaction sigaInst;

	sigaInst.sa_flags = 0;
	sigemptyset(&sigaInst.sa_mask);
	sigaddset(&sigaInst.sa_mask, SIGINT);
	sigaddset(&sigaInst.sa_mask, SIGTERM);

	sigaInst.sa_handler = signalHandler;
	sigaction(SIGINT, &sigaInst, NULL);
	sigaction(SIGTERM, &sigaInst, NULL);
}

static int getRTSPMode()
{
	const char *input = getenv("CVI_RTSP_MODE");

	int single_output = 1;		// default is single output (for internal application)

	if (input) {
		int rtspMode = atoi(input);
		if (rtspMode != 0) {
			single_output = 0;
		}
	}

	return single_output;
}

#define JSONRPC_PORT	(5566)
#define ENV_RAW_DUMP_SAVE_PATH  "RAW_DUMP_SAVE_PATH"
#define ENV_RAW_DUMP_PARAM      "RAW_DUMP_PARAM"
#define ENV_RAW_DUMP_REPEAT     "RAW_DUMP_REPEAT"

static int gRawDumpRepeat;
static RAW_DUMP_INFO_S gstRawDumpInfo;
static bool init_raw_dump(void);

#define MAX_RAW_DUMP_PATH_LEN 128
static char gRawDumpPath[MAX_RAW_DUMP_PATH_LEN];
static char *get_raw_dump_path(void);

int main(int argc, char **argv)
{
	openlog("ISP Tool Daemon", 0, LOG_USER);

	shared_ptr<CVIStreamer> cvi_streamer =
		make_shared<CVIStreamer>();

	//sample_common_platform.c will sigaction _SAMPLE_PLAT_SYS_HandleSig
	//we substitute signalHander for _SAMPLE_PLAT_SYS_HandleSig
	initSignal();

	RegisterBacktraceHandler();

	isp_daemon2_init(JSONRPC_PORT);

	cvi_raw_dump_init();

	int single_output = getRTSPMode();

	isp_daemon2_enable_device_bind_control(single_output ? 1 : 0);

	int uart_flag = 0;

	if (argc >= 2 && strncmp(argv[1], "/dev/", 5) == 0) {
		if(isp_daemon2_init_uart(argv[1], single_output) >= 0) {
			uart_flag = 1;
		}
	}

	bool bEnableRawDump = init_raw_dump();

	char *boardPath = getenv("REPLAY_FROM_BOARD_PATH");
	if (boardPath != NULL) {
		printf("Loading Raw Replay Path: %s\n", boardPath);
		if (raw_replay_offline_init(boardPath) != CVI_SUCCESS) {
			printf("raw_replay_offline_init failed!\n");
			raw_replay_offline_uninit();
			return CVI_FAILURE;
		}
		if (start_raw_replay_offline(0)) {
			printf("start_raw_replay_offline failed!\n");
			raw_replay_offline_uninit();
			return CVI_FAILURE;
		} else {
			printf("Start Raw Replay by Path: %s\n", boardPath);
		}

	}

	gLoopRun = 1;
	while (gLoopRun) {
		sleep(1);

		if (bEnableRawDump) {
			if (gRawDumpRepeat > 0 ||
				gRawDumpRepeat == -1) {
				gstRawDumpInfo.pathPrefix = get_raw_dump_path();
				cvi_raw_dump(0, &gstRawDumpInfo);
				if (gRawDumpRepeat > 0) {
					gRawDumpRepeat--;
				}
			}
		}
		if (uart_flag && !isp_daemon2_get_uart_run_state()) {
			console_recover();
			uart_flag = 0;
		}

	}
	if (boardPath != NULL) {
		raw_replay_offline_uninit();
	}

	cvi_raw_dump_uninit();

	isp_daemon2_uninit();

	isp_daemon2_uninit_uart();

	closelog();
	return 0;
}

static char *get_raw_dump_path(void)
{
	struct tm *t;
	time_t tt;
	char cmd[MAX_RAW_DUMP_PATH_LEN + 32];
	const char *path = getenv(ENV_RAW_DUMP_SAVE_PATH);

	time(&tt);
	t = localtime(&tt);

	snprintf(gRawDumpPath, MAX_RAW_DUMP_PATH_LEN, "%s/%04d%02d%02d%02d%02d%02d",
		path,
		t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
		t->tm_hour, t->tm_min, t->tm_sec);


	snprintf(cmd, (MAX_RAW_DUMP_PATH_LEN + 32), "mkdir %s;sync", gRawDumpPath);
	system(cmd);

	return gRawDumpPath;
}

static bool init_raw_dump(void)
{
	const char *path = getenv(ENV_RAW_DUMP_SAVE_PATH);
	const char *param = getenv(ENV_RAW_DUMP_PARAM);
	const char *repeat = getenv(ENV_RAW_DUMP_REPEAT);

	if (path == NULL) {
		return false;
	}

	if (param == NULL || repeat == NULL) {
		printf("run raw dump fail, parameter missing...\n");
		return false;
	}

	printf("init raw dump, %s %s %s\n", path, param, repeat);

	memset(&gstRawDumpInfo, 0, sizeof(gstRawDumpInfo));

	if (access(path, F_OK) == 0) {
		gstRawDumpInfo.pathPrefix = (char *) path;
	} else {
		printf("%s not exist...\n", path);
		return false;
	}

	sscanf(param, "%d,%d,%d,%d,%d",
		&gstRawDumpInfo.u32TotalFrameCnt,
		&gstRawDumpInfo.stRoiRect.s32X,
		&gstRawDumpInfo.stRoiRect.s32Y,
		&gstRawDumpInfo.stRoiRect.u32Width,
		&gstRawDumpInfo.stRoiRect.u32Height);

	printf("raw dump count: %d, roi: %d,%d,%d,%d\n",
		gstRawDumpInfo.u32TotalFrameCnt,
		gstRawDumpInfo.stRoiRect.s32X,
		gstRawDumpInfo.stRoiRect.s32Y,
		gstRawDumpInfo.stRoiRect.u32Width,
		gstRawDumpInfo.stRoiRect.u32Height);

	gRawDumpRepeat = atoi(repeat);

	return true;
}
