/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_dpc.c
 * Description:
 *
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <memory.h>
#include <assert.h>

#include "isp_algo_dpc.h"
#include "isp_algo_debug.h"

//-----------------------------------------------------------------------------
//  define
//-----------------------------------------------------------------------------
#define ISP_HOT_DEV_THRESH (20)
#define ISP_DEAD_DEV_THRESH (15)

#if SIMULATION
#define VERIFY_TRAVERSAL 0
#define TABLE_SIZE 4096
#define VERBOSE 0
// #define DEBUG   ((100 <<16) + 100)	//row=100, column=100
#define DEBUG ((0 << 16) + 2) //row=0, column=2
// #define DEBUG   0
#define TEST_DPC_DARK 0
#endif

#define LIMIT_RANGE(v, min, max)                                                                                       \
	({                                                                                                             \
		typeof(v) _v = (v);                                                                                    \
		typeof(min) _min = (min);                                                                              \
		typeof(max) _max = (max);                                                                              \
		_v = (_v < _min) ? _min : _v;                                                                          \
		_v = (_v > _max) ? _max : _v;                                                                          \
	})

#ifdef DPC_DEBUG
static uint32_t g_Verbose;
static uint32_t g_Debug;
#endif // DPC_DEBUG

//-----------------------------------------------------------------------------
//  basic functions
//-----------------------------------------------------------------------------
CVI_S32 isp_algo_dpc_main(struct dpc_param_in *dpc_param_in, struct dpc_param_out *dpc_param_out)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(dpc_param_in);
	UNUSED(dpc_param_out);

	return ret;
}

CVI_S32 isp_algo_dpc_init(void)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	return ret;
}

CVI_S32 isp_algo_dpc_uninit(void)
{
	// ISP_DEBUG(LOG_INFO, "%s\n", "+");
	CVI_S32 ret = CVI_SUCCESS;

	return ret;
}

#ifdef DPC_DEBUG
static inline void print_array(uint16_t *data, uint32_t data_size)
{
	for (uint32_t i = 0; i < data_size; i++)
		printf("%u,\t", data[i]);
	printf("\n");
}

static inline void print_matrix(uint16_t *data, uint16_t width, uint16_t height, uint16_t stride)
{
	for (uint16_t i = 0; i < height; i++) {
		printf("[");
		for (uint16_t j = 0; j < width; j++) {
			printf("%u, ", data[i * stride + j]);
		}
		printf("]\n");
	}
}

static inline void print_pixel(uint16_t *image, uint16_t width, uint16_t height, uint16_t stride, uint16_t r,
			       uint16_t c)
{
	uint16_t _r = LIMIT_RANGE(r, 2, height - 2 - 1);
	uint16_t _c = LIMIT_RANGE(c, 2, width - 2 - 1);

	for (uint16_t i = _r - 2; i <= _r + 2; i++) {
		printf("\t");
		for (uint16_t j = _c - 2; j <= _c + 2; j++) {
			if (i == r && j == c) {
				printf("+%u, ", image[i * stride + j]);
			} else {
				printf("%u, ", image[i * stride + j]);
			}
		}
		printf("\n");
	}
}
#endif // DPC_DEBUG

static inline void sort2(uint16_t *a, uint16_t *b)
{
	if (*a > *b) {
		uint16_t temp = *a;
		*a = *b;
		*b = temp;
	}
}

static inline void sort3(uint16_t *data)
{
	sort2(data, data + 2);
	sort2(data, data + 1);
	sort2(data + 1, data + 2);
}

static inline void merge(uint16_t *data, uint16_t *data1, uint16_t size1, uint16_t *data2, uint16_t size2)
{
	uint16_t i = 0, i1 = 0, i2 = 0;

	while (i1 < size1 && i2 < size2) {
		if (data1[i1] < data2[i2]) {
			data[i] = data1[i1];
			i++;
			i1++;
		} else {
			data[i] = data2[i2];
			i++;
			i2++;
		}
	}
	while (i1 < size1) {
		data[i] = data1[i1];
		i++;
		i1++;
	}
	while (i2 < size2) {
		data[i] = data2[i2];
		i++;
		i2++;
	}
}

// only accept array size=3, 12bit domain
// @see https://reader.elsevier.com/reader/sd/pii/S1319157820303402
static inline uint16_t mid_value_decision_median(uint16_t *data)
{
	// sort data
	sort2(data, data + 2);
	sort2(data, data + 1);
	sort2(data + 1, data + 2);

	return (data[1] == 4095) ? data[0] : ((data[1] == 0) ? data[2] : data[1]);
}

static inline uint16_t median3(uint16_t *data)
{
	// sort data
	sort2(data, data + 2);
	sort2(data, data + 1);
	sort2(data + 1, data + 2);

	return data[1];
}

static inline uint16_t median_filter_IAMFA1_next(uint16_t *mid, uint16_t index, uint16_t *C)
{
	mid[index] = mid_value_decision_median(C);
	return mid_value_decision_median(mid);
}

static inline uint16_t median_filter_IAMFA1(uint16_t *col0, uint16_t *col1, uint16_t *col2)
{
	uint16_t mid[3];

	mid[0] = mid_value_decision_median(col0);
	mid[1] = mid_value_decision_median(col1);
	mid[2] = mid_value_decision_median(col2);
	return mid_value_decision_median(mid);
}

static inline uint16_t median_filter_DP(uint16_t *col0, uint16_t *col1, uint16_t *col2)
{
	uint16_t mid[3];

	mid[0] = median3(col0);
	mid[1] = median3(col1);
	mid[2] = median3(col2);
	return median3(mid);
}

static inline uint16_t median_filter(uint16_t *col0, uint16_t *col1, uint16_t *col2)
{
	uint16_t temp1[6];
	uint16_t temp2[9];

	sort3(col0);
	sort3(col1);
	sort3(col2);
	merge(temp1, col0, 3, col1, 3);
	merge(temp2, temp1, 6, col2, 3);
	return temp2[4];
}

static inline float average(uint16_t *data, uint32_t data_size)
{
	float sum = 0;

	for (uint32_t i = 0; i < data_size; i++) {
		sum += data[i];
	}
	return sum / data_size;
}

static inline float stddev(uint16_t *data, uint32_t data_size, float average)
{
	float error = 0;

	for (uint32_t i = 0; i < data_size; i++) {
		float diff = data[i] - average;

		error += diff * diff;
	}

	return sqrtf(error / (data_size - 1));
}

static inline uint32_t order_bright(uint16_t *data, uint32_t data_size, uint16_t value)
{
	// for bright pixel search, if value = 1, order should be 1
	//value 0 1 1 1 1 1 1 1 2
	//order 0 1 2 3 4 5 6 7 8
	//        ^           ^
	//      not bright
	//                      bright
	uint32_t o = 0;

	for (uint32_t i = 0; i < data_size; i++) {
		if (value > data[i]) { //notice '>' rather than '>='
			o++;
		}
	}
	return o;
}

static inline uint32_t order_dark(uint16_t *data, uint32_t data_size, uint16_t value)
{
	uint32_t o = 0;

	for (int32_t i = data_size - 1; i >= 0; i--) {
		if (value < data[i]) { //notice '<' rather than '<='
			o++;
		}
	}
	return o;
}

//-----------------------------------------------------------------------------
//  dpc functions
//-----------------------------------------------------------------------------
static bool dpc_detect_bright(uint16_t *data, uint32_t data_size, uint16_t abs_thresh, uint16_t v)
{
	bool result = true;

	// save center value, or median_filter() will reorder data
	// uint16_t v = data[4];

	// early break
	uint32_t order = order_bright(data, data_size, v);
	// if (order < 4) {    //0.6s
	if (order < 6) { //0.48s
		//return false;
		result = false;

#ifdef DPC_DEBUG
		if (g_Verbose) {
			printf("v=%u, order=%u\n", v, order);
		}
#endif // DPC_DEBUG
		goto RESULT;
	}

	// test by median filter
	uint16_t *col0 = data;
	uint16_t *col1 = data + 3;
	uint16_t *col2 = data + 6;
	// 0.14s, 209px
	// uint16_t median = median_filter_IAMFA1(col0, col1, col2);
	// 0.04s, 209px
	// uint16_t median = median_filter_DP(col0, col1, col2);
	// 0.10s, 211px
	uint16_t median = median_filter(col0, col1, col2);

	uint16_t threshold_abs;
	uint16_t threshold_dev;

	threshold_abs = median + abs_thresh;
	result = (v <= threshold_abs) ? false : result;

	// float threshold_dev = 1.2 * median - 0.5;
	threshold_dev = ((128 + (((ISP_HOT_DEV_THRESH << 7) + 50) / 100)) * median) >> 7;
	result = (v < threshold_dev) ? false : result;

#ifdef DPC_DEBUG
	if (g_Verbose) {
		printf("v=%u, thresh_abs=%u, thresh_dev=%u, median=%u\n", v, threshold_abs, threshold_dev, median);
	}
#endif // DPC_DEBUG

RESULT:
	return result;
}

static bool dpc_detect_dark(uint16_t *data, uint32_t data_size, uint16_t abs_thresh, uint16_t v)
{
	bool result = true;

	// save center value, or median_filter() will reorder data
	// uint16_t v = data[4];

	// early break
	// if(g_Verbose) print_matrix(data, 3, 3, 3);
	uint32_t order = order_dark(data, data_size, v);
	// printf("order=%d\n", order);
	// if (order < 4) {    //0.6s
	if (order < 6) { //0.48s
		//return false;
		result = false;
		goto RESULT;
	}

	// test by median filter
	uint16_t *col0 = data;
	uint16_t *col1 = data + 3;
	uint16_t *col2 = data + 6;
	// 0.14s, 209px
	// uint16_t median = median_filter_IAMFA1(col0, col1, col2);
	// 0.04s, 209px
	// uint16_t median = median_filter_DP(col0, col1, col2);
	// 0.10s, 211px
	uint16_t median = median_filter(col0, col1, col2);

	uint16_t threshold_abs;
	uint16_t threshold_dev;

	threshold_abs = median - abs_thresh;
	result = (v >= threshold_abs) ? false : result;

	// float threshold_dev = 1.2 * median - 0.5;
	threshold_dev = ((128 - (((ISP_DEAD_DEV_THRESH << 7) + 50) / 100)) * median) >> 7;
	result = (v > threshold_dev) ? false : result;

RESULT:
#ifdef DPC_DEBUG
	if (g_Verbose && result) {
		printf("v=%u, thresh_abs=%u, thresh_dev=%u, median=%u\n", v, threshold_abs, threshold_dev, median);
	}
#endif // DPC_DEBUG

	return result;
}

static inline bool dpc_detect(uint16_t *p, uint16_t stride, uint16_t abs_thresh, uint8_t position, uint8_t dpType)
{
	// return true;

	uint16_t data[9];
	// col0 = data + 0
	// col1 = data + 3
	// col2 = data + 6
	data[0] = p[0];
	data[3] = p[2];
	data[6] = p[4];
	data[1] = p[(stride << 1)];
	data[4] = p[(stride << 1) + 2]; //center
	data[7] = p[(stride << 1) + 4];
	data[2] = p[(stride << 2)];
	data[5] = p[(stride << 2) + 2];
	data[8] = p[(stride << 2) + 4];

	// g_Verbose = data[position] < 185;

	// center
	// 0 3 6
	// 1 4 7
	// 2 5 8
	bool result = (dpType == 0) ? dpc_detect_bright(data, 9, abs_thresh, data[position]) :
					    dpc_detect_dark(data, 9, abs_thresh, data[position]);
	return result;
}

static inline void dpc_table_add(uint32_t *table, uint32_t *count, uint32_t table_max_size, uint16_t r, uint16_t c)
{
	if (*count < table_max_size - 1) {
		table[*count] = (r << 16) + c;
		(*count)++;
	} else {
		ISP_ALGO_LOG_WARNING("table over size %d at [%u,%u]\n", table_max_size, r, c);
	}
}

int32_t Bayer_12bit_2_16bit(uint8_t *bayerBuffer, uint16_t *outBuffer,
					uint16_t width, uint16_t height, uint16_t stride)
{
	uint32_t r, c;
	uint32_t au16Temp[2] = {};

	if (bayerBuffer == NULL || outBuffer == NULL) {
		ISP_ALGO_LOG_WARNING("pointer is NULL\n");
		return 1;
	}

	uint16_t *p = outBuffer;

	for (r = 0; r < height; r++) {
		for (c = 0; c < width; c += 2) {
			au16Temp[0] = (*bayerBuffer << 4) + ((*(bayerBuffer + 2) & 0xF0) >> 4);
			au16Temp[1] = (*(bayerBuffer + 1) << 4) + (*(bayerBuffer + 2) & 0xF);
			*p = au16Temp[0];
			*(p+1) = au16Temp[1];
			bayerBuffer += 3;
			p += 2;
		}
		bayerBuffer += (uint16_t)((stride - width)*1.5);
	}
	return 0;
}

uint16_t DPC_Calib(const DPC_Input *input, DPC_Output *output)
{
	printf("%s+\n", __func__);
	// input
	uint16_t *image = input->image;
	uint16_t width = input->width;
	uint16_t height = input->height;
	uint16_t stride = input->stride;
	uint16_t abs_thresh = input->abs_thresh;
	uint8_t dpType = input->dpType;
	uint32_t table_max_size = input->table_max_size;

#ifdef DPC_DEBUG
	g_Verbose = input->verbose;
	g_Debug = input->debug;
#endif // DPC_DEBUG

	//output
	uint32_t *table = output->table;
	uint32_t count = 0;

#if VERIFY_TRAVERSAL
	bool verifyTable[2160][3840] = {};
#endif

	uint16_t left = 3, right = width - 3 - 1, upper = 3, bottom = height - 3 - 1;

	for (uint16_t r = 0; r < height; r++) {
		uint16_t *p;

		if (r <= upper) {
			p = image + (r - 0) * stride;
		} else if (r < bottom) {
			p = image + (r - 2) * stride;
		} else {
			p = image + (r - 4) * stride;
		}

		for (uint16_t c = 2; c < width - 2; c++) {
			// g_Verbose = isHisiDp(r, c, 0);

			bool result;

#ifdef DPC_DEBUG
			if (g_Debug) {
				if (r == (g_Debug >> 16) && c == (g_Debug & 0xFFFF)) {
					printf("debug:\n");
					print_pixel(image, width, height, stride, r, c);
					g_Verbose = 1;
				} else {
					g_Verbose = input->verbose;
				}
			}
			if (g_Verbose) {
				printf("[%u, %u]: ", r, c);
			}
#endif // DPC_DEBUG
			//position
			//0 3 6
			//1 4 7
			//2 5 8
			if (r <= upper) {
				//0
				if (c <= left) {
					result = dpc_detect(p, stride, abs_thresh, 0, dpType);
					if (result)
						dpc_table_add(table, &count, table_max_size, r, c - 2);
#if VERIFY_TRAVERSAL
					verifyTable[r][c - 2] = true;
#endif
				}

				//3
				result = dpc_detect(p, stride, abs_thresh, 3, dpType);
				if (result)
					dpc_table_add(table, &count, table_max_size, r, c);
#if VERIFY_TRAVERSAL
				verifyTable[r][c] = true;
#endif

				//6
				if (c >= right) {
					result = dpc_detect(p, stride, abs_thresh, 6, dpType);
					if (result)
						dpc_table_add(table, &count, table_max_size, r, c + 2);
#if VERIFY_TRAVERSAL
					verifyTable[r][c + 2] = true;
#endif
				}
			} else if (r < bottom) {
				//1
				if (c <= left) {
					result = dpc_detect(p, stride, abs_thresh, 1, dpType);
					if (result)
						dpc_table_add(table, &count, table_max_size, r, c - 2);
#if VERIFY_TRAVERSAL
					verifyTable[r][c - 2] = true;
#endif
				}

				//4
				result = dpc_detect(p, stride, abs_thresh, 4, dpType);
				if (result)
					dpc_table_add(table, &count, table_max_size, r, c);
#if VERIFY_TRAVERSAL
				verifyTable[r][c] = true;
#endif

				//7
				if (c >= right) {
					result = dpc_detect(p, stride, abs_thresh, 7, dpType);
					if (result)
						dpc_table_add(table, &count, table_max_size, r, c + 2);
#if VERIFY_TRAVERSAL
					verifyTable[r][c + 2] = true;
#endif
				}
			} else {
				//2
				if (c <= left) {
					result = dpc_detect(p, stride, abs_thresh, 2, dpType);
					if (result)
						dpc_table_add(table, &count, table_max_size, r, c - 2);
#if VERIFY_TRAVERSAL
					verifyTable[r][c - 2] = true;
#endif
				}

				//5
				result = dpc_detect(p, stride, abs_thresh, 5, dpType);
				if (result)
					dpc_table_add(table, &count, table_max_size, r, c);
#if VERIFY_TRAVERSAL
				verifyTable[r][c] = true;
#endif

				//8
				if (c >= right) {
					result = dpc_detect(p, stride, abs_thresh, 8, dpType);
					if (result)
						dpc_table_add(table, &count, table_max_size, r, c + 2);
					count += result;
#if VERIFY_TRAVERSAL
					verifyTable[r][c + 2] = true;
#endif
				}
			}

			p++;
		}
	}

#if VERIFY_TRAVERSAL
	//verify
	uint32_t verify = 0;

	for (uint16_t r = 0; r < height; r++) {
		for (uint16_t c = 0; c < width; c++) {
			verify += verifyTable[r][c];
		}
	}
	printf("verify=%u, npixel=%u\n", verify, width * height);
#endif

	printf("%s-\n", __func__);
	//output
	output->table_size = count;
	return 0;
}

#ifdef DPC_DEBUG
void DPC_PrintInput(const DPC_Input *input)
{
	printf("DPC_Input:\n");
	printf("image=%p\n", input->image);
	printf("width=%u\n", input->width);
	printf("height=%u\n", input->height);
	printf("stride=%u\n", input->stride);
	printf("abs_thresh=%u\n", input->abs_thresh);
	printf("dpType=%u\n", input->dpType);
	printf("table_max_size=%u\n", input->table_max_size);
	printf("verbose=%u\n", input->verbose);
	printf("debug=%u=[%u, %u]\n", input->debug, input->debug >> 16, input->debug & 0xFFFF);
	printf("\n");
}

void DPC_PrintOutput(const DPC_Input *input, const DPC_Output *output)
{
	printf("DPC_Output:\n");
	for (uint32_t i = 0; i < output->table_size; i++) {
		uint16_t r = output->table[i] >> 16;
		uint16_t c = output->table[i] & 0xFFFF;

		printf("%u: [%u, %u] = %u\n", i, r, c, input->image[r * input->stride + c]);
		print_pixel(input->image, input->width, input->height, input->stride, r, c);
	}
	printf("total=%u\n", output->table_size);
}
#endif // DPC_DEBUG

#if SIMULATION
//-----------------------------------------------------------------------------
//  unit test functions
//-----------------------------------------------------------------------------
// false, a simple case
static void test1(void)
{
	printf("\n# %s:\n", __func__);
	bool expect = true;
	uint16_t data[9] = {
		200, 200, 200, 200, 210, 200, 200, 200, 200,
	};
	uint8_t abs_thresh = 4;
	bool result = dpc_detect_bright(data, 9, abs_thresh, data[4]);

	assert(result == expect);
}

// false, no difference
static void test2(void)
{
	printf("\n# %s:\n", __func__);
	bool expect = false;
	uint16_t data[9] = {
		200, 200, 200, 200, 200, 200, 200, 200, 200,
	};
	uint8_t abs_thresh = 4;
	bool result = dpc_detect_bright(data, 9, abs_thresh, data[4]);

	assert(result == expect);
}

// false, outlier isn't center
static void test3(void)
{
	printf("\n# %s:\n", __func__);
	bool expect = false;
	uint16_t data[9] = {
		200, 200, 210, 200, 200, 200, 200, 200, 200,
	};
	uint8_t abs_thresh = 4;
	bool result = dpc_detect_bright(data, 9, abs_thresh, data[4]);

	assert(result == expect);
}

// execution time
static void test4(void)
{
	printf("\n# %s:\n", __func__);
	// uint32_t times = 8 * 1000 * 1000;
	uint32_t times = 1000 * 1000;
	bool expect = true;
	uint8_t abs_thresh = 4;

	for (uint32_t i = 0; i < times; i++) {
		uint16_t data[9] = {
			200, 200, 200, 200, 210, 200, 200, 200, 200,
		};
		bool result = dpc_detect_bright(data, 9, abs_thresh, data[4]);
		// printf("result=%u, expect=%u\n", result, expect);
		assert(result == expect);
	}
}

/**
 *  test bright pixel and dark pixel
 */
static void test5(void)
{
	printf("\n# %s:\n", __func__);

	uint16_t abs_thresh = 16; //cvi=211, h=94
	char filename[] = "imx334/coutMin0_startThresh4/HisiRAW_3840x2160_12bits_RGGB_Linear_20210317184551.raw_hiiso";
	char type[] = "RGGB";
	const uint16_t width = 3840;
	const uint16_t height = 2160;
	const uint16_t stride = 3840;
	uint16_t source = 0;

	// open file
	ssize_t size = stride * height * sizeof(uint16_t);
	uint16_t *image = malloc(size);

	if (!image) {
		printf("can't alloc image with size = %zu\n", size);
		return;
	}

	FILE *fd = fopen(filename, "rb");

	if (!fd) {
		printf("can't open file %s\n", filename);
		free(image);
		return;
	}

	ssize_t readSize = fread(image, 1, stride * height * sizeof(uint16_t), fd);

	if (readSize != size) {
		printf("readSize=%zu, size=%zu\n", readSize, size);
		fclose(fd);
		free(image);
		goto ERROR;
	}

	// dpc bright pixel
	uint32_t table[TABLE_SIZE] = { 0 };
	DPC_Input input = {
		.image = image,
		// .width = width/2,
		.width = width,
		.height = height,
		.stride = width,
		.abs_thresh = abs_thresh,
		.dpType = 0,
		.table_max_size = TABLE_SIZE,
		.verbose = VERBOSE,
		.debug = DEBUG,
	};

	DPC_Output output = {
		.table = table,
	};
	DPC_Calib(&input, &output);
	DPC_PrintInput(&input);
	DPC_PrintOutput(&input, &output);

#if TEST_DPC_DARK
	// dpc dark pixel
	for (uint16_t r = 0; r < height; r++) {
		uint16_t *p = image + r * stride;

		for (uint16_t c = 0; c < width; c++) {
			*p++ = 4095 - *p;
		}
	}
	DPC_Calib(image, width, height, abs_thresh, 1);
#endif

ERROR:
	// close file
	fclose(fd);
	free(image);
}

void main(int argc, char **argv)
{
	if (argc == 7) {
		uint8_t image_bit = atoi(argv[2]);
		uint16_t width = atoi(argv[3]);
		uint16_t height = atoi(argv[4]);
		uint16_t stride = atoi(argv[5]);
		uint16_t thresh = atoi(argv[6]);
		ssize_t size = stride * height * sizeof(uint16_t);

		uint16_t *image = malloc(size);

		if (!image) {
			printf("can't alloc image with size = %zu\n", size);
			return;
		}

		FILE *fd = fopen(argv[1], "rb");

		if (!fd) {
			printf("can't open file %s\n", argv[1]);
			free(image);
			return;
		}

		if (image_bit == 16) {
			ssize_t readSize = fread(image, 1, stride * height * sizeof(uint16_t), fd);

			if (readSize != size) {
				printf("readSize=%zu, size=%zu\n", readSize, size);
				fclose(fd);
				free(image);
				return;
			}
		} else if (image_bit == 12) {
			uint8_t *tmpImage = malloc(stride * height * 1.5);

			if (!tmpImage) {
				printf("can't alloc tmpImage with size = %zu\n", size);
				fclose(fd);
				free(image);
				return;
			}
			ssize_t readSize = fread(tmpImage, 1, stride * height * 1.5, fd);

			if (readSize != (stride * height * 1.5)) {
				printf("readSize=%zu, size=%f\n", readSize, (stride * height * 1.5));
				fclose(fd);
				free(image);
				free(tmpImage);
				return;
			}
			Bayer_12bit_2_16bit(tmpImage, image,
					width, height, stride);
			free(tmpImage); tmpImage = NULL;
		} else {
			printf("raw_bit only support 12 or 16\n");
			fclose(fd);
			free(image);
			return;
		}
		printf("image buffread done\n");

		uint32_t table[TABLE_SIZE] = { 0 };
		DPC_Input input = {
			.image = image,
			// .width = width/2,
			.width = width,
			.height = height,
			.stride = width,
			.abs_thresh = thresh << 2,
			.dpType = 0,
			.table_max_size = TABLE_SIZE,
			.verbose = VERBOSE,
			.debug = 0,
		};

		DPC_Output output = {
			.table = table,
		};
		DPC_Calib(&input, &output);
		DPC_PrintInput(&input);
		DPC_PrintOutput(&input, &output);
		} else {
			printf("arguments should be[rawfile, raw_bit, width, height, stride, thresh]\n");
		}
}

#endif
