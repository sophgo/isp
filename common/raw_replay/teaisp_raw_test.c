
#include <stdlib.h>
#include "teaisp_raw_test.h"

#ifdef ENABLE_TEAISP_RAW_REPLAY_TEST
#include "clog.h"
#include "raw_replay.h"
#include "bmruntime_interface.h"

#define TPU_DEVICE_ID (0)
#define __BMODEL_PATH "/mnt/data/bnr.bmodel"

#define LOG_OUT(...) clog_error("TEAISP", __VA_ARGS__)

#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif

typedef struct {
	bm_handle_t bm_handle;
	void *p_bmrt;
	bm_tensor_t *input_tensors;
	bm_tensor_t *output_tensors;
	bm_data_type_t *input_dtypes;
	uint32_t input_num;
	uint32_t output_num;
	const char **net_names;
} TEAISP_RAW_TEST_HANDLE_S;

static TEAISP_RAW_TEST_HANDLE_S *phandle;

static int teaisp_raw_test_init(void)
{
	bm_status_t status;

	if (phandle != NULL) {
		return CVI_SUCCESS;
	}

	phandle = (TEAISP_RAW_TEST_HANDLE_S*) calloc(1, sizeof(TEAISP_RAW_TEST_HANDLE_S));

	if (phandle == NULL) {
		return CVI_FAILURE;
	}

	memset(phandle, 0, sizeof(TEAISP_RAW_TEST_HANDLE_S));

	LOG_OUT("load bmodel++: %s\n", __BMODEL_PATH);

	status = bm_dev_request(&phandle->bm_handle, TPU_DEVICE_ID);
	if (status != BM_SUCCESS) {
		LOG_OUT("request device fail, id: %d\n", TPU_DEVICE_ID);
		goto load_model_fail;
	}

	phandle->p_bmrt = bmrt_create(phandle->bm_handle);
	if (phandle->p_bmrt == NULL) {
		LOG_OUT("bmrt create fail\n");
		goto load_model_fail;
	}

	bool ret = bmrt_load_bmodel(phandle->p_bmrt, __BMODEL_PATH);

	if (!ret) {
		LOG_OUT("load bmodel fail, %s\n", __BMODEL_PATH);
		goto load_model_fail;
	}

	bmrt_show_neuron_network(phandle->p_bmrt);

	bmrt_get_network_names(phandle->p_bmrt, &phandle->net_names);

	//int net_num = bmrt_get_network_number(m->p_bmrt);

	const bm_net_info_t *net_info = bmrt_get_network_info(phandle->p_bmrt, phandle->net_names[0]);

	phandle->input_tensors = (bm_tensor_t *) calloc(net_info->input_num, sizeof(bm_tensor_t));

	if (phandle->input_tensors == NULL) {
		goto load_model_fail;
	}

	phandle->output_tensors = (bm_tensor_t *) calloc(net_info->input_num, sizeof(bm_tensor_t));

	if (phandle->output_tensors == NULL) {
		goto load_model_fail;
	}

	phandle->input_dtypes = (uint32_t *) calloc(net_info->input_num, sizeof(bm_data_type_t));

	if (phandle->input_dtypes == NULL) {
		goto load_model_fail;
	}

	LOG_OUT("net name: %s, in: %d, on: %d\n", phandle->net_names[0], net_info->input_num, net_info->output_num);

	phandle->input_num = net_info->input_num;
	phandle->output_num = net_info->output_num;

	bm_stage_info_t stage = net_info->stages[0];

	for (int i = 0; i < net_info->input_num; i++) {
		bmrt_tensor(&phandle->input_tensors[i], phandle->p_bmrt, net_info->input_dtypes[i],
			stage.input_shapes[i]);
		phandle->input_dtypes[i] = net_info->input_dtypes[i];
	}

	for (int i = 0; i < net_info->output_num; i++) {
		bmrt_tensor(&phandle->output_tensors[i], phandle->p_bmrt, net_info->output_dtypes[i],
			stage.output_shapes[i]);
	}

	LOG_OUT("load model success, %p\n", phandle);

	return CVI_SUCCESS;

load_model_fail:

	if (phandle != NULL) {

		if (phandle->input_tensors) {
			free(phandle->input_tensors);
			phandle->input_tensors = NULL;
		}

		if (phandle->output_tensors) {
			free(phandle->output_tensors);
			phandle->output_tensors = NULL;
		}

		if (phandle->input_dtypes) {
			free(phandle->input_dtypes);
			phandle->input_dtypes = NULL;
		}

		if (phandle->p_bmrt) {
			bmrt_destroy(phandle->p_bmrt);
		}

		bm_dev_free(phandle->bm_handle);

		free(phandle);
		phandle = NULL;
	}

	return CVI_FAILURE;
}

static int teaisp_raw_test_deinit(void)
{
	if (phandle == NULL) {
		return CVI_FAILURE;
	}

	if (phandle->input_tensors) {
		free(phandle->input_tensors);
		phandle->input_tensors = NULL;
	}

	if (phandle->output_tensors) {
		free(phandle->output_tensors);
		phandle->output_tensors = NULL;
	}

	if (phandle->input_dtypes) {
		free(phandle->input_dtypes);
		phandle->input_dtypes = NULL;
	}

	if (phandle->p_bmrt) {
		bmrt_destroy(phandle->p_bmrt);
	}

	bm_dev_free(phandle->bm_handle);

	free(phandle);
	phandle = NULL;

	LOG_OUT("unload bmodel--: %s\n", __BMODEL_PATH);

	return CVI_SUCCESS;
}

CVI_S32 teaisp_raw_inference_test(const CVI_VOID *header, CVI_VOID *data,
							CVI_U32 totalFrame, CVI_U32 curFrame, CVI_U32 rawFrameSize)
{
	bm_status_t status;

	UNUSED(header);
	UNUSED(totalFrame);
	UNUSED(rawFrameSize);

	if (curFrame == 0) {
		teaisp_raw_test_deinit();
		teaisp_raw_test_init();
	}

	if (phandle == NULL) {
		LOG_OUT("init fail...\n");
		return CVI_FAILURE;
	}

	if (phandle->input_num != 6) {
		LOG_OUT("please check model...%s\n", __BMODEL_PATH);
		return CVI_FAILURE;
	}

	LOG_OUT("prepare input tensors+++\n");

	if (curFrame == 0) {

		float coeff_a = 0.0114787;
		float coeff_b = 0.0004039;
		float blend = 0;

		bm_memcpy_s2d(phandle->bm_handle, phandle->input_tensors[3].device_mem, &coeff_a);
		bm_memcpy_s2d(phandle->bm_handle, phandle->input_tensors[4].device_mem, &coeff_b);
		bm_memcpy_s2d(phandle->bm_handle, phandle->input_tensors[5].device_mem, &blend);

	} else {
		float blend = 1;

		bm_memcpy_s2d(phandle->bm_handle, phandle->input_tensors[5].device_mem, &blend);
	}

	bm_memcpy_s2d(phandle->bm_handle, phandle->input_tensors[0].device_mem, data);

	LOG_OUT("start inference+++\n");

	bool ret = bmrt_launch_tensor_ex(phandle->p_bmrt, phandle->net_names[0],
		phandle->input_tensors, phandle->input_num, phandle->output_tensors,
		phandle->output_num, true, false);

	if (ret) {
		status = bm_thread_sync(phandle->bm_handle);
	}

	if (!ret || BM_SUCCESS != status) {
		LOG_OUT("%s, inference failed...\n", phandle->net_names[0]);
		return CVI_FAILURE;
	}

	LOG_OUT("processor output tensors+++\n");

	bm_memcpy_d2s(phandle->bm_handle, data, phandle->output_tensors[0].device_mem);

	//bm_memcpy_d2d(phandle->bm_handle, phandle->input_tensors[1].device_mem, 0,
	//	phandle->output_tensors[1].device_mem, 0,
	//	bmrt_tensor_bytesize(phandle->input_tensors[1]));
	//bm_memcpy_d2d(phandle->bm_handle, phandle->input_tensors[2].device_mem, 0,
	//	phandle->output_tensors[2].device_mem, 0,
	//	bmrt_tensor_bytesize(phandle->input_tensors[2]));

	bm_tensor_t temp;

	temp = phandle->input_tensors[1];
	phandle->input_tensors[1] = phandle->output_tensors[1];
	phandle->output_tensors[1] = temp;

	temp = phandle->input_tensors[2];
	phandle->input_tensors[2] = phandle->output_tensors[2];
	phandle->output_tensors[2] = temp;

	LOG_OUT("inference end---\n");

	return CVI_SUCCESS;
}
#endif

