
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/time.h>
#include <pthread.h>
#include <fcntl.h>

#include <linux/videodev2.h>
#include <sns_v4l2_uapi.h>
#include <sys/ioctl.h>

#include "cvi_bin.h"
#include "cvi_awb.h"
#include "cvi_af.h"

#include "cvi_sns_ctrl.h"
#include "cvi_ae.h"
#include "cvi_isp.h"
#include "cvi_isp_v4l2.h"
#include "cvi_vi.h"
#include "cvi_sys.h"

static pthread_t g_IspPid[VI_MAX_DEV_NUM];

static int open_v4l2_sensor(int pipe)
{
	char devicename[64];
	int sns_fd;
	int index = 2 + pipe;

	sprintf(devicename, "/dev/v4l-subdev%d", index);

	sns_fd = open(devicename, O_RDWR | O_NONBLOCK);

	return sns_fd;
}

void *get_sensor_obj(int pipe)
{
	ISP_SNS_OBJ_S *pstSnsObj = NULL;
	int sns_fd = open_v4l2_sensor(pipe);

	if (sns_fd < 0) {
		printf("open pipe %d sensor fail!\n", pipe);
		return pstSnsObj;
	}

	int sns_type;

	if (ioctl(sns_fd, SNS_V4L2_GET_TYPE, &sns_type) < 0) {
		printf("get sensor type fail !\n");
		return pstSnsObj;
	} else {
		printf("pipe: %d, sensor type: %d\n", pipe, sns_type);
	}

	close(sns_fd);

	switch (sns_type) {
#if defined(SENSOR_BRIGATES_BG0808)
	case V4L2_BRIGATES_BG0808_MIPI_2M_30FPS_10BIT:
	case V4L2_BRIGATES_BG0808_MIPI_2M_30FPS_10BIT_WDR2TO1:
		pstSnsObj = &stSnsBG0808_Obj;
		break;
#endif
#if defined(SENSOR_GCORE_GC02M1)
	case V4L2_GCORE_GC02M1_MIPI_2M_30FPS_10BIT:
		pstSnsObj =  &stSnsGc02m1_Obj;
		break;
#endif
#if defined(SENSOR_GCORE_GC1054)
	case V4L2_GCORE_GC1054_MIPI_1M_30FPS_10BIT:
		pstSnsObj = &stSnsGc1054_Obj;
		break;
#endif
#if defined(SENSOR_GCORE_GC2053)
	case V4L2_GCORE_GC2053_MIPI_2M_30FPS_10BIT:
		pstSnsObj = &stSnsGc2053_Obj;
		break;
#endif
#if defined(SENSOR_GCORE_GC2053_SLAVE)
	case V4L2_GCORE_GC2053_SLAVE_MIPI_2M_30FPS_10BIT:
		pstSnsObj = &stSnsGc2053_Slave_Obj;
		break;
#endif
#if defined(SENSOR_GCORE_GC2053_1L)
	case V4L2_GCORE_GC2053_1L_MIPI_2M_30FPS_10BIT:
		pstSnsObj = &stSnsGc2053_1l_Obj;
		break;
#endif
#if defined(SENSOR_GCORE_GC2093)
	case V4L2_GCORE_GC2093_MIPI_2M_30FPS_10BIT:
	case V4L2_GCORE_GC2093_MIPI_2M_30FPS_10BIT_WDR2TO1:
		pstSnsObj = &stSnsGc2093_Obj;
		break;
#endif
#if defined(SENSOR_GCORE_GC2093_SLAVE)
	case V4L2_GCORE_GC2093_SLAVE_MIPI_2M_30FPS_10BIT:
	case V4L2_GCORE_GC2093_SLAVE_MIPI_2M_30FPS_10BIT_WDR2TO1:
		pstSnsObj = &stSnsGc2093_Slave_Obj;
		break;
#endif
#if defined(SENSOR_GCORE_GC2145)
	case V4L2_GCORE_GC2145_MIPI_2M_12FPS_8BIT:
		pstSnsObj = &stSnsGc2145_Obj;
		break;
#endif
#if defined(SENSOR_GCORE_GC4023)
	case V4L2_GCORE_GC4023_MIPI_4M_30FPS_10BIT:
		pstSnsObj = &stSnsGc4023_Obj;
		break;
#endif
#if defined(SENSOR_GCORE_GC4653)
	case V4L2_GCORE_GC4653_MIPI_4M_30FPS_10BIT:
		pstSnsObj = &stSnsGc4653_Obj;
		break;
#endif
#if defined(SENSOR_GCORE_GC4653_SLAVE)
	case V4L2_GCORE_GC4653_SLAVE_MIPI_4M_30FPS_10BIT:
		pstSnsObj = &stSnsGc4653_Slave_Obj;
		break;
#endif
#if defined(SENSOR_NEXTCHIP_N5)
	case V4L2_NEXTCHIP_N5_2M_25FPS_8BIT:
	case V4L2_NEXTCHIP_N5_1M_2CH_25FPS_8BIT:
		pstSnsObj = &stSnsN5_Obj;
		break;
#endif
#if defined(SENSOR_NEXTCHIP_N6)
	case V4L2_NEXTCHIP_N6_2M_4CH_25FPS_8BIT:
		pstSnsObj = &stSnsN6_Obj;
		break;
#endif

#if defined(SENSOR_ONSEMI_AR2020)
	case V4L2_ONSEMI_AR2020_20M_25FPS_10BIT:
		pstSnsObj = &stSnsAR2020_Obj;
		break;
#endif

#if defined(SENSOR_OV_OS02D10)
	case V4L2_OV_OS02D10_MIPI_2M_30FPS_10BIT:
		pstSnsObj = &stSnsOs02d10_Obj;
		break;
#endif
#if defined(SENSOR_OV_OS02D10_SLAVE)
	case V4L2_OV_OS02D10_SLAVE_MIPI_2M_30FPS_10BIT:
		pstSnsObj = &stSnsOs02d10_Slave_Obj;
		break;
#endif
#if defined(SENSOR_OV_OS02K10_SLAVE)
	case V4L2_OV_OS02K10_SLAVE_MIPI_2M_30FPS_12BIT:
		pstSnsObj = &stSnsOs02k10_Slave_Obj;
		break;
#endif
#if defined(SENSOR_OV_OS04A10)
	case V4L2_OV_OS04A10_MIPI_4M_1440P_30FPS_12BIT:
	case V4L2_OV_OS04A10_MIPI_4M_1440P_2L_10BIT:
	case V4L2_OV_OS04A10_MIPI_4M_1440P_30FPS_10BIT_WDR2TO1:
	case V4L2_OV_OS04A10_MASTER_MIPI_4M_1440P_2L_10BIT:
	case V4L2_OV_OS04A10_SLAVE_MIPI_4M_1440P_2L_10BIT:
	case V4L2_OV_OS04A10_MASTER_MIPI_4M_1440P_2L_10BIT_WDR2TO1:
	case V4L2_OV_OS04A10_SLAVE_MIPI_4M_1440P_2L_10BIT_WDR2TO1:
		pstSnsObj = &stSnsOs04a10_Obj;
		break;
#endif
#if defined(SENSOR_OV_OS04C10)
	case V4L2_OV_OS04C10_MIPI_4M_30FPS_12BIT:
	case V4L2_OV_OS04C10_MIPI_4M_1440P_30FPS_12BIT:
	case V4L2_OV_OS04C10_MIPI_4M_30FPS_10BIT_WDR2TO1:
	case V4L2_OV_OS04C10_MIPI_4M_1440P_30FPS_10BIT_WDR2TO1:
		pstSnsObj = &stSnsOs04c10_Obj;
		break;
#endif
#if defined(SENSOR_OV_OS04C10_SLAVE)
	case V4L2_OV_OS04C10_SLAVE_MIPI_4M_30FPS_12BIT:
	case V4L2_OV_OS04C10_SLAVE_MIPI_4M_30FPS_10BIT_WDR2TO1:
		pstSnsObj = &stSnsOs04c10_Slave_Obj;
		break;
#endif
#if defined(SENSOR_OV_OS04E10)
	case V4L2_OV_OS04E10_MIPI_4M_30FPS_12BIT:
	case V4L2_OV_OS04E10_MIPI_4M_30FPS_2L_10BIT:
	case V4L2_OV_OS04E10_SALVE_MIPI_4M_30FPS_2L_10BIT:
	case V4L2_OV_OS04E10_MIPI_4M_30FPS_10BIT_WDR2TO1:
	case V4L2_OV_OS04E10_MIPI_4M_30FPS_2L_10BIT_WDR2TO1:
	case V4L2_OV_OS04E10_SLAVE_MIPI_4M_30FPS_2L_10BIT_WDR2TO1:
		pstSnsObj = &stSnsOs04e10_Obj;
		break;
#endif
#if defined(SENSOR_OV_OS08B10)
	case V4L2_OV_OS08B10_MIPI_8M_30FPS_10BIT:
	case V4L2_OV_OS08B10_MIPI_8M_30FPS_10BIT_WDR2TO1:
		pstSnsObj = &stSnsOs08b10_Obj;
		break;
#endif
#if defined(SENSOR_OV_OS08A20)
	case V4L2_OV_OS08A20_MIPI_4M_30FPS_10BIT:
	case V4L2_OV_OS08A20_MIPI_4M_30FPS_10BIT_WDR2TO1:
	case V4L2_OV_OS08A20_MIPI_5M_30FPS_10BIT:
	case V4L2_OV_OS08A20_MIPI_5M_30FPS_10BIT_WDR2TO1:
	case V4L2_OV_OS08A20_MIPI_8M_30FPS_10BIT:
	case V4L2_OV_OS08A20_MIPI_8M_30FPS_10BIT_WDR2TO1:
		pstSnsObj = &stSnsOs08a20_Obj;
		break;
#endif
#if defined(SENSOR_OV_OS08A20_SLAVE)
	case V4L2_OV_OS08A20_SLAVE_MIPI_4M_30FPS_10BIT:
	case V4L2_OV_OS08A20_SLAVE_MIPI_4M_30FPS_10BIT_WDR2TO1:
	case V4L2_OV_OS08A20_SLAVE_MIPI_5M_30FPS_10BIT:
	case V4L2_OV_OS08A20_SLAVE_MIPI_5M_30FPS_10BIT_WDR2TO1:
	case V4L2_OV_OS08A20_SLAVE_MIPI_8M_30FPS_10BIT:
	case V4L2_OV_OS08A20_SLAVE_MIPI_8M_30FPS_10BIT_WDR2TO1:
		pstSnsObj = &stSnsOs08a20_Slave_Obj;
		break;
#endif
#if defined(SENSOR_OV_OV4689)
	case V4L2_OV_OV4689_MIPI_4M_30FPS_10BIT:
		pstSnsObj = &stSnsOv4689_Obj;
		break;
#endif
#if defined(SENSOR_OV_OV6211)
	case V4L2_OV_OV6211_MIPI_400P_120FPS_10BIT:
		pstSnsObj = &stSnsOv6211_Obj;
		break;
#endif
#if defined(SENSOR_OV_OV7251)
	case V4L2_OV_OV7251_MIPI_480P_120FPS_10BIT:
		pstSnsObj = &stSnsOv7251_Obj;
		break;
#endif
#if defined(SENSOR_PICO_384)
	case V4L2_PICO384_THERMAL_384X288:
		pstSnsObj = &stSnsPICO384_Obj;
		break;
#endif
#if defined(SENSOR_PICO_640)
	case V4L2_PICO640_THERMAL_479P:
		pstSnsObj = &stSnsPICO640_Obj;
		break;
#endif
#if defined(SENSOR_PIXELPLUS_PR2020)
	case V4L2_PIXELPLUS_PR2020_1M_25FPS_8BIT:
	case V4L2_PIXELPLUS_PR2020_1M_30FPS_8BIT:
	case V4L2_PIXELPLUS_PR2020_2M_25FPS_8BIT:
	case V4L2_PIXELPLUS_PR2020_2M_30FPS_8BIT:
		pstSnsObj = &stSnsPR2020_Obj;
		break;
#endif
#if defined(SENSOR_PIXELPLUS_PR2100)
	case V4L2_PIXELPLUS_PR2100_2M_25FPS_8BIT:
	case V4L2_PIXELPLUS_PR2100_2M_2CH_25FPS_8BIT:
	case V4L2_PIXELPLUS_PR2100_2M_4CH_25FPS_8BIT:
		pstSnsObj = &stSnsPR2100_Obj;
		break;
#endif
#if defined(SENSOR_SMS_SC035GS)
	case V4L2_SMS_SC035GS_MIPI_480P_120FPS_12BIT:
		pstSnsObj = &stSnsSC035GS_Obj;
		break;
#endif
#if defined(SENSOR_SMS_SC035GS_1L)
	case V4L2_SMS_SC035GS_1L_MIPI_480P_120FPS_10BIT:
		pstSnsObj = &stSnsSC035GS_1L_Obj;
		break;
#endif
#if defined(SENSOR_SMS_SC035HGS)
	case V4L2_SMS_SC035HGS_MIPI_480P_120FPS_12BIT:
		pstSnsObj = &stSnsSC035HGS_Obj;
		break;
#endif
#if defined(SENSOR_SMS_SC200AI)
	case V4L2_SMS_SC200AI_MIPI_2M_30FPS_10BIT:
	case V4L2_SMS_SC200AI_MIPI_2M_30FPS_10BIT_WDR2TO1:
		pstSnsObj = &stSnsSC200AI_Obj;
		break;
#endif
#if defined(SENSOR_SMS_SC301IOT)
	case V4L2_SMS_SC301IOT_MIPI_3M_30FPS_10BIT:
		pstSnsObj = &stSnsSC301IOT_Obj;
		break;
#endif
#if defined(SENSOR_SMS_SC401AI)
	case V4L2_SMS_SC401AI_MIPI_4M_30FPS_10BIT:
	case V4L2_SMS_SC401AI_MIPI_3M_30FPS_10BIT:
		pstSnsObj = &stSnsSC401AI_Obj;
		break;
#endif
#if defined(SENSOR_SMS_SC500AI)
	case V4L2_SMS_SC500AI_MIPI_5M_30FPS_10BIT:
	case V4L2_SMS_SC500AI_MIPI_5M_30FPS_10BIT_WDR2TO1:
	case V4L2_SMS_SC500AI_MIPI_4M_30FPS_10BIT:
	case V4L2_SMS_SC500AI_MIPI_4M_30FPS_10BIT_WDR2TO1:
		pstSnsObj = &stSnsSC500AI_Obj;
		break;
#endif
#if defined(SENSOR_SMS_SC501AI_2L)
	case V4L2_SMS_SC501AI_2L_MIPI_5M_30FPS_10BIT:
		pstSnsObj = &stSnsSC501AI_2L_Obj;
		break;
#endif
#if defined(SENSOR_SMS_SC531AI_2L)
	case V4L2_SMS_SC531AI_2L_MIPI_5M_30FPS_10BIT:
		pstSnsObj = &stSnsSC531AI_2L_Obj;
		break;
#endif
#if defined(SENSOR_SMS_SC850SL)
	case V4L2_SMS_SC850SL_MIPI_8M_30FPS_12BIT:
	case V4L2_SMS_SC850SL_MIPI_8M_30FPS_10BIT_WDR2TO1:
		pstSnsObj = &stSnsSC850SL_Obj;
		break;
#endif
#if defined(SENSOR_SMS_SC3332)
	case V4L2_SMS_SC3332_MIPI_3M_30FPS_10BIT:
		pstSnsObj = &stSnsSC3332_Obj;
		break;
#endif
#if defined(SENSOR_SMS_SC3335)
	case V4L2_SMS_SC3335_MIPI_3M_30FPS_10BIT:
		pstSnsObj = &stSnsSC3335_Obj;
		break;
#endif
#if defined(SENSOR_SMS_SC3335_SLAVE)
	case V4L2_SMS_SC3335_SLAVE_MIPI_3M_30FPS_10BIT:
		pstSnsObj = &stSnsSC3335_Slave_Obj;
		break;
#endif
#if defined(SENSOR_SMS_SC3336)
	case V4L2_SMS_SC3336_MIPI_3M_30FPS_10BIT:
		pstSnsObj = &stSnsSC3336_Obj;
		break;
#endif
#if defined(SENSOR_SMS_SC2335)
	case V4L2_SMS_SC2335_MIPI_2M_30FPS_10BIT:
		pstSnsObj = &stSnsSC2335_Obj;
		break;
#endif
#if defined(SENSOR_SMS_SC4210)
	case V4L2_SMS_SC4210_MIPI_4M_30FPS_12BIT:
	case V4L2_SMS_SC4210_MIPI_4M_30FPS_10BIT_WDR2TO1:
		pstSnsObj = &stSnsSC4210_Obj;
		break;
#endif
#if defined(SENSOR_SMS_SC4336)
	case V4L2_SMS_SC4336_MIPI_4M_30FPS_10BIT:
		pstSnsObj = &stSnsSC4336_Obj;
		break;
#endif
#if defined(SENSOR_SMS_SC4336P)
	case V4L2_SMS_SC4336P_MIPI_4M_30FPS_10BIT:
		pstSnsObj = &stSnsSC4336P_Obj;
		break;
#endif
#if defined(SENSOR_SMS_SC4336P_SLAVE)
	case V4L2_SMS_SC4336P_SLAVE_MIPI_4M_30FPS_10BIT:
		pstSnsObj = &stSnsSC4336P_SLAVE_Obj;
		break;
#endif
#if defined(SENSOR_SMS_SC8238)
	case V4L2_SMS_SC8238_MIPI_8M_30FPS_10BIT:
	case V4L2_SMS_SC8238_MIPI_8M_15FPS_10BIT_WDR2TO1:
		pstSnsObj = &stSnsSC8238_Obj;
		break;
#endif
#if defined(SENSOR_SOI_F23)
	case V4L2_SOI_F23_MIPI_2M_30FPS_10BIT:
		pstSnsObj = &stSnsF23_Obj;
		break;
#endif
#if defined(SENSOR_SOI_F35)
	case V4L2_SOI_F35_MIPI_2M_30FPS_10BIT:
	case V4L2_SOI_F35_MIPI_2M_30FPS_10BIT_WDR2TO1:
		pstSnsObj = &stSnsF35_Obj;
		break;
#endif
#if defined(SENSOR_SOI_F35_SLAVE)
	case V4L2_SOI_F35_SLAVE_MIPI_2M_30FPS_10BIT:
	case V4L2_SOI_F35_SLAVE_MIPI_2M_30FPS_10BIT_WDR2TO1:
		pstSnsObj = &stSnsF35_Slave_Obj;
		break;
#endif
#if defined(SENSOR_SOI_F37P)
	case V4L2_SOI_F37P_MIPI_2M_30FPS_10BIT:
		pstSnsObj = &stSnsF37P_Obj;
		break;
#endif
#if defined(SENSOR_SOI_H65)
	case V4L2_SOI_H65_MIPI_1M_30FPS_10BIT:
		pstSnsObj = &stSnsH65_Obj;
		break;
#endif
#if defined(SENSOR_SOI_K06)
	case V4L2_SOI_K06_MIPI_4M_25FPS_10BIT:
		pstSnsObj = &stSnsK06_Obj;
		break;
#endif
#if defined(SENSOR_SOI_Q03)
	case V4L2_SOI_Q03_MIPI_3M_30FPS_10BIT:
		pstSnsObj = &stSnsQ03_Obj;
		break;
#endif
#if defined(SENSOR_SONY_IMX290_2L)
	case V4L2_SONY_IMX290_MIPI_1M_30FPS_12BIT:
	case V4L2_SONY_IMX290_MIPI_2M_60FPS_12BIT:
		pstSnsObj = &stSnsImx290_2l_Obj;
		break;
#endif
#if defined(SENSOR_SONY_IMX307)
	case V4L2_SONY_IMX307_MIPI_2M_30FPS_12BIT:
	case V4L2_SONY_IMX307_MIPI_2M_60FPS_12BIT:
	case V4L2_SONY_IMX307_MIPI_2M_30FPS_12BIT_WDR2TO1:
		pstSnsObj = &stSnsImx307_Obj;
		break;
#endif
#if defined(SENSOR_SONY_IMX307_SLAVE)
	case V4L2_SONY_IMX307_SLAVE_MIPI_2M_30FPS_12BIT:
	case V4L2_SONY_IMX307_SLAVE_MIPI_2M_30FPS_12BIT_WDR2TO1:
		pstSnsObj = &stSnsImx307_Slave_Obj;
		break;
#endif
#if defined(SENSOR_SONY_IMX307_2L)
	case V4L2_SONY_IMX307_2L_MIPI_2M_30FPS_12BIT:
	case V4L2_SONY_IMX307_2L_MIPI_2M_30FPS_12BIT_WDR2TO1:
		pstSnsObj = &stSnsImx307_2l_Obj;
		break;
#endif
#if defined(SENSOR_SONY_IMX307_SUBLVDS)
	case V4L2_SONY_IMX307_SUBLVDS_2M_30FPS_12BIT:
	case V4L2_SONY_IMX307_SUBLVDS_2M_60FPS_12BIT:
	case V4L2_SONY_IMX307_SUBLVDS_2M_30FPS_12BIT_WDR2TO1:
		pstSnsObj = &stSnsImx307_Sublvds_Obj;
		break;
#endif
#if defined(SENSOR_SONY_IMX327)
	case V4L2_SONY_IMX327_MIPI_2M_30FPS_12BIT:
	case V4L2_SONY_IMX327_MIPI_2M_60FPS_12BIT:
	case V4L2_SONY_IMX327_MIPI_2M_30FPS_12BIT_WDR2TO1:
		pstSnsObj = &stSnsImx327_Obj;
		break;
#endif
#if defined(SENSOR_SONY_IMX327_SLAVE)
	case V4L2_SONY_IMX327_SLAVE_MIPI_2M_30FPS_12BIT:
	case V4L2_SONY_IMX327_SLAVE_MIPI_2M_30FPS_12BIT_WDR2TO1:
		pstSnsObj = &stSnsImx327_Slave_Obj;
		break;
#endif
#if defined(SENSOR_SONY_IMX327_2L)
	case V4L2_SONY_IMX327_2L_MIPI_2M_30FPS_12BIT:
	case V4L2_SONY_IMX327_2L_MIPI_2M_30FPS_12BIT_WDR2TO1:
		pstSnsObj = &stSnsImx327_2l_Obj;
		break;
#endif
#if defined(SENSOR_SONY_IMX327_FPGA)
	case V4L2_BOARD_FULL_SIZE_MIPI_30FPS_12BIT:
	case V4L2_BOARD_MAX_SIZE_MIPI_30FPS_12BIT:
	case V4L2_SONY_IMX327_MIPI_1M_30FPS_10BIT:
	case V4L2_SONY_IMX327_MIPI_1M_30FPS_10BIT_WDR2TO1:
		pstSnsObj = &stSnsImx327_fpga_Obj;
		break;
#endif
#if defined(SENSOR_SONY_IMX327_SUBLVDS)
	case V4L2_SONY_IMX327_SUBLVDS_2M_30FPS_12BIT:
	case V4L2_SONY_IMX327_SUBLVDS_2M_30FPS_12BIT_WDR2TO1:
		pstSnsObj = &stSnsImx327_Sublvds_Obj;
		break;
#endif
#if defined(SENSOR_SONY_IMX334)
	case V4L2_SONY_IMX334_MIPI_8M_30FPS_12BIT:
	case V4L2_SONY_IMX334_MIPI_8M_30FPS_12BIT_WDR2TO1:
		pstSnsObj = &stSnsImx334_Obj;
		break;
#endif
#if defined(SENSOR_SONY_IMX335)
	case V4L2_SONY_IMX335_MIPI_2M_30FPS_10BIT_WDR2TO1:
	case V4L2_SONY_IMX335_MIPI_4M_30FPS_12BIT:
	case V4L2_SONY_IMX335_MIPI_4M_30FPS_10BIT_WDR2TO1:
	case V4L2_SONY_IMX335_2L_MIPI_4M_30FPS_10BIT:
	case V4L2_SONY_IMX335_MIPI_4M_1600P_30FPS_12BIT:
	case V4L2_SONY_IMX335_MIPI_4M_1600P_30FPS_10BIT_WDR2TO1:
	case V4L2_SONY_IMX335_MIPI_5M_30FPS_12BIT:
	case V4L2_SONY_IMX335_MIPI_5M_30FPS_10BIT_WDR2TO1:
	case V4L2_SONY_IMX335_MIPI_2M_60FPS_10BIT:
	case V4L2_SONY_IMX335_MIPI_4M_60FPS_10BIT:
	case V4L2_SONY_IMX335_MIPI_5M_60FPS_10BIT:
		pstSnsObj = &stSnsImx335_Obj;
		break;
#endif
#if defined(SENSOR_SONY_IMX347)
	case V4L2_SONY_IMX347_MIPI_4M_60FPS_12BIT:
	case V4L2_SONY_IMX347_MIPI_4M_30FPS_12BIT_WDR2TO1:
		return &stSnsImx347_Obj;
#endif
#if defined(SENSOR_SONY_IMX385)
	case V4L2_SONY_IMX385_MIPI_2M_30FPS_12BIT:
	case V4L2_SONY_IMX385_MIPI_2M_30FPS_12BIT_WDR2TO1:
		pstSnsObj = &stSnsImx385_Obj;
		break;
#endif
#if defined(SENSOR_SONY_IMX585)
	case V4L2_SONY_IMX585_MIPI_8M_30FPS_12BIT:
	case V4L2_SONY_IMX585_MIPI_8M_25FPS_12BIT_WDR2TO1:
		pstSnsObj = &stSnsImx585_Obj;
		break;
#endif
#if defined(SENSOR_SONY_IMX412)
	case V4L2_SONY_IMX412_MIPI_12M_30FPS_12BIT:
		pstSnsObj = &stSnsImx412_Obj;
		break;
#endif
#if defined(SENSOR_TECHPOINT_TP2850)
	case V4L2_TECHPOINT_TP2850_MIPI_2M_30FPS_8BIT:
	case V4L2_TECHPOINT_TP2850_MIPI_4M_30FPS_8BIT:
		pstSnsObj = &stSnsTP2850_Obj;
		break;
#endif
#if defined(SENSOR_TECHPOINT_TP2860)
	case V4L2_TECHPOINT_TP2860_MIPI_2M_25FPS_8BIT:
		pstSnsObj = &stSnsTP2860_Obj;
		break;
#endif
#if defined(SENSOR_VIVO_MCS369)
	case V4L2_VIVO_MCS369_2M_30FPS_12BIT:
		pstSnsObj = &stSnsMCS369_Obj;
		break;
#endif
#if defined(SENSOR_VIVO_MCS369Q)
	case V4L2_VIVO_MCS369Q_4M_30FPS_12BIT:
		pstSnsObj = &stSnsMCS369Q_Obj;
		break;
#endif
#if defined(SENSOR_VIVO_MM308M2)
	case V4L2_VIVO_MM308M2_2M_25FPS_8BIT:
		pstSnsObj = &stSnsMM308M2_Obj;
		break;
#endif
#if defined(SENSOR_LONTIUM_LT6911)
	case V4L2_LONTIUM_MIPI_LT6911_1M_60FPS_8BIT:
	case V4L2_LONTIUM_MIPI_LT6911_2M_60FPS_8BIT:
	case V4L2_LONTIUM_MIPI_LT6911_8M_60FPS_8BIT:
	case V4L2_LONTIUM_MIPI_LT6911_1M_30FPS_8BIT:
	case V4L2_LONTIUM_MIPI_LT6911_2M_30FPS_8BIT:
	case V4L2_LONTIUM_MIPI_LT6911_8M_30FPS_8BIT:
		pstSnsObj = &stSnsLT6911_Obj;
		break;
#endif
	default:
		pstSnsObj = CVI_NULL;
		break;
	}

	return pstSnsObj;
}

static int get_isp_attr_by_sensor(int pipe, ISP_PUB_ATTR_S *pstPubAttr)
{
	char devicename[64];
	int sns_fd;
	int index = 2 + pipe;

	sprintf(devicename, "/dev/v4l-subdev%d", index);
	sns_fd = open(devicename, O_RDWR | O_NONBLOCK);

	if (sns_fd < 0) {
		printf("open %s fail!\n", devicename);
		return -1;
	}

	int sns_type;

	if (ioctl(sns_fd, SNS_V4L2_GET_TYPE, &sns_type) < 0) {
		printf("get sensor type fail !\n");
		return -1;
	}

	close(sns_fd);

	struct v4l2_format g_format;

	g_format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	int vi_fd;

	CVI_ISP_V4L2_GetFd(pipe, &vi_fd);
	if (ioctl(vi_fd, VIDIOC_G_FMT, &g_format) < 0) {
		printf("get format fail!\n");
		return -1;
	}

	memset(pstPubAttr, 0, sizeof(ISP_PUB_ATTR_S));

	pstPubAttr->stWndRect.s32X = 0;
	pstPubAttr->stWndRect.s32Y = 0;
	pstPubAttr->stWndRect.u32Height = g_format.fmt.pix.height;
	pstPubAttr->stWndRect.u32Width = g_format.fmt.pix.width;
	pstPubAttr->stSnsSize.u32Height = g_format.fmt.pix.height;
	pstPubAttr->stSnsSize.u32Width = g_format.fmt.pix.width;

	/* WDR mode */
	if (sns_type >= V4L2_SNS_TYPE_LINEAR_BUTT) {
		pstPubAttr->enWDRMode = WDR_MODE_2To1_LINE;
	}

	/* FPS */
	switch (sns_type) {
	case V4L2_SMS_SC035GS_MIPI_480P_120FPS_12BIT:
	case V4L2_SMS_SC035GS_1L_MIPI_480P_120FPS_10BIT:
	case V4L2_SMS_SC035HGS_MIPI_480P_120FPS_12BIT:
	case V4L2_OV_OV6211_MIPI_400P_120FPS_10BIT:
	case V4L2_OV_OV7251_MIPI_480P_120FPS_10BIT:
		pstPubAttr->f32FrameRate = 120;
		break;
	case V4L2_SONY_IMX307_MIPI_2M_60FPS_12BIT:
	case V4L2_SONY_IMX307_SUBLVDS_2M_60FPS_12BIT:
	case V4L2_SONY_IMX327_MIPI_2M_60FPS_12BIT:
	case V4L2_SONY_IMX335_MIPI_2M_60FPS_10BIT:
	case V4L2_SONY_IMX335_MIPI_4M_60FPS_10BIT:
	case V4L2_SONY_IMX335_MIPI_5M_60FPS_10BIT:
	case V4L2_SONY_IMX347_MIPI_4M_60FPS_12BIT:
	case V4L2_LONTIUM_MIPI_LT6911_1M_60FPS_8BIT:
	case V4L2_LONTIUM_MIPI_LT6911_2M_60FPS_8BIT:
	case V4L2_LONTIUM_MIPI_LT6911_8M_60FPS_8BIT:
		pstPubAttr->f32FrameRate = 60;
		break;
	case V4L2_TECHPOINT_TP2850_MIPI_2M_30FPS_8BIT:
	case V4L2_TECHPOINT_TP2850_MIPI_4M_30FPS_8BIT:
	case V4L2_SONY_IMX307_2L_MIPI_2M_30FPS_12BIT:
	case V4L2_SONY_IMX335_MIPI_2M_30FPS_10BIT_WDR2TO1:
	case V4L2_SONY_IMX347_MIPI_4M_30FPS_12BIT_WDR2TO1:
	case V4L2_SONY_IMX412_MIPI_12M_30FPS_12BIT:
	case V4L2_SONY_IMX585_MIPI_8M_30FPS_12BIT:
	case V4L2_OV_OS04A10_MIPI_4M_1440P_30FPS_12BIT:
	case V4L2_OV_OS08B10_MIPI_8M_30FPS_10BIT:
	case V4L2_OV_OS08B10_MIPI_8M_30FPS_10BIT_WDR2TO1:
	case V4L2_LONTIUM_MIPI_LT6911_1M_30FPS_8BIT:
	case V4L2_LONTIUM_MIPI_LT6911_2M_30FPS_8BIT:
	case V4L2_LONTIUM_MIPI_LT6911_8M_30FPS_8BIT:
		pstPubAttr->f32FrameRate = 30;
		break;
	case V4L2_OV_OS04E10_MIPI_4M_30FPS_2L_10BIT_WDR2TO1:
	case V4L2_OV_OS04E10_SLAVE_MIPI_4M_30FPS_2L_10BIT_WDR2TO1:
		pstPubAttr->f32FrameRate = 20;
		break;
	case V4L2_GCORE_GC2145_MIPI_2M_12FPS_8BIT:
		pstPubAttr->f32FrameRate = 12;
		break;
	case V4L2_SONY_IMX327_MIPI_1M_30FPS_10BIT:
	case V4L2_SONY_IMX327_MIPI_1M_30FPS_10BIT_WDR2TO1:
		pstPubAttr->f32FrameRate = 10;
		break;
	default:
		pstPubAttr->f32FrameRate = 25;
		break;
	}

	/* bayerid */
	switch (sns_type) {
	case V4L2_SOI_K06_MIPI_4M_25FPS_10BIT:
		pstPubAttr->enBayer = BAYER_GBRG;
		break;
	// Sony
	case V4L2_BOARD_FULL_SIZE_MIPI_30FPS_12BIT:
	case V4L2_BOARD_MAX_SIZE_MIPI_30FPS_12BIT:
	case V4L2_SONY_IMX307_MIPI_2M_30FPS_12BIT:
	case V4L2_SONY_IMX307_MIPI_2M_30FPS_12BIT_WDR2TO1:
	case V4L2_SONY_IMX307_SLAVE_MIPI_2M_30FPS_12BIT:
	case V4L2_SONY_IMX307_SLAVE_MIPI_2M_30FPS_12BIT_WDR2TO1:
	case V4L2_SONY_IMX307_2L_MIPI_2M_30FPS_12BIT:
	case V4L2_SONY_IMX307_2L_MIPI_2M_30FPS_12BIT_WDR2TO1:
	case V4L2_SONY_IMX307_SUBLVDS_2M_30FPS_12BIT:
	case V4L2_SONY_IMX307_SUBLVDS_2M_30FPS_12BIT_WDR2TO1:
	case V4L2_SONY_IMX307_MIPI_2M_60FPS_12BIT:
	case V4L2_SONY_IMX307_SUBLVDS_2M_60FPS_12BIT:
	case V4L2_SONY_IMX327_MIPI_1M_30FPS_10BIT:
	case V4L2_SONY_IMX327_MIPI_1M_30FPS_10BIT_WDR2TO1:
	case V4L2_SONY_IMX327_MIPI_2M_30FPS_12BIT:
	case V4L2_SONY_IMX327_MIPI_2M_30FPS_12BIT_WDR2TO1:
	case V4L2_SONY_IMX327_SLAVE_MIPI_2M_30FPS_12BIT:
	case V4L2_SONY_IMX327_SLAVE_MIPI_2M_30FPS_12BIT_WDR2TO1:
	case V4L2_SONY_IMX327_2L_MIPI_2M_30FPS_12BIT:
	case V4L2_SONY_IMX327_2L_MIPI_2M_30FPS_12BIT_WDR2TO1:
	case V4L2_SONY_IMX327_SUBLVDS_2M_30FPS_12BIT:
	case V4L2_SONY_IMX327_SUBLVDS_2M_30FPS_12BIT_WDR2TO1:
	case V4L2_SONY_IMX327_MIPI_2M_60FPS_12BIT:
	case V4L2_SONY_IMX334_MIPI_8M_30FPS_12BIT:
	case V4L2_SONY_IMX334_MIPI_8M_30FPS_12BIT_WDR2TO1:
	case V4L2_SONY_IMX335_MIPI_2M_30FPS_10BIT_WDR2TO1:
	case V4L2_SONY_IMX335_MIPI_4M_30FPS_12BIT:
	case V4L2_SONY_IMX335_MIPI_4M_30FPS_10BIT_WDR2TO1:
	case V4L2_SONY_IMX335_2L_MIPI_4M_30FPS_10BIT:
	case V4L2_SONY_IMX335_MIPI_4M_1600P_30FPS_12BIT:
	case V4L2_SONY_IMX335_MIPI_4M_1600P_30FPS_10BIT_WDR2TO1:
	case V4L2_SONY_IMX335_MIPI_5M_30FPS_12BIT:
	case V4L2_SONY_IMX335_MIPI_5M_30FPS_10BIT_WDR2TO1:
	case V4L2_SONY_IMX335_MIPI_2M_60FPS_10BIT:
	case V4L2_SONY_IMX335_MIPI_4M_60FPS_10BIT:
	case V4L2_SONY_IMX335_MIPI_5M_60FPS_10BIT:
	case V4L2_SONY_IMX347_MIPI_4M_60FPS_12BIT:
	case V4L2_SONY_IMX347_MIPI_4M_30FPS_12BIT_WDR2TO1:
	case V4L2_SONY_IMX385_MIPI_2M_30FPS_12BIT:
	case V4L2_SONY_IMX385_MIPI_2M_30FPS_12BIT_WDR2TO1:
	case V4L2_SONY_IMX412_MIPI_12M_30FPS_12BIT:
	case V4L2_SONY_IMX585_MIPI_8M_30FPS_12BIT:
	case V4L2_SONY_IMX585_MIPI_8M_25FPS_12BIT_WDR2TO1:
	// GalaxyCore
	case V4L2_GCORE_GC02M1_MIPI_2M_30FPS_10BIT:
	case V4L2_GCORE_GC1054_MIPI_1M_30FPS_10BIT:
	case V4L2_GCORE_GC2053_MIPI_2M_30FPS_10BIT:
	case V4L2_GCORE_GC2053_SLAVE_MIPI_2M_30FPS_10BIT:
	case V4L2_GCORE_GC2053_1L_MIPI_2M_30FPS_10BIT:
	case V4L2_GCORE_GC2093_MIPI_2M_30FPS_10BIT:
	case V4L2_GCORE_GC2093_MIPI_2M_30FPS_10BIT_WDR2TO1:
	case V4L2_GCORE_GC2093_SLAVE_MIPI_2M_30FPS_10BIT:
	case V4L2_GCORE_GC2093_SLAVE_MIPI_2M_30FPS_10BIT_WDR2TO1:
	case V4L2_GCORE_GC4023_MIPI_4M_30FPS_10BIT:
		pstPubAttr->enBayer = BAYER_RGGB;
		break;
	case V4L2_GCORE_GC4653_MIPI_4M_30FPS_10BIT:
	case V4L2_GCORE_GC4653_SLAVE_MIPI_4M_30FPS_10BIT:
	case V4L2_TECHPOINT_TP2850_MIPI_2M_30FPS_8BIT:
	case V4L2_TECHPOINT_TP2850_MIPI_4M_30FPS_8BIT:
	case V4L2_ONSEMI_AR2020_20M_25FPS_10BIT:
		pstPubAttr->enBayer = BAYER_GRBG;
		break;
	default:
		pstPubAttr->enBayer = BAYER_BGGR;
		break;
	};

	/* Lane num */
	switch (sns_type) {
	case V4L2_OV_OS04A10_MIPI_4M_1440P_2L_10BIT:
	case V4L2_OV_OS04A10_MASTER_MIPI_4M_1440P_2L_10BIT:
	case V4L2_OV_OS04A10_SLAVE_MIPI_4M_1440P_2L_10BIT:
	case V4L2_OV_OS04A10_MASTER_MIPI_4M_1440P_2L_10BIT_WDR2TO1:
	case V4L2_OV_OS04A10_SLAVE_MIPI_4M_1440P_2L_10BIT_WDR2TO1:
	case V4L2_OV_OS04E10_MIPI_4M_30FPS_2L_10BIT:
	case V4L2_OV_OS04E10_SALVE_MIPI_4M_30FPS_2L_10BIT:
	case V4L2_OV_OS04E10_MIPI_4M_30FPS_2L_10BIT_WDR2TO1:
	case V4L2_OV_OS04E10_SLAVE_MIPI_4M_30FPS_2L_10BIT_WDR2TO1:
		pstPubAttr->u8LaneNum = 2;
		break;
	default:
		pstPubAttr->u8LaneNum = 4;
		break;
	}

	/* enable master : 1 slave : 0 */
	switch (sns_type) {
	case V4L2_OV_OS04A10_SLAVE_MIPI_4M_1440P_2L_10BIT:
	case V4L2_OV_OS04A10_SLAVE_MIPI_4M_1440P_2L_10BIT_WDR2TO1:
	case V4L2_OV_OS04E10_SALVE_MIPI_4M_30FPS_2L_10BIT:
	case V4L2_OV_OS04E10_SLAVE_MIPI_4M_30FPS_2L_10BIT_WDR2TO1:
		pstPubAttr->u8EnableMaster = 0;
		break;
	case V4L2_OV_OS04A10_MASTER_MIPI_4M_1440P_2L_10BIT:
	case V4L2_OV_OS04A10_MASTER_MIPI_4M_1440P_2L_10BIT_WDR2TO1:
	case V4L2_OV_OS04E10_MIPI_4M_30FPS_2L_10BIT:
	case V4L2_OV_OS04E10_MIPI_4M_30FPS_2L_10BIT_WDR2TO1:
		pstPubAttr->u8EnableMaster = 1;
		break;
	default:
		pstPubAttr->u8EnableMaster = 2;
		break;
	}

	return 0;
}

static CVI_S32 reg_sensor(int pipe)
{
	CVI_S32 s32Ret;
	ISP_SNS_OBJ_S *pstSnsObj = get_sensor_obj(pipe);

	/* set i2c bus info */
	sns_i2c_info_t i2c_info;
	ISP_SNS_COMMBUS_U unSNSBusInfo;
	ISP_SENSOR_EXP_FUNC_S stSnsrSensorFunc;
	ISP_CMOS_SENSOR_IMAGE_MODE_S stSnsrMode;
	WDR_MODE_E wdrMode;
	ISP_PUB_ATTR_S stPubAttr;
	int sns_fd = open_v4l2_sensor(pipe);
	get_isp_attr_by_sensor(pipe, &stPubAttr);
	if (sns_fd < 0) {
		printf("open pipe %d sensor fail!\n", pipe);
		return -1;
	}

	if (ioctl(sns_fd, SNS_V4L2_GET_I2C_INFO, &i2c_info) < 0) {
		printf("get sns i2c information fail!\n");
		return -1;
	}
	printf("pipe: %d, i2c_idx: %d, i2c_addr: %d\n", pipe, i2c_info.i2c_idx, i2c_info.i2c_addr);

	close(sns_fd);

	unSNSBusInfo.s8I2cDev = i2c_info.i2c_idx;

	if (pstSnsObj->pfnSetBusInfo(pipe, unSNSBusInfo) < 0) {
		printf("pipe %d pfnSetBusInfo error\n", pipe);
		return -1;
	}

	if (pstSnsObj->pfnPatchI2cAddr) {
		pstSnsObj->pfnPatchI2cAddr(pipe, i2c_info.i2c_addr);
	}

	/* register call back */
	ALG_LIB_S stAeLib;
	ALG_LIB_S stAwbLib;

	stAeLib.s32Id = pipe;
	stAwbLib.s32Id = pipe;
	strncpy(stAeLib.acLibName, CVI_AE_LIB_NAME, sizeof(stAeLib.acLibName));
	strncpy(stAwbLib.acLibName, CVI_AWB_LIB_NAME, sizeof(stAwbLib.acLibName));

	if (pstSnsObj->pfnRegisterCallback != CVI_NULL) {
		s32Ret = pstSnsObj->pfnRegisterCallback(pipe, &stAeLib, &stAwbLib);
		if (s32Ret != CVI_SUCCESS) {
			printf("sensor_register_callback failed with %#x!\n", s32Ret);
			return s32Ret;
		}
	} else {
		printf("sensor_register_callback failed with CVI_NULL!\n");
		return CVI_FAILURE;
	}
	stSnsrMode.u16Width = stPubAttr.stSnsSize.u32Width;
	stSnsrMode.u16Height = stPubAttr.stSnsSize.u32Height;
	stSnsrMode.f32Fps = stPubAttr.f32FrameRate;
	stSnsrMode.u8LaneNum = stPubAttr.u8LaneNum;
	stSnsrMode.u8EnableMaster = stPubAttr.u8EnableMaster;
	wdrMode = stPubAttr.enWDRMode;

	pstSnsObj->pfnExpSensorCb(&stSnsrSensorFunc);

	if (stSnsrSensorFunc.pfn_cmos_set_wdr_mode) {
		s32Ret = stSnsrSensorFunc.pfn_cmos_set_wdr_mode(pipe, wdrMode);
		if (s32Ret != CVI_SUCCESS) {
			CVI_TRACE_LOG(CVI_DBG_ERR, "sensor set wdr mode failed!\n");
			return s32Ret;
		}
	}

	if (stSnsrSensorFunc.pfn_cmos_set_image_mode) {
		s32Ret = stSnsrSensorFunc.pfn_cmos_set_image_mode(pipe, &stSnsrMode);
		if (s32Ret != CVI_SUCCESS) {
			CVI_TRACE_LOG(CVI_DBG_ERR, "sensor set image mode failed!\n");
			return s32Ret;
		}
	}

	return CVI_SUCCESS;
}

static CVI_S32 unreg_sensor(int pipe)
{
	ALG_LIB_S stAeLib;
	ALG_LIB_S stAwbLib;
	const ISP_SNS_OBJ_S *pstSnsObj;
	CVI_S32 s32Ret = -1;

	pstSnsObj = (ISP_SNS_OBJ_S *)get_sensor_obj(pipe);

	stAeLib.s32Id = pipe;
	stAwbLib.s32Id = pipe;
	strncpy(stAeLib.acLibName, CVI_AE_LIB_NAME, sizeof(stAeLib.acLibName));
	strncpy(stAwbLib.acLibName, CVI_AWB_LIB_NAME, sizeof(stAwbLib.acLibName));

	if (pstSnsObj->pfnUnRegisterCallback != CVI_NULL) {
		s32Ret = pstSnsObj->pfnUnRegisterCallback(pipe, &stAeLib, &stAwbLib);

		if (s32Ret != CVI_SUCCESS) {
			printf("sensor_unregister_callback failed with %#x!\n", s32Ret);
			return s32Ret;
		}
	} else {
		printf("sensor_unregister_callback failed with CVI_NULL!\n");
	}

	return CVI_SUCCESS;
}

static CVI_S32 reg_awblib(int pipe)
{
	ALG_LIB_S stAwbLib;
	CVI_S32 s32Ret = 0;

	stAwbLib.s32Id = pipe;
	strncpy(stAwbLib.acLibName, CVI_AWB_LIB_NAME, sizeof(stAwbLib.acLibName));
	s32Ret = CVI_AWB_Register(pipe, &stAwbLib);
	if (s32Ret != CVI_SUCCESS) {
		printf("AWB Algo register failed!, error: %d\n",	s32Ret);
		return s32Ret;
	}
	return CVI_SUCCESS;
}

static CVI_S32 unreg_awblib(int pipe)
{
	CVI_S32 s32Ret = 0;
	ALG_LIB_S stAwbLib;

	stAwbLib.s32Id = pipe;
	strncpy(stAwbLib.acLibName, CVI_AWB_LIB_NAME, sizeof(stAwbLib.acLibName));
	s32Ret = CVI_AWB_UnRegister(pipe, &stAwbLib);
	if (s32Ret) {
		printf("AWB Algo unRegister failed!, error: %d\n",	s32Ret);
		return s32Ret;
	}
	return CVI_SUCCESS;
}

static CVI_S32 reg_aelib(int pipe)
{
	CVI_S32 s32Ret = 0;
	ALG_LIB_S stAeLib;

	stAeLib.s32Id = pipe;
	strncpy(stAeLib.acLibName, CVI_AE_LIB_NAME, sizeof(stAeLib.acLibName));
	s32Ret = CVI_AE_Register(pipe, &stAeLib);
	if (s32Ret != CVI_SUCCESS) {
		printf("AE Algo register failed!, error: %d\n",	s32Ret);
		return s32Ret;
	}
	return CVI_SUCCESS;
}

static CVI_S32 unreg_aelib(int pipe)
{
	CVI_S32 s32Ret = 0;
	ALG_LIB_S stAeLib;

	stAeLib.s32Id = pipe;
	strncpy(stAeLib.acLibName, CVI_AE_LIB_NAME, sizeof(stAeLib.acLibName));
	s32Ret = CVI_AE_UnRegister(pipe, &stAeLib);
	if (s32Ret) {
		printf("AE Algo unRegister failed!, error: %d\n",	s32Ret);
		return s32Ret;
	}
	return CVI_SUCCESS;
}

#if ENABLE_AF_LIB
static CVI_S32 reg_aflib(int pipe)
{
	ALG_LIB_S stAfLib;
	CVI_S32 s32Ret = 0;

	stAfLib.s32Id = pipe;
	strncpy(stAfLib.acLibName, CVI_AF_LIB_NAME, sizeof(stAfLib.acLibName));
	s32Ret = CVI_AF_Register(pipe, &stAfLib);

	if (s32Ret != CVI_SUCCESS) {
		printf("AF Algo register failed!, error: %d\n", s32Ret);
		return s32Ret;
	}
	return CVI_SUCCESS;
}

static CVI_S32 unreg_aflib(int pipe)
{
	CVI_S32 s32Ret = 0;
	ALG_LIB_S stAfLib;

	stAfLib.s32Id = pipe;
	strncpy(stAfLib.acLibName, CVI_AF_LIB_NAME, sizeof(stAfLib.acLibName));
	s32Ret = CVI_AF_UnRegister(pipe, &stAfLib);
	if (s32Ret) {
		printf("AF Algo unRegister failed!, error: %d\n",	s32Ret);
		return s32Ret;
	}
	return CVI_SUCCESS;
}
#endif

static CVI_S32 isp_init(int pipe)
{
	CVI_S32 s32Ret = CVI_FAILURE;
	ISP_PUB_ATTR_S stPubAttr;
	ISP_BIND_ATTR_S stBindAttr;

	reg_aelib(pipe);
	reg_awblib(pipe);
#if ENABLE_AF_LIB
	reg_aflib(pipe);
#endif
	snprintf(stBindAttr.stAeLib.acLibName, sizeof(CVI_AE_LIB_NAME), "%s", CVI_AE_LIB_NAME);
	stBindAttr.stAeLib.s32Id = pipe;
	stBindAttr.sensorId = 0;
	snprintf(stBindAttr.stAwbLib.acLibName, sizeof(CVI_AWB_LIB_NAME), "%s", CVI_AWB_LIB_NAME);
	stBindAttr.stAwbLib.s32Id = pipe;
#if ENABLE_AF_LIB
	snprintf(stBindAttr.stAfLib.acLibName, sizeof(CVI_AF_LIB_NAME), "%s", CVI_AF_LIB_NAME);
	stBindAttr.stAfLib.s32Id = pipe;
#endif

	s32Ret = CVI_ISP_SetBindAttr(pipe, &stBindAttr);
	if (s32Ret != CVI_SUCCESS) {
		printf("Bind Algo failed with %#x!\n", s32Ret);
	}

	s32Ret = CVI_ISP_MemInit(pipe);
	if (s32Ret != CVI_SUCCESS) {
		printf("Init Ext memory failed with %#x!\n", s32Ret);
		return s32Ret;
	}

	get_isp_attr_by_sensor(pipe, &stPubAttr);
	printf("------pipe: %d pub attr setting------\nheight: %d\nwidth: %d\nfps:%f\nwdr:%d\nenBayer: %d\n",
		pipe, stPubAttr.stSnsSize.u32Height,
		stPubAttr.stSnsSize.u32Width, stPubAttr.f32FrameRate, stPubAttr.enWDRMode, stPubAttr.enBayer);

	s32Ret = CVI_ISP_SetPubAttr(pipe, &stPubAttr);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_LOG(CVI_DBG_ERR, "SetPubAttr failed with %#x!\n", s32Ret);
		return s32Ret;
	}

	s32Ret = CVI_ISP_Init(pipe);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_LOG(CVI_DBG_ERR, "ISP Init failed with %#x!\n", s32Ret);
		return s32Ret;
	}

	return s32Ret;
}

static CVI_S32 sns_default_init(int pipe)
{
	CVI_S32 s32Ret = CVI_FAILURE;
	ISP_SNS_SYNC_INFO_S stSnsSyncCfg;
	sns_sync_info_t sns_sync_info;
	ISP_SNS_OBJ_S *pstSnsObj = get_sensor_obj(pipe);
	ISP_SENSOR_EXP_FUNC_S stSnsrSensorFunc;
	CVI_U32 i;
	int sns_fd;

	pstSnsObj->pfnExpSensorCb(&stSnsrSensorFunc);

	if (stSnsrSensorFunc.pfn_cmos_get_sns_reg_info != CVI_NULL) {
		s32Ret = stSnsrSensorFunc.pfn_cmos_get_sns_reg_info(pipe, &stSnsSyncCfg);
		stSnsSyncCfg.snsCfg.bConfig = CVI_TRUE;
		if (s32Ret != CVI_SUCCESS) {
			CVI_TRACE_LOG(CVI_DBG_ERR, "sensor reg config  mode failed!\n");
			return s32Ret;
		}
	} else {
		CVI_TRACE_LOG(CVI_DBG_ERR, "sensor reg config not registered\n");
		s32Ret = -1;
		return s32Ret;
	}

	for (i = 0; i < stSnsSyncCfg.snsCfg.u32RegNum; i++) {
		sns_sync_info.regs[i].address = stSnsSyncCfg.snsCfg.astI2cData[i].u32RegAddr;
		sns_sync_info.regs[i].val     = stSnsSyncCfg.snsCfg.astI2cData[i].u32Data;
	}

	sns_sync_info.num_of_regs = stSnsSyncCfg.snsCfg.u32RegNum;

	sns_fd = open_v4l2_sensor(pipe);

	if (sns_fd < 0) {
		printf("open pipe %d sensor fail!\n", pipe);
		s32Ret = -1;
		return s32Ret;
	}

	if (ioctl(sns_fd, SNS_V4L2_SET_SNS_SYNC_INFO, &sns_sync_info) < 0) {
		CVI_TRACE_LOG(CVI_DBG_ERR, "set sensor snyc  information fail!\n");
		s32Ret = -1;
		return s32Ret;
	}

	close(sns_fd);
	return s32Ret;
}

static CVI_S32 _getFileSize(FILE *fp, CVI_U32 *size)
{
	CVI_S32 ret = CVI_SUCCESS;

	fseek(fp, 0L, SEEK_END);
	*size = ftell(fp);
	rewind(fp);

	return ret;
}

CVI_S32 load_pqbin(int pipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	FILE *fp = NULL;
	CVI_U8 *buf = NULL;
	CVI_CHAR binName[BIN_FILE_LENGTH] = {0};
	CVI_U32 u32file_size = 0;
	ISP_PUB_ATTR_S stPubAttr;

	get_isp_attr_by_sensor(pipe, &stPubAttr);

	if (stPubAttr.enWDRMode == WDR_MODE_NONE) {
		snprintf(binName, BIN_FILE_LENGTH, "%s", "/mnt/cfg/param/cvi_sdr_bin");
	} else {
		snprintf(binName, BIN_FILE_LENGTH, "%s", "/mnt/cfg/param/cvi_wdr_bin");
	}

	fp = fopen((const CVI_CHAR *)binName, "rb");
	if (fp == NULL) {
		printf("Can't find bin(%s)\n", binName);
		ret = CVI_FAILURE;
		goto ERROR_HANDLER;
	} else {
		printf("Bin exist (%s)\n", binName);
	}

	_getFileSize(fp, &u32file_size);

	buf = (CVI_U8 *)malloc(u32file_size);
	if (buf == NULL) {
		ret = CVI_FAILURE;
		printf("Allocate memory fail\n");
		goto ERROR_HANDLER;
	}

	CVI_U32 u32TempLen = 0;

	u32TempLen = fread(buf, u32file_size, 1, fp);
	if (u32TempLen <= 0) {
		printf("read data to buff fail!\n");
		ret = CVI_FAILURE;
		goto ERROR_HANDLER;
	}

	ret = CVI_BIN_ImportBinData(buf, (CVI_U32)u32file_size);
	if (ret != CVI_SUCCESS) {
		printf("CVI_BIN_ImportBinData error! value:(0x%x)\n", ret);
		goto ERROR_HANDLER;
	}

ERROR_HANDLER:
	if (fp != NULL) {
		fclose(fp);
	}
	if (buf != NULL) {
		free(buf);
	}

	return ret;
}

static CVI_VOID *isp_thread(void *arg)
{
	printf("Running in the isp thread ...\n");

	CVI_S32 s32Ret = 0;
	CVI_U8 pipe = *(CVI_U8 *) arg;
	char szThreadName[20];

	free(arg);
	snprintf(szThreadName, sizeof(szThreadName), "ISP%d_RUN", pipe);
	prctl(PR_SET_NAME, szThreadName, 0, 0, 0);

	printf("ISP Dev %d running!\n", pipe);

	s32Ret = CVI_ISP_Run(pipe);
	if (s32Ret != 0) {
		printf("CVI_ISP_Run failed with %#x!\n", s32Ret);
		return NULL;
	}

	return NULL;
}

static CVI_S32 isp_run(int pipe)
{
	CVI_S32 s32Ret = 0;
	CVI_U8 *arg = malloc(sizeof(*arg));
	//struct sched_param param;
	//pthread_attr_t attr;

	*arg = pipe;
	//param.sched_priority = 80;

	//pthread_attr_init(&attr);
	//pthread_attr_setschedpolicy(&attr, SCHED_RR);
	//pthread_attr_setschedparam(&attr, &param);
	//pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
	//s32Ret = pthread_create(&g_IspPid[pipe], &attr, isp_thread, arg);
	s32Ret = pthread_create(&g_IspPid[pipe], NULL, isp_thread, arg);
	if (s32Ret != 0) {
		printf("create isp running thread failed!, error: %d, %s\r\n",
			s32Ret, strerror(s32Ret));
	}

	return s32Ret;
}

static void isp_stop(int pipe)
{
	CVI_S32 s32Ret = CVI_FAILURE;

	if (g_IspPid[pipe]) {
		s32Ret = CVI_ISP_Exit(pipe);
		if (s32Ret != CVI_SUCCESS) {
			printf("CVI_ISP_Exit fail with %#x!\n", s32Ret);
			return;
		}
		printf("stop the isp pthread id: %llu\n", (unsigned long long)g_IspPid[pipe]);
		pthread_join(g_IspPid[pipe], NULL);
		g_IspPid[pipe] = 0;
		unreg_sensor(pipe);
		unreg_aelib(pipe);
		unreg_awblib(pipe);
#if ENABLE_AF_LIB
		unreg_aflib(pipe);
#endif
	}
}

static int set_dev_attr(int pipe)
{
	int ret;
	VI_DEV_ATTR_S stViDevAttr;
	ISP_PUB_ATTR_S stPubAttr;
	int sns_fd = open_v4l2_sensor(pipe);
	int enSnsType;

	if (ioctl(sns_fd, SNS_V4L2_GET_TYPE, &enSnsType) < 0) {
		printf("pipe: %d, get sensor type fail !\n", pipe);
		return -1;
	}

	close(sns_fd);

	// get dev attr by sensor
	VI_DEV_ATTR_S DEV_ATTR_SENSOR_BASE = {
		VI_MODE_MIPI,
		VI_WORK_MODE_1Multiplex,
		VI_SCAN_PROGRESSIVE,
		{-1, -1, -1, -1},
		VI_DATA_SEQ_YUYV,
		{
		/*port_vsync   port_vsync_neg     port_hsync        port_hsync_neg        */
		VI_VSYNC_PULSE, VI_VSYNC_NEG_LOW, VI_HSYNC_VALID_SINGNAL, VI_HSYNC_NEG_HIGH,
		VI_VSYNC_VALID_SIGNAL, VI_VSYNC_VALID_NEG_HIGH,
		/*hsync_hfb    hsync_act    hsync_hhb*/
		{0,            1920,        0,
		/*vsync0_vhb vsync0_act vsync0_hhb*/
		0,            1080,        0,
		/*vsync1_vhb vsync1_act vsync1_hhb*/
		0,            0,            0}
		},
		VI_DATA_TYPE_RGB,
		{1920, 1080},
		{
			WDR_MODE_NONE,
			1080
		},
		.enBayerFormat = BAYER_FORMAT_BG,
	};

	memcpy(&stViDevAttr, &DEV_ATTR_SENSOR_BASE, sizeof(VI_DEV_ATTR_S));

	// WDR mode
	if (enSnsType >= V4L2_SNS_TYPE_LINEAR_BUTT)
		stViDevAttr.stWDRAttr.enWDRMode = WDR_MODE_2To1_LINE;

	// YUV Sensor
	switch (enSnsType) {
	case V4L2_NEXTCHIP_N5_1M_2CH_25FPS_8BIT:
	case V4L2_NEXTCHIP_N5_2M_25FPS_8BIT:
	case V4L2_NEXTCHIP_N6_2M_4CH_25FPS_8BIT:
	case V4L2_PICO384_THERMAL_384X288:
	case V4L2_PICO640_THERMAL_479P:
	case V4L2_PIXELPLUS_PR2020_1M_25FPS_8BIT:
	case V4L2_PIXELPLUS_PR2020_1M_30FPS_8BIT:
	case V4L2_PIXELPLUS_PR2020_2M_25FPS_8BIT:
	case V4L2_PIXELPLUS_PR2020_2M_30FPS_8BIT:
	case V4L2_PIXELPLUS_PR2100_2M_25FPS_8BIT:
	case V4L2_PIXELPLUS_PR2100_2M_2CH_25FPS_8BIT:
	case V4L2_PIXELPLUS_PR2100_2M_4CH_25FPS_8BIT:
	case V4L2_VIVO_MCS369_2M_30FPS_12BIT:
	case V4L2_VIVO_MCS369Q_4M_30FPS_12BIT:
	case V4L2_VIVO_MM308M2_2M_25FPS_8BIT:
		stViDevAttr.enDataSeq = VI_DATA_SEQ_YUYV;
		stViDevAttr.enInputDataType = VI_DATA_TYPE_YUV;
		stViDevAttr.enIntfMode = VI_MODE_MIPI_YUV422;
		break;
	case V4L2_GCORE_GC2145_MIPI_2M_12FPS_8BIT:
		stViDevAttr.enDataSeq = VI_DATA_SEQ_YUYV;
		stViDevAttr.enInputDataType = VI_DATA_TYPE_YUV;
		stViDevAttr.enIntfMode = VI_MODE_BT601;
		break;
	case V4L2_LONTIUM_MIPI_LT6911_1M_60FPS_8BIT:
	case V4L2_LONTIUM_MIPI_LT6911_2M_60FPS_8BIT:
	case V4L2_LONTIUM_MIPI_LT6911_8M_60FPS_8BIT:
	case V4L2_LONTIUM_MIPI_LT6911_1M_30FPS_8BIT:
	case V4L2_LONTIUM_MIPI_LT6911_2M_30FPS_8BIT:
	case V4L2_LONTIUM_MIPI_LT6911_8M_30FPS_8BIT:
		stViDevAttr.enDataSeq = VI_DATA_SEQ_UYVY;
		stViDevAttr.enInputDataType = VI_DATA_TYPE_YUV;
		stViDevAttr.enIntfMode = VI_MODE_MIPI_YUV422;
		break;
	default:
		break;
	};

	// BT601
	switch (enSnsType) {
	case V4L2_GCORE_GC2145_MIPI_2M_12FPS_8BIT:
		stViDevAttr.enIntfMode = VI_MODE_BT601;
		break;
	default:
		break;
	};

	// BT656
	switch (enSnsType) {
	case V4L2_NEXTCHIP_N5_1M_2CH_25FPS_8BIT:
	case V4L2_NEXTCHIP_N5_2M_25FPS_8BIT:
	case V4L2_PIXELPLUS_PR2020_1M_25FPS_8BIT:
	case V4L2_PIXELPLUS_PR2020_1M_30FPS_8BIT:
	case V4L2_PIXELPLUS_PR2020_2M_25FPS_8BIT:
	case V4L2_PIXELPLUS_PR2020_2M_30FPS_8BIT:
	case V4L2_TECHPOINT_TP2860_MIPI_2M_25FPS_8BIT:
		stViDevAttr.enIntfMode = VI_MODE_BT656;
		break;
	default:
		break;
	};

	// BT1120
	switch (enSnsType) {
	case V4L2_VIVO_MCS369_2M_30FPS_12BIT:
	case V4L2_VIVO_MCS369Q_4M_30FPS_12BIT:
	case V4L2_VIVO_MM308M2_2M_25FPS_8BIT:
		stViDevAttr.enIntfMode = VI_MODE_BT1120_STANDARD;
		break;
	default:
		break;
	};

	// subLVDS
	switch (enSnsType) {
	case V4L2_SONY_IMX307_SUBLVDS_2M_30FPS_12BIT:
	case V4L2_SONY_IMX307_SUBLVDS_2M_30FPS_12BIT_WDR2TO1:
	case V4L2_SONY_IMX307_SUBLVDS_2M_60FPS_12BIT:
	case V4L2_SONY_IMX327_SUBLVDS_2M_30FPS_12BIT:
	case V4L2_SONY_IMX327_SUBLVDS_2M_30FPS_12BIT_WDR2TO1:
		stViDevAttr.enIntfMode = VI_MODE_LVDS;
		break;
	default:
		break;
	};

	switch (enSnsType) {
	// Sony
	case V4L2_BOARD_FULL_SIZE_MIPI_30FPS_12BIT:
	case V4L2_BOARD_MAX_SIZE_MIPI_30FPS_12BIT:
	case V4L2_SONY_IMX307_MIPI_2M_30FPS_12BIT:
	case V4L2_SONY_IMX307_MIPI_2M_30FPS_12BIT_WDR2TO1:
	case V4L2_SONY_IMX307_SLAVE_MIPI_2M_30FPS_12BIT:
	case V4L2_SONY_IMX307_SLAVE_MIPI_2M_30FPS_12BIT_WDR2TO1:
	case V4L2_SONY_IMX307_2L_MIPI_2M_30FPS_12BIT:
	case V4L2_SONY_IMX307_2L_MIPI_2M_30FPS_12BIT_WDR2TO1:
	case V4L2_SONY_IMX307_SUBLVDS_2M_30FPS_12BIT:
	case V4L2_SONY_IMX307_SUBLVDS_2M_30FPS_12BIT_WDR2TO1:
	case V4L2_SONY_IMX307_MIPI_2M_60FPS_12BIT:
	case V4L2_SONY_IMX307_SUBLVDS_2M_60FPS_12BIT:
	case V4L2_SONY_IMX327_MIPI_1M_30FPS_10BIT:
	case V4L2_SONY_IMX327_MIPI_1M_30FPS_10BIT_WDR2TO1:
	case V4L2_SONY_IMX327_MIPI_2M_30FPS_12BIT:
	case V4L2_SONY_IMX327_MIPI_2M_30FPS_12BIT_WDR2TO1:
	case V4L2_SONY_IMX327_SLAVE_MIPI_2M_30FPS_12BIT:
	case V4L2_SONY_IMX327_SLAVE_MIPI_2M_30FPS_12BIT_WDR2TO1:
	case V4L2_SONY_IMX327_2L_MIPI_2M_30FPS_12BIT:
	case V4L2_SONY_IMX327_2L_MIPI_2M_30FPS_12BIT_WDR2TO1:
	case V4L2_SONY_IMX327_SUBLVDS_2M_30FPS_12BIT:
	case V4L2_SONY_IMX327_SUBLVDS_2M_30FPS_12BIT_WDR2TO1:
	case V4L2_SONY_IMX327_MIPI_2M_60FPS_12BIT:
	case V4L2_SONY_IMX334_MIPI_8M_30FPS_12BIT:
	case V4L2_SONY_IMX334_MIPI_8M_30FPS_12BIT_WDR2TO1:
	case V4L2_SONY_IMX335_MIPI_2M_30FPS_10BIT_WDR2TO1:
	case V4L2_SONY_IMX335_MIPI_4M_30FPS_12BIT:
	case V4L2_SONY_IMX335_MIPI_4M_30FPS_10BIT_WDR2TO1:
	case V4L2_SONY_IMX335_2L_MIPI_4M_30FPS_10BIT:
	case V4L2_SONY_IMX335_MIPI_4M_1600P_30FPS_12BIT:
	case V4L2_SONY_IMX335_MIPI_4M_1600P_30FPS_10BIT_WDR2TO1:
	case V4L2_SONY_IMX335_MIPI_5M_30FPS_12BIT:
	case V4L2_SONY_IMX335_MIPI_5M_30FPS_10BIT_WDR2TO1:
	case V4L2_SONY_IMX335_MIPI_2M_60FPS_10BIT:
	case V4L2_SONY_IMX335_MIPI_4M_60FPS_10BIT:
	case V4L2_SONY_IMX335_MIPI_5M_60FPS_10BIT:
	case V4L2_SONY_IMX347_MIPI_4M_60FPS_12BIT:
	case V4L2_SONY_IMX347_MIPI_4M_30FPS_12BIT_WDR2TO1:
	case V4L2_SONY_IMX385_MIPI_2M_30FPS_12BIT:
	case V4L2_SONY_IMX385_MIPI_2M_30FPS_12BIT_WDR2TO1:
	case V4L2_SONY_IMX412_MIPI_12M_30FPS_12BIT:
	case V4L2_SONY_IMX585_MIPI_8M_30FPS_12BIT:
	case V4L2_SONY_IMX585_MIPI_8M_25FPS_12BIT_WDR2TO1:
	// GalaxyCore
	case V4L2_GCORE_GC02M1_MIPI_2M_30FPS_10BIT:
	case V4L2_GCORE_GC1054_MIPI_1M_30FPS_10BIT:
	case V4L2_GCORE_GC2053_MIPI_2M_30FPS_10BIT:
	case V4L2_GCORE_GC2053_SLAVE_MIPI_2M_30FPS_10BIT:
	case V4L2_GCORE_GC2053_1L_MIPI_2M_30FPS_10BIT:
	case V4L2_GCORE_GC2093_MIPI_2M_30FPS_10BIT:
	case V4L2_GCORE_GC2093_MIPI_2M_30FPS_10BIT_WDR2TO1:
	case V4L2_GCORE_GC2093_SLAVE_MIPI_2M_30FPS_10BIT:
	case V4L2_GCORE_GC2093_SLAVE_MIPI_2M_30FPS_10BIT_WDR2TO1:
	case V4L2_GCORE_GC4023_MIPI_4M_30FPS_10BIT:
		stViDevAttr.enBayerFormat = BAYER_FORMAT_RG;
		break;
		// brigates
	case V4L2_BRIGATES_BG0808_MIPI_2M_30FPS_10BIT:
	case V4L2_BRIGATES_BG0808_MIPI_2M_30FPS_10BIT_WDR2TO1:
	case V4L2_SMS_SC4336_MIPI_4M_30FPS_10BIT:
	case V4L2_GCORE_GC4653_MIPI_4M_30FPS_10BIT:
	case V4L2_GCORE_GC4653_SLAVE_MIPI_4M_30FPS_10BIT:
	case V4L2_TECHPOINT_TP2850_MIPI_2M_30FPS_8BIT:
	case V4L2_TECHPOINT_TP2850_MIPI_4M_30FPS_8BIT:
	case V4L2_ONSEMI_AR2020_20M_25FPS_10BIT:
		stViDevAttr.enBayerFormat = BAYER_FORMAT_GR;
		break;
	case V4L2_SOI_K06_MIPI_4M_25FPS_10BIT:
		stViDevAttr.enBayerFormat = BAYER_FORMAT_GB;
		break;
	default:
		stViDevAttr.enBayerFormat = BAYER_FORMAT_BG;
		break;
	};

	// virtual channel for multi-ch
	switch (enSnsType) {
	case V4L2_PIXELPLUS_PR2100_2M_2CH_25FPS_8BIT:
	case V4L2_NEXTCHIP_N5_1M_2CH_25FPS_8BIT:
		stViDevAttr.enWorkMode = VI_WORK_MODE_2Multiplex;
		break;
	case V4L2_PIXELPLUS_PR2100_2M_4CH_25FPS_8BIT:
		stViDevAttr.enWorkMode = VI_WORK_MODE_4Multiplex;
		break;
	default:
		stViDevAttr.enWorkMode = VI_WORK_MODE_1Multiplex;
		break;
	}

	get_isp_attr_by_sensor(pipe, &stPubAttr);

	stViDevAttr.stSize.u32Width = stPubAttr.stWndRect.u32Width;
	stViDevAttr.stSize.u32Height = stPubAttr.stWndRect.u32Height;
	stViDevAttr.stWDRAttr.u32CacheLine = stPubAttr.stWndRect.u32Height;
	stViDevAttr.snrFps = (CVI_U32)stPubAttr.f32FrameRate;

	ret = CVI_VI_SetDevAttr(pipe, &stViDevAttr);

	if (ret != 0) {
		printf("pipe: %d, set dev attr fail!\n", pipe);
	} else {
		printf("pipe: %d, set dev attr success!\n", pipe);
	}

	return ret;
}

int CVI_ISP_V4L2_Init(int pipe, int fd)
{
	if (pipe >= VI_MAX_DEV_NUM || fd <= 0) {
		return -1;
	}

	CVI_SYS_Init();
	CVI_ISP_V4L2_SetFd(pipe, fd);
	set_dev_attr(pipe);
	reg_sensor(pipe);
	isp_init(pipe);
	sns_default_init(pipe);
	load_pqbin(pipe);
	isp_run(pipe);

	return 0;
}

int CVI_ISP_V4L2_Exit(int pipe)
{
	if (pipe >= VI_MAX_DEV_NUM) {
		return -1;
	}

	isp_stop(pipe);
	CVI_SYS_Exit();

	return 0;
}

