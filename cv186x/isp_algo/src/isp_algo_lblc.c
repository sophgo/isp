/*
 * Copyright (C) Cvitek Co., Ltd. 2023-2024. All rights reserved.
 *
 * File Name: isp_algo_lblc.c
 * Description:
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "isp_algo_lblc.h"
#include "isp_algo_debug.h"
#include "isp_algo_utility.h"

static int lblc_lut_fusion(uint32_t current_iso, uint32_t *iso_tbl, int iso_tbl_size,
	uint16_t **r_in, uint16_t **gr_in, uint16_t **gb_in, uint16_t **b_in,
	uint16_t *r_out, uint16_t *gr_out, uint16_t *gb_out, uint16_t *b_out);

CVI_S32 isp_algo_lblc_main(struct lblc_param_in *lblc_param_in, struct lblc_param_out *lblc_param_out)
{
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(lblc_param_in);
	UNUSED(lblc_param_out);

	ret = lblc_lut_fusion(lblc_param_in->iso, lblc_param_in->iso_tbl, lblc_param_in->iso_tbl_size,
		(uint16_t **) lblc_param_in->lblc_lut_r,
		(uint16_t **) lblc_param_in->lblc_lut_gr,
		(uint16_t **) lblc_param_in->lblc_lut_gb,
		(uint16_t **) lblc_param_in->lblc_lut_b,
		ISP_PTR_CAST_U16(lblc_param_out->lblc_lut_r),
		ISP_PTR_CAST_U16(lblc_param_out->lblc_lut_gr),
		ISP_PTR_CAST_U16(lblc_param_out->lblc_lut_gb),
		ISP_PTR_CAST_U16(lblc_param_out->lblc_lut_b));

	return ret;
}

CVI_S32 isp_algo_lblc_init(void)
{
	CVI_S32 ret = CVI_SUCCESS;

	return ret;
}

CVI_S32 isp_algo_lblc_uninit(void)
{
	CVI_S32 ret = CVI_SUCCESS;

	return ret;
}

static int lblc_lut_fusion(uint32_t current_iso, uint32_t *iso_tbl, int iso_tbl_size,
	uint16_t **r_in, uint16_t **gr_in, uint16_t **gb_in, uint16_t **b_in,
	uint16_t *r_out, uint16_t *gr_out, uint16_t *gb_out, uint16_t *b_out)
{
	int table_0 = 0, table_1 = 0;
	float ratio_0 = 0, ratio_1 = 0;

	// 0. re-order iso from low to high
	uint32_t reorder_iso[ISP_LBLC_ISO_SIZE];
	uint16_t *reorder_r_in[ISP_LBLC_ISO_SIZE];
	uint16_t *reorder_gr_in[ISP_LBLC_ISO_SIZE];
	uint16_t *reorder_gb_in[ISP_LBLC_ISO_SIZE];
	uint16_t *reorder_b_in[ISP_LBLC_ISO_SIZE];

	for (int i = 0; i < iso_tbl_size; i++) {
		reorder_iso[i] = iso_tbl[i];
		reorder_r_in[i] = ISP_PTR_CAST_U16((((ISP_U16_PTR *)r_in)[i]));
		reorder_gr_in[i] = ISP_PTR_CAST_U16((((ISP_U16_PTR *)gr_in)[i]));
		reorder_gb_in[i] = ISP_PTR_CAST_U16((((ISP_U16_PTR *)gb_in)[i]));
		reorder_b_in[i] = ISP_PTR_CAST_U16((((ISP_U16_PTR *)b_in)[i]));
	}

	for (int i = 0; i < iso_tbl_size - 1; i++) {
		for (int j = 0; j < iso_tbl_size - i - 1; j++) {
			if (reorder_iso[j] > reorder_iso[j + 1]) {

				uint32_t tmp = reorder_iso[j + 1];

				reorder_iso[j + 1] = reorder_iso[j];
				reorder_iso[j] = tmp;

				uint16_t *tmp_addr = NULL;

				tmp_addr = reorder_r_in[j + 1];
				reorder_r_in[j + 1] = reorder_r_in[j];
				reorder_r_in[j] = tmp_addr;

				tmp_addr = reorder_gr_in[j + 1];
				reorder_gr_in[j + 1] = reorder_gr_in[j];
				reorder_gr_in[j] = tmp_addr;

				tmp_addr = reorder_gb_in[j + 1];
				reorder_gb_in[j + 1] = reorder_gb_in[j];
				reorder_gb_in[j] = tmp_addr;

				tmp_addr = reorder_b_in[j + 1];
				reorder_b_in[j + 1] = reorder_b_in[j];
				reorder_b_in[j] = tmp_addr;
			}
		}
	}

	// 1. single table case
	if (iso_tbl_size == 1) {

		memcpy(r_out, reorder_r_in[0], ISP_LBLC_GRID_POINTS * sizeof(uint16_t));
		memcpy(gr_out, reorder_gr_in[0], ISP_LBLC_GRID_POINTS * sizeof(uint16_t));
		memcpy(gb_out, reorder_gb_in[0], ISP_LBLC_GRID_POINTS * sizeof(uint16_t));
		memcpy(b_out, reorder_b_in[0], ISP_LBLC_GRID_POINTS * sizeof(uint16_t));

	} else { // 2. multiple table case

		if (current_iso <= reorder_iso[0]) {

			memcpy(r_out, reorder_r_in[0], ISP_LBLC_GRID_POINTS * sizeof(uint16_t));
			memcpy(gr_out, reorder_gr_in[0], ISP_LBLC_GRID_POINTS * sizeof(uint16_t));
			memcpy(gb_out, reorder_gb_in[0], ISP_LBLC_GRID_POINTS * sizeof(uint16_t));
			memcpy(b_out, reorder_b_in[0], ISP_LBLC_GRID_POINTS * sizeof(uint16_t));

		} else if (current_iso >= reorder_iso[iso_tbl_size - 1]) {

			memcpy(r_out, reorder_r_in[iso_tbl_size - 1], ISP_LBLC_GRID_POINTS * sizeof(uint16_t));
			memcpy(gr_out, reorder_gr_in[iso_tbl_size - 1], ISP_LBLC_GRID_POINTS * sizeof(uint16_t));
			memcpy(gb_out, reorder_gb_in[iso_tbl_size - 1], ISP_LBLC_GRID_POINTS * sizeof(uint16_t));
			memcpy(b_out, reorder_b_in[iso_tbl_size - 1], ISP_LBLC_GRID_POINTS * sizeof(uint16_t));

		} else {

			for (int i = 0; i < iso_tbl_size - 1; i++) {
				if (reorder_iso[i] <= current_iso &&
					current_iso < reorder_iso[i + 1]) {
					table_0 = i;
					table_1 = i + 1;
					ratio_1 = (float)(current_iso - reorder_iso[i]) /
						(float)(reorder_iso[i + 1] - reorder_iso[i]);
					ratio_0 = 1.0 - ratio_1;
					break;
				}
			}

			for (int i = 0; i < ISP_LBLC_GRID_POINTS; i++) {

				r_out[i] = reorder_r_in[table_0][i] * ratio_0 +
					reorder_r_in[table_1][i] * ratio_1;

				gr_out[i] = reorder_gr_in[table_0][i] * ratio_0 +
					reorder_gr_in[table_1][i] * ratio_1;

				gb_out[i] = reorder_gb_in[table_0][i] * ratio_0 +
					reorder_gb_in[table_1][i] * ratio_1;

				b_out[i] = reorder_b_in[table_0][i] * ratio_0 +
					reorder_b_in[table_1][i] * ratio_1;
			}

			//for (int i = 0; i < 8; i++) {
			//	printf("r(%d,%d,%d),",
			//	reorder_r_in[table_0][i], r_out[i], reorder_r_in[table_1][i]);
			//	printf("gr(%d,%d,%d),",
			//	reorder_gr_in[table_0][i], gr_out[i], reorder_gr_in[table_1][i]);
			//	printf("gb(%d,%d,%d),",
			//	reorder_gb_in[table_0][i], gb_out[i], reorder_gb_in[table_1][i]);
			//	printf("b(%d,%d,%d)\n",
			//	reorder_b_in[table_0][i], b_out[i], reorder_b_in[table_1][i]);
			//}
		}
	}

	//printf("iso: %d, %d, %d\n", reorder_iso[table_0], current_iso, reorder_iso[table_1]);

	//printf("r_out:");
	//for (int i = 0; i < ISP_LBLC_GRID_POINTS; i++) {
	//	printf("%d ", r_out[i]);
	//}
	//printf("\n");

	//printf("gr_out:");
	//for (int i = 0; i < 16; i++) {
	//	printf("%d ", gr_out[i]);
	//}
	//printf("\n");

	//printf("gb_out:");
	//for (int i = 0; i < 16; i++) {
	//	printf("%d ", gb_out[i]);
	//}
	//printf("\n");

	//printf("b_out:");
	//for (int i = 0; i < 16; i++) {
	//	printf("%d ", b_out[i]);
	//}
	//printf("\n");

	return 0;
}

