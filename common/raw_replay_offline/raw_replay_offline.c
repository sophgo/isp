#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "cvi_buffer.h"
#include "cvi_comm_vb.h"
#include "cvi_comm_video.h"
#include "cvi_comm_isp.h"
#include "cvi_comm_3a.h"
#include "cvi_vi.h"
#include "cvi_sys.h"
#include "cvi_vb.h"
#include "cvi_ae.h"
#include "cvi_isp.h"
#include "cvi_awb.h"

#include "3A_internal.h"
#include "raw_replay.h"
#include "raw_replay_offline.h"

#if defined(__GNUC__) && defined(__riscv)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation" /* Or  "-Wformat-overflow"  */
#endif

#define LOGOUT(fmt, arg...) printf("%s,%d: " fmt, __func__, __LINE__, ##arg)

#define PATH_MAX_LEN 256
#define SEC_TO_USEC 1000000

typedef enum {
	RAW_REPLAY_INIT,
	RAW_REPLAY_RELOAD,
	RAW_REPLAY_UPDATE,
	RAW_REPLAY_SLEEP,
	RAW_REPLAY_STOP,
} RAW_REPLAY_OFFLINE_STATUS;

typedef struct {
	RAW_REPLAY_INFO *pRawHeader;	//pRawHeader[1]: n frame have the same one info (which load from same file)
	CVI_U16 *pData16bit;
	CVI_U8 *pData12bit;
	CVI_U32 size16bit; // CVI_S32 size;
	CVI_U32 size12bit;
	FILE *rawfp;

	VI_PIPE ViPipe;
	VB_POOL PoolID;
	VB_BLK blk;
	CVI_U32 u32BlkSize;
	CVI_U64 u64PhyAddr[2];
	CVI_U8 *pu8VirAddr[2];

	pthread_t rawReplayTid;
	CVI_BOOL bRawReplayThreadEnabled;

	CVI_BOOL isRawReplayReady;
	RAW_REPLAY_OFFLINE_STATUS rawReplayStatus;
	VI_DEV_TIMING_ATTR_S timingAttr;
} RAW_REPLAY_BOARD_CTX_S;

static RAW_REPLAY_BOARD_CTX_S *pRawReplayBoardCtx;

static CVI_S32 load_rawFileName_from_path(char *boardPath, char *rawFileName, int maxFileNameLen);
static CVI_S32 load_raw_info_from_file(char *boardPath, char *rawFileName, RAW_REPLAY_INFO *pRawInfo);
static void *raw_replay_offline_thread(void *arg);

CVI_S32 raw_replay_offline_init(char *boardPath)
{
	char rawFileName[PATH_MAX_LEN];
	char rawFilePath[PATH_MAX_LEN];

	// Calloc and Reset Raw_Replay_Ctx
	pRawReplayBoardCtx = (RAW_REPLAY_BOARD_CTX_S *) calloc(1, sizeof(RAW_REPLAY_BOARD_CTX_S));
	if (pRawReplayBoardCtx == NULL) {
		LOGOUT("pRawReplayBoardCtx calloc fail!!!\n");
		return CVI_FAILURE;
	}
	memset(pRawReplayBoardCtx, 0, sizeof(RAW_REPLAY_BOARD_CTX_S));
	pRawReplayBoardCtx->rawReplayStatus = RAW_REPLAY_INIT;

	// Calloc and Reset Raw Info
	pRawReplayBoardCtx->pRawHeader = (RAW_REPLAY_INFO *) calloc(1, sizeof(RAW_REPLAY_INFO));
	if (pRawReplayBoardCtx->pRawHeader == NULL) {
		LOGOUT("pRawReplayBoardCtx->pRawHeader calloc failed!\n");
		return CVI_FAILURE;
	}
	memset(pRawReplayBoardCtx->pRawHeader, 0, sizeof(RAW_REPLAY_INFO));

	// Load Raw Info from path
	load_rawFileName_from_path(boardPath, rawFileName, PATH_MAX_LEN);
	LOGOUT("Loading Raw FileName:%s\n", rawFileName);
	load_raw_info_from_file(boardPath, rawFileName, pRawReplayBoardCtx->pRawHeader);
	RAW_REPLAY_INFO *pRawInfo = (RAW_REPLAY_INFO *) pRawReplayBoardCtx->pRawHeader;

	LOGOUT("RawFileInfo: %dX%d, numFrame=%d, WDR=%d, bayer=%d, size=%dX%d\n", pRawInfo->width,
					pRawInfo->height, pRawInfo->numFrame, pRawInfo->enWDR,
					pRawInfo->bayerID, pRawReplayBoardCtx->size16bit, pRawInfo->numFrame);

	// Calloc and Reset Raw Data(16bit&12bit) by Raw Info
	pRawReplayBoardCtx->pData16bit = (CVI_U16 *) calloc(1, pRawReplayBoardCtx->size16bit);
	if (pRawReplayBoardCtx->pData16bit == NULL) {
		LOGOUT("pRawReplayBoardCtx->pData16bit calloc failed!\n");
		return CVI_FAILURE;
	}
	memset(pRawReplayBoardCtx->pData16bit, 0, sizeof(pRawReplayBoardCtx->size16bit));
	pRawReplayBoardCtx->pData12bit = (CVI_U8 *) calloc(1, pRawReplayBoardCtx->size12bit);
	if (pRawReplayBoardCtx->pData12bit == NULL) {
		LOGOUT("pRawReplayBoardCtx->pData12bit calloc failed!\n");
		return CVI_FAILURE;
	}
	memset(pRawReplayBoardCtx->pData12bit, 0, sizeof(pRawReplayBoardCtx->size12bit));

	// Open RawFile to Load Data
	snprintf(rawFilePath, PATH_MAX_LEN, "%s/%s.raw", boardPath, rawFileName);
	pRawReplayBoardCtx->rawfp = fopen(rawFilePath, "rb");
	if (pRawReplayBoardCtx->rawfp == NULL) {
		LOGOUT("fopen %s failed!\n", rawFilePath);
		return CVI_FAILURE;
	}

	// pRawReplayBoardCtx Init
	pRawReplayBoardCtx->bRawReplayThreadEnabled = CVI_FALSE;
	pRawReplayBoardCtx->isRawReplayReady = CVI_FALSE;
	pRawReplayBoardCtx->timingAttr.bEnable = CVI_FALSE;
	pRawReplayBoardCtx->timingAttr.s32FrmRate = 25;

	return CVI_SUCCESS;
}

void raw_replay_offline_uninit(void)
{
	// Stop raw_replay_offline_thread if thread not stop
	if (pRawReplayBoardCtx->rawReplayStatus != RAW_REPLAY_STOP) {
		stop_raw_replay_offline();
	}
	if (pRawReplayBoardCtx->pRawHeader != NULL) {
		free(pRawReplayBoardCtx->pRawHeader);
	}
	if (pRawReplayBoardCtx->pData16bit != NULL) {
		free(pRawReplayBoardCtx->pData16bit);
	}
	if (pRawReplayBoardCtx->pData12bit != NULL) {
		free(pRawReplayBoardCtx->pData12bit);
	}
	if (pRawReplayBoardCtx->rawfp != NULL) {
		fclose(pRawReplayBoardCtx->rawfp);
	}
	free(pRawReplayBoardCtx);
}

CVI_S32 start_raw_replay_offline(VI_PIPE ViPipe)
{
	if (pRawReplayBoardCtx == NULL) {
		LOGOUT("pRawReplayBoardCtx == NULL\n");
		return CVI_FAILURE;
	}

	CVI_ISP_AESetRawReplayMode(0, CVI_TRUE);

	if (!pRawReplayBoardCtx->bRawReplayThreadEnabled) {
		pRawReplayBoardCtx->ViPipe = ViPipe;
		pRawReplayBoardCtx->bRawReplayThreadEnabled = CVI_TRUE;

		if (pthread_create(&pRawReplayBoardCtx->rawReplayTid, NULL, raw_replay_offline_thread,
									pRawReplayBoardCtx->pRawHeader) != 0) {
			LOGOUT("pthread_create failed!\n");
			return CVI_FAILURE;
		}
	}

	return CVI_SUCCESS;
}

CVI_S32 stop_raw_replay_offline(void)
{
	CVI_S32 s32Ret;

	CVI_ISP_AESetRawReplayMode(0, CVI_FALSE);

	if (pRawReplayBoardCtx->bRawReplayThreadEnabled) {

		pRawReplayBoardCtx->bRawReplayThreadEnabled = CVI_FALSE;

		pthread_join(pRawReplayBoardCtx->rawReplayTid, NULL);

		s32Ret = CVI_SYS_Munmap(pRawReplayBoardCtx->pu8VirAddr[0], pRawReplayBoardCtx->u32BlkSize);
		if (s32Ret != CVI_SUCCESS) {
			LOGOUT("Error: CVI_SYS_Munmap Failed!\n");
		}
		s32Ret = CVI_VB_ReleaseBlock(pRawReplayBoardCtx->blk);
		if (s32Ret != CVI_SUCCESS) {
			LOGOUT("Error: CVI_VB_ReleaseBlock Failed!\n");
		}
		s32Ret = CVI_VB_DestroyPool(pRawReplayBoardCtx->PoolID);
		if (s32Ret != CVI_SUCCESS) {
			LOGOUT("Error: CVI_VB_DestroyPool Failed!\n");
		}
		pRawReplayBoardCtx->rawReplayStatus = RAW_REPLAY_STOP;
	}
	return CVI_SUCCESS;
}

// load rawFileName from filePath
static CVI_S32 load_rawFileName_from_path(char *path, char *rawFileName, int maxFileNameLen)
{
	DIR *dir = NULL;
	struct dirent *ptr = NULL;
	int len = 0;

	dir = opendir(path);
	if (dir == NULL) {
		perror("Failed to open directory");
		return CVI_FAILURE;
	}
	while ((ptr = readdir(dir)) != NULL) {
		if (strcmp(ptr->d_name, "..") == 0) {
			continue;
		}
		// Load file name from .raw
		if (ptr->d_type == DT_REG) {
			if (strstr(ptr->d_name, ".raw") != 0) {
				len = strlen(ptr->d_name);
				if (len > maxFileNameLen) {
					printf("file name is too long!\n");
					return CVI_FAILURE;
				}
				snprintf(rawFileName, len - 3, "%s", ptr->d_name);
				break;
			}
		}
	}
	closedir(dir);
	return CVI_SUCCESS;
}

static CVI_S32 Bayer_16bit_2_12bit(CVI_U16 *Buffer16bit, CVI_U8 *Buffer12bit,
						CVI_U16 width, CVI_U16 height, CVI_U16 stride)
{
	CVI_U32 r, c;
	CVI_U16 pixel1, pixel2;
	CVI_U8 *p = NULL;

	if (Buffer16bit == NULL || Buffer12bit == NULL) {
		LOGOUT("pointer is NULL\n");
		return CVI_FAILURE;
	}
	p = Buffer12bit;
	for (r = 0; r < height; r++) {
		for (c = 0; c < width; c += 2) {
			pixel1 = *Buffer16bit;
			pixel2 = *(Buffer16bit + 1);

			*p = (pixel1 >> 4) & 0xFF;
			*(p + 1) = (pixel2 >> 4) & 0xFF;
			*(p + 2) = ((pixel1 & 0x0F) << 4) | (pixel2 & 0x0F);
			Buffer16bit += 2;
			p += 3;
		}
		Buffer16bit += (CVI_U16)((stride - width) * 2);
	}
	return CVI_SUCCESS;
}

// load 1 frame data(12bit) from the raw file(16bit)
static CVI_S32 load_one_frame_data_from_file(FILE *fp, RAW_REPLAY_INFO *pRawInfo, CVI_S32 curFrame)
{
	long offset = (long)curFrame * pRawReplayBoardCtx->size16bit;
	CVI_U64 frameRead;

	fseek(fp, offset, SEEK_SET);
	// Reset pData
	memset(pRawReplayBoardCtx->pData16bit, 0, sizeof(pRawReplayBoardCtx->size16bit));
	memset(pRawReplayBoardCtx->pData12bit, 0, sizeof(pRawReplayBoardCtx->size12bit));

	// Read and Transform 16bit Data to 12bit
	frameRead = fread(pRawReplayBoardCtx->pData16bit, 1, pRawReplayBoardCtx->size16bit, fp);
	if (frameRead < (CVI_U64) pRawReplayBoardCtx->size16bit) {
		// return failure if rearch the end of file
		if (feof(fp)) {
			LOGOUT("End of file reached prematurely after %d frames[%lu/%u].\n",
						curFrame+1, frameRead, pRawReplayBoardCtx->size16bit);
		} else {
			LOGOUT("File read error after %d frames[%lu/%u].\n", curFrame+1,
						frameRead, pRawReplayBoardCtx->size16bit);
		}
		return CVI_FAILURE;
	}
	Bayer_16bit_2_12bit(pRawReplayBoardCtx->pData16bit, pRawReplayBoardCtx->pData12bit,
						pRawInfo->width, pRawInfo->height, pRawInfo->width);
	return CVI_SUCCESS;
}

static char *get_str_value(const char *src, const char *str)
{
	int i = 0;
	char *temp = NULL;
	static char value[PATH_MAX_LEN] = {0};

	temp = strstr(src, str);
	if (temp == NULL) {
		goto FAIL;
	}
	temp = strchr(temp, '=');
	if (temp == NULL) {
		goto FAIL;
	}
	temp = strchr(temp, ' ');
	if (temp == NULL) {
		goto FAIL;
	}
	temp++;

	memset(value, '\0', PATH_MAX_LEN);
	for (i = 0; i < PATH_MAX_LEN; i++) {
		value[i] = *temp;
		temp++;
		if (*temp == '\r' || *temp == '\n') {
			break;
		}
	}
	return value;

FAIL:
	LOGOUT("get %s value fail!\n", str);
	return NULL;
}

// load raw info from TXT file
static CVI_S32 load_raw_info_from_file(char *boardPath, char *rawFileName, RAW_REPLAY_INFO *pRawInfo)
{
	FILE *fp = NULL;
	char *src = NULL;
	char *value = NULL;
	struct stat statBuf;
	char rawInfoFile[PATH_MAX_LEN] = {0};
	char bayerFormat[20], wdrFormat[20];
	char *bayerAndWdr = NULL;

	LOGOUT("raw info:\n");
	// Load Raw Info from RawFile Name
	// e.g:2560X1440_GRBG_Linear_-color=2_-bits=12_-frame=15_-hdr=1_ISO=100_20241029101706
	if (sscanf(rawFileName, "%dX%d", &pRawInfo->width, &pRawInfo->height) != 2) {
		LOGOUT("Failed to load Raw W*H.\n");
		goto FAIL;
	}
	bayerAndWdr = strchr(rawFileName, '_') + 1;
	if (sscanf(bayerAndWdr, "%16[^_]_%16[^_-]", bayerFormat, wdrFormat) != 2) {
		LOGOUT("Failed to load Raw Format.\n");
		goto FAIL;
	}
	if (strcmp(bayerFormat, "BGGR") == 0) {
		pRawInfo->bayerID = 0;
	} else if (strcmp(bayerFormat, "GBRG") == 0) {
		pRawInfo->bayerID = 1;
	} else if (strcmp(bayerFormat, "GRBG") == 0) {
		pRawInfo->bayerID = 2;
	} else if (strcmp(bayerFormat, "RGGB") == 0) {
		pRawInfo->bayerID = 3;
	} else {
		LOGOUT("bayerFormat(%s) is wrong! Please recheck.\n", bayerFormat);
		goto FAIL;
	}
	if (strcmp(wdrFormat, "Linear") == 0) {
		pRawInfo->enWDR = 0;
	} else if (strcmp(wdrFormat, "WDR") == 0) {
		pRawInfo->enWDR = 1;
	} else {
		LOGOUT("wdrFormat(%s) is wrong! Please recheck.\n", wdrFormat);
		goto FAIL;
	}
	sscanf(strstr(rawFileName, "-frame="), "-frame=%d", &pRawInfo->numFrame);
	pRawInfo->curFrame = 0;
	pRawInfo->pixFormat = 0;		// PixelFormat: 0 raw, !0 yuv(22 yuyv422,...)
	pRawReplayBoardCtx->size16bit = (CVI_U32) pRawInfo->width * pRawInfo->height * 2;		// 16bit RawSize
	pRawReplayBoardCtx->size12bit = (CVI_U32) pRawInfo->width * pRawInfo->height * 1.5;	// 12bit RawSize

	// Load Raw Info from TXT File
	snprintf(rawInfoFile, PATH_MAX_LEN, "%s/%s.txt", boardPath, rawFileName);
	fp = fopen(rawInfoFile, "rb");
	if (fp == NULL) {
		LOGOUT("fopen(%s) failed!\n", rawInfoFile);
		goto FAIL;
	}

	stat(rawInfoFile, &statBuf);
	src = (char *) calloc(statBuf.st_size + 1, 1);
	if (src == NULL) {
		LOGOUT("src == NULL\n");
		goto FAIL;
	}
	fread(src, statBuf.st_size, 1, fp);
	fclose(fp);
	fp = NULL;

	value = get_str_value(src, "ISO");
	if (value != NULL) {
		pRawInfo->ISO = atoi(value);
		LOGOUT("ISO = %d\n", pRawInfo->ISO);
	}

	value = get_str_value(src, "Light Value");
	if (value != NULL) {
		pRawInfo->lightValue = atof(value);
		LOGOUT("Light value = %f\n", pRawInfo->lightValue);
	}

	value = get_str_value(src, "Color Temp.");
	if (value != NULL) {
		pRawInfo->colorTemp = atoi(value);
		LOGOUT("Color Temp. = %d\n", pRawInfo->colorTemp);
	}

	value = get_str_value(src, "ISP DGain");
	if (value != NULL) {
		pRawInfo->ispDGain = atoi(value);
		LOGOUT("ISP DGain = %d\n", pRawInfo->ispDGain);
	}

	value = get_str_value(src, "Exposure Time");
	if (value != NULL) {
		pRawInfo->longExposure = atoi(value);
		LOGOUT("Exposure Time = %d\n", pRawInfo->longExposure);
	}

	value = get_str_value(src, "Short Exposure");
	if (value != NULL) {
		pRawInfo->shortExposure = atoi(value);
		LOGOUT("Short Exposure = %d\n", pRawInfo->shortExposure);
	}

	value = get_str_value(src, "Exposure Ratio");
	if (value != NULL) {
		pRawInfo->exposureRatio = atoi(value);
		LOGOUT("Exposure Ratio = %d\n", pRawInfo->exposureRatio);
	}

	value = get_str_value(src, "Exposure AGain");
	if (value != NULL) {
		pRawInfo->exposureAGain = atoi(value);
		LOGOUT("Exposure AGain = %d\n", pRawInfo->exposureAGain);
	}

	value = get_str_value(src, "Exposure DGain");
	if (value != NULL) {
		pRawInfo->exposureDGain = atoi(value);
		LOGOUT("Exposure DGain = %d\n", pRawInfo->exposureDGain);
	}

	value = get_str_value(src, "reg_wbg_rgain");
	if (value != NULL) {
		pRawInfo->WB_RGain = atoi(value);
		LOGOUT("reg_wbg_rgain = %d\n", pRawInfo->WB_RGain);
	}

	value = get_str_value(src, "reg_wbg_bgain");
	if (value != NULL) {
		pRawInfo->WB_BGain = atoi(value);
		LOGOUT("reg_wbg_bgain = %d\n", pRawInfo->WB_BGain);
	}

	value = get_str_value(src, "reg_wbg_grgain");
	if (value != NULL) {
		pRawInfo->WB_GGain = atoi(value);
		LOGOUT("reg_wbg_ggain = %d\n", pRawInfo->WB_GGain);
	}

	value = get_str_value(src, "reg_ccm_00");
	if (value != NULL) {
		pRawInfo->CCM[0] = atoi(value);
	}
	value = get_str_value(src, "reg_ccm_01");
	if (value != NULL) {
		pRawInfo->CCM[1] = atoi(value);
	}
	value = get_str_value(src, "reg_ccm_02");
	if (value != NULL) {
		pRawInfo->CCM[2] = atoi(value);
	}
	value = get_str_value(src, "reg_ccm_10");
	if (value != NULL) {
		pRawInfo->CCM[3] = atoi(value);
	}
	value = get_str_value(src, "reg_ccm_11");
	if (value != NULL) {
		pRawInfo->CCM[4] = atoi(value);
	}
	value = get_str_value(src, "reg_ccm_12");
	if (value != NULL) {
		pRawInfo->CCM[5] = atoi(value);
	}
	value = get_str_value(src, "reg_ccm_20");
	if (value != NULL) {
		pRawInfo->CCM[6] = atoi(value);
	}
	value = get_str_value(src, "reg_ccm_21");
	if (value != NULL) {
		pRawInfo->CCM[7] = atoi(value);
	}
	value = get_str_value(src, "reg_ccm_22");
	if (value != NULL) {
		pRawInfo->CCM[8] = atoi(value);
		LOGOUT("reg_ccm = \t%d, %d, %d\n\t\t%d, %d, %d\n\t\t%d, %d, %d\n",
						pRawInfo->CCM[0], pRawInfo->CCM[1], pRawInfo->CCM[2],
						pRawInfo->CCM[3], pRawInfo->CCM[4], pRawInfo->CCM[5],
						pRawInfo->CCM[6], pRawInfo->CCM[7], pRawInfo->CCM[8]);
	}

	value = get_str_value(src, "reg_blc_offset_r");
	if (value != NULL) {
		pRawInfo->BLC_Offset[0] = atoi(value);
	}
	value = get_str_value(src, "reg_blc_offset_gr");
	if (value != NULL) {
		pRawInfo->BLC_Offset[1] = atoi(value);
	}
	value = get_str_value(src, "reg_blc_offset_gb");
	if (value != NULL) {
		pRawInfo->BLC_Offset[2] = atoi(value);
	}
	value = get_str_value(src, "reg_blc_offset_b");
	if (value != NULL) {
		pRawInfo->BLC_Offset[3] = atoi(value);
		LOGOUT("reg_blc_offset = %d, %d, %d, %d\n",
						pRawInfo->BLC_Offset[0], pRawInfo->BLC_Offset[1],
						pRawInfo->BLC_Offset[2], pRawInfo->BLC_Offset[3]);
	}

	value = get_str_value(src, "reg_blc_gain_r");
	if (value != NULL) {
		pRawInfo->BLC_Gain[0] = atoi(value);
	}
	value = get_str_value(src, "reg_blc_gain_gr");
	if (value != NULL) {
		pRawInfo->BLC_Gain[1] = atoi(value);
	}
	value = get_str_value(src, "reg_blc_gain_gb");
	if (value != NULL) {
		pRawInfo->BLC_Gain[2] = atoi(value);
	}
	value = get_str_value(src, "reg_blc_gain_b");
	if (value != NULL) {
		pRawInfo->BLC_Gain[3] = atoi(value);
		LOGOUT("reg_blc_gain = %d, %d, %d, %d\n",
						pRawInfo->BLC_Gain[0], pRawInfo->BLC_Gain[1],
						pRawInfo->BLC_Gain[2], pRawInfo->BLC_Gain[3]);
	}

	free(src);
	src = NULL;

	return CVI_SUCCESS;

FAIL:
	if (fp != NULL) {
		fclose(fp);
		fp = NULL;
	}
	if (src != NULL) {
		free(src);
		src = NULL;
	}

	return CVI_FAILURE;
}

// put one frame raw data into vbpool
static CVI_S32 set_1_frame_raw_data_into_vbpool(RAW_REPLAY_INFO *pRawInfo,
						CVI_U8 *Buffer12bit, CVI_U32 size12bit, CVI_S32 curFrame)
{
	CVI_U64 tmpPhyAddr;
	VB_POOL_CONFIG_S vbCfg;

	if (curFrame == 0) {
		if (pRawReplayBoardCtx->pRawHeader == NULL) {
			LOGOUT("Abort: pRawReplayBoardCtx->pRawHeader == NULL\n");
			abort();
		}

		vbCfg.u32BlkSize = size12bit;
		vbCfg.u32BlkCnt = 1;
		vbCfg.enRemapMode = VB_REMAP_MODE_CACHED;
		snprintf(vbCfg.acName, MAX_VB_POOL_NAME_LEN, "%s", "raw_replay_offline_vb");
		pRawReplayBoardCtx->u32BlkSize = vbCfg.u32BlkSize;

		if (pRawReplayBoardCtx->PoolID == 0) {
			pRawReplayBoardCtx->PoolID = CVI_VB_CreatePool(&vbCfg);
			if (pRawReplayBoardCtx->PoolID == VB_INVALID_POOLID) {
				LOGOUT("Abort: pRawReplayBoardCtx->PoolID == VB_INVALID_POOLID\n");
				abort();
			}

			pRawReplayBoardCtx->blk = CVI_VB_GetBlock(pRawReplayBoardCtx->PoolID, vbCfg.u32BlkSize);
			tmpPhyAddr = CVI_VB_Handle2PhysAddr(pRawReplayBoardCtx->blk);
			pRawReplayBoardCtx->u64PhyAddr[0] = tmpPhyAddr;
			pRawReplayBoardCtx->pu8VirAddr[0] = (CVI_U8 *) CVI_SYS_MmapCache(tmpPhyAddr, vbCfg.u32BlkSize);
			if (pRawInfo->enWDR || pRawInfo->pixFormat) {
				pRawReplayBoardCtx->u64PhyAddr[1] =
						pRawReplayBoardCtx->u64PhyAddr[0] + pRawReplayBoardCtx->u32BlkSize / 2;
				pRawReplayBoardCtx->pu8VirAddr[1] =
						pRawReplayBoardCtx->pu8VirAddr[0] + pRawReplayBoardCtx->u32BlkSize / 2;
			}
			LOGOUT("create vb pool cnt: %d, blksize: %d phyAddr: %lu\n", 1,
						vbCfg.u32BlkSize, pRawReplayBoardCtx->u64PhyAddr[0]);
		}
	}
	if (curFrame < pRawInfo->numFrame) {
		if (pRawInfo->enWDR || pRawInfo->pixFormat) {
			memcpy(pRawReplayBoardCtx->pu8VirAddr[0], Buffer12bit, size12bit / 2);
			memcpy(pRawReplayBoardCtx->pu8VirAddr[1], Buffer12bit + size12bit / 2, size12bit / 2);
			CVI_SYS_IonFlushCache(pRawReplayBoardCtx->u64PhyAddr[0],
						pRawReplayBoardCtx->pu8VirAddr[0], size12bit / 2);
			CVI_SYS_IonFlushCache(pRawReplayBoardCtx->u64PhyAddr[1],
						pRawReplayBoardCtx->pu8VirAddr[1], size12bit / 2);
		} else {
			memcpy(pRawReplayBoardCtx->pu8VirAddr[0], Buffer12bit, size12bit);
			CVI_SYS_IonFlushCache(pRawReplayBoardCtx->u64PhyAddr[0],
						pRawReplayBoardCtx->pu8VirAddr[0], size12bit);
		}
	} else {
		LOGOUT("curFrame[%u] >= TotalFrame[%u]!\n", curFrame, pRawInfo->numFrame);
		return CVI_FAILURE;
	}
	return CVI_SUCCESS;
}

static void get_current_awb_info(ISP_MWB_ATTR_S *pstMwbAttr)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	ISP_WB_INFO_S stWBInfo;

	memset(&stWBInfo, 0, sizeof(ISP_WB_INFO_S));
	s32Ret = CVI_ISP_QueryWBInfo(pRawReplayBoardCtx->ViPipe, &stWBInfo);
	if (s32Ret != CVI_SUCCESS) {
		LOGOUT("Error: CVI_ISP_QueryWBInfo Failed!\n");
	}

	pstMwbAttr->u16Rgain = stWBInfo.u16Rgain;
	pstMwbAttr->u16Grgain = stWBInfo.u16Grgain;
	pstMwbAttr->u16Gbgain = stWBInfo.u16Gbgain;
	pstMwbAttr->u16Bgain = stWBInfo.u16Bgain;
}

static void update_awb_config(const ISP_MWB_ATTR_S *pstMwbAttr, ISP_OP_TYPE_E eType)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	ISP_WB_ATTR_S stWbAttr;

	if (DISABLE_AWB_UPDATE_CTRL != 0) {
		return;
	}

	memset(&stWbAttr, 0, sizeof(ISP_WB_ATTR_S));
	s32Ret = CVI_ISP_GetWBAttr(pRawReplayBoardCtx->ViPipe, &stWbAttr);
	if (s32Ret != CVI_SUCCESS) {
		LOGOUT("Error: CVI_ISP_GetWBAttr Failed!\n");
	}

	if (eType == OP_TYPE_MANUAL) {
		stWbAttr.enOpType = OP_TYPE_MANUAL;
		stWbAttr.stManual = *pstMwbAttr;
	} else {
		stWbAttr.enOpType = OP_TYPE_AUTO;
	}

	s32Ret = CVI_ISP_SetWBAttr(pRawReplayBoardCtx->ViPipe, &stWbAttr);
	if (s32Ret != CVI_SUCCESS) {
		LOGOUT("Error: CVI_ISP_SetWBAttr Failed!\n");
	}
}

static void get_current_ae_info(ISP_EXP_INFO_S *pstExpInfo)
{
	CVI_S32 s32Ret = CVI_SUCCESS;

	s32Ret = CVI_ISP_QueryExposureInfo(pRawReplayBoardCtx->ViPipe, pstExpInfo);
	if (s32Ret != CVI_SUCCESS) {
		LOGOUT("Error: CVI_ISP_QueryExposureInfo Failed!\n");
	}
}

static void update_ae_config(const ISP_EXP_INFO_S *pstExpInfo, ISP_OP_TYPE_E eType)
{
	CVI_S32 s32Ret = CVI_SUCCESS;

	ISP_EXPOSURE_ATTR_S stAEAttr;
	ISP_WDR_EXPOSURE_ATTR_S stWdrExpAttr;

	memset(&stAEAttr, 0, sizeof(ISP_EXPOSURE_ATTR_S));
	memset(&stWdrExpAttr, 0, sizeof(ISP_WDR_EXPOSURE_ATTR_S));
	s32Ret = CVI_ISP_GetExposureAttr(pRawReplayBoardCtx->ViPipe, &stAEAttr);
	if (s32Ret != CVI_SUCCESS) {
		LOGOUT("Error: CVI_ISP_GetExposureAttr Failed!\n");
	}
	s32Ret = CVI_ISP_GetWDRExposureAttr(pRawReplayBoardCtx->ViPipe, &stWdrExpAttr);
	if (s32Ret != CVI_SUCCESS) {
		LOGOUT("Error: CVI_ISP_GetWDRExposureAttr Failed!\n");
	}

	if (eType == OP_TYPE_MANUAL) {
		stAEAttr.enOpType = OP_TYPE_MANUAL;

		stAEAttr.stManual.enExpTimeOpType = OP_TYPE_MANUAL;
		stAEAttr.stManual.enAGainOpType = OP_TYPE_MANUAL;
		stAEAttr.stManual.enDGainOpType = OP_TYPE_MANUAL;
		stAEAttr.stManual.enISPDGainOpType = OP_TYPE_MANUAL;
		stAEAttr.stManual.enGainType = AE_TYPE_GAIN;

		stAEAttr.stManual.u32ExpTime = pstExpInfo->u32ExpTime;
		stAEAttr.stManual.u32AGain = pstExpInfo->u32AGain;
		stAEAttr.stManual.u32DGain = pstExpInfo->u32DGain;
		stAEAttr.stManual.u32ISPDGain = pstExpInfo->u32ISPDGain;

		if (pRawReplayBoardCtx->pRawHeader->numFrame == 1) {
			stWdrExpAttr.enExpRatioType = OP_TYPE_MANUAL;

			for (CVI_U32 i = 0; i < WDR_EXP_RATIO_NUM; i++) {
				stWdrExpAttr.au32ExpRatio[i] = pstExpInfo->u32WDRExpRatio;
			}
		}

		memcpy((CVI_U8 *)&stAEAttr.stAuto.au32Reserve[0], (CVI_U8 *)&pstExpInfo->fLightValue,
								sizeof(CVI_FLOAT));
	} else {
		stAEAttr.enOpType = OP_TYPE_AUTO;

		stAEAttr.stManual.enExpTimeOpType = OP_TYPE_AUTO;
		stAEAttr.stManual.enGainType = AE_TYPE_GAIN;
		stAEAttr.stManual.enISONumOpType = OP_TYPE_AUTO;

		stWdrExpAttr.enExpRatioType = OP_TYPE_AUTO;
	}

	s32Ret = CVI_ISP_SetExposureAttr(pRawReplayBoardCtx->ViPipe, &stAEAttr);
	if (s32Ret != CVI_SUCCESS) {
		LOGOUT("Error: CVI_ISP_SetExposureAttr Failed!\n");
	}
	s32Ret = CVI_ISP_SetWDRExposureAttr(pRawReplayBoardCtx->ViPipe, &stWdrExpAttr);
	if (s32Ret != CVI_SUCCESS) {
		LOGOUT("Error: CVI_ISP_SetWDRExposureAttr Failed!\n");
	}

	if (eType == OP_TYPE_MANUAL)
		CVI_ISP_AESetRawReplayExposure(pRawReplayBoardCtx->ViPipe, pstExpInfo);
}

static void apply_raw_info(ISP_MWB_ATTR_S *pstMwbAttr, ISP_EXP_INFO_S *pstExpInfo, RAW_REPLAY_INFO *pRawInfo)
{
	pstMwbAttr->u16Rgain = pRawInfo->WB_RGain;
	pstMwbAttr->u16Grgain = pstMwbAttr->u16Gbgain = pRawInfo->WB_GGain;
	pstMwbAttr->u16Bgain = pRawInfo->WB_BGain;

	pstExpInfo->u32ShortExpTime = pRawInfo->shortExposure;
	pstExpInfo->u32ExpTime = pRawInfo->longExposure;
	pstExpInfo->u32AGain = pRawInfo->exposureAGain;
	pstExpInfo->u32DGain = pRawInfo->exposureDGain;
	pstExpInfo->u32ISPDGain = pRawInfo->ispDGain;

	pstExpInfo->u32ISO = pRawInfo->ISO;
	pstExpInfo->fLightValue = pRawInfo->lightValue;

	pstExpInfo->u32WDRExpRatio = pRawInfo->exposureRatio;
}

CVI_BOOL is_raw_replay_offline_ready(void)
{
	if (pRawReplayBoardCtx != NULL) {
		return pRawReplayBoardCtx->isRawReplayReady;
	} else {
		return CVI_FALSE;
	}
}

// Update Frame by RawInfo (Size\Addr\Format\...)
static void update_video_frame(VIDEO_FRAME_INFO_S *stVideoFrame, RAW_REPLAY_INFO *pRawInfo)
{
	CVI_U8 mode = pRawInfo->enWDR;
	//LOGOUT("wdrmode: %d, width: %d, height: %d\n", mode, pRawInfo->width, pRawInfo->height);

	if (mode) {
		stVideoFrame->stVFrame.u32Width = pRawInfo->width >> 1;
	} else {
		stVideoFrame->stVFrame.u32Width = pRawInfo->width;
	}

	stVideoFrame->stVFrame.u32Height = pRawInfo->height;

	stVideoFrame->stVFrame.s16OffsetLeft = stVideoFrame->stVFrame.s16OffsetTop =
		stVideoFrame->stVFrame.s16OffsetRight = stVideoFrame->stVFrame.s16OffsetBottom = 0;

	stVideoFrame->stVFrame.enBayerFormat = (BAYER_FORMAT_E) pRawInfo->bayerID;
	stVideoFrame->stVFrame.enPixelFormat = (PIXEL_FORMAT_E) 0;

	if (mode) {
		stVideoFrame->stVFrame.enDynamicRange = DYNAMIC_RANGE_HDR10;
		stVideoFrame->stVFrame.u64PhyAddr[0] = pRawReplayBoardCtx->u64PhyAddr[0];
		stVideoFrame->stVFrame.u64PhyAddr[1] = pRawReplayBoardCtx->u64PhyAddr[1];
	} else if (pRawInfo->pixFormat) {
		stVideoFrame->stVFrame.enDynamicRange = DYNAMIC_RANGE_SDR10;
		stVideoFrame->stVFrame.enPixelFormat = (PIXEL_FORMAT_E) pRawInfo->pixFormat;
		stVideoFrame->stVFrame.u64PhyAddr[0] = pRawReplayBoardCtx->u64PhyAddr[0];
		stVideoFrame->stVFrame.u64PhyAddr[1] = pRawReplayBoardCtx->u64PhyAddr[1];
	} else {
		stVideoFrame->stVFrame.enDynamicRange = DYNAMIC_RANGE_SDR10;
		stVideoFrame->stVFrame.u64PhyAddr[0] = pRawReplayBoardCtx->u64PhyAddr[0];
	}
}

static void send_replay_frame_2_vi(ISP_MWB_ATTR_S *pstMwbAttr,
						ISP_EXP_INFO_S *pstExpInfo, VIDEO_FRAME_INFO_S *pstVideoFrame)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	VI_PIPE PipeId[] = {0};
	const VIDEO_FRAME_INFO_S *pstVideoFrameInfo[1];

	pstVideoFrameInfo[0] = pstVideoFrame;

	if (!pRawReplayBoardCtx->pRawHeader->pixFormat) {
		apply_raw_info(pstMwbAttr, pstExpInfo, pRawReplayBoardCtx->pRawHeader);
		update_awb_config(pstMwbAttr, OP_TYPE_MANUAL);
		update_ae_config(pstExpInfo, OP_TYPE_MANUAL);
	}

	if (pRawReplayBoardCtx->pRawHeader->enWDR  || pRawReplayBoardCtx->pRawHeader->pixFormat) {
		pstVideoFrame->stVFrame.u64PhyAddr[0] = pRawReplayBoardCtx->u64PhyAddr[0];
		pstVideoFrame->stVFrame.u64PhyAddr[1] = pRawReplayBoardCtx->u64PhyAddr[1];
	} else {
		pstVideoFrame->stVFrame.u64PhyAddr[0] = pRawReplayBoardCtx->u64PhyAddr[0];
	}

	s32Ret = CVI_VI_SendPipeRaw(1, PipeId, pstVideoFrameInfo, 0);
	if (s32Ret != CVI_SUCCESS) {
		LOGOUT("Error: CVI_VI_SendPipeRaw Failed!\n");
	}

	CVI_ISP_GetVDTimeOut(0, ISP_VD_FE_END, 0);
}

static void *raw_replay_offline_thread(void *arg)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	VIDEO_FRAME_INFO_S stVideoFrame;
	ISP_MWB_ATTR_S stMWBAttr;
	ISP_EXP_INFO_S stExpInfo;
	VI_DEV_TIMING_ATTR_S stTimingAttr;
	RAW_REPLAY_INFO *pRawInfo = (RAW_REPLAY_INFO *) arg;
	struct timeval timeLoad, timeSend;
	CVI_U32 frameTime, frameRate;
	CVI_S32 curFrame = 0, replayStep = 1;

	// Get AE/AWB Info
	memset(&stMWBAttr, 0, sizeof(ISP_MWB_ATTR_S));
	memset(&stExpInfo, 0, sizeof(ISP_EXP_INFO_S));
	get_current_awb_info(&stMWBAttr);
	get_current_ae_info(&stExpInfo);
	LOGOUT("wbRGain:%d,wbGGain:%d,wbBGain:%d,exptime:%d,iso:%d,expratio:%d,LV:%f\n",
				pRawInfo->WB_RGain, pRawInfo->WB_GGain, pRawInfo->WB_BGain, pRawInfo->longExposure,
				pRawInfo->ISO, pRawInfo->exposureRatio, pRawInfo->lightValue);

	s32Ret = CVI_VI_GetDevTimingAttr(0, &stTimingAttr);
	if (s32Ret == CVI_SUCCESS) {
		pRawReplayBoardCtx->timingAttr.bEnable = stTimingAttr.bEnable;
		pRawReplayBoardCtx->timingAttr.s32FrmRate = stTimingAttr.s32FrmRate;
	}
	LOGOUT("\n\nstart raw replay, mode %d (0 manu, 1 auto), framerate %d...\n\n",
		pRawReplayBoardCtx->timingAttr.bEnable, pRawReplayBoardCtx->timingAttr.s32FrmRate);

	// Reload & Update & Send one Frame in RawData to VI
	while (pRawReplayBoardCtx->bRawReplayThreadEnabled) {
		frameRate = pRawReplayBoardCtx->timingAttr.s32FrmRate;
		gettimeofday(&timeLoad, NULL);

		//Reload one Frame Data from Raw File
		pRawReplayBoardCtx->rawReplayStatus = RAW_REPLAY_RELOAD;
		load_one_frame_data_from_file(pRawReplayBoardCtx->rawfp, pRawReplayBoardCtx->pRawHeader, curFrame);
		set_1_frame_raw_data_into_vbpool(pRawReplayBoardCtx->pRawHeader, pRawReplayBoardCtx->pData12bit,
				pRawReplayBoardCtx->size12bit, curFrame);

		// Update one Frame Data to VB
		pRawReplayBoardCtx->rawReplayStatus = RAW_REPLAY_UPDATE;
		update_video_frame(&stVideoFrame, pRawInfo);
		pRawReplayBoardCtx->isRawReplayReady = CVI_TRUE;
		send_replay_frame_2_vi(&stMWBAttr, &stExpInfo, &stVideoFrame);
		pRawReplayBoardCtx->isRawReplayReady = CVI_FALSE;

		// Refresh one Frame per frameRate
		pRawReplayBoardCtx->rawReplayStatus = RAW_REPLAY_SLEEP;
		gettimeofday(&timeSend, NULL);
		if (!pRawReplayBoardCtx->timingAttr.bEnable) {
			frameRate = frameRate > 0 ? frameRate : 25;
			frameTime = SEC_TO_USEC / frameRate;
			CVI_U64 interval = (timeSend.tv_sec - timeLoad.tv_sec) * SEC_TO_USEC
							+ (timeSend.tv_usec - timeLoad.tv_usec);
			if (frameTime > interval)
				usleep(frameTime - interval);
		}

		// Rewind the Raw when the last frame is reached
		if ((curFrame == pRawInfo->numFrame - 1) && (replayStep == 1)) {
			replayStep = -replayStep;
		} else if ((curFrame == 0) && (replayStep == -1)) {
			replayStep = -replayStep;
		}
		curFrame += replayStep;
	}
	LOGOUT("/*** raw replay therad end ***/\n");
	return NULL;
}

#if defined(__GNUC__) && defined(__riscv)
#pragma GCC diagnostic pop
#endif
