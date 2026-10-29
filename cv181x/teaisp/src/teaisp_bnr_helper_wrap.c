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

#include "cviruntime.h"

#include "teaisp_bnr_helper_wrap.h"

#if defined(__riscv_vector)
#include <riscv_vector.h>
#endif

#define BNR_IN_INPUT_IMG      (0)
#define BNR_IN_FUSION_IMG     (1)
#define BNR_IN_SIGMA          (2)
#define BNR_IN_COEFF_A        (3)
#define BNR_IN_BLEND          (4)
#define BNR_IN_BLACK_LEVEL    (5)
#define BNR_IN_NUM            (6)

#define BNR_OUT_INPUT_IMG     (0)
#define BNR_OUT_SIGMA         (1)
#define BNR_OUT_GAMMA         (2)
#define BNR_OUT_NOISY         (3)
#define BNR_OUT_NUM           (4)


#define CLIP(value, min_val, max_val) \
	((value) < (min_val) ? (min_val) : ((value) > (max_val) ? (max_val) : (value)))

#define VCLIP_CONST_F(src, min_val, max_val, vl, SUFFIX) ({ \
	typeof(src) tmp = vfmax_vf_##SUFFIX((src), (min_val), (vl)); \
	vfmin_vf_##SUFFIX(tmp, (max_val), (vl)); \
})

#define VCLIP_F32_CONST(src, min, max, vl) \
	VCLIP_CONST_F(src, min, max, vl, f32m8)

typedef struct {
	int pipe;
	uint32_t core_id;
	uint32_t tuning_index;
	CVI_MODEL_HANDLE cm_handle;
	const char **net_names;
	CVI_TENSOR *input_tensors;
	CVI_TENSOR *output_tensors;
	int input_num;
	int output_num;
	float coeff_a_buff;
	float aegain_buff;
	float blend_buff;
	float black_level_buff;
} TEAISP_MODEL_S;

typedef struct {
	uint8_t enable_lauch_thread;
	pthread_t launch_thread_id;

	TEAISP_MODEL_S *cmodel0;
	TEAISP_MODEL_S *cmodel1;
	TEAISP_MODEL_TYPE_E enModelType;
} TEAISP_BNR_CTX_S;

static TEAISP_BNR_CTX_S *bnr_ctx[VI_MAX_PIPE_NUM];

static int teaisp_bnr_get_raw(VI_PIPE ViPipe, uint64_t *input_raw, uint64_t *output_raw, uint64_t *rgbmap)
{
	uint64_t tmp[3] = {0, 0, 0};

	G_EXT_CTRLS_PTR(VI_IOCTL_GET_AI_ISP_RAW, &tmp);

	*input_raw = tmp[0];
	*output_raw = tmp[1];
	*rgbmap = tmp[2];

	return 0;
}

static int teaisp_bnr_put_raw(VI_PIPE ViPipe, uint64_t *input_raw, uint64_t *output_raw, uint64_t *rgbmap)
{
	uint64_t tmp[3] = {0, 0, 0};

	tmp[0] = *input_raw;
	tmp[1] = *output_raw;
	tmp[2] = *rgbmap;

	S_EXT_CTRLS_PTR(VI_IOCTL_PUT_AI_ISP_RAW, &tmp);

	return 0;
}

static void teaisp_bnr_dump_input(TEAISP_MODEL_S *model, uint64_t input_raw_addr, int pipe)
{
	void *addr_tmp = NULL;
	int size = 0;
	FILE *fp = NULL;
	char path[128] = {0};

	snprintf(path, sizeof(path), "/tmp/dump/input0_%d_img.bin", pipe);
	size = model->input_tensors[0].mem_size;
	addr_tmp = CVI_SYS_MmapCache(input_raw_addr, size);
	fp = fopen(path, "wb");
	fwrite(addr_tmp, size, 1, fp);
	fclose(fp);
	CVI_SYS_Munmap(addr_tmp, size);

	snprintf(path, sizeof(path), "/tmp/dump/input1_%d_fusion.bin", pipe);
	size = model->input_tensors[BNR_IN_FUSION_IMG].mem_size;
	addr_tmp = CVI_NN_TensorPtr(&model->input_tensors[BNR_IN_FUSION_IMG]);
	fp = fopen(path, "wb");
	fwrite(addr_tmp, size, 1, fp);
	fclose(fp);

	snprintf(path, sizeof(path), "/tmp/dump/input2_%d_sigma.bin", pipe);
	size = model->input_tensors[BNR_IN_SIGMA].mem_size;
	addr_tmp = CVI_NN_TensorPtr(&model->input_tensors[BNR_IN_SIGMA]);
	fp = fopen(path, "wb");
	fwrite(addr_tmp, size, 1, fp);
	fclose(fp);

	snprintf(path, sizeof(path), "/tmp/dump/input3_%d_coeff_a.bin", pipe);
	size = model->input_tensors[BNR_IN_COEFF_A].mem_size;
	addr_tmp = CVI_NN_TensorPtr(&model->input_tensors[BNR_IN_COEFF_A]);
	fp = fopen(path, "wb");
	fwrite(addr_tmp, size, 1, fp);
	fclose(fp);

	snprintf(path, sizeof(path), "/tmp/dump/input4_%d_blend.bin", pipe);
	size = model->input_tensors[BNR_IN_BLEND].mem_size;
	addr_tmp = &model->blend_buff;
	fp = fopen(path, "wb");
	fwrite(addr_tmp, size, 1, fp);
	fclose(fp);

	snprintf(path, sizeof(path), "/tmp/dump/input5_%d_blc.bin", pipe);
	size = model->input_tensors[BNR_IN_BLACK_LEVEL].mem_size;
	addr_tmp = &model->black_level_buff;
	fp = fopen(path, "wb");
	fwrite(addr_tmp, size, 1, fp);
	fclose(fp);

}

static void teaisp_bnr_dump_output(TEAISP_MODEL_S *model, uint64_t output_raw_addr, int pipe)
{
	void *addr_tmp = NULL;
	int size = 0;
	FILE *fp = NULL;
	char path[128] = {0};

	snprintf(path, sizeof(path), "/tmp/dump/output0_%d_img.bin", pipe);
	size = model->output_tensors[0].mem_size;
	addr_tmp = CVI_SYS_MmapCache(output_raw_addr, size);
	fp = fopen(path, "wb");
	fwrite(addr_tmp, size, 1, fp);
	fclose(fp);
	CVI_SYS_Munmap(addr_tmp, size);

	snprintf(path, sizeof(path), "/tmp/dump/output1_%d_sigma.bin", pipe);
	size = model->output_tensors[BNR_OUT_SIGMA].mem_size;
	addr_tmp = CVI_NN_TensorPtr(&model->output_tensors[BNR_OUT_SIGMA]);
	fp = fopen(path, "wb");
	fwrite(addr_tmp, size, 1, fp);
	fclose(fp);

	snprintf(path, sizeof(path), "/tmp/dump/output2_%d_gamma.bin", pipe);
	size = model->output_tensors[BNR_OUT_GAMMA].mem_size;
	addr_tmp = CVI_NN_TensorPtr(&model->output_tensors[BNR_OUT_GAMMA]);
	fp = fopen(path, "wb");
	fwrite(addr_tmp, size, 1, fp);
	fclose(fp);

	snprintf(path, sizeof(path), "/tmp/dump/output3_%d_noisy.bin", pipe);
	size = model->output_tensors[BNR_OUT_NOISY].mem_size;
	addr_tmp = CVI_NN_TensorPtr(&model->output_tensors[BNR_OUT_NOISY]);
	fp = fopen(path, "wb");
	fwrite(addr_tmp, size, 1, fp);
	fclose(fp);
}

static float teaisp_bnr_model_preprocess(TEAISP_MODEL_S *model)
{
	float coeff_a = model->coeff_a_buff;
	float black_level = model->black_level_buff;
	float aegain = model->aegain_buff;
	CVI_TENSOR *fusion_img_t = &model->input_tensors[BNR_IN_FUSION_IMG];

	float *fusion_img = (float *)CVI_SYS_MmapCache(fusion_img_t->paddr, fusion_img_t->mem_size);
	float cvt_k = CLIP(0.024641f / coeff_a, 0.2f, 20.0f);
	float scale_factor = cvt_k / (4319.0f - black_level);
	float scale_aegain = scale_factor / aegain;
	float bias_factor = 112.0f * scale_factor;

	CVI_SYS_IonInvalidateCache(fusion_img_t->paddr, fusion_img, fusion_img_t->mem_size);

#if defined(__riscv_vector)
	size_t vl;
	for (size_t pos = 0; pos < fusion_img_t->count; pos += vl) {
		vl = vsetvl_e32m8(fusion_img_t->count - pos);

		vfloat32m8_t v_fusion = vle32_v_f32m8(fusion_img + pos, vl);

		v_fusion = VCLIP_F32_CONST(v_fusion, 0.0f, 4095.0f, vl);

		//fusion_img = fusion_img*scale_aegain + bias_factor
		v_fusion = vfadd_vf_f32m8(vfmul_vf_f32m8(v_fusion, scale_aegain, vl),
															bias_factor, vl);

		vse32_v_f32m8(fusion_img + pos, v_fusion, vl);
	}
#else
	for (size_t i = 0; i < fusion_img_t->count; i++) {
		fusion_img[i] = CLIP(fusion_img[i], 0.0f, 4095.0f) * scale_aegain + bias_factor;
	}
#endif

	CVI_SYS_IonFlushCache(fusion_img_t->paddr, fusion_img, fusion_img_t->mem_size);
	CVI_SYS_Munmap(fusion_img, fusion_img_t->mem_size);

	return cvt_k;
}

static void teaisp_bnr_model_postprocess(TEAISP_MODEL_S *model, float cvt_k)
{
	CVI_TENSOR *fusion_img_t = &model->input_tensors[BNR_IN_FUSION_IMG];
	CVI_TENSOR *pre_sigma_t = &model->input_tensors[BNR_IN_SIGMA];
	CVI_TENSOR *out_img_t = &model->output_tensors[BNR_OUT_INPUT_IMG];
	CVI_TENSOR *curr_sigma_t = &model->output_tensors[BNR_OUT_SIGMA];
	CVI_TENSOR *gamma_t = &model->output_tensors[BNR_OUT_GAMMA];
	CVI_TENSOR *noisy_out_t = &model->output_tensors[BNR_OUT_NOISY];

	if (fusion_img_t->count != noisy_out_t->count ||
		fusion_img_t->count * 1.5 != out_img_t->count ||
		pre_sigma_t->count != curr_sigma_t->count) {
		ISP_LOG_ERR("model shape error!\n");
		ISP_LOG_ERR("fusion_img_t->count %d, noisy_out_t->count %d, out_img_t->count %d\n",
					fusion_img_t->count, noisy_out_t->count, out_img_t->count);
		ISP_LOG_ERR("pre_sigma_t->count %d, curr_sigma_t->count %d\n",
					pre_sigma_t->count, curr_sigma_t->count);
		return;
	}

	uint8_t *out_img_addr = (uint8_t *)CVI_SYS_MmapCache(out_img_t->paddr, out_img_t->mem_size);
	float *fusion_img = (float *)CVI_SYS_MmapCache(fusion_img_t->paddr, fusion_img_t->mem_size);
	float *prev_sigma = (float *)CVI_SYS_MmapCache(pre_sigma_t->paddr, pre_sigma_t->mem_size);
	float *noisy_out = (float *)CVI_NN_TensorPtr(noisy_out_t);
	float *gamma = (float *)CVI_NN_TensorPtr(gamma_t);
	float *curr_sigma = (float *)CVI_NN_TensorPtr(curr_sigma_t);
	float black_level = model->black_level_buff;
	float scale_factor = (4319.0f - black_level) / cvt_k;
	size_t gamma_up_count = fusion_img_t->count / fusion_img_t->shape.dim[1];

	CVI_SYS_IonInvalidateCache(out_img_t->paddr, out_img_addr, out_img_t->mem_size);
	CVI_SYS_IonInvalidateCache(fusion_img_t->paddr, fusion_img, fusion_img_t->mem_size);
	CVI_SYS_IonInvalidateCache(pre_sigma_t->paddr, prev_sigma, pre_sigma_t->mem_size);

#if defined(__riscv_vector)
	size_t vl;
	size_t chn = fusion_img_t->shape.dim[1];
	for (size_t c = 0; c < chn; c++) {
		for (size_t i = 0; i < gamma_up_count; i += vl) {
			vl = vsetvl_e32m8(gamma_up_count - i);

			vuint8m2_t v_gamma_u8 = vlsbu_v_u8m2(out_img_addr + i * 6, 6, vl);
			vuint16m4_t v_gamma_u16 = vwcvtu_x_x_v_u16m4(v_gamma_u8, vl);
			vfloat32m8_t v_gamma_up = vfwcvt_f_xu_v_f32m8(v_gamma_u16, vl);

			v_gamma_up = vfdiv_vf_f32m8(v_gamma_up, 255.0f, vl);

			vfloat32m8_t v_fusion = vle32_v_f32m8(&fusion_img[gamma_up_count * c + i], vl);
			vfloat32m8_t v_noisy = vle32_v_f32m8(&noisy_out[gamma_up_count * c + i], vl);

			//fusion = fusion*(1-gamma) + noisy*gamma
			vfloat32m8_t v_one_minus = vfrsub_vf_f32m8(v_gamma_up, 1.0f, vl);
			vfloat32m8_t v_result = vfadd_vv_f32m8(vfmul_vv_f32m8(v_fusion, v_one_minus, vl),
												vfmul_vv_f32m8(v_noisy, v_gamma_up, vl), vl);

			//fusion = fusion*scale_factor - 112.0f
			v_result = vfmsub_vf_f32m8(v_result, scale_factor, vfmv_v_f_f32m8(112.0f, vl), vl);

			vse32_v_f32m8(&fusion_img[gamma_up_count * c + i], v_result, vl);
		}
	}

	for (size_t i = 0; i < pre_sigma_t->count; i += vl) {
		vl = vsetvl_e32m8(pre_sigma_t->count - i);

		vfloat32m8_t v_gamma = vle32_v_f32m8(gamma + i, vl);
		vfloat32m8_t v_prev = vle32_v_f32m8(prev_sigma + i, vl);
		vfloat32m8_t v_curr = vle32_v_f32m8(curr_sigma + i, vl);

		// gamma[i] * gamma[i] * prev_sigma[i]
		vfloat32m8_t v_gamma_sq = vfmul_vv_f32m8(v_gamma, v_gamma, vl);
		vfloat32m8_t term1 = vfmul_vv_f32m8(v_gamma_sq, v_prev, vl);

		// (1 - gamma[i]) * (1 - gamma[i]) * curr_sigma[i]
		vfloat32m8_t v_omg = vfsub_vf_f32m8(v_gamma, 1.0f, vl); //(gamma-1)^2 = (1-gamma)^2
		vfloat32m8_t v_omg_sq = vfmul_vv_f32m8(v_omg, v_omg, vl);
		vfloat32m8_t term2 = vfmul_vv_f32m8(v_omg_sq, v_curr, vl);

		vfloat32m8_t v_res = vfadd_vv_f32m8(term1, term2, vl);
		vse32_v_f32m8(prev_sigma + i, v_res, vl);
	}
#else
	float gamma_up = 0;
	for (size_t i = 0; i < fusion_img_t->count; i++) {
		gamma_up = out_img_addr[i % gamma_up_count * 6] / 255.0f;
		fusion_img[i] = fusion_img[i] * (1.0 - gamma_up) + noisy_out[i] * gamma_up;
		fusion_img[i] = fusion_img[i] * scale_factor - 112.0f;
	}

	for (size_t i = 0; i < pre_sigma_t->count; i++) {
		prev_sigma[i] = gamma[i] * gamma[i] * prev_sigma[i] + (1 - gamma[i]) * (1 - gamma[i]) * curr_sigma[i];
	}
#endif

	CVI_SYS_IonFlushCache(out_img_t->paddr, out_img_addr, out_img_t->mem_size);
	CVI_SYS_IonFlushCache(fusion_img_t->paddr, fusion_img, fusion_img_t->mem_size);
	CVI_SYS_IonFlushCache(pre_sigma_t->paddr, prev_sigma, pre_sigma_t->mem_size);
	CVI_SYS_Munmap(out_img_addr, out_img_t->mem_size);
	CVI_SYS_Munmap(fusion_img, fusion_img_t->mem_size);
	CVI_SYS_Munmap(prev_sigma, pre_sigma_t->mem_size);
}

static void *teaisp_bnr_launch_thread(void *param)
{
	int pipe = (int) (uintptr_t) param;
	uint64_t input_raw_addr = 0;
	uint64_t output_raw_addr = 0;
	uint64_t rgbmap_addr = 0;
	uint64_t output_addr = 0;

	//ISP_LOG_INFO("run bnr launch thread, %d\n", pipe);
	printf("run bnr launch thread, %d\n", pipe);

reset_launch:

	while (bnr_ctx[pipe]->enable_lauch_thread) {
		if (bnr_ctx[pipe]->cmodel0 == NULL) {
			usleep(5 * 1000);
		} else {
			break;
		}
	}

	TEAISP_MODEL_S *model = bnr_ctx[pipe]->cmodel0;

	bnr_ctx[pipe]->cmodel1 = bnr_ctx[pipe]->cmodel0;

	// wait param update
	while (bnr_ctx[pipe]->enable_lauch_thread) {
		if (model->tuning_index <= 0) {
			usleep(5 * 1000);
		} else {
			break;
		}
	}

	// launch
	while (bnr_ctx[pipe]->enable_lauch_thread) {

		if (bnr_ctx[pipe]->cmodel0 != bnr_ctx[pipe]->cmodel1) {
			//ISP_LOG_INFO("reset bnr launch....\n");
			printf("reset bnr launch....\n");
			goto reset_launch;
		}

		teaisp_bnr_get_raw(pipe, &input_raw_addr, &output_raw_addr, &rgbmap_addr);

		output_addr = output_raw_addr;
		if (bnr_ctx[pipe]->enModelType == TEAISP_MODEL_MOTION) {
			output_addr = rgbmap_addr;
		}

#ifndef ENABLE_BYPASS_TPU
		CVI_NN_SetTensorPhysicalAddr(&model->input_tensors[0], input_raw_addr);
		CVI_NN_SetTensorPhysicalAddr(&model->output_tensors[0], output_addr);

		if (access("/tmp/teaisp_bnr_dump", F_OK) == 0) {
                	system("mkdir -p /tmp/dump");
			teaisp_bnr_dump_input(model, input_raw_addr, pipe);
			system("rm /tmp/teaisp_bnr_dump;touch /tmp/teaisp_bnr_dump_output");
		}

		//struct timeval tv1, tv2;
		//gettimeofday(&tv1, NULL);

		float cvt_k = teaisp_bnr_model_preprocess(model);

		memcpy(CVI_NN_TensorPtr(&model->input_tensors[BNR_IN_COEFF_A]), &cvt_k, sizeof(float));
		memcpy(CVI_NN_TensorPtr(&model->input_tensors[BNR_IN_BLEND]), &model->blend_buff, sizeof(float));
		memcpy(CVI_NN_TensorPtr(&model->input_tensors[BNR_IN_BLACK_LEVEL]),
								&model->black_level_buff, sizeof(float));

		CVI_RC ret = CVI_NN_Forward(model->cm_handle, model->input_tensors, model->input_num,
									model->output_tensors, model->output_num);

		teaisp_bnr_model_postprocess(model, cvt_k);

		if (ret != CVI_RC_SUCCESS) {
			ISP_LOG_ERR("model forward failed.\n");
		}

		if (access("/tmp/teaisp_bnr_dump_output", F_OK) == 0) {
			teaisp_bnr_dump_output(model, output_addr, pipe);
			system("rm /tmp/teaisp_bnr_dump_output");
			system("sync");
		}

		//gettimeofday(&tv2, NULL);
		//ISP_LOG_ERR("launch time diff, %d, %d\n",
		//	pipe, ((tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec)));

#else
		void *src_addr;
		void *dst_addr;

		int size = model->input_tensors[0].mem_size;

		src_addr = CVI_SYS_MmapCache(input_raw_addr, size);
		dst_addr = CVI_SYS_MmapCache(output_raw_addr, size);

		memcpy(dst_addr, src_addr, size);

		usleep(45 * 1000);

		CVI_SYS_IonFlushCache(output_raw_addr, dst_addr, size);

		CVI_SYS_Munmap(src_addr, size);
		CVI_SYS_Munmap(dst_addr, size);
#endif

		if (bnr_ctx[pipe]->enModelType == TEAISP_MODEL_MOTION)
			teaisp_bnr_put_raw(pipe, &output_raw_addr, &input_raw_addr, &rgbmap_addr);
		else
			teaisp_bnr_put_raw(pipe, &input_raw_addr, &output_raw_addr, &rgbmap_addr);
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
		teaisp_bnr_launch_thread, (void *) (uintptr_t) pipe);

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
	TEAISP_MODEL_S *m = (TEAISP_MODEL_S *) ISP_CALLOC(1, sizeof(TEAISP_MODEL_S));

	if (m == NULL) {
		return CVI_FAILURE;
	}

	struct timeval tv1, tv2;

	gettimeofday(&tv1, NULL);

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

	if (bnr_ctx[ViPipe] == NULL) {
		bnr_ctx[ViPipe] = (TEAISP_BNR_CTX_S *) ISP_CALLOC(1, sizeof(TEAISP_BNR_CTX_S));

		if (bnr_ctx[ViPipe] == NULL) {
			return CVI_FAILURE;
		}
		bnr_ctx[ViPipe]->enModelType = TEAISP_MODEL_NONE;
	}
	bnr_ctx[ViPipe]->enModelType = TEAISP_MODEL_BNR;

	CVI_RC ret = CVI_NN_RegisterModel(path, &m->cm_handle);

	if (ret != CVI_RC_SUCCESS) {
		ISP_LOG_ERR("Failed to register and load model %s\n", path);
		goto load_model_fail;
	}

	//CVI_NN_SetConfig(m->cm_handle, OPTION_PROGRAM_INDEX, m->core_id);
	//CVI_NN_SetConfig(m->cm_handle, OPTION_OUTPUT_ALL_TENSORS, true);
	CVI_NN_SetConfig(m->cm_handle, OPTION_IOMEM_EMPTY);

	ret = CVI_NN_GetInputOutputTensors(m->cm_handle, &m->input_tensors, &m->input_num,
												&m->output_tensors, &m->output_num);
	if (ret != CVI_RC_SUCCESS) {
		ISP_LOG_ERR("Failed to get inputs & outputs from model\n");
		goto load_model_fail;
	}

	ISP_LOG_INFO("model path: %s, in: %d, on: %d\n", path, m->input_num, m->output_num);

	if (m->input_num != BNR_IN_NUM || m->output_num != BNR_OUT_NUM) {
		ISP_LOG_ASSERT("model param num not match, in: %d, %d, out: %d, %d\n",
			m->input_num, BNR_IN_NUM, m->output_num, BNR_OUT_NUM);
		goto load_model_fail;
	}

	for (int i = 0; i < m->input_num; i++) {
		ISP_LOG_ERR("%s in: %d, dtype: %d, shape: %dx%dx%dx%d, %d, 0x%lx, mem_size: %d, count: %d\n",
			m->input_tensors[i].name, i, m->input_tensors[i].fmt,
			m->input_tensors[i].shape.dim[0], m->input_tensors[i].shape.dim[1],
			m->input_tensors[i].shape.dim[2], m->input_tensors[i].shape.dim[3],
			m->input_tensors[i].shape.dim_size,
			m->input_tensors[i].paddr,
			m->input_tensors[i].mem_size,
			m->input_tensors[i].count);

		if (i == BNR_IN_INPUT_IMG) {
			continue;
		}
		if (i == BNR_IN_FUSION_IMG || i == BNR_IN_SIGMA) {
			CVI_VOID *input_vaddr_tmp;
			CVI_U64 input_addr_tmp;
			CVI_TENSOR *tensor_tmp = &m->input_tensors[i];

			CVI_SYS_IonAlloc_Cached(&input_addr_tmp, &input_vaddr_tmp, NULL, tensor_tmp->mem_size);
			CVI_NN_SetTensorPhysicalAddr(tensor_tmp, input_addr_tmp);
			CVI_SYS_IonInvalidateCache(tensor_tmp->paddr, input_vaddr_tmp, tensor_tmp->mem_size);
			memset(input_vaddr_tmp, 0, tensor_tmp->mem_size);
			CVI_SYS_IonFlushCache(tensor_tmp->paddr, input_vaddr_tmp, tensor_tmp->mem_size);
			CVI_SYS_Munmap(input_vaddr_tmp, tensor_tmp->mem_size);
			continue;
		}
		memset((void *)CVI_NN_TensorPtr(&m->input_tensors[i]), 0, m->input_tensors[i].mem_size);
	}

	for (int i = 0; i < m->output_num; i++) {
		//memset((void *)CVI_NN_TensorPtr(&m->output_tensors[i]), 0, m->output_tensors[i].mem_size);
		ISP_LOG_ERR("%s out: %d, dtype: %d, shape: %dx%dx%dx%d, %d, 0x%lx, mem_size: %d, count: %d\n",
			m->output_tensors[i].name, i, m->output_tensors[i].fmt,
			m->output_tensors[i].shape.dim[0], m->output_tensors[i].shape.dim[1],
			m->output_tensors[i].shape.dim[2], m->output_tensors[i].shape.dim[3],
			m->output_tensors[i].shape.dim_size,
			m->output_tensors[i].paddr,
			m->output_tensors[i].mem_size,
			m->output_tensors[i].count);
	}

	*model = m;

	gettimeofday(&tv2, NULL);

	//ISP_LOG_INFO("load bnr model success, %p\n", m);
	printf("load cvimodel success: pipe:%d, core_id: %d, cost time: %ld, %s\n", ViPipe, m->core_id,
		(long int)((tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec)), path);

	int input_size = m->input_tensors[0].mem_size;
	int output_size = m->output_tensors[0].mem_size;

	if (input_size != output_size) {
		bnr_ctx[ViPipe]->enModelType = TEAISP_MODEL_MOTION;
	}

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
		bnr_ctx[ViPipe]->cmodel1,
		bnr_ctx[ViPipe]->enable_lauch_thread);

	while (m == bnr_ctx[ViPipe]->cmodel1 &&
		 bnr_ctx[ViPipe]->cmodel1 != NULL &&
		 bnr_ctx[ViPipe]->enable_lauch_thread) {
		ISP_LOG_INFO("unload model wait..., %p, %p, %d\n", m,
			bnr_ctx[ViPipe]->cmodel1,
			bnr_ctx[ViPipe]->enable_lauch_thread);
		usleep(5 * 1000);
	}

	if (m->net_names) {
		free(m->net_names);
		m->net_names = NULL;
	}
	CVI_SYS_IonFree(m->input_tensors[BNR_IN_FUSION_IMG].paddr, NULL);
	CVI_SYS_IonFree(m->input_tensors[BNR_IN_SIGMA].paddr, NULL);

	if (m->cm_handle) {
		CVI_NN_CleanupModel(m->cm_handle);
	}

	ISP_RELEASE_MEMORY(m);

	bnr_ctx[ViPipe]->enModelType = TEAISP_MODEL_NONE;

	return CVI_SUCCESS;
}

CVI_S32 teaisp_bnr_set_driver_init_wrap(VI_PIPE ViPipe)
{
	ISP_LOG_INFO("set driver init, %d\n", ViPipe);

	uint32_t swap_buf_index[2] = {0x0};
	ai_isp_bnr_cfg_t bnr_cfg;

	ai_isp_cfg_t cfg;

	memset(&cfg, 0, sizeof(ai_isp_cfg_t));

	cfg.vi_pipe = ViPipe;
	cfg.ai_isp_cfg_type = AI_ISP_CFG_INIT;
	cfg.ai_isp_type = AI_ISP_TYPE_BNR;

	memset(&bnr_cfg, 0, sizeof(ai_isp_bnr_cfg_t));

	swap_buf_index[0] = ((BNR_OUT_NOISY << 16) | BNR_IN_FUSION_IMG);
	swap_buf_index[1] = ((BNR_OUT_SIGMA << 16) | BNR_IN_SIGMA);

	bnr_cfg.swap_buf_index = (uint64_t)(uintptr_t) swap_buf_index;
	bnr_cfg.swap_buf_count = 2;
	bnr_cfg.ai_rgbmap = false;

	//if you want this work, you must set ENABLE_PRELOAD_BNR_MODEL to 1 in teaisp_bnr_ctrl.c
	if (bnr_ctx[ViPipe] && bnr_ctx[ViPipe]->enModelType == TEAISP_MODEL_MOTION) {
		bnr_cfg.ai_rgbmap = true;
	}

	ISP_LOG_ERR("set ai_rgbmap: %d\n", bnr_cfg.ai_rgbmap);

	cfg.param_addr = (uint64_t)(uintptr_t) &bnr_cfg;
	cfg.param_size = sizeof(ai_isp_bnr_cfg_t);

	S_EXT_CTRLS_PTR(VI_IOCTL_AI_ISP_CFG, &cfg);

	if (bnr_ctx[ViPipe] == NULL) {
		bnr_ctx[ViPipe] = (TEAISP_BNR_CTX_S *) ISP_CALLOC(1, sizeof(TEAISP_BNR_CTX_S));

		if (bnr_ctx[ViPipe] == NULL) {
			return CVI_FAILURE;
		}
		bnr_ctx[ViPipe]->enModelType = TEAISP_MODEL_NONE;
	}

	return CVI_SUCCESS;
}

CVI_S32 teaisp_bnr_set_driver_deinit_wrap(VI_PIPE ViPipe)
{
	ISP_LOG_INFO("set driver deinit, %d\n", ViPipe);

	ai_isp_cfg_t cfg;

	memset(&cfg, 0, sizeof(ai_isp_cfg_t));

	cfg.vi_pipe = ViPipe;
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

	cfg.vi_pipe = ViPipe;
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

	cfg.vi_pipe = ViPipe;
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

	memcpy(&m->coeff_a_buff, &bnr_cfg->coeff_a, sizeof(float));
	memcpy(&m->aegain_buff, &bnr_cfg->coeff_b, sizeof(float));
	memcpy(&m->blend_buff, &bnr_cfg->blend, sizeof(float));
	memcpy(&m->black_level_buff, &bnr_cfg->blc, sizeof(float));

	ISP_LOG_ERR("float bnr param: %.5f, %.5f, %.5f, %.5f\n",
		m->coeff_a_buff, m->aegain_buff,
		m->blend_buff, m->black_level_buff);

	UNUSED(is_new);

	bnr_ctx[ViPipe]->cmodel0 = m;

	return CVI_SUCCESS;
}

CVI_S32 teaisp_bnr_get_model_type_wrap(VI_PIPE ViPipe, void *model_type)
{
	if (model_type == NULL) {
		return CVI_SUCCESS;
	}

	TEAISP_MODEL_TYPE_E *m_type = (TEAISP_MODEL_TYPE_E *) model_type;

	*m_type = TEAISP_MODEL_NONE;

	if (bnr_ctx[ViPipe] && bnr_ctx[ViPipe]->enable_lauch_thread)
		*m_type = bnr_ctx[ViPipe]->enModelType;

	return CVI_SUCCESS;
}
