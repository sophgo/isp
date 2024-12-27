#ifndef _ISP_TOOL_DAEMON_V4L2_UTILS_H_
#define _ISP_TOOL_DAEMON_V4L2_UTILS_H_

#include "cvi_json.h"

#define MAX_BNR_MODEL_LIST_PATH_LEN 128
#define MAX_VC_JSON_PATH_LEN 128
#define MAX_CODEC_LEN 16
#define MAX_COMPRESS_MODE_LEN 16

typedef struct {
	int FrmLostOpen;
	int LostMode;
	int FrmLostBpsThr;
	int EncFrmGaps;
	int IntraCost;
	int aspectRatioInfoPresentFlag;
	int overscanInfoPresentFlag;
	int videoSignalTypePresentFlag;
	int videoFormat;
	int videoFullRangeFlag;
	int colourDescriptionPresentFlag;
	int ChromaQpOffset;
	int CbQpOffset;
	int CrQpOffset;
} CODING_PARAM;

typedef struct {
	int GopMode;
	int IPQpDelta;
	int BgInterval;
	int BgQpDelta;
	int ViQpDelta;
} GOP_MODE;

typedef struct {
	int RcMode;
	int Gop;
	int VariableFPS;
	int SrcFrmRate;
	int DstFrmRate;
	int StatTime;
	int BitRate;
	int MaxBitrate;
	int IQP;
	int PQP;
} RC_ATTR;

typedef struct {
	int ThrdLv;
	int FirstFrameStartQp;
	int InitialDelay;
	int MaxQp;
	int MinQp;
	int MaxIQp;
	int MinIQp;
	int ChangePos;
	int MinStillPercent;
	int MaxStillQP;
	int MotionSensitivity;
	int PureStillThr;
	int AvbrFrmLostOpen;
	int AvbrFrmGap;
} RC_PARAM;

typedef struct {
	CODING_PARAM st_coding_param;
	GOP_MODE st_gop_mode;
	RC_ATTR st_rc_attr;
	RC_PARAM st_rc_param;
} VC_CFG;

typedef struct {
	int chn;
	int buf_blk_cnt;
	int is_wdr_mode;
	int enable_hdmi;
	int enable_teaisp_bnr;
	char bnr_model_list[MAX_BNR_MODEL_LIST_PATH_LEN];
	char venc_json[MAX_VC_JSON_PATH_LEN];
	char codec[MAX_CODEC_LEN];
	int gop;
	int bitrate;
	char compress_mode[MAX_COMPRESS_MODE_LEN];
	VC_CFG st_vc_cfg;
} VIDEO_SRC_CFG;

typedef struct {
	int dev_num;
	int rtsp_port;
	unsigned long long rtsp_max_buf_size;
	VIDEO_SRC_CFG *pa_video_src_cfg;
} RTSP_CFG;

int get_json_object_from_file(const char *json_path, struct cvi_json_object **json_obj);

int init_rtsp_from_json(const char *json_path, RTSP_CFG *p_rtsp_cfg);
void deinit_rtsp_from_json(RTSP_CFG *p_rtsp_cfg);
int print_rtsp_cfg(RTSP_CFG *p_rtsp_cfg);

int init_vc_from_json(const char *json_path, VC_CFG *p_vc_cfg);
int print_vc_key_val(VC_CFG *p_vc_cfg);

#endif
