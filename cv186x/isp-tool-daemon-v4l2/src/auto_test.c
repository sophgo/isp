#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CLOG_TAG "AUTO_TEST"

#include "cvi_isp.h"
#include "clog.h"

#define _TEST_FAIL_TAG "[ISP_TEST_FAIL] "
#define _ISP_AUTO_TEST_LIB_NAME "libisp_test.so"

// isp/inc/isp_debug.c
extern int CVI_DEBUG_SetDebugLevel(int level);

typedef int (*test_fn)(VI_PIPE ViPipe);

static int run_isp_module_api_test_case(int dev_num)
{
	clog_i("Run ISP module API test case\n");

	void *dl = dlopen(_ISP_AUTO_TEST_LIB_NAME, RTLD_LAZY);

	if (dl == NULL) {
		clog_a(_TEST_FAIL_TAG"dlopen [%s] failed: %s\n", _ISP_AUTO_TEST_LIB_NAME, dlerror());
		return -1;
	}

	test_fn fn = (test_fn)dlsym(dl, "api_test_main");

	if (fn == NULL) {
		clog_a(_TEST_FAIL_TAG"dlsym [%s] failed: %s\n", "api_test_main", dlerror());
		dlclose(dl);
		return -1;
	}

	int ret = 0;

	for (int pipe = 0; pipe < dev_num; ++pipe) {
		ret |= fn(pipe);
	}

	dlclose(dl);
	return ret;
}

static int run_isp_dump_test_case(int dev_num)
{
	clog_i("Run ISP dump test case\n");

	void *dl = dlopen(_ISP_AUTO_TEST_LIB_NAME, RTLD_LAZY);

	if (dl == NULL) {
		clog_a(_TEST_FAIL_TAG"dlopen [%s] failed: %s\n", _ISP_AUTO_TEST_LIB_NAME, dlerror());
		return -1;
	}

	test_fn fn = (test_fn)dlsym(dl, "dump_test_main");

	if (fn == NULL) {
		clog_a(_TEST_FAIL_TAG"dlsym [%s] failed: %s\n", "dump_test_main", dlerror());
		dlclose(dl);
		return -1;
	}

	int ret = 0;

	for (int pipe = 0; pipe < dev_num; ++pipe) {
		ret |= fn(pipe);
	}

	dlclose(dl);
	return ret;
}

static int run_isp_pqbin_test_case(int dev_num)
{
	clog_i("Run ISP pqbin test case\n");

	void *dl = dlopen(_ISP_AUTO_TEST_LIB_NAME, RTLD_LAZY);

	if (dl == NULL) {
		clog_a(_TEST_FAIL_TAG"dlopen [%s] failed: %s\n", _ISP_AUTO_TEST_LIB_NAME, dlerror());
		return -1;
	}

	test_fn fn = (test_fn)dlsym(dl, "pqbin_test_main");

	if (fn == NULL) {
		clog_a(_TEST_FAIL_TAG"dlsym [%s] failed: %s\n", "pqbin_test_main", dlerror());
		dlclose(dl);
		return -1;
	}

	int ret = fn(dev_num);

	dlclose(dl);

	return ret;
}

int run_isp_auto_test_case(int case_num, int dev_num)
{
	int ret = 0;

	clog_i("Run ISP auto test case: %d\n", case_num);

	switch (case_num) {
	case 0:
		ret = 0;
		break;
	case 1:
		CVI_DEBUG_SetDebugLevel(CLOG_LVL_ASSERT);
		ret = run_isp_module_api_test_case(dev_num);
		CVI_DEBUG_SetDebugLevel(CLOG_LVL_ERROR);
		if (ret == 0) {
			printf("ISP_MODULE_API_TEST SUCCESS!\n");
		}
		break;
	case 2:
		ret = run_isp_dump_test_case(dev_num);
		if (ret == 0) {
			printf("ISP_DUMP_TEST SUCCESS!\n");
		}
		break;
	case 3:
		sleep(2);
		ret = run_isp_pqbin_test_case(dev_num);
		if (ret == 0) {
			printf("ISP_PQBIN_TEST SUCCESS!\n");
		}
		break;
	default:
		break;
	}

	clog_i("Run ISP auto test case %d finished with result: %d\n", case_num, ret);

	return ret;
}
