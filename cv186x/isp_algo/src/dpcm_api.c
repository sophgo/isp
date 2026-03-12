/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: dpcm_api.c
 * Description:
 *
 */

#include "stdio.h"
#include "stdlib.h"
#include "dpcm_api.h"

#include "isp_algo_debug.h"
#include "isp_algo_utility.h"

#define COMPRESSMODE 7//0 bypass, 1~3 reserved, 4 12-10-12 codec, 5 12-8-12 codec, 6 12-7-12 codec, 7 12-6-12 codec
#define PREDICTOR_BYTES 8//12-6-12 spec is 8 bytes
#define TILE_MODE_WIDTH 2304//know it by hw cv183x:2688 cv182x:2304
#define TITLE_START_IDX 1536
#define DEBUG_COMPRESS_RAW 0
#define alignment_up(x, size) ({((x)+(size)-1) & (~((size)-1)); })

static inline bool get_sign(CVI_S16 a)  {return a <= 0 ? 1 : 0; }
static inline CVI_U16  fix_zero(CVI_U16 a)  {return a == 0 ? 1 : a; }
static inline CVI_U16  deco_dpcm(CVI_U16 xp, bool s, CVI_U16 v) {return s ? MAX(xp-v, 0) : MIN(xp+v, 4095); }
static inline CVI_U16  decp_pcm(CVI_U16 xp, CVI_U16 ad, CVI_U16 v) {return (v > xp) ? v+ad : v+ad+1; }

static CVI_U16 decode_12_10_12(bool nop, CVI_U16 xenco, CVI_U16 xpred)
{
	CVI_U16 xdeco;

	if (nop)
		xdeco = (xenco << 2) + 2;
	else if ((xenco & 0x300) == 0x000)
		xdeco = deco_dpcm(xpred, xenco & 0x80, xenco & 0x7f);
	else if ((xenco & 0x380) == 0x100)
		xdeco = deco_dpcm(xpred, xenco & 0x40, ((xenco & 0x3f)<<1) + 128);
	else if ((xenco & 0x380) == 0x180)
		xdeco = deco_dpcm(xpred, xenco & 0x40, ((xenco & 0x3f)<<2) + 257);
	else
		xdeco = decp_pcm(xpred, 3, (xenco & 0x1ff)<<3);
	return xdeco;
}

static CVI_U16 decode_12_8_12(bool nop, CVI_U16 xenco, CVI_U16 xpred)
{
	CVI_U16 xdeco;

	if (nop)
		xdeco = (xenco << 4) + 8;
	else if ((xenco & 0xf0) == 0x00)
		xdeco = deco_dpcm(xpred, xenco & 0x8, xenco & 0x7);
	else if ((xenco & 0xe0) == 0x60)
		xdeco = deco_dpcm(xpred, xenco & 0x10, ((xenco & 0xf)<<1) + 8);
	else if ((xenco & 0xe0) == 0x40)
		xdeco = deco_dpcm(xpred, xenco & 0x10, ((xenco & 0xf)<<2) + 41);
	else if ((xenco & 0xe0) == 0x20)
		xdeco = deco_dpcm(xpred, xenco & 0x10, ((xenco & 0xf)<<3) + 107);
	else if ((xenco & 0xf0) == 0x10)
		xdeco = deco_dpcm(xpred, xenco & 0x8, ((xenco & 0x7)<<4) + 239);
	else
		xdeco = decp_pcm(xpred, 15, (xenco & 0x7f)<<5);
	return xdeco;
}

static CVI_U16 decode_12_7_12(bool nop, CVI_U16 xenco, CVI_U16 xpred)
{
	CVI_U16 xdeco;

	if (nop)
		xdeco = (xenco << 5) + 16;
	else if ((xenco & 0x78) == 0x00)
		xdeco = deco_dpcm(xpred, xenco & 0x4,   xenco & 0x3);
	else if ((xenco & 0x78) == 0x08)
		xdeco = deco_dpcm(xpred, xenco & 0x4, ((xenco & 0x3)<<1) + 4);
	else if ((xenco & 0x78) == 0x10)
		xdeco = deco_dpcm(xpred, xenco & 0x4, ((xenco & 0x3)<<2) + 13);
	else if ((xenco & 0x70) == 0x20)
		xdeco = deco_dpcm(xpred, xenco & 0x8, ((xenco & 0x7)<<3) + 31);
	else if ((xenco & 0x70) == 0x30)
		xdeco = deco_dpcm(xpred, xenco & 0x8, ((xenco & 0x7)<<4) + 99);
	else if ((xenco & 0x78) == 0x18)
		xdeco = deco_dpcm(xpred, xenco & 0x4, ((xenco & 0x3)<<5) + 235);
	else
		xdeco = decp_pcm(xpred, 31, (xenco & 0x3f)<<6);
	return xdeco;
}

static CVI_U16 decode_12_6_12(bool nop, CVI_U16 xenco, CVI_U16 xpred)
{
	CVI_U16 xdeco;

	if (nop)
		xdeco = (xenco << 6) + 32;
	else if ((xenco & 0x3c) == 0x00)
		xdeco = deco_dpcm(xpred, xenco & 0x2, xenco & 0x1);
	else if ((xenco & 0x3c) == 0x04)
		xdeco = deco_dpcm(xpred, xenco & 0x2, ((xenco & 0x1)<<2) +   3);
	else if ((xenco & 0x38) == 0x10)
		xdeco = deco_dpcm(xpred, xenco & 0x4, ((xenco & 0x3)<<3) +  13);
	else if ((xenco & 0x3c) == 0x08)
		xdeco = deco_dpcm(xpred, xenco & 0x2, ((xenco & 0x1)<<4) +  49);
	else if ((xenco & 0x38) == 0x18)
		xdeco = deco_dpcm(xpred, xenco & 0x4, ((xenco & 0x3)<<5) +  89);
	else if ((xenco & 0x3c) == 0x0c)
		xdeco = deco_dpcm(xpred, xenco & 0x2, ((xenco & 0x1)<<6) + 233);
	else
		xdeco = decp_pcm(xpred, 63, (xenco & 0x1f)<<7);
	return xdeco;
}

static CVI_U16 predictor1(CVI_U16 x, CVI_U16 *pb, bool *nop)
{
	*nop = (x <= 1);
	return  (*nop) ? 0 : pb[1];
}

static CVI_U16 predictor2(CVI_U16 x, CVI_U16 *pb, bool *nop)
{
	CVI_U16 xpred = 0;
	*nop = (x == 0);

	if (x == 1)
		xpred = pb[0]; // use previous decoded different color value
	else if (x == 2)
		xpred = pb[1]; // use previous coded same color value
	else if (x == 3)
		xpred = ((pb[0] <= pb[1] && pb[1] <= pb[2]) || (pb[0] >= pb[1] && pb[1] >= pb[2])) ? pb[0] : pb[1];
	else if (x >= 4)
		xpred = ((pb[0] <= pb[1] && pb[1] <= pb[2]) || (pb[0] >= pb[1] && pb[1] >= pb[2])) ? pb[0] :
				((pb[0] <= pb[2] && pb[1] <= pb[3]) || (pb[0] >= pb[2] && pb[1] >= pb[3])) ? pb[1] :
				((pb[1] + pb[3] + 1) >> 1);
	return (*nop) ? 0 : xpred;
}

//In compressMode,Each pixel is stored with 6 bits, and it is out of order
//We need to rearrange it and store it with 2 bytes, so that DPCM can decode properly
static void repack_bit6_to_bit16(uint8_t *pu8In6bit, uint8_t *pu8Out16bit,
		uint32_t u32Width, uint32_t u32Height, uint32_t u32InStride, uint32_t u32OutStride)
{
	uint8_t *pu8In6bitAddr = pu8In6bit;
	uint8_t *pu8Out16bitAddr = pu8Out16bit;
	uint16_t *pu16Out16bitAddr;
	uint8_t buffer[3];

	for (uint32_t i = 0; i < u32Height; ++i, pu8In6bitAddr += u32InStride, pu8Out16bitAddr += u32OutStride) {
		pu16Out16bitAddr = (uint16_t *)pu8Out16bitAddr;
		for (uint32_t j16 = 0, j6 = 0; j16 < u32Width; j16 += 4, j6 += 3) {
			buffer[0] = pu8In6bitAddr[j6];
			buffer[1] = pu8In6bitAddr[j6 + 1];
			buffer[2] = pu8In6bitAddr[j6 + 2];
			pu16Out16bitAddr[j16 + 0] = ((buffer[0] & 0x03) << 4) | (buffer[2] & 0x0F);
			pu16Out16bitAddr[j16 + 1] = ((buffer[0] & 0xFC) >> 2);
			pu16Out16bitAddr[j16 + 2] = ((buffer[2] & 0xF0) >> 4) | ((buffer[1] & 0x03) << 4);
			pu16Out16bitAddr[j16 + 3] = ((buffer[1] & 0xFC) >> 2);
		}
	}
}

typedef struct {
	CVI_U32 startPos;//From which point is predictor data
	CVI_U32 size;//unit is pixel
} PREDICTOR_INFO;

//In tileMode, Each line has 8bytes of prediction data, which needs to be removed
static void deletePredictorData(uint16_t *inBuf, uint16_t *outBuf, uint32_t u32Width, uint32_t u32Height,
					PREDICTOR_INFO predictorInfo, uint32_t u32InStride, uint32_t u32OutStride)
{
	for (uint32_t i = 0; i < u32Height; ++i) {
		for (uint32_t j = 0, k = 0; j < u32Width; ++j, ++k) {
			if (j == (predictorInfo.startPos)) {
				k += predictorInfo.size;
			}
			outBuf[i*u32OutStride + j] = inBuf[i*u32InStride + k];
		}
	}
}

//In compressMode, The data is encoded by DPCM, so we need DPCM decoding to get the original data
static void dpcm_rx(const CVI_U16 *idata, CVI_U16 *odata, CVI_U32 width, CVI_U32 height, CVI_U8 decMode)
{
	CVI_U16 xpred, xdeco, pix_buf[4];
	bool nop;

	CVI_U16 (*predictor)(CVI_U16, CVI_U16 *, bool *) = NULL;
	CVI_U16 (*decoder)(bool, CVI_U16, CVI_U16) = NULL;

	switch (decMode) {
	case 4:
		decoder = decode_12_10_12;
		predictor = predictor1;
		break;
	case 5:
		decoder = decode_12_8_12;
		predictor = predictor1;
		break;
	case 6:
		decoder = decode_12_7_12;
		predictor = predictor2;
		break;
	case 7:
		decoder = decode_12_6_12;
		predictor = predictor2;
		break;
	}

	for (CVI_U32 y = 0; y < height; y++) {
		for (CVI_U32 x = 0; x < width; x++) {
			CVI_U32 idx = y * width + x;

			if (decMode == 0) // bypass
				odata[idx] = idata[idx];
			else {
				xpred = predictor(x, pix_buf, &nop);
				xdeco = decoder(nop, idata[idx], xpred);
				odata[idx] = xdeco;

				// buffer shift
				pix_buf[3] = pix_buf[2];
				pix_buf[2] = pix_buf[1];
				pix_buf[1] = pix_buf[0];
				pix_buf[0] = xdeco;
			}
		}
	}
}

CVI_S32 decoderRaw(RAW_INFO rawInfo, CVI_U8 *outPut)
{
	CVI_U32 width = rawInfo.width;
	CVI_U32 height = rawInfo.height;
	CVI_U32 stride = rawInfo.stride;
	CVI_U8 *pUnPackImg = malloc(width * height * 2);

	if ((rawInfo.buffer == NULL) || (rawInfo.width == 0) ||
		(rawInfo.height == 0)) {
		ISP_ALGO_LOG_ERR("input rawInfo is err, please check\n");
		free(pUnPackImg);
		return CVI_FAILURE;
	}

	if (pUnPackImg == NULL) {
		ISP_ALGO_LOG_ERR("malloc size:%u fail\n", width * height * 2);
		return CVI_FAILURE;
	}

	if (width > TILE_MODE_WIDTH) {
		//tileMode
		PREDICTOR_INFO predictorInfo = {};

		predictorInfo.startPos = TITLE_START_IDX;
		predictorInfo.size = PREDICTOR_BYTES;
		CVI_U8 *pOriImg = malloc((width + predictorInfo.size) * height * 2);

		if (pOriImg == NULL) {
			ISP_ALGO_LOG_ERR("malloc size:%u fail\n",
				(width + predictorInfo.size) * height * 2);
			free(pUnPackImg);
			return CVI_FAILURE;
		}

		repack_bit6_to_bit16(rawInfo.buffer, pOriImg, (width + predictorInfo.size), height,
			stride, (width + predictorInfo.size) * 2);
		deletePredictorData((uint16_t *)pOriImg, (uint16_t *)pUnPackImg, width, height,
			predictorInfo, (width + predictorInfo.size), width);
		free(pOriImg); pOriImg = NULL;
	} else {
		repack_bit6_to_bit16(rawInfo.buffer, pUnPackImg, width, height, stride, width * 2);
	}
	dpcm_rx((CVI_U16 *)pUnPackImg, (CVI_U16 *)outPut, width, height, COMPRESSMODE);
	free(pUnPackImg); pUnPackImg = NULL;
#if DEBUG_COMPRESS_RAW
	FILE *fd = fopen("ori.raw", "w");

	fwrite((uint8_t *)rawInfo.buffer, image_size, 1, fd);
	fclose(fd);
	fd = fopen("uncompress.raw", "w");
	fwrite((uint8_t *)outPut, width * height * 2, 1, fd);
	fclose(fd);
#endif
	return CVI_SUCCESS;
}

#ifdef SIMULATION

int crop_image(uint32_t ofs_x, uint32_t ofs_y, uint32_t stride, uint32_t width, uint32_t height,
		uint8_t *src, uint8_t *dst)
{
	stride *= 2;
	uint8_t *addr_src = src + ofs_y * stride + ofs_x * 2;
	uint8_t *addr_dst = dst;

	for (uint32_t i = 0 ; i < height ; i++) {
		memcpy(addr_dst, addr_src, width * 2);
		addr_src += stride;
		addr_dst += width * 2;
	}
}

void main(int argc, const char *argv[])
{
	FILE *pFile = NULL;
	uint32_t oriWidth, oriHeight, cropWidth, cropHeight, stride, fileSize, ofs_x, ofs_y;
	uint8_t outFileName[200];

	if (argc != 6 && argc != 4) {
		printf("Invalid param, Please input the following param, cropParam is not necessary\n");
		printf("./decoderRaw oriWidth oriHeight [cropWidth] [cropHeight] fileName\n");
		return;
	} else if (argc == 4) {
		oriWidth = atoi(argv[1]);
		oriHeight = atoi(argv[2]);
		cropWidth = oriWidth;
		cropHeight = oriHeight;
		pFile = fopen(argv[3], "rb");
		if (pFile == NULL) {
			printf("%d file %s open fail\n", __LINE__, argv[3]);
			return;
		}
		snprintf(outFileName, sizeof(outFileName), "Dec[%d-%d]%s", cropWidth, cropHeight, argv[3]);
	} else if (argc == 6) {
		oriWidth = atoi(argv[1]);
		oriHeight = atoi(argv[2]);
		cropWidth = atoi(argv[3]);
		cropHeight = atoi(argv[4]);
		pFile = fopen(argv[5], "rb");
		if (pFile == NULL) {
			printf("%d file %s open fail\n", __LINE__, argv[5]);
			return;
		}
		snprintf(outFileName, sizeof(outFileName), "Dec[%d-%d]%s", cropWidth, cropHeight, argv[5]);
	}

	//get file size
	fseek(pFile, 0, SEEK_END);
	fileSize = ftell(pFile);
	fseek(pFile, 0, SEEK_SET);
	//get stride
	stride = fileSize/oriHeight;
	//read file
	uint8_t *inBuff = malloc(fileSize);

	if (inBuff == NULL) {
		printf("malloc %d fail\n", fileSize);
		return;
	}
	fread(inBuff, fileSize, 1, pFile);
	fclose(pFile);
	//decodeRaw
	RAW_INFO rawInfo;

	rawInfo.buffer = inBuff;
	rawInfo.width = oriWidth;
	rawInfo.height = oriHeight;
	rawInfo.stride = stride;

	uint8_t *pDecImg = malloc(oriWidth * oriHeight * 2);

	if (pDecImg == NULL) {
		free(inBuff);
		printf("malloc %d fail\n", oriWidth * oriHeight * 2);
		return;
	}

	decoderRaw(rawInfo, pDecImg);
	//crop
	if (oriWidth != cropWidth || oriHeight != cropHeight) {
		uint8_t *pCropImg = malloc(cropWidth * cropHeight * 2);

		if (pCropImg == NULL) {
			free(inBuff);
			free(pDecImg);
			printf("malloc %d fail\n", cropWidth * cropHeight * 2);
			return;
		}
		ofs_x = (oriWidth - cropWidth) >> 1;
		ofs_y = (oriHeight - cropHeight) >> 1;
		crop_image(ofs_x, ofs_y, oriWidth, cropWidth, cropHeight, pDecImg, pCropImg);
		pFile = fopen(outFileName, "wb");
		if (pFile == NULL) {
			printf("%d file %s open fail\n", __LINE__, outFileName);
			return;
		}
		fwrite(pCropImg, cropWidth * cropHeight * 2, 1, pFile);
		fclose(pFile);
		free(inBuff);
		free(pDecImg);
		free(pCropImg);
	} else {
		pFile = fopen(outFileName, "wb");
		if (pFile == NULL) {
			printf("%d file %s open fail\n", __LINE__, outFileName);
			return;
		}
		fwrite(pDecImg, oriWidth * oriHeight * 2, 1, pFile);
		fclose(pFile);
		free(inBuff);
		free(pDecImg);
	}
}

#endif
