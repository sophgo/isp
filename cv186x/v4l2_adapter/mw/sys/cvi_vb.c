#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/queue.h>
#include <pthread.h>
#include <stdatomic.h>
#include <inttypes.h>

#include "cvi_base.h"
#include "cvi_vb.h"
#include "cvi_sys.h"

#define UNUSED(x) ((void)(x))


typedef struct _pool {
	CVI_U64 memBase;
	void *vmemBase;
	CVI_U32 u32BlkSize;
	CVI_U32 u32BlkCnt;
	VB_REMAP_MODE_E enRemapMode;
} VB_POOL_S;

pthread_mutex_t hash_lock;

VB_BLK CVI_VB_GetBlock(VB_POOL Pool, CVI_U32 u32BlkSize)
{
	UNUSED(Pool);
	UNUSED(u32BlkSize);
	return 0;
}

CVI_S32 CVI_VB_ReleaseBlock(VB_BLK Block)
{
	UNUSED(Block);
	return 0;
}

VB_BLK CVI_VB_PhysAddr2Handle(CVI_U64 u64PhyAddr)
{
	UNUSED(u64PhyAddr);
	return 0;
}

CVI_U64 CVI_VB_Handle2PhysAddr(VB_BLK Block)
{
	UNUSED(Block);
	return 0;
}

VB_POOL CVI_VB_CreatePool(VB_POOL_CONFIG_S *pstVbPoolCfg)
{
	UNUSED(pstVbPoolCfg);
	return 0;
}

CVI_S32 CVI_VB_DestroyPool(VB_POOL Pool)
{
	UNUSED(Pool);
	return 0;
}
