#include "stdio.h"
#include "utils.h"
#include "json-c/json.h"

#define RTSP_JSON_PATH "cfg.json"
#define VC_PARAM_JSON_PATH "vc_param.json"

int main(void)
{

	int ret;

	RTSP_CFG rtsp_cfg;

	ret = init_rtsp_from_json(RTSP_JSON_PATH, &rtsp_cfg);

	deinit_rtsp_from_json(&rtsp_cfg);

	return ret;

	//printf("---------------------------------------test rstp conf-----------------------------------------\n");
	//RTSP_CFG rtsp_cfg;
	//if (init_rtsp_from_json(RTSP_JSON_PATH, &rtsp_cfg) == 0) {
	//	printf("init the rtsp from the json file ok!\n");
	//} else {
	//	printf("can not init the rtsp from the json file!\n");
	//	return -1;
	//}

	//print_rtsp_cfg(&rtsp_cfg);

	//printf("---------------------------------------test vc conf -----------------------------------------\n");
	//VC_CFG vc_cfg;

	//if (init_vc_from_json(VC_PARAM_JSON_PATH, &vc_cfg) == 0) {
	//	printf("init the vc from the file ok!\n");
	//} else {
	//	printf("can not init the vc from the file!\n");
	//	return -1;
	//}

	//print_vc_key_val(&vc_cfg);


	//return 0;
}
