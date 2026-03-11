
#ifndef _AF_DEBUG_H_
#define _AF_DEBUG_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define CLOG_TAG "af"
#define CLOG_OUTPUT_LVL CLOG_LVL_INFO
//#define CLOG_OUTPUT_LVL CLOG_LVL_DEBUG
//#define CLOG_OUTPUT_LVL CLOG_LVL_VERBOSE
#include "clog.h"

#define af_loge(...) clog_error(CLOG_TAG, __VA_ARGS__)
#define af_logw(...) clog_warn(CLOG_TAG, __VA_ARGS__)
#define af_logi(...) clog_info(CLOG_TAG, __VA_ARGS__)
#define af_logd(...) clog_debug(CLOG_TAG, __VA_ARGS__)
#define af_logv(...) clog_verbose(CLOG_TAG, __VA_ARGS__)
#define af_logr(...) clog_output_raw(__VA_ARGS__)

typedef enum _AF_DBG_MODE {
	AF_DBG_DISABLE,
	AF_CALIB_CLOW,
} AF_DBG_MODE;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _AF_DEBUG_H_

