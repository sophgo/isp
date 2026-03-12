/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_utility.h
 * Description:
 *
 */

#ifndef _ISP_ALGO_UTILITY_H_
#define _ISP_ALGO_UTILITY_H_

#include "stdint.h"
#include "isp_algo_comm_inc.h"
#include "isp_algo_debug.h"

#ifndef MIN2
#define MIN2(a, b)\
	({\
		__typeof__(a) _a = (a);\
		__typeof__(b) _b = (b);\
		_a < _b ? _a : _b;\
	})
#endif

#ifndef MAX2
#define MAX2(a, b)\
({\
	__typeof__(a) _a = (a);\
	__typeof__(b) _b = (b);\
	_a > _b ? _a : _b;\
})

#endif

#ifndef MIN3
#define MIN3(a, b, c) (MIN2(MIN2(a, b), c))
#endif

#ifndef MAX3
#define MAX3(a, b, c) ({ MAX2(MAX2((a), (b)), (c)); })
#endif

#ifndef LIMIT_RANGE
#define LIMIT_RANGE(v, min, max) ({ MIN2(MAX2((v), (min)), max); })
#endif

#define ROUND(v)                                                                                                       \
	({                                                                                                             \
		typeof(v) _v = (v);                                                                                    \
		(_v > 0) ? _v + 0.5 : _v - 0.5;                                                                        \
	})

#ifndef ABS
#define ABS(a)        (((a) < 0) ? -(a) : (a))
#endif

#define ISP_ALGO_CREATE_RUNTIME(_instance, _type)\
({\
	if (_instance == NULL) {\
		_instance = (_type *)calloc(1, sizeof(_type));\
		ISP_ALGO_LOG_DEBUG("runtime(%p)\n", _instance);\
		if (_instance == NULL) {\
			ISP_ALGO_LOG_ERR("can't allocate memory(%p)\n", _instance);\
			ret = CVI_FAILURE;\
		} \
	} else {\
		ISP_ALGO_LOG_ERR("initialized(%p)\n", _instance);\
	} \
})

#define ISP_ALGO_RELEASE_MEMORY(x) do { \
	if (x) { \
		free(x); \
		x = 0; \
	} \
} while (0)

#define ISP_ALGO_CHECK_POINTER(addr)\
do {\
	if (addr == NULL)\
		return CVI_FAILURE;\
} while (0)

static inline uint64_t FindPower2(uint64_t x)
{
	unsigned int msb = 0;

	x >>= 1;
	while (x) {
		msb++;
		x >>= 1;
	}
	return msb;
}

int get_lut_slp(int lut_in_0, int lut_in_1, int lut_out_0, int lut_out_1, int SLOPE_F_BITS);
CVI_S32 get_iso_tbl(CVI_U32 **tbl);

#endif // _ISP_ALGO_UTILITY_H_
