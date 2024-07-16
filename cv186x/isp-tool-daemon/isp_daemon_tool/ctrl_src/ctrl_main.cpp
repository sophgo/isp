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

extern "C" {
#include "cvi_type.h"
#include "cvi_comm_video.h"
#include "cvi_ispd2.h"
#include "raw_dump.h"
#include "cvi_isp.h"
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

int getRawDumpFlag()
{
	const char *input = getenv("CVI_ENABLE_RAW_DUMP");

	if (input) {
		return (atoi(input));
	} else {
		return 0;
	}
}

#define JSONRPC_PORT	(8888)

int main(void)
{
	int enable_raw_dump = getRawDumpFlag();

	initSignal();

	CVI_ISP_Client_Init(0);

	isp_daemon2_init(JSONRPC_PORT);
	int single_output = getRTSPMode();
	isp_daemon2_enable_device_bind_control(single_output ? 1 : 0);

	if (enable_raw_dump != 0) {
		cvi_raw_dump_init();
	}

	gLoopRun = 1;
	while (gLoopRun) {
		sleep(1);
	}

	if (enable_raw_dump != 0) {
		cvi_raw_dump_uninit();
	}

	isp_daemon2_uninit();

	CVI_ISP_Client_Exit(0);

	return 0;
}

