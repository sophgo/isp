
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cvi_ispd2.h"

#include "cvi_sys.h"

#include "vi.h"
#include "venc.h"
#include "rtsp.h"
#include "utils.h"
#include "vo.h"

#ifndef UNUSED
#define UNUSED(x) ((void)(x))
#endif

#include <signal.h>     /* for signal */
#include <execinfo.h>   /* for backtrace() */

#define BACKTRACE_SIZE   16
#define RTSP_JSON_PATH "cfg.json"

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

	sprintf(buff, "cat /proc/%d/maps", getpid());

	system((const char *) buff);
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

static int gLoopRun;

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

static void initSignal(void)
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

#define JSONRPC_PORT	(5566)

int main(int argc, char *argv[])
{
	UNUSED(argc);
	UNUSED(argv);

	initSignal();

	RegisterBacktraceHandler();

	RTSP_CFG rtsp_cfg;

	if (init_rtsp_from_json(RTSP_JSON_PATH, &rtsp_cfg) != 0) {
		printf("parse rtsp json cfg error\n");
		return -1;
	}

	start_vi(&rtsp_cfg);
	start_venc(&rtsp_cfg);
	start_rtsp(&rtsp_cfg);
	start_vo(&rtsp_cfg);

	isp_daemon2_init(JSONRPC_PORT);

	gLoopRun = 1;

	while (gLoopRun) {
		sleep(1);
	}

	isp_daemon2_uninit();

	stop_vo(&rtsp_cfg);
	stop_rtsp(&rtsp_cfg);
	stop_venc(&rtsp_cfg);
	stop_vi(&rtsp_cfg);

	deinit_rtsp_from_json(&rtsp_cfg);

	return 0;
}

