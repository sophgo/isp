/*
 * Copyright (C) Cvitek Co., Ltd. 2023-2024. All rights reserved.
 *
 * File Name: teaisp_helper.c
 * Description:
 *
 */

#include <unistd.h>
#include "cvi_sys.h"
#include "isp_debug.h"
#include "isp_defines.h"
#include "teaisp_helper.h"
#include "isp_ioctl.h"

#include "teaisp_bnr_ctrl.h"
#include "teaisp.h"

#include "api.h"
#include "bmdef.h"
#include "bmruntime_legacy.h"
#include "bmruntime_interface.h"

#include "teaisp_bnr_helper_wrap.h"

#define TEAISP_MAX_TUNING_INDEX (2)

#define BNR_IN_INPUT_IMG      (0)
#define BNR_IN_FUSION_IMG     (1)
#define BNR_IN_SIGMA          (2)
#define BNR_IN_COEFF_A        (3)
#define BNR_IN_COEFF_B        (4)
#define BNR_IN_BLEND          (5)
#define BNR_IN_MOTION_STR_2D  (6)
#define BNR_IN_STATIC_STR_2D  (7)
#define BNR_IN_BLACK_LEVEL    (8)
#define BNR_IN_STR_3D         (9)
#define BNR_IN_NUM            (10)

#define BNR_OUT_INPUT_IMG     (0)
#define BNR_OUT_FUSION_IMG    (1)
#define BNR_OUT_SIGMA         (2)
#define BNR_OUT_NUM           (3)

typedef struct {
	int pipe;
	uint32_t core_id;
	uint32_t tuning_index;
	void *p_bmrt;
	const char **net_names;
	void **input_vaddr[TEAISP_MAX_TUNING_INDEX];
	bm_tensor_t *input_tensors[TEAISP_MAX_TUNING_INDEX];
	bm_tensor_t *output_tensors[TEAISP_MAX_TUNING_INDEX];
	int input_num;
	int output_num;
} TEAISP_MODEL_S;

typedef struct {
	uint8_t enable_lauch_thread;
	pthread_t launch_thread_id;

	bm_handle_t bm_handle;
	TEAISP_MODEL_S *bmodel0;
	TEAISP_MODEL_S *bmodel1;
} TEAISP_BNR_CTX_S;

static TEAISP_BNR_CTX_S *bnr_ctx[VI_MAX_PIPE_NUM];

static int teaisp_bnr_get_raw(VI_PIPE ViPipe, uint64_t *input_raw, uint64_t *output_raw)
{
	uint64_t tmp[2] = {0, 0};

	G_EXT_CTRLS_PTR(VI_IOCTL_GET_AI_ISP_RAW, &tmp);

	*input_raw = tmp[0];
	*output_raw = tmp[1];

	return 0;
}

static int teaisp_bnr_put_raw(VI_PIPE ViPipe, uint64_t *input_raw, uint64_t *output_raw)
{
	uint64_t tmp[2] = {0, 0};

	tmp[0] = *input_raw;
	tmp[1] = *output_raw;

	S_EXT_CTRLS_PTR(VI_IOCTL_PUT_AI_ISP_RAW, &tmp);

	return 0;
}

static void teaisp_bnr_dump_input(TEAISP_MODEL_S *model, uint64_t input_raw_addr,
	uint8_t tuning_index, int pipe)
{
	void *addr_tmp = NULL;
	int size = 0;
	FILE *fp = NULL;
	char path[128] = {0};

	snprintf(path, sizeof(path), "/tmp/input0%d_img.bin", pipe);
	size = model->input_tensors[tuning_index][0].device_mem.size;
	addr_tmp = CVI_SYS_MmapCache(input_raw_addr, size);
	fp = fopen(path, "wb");
	fwrite(addr_tmp, size, 1, fp);
	fclose(fp);
	CVI_SYS_Munmap(addr_tmp, size);

	snprintf(path, sizeof(path), "/tmp/input1%d_fusion.bin", pipe);
	size = model->input_tensors[tuning_index][BNR_IN_FUSION_IMG].device_mem.size;
	addr_tmp = CVI_SYS_MmapCache(
		model->input_tensors[tuning_index][BNR_IN_FUSION_IMG].device_mem.u.device.device_addr, size);
	fp = fopen(path, "wb");
	fwrite(addr_tmp, size, 1, fp);
	fclose(fp);
	CVI_SYS_Munmap(addr_tmp, size);

	snprintf(path, sizeof(path), "/tmp/input2%d_sigma.bin", pipe);
	size = model->input_tensors[tuning_index][BNR_IN_SIGMA].device_mem.size;
	addr_tmp = CVI_SYS_MmapCache(
		model->input_tensors[tuning_index][BNR_IN_SIGMA].device_mem.u.device.device_addr, size);
	fp = fopen(path, "wb");
	fwrite(addr_tmp, size, 1, fp);
	fclose(fp);
	CVI_SYS_Munmap(addr_tmp, size);

	snprintf(path, sizeof(path), "/tmp/input3%d_coeff_a.bin", pipe);
	size = model->input_tensors[tuning_index][BNR_IN_COEFF_A].device_mem.size;
	addr_tmp = model->input_vaddr[tuning_index][BNR_IN_COEFF_A];
	fp = fopen(path, "wb");
	fwrite(addr_tmp, size, 1, fp);
	fclose(fp);

	snprintf(path, sizeof(path), "/tmp/input4%d_coeff_b.bin", pipe);
	size = model->input_tensors[tuning_index][BNR_IN_COEFF_B].device_mem.size;
	addr_tmp = model->input_vaddr[tuning_index][BNR_IN_COEFF_B];
	fp = fopen(path, "wb");
	fwrite(addr_tmp, size, 1, fp);
	fclose(fp);

	snprintf(path, sizeof(path), "/tmp/input5%d_blend.bin", pipe);
	size = model->input_tensors[tuning_index][BNR_IN_BLEND].device_mem.size;
	addr_tmp = model->input_vaddr[tuning_index][BNR_IN_BLEND];
	fp = fopen(path, "wb");
	fwrite(addr_tmp, size, 1, fp);
	fclose(fp);

	snprintf(path, sizeof(path), "/tmp/input6%d_motion_str_2d.bin", pipe);
	size = model->input_tensors[tuning_index][BNR_IN_MOTION_STR_2D].device_mem.size;
	addr_tmp = model->input_vaddr[tuning_index][BNR_IN_MOTION_STR_2D];
	fp = fopen(path, "wb");
	fwrite(addr_tmp, size, 1, fp);
	fclose(fp);

	snprintf(path, sizeof(path), "/tmp/input7%d_static_str_2d.bin", pipe);
	size = model->input_tensors[tuning_index][BNR_IN_STATIC_STR_2D].device_mem.size;
	addr_tmp = model->input_vaddr[tuning_index][BNR_IN_STATIC_STR_2D];
	fp = fopen(path, "wb");
	fwrite(addr_tmp, size, 1, fp);
	fclose(fp);

	snprintf(path, sizeof(path), "/tmp/input8%d_blc.bin", pipe);
	size = model->input_tensors[tuning_index][BNR_IN_BLACK_LEVEL].device_mem.size;
	addr_tmp = model->input_vaddr[tuning_index][BNR_IN_BLACK_LEVEL];
	fp = fopen(path, "wb");
	fwrite(addr_tmp, size, 1, fp);
	fclose(fp);

	snprintf(path, sizeof(path), "/tmp/input9%d_str_3d.bin", pipe);
	size = model->input_tensors[tuning_index][BNR_IN_STR_3D].device_mem.size;
	addr_tmp = model->input_vaddr[tuning_index][BNR_IN_STR_3D];
	fp = fopen(path, "wb");
	fwrite(addr_tmp, size, 1, fp);
	fclose(fp);
}

static void teaisp_bnr_dump_output(TEAISP_MODEL_S *model, uint64_t output_raw_addr,
	uint8_t tuning_index, int pipe)
{
	void *addr_tmp = NULL;
	int size = 0;
	FILE *fp = NULL;
	char path[128] = {0};

	snprintf(path, sizeof(path), "/tmp/output0%d_img.bin", pipe);
	size = model->output_tensors[tuning_index][0].device_mem.size;
	addr_tmp = CVI_SYS_MmapCache(output_raw_addr, size);
	fp = fopen(path, "wb");
	fwrite(addr_tmp, size, 1, fp);
	fclose(fp);
	CVI_SYS_Munmap(addr_tmp, size);

	snprintf(path, sizeof(path), "/tmp/output1%d_fusion.bin", pipe);
	size = model->output_tensors[tuning_index][BNR_OUT_FUSION_IMG].device_mem.size;
	addr_tmp = CVI_SYS_MmapCache(
		model->output_tensors[tuning_index][BNR_OUT_FUSION_IMG].device_mem.u.device.device_addr, size);
	fp = fopen(path, "wb");
	fwrite(addr_tmp, size, 1, fp);
	fclose(fp);
	CVI_SYS_Munmap(addr_tmp, size);

	snprintf(path, sizeof(path), "/tmp/output2%d_sigma.bin", pipe);
	size = model->output_tensors[tuning_index][BNR_OUT_SIGMA].device_mem.size;
	addr_tmp = CVI_SYS_MmapCache(
		model->output_tensors[tuning_index][BNR_OUT_SIGMA].device_mem.u.device.device_addr, size);
	fp = fopen(path, "wb");
	fwrite(addr_tmp, size, 1, fp);
	fclose(fp);
	CVI_SYS_Munmap(addr_tmp, size);
}

static void *teaisp_bnr_launch_thread(void *param)
{
	int pipe = (int) (uint64_t) param;
	uint64_t input_raw_addr = 0;
	uint64_t output_raw_addr = 0;

	//ISP_LOG_INFO("run bnr launch thread, %d\n", pipe);
	printf("run bnr launch thread, %d\n", pipe);

reset_launch:

	while (bnr_ctx[pipe]->enable_lauch_thread) {
		if (bnr_ctx[pipe]->bmodel0 == NULL) {
			usleep(5 * 1000);
		} else {
			break;
		}
	}

	TEAISP_MODEL_S *model = bnr_ctx[pipe]->bmodel0;

	bnr_ctx[pipe]->bmodel1 = bnr_ctx[pipe]->bmodel0;

	// wait param update
	while (bnr_ctx[pipe]->enable_lauch_thread) {
		if (model->tuning_index <= 0) {
			usleep(5 * 1000);
		} else {
			break;
		}
	}

	bm_tensor_t input_fusion_img;
	bm_tensor_t output_fusion_img;
	bm_tensor_t input_sigma;
	bm_tensor_t output_sigma;

	input_fusion_img = model->input_tensors[0][BNR_IN_FUSION_IMG];
	output_fusion_img = model->output_tensors[0][BNR_OUT_FUSION_IMG];
	input_sigma = model->input_tensors[0][BNR_IN_SIGMA];
	output_sigma = model->output_tensors[0][BNR_OUT_SIGMA];

	// launch
	while (bnr_ctx[pipe]->enable_lauch_thread) {

		if (bnr_ctx[pipe]->bmodel0 != bnr_ctx[pipe]->bmodel1) {
			//ISP_LOG_INFO("reset bnr launch....\n");
			printf("reset bnr launch....\n");
			goto reset_launch;
		}

		uint8_t tuning_index = model->tuning_index % TEAISP_MAX_TUNING_INDEX;

		teaisp_bnr_get_raw(pipe, &input_raw_addr, &output_raw_addr);

#ifndef ENABLE_BYPASS_TPU
		bm_status_t status = BM_SUCCESS;

		model->input_tensors[tuning_index][0].device_mem.u.device.device_addr = input_raw_addr;
		model->output_tensors[tuning_index][0].device_mem.u.device.device_addr = output_raw_addr;

		model->input_tensors[tuning_index][BNR_IN_FUSION_IMG] = input_fusion_img;
		model->output_tensors[tuning_index][BNR_IN_FUSION_IMG] = output_fusion_img;
		model->input_tensors[tuning_index][BNR_IN_SIGMA] = input_sigma;
		model->output_tensors[tuning_index][BNR_OUT_SIGMA] = output_sigma;

		//struct timeval tv1, tv2;

		//gettimeofday(&tv1, NULL);

		if (access("/tmp/teaisp_bnr_dump", F_OK) == 0) {
			teaisp_bnr_dump_input(model, input_raw_addr, tuning_index, pipe);
			system("rm /tmp/teaisp_bnr_dump;touch /tmp/teaisp_bnr_dump_output");
		}

		bool ret = bmrt_launch_tensor_multi_cores(model->p_bmrt, model->net_names[0],
			model->input_tensors[tuning_index], model->input_num,
			model->output_tensors[tuning_index], model->output_num, true, false,
			(const int *) &model->core_id, 1);

		if (ret) {
			status = bm_thread_sync_from_core(bnr_ctx[pipe]->bm_handle, model->core_id);
		}

		if (!ret || BM_SUCCESS != status) {
			ISP_LOG_ERR("%s, inference failed...\n", model->net_names[0]);
		}

		if (access("/tmp/teaisp_bnr_dump_output", F_OK) == 0) {
			teaisp_bnr_dump_output(model, output_raw_addr, tuning_index, pipe);
			system("rm /tmp/teaisp_bnr_dump_output");
		}

		//gettimeofday(&tv2, NULL);

		//ISP_LOG_ERR("launch time diff, %d, %d\n",
		//	pipe, ((tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec)));

#else
		void *src_addr;
		void *dst_addr;

		int size = model->input_tensors[tuning_index][0].device_mem.size;

		src_addr = CVI_SYS_MmapCache(input_raw_addr, size);
		dst_addr = CVI_SYS_MmapCache(output_raw_addr, size);

		memcpy(dst_addr, src_addr, size);

		usleep(45 * 1000);

		CVI_SYS_IonFlushCache(output_raw_addr, dst_addr, size);

		CVI_SYS_Munmap(src_addr, size);
		CVI_SYS_Munmap(dst_addr, size);
#endif
		teaisp_bnr_put_raw(pipe, &input_raw_addr, &output_raw_addr);

		bm_tensor_t temp;

		temp = input_fusion_img;
		input_fusion_img = output_fusion_img;
		output_fusion_img = temp;

		temp = input_sigma;
		input_sigma = output_sigma;
		output_sigma = temp;
	}

	//ISP_LOG_INFO("bnr launch thread end, %d\n", pipe);
	printf("bnr launch thread end, %d\n", pipe);

	return NULL;
}

static int start_launch_thread(VI_PIPE ViPipe)
{
	int pipe = ViPipe;

	// TODO: set priority
	bnr_ctx[pipe]->enable_lauch_thread = 1;
	pthread_create(&bnr_ctx[pipe]->launch_thread_id, NULL,
		teaisp_bnr_launch_thread, (void *) (uint64_t) pipe);

	return 0;
}

static int stop_launch_thread(VI_PIPE ViPipe)
{
	int pipe = ViPipe;

	bnr_ctx[pipe]->enable_lauch_thread = 0;
	pthread_join(bnr_ctx[pipe]->launch_thread_id, NULL);

	return 0;
}

CVI_S32 teaisp_bnr_load_model_wrap(VI_PIPE ViPipe, const char *path, void **model)
{
	bm_status_t status;
	TEAISP_MODEL_S *m = (TEAISP_MODEL_S *) ISP_CALLOC(1, sizeof(TEAISP_MODEL_S));

	if (m == NULL) {
		return CVI_FAILURE;
	}

	memset(m, 0, sizeof(TEAISP_MODEL_S));

	m->pipe = ViPipe;

	int maxDev = 0;

	CVI_TEAISP_GetMaxDev(ViPipe, &maxDev);

	if (maxDev > 1) {
		m->core_id = ViPipe % maxDev; // TODO:mason
	} else {
		m->core_id = 0;
	}

	ISP_LOG_INFO("load bmodel++: pipe:%d, core_id: %d, %s\n", ViPipe, m->core_id, path);

	m->p_bmrt = bmrt_create(bnr_ctx[ViPipe]->bm_handle);
	if (m->p_bmrt == NULL) {
		ISP_LOG_ERR("bmrt create fail\n");
		goto load_model_fail;
	}

	bool ret = bmrt_load_bmodel(m->p_bmrt, path);

	if (!ret) {
		ISP_LOG_ERR("load bmodel fail, %s\n", path);
		goto load_model_fail;
	}

	//bmrt_show_neuron_network(m->p_bmrt); // !!!

	bmrt_get_network_names(m->p_bmrt, &m->net_names);

	//int net_num = bmrt_get_network_number(m->p_bmrt);

	const bm_net_info_t *net_info = bmrt_get_network_info(m->p_bmrt, m->net_names[0]); // !!!

	ISP_LOG_INFO("net name: %s, in: %d, on: %d\n", m->net_names[0], net_info->input_num, net_info->output_num);

	m->input_num = net_info->input_num;
	m->output_num = net_info->output_num;

	if (m->input_num != BNR_IN_NUM || m->output_num != BNR_OUT_NUM) {
		ISP_LOG_ASSERT("model param num not match, in: %d, %d, out: %d, %d\n",
			m->input_num, BNR_IN_NUM, m->output_num, BNR_OUT_NUM);
		goto load_model_fail;
	}

	for (int index = 0; index < TEAISP_MAX_TUNING_INDEX; index++) {

		m->input_vaddr[index] = (void *) ISP_CALLOC(net_info->input_num, sizeof(void *));

		if (m->input_vaddr[index] == NULL) {
			goto load_model_fail;
		}

		m->input_tensors[index] = (bm_tensor_t *) ISP_CALLOC(net_info->input_num, sizeof(bm_tensor_t));

		if (m->input_tensors[index] == NULL) {
			goto load_model_fail;
		}

		m->output_tensors[index] = (bm_tensor_t *) ISP_CALLOC(net_info->input_num, sizeof(bm_tensor_t));

		if (m->output_tensors[index] == NULL) {
			goto load_model_fail;
		}

		for (int i = 0; i < net_info->input_num; i++) {
			m->input_tensors[index][i].dtype = net_info->input_dtypes[i];
			m->input_tensors[index][i].shape = net_info->stages[0].input_shapes[i];
			m->input_tensors[index][i].st_mode = BM_STORE_1N;

			if (i == BNR_IN_INPUT_IMG) {
				memset(&m->input_tensors[index][i].device_mem, 0, sizeof(bm_device_mem_t));
				m->input_tensors[index][i].device_mem.size = net_info->max_input_bytes[i];
			} else {

				if (index > 0 && (i == BNR_IN_FUSION_IMG || i == BNR_IN_SIGMA)) {
					m->input_tensors[index][i].device_mem = m->input_tensors[0][i].device_mem;
					continue;
				}

				status = bm_malloc_device_byte(bnr_ctx[ViPipe]->bm_handle,
					&m->input_tensors[index][i].device_mem,
					net_info->max_input_bytes[i]);
				if (status != BM_SUCCESS) {
					goto load_model_fail;
				}

				unsigned long long vmem;

				bm_mem_mmap_device_mem_no_cache(bnr_ctx[ViPipe]->bm_handle,
					&m->input_tensors[index][i].device_mem, &vmem);

				memset((void *) vmem, 0, net_info->max_input_bytes[i]);

				m->input_vaddr[index][i] = (void *) vmem;
			}

			ISP_LOG_ERR("index: %d, in: %d, dtype: %d, shape: %dx%dx%dx%d, %d, 0x%lx, size: %d\n",
				index, i,
				m->input_tensors[index][i].dtype,
				m->input_tensors[index][i].shape.dims[0], m->input_tensors[index][i].shape.dims[1],
				m->input_tensors[index][i].shape.dims[2], m->input_tensors[index][i].shape.dims[3],
				m->input_tensors[index][i].shape.num_dims,
				m->input_tensors[index][i].device_mem.u.device.device_addr,
				(int) net_info->max_input_bytes[i]);
		}

		for (int i = 0; i < net_info->output_num; i++) {
			m->output_tensors[index][i].dtype = net_info->output_dtypes[i];
			m->output_tensors[index][i].shape = net_info->stages[0].output_shapes[i];
			m->output_tensors[index][i].st_mode = BM_STORE_1N;

			if (i == BNR_OUT_INPUT_IMG) {
				memset(&m->output_tensors[index][i].device_mem, 0, sizeof(bm_device_mem_t));
				m->output_tensors[index][i].device_mem.size = net_info->max_output_bytes[i];
			} else {

				if (index > 0 && (i == BNR_OUT_FUSION_IMG || i == BNR_OUT_SIGMA)) {
					m->output_tensors[index][i].device_mem = m->output_tensors[0][i].device_mem;
					continue;
				}

				status = bm_malloc_device_byte(bnr_ctx[ViPipe]->bm_handle,
					&m->output_tensors[index][i].device_mem,
					net_info->max_output_bytes[i]);
				if (status != BM_SUCCESS) {
					goto load_model_fail;
				}

				unsigned long long vmem;

				bm_mem_mmap_device_mem(bnr_ctx[ViPipe]->bm_handle,
					&m->output_tensors[index][i].device_mem, &vmem);

				memset((void *) vmem, 0, net_info->max_output_bytes[i]);

				bm_mem_flush_device_mem(bnr_ctx[ViPipe]->bm_handle,
					&m->output_tensors[index][i].device_mem);
				bm_mem_unmap_device_mem(bnr_ctx[ViPipe]->bm_handle,
					(void *) vmem, (int) net_info->max_output_bytes[i]);
			}

			ISP_LOG_ERR("index: %d, out: %d, dtype: %d, shape: %dx%dx%dx%d, %d, 0x%lx, size: %d\n",
				index, i,
				m->output_tensors[index][i].dtype,
				m->output_tensors[index][i].shape.dims[0], m->output_tensors[index][i].shape.dims[1],
				m->output_tensors[index][i].shape.dims[2], m->output_tensors[index][i].shape.dims[3],
				m->output_tensors[index][i].shape.num_dims,
				m->output_tensors[index][i].device_mem.u.device.device_addr,
				(int) net_info->max_output_bytes[i]);
		}
	}

	*model = m;

	bnr_ctx[ViPipe]->bmodel0 = m;

	ISP_LOG_INFO("load bnr model success, %p\n", m);

	return CVI_SUCCESS;

load_model_fail:

	ISP_LOG_ERR("load bnr model error...\n");

	teaisp_bnr_unload_model_wrap(ViPipe, (void *) m);

	return CVI_FAILURE;
}

CVI_S32 teaisp_bnr_unload_model_wrap(VI_PIPE ViPipe, void *model)
{
	UNUSED(ViPipe);

	if (model == NULL) {
		return CVI_SUCCESS;
	}

	TEAISP_MODEL_S *m = (TEAISP_MODEL_S *) model;

	ISP_LOG_INFO("unload model++, %p\n", m);
	ISP_LOG_INFO("unload model wait..., %p, %p, %d\n", m,
		bnr_ctx[ViPipe]->bmodel1,
		bnr_ctx[ViPipe]->enable_lauch_thread);

	while (m == bnr_ctx[ViPipe]->bmodel1 &&
		 bnr_ctx[ViPipe]->bmodel1 != NULL &&
		 bnr_ctx[ViPipe]->enable_lauch_thread) {
		ISP_LOG_INFO("unload model wait..., %p, %p, %d\n", m,
			bnr_ctx[ViPipe]->bmodel1,
			bnr_ctx[ViPipe]->enable_lauch_thread);
		usleep(5 * 1000);
	}

	for (int index = 0; index < TEAISP_MAX_TUNING_INDEX; index++) {

		for (int i = 0; i < m->input_num; i++) {
			if (m->input_vaddr[index][i]) {
				bm_mem_unmap_device_mem(bnr_ctx[ViPipe]->bm_handle,
					m->input_vaddr[index][i],
					(int) m->input_tensors[index][i].device_mem.size);
			}
		}

		for (int i = 0; i < m->input_num; i++) {

			if (i == BNR_IN_INPUT_IMG) {
				continue;
			}

			if (index > 0 && (i == BNR_IN_FUSION_IMG || i == BNR_IN_SIGMA)) {
				continue;
			}

			if (m->input_tensors[index][i].device_mem.u.device.device_addr != 0) {
				bm_free_device(bnr_ctx[ViPipe]->bm_handle, m->input_tensors[index][i].device_mem);
			}
		}

		for (int i = 0; i < m->output_num; i++) {

			if (i == BNR_OUT_INPUT_IMG) {
				continue;
			}

			if (index > 0 && (i == BNR_OUT_FUSION_IMG || i == BNR_OUT_SIGMA)) {
				continue;
			}

			if (m->output_tensors[index][i].device_mem.u.device.device_addr != 0) {
				bm_free_device(bnr_ctx[ViPipe]->bm_handle,
					m->output_tensors[index][i].device_mem);
			}
		}

		ISP_RELEASE_MEMORY(m->input_vaddr[index]);
		ISP_RELEASE_MEMORY(m->input_tensors[index]);
		ISP_RELEASE_MEMORY(m->output_tensors[index]);
	}

	if (m->net_names) {
		free(m->net_names);
		m->net_names = NULL;
	}

	if (m->p_bmrt) {
		bmrt_destroy(m->p_bmrt);
	}

	ISP_RELEASE_MEMORY(m);

	return CVI_SUCCESS;
}

CVI_S32 teaisp_bnr_set_driver_init_wrap(VI_PIPE ViPipe)
{
	ISP_LOG_INFO("set driver init, %d\n", ViPipe);

	uint32_t swap_buf_index[2] = {0x0};
	ai_isp_bnr_cfg_t bnr_cfg;

	ai_isp_cfg_t cfg;

	memset(&cfg, 0, sizeof(ai_isp_cfg_t));

	cfg.ViPipe = ViPipe;
	cfg.ai_isp_cfg_type = AI_ISP_CFG_INIT;
	cfg.ai_isp_type = AI_ISP_TYPE_BNR;

	memset(&bnr_cfg, 0, sizeof(ai_isp_bnr_cfg_t));

	swap_buf_index[0] = ((BNR_OUT_FUSION_IMG << 16) | BNR_IN_FUSION_IMG);
	swap_buf_index[1] = ((BNR_OUT_SIGMA << 16) | BNR_IN_SIGMA);

	bnr_cfg.swap_buf_index = (uint64_t) swap_buf_index;
	bnr_cfg.swap_buf_count = 2;

	cfg.param_addr = (uint64_t) &bnr_cfg;
	cfg.param_size = sizeof(ai_isp_bnr_cfg_t);

	S_EXT_CTRLS_PTR(VI_IOCTL_AI_ISP_CFG, &cfg);

	bnr_ctx[ViPipe] = (TEAISP_BNR_CTX_S *) ISP_CALLOC(1, sizeof(TEAISP_BNR_CTX_S));

	if (bnr_ctx[ViPipe] == NULL) {
		return CVI_FAILURE;
	}

	bm_status_t status = bm_dev_request(&bnr_ctx[ViPipe]->bm_handle, 0);

	if (status != BM_SUCCESS) {
		ISP_LOG_ERR("request device fail, pipe: %d\n", ViPipe);
		return CVI_FAILURE;
	}

	return CVI_SUCCESS;
}

CVI_S32 teaisp_bnr_set_driver_deinit_wrap(VI_PIPE ViPipe)
{
	ISP_LOG_INFO("set driver deinit, %d\n", ViPipe);

	ai_isp_cfg_t cfg;

	memset(&cfg, 0, sizeof(ai_isp_cfg_t));

	if (bnr_ctx[ViPipe]) {
		bm_dev_free(bnr_ctx[ViPipe]->bm_handle);
		ISP_RELEASE_MEMORY(bnr_ctx[ViPipe]);
	}

	cfg.ViPipe = ViPipe;
	cfg.ai_isp_cfg_type = AI_ISP_CFG_DEINIT;
	cfg.ai_isp_type = AI_ISP_TYPE_BNR;

	S_EXT_CTRLS_PTR(VI_IOCTL_AI_ISP_CFG, &cfg);

	return CVI_SUCCESS;
}

CVI_S32 teaisp_bnr_set_driver_start_wrap(VI_PIPE ViPipe)
{
	ISP_LOG_INFO("set driver start, %d\n", ViPipe);

	ai_isp_cfg_t cfg;

	memset(&cfg, 0, sizeof(ai_isp_cfg_t));

	cfg.ViPipe = ViPipe;
	cfg.ai_isp_cfg_type = AI_ISP_CFG_ENABLE;
	cfg.ai_isp_type = AI_ISP_TYPE_BNR;

	S_EXT_CTRLS_PTR(VI_IOCTL_AI_ISP_CFG, &cfg);

	start_launch_thread(ViPipe);

	return CVI_SUCCESS;
}

CVI_S32 teaisp_bnr_set_driver_stop_wrap(VI_PIPE ViPipe)
{
	ISP_LOG_INFO("set driver stop, %d\n", ViPipe);

	stop_launch_thread(ViPipe);

	ai_isp_cfg_t cfg;

	memset(&cfg, 0, sizeof(ai_isp_cfg_t));

	cfg.ViPipe = ViPipe;
	cfg.ai_isp_cfg_type = AI_ISP_CFG_DISABLE;
	cfg.ai_isp_type = AI_ISP_TYPE_BNR;

	S_EXT_CTRLS_PTR(VI_IOCTL_AI_ISP_CFG, &cfg);

	return CVI_SUCCESS;
}

CVI_S32 teaisp_bnr_set_api_info_wrap(VI_PIPE ViPipe, void *model, void *param, int is_new)
{
	TEAISP_MODEL_S *m = (TEAISP_MODEL_S *) model;

	ISP_LOG_INFO("bnr set api info, %d, %d, %p\n", ViPipe, m->core_id, model);

	struct teaisp_bnr_config *bnr_cfg = (struct teaisp_bnr_config *) param;

	m->tuning_index++;

	uint8_t tuning_index = m->tuning_index % TEAISP_MAX_TUNING_INDEX;

	memcpy(m->input_vaddr[tuning_index][BNR_IN_COEFF_A], &bnr_cfg->coeff_a,
		m->input_tensors[tuning_index][BNR_IN_COEFF_A].device_mem.size);

	memcpy(m->input_vaddr[tuning_index][BNR_IN_COEFF_B], &bnr_cfg->coeff_b,
		m->input_tensors[tuning_index][BNR_IN_COEFF_B].device_mem.size);

	memcpy(m->input_vaddr[tuning_index][BNR_IN_BLEND], &bnr_cfg->blend,
		m->input_tensors[tuning_index][BNR_IN_BLEND].device_mem.size);

	memcpy(m->input_vaddr[tuning_index][BNR_IN_MOTION_STR_2D], &bnr_cfg->filter_motion_str_2d,
		m->input_tensors[tuning_index][BNR_IN_MOTION_STR_2D].device_mem.size);

	memcpy(m->input_vaddr[tuning_index][BNR_IN_STATIC_STR_2D], &bnr_cfg->filter_static_str_2d,
		m->input_tensors[tuning_index][BNR_IN_STATIC_STR_2D].device_mem.size);

	memcpy(m->input_vaddr[tuning_index][BNR_IN_STR_3D], &bnr_cfg->filter_str_3d,
		m->input_tensors[tuning_index][BNR_IN_STR_3D].device_mem.size);

	if ((int) m->input_tensors[tuning_index][BNR_IN_BLACK_LEVEL].device_mem.size == (int) sizeof(float)) {
		memcpy(m->input_vaddr[tuning_index][BNR_IN_BLACK_LEVEL], &bnr_cfg->blc,
			m->input_tensors[tuning_index][BNR_IN_BLACK_LEVEL].device_mem.size);
	} else {
		int lblc_cnt = 0;
		float *lblc = (float *) m->input_vaddr[tuning_index][BNR_IN_BLACK_LEVEL];

		for (int i = 0; i < ISP_LBLC_GRID_POINTS; i++) {
			*lblc++ = (float) bnr_cfg->lblcOffsetR[i];
			lblc_cnt++;
		}

		for (int i = 0; i < ISP_LBLC_GRID_POINTS; i++) {
			*lblc++ = (float) bnr_cfg->lblcOffsetGr[i];
			lblc_cnt++;
		}

		for (int i = 0; i < ISP_LBLC_GRID_POINTS; i++) {
			*lblc++ = (float) bnr_cfg->lblcOffsetB[i];
			lblc_cnt++;
		}

		for (int i = 0; i < ISP_LBLC_GRID_POINTS; i++) {
			*lblc++ = (float) bnr_cfg->lblcOffsetGb[i];
			lblc_cnt++;
		}

		if ((int) (lblc_cnt * sizeof(float)) !=
			(int) m->input_tensors[tuning_index][BNR_IN_BLACK_LEVEL].device_mem.size) {
			ISP_LOG_ERR("fill lblc cnt error, %d, %d\n",
				lblc_cnt, m->input_tensors[tuning_index][BNR_IN_BLACK_LEVEL].device_mem.size);
		}
	}

	ISP_LOG_INFO("int bnr param: %d, %d, %d, %d, %d, %d, %d\n",
		*((uint32_t *) m->input_vaddr[tuning_index][BNR_IN_COEFF_A]),
		*((uint32_t *) m->input_vaddr[tuning_index][BNR_IN_COEFF_B]),
		*((uint32_t *) m->input_vaddr[tuning_index][BNR_IN_BLEND]),
		*((uint32_t *) m->input_vaddr[tuning_index][BNR_IN_MOTION_STR_2D]),
		*((uint32_t *) m->input_vaddr[tuning_index][BNR_IN_STATIC_STR_2D]),
		*((uint32_t *) m->input_vaddr[tuning_index][BNR_IN_BLACK_LEVEL]),
		*((uint32_t *) m->input_vaddr[tuning_index][BNR_IN_STR_3D]));

	ISP_LOG_ERR("float bnr param: %.5f, %.5f, %.5f, %.5f, %.5f, %.5f, %.5f\n",
		*((float *) m->input_vaddr[tuning_index][BNR_IN_COEFF_A]),
		*((float *) m->input_vaddr[tuning_index][BNR_IN_COEFF_B]),
		*((float *) m->input_vaddr[tuning_index][BNR_IN_BLEND]),
		*((float *) m->input_vaddr[tuning_index][BNR_IN_MOTION_STR_2D]),
		*((float *) m->input_vaddr[tuning_index][BNR_IN_STATIC_STR_2D]),
		*((float *) m->input_vaddr[tuning_index][BNR_IN_BLACK_LEVEL]),
		*((float *) m->input_vaddr[tuning_index][BNR_IN_STR_3D]));

	UNUSED(is_new);

	return CVI_SUCCESS;
}

