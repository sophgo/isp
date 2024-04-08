/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_mw_compat.c
 * Description:
 *
 */

#include <unistd.h>

#include "isp_mw_compat.h"
#include "isp_defines.h"

CVI_VOID *MW_COMPAT_Mmap(CVI_U64 u64PhyAddr, CVI_U32 u32Size)
{
	UNUSED(u32Size);
	return ISP_PTR_CAST_VOID(u64PhyAddr);
}

CVI_VOID *MW_COMPAT_MmapCache(CVI_U64 u64PhyAddr, CVI_U32 u32Size)
{
	UNUSED(u32Size);
	return ISP_PTR_CAST_VOID(u64PhyAddr);
}

CVI_S32 MW_COMPAT_usleep(CVI_U32 us)
{
	usleep(us);

	return CVI_SUCCESS;
}

CVI_S32 MW_COMPAT_InvalidateCache(CVI_U64 u64PhyAddr, CVI_VOID *pVirAddr, CVI_U32 u32Len)
{
	return CVI_SYS_IonInvalidateCache(u64PhyAddr, pVirAddr, u32Len);
}

CVI_S32 MW_COMPAT_FlushCache(CVI_U64 u64PhyAddr, CVI_VOID *pVirAddr, CVI_U32 u32Len)
{
	return CVI_SYS_IonFlushCache(u64PhyAddr, pVirAddr, u32Len);
}

