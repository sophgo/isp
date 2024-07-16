#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/queue.h>
#include <pthread.h>
#include <stdatomic.h>
#include <inttypes.h>
#include <sys/mman.h>

#include <fcntl.h>		/* low-level i/o */
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include "cvi_base.h"
#include "cvi_sys.h"
#include "hashmap.h"
#include <base_uapi.h>
#include <sys_uapi.h>
#include "devmem.h"

#define UNUSED(x) ((void)(x))

static int devm_fd = -1, devm_cached_fd = -1;
static Hashmap *ionHashmap = NULL;
CVI_S32 *log_levels;
CVI_CHAR const *log_name[8] = {
	(CVI_CHAR *)"EMG", (CVI_CHAR *)"ALT", (CVI_CHAR *)"CRI", (CVI_CHAR *)"ERR",
	(CVI_CHAR *)"WRN", (CVI_CHAR *)"NOT", (CVI_CHAR *)"INF", (CVI_CHAR *)"DBG"
};

static int _ion_hash_key(void *key)
{
	return (((uintptr_t)key) >> 10);
}

static bool _ion_hash_equals(void *keyA, void *keyB)
{
	return (keyA == keyB);
}

CVI_S32 CVI_SYS_Bind(const MMF_CHN_S *pstSrcChn, const MMF_CHN_S *pstDestChn)
{
	UNUSED(pstSrcChn);
	UNUSED(pstDestChn);
	return 0;
}

CVI_S32 CVI_SYS_UnBind(const MMF_CHN_S *pstSrcChn, const MMF_CHN_S *pstDestChn)
{
	UNUSED(pstSrcChn);
	UNUSED(pstDestChn);
	return 0;
}

CVI_S32 CVI_SYS_GetBindbySrc(const MMF_CHN_S *pstSrcChn, MMF_BIND_DEST_S *pstBindDest)
{
	UNUSED(pstSrcChn);
	UNUSED(pstBindDest);
	return 0;
}

CVI_S32 CVI_SYS_GetChipId(CVI_U32 *pu32ChipId)
{
	int id = 26;
	*pu32ChipId = id;

	return 0;
}

CVI_S32 CVI_SYS_DevMem_Open(void)
{
	if (devm_fd < 0)
		devm_fd = devm_open();

	if (devm_cached_fd < 0)
		devm_cached_fd = devm_open_cached();

	if (devm_fd < 0 || devm_cached_fd < 0) {
		perror("devmem open failed\n");
		return CVI_FAILURE;
	}

	return CVI_SUCCESS;
}

CVI_S32 CVI_SYS_DevMem_Close(void)
{
	if (devm_fd < 0 || devm_cached_fd < 0)
		return CVI_SUCCESS;

	devm_close(devm_fd);
	devm_fd = -1;
	devm_close(devm_cached_fd);
	devm_cached_fd = -1;
	return CVI_SUCCESS;
}

void *CVI_SYS_Mmap(CVI_U64 u64PhyAddr, CVI_U32 u32Size)
{
	CVI_SYS_DevMem_Open();

	return devm_map(devm_fd, u64PhyAddr, u32Size);
}

void *CVI_SYS_MmapCache(CVI_U64 u64PhyAddr, CVI_U32 u32Size)
{
	CVI_SYS_DevMem_Open();

	void *addr = devm_map(devm_cached_fd, u64PhyAddr, u32Size);

	if (addr)
		CVI_SYS_IonInvalidateCache(u64PhyAddr, addr, u32Size);

	return addr;
}

CVI_S32 CVI_SYS_Munmap(void *pVirAddr, CVI_U32 u32Size)
{
	devm_unmap(pVirAddr, u32Size);
	return CVI_SUCCESS;
}

CVI_S32 ionMalloc(struct sys_ion_data *para)
{
	CVI_S32 fd = -1;
	CVI_S32 ret;

	if ((fd = get_base_fd()) == -1)
		return CVI_ERR_SYS_NOTREADY;


	ret = ioctl(fd, BASE_ION_ALLOC, para);
	if (ret) {
		printf("%s: %s, ioctl BASE_ION_ALLOC failed!\n", __FILE__, __func__);
		return ret;
	}

	return CVI_SUCCESS;
}

CVI_S32 ionFree(struct sys_ion_data *para)
{
	CVI_S32 fd = -1;
	CVI_S32 ret;

	if ((fd = get_base_fd()) == -1)
		return CVI_ERR_SYS_NOTREADY;

	ret = ioctl(fd, BASE_ION_FREE, para);
	if (ret) {
		printf("%s: %s, ioctl BASE_ION_FREE failed!\n", __FILE__, __func__);
		return ret;
	}

	return CVI_SUCCESS;
}

static CVI_S32 _SYS_IonAlloc(CVI_U64 *pu64PhyAddr, CVI_VOID **ppVirAddr,
			     CVI_U32 u32Len, CVI_BOOL cached, const CVI_CHAR *name)
{
	struct sys_ion_data *ion_data;

	if (!ionHashmap)
		ionHashmap = hashmapCreate(20, _ion_hash_key, _ion_hash_equals);

	ion_data = malloc(sizeof(*ion_data));
	ion_data->size = u32Len;
	ion_data->cached = cached;
	// Set buffer as "anonymous" when user is passing null pointer.
	if (name)
		strncpy((char *)(ion_data->name), name, MAX_ION_BUFFER_NAME);
	else
		strncpy((char *)(ion_data->name), "anonymous", MAX_ION_BUFFER_NAME);

	if (ionMalloc(ion_data) != CVI_SUCCESS) {
		free(ion_data);
		printf("%s: %s, alloc failed.\n", __FILE__, __func__);
		return CVI_ERR_SYS_NOMEM;
	}

	*pu64PhyAddr = ion_data->addr_p;
	hashmapPut(ionHashmap, (void *)(uintptr_t)*pu64PhyAddr, ion_data);

	if (ppVirAddr) {
		if (cached)
			*ppVirAddr = CVI_SYS_MmapCache(*pu64PhyAddr, u32Len);
		else
			*ppVirAddr = CVI_SYS_Mmap(*pu64PhyAddr, u32Len);
		if (*ppVirAddr == NULL) {
			hashmapGet(ionHashmap, (void *)(uintptr_t)*pu64PhyAddr);
			ionFree(ion_data);
			free(ion_data);
			printf("%s: %s, mmap failed.\n", __FILE__, __func__);
			return CVI_ERR_SYS_REMAPPING;
		}
	}
	return CVI_SUCCESS;
}

CVI_S32 CVI_SYS_IonAlloc_Cached(CVI_U64 *pu64PhyAddr, CVI_VOID **ppVirAddr,
				 const CVI_CHAR *strName, CVI_U32 u32Len)
{
	MOD_CHECK_NULL_PTR(CVI_ID_SYS, pu64PhyAddr);

	return _SYS_IonAlloc(pu64PhyAddr, ppVirAddr, u32Len, CVI_TRUE, strName);
}

CVI_S32 CVI_SYS_IonFree(CVI_U64 u64PhyAddr, CVI_VOID *pVirAddr)
{
	struct sys_ion_data *ion_data;

	if (ionHashmap == NULL) {
		printf("%s: %s, ion not alloc before.\n", __FILE__, __func__);
		return CVI_ERR_SYS_NOTREADY;
	}

	ion_data = hashmapGet(ionHashmap, (void *)(uintptr_t)u64PhyAddr);
	if (ion_data == NULL) {
		printf("%s: %s, u64PhyAddr(0x%"PRIx64") not found in ion.\n", __FILE__, __func__, u64PhyAddr);
		return CVI_ERR_SYS_ILLEGAL_PARAM;
	}
	hashmapRemove(ionHashmap, (void *)(uintptr_t)u64PhyAddr);
	if (pVirAddr)
		devm_unmap(pVirAddr, ion_data->size);
	ionFree(ion_data);
	free(ion_data);

	if (hashmapSize(ionHashmap) == 0) {
		hashmapFree(ionHashmap);
		ionHashmap = NULL;
	}

	return CVI_SUCCESS;
}

CVI_S32 CVI_SYS_IonFlushCache(CVI_U64 u64PhyAddr, CVI_VOID *pVirAddr, CVI_U32 u32Len)
{
	CVI_S32 fd = -1;
	CVI_S32 ret = CVI_SUCCESS;
	struct sys_cache_op cache_cfg;

	if ((fd = get_base_fd()) == -1)
		return CVI_ERR_SYS_NOTREADY;

	if (pVirAddr == NULL) {
		printf("%s: %s, pVirAddr Null.\n", __FILE__, __func__);
		return CVI_ERR_SYS_NULL_PTR;
	}

	cache_cfg.addr_p = u64PhyAddr;
	cache_cfg.addr_v = pVirAddr;
	cache_cfg.size = u32Len;

	ret = ioctl(fd, BASE_CACHE_FLUSH, &cache_cfg);
	if (ret < 0) {
		printf("%s: %s, ion flush err.\n", __FILE__, __func__);
		ret = CVI_ERR_SYS_NOTREADY;
	}
	return ret;
}

CVI_S32 CVI_SYS_IonInvalidateCache(CVI_U64 u64PhyAddr, CVI_VOID *pVirAddr, CVI_U32 u32Len)
{
	CVI_S32 fd = -1;
	CVI_S32 ret = CVI_SUCCESS;
	struct sys_cache_op cache_cfg;

	if ((fd = get_base_fd()) == -1)
		return CVI_ERR_SYS_NOTREADY;

	if (pVirAddr == NULL) {
		printf("%s: %s, pVirAddr Null.\n", __FILE__, __func__);
		return CVI_ERR_SYS_NULL_PTR;
	}

	cache_cfg.addr_p = u64PhyAddr;
	cache_cfg.addr_v = pVirAddr;
	cache_cfg.size = u32Len;

	ret = ioctl(fd, BASE_CACHE_INVLD, &cache_cfg);
	if (ret < 0) {
		printf("%s: %s, ion invalid err.\n", __FILE__, __func__);
		ret = CVI_ERR_SYS_NOTREADY;
	}
	return ret;
}

CVI_S32 CVI_SYS_SetVIVPSSMode(const VI_VPSS_MODE_S *pstVIVPSSMode)
{
	UNUSED(pstVIVPSSMode);
	return 0;
}

CVI_S32 CVI_SYS_GetVIVPSSMode(VI_VPSS_MODE_S *pstVIVPSSMode)
{
	if (pstVIVPSSMode == NULL) {
		return -1;
	}

	for (CVI_S32 i = 0; i < VI_MAX_PIPE_NUM; i++) {
		pstVIVPSSMode->aenMode[i] = VI_OFFLINE_VPSS_OFFLINE;
	}

	return 0;
}

const CVI_CHAR *CVI_SYS_GetModName(MOD_ID_E id)
{
	return CVI_GET_MOD_NAME(id);
}

CVI_S32 CVI_SYS_CDMACopy2D(const CVI_CDMA_2D_S *param)
{
	UNUSED(param);
	return 0;
}

CVI_S32 CVI_SYS_Init(void)
{
	CVI_S32 s32ret = CVI_SUCCESS;

	s32ret = base_dev_open();
	if (s32ret != CVI_SUCCESS) {
		printf("%s: %s, base_dev_open failed.\n", __FILE__, __func__);
		return CVI_ERR_SYS_NOTREADY;
	}

	if (CVI_SYS_DevMem_Open() != CVI_SUCCESS) {
		printf("%s: %s, devmem open failed.\n", __FILE__, __func__);
		return CVI_ERR_SYS_NOTREADY;
	}

	return s32ret;
}

CVI_S32 CVI_SYS_Exit(void)
{
	CVI_S32 s32ret = CVI_SUCCESS;

	s32ret = CVI_SYS_DevMem_Close();
	if (s32ret != CVI_SUCCESS) {
		printf("%s: %s, devmem close failed.\n", __FILE__, __func__);
		return CVI_ERR_SYS_NOTREADY;
	}

	base_dev_close();

	return s32ret;
}
