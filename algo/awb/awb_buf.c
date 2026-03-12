
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cvi_comm_3a.h"
#include "cvi_awb_comm.h"
#include "awb_buf.h"

#if defined(__CV181X__) || defined(__CV180X__) || defined(__CV186X__)
#include "isp_mgr_buf.h"
#else
CVI_U32 u32AWBParamUpdateFlag[AWB_SENSOR_NUM];
#endif

#ifdef ARCH_RTOS_CV181X
#include "malloc.h"
#endif

static CVI_S32 awb_malloc_cnt[AWB_SENSOR_NUM];

ISP_WB_Q_INFO_S *pstAwb_Q_Info[AWB_SENSOR_NUM];
ISP_WB_ATTR_S *pstAwbMpiAttr[AWB_SENSOR_NUM];
ISP_AWB_ATTR_EX_S *pstAwbMpiAttrEx[AWB_SENSOR_NUM];
ISP_AWB_Calibration_Gain_S *pstWbCalibration[AWB_SENSOR_NUM];

ISP_WB_ATTR_S *stAwbAttrInfo[AWB_SENSOR_NUM];
ISP_AWB_ATTR_EX_S *stAwbAttrInfoEx[AWB_SENSOR_NUM];
ISP_AWB_Calibration_Gain_S *stWbDefCalibration[AWB_SENSOR_NUM];
ISP_AWB_Calibration_Gain_S_EX *stWbCalibrationEx[AWB_SENSOR_NUM];
sWBInfo *WBInfo[AWB_SENSOR_NUM];
sWBSampleInfo *stSampleInfo[AWB_SENSOR_NUM];

#ifndef ARCH_RTOS_CV181X // TODO@CV181X
void *AWB_Malloc(CVI_U8 sID, size_t nsize)
{
	void *ptr = NULL;

	ptr = malloc(nsize);
	if (ptr != NULL) {
		awb_malloc_cnt[sID]++;
	}
	return ptr;
}
#else
void *AWB_Malloc(CVI_U8 sID, size_t nsize)
{
	void *ptr = NULL;

	ptr = pvPortMalloc(nsize);
	if (ptr != NULL) {
		awb_malloc_cnt[sID]++;
	}
	return ptr;
}
#endif

#ifndef ARCH_RTOS_CV181X // TODO@CV181X
void AWB_Free(CVI_U8 sID, void *ptr)
{
	if (ptr != NULL) {
		awb_malloc_cnt[sID]--;
		free(ptr);
	}
}
#else
void AWB_Free(CVI_U8 sID, void *ptr)
{
	if (ptr != NULL) {
		awb_malloc_cnt[sID]--;
		vPortFree(ptr);
	}
}
#endif

void AWB_CheckMemFree(CVI_U8 sID)
{
	if (awb_malloc_cnt[sID] != 0) {
		printf("awb check memory free, pipe: %d error: malloc/free is not match %d\n", sID,
		awb_malloc_cnt[sID]);
	}
}

void awb_buf_init(CVI_U8 sID)
{
	CVI_S32 ret = CVI_SUCCESS;

	if (stAwbAttrInfo[sID] == CVI_NULL) {
		stAwbAttrInfo[sID] =
			(ISP_WB_ATTR_S *) AWB_Malloc(sID, sizeof(ISP_WB_ATTR_S));
		if (stAwbAttrInfo[sID] == CVI_NULL) {
			ret = CVI_FAILURE;
		}
	}

	if (stAwbAttrInfoEx[sID] == CVI_NULL) {
		stAwbAttrInfoEx[sID] =
			(ISP_AWB_ATTR_EX_S *) AWB_Malloc(sID, sizeof(ISP_AWB_ATTR_EX_S));
		if (stAwbAttrInfoEx[sID] == CVI_NULL) {
			ret = CVI_FAILURE;
		}
	}

	if (WBInfo[sID] == CVI_NULL) {
		WBInfo[sID] =
			(sWBInfo *) AWB_Malloc(sID, sizeof(sWBInfo));
		if (WBInfo[sID] == CVI_NULL) {
			ret = CVI_FAILURE;
		}
	}

	if (stWbDefCalibration[sID] == CVI_NULL) {
		stWbDefCalibration[sID] =
			(ISP_AWB_Calibration_Gain_S *) AWB_Malloc(sID, sizeof(ISP_AWB_Calibration_Gain_S));
		if (stWbDefCalibration[sID] == CVI_NULL) {
			ret = CVI_FAILURE;
		}
	}

	if (stWbCalibrationEx[sID] == CVI_NULL) {
		stWbCalibrationEx[sID] =
			(ISP_AWB_Calibration_Gain_S_EX *) AWB_Malloc(sID, sizeof(ISP_AWB_Calibration_Gain_S_EX));
		if (stWbCalibrationEx[sID] == CVI_NULL) {
			ret = CVI_FAILURE;
		}
	}

	if (stSampleInfo[sID] == CVI_NULL) {
		stSampleInfo[sID] =
			(sWBSampleInfo *) AWB_Malloc(sID, sizeof(sWBSampleInfo));
		if (stSampleInfo[sID] == CVI_NULL) {
			ret = CVI_FAILURE;
		}
	}

#if defined(__CV181X__) || defined(__CV180X__) || defined(__CV186X__)
	struct isp_3a_shared_buffer *p = CVI_NULL;

	isp_mgr_buf_get_3a_addr(sID, (CVI_VOID **) &p);

	pstAwb_Q_Info[sID] = &p->stWB_Q_Info;
	pstAwbMpiAttr[sID] = &p->stWBAttr;
	pstAwbMpiAttrEx[sID] = &p->stAWBAttrEx;
	pstWbCalibration[sID] = &p->stWBCalib;
#else
	if (pstAwb_Q_Info[sID] == CVI_NULL) {
		pstAwb_Q_Info[sID] =
			(ISP_WB_Q_INFO_S *) AWB_Malloc(sID, sizeof(ISP_WB_Q_INFO_S));
		if (pstAwb_Q_Info[sID] == CVI_NULL) {
			ret = CVI_FAILURE;
		}
	}

	if (pstAwbMpiAttr[sID] == CVI_NULL) {
		pstAwbMpiAttr[sID] =
			(ISP_WB_ATTR_S *) AWB_Malloc(sID, sizeof(ISP_WB_ATTR_S));
		if (pstAwbMpiAttr[sID] == CVI_NULL) {
			ret = CVI_FAILURE;
		}
	}

	if (pstAwbMpiAttrEx[sID] == CVI_NULL) {
		pstAwbMpiAttrEx[sID] =
			(ISP_AWB_ATTR_EX_S *) AWB_Malloc(sID, sizeof(ISP_AWB_ATTR_EX_S));
		if (pstAwbMpiAttrEx[sID] == CVI_NULL) {
			ret = CVI_FAILURE;
		}
	}

	if (pstWbCalibration[sID] == CVI_NULL) {
		pstWbCalibration[sID] =
			(ISP_AWB_Calibration_Gain_S *) AWB_Malloc(sID, sizeof(ISP_AWB_Calibration_Gain_S));
		if (pstWbCalibration[sID] == CVI_NULL) {
			ret = CVI_FAILURE;
		}
	}
#endif

	if (ret != CVI_SUCCESS) {
		printf("error, awb buf init fail\n");
	}
}

void awb_buf_deinit(CVI_U8 sID)
{
#if defined(__CV181X__) || defined(__CV180X__) || defined(__CV186X__)
	pstAwb_Q_Info[sID] = CVI_NULL;
	pstAwbMpiAttr[sID] = CVI_NULL;
	pstAwbMpiAttrEx[sID] = CVI_NULL;
	pstWbCalibration[sID] = CVI_NULL;
#else
	if (pstAwb_Q_Info[sID] != CVI_NULL) {
		AWB_Free(sID, pstAwb_Q_Info[sID]);
		pstAwb_Q_Info[sID] = CVI_NULL;
	}

	if (pstAwbMpiAttr[sID] != CVI_NULL) {
		AWB_Free(sID, pstAwbMpiAttr[sID]);
		pstAwbMpiAttr[sID] = CVI_NULL;
	}

	if (pstAwbMpiAttrEx[sID] != CVI_NULL) {
		AWB_Free(sID, pstAwbMpiAttrEx[sID]);
		pstAwbMpiAttrEx[sID] = CVI_NULL;
	}

	if (pstWbCalibration[sID] != CVI_NULL) {
		AWB_Free(sID, pstWbCalibration[sID]);
		pstWbCalibration[sID] = CVI_NULL;
	}
#endif

	if (stSampleInfo[sID] != CVI_NULL) {
		AWB_Free(sID, stSampleInfo[sID]);
		stSampleInfo[sID] = CVI_NULL;
	}

	if (stWbCalibrationEx[sID] != CVI_NULL) {
		AWB_Free(sID, stWbCalibrationEx[sID]);
		stWbCalibrationEx[sID] = CVI_NULL;
	}

	if (stWbDefCalibration[sID] != CVI_NULL) {
		AWB_Free(sID, stWbDefCalibration[sID]);
		stWbDefCalibration[sID] = CVI_NULL;
	}

	if (WBInfo[sID] != CVI_NULL) {
		AWB_Free(sID, WBInfo[sID]);
		WBInfo[sID] = CVI_NULL;
	}

	if (stAwbAttrInfoEx[sID] != CVI_NULL) {
		AWB_Free(sID, stAwbAttrInfoEx[sID]);
		stAwbAttrInfoEx[sID] = CVI_NULL;
	}

	if (stAwbAttrInfo[sID] != CVI_NULL) {
		AWB_Free(sID, stAwbAttrInfo[sID]);
		stAwbAttrInfo[sID] = CVI_NULL;
	}
}

void AWB_SetParamUpdateFlag(CVI_U8 sID, AWB_PARAMETER_UPDATE flag)
{
#if defined(__CV181X__) || defined(__CV180X__) || defined(__CV186X__)
	struct isp_3a_shared_buffer *p = CVI_NULL;

	isp_mgr_buf_get_3a_addr(sID, (CVI_VOID **) &p);
	p->u32AWBParamUpdateFlag |= (0x01 << flag);
#else
	u32AWBParamUpdateFlag[sID] |= (0x01 << flag);
#endif
}

void AWB_CheckParamUpdateFlag(CVI_U8 sID)
{
	CVI_U32 flag = 0;

#if defined(__CV181X__) || defined(__CV180X__) || defined(__CV186X__)
	struct isp_3a_shared_buffer *p = CVI_NULL;

	isp_mgr_buf_get_3a_addr(sID, (CVI_VOID **) &p);
	flag = p->u32AWBParamUpdateFlag;
	if (flag != 0x00) {
		p->u32AWBParamUpdateFlag = 0x00;
	}
#else
	flag = u32AWBParamUpdateFlag[sID];
	if (flag != 0x00) {
		u32AWBParamUpdateFlag[sID] = 0x00;
	}
#endif

	if (flag != 0x00) {

		if (flag & (0x01 << AWB_ATTR_UPDATE)) {
			isNeedUpdateAttr[sID] = 1;
		}

		if (flag & (0x01 << AWB_ATTR_EX_UPDATE)) {
			isNeedUpdateAttrEx[sID] = 1;
		}

		if (flag & (0x01 << AWB_CALIB_UPDATE)) {
			isNeedUpdateCalib[sID] = 1;
		}
	}
}

#ifdef ARCH_RTOS_CV181X // TODO: mason.zou
#include "isp_mgr_buf.h"
void AWB_RtosBufInit(CVI_U8 sID)
{
	struct isp_3a_shared_buffer *p = CVI_NULL;

	isp_mgr_buf_get_3a_addr(sID, (CVI_VOID **) &p);

	pstAwb_Q_Info[sID] = &p->stWB_Q_Info;
	pstAwbMpiAttr[sID] = &p->stWBAttr;
	pstAwbMpiAttrEx[sID] = &p->stAWBAttrEx;
	pstWbCalibration[sID] = &p->stWBCalib;
}
#endif

#ifdef ENABLE_ISP_IPC
#include "isp_mgr_buf.h"
void AWB_BufInit(CVI_U8 sID)
{
	struct isp_3a_shared_buffer *p = CVI_NULL;

	isp_mgr_buf_get_3a_addr(sID, (CVI_VOID **) &p);

	pstAwb_Q_Info[sID] = &p->stWB_Q_Info;
	pstAwbMpiAttr[sID] = &p->stWBAttr;
	pstAwbMpiAttrEx[sID] = &p->stAWBAttrEx;
	pstWbCalibration[sID] = &p->stWBCalib;
}
#endif
