#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"
#include "rtsp.h"
#include "venc.h"

#include "cvi_json.h"

#define UNUSED(x) ((void)(x))

#define GET_VC_KEY_VAL(p_vc_cfg, sub_cfg, key, sub_key)\
{\
	if (cvi_json_object_object_get_ex(json_obj, key, &json_val)) {\
		if (cvi_json_object_object_get_ex(json_val, "items", &items_val)) {\
			arr_len = cvi_json_object_array_length(items_val);\
			for (int i = 0; i < arr_len; ++i) {\
				item_obj = cvi_json_object_array_get_idx(items_val, i);\
				if (cvi_json_object_object_get_ex(item_obj, "key", &item_key_val)) {\
					key_val_str = cvi_json_object_get_string(item_key_val);\
					if (strcmp(key_val_str, #sub_key) == 0) {\
						if (cvi_json_object_object_get_ex(item_obj, "value", &value)) {\
							p_vc_cfg->sub_cfg.sub_key = cvi_json_object_get_int(value);\
						} \
					} \
				} \
			} \
		} \
	} \
} \

#define PR_VC_KEY_VAL(p_vc_cfg, sub_cfg, key, sub_key)\
{\
	printf(key": "#sub_key": %d\n", p_vc_cfg->sub_cfg.sub_key);\
} \


int get_json_object_from_file(const char *json_path, struct cvi_json_object **json_obj)
{
	FILE *fp = fopen(json_path, "r");

	if (!fp) {
		fprintf(stderr, "Error opening file.\n");
		return -1;
	}

	fseek(fp, 0, SEEK_END);

	unsigned long long file_size = ftell(fp);

	fseek(fp, 0, SEEK_SET);

	char *buffer = (char *)malloc(file_size + 1);

	if (!buffer) {
		fprintf(stderr, "Memory allocation error.\n");
		fclose(fp);
		return -1;
	}

	fread(buffer, 1, file_size, fp);
	fclose(fp);
	buffer[file_size] = '\0';

	*json_obj = cvi_json_tokener_parse(buffer);

	if (*json_obj == NULL) {
		fprintf(stderr, "Error parsing JSON.\n");
		free(buffer);
		return -1;
	}

	free(buffer);

	return 0;
}

int init_rtsp_from_json(const char *json_path, RTSP_CFG *p_rtsp_cfg)
{
	struct cvi_json_object *json_obj = NULL;

	if (get_json_object_from_file(json_path, &json_obj) != 0) {
		printf("parse rstp json fail!\n");
		return -1;
	}

	memset(p_rtsp_cfg, 0, sizeof(RTSP_CFG));
	struct cvi_json_object *val_json_object = cvi_json_object_new_object();

	if (cvi_json_object_object_get_ex(json_obj, "dev-num", &val_json_object)) {
		p_rtsp_cfg->dev_num = cvi_json_object_get_int(val_json_object);
	}

	if (cvi_json_object_object_get_ex(json_obj, "rtsp-port", &val_json_object)) {
		p_rtsp_cfg->rtsp_port = cvi_json_object_get_int(val_json_object);
	}

	if (cvi_json_object_object_get_ex(json_obj, "rtsp-max-buf-size", &val_json_object)) {
		p_rtsp_cfg->rtsp_max_buf_size = cvi_json_object_get_int(val_json_object);
	}

	// video src info
	printf("run the video_src_info\n");
	if (cvi_json_object_object_get_ex(json_obj, "video-src-info", &val_json_object)) {
		int ch_num = cvi_json_object_array_length(val_json_object);

		if (p_rtsp_cfg->dev_num > ch_num) {
			printf("rtsp cfg error: dev_num must greater than video src info array length!\n");
			return -1;
		}

		struct cvi_json_object *array_ele = NULL;
		struct cvi_json_object *arr_val_json_object = NULL;

		p_rtsp_cfg->pa_video_src_cfg = (VIDEO_SRC_CFG *)calloc(p_rtsp_cfg->dev_num, sizeof(VIDEO_SRC_CFG));
		if (p_rtsp_cfg->pa_video_src_cfg == NULL) {
			printf("%s, %s, %d, memory allocation failed.\n", __FILE__, __func__, __LINE__);
			return -1;
		}

		for (int i = 0; i < p_rtsp_cfg->dev_num; ++i) {
			array_ele = cvi_json_object_array_get_idx(val_json_object, i);
			p_rtsp_cfg->pa_video_src_cfg[i].chn = i;

			if (cvi_json_object_object_get_ex(array_ele, "buf-blk-cnt", &arr_val_json_object)) {
				p_rtsp_cfg->pa_video_src_cfg[i].buf_blk_cnt = cvi_json_object_get_int(arr_val_json_object);
			}

			if (cvi_json_object_object_get_ex(array_ele, "is_wdr_mode", &arr_val_json_object)) {
				const char *tmp_str = cvi_json_object_get_string(arr_val_json_object);

				if (strcmp(tmp_str, "true") == 0) {
					p_rtsp_cfg->pa_video_src_cfg[i].is_wdr_mode = 1;
				} else {
					p_rtsp_cfg->pa_video_src_cfg[i].is_wdr_mode = 0;
				}
			}

			if (cvi_json_object_object_get_ex(array_ele, "enable-hdmi", &arr_val_json_object)) {
				const char *tmp_str = cvi_json_object_get_string(arr_val_json_object);

				if (strcmp(tmp_str, "true") == 0) {
					p_rtsp_cfg->pa_video_src_cfg[i].enable_hdmi = 1;
				} else {
					p_rtsp_cfg->pa_video_src_cfg[i].enable_hdmi = 0;
				}
			}

			if (cvi_json_object_object_get_ex(array_ele, "enable-teaisp-bnr", &arr_val_json_object)) {
				const char *tmp_str = cvi_json_object_get_string(arr_val_json_object);

				if (strcmp(tmp_str, "true") == 0) {
					p_rtsp_cfg->pa_video_src_cfg[i].enable_teaisp_bnr = 1;
				} else {
					p_rtsp_cfg->pa_video_src_cfg[i].enable_teaisp_bnr = 0;
				}
			}

			if (cvi_json_object_object_get_ex(array_ele, "teaisp_model_list", &arr_val_json_object)) {
				const char *tmp_str = cvi_json_object_get_string(arr_val_json_object);

				snprintf(p_rtsp_cfg->pa_video_src_cfg[i].bnr_model_list, MAX_BNR_MODEL_LIST_PATH_LEN,
				 "%s",  tmp_str);
			}

			if (cvi_json_object_object_get_ex(array_ele, "venc_json", &arr_val_json_object)) {
				const char *tmp_str = cvi_json_object_get_string(arr_val_json_object);

				snprintf(p_rtsp_cfg->pa_video_src_cfg[i].venc_json, MAX_VC_JSON_PATH_LEN,
				 "%s",  tmp_str);
			}

			if (cvi_json_object_object_get_ex(array_ele, "codec", &arr_val_json_object)) {
				const char *tmp_str = cvi_json_object_get_string(arr_val_json_object);

				snprintf(p_rtsp_cfg->pa_video_src_cfg[i].codec, MAX_CODEC_LEN, "%s", tmp_str);
			}

			if (cvi_json_object_object_get_ex(array_ele, "gop", &arr_val_json_object)) {
				p_rtsp_cfg->pa_video_src_cfg[i].gop = cvi_json_object_get_int(arr_val_json_object);
			}

			if (cvi_json_object_object_get_ex(array_ele, "bitrate", &arr_val_json_object)) {
				p_rtsp_cfg->pa_video_src_cfg[i].bitrate = cvi_json_object_get_int(arr_val_json_object);
			}

			if (cvi_json_object_object_get_ex(array_ele, "compress-mode", &arr_val_json_object)) {
				const char *tmp_str = cvi_json_object_get_string(arr_val_json_object);

				snprintf(p_rtsp_cfg->pa_video_src_cfg[i].compress_mode, MAX_COMPRESS_MODE_LEN, "%s", tmp_str);
			}
		}
		print_rtsp_cfg(p_rtsp_cfg);

		// get the vc param
		for (int i = 0; i < p_rtsp_cfg->dev_num; ++i) {
			init_vc_from_json(p_rtsp_cfg->pa_video_src_cfg[i].venc_json, &p_rtsp_cfg->pa_video_src_cfg[i].st_vc_cfg);
			printf(">>> ch %d vc cfg <<<\n", i);
			print_vc_key_val(&p_rtsp_cfg->pa_video_src_cfg[i].st_vc_cfg);
		}

	}

	// free
	cvi_json_object_put(json_obj);

	return 0;
}

void deinit_rtsp_from_json(RTSP_CFG *p_rtsp_cfg)
{
	if (p_rtsp_cfg->pa_video_src_cfg != NULL) {
		free(p_rtsp_cfg->pa_video_src_cfg);
	}
}

int print_rtsp_cfg(RTSP_CFG *p_rtsp_cfg)
{
	printf("-------------------------------------- rtsp Param --------------------------------------\n");

	printf("dev-num:%d\n", p_rtsp_cfg->dev_num);
	printf("rtsp-port:%d\n", p_rtsp_cfg->rtsp_port);
	printf("rtsp-max-buf-size:%llu\n", p_rtsp_cfg->rtsp_max_buf_size);

	for (int i = 0; i < p_rtsp_cfg->dev_num; ++i) {
		printf("------------------------chn: %d, video src info--------------------------\n", i);
		printf("chn: %d\n", p_rtsp_cfg->pa_video_src_cfg[i].chn);
		printf("buf-blk-cnt: %d\n", p_rtsp_cfg->pa_video_src_cfg[i].buf_blk_cnt);
		printf("is_wdr_mode: %d\n", p_rtsp_cfg->pa_video_src_cfg[i].is_wdr_mode);
		printf("enable-hmdi: %d\n", p_rtsp_cfg->pa_video_src_cfg[i].enable_hdmi);
		printf("venc_json: %s\n", p_rtsp_cfg->pa_video_src_cfg[i].venc_json);
		printf("codec: %s\n", p_rtsp_cfg->pa_video_src_cfg[i].codec);
		printf("gop: %d\n", p_rtsp_cfg->pa_video_src_cfg[i].gop);
		printf("bitrate: %d\n", p_rtsp_cfg->pa_video_src_cfg[i].bitrate);
		printf("compress-mode: %s\n", p_rtsp_cfg->pa_video_src_cfg[i].compress_mode);
	}

	return 0;
}

int init_vc_from_json(const char *json_path, VC_CFG *p_vc_cfg)
{
	struct cvi_json_object *json_obj = NULL;
	struct cvi_json_object *json_val = NULL;
	struct cvi_json_object *items_val = NULL;
	struct cvi_json_object *item_obj = NULL;
	struct cvi_json_object *item_key_val = NULL;
	struct cvi_json_object *value = NULL;
	const char *key_val_str;
	int arr_len = 0;

	if (get_json_object_from_file(json_path, &json_obj) != 0) {
		printf("parse rstp json fail!\n");
		return -1;
	}

	memset(p_vc_cfg, 0, sizeof(VC_CFG));

	// Coding Param
	GET_VC_KEY_VAL(p_vc_cfg, st_coding_param, "Coding Param", FrmLostOpen);
	GET_VC_KEY_VAL(p_vc_cfg, st_coding_param, "Coding Param", LostMode);
	GET_VC_KEY_VAL(p_vc_cfg, st_coding_param, "Coding Param", FrmLostBpsThr);
	GET_VC_KEY_VAL(p_vc_cfg, st_coding_param, "Coding Param", EncFrmGaps);
	GET_VC_KEY_VAL(p_vc_cfg, st_coding_param, "Coding Param", IntraCost);
	GET_VC_KEY_VAL(p_vc_cfg, st_coding_param, "Coding Param", aspectRatioInfoPresentFlag);
	GET_VC_KEY_VAL(p_vc_cfg, st_coding_param, "Coding Param", overscanInfoPresentFlag);
	GET_VC_KEY_VAL(p_vc_cfg, st_coding_param, "Coding Param", videoSignalTypePresentFlag);
	GET_VC_KEY_VAL(p_vc_cfg, st_coding_param, "Coding Param", videoFormat);
	GET_VC_KEY_VAL(p_vc_cfg, st_coding_param, "Coding Param", videoFullRangeFlag);
	GET_VC_KEY_VAL(p_vc_cfg, st_coding_param, "Coding Param", colourDescriptionPresentFlag);
	GET_VC_KEY_VAL(p_vc_cfg, st_coding_param, "Coding Param", ChromaQpOffset);
	GET_VC_KEY_VAL(p_vc_cfg, st_coding_param, "Coding Param", CbQpOffset);
	GET_VC_KEY_VAL(p_vc_cfg, st_coding_param, "Coding Param", CrQpOffset);
	// Gop Mode
	GET_VC_KEY_VAL(p_vc_cfg, st_gop_mode, "Gop Mode", GopMode);
	GET_VC_KEY_VAL(p_vc_cfg, st_gop_mode, "Gop Mode", IPQpDelta);
	GET_VC_KEY_VAL(p_vc_cfg, st_gop_mode, "Gop Mode", BgInterval);
	GET_VC_KEY_VAL(p_vc_cfg, st_gop_mode, "Gop Mode", BgQpDelta);
	GET_VC_KEY_VAL(p_vc_cfg, st_gop_mode, "Gop Mode", ViQpDelta);
	// RC Attr
	GET_VC_KEY_VAL(p_vc_cfg, st_rc_attr, "RC Attr", RcMode);
	GET_VC_KEY_VAL(p_vc_cfg, st_rc_attr, "RC Attr", Gop);
	GET_VC_KEY_VAL(p_vc_cfg, st_rc_attr, "RC Attr", VariableFPS);
	GET_VC_KEY_VAL(p_vc_cfg, st_rc_attr, "RC Attr", SrcFrmRate);
	GET_VC_KEY_VAL(p_vc_cfg, st_rc_attr, "RC Attr", DstFrmRate);
	GET_VC_KEY_VAL(p_vc_cfg, st_rc_attr, "RC Attr", StatTime);
	GET_VC_KEY_VAL(p_vc_cfg, st_rc_attr, "RC Attr", BitRate);
	GET_VC_KEY_VAL(p_vc_cfg, st_rc_attr, "RC Attr", MaxBitrate);
	GET_VC_KEY_VAL(p_vc_cfg, st_rc_attr, "RC Attr", IQP);
	GET_VC_KEY_VAL(p_vc_cfg, st_rc_attr, "RC Attr", PQP);
	// RC Param
	GET_VC_KEY_VAL(p_vc_cfg, st_rc_param, "RC Param", ThrdLv);
	GET_VC_KEY_VAL(p_vc_cfg, st_rc_param, "RC Param", FirstFrameStartQp);
	GET_VC_KEY_VAL(p_vc_cfg, st_rc_param, "RC Param", InitialDelay);
	GET_VC_KEY_VAL(p_vc_cfg, st_rc_param, "RC Param", MaxQp);
	GET_VC_KEY_VAL(p_vc_cfg, st_rc_param, "RC Param", MinQp);
	GET_VC_KEY_VAL(p_vc_cfg, st_rc_param, "RC Param", MaxIQp);
	GET_VC_KEY_VAL(p_vc_cfg, st_rc_param, "RC Param", MinIQp);
	GET_VC_KEY_VAL(p_vc_cfg, st_rc_param, "RC Param", ChangePos);
	GET_VC_KEY_VAL(p_vc_cfg, st_rc_param, "RC Param", MinStillPercent);
	GET_VC_KEY_VAL(p_vc_cfg, st_rc_param, "RC Param", MaxStillQP);
	GET_VC_KEY_VAL(p_vc_cfg, st_rc_param, "RC Param", MotionSensitivity);
	GET_VC_KEY_VAL(p_vc_cfg, st_rc_param, "RC Param", PureStillThr);
	GET_VC_KEY_VAL(p_vc_cfg, st_rc_param, "RC Param", AvbrFrmLostOpen);
	GET_VC_KEY_VAL(p_vc_cfg, st_rc_param, "RC Param", AvbrFrmGap);

	// free
	cvi_json_object_put(json_obj);

	return 0;
}

int print_vc_key_val(VC_CFG *p_vc_cfg)
{
	printf("-------------------------------------- VC Param --------------------------------------\n");
	// Coding Param
	PR_VC_KEY_VAL(p_vc_cfg, st_coding_param, "Coding Param", FrmLostOpen);
	PR_VC_KEY_VAL(p_vc_cfg, st_coding_param, "Coding Param", LostMode);
	PR_VC_KEY_VAL(p_vc_cfg, st_coding_param, "Coding Param", FrmLostBpsThr);
	PR_VC_KEY_VAL(p_vc_cfg, st_coding_param, "Coding Param", EncFrmGaps);
	PR_VC_KEY_VAL(p_vc_cfg, st_coding_param, "Coding Param", IntraCost);
	PR_VC_KEY_VAL(p_vc_cfg, st_coding_param, "Coding Param", aspectRatioInfoPresentFlag);
	PR_VC_KEY_VAL(p_vc_cfg, st_coding_param, "Coding Param", overscanInfoPresentFlag);
	PR_VC_KEY_VAL(p_vc_cfg, st_coding_param, "Coding Param", videoSignalTypePresentFlag);
	PR_VC_KEY_VAL(p_vc_cfg, st_coding_param, "Coding Param", videoFormat);
	PR_VC_KEY_VAL(p_vc_cfg, st_coding_param, "Coding Param", videoFullRangeFlag);
	PR_VC_KEY_VAL(p_vc_cfg, st_coding_param, "Coding Param", colourDescriptionPresentFlag);
	PR_VC_KEY_VAL(p_vc_cfg, st_coding_param, "Coding Param", ChromaQpOffset);
	PR_VC_KEY_VAL(p_vc_cfg, st_coding_param, "Coding Param", CbQpOffset);
	PR_VC_KEY_VAL(p_vc_cfg, st_coding_param, "Coding Param", CrQpOffset);
	// Gop Mode
	PR_VC_KEY_VAL(p_vc_cfg, st_gop_mode, "Gop Mode", GopMode);
	PR_VC_KEY_VAL(p_vc_cfg, st_gop_mode, "Gop Mode", IPQpDelta);
	PR_VC_KEY_VAL(p_vc_cfg, st_gop_mode, "Gop Mode", BgInterval);
	PR_VC_KEY_VAL(p_vc_cfg, st_gop_mode, "Gop Mode", BgQpDelta);
	PR_VC_KEY_VAL(p_vc_cfg, st_gop_mode, "Gop Mode", ViQpDelta);
	// RC Attr
	PR_VC_KEY_VAL(p_vc_cfg, st_rc_attr, "RC Attr", RcMode);
	PR_VC_KEY_VAL(p_vc_cfg, st_rc_attr, "RC Attr", Gop);
	PR_VC_KEY_VAL(p_vc_cfg, st_rc_attr, "RC Attr", VariableFPS);
	PR_VC_KEY_VAL(p_vc_cfg, st_rc_attr, "RC Attr", SrcFrmRate);
	PR_VC_KEY_VAL(p_vc_cfg, st_rc_attr, "RC Attr", DstFrmRate);
	PR_VC_KEY_VAL(p_vc_cfg, st_rc_attr, "RC Attr", StatTime);
	PR_VC_KEY_VAL(p_vc_cfg, st_rc_attr, "RC Attr", BitRate);
	PR_VC_KEY_VAL(p_vc_cfg, st_rc_attr, "RC Attr", MaxBitrate);
	PR_VC_KEY_VAL(p_vc_cfg, st_rc_attr, "RC Attr", IQP);
	PR_VC_KEY_VAL(p_vc_cfg, st_rc_attr, "RC Attr", PQP);
	// RC Param
	PR_VC_KEY_VAL(p_vc_cfg, st_rc_param, "RC Param", ThrdLv);
	PR_VC_KEY_VAL(p_vc_cfg, st_rc_param, "RC Param", FirstFrameStartQp);
	PR_VC_KEY_VAL(p_vc_cfg, st_rc_param, "RC Param", InitialDelay);
	PR_VC_KEY_VAL(p_vc_cfg, st_rc_param, "RC Param", MaxQp);
	PR_VC_KEY_VAL(p_vc_cfg, st_rc_param, "RC Param", MinQp);
	PR_VC_KEY_VAL(p_vc_cfg, st_rc_param, "RC Param", MaxIQp);
	PR_VC_KEY_VAL(p_vc_cfg, st_rc_param, "RC Param", MinIQp);
	PR_VC_KEY_VAL(p_vc_cfg, st_rc_param, "RC Param", ChangePos);
	PR_VC_KEY_VAL(p_vc_cfg, st_rc_param, "RC Param", MinStillPercent);
	PR_VC_KEY_VAL(p_vc_cfg, st_rc_param, "RC Param", MaxStillQP);
	PR_VC_KEY_VAL(p_vc_cfg, st_rc_param, "RC Param", MotionSensitivity);
	PR_VC_KEY_VAL(p_vc_cfg, st_rc_param, "RC Param", PureStillThr);
	PR_VC_KEY_VAL(p_vc_cfg, st_rc_param, "RC Param", AvbrFrmLostOpen);
	PR_VC_KEY_VAL(p_vc_cfg, st_rc_param, "RC Param", AvbrFrmGap);

	return 0;
}
