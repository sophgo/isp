/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: aealgo.c
 * Description:
 *
 */

#ifndef AAA_PC_PLATFORM

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#ifndef ARCH_RTOS_CV181X // TODO@CV181X
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <sys/time.h> //for gettimeofday()
#include <unistd.h> //for usleep()
#else
#include "FreeRTOS.h"
#include "task.h"
#include "time.h"
#endif
#include "ae_project_param.h"
#include "aealgo.h"
//#include "awbalgo.h"
#include "cvi_ae.h"
#include "cvi_isp.h"
#include "cvi_comm_3a.h"
#include "isp_main.h"
#include "ae_debug.h"
#include "isp_defines.h"
#include "isp_debug.h"
#include "ae_detect_flicker.h"
#include "ae_iris.h"
#include "ae_buf.h"

#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>

#include "cvi_type.h"
#include "cvi_comm_isp.h"
#include "cvi_comm_3a.h"

#include "ae_platform.h"

#include "aealgo.h"
#include "ae_common.h"
#include "ae_project_param.h"
#include "ae_detect_flicker.h"

#include "misc.h"
#endif

#define CHECK_AE_SIM	(0)

static void AE_SaveLog(CVI_U8 sID, const ISP_AE_RESULT_S *pstAeResult);

AE_CTX_S g_astAeCtx[MAX_AE_LIB_NUM] = { { 0 } };

SEXPOSURE_TV_SV_CURVE userDefineRoute[AE_SENSOR_NUM][AE_MAX_WDR_FRAME_NUM][ISP_AE_ROUTE_MAX_NODES];

SFACE_DETECT_INFO AeFaceDetect[AE_SENSOR_NUM];
SAE_INFO AeInfo[AE_SENSOR_NUM];
AE_FASTBOOT_EXPOSURE	stAeFastBootExposure[AE_SENSOR_NUM];
ISP_EXP_INFO_S stReplayExpInfo;
CVI_U8 u8AeDebugPrintMode[AE_SENSOR_NUM];
CVI_U8	u8SensorNum = 1;

//#define FRAME_ID_CHECK
CVI_BOOL bTableUpdateEnable[AE_SENSOR_NUM] = {0};
AE_RAW_REPLAY_EXP_INFO *pstRawReplayExpInfo[AE_SENSOR_NUM] = {CVI_NULL};
CVI_U16 aeRawReplayTableSize[AE_SENSOR_NUM] = {0};

CVI_BOOL bisSimpleAEB;
CVI_U8 u8SimpleAEB_Cnt;

s_AE_SIM_S sAe_sim;

#define LOG2_TB_SIZE 1024
const CVI_S16 Log2x100Tbl[LOG2_TB_SIZE] = {
	0,   0,	  100, 158, 200, 232, 258, 281, 300, 317,  332,	 346,  358, 370, 381, 391, 400, 409, 417, 425, 432, 439,
	446, 452, 458, 464, 470, 475, 481, 486, 491, 495,  500,	 504,  509, 513, 517, 521, 525, 529, 532, 536, 539, 543,
	546, 549, 552, 555, 558, 561, 564, 567, 570, 573,  575,	 578,  581, 583, 586, 588, 591, 593, 595, 598, 600, 602,
	604, 607, 609, 611, 613, 615, 617, 619, 621, 623,  625,	 627,  629, 630, 632, 634, 636, 638, 639, 641, 643, 644,
	646, 648, 649, 651, 652, 654, 655, 657, 658, 660,  661,	 663,  664, 666, 667, 669, 670, 671, 673, 674, 675, 677,
	678, 679, 681, 682, 683, 685, 686, 687, 688, 689,  691,	 692,  693, 694, 695, 697, 698, 699, 700, 701, 702, 703,
	704, 706, 707, 708, 709, 710, 711, 712, 713, 714,  715,	 716,  717, 718, 719, 720, 721, 722, 723, 724, 725, 726,
	727, 728, 729, 729, 730, 731, 732, 733, 734, 735,  736,	 737,  738, 738, 739, 740, 741, 742, 743, 743, 744, 745,
	746, 747, 748, 748, 749, 750, 751, 752, 752, 753,  754,	 755,  755, 756, 757, 758, 758, 759, 760, 761, 761, 762,
	763, 764, 764, 765, 766, 767, 767, 768, 769, 769,  770,	 771,  771, 772, 773, 773, 774, 775, 775, 776, 777, 777,
	778, 779, 779, 780, 781, 781, 782, 783, 783, 784,  785,	 785,  786, 786, 787, 788, 788, 789, 789, 790, 791, 791,
	792, 792, 793, 794, 794, 795, 795, 796, 797, 797,  798,	 798,  799, 799, 800, 801, 801, 802, 802, 803, 803, 804,
	804, 805, 806, 806, 807, 807, 808, 808, 809, 809,  810,	 810,  811, 811, 812, 812, 813, 813, 814, 814, 815, 815,
	816, 816, 817, 817, 818, 818, 819, 819, 820, 820,  821,	 821,  822, 822, 823, 823, 824, 824, 825, 825, 826, 826,
	827, 827, 828, 828, 829, 829, 829, 830, 830, 831,  831,	 832,  832, 833, 833, 834, 834, 834, 835, 835, 836, 836,
	837, 837, 838, 838, 838, 839, 839, 840, 840, 841,  841,	 841,  842, 842, 843, 843, 843, 844, 844, 845, 845, 846,
	846, 846, 847, 847, 848, 848, 848, 849, 849, 850,  850,	 850,  851, 851, 852, 852, 852, 853, 853, 854, 854, 854,
	855, 855, 855, 856, 856, 857, 857, 857, 858, 858,  858,	 859,  859, 860, 860, 860, 861, 861, 861, 862, 862, 863,
	863, 863, 864, 864, 864, 865, 865, 865, 866, 866,  867,	 867,  867, 868, 868, 868, 869, 869, 869, 870, 870, 870,
	871, 871, 871, 872, 872, 872, 873, 873, 873, 874,  874,	 874,  875, 875, 875, 876, 876, 876, 877, 877, 877, 878,
	878, 878, 879, 879, 879, 880, 880, 880, 881, 881,  881,	 882,  882, 882, 883, 883, 883, 884, 884, 884, 885, 885,
	885, 885, 886, 886, 886, 887, 887, 887, 888, 888,  888,	 889,  889, 889, 889, 890, 890, 890, 891, 891, 891, 892,
	892, 892, 892, 893, 893, 893, 894, 894, 894, 895,  895,	 895,  895, 896, 896, 896, 897, 897, 897, 897, 898, 898,
	898, 899, 899, 899, 899, 900, 900, 900, 901, 901,  901,	 901,  902, 902, 902, 903, 903, 903, 903, 904, 904, 904,
	904, 905, 905, 905, 906, 906, 906, 906, 907, 907,  907,	 907,  908, 908, 908, 908, 909, 909, 909, 910, 910, 910,
	910, 911, 911, 911, 911, 912, 912, 912, 912, 913,  913,	 913,  913, 914, 914, 914, 914, 915, 915, 915, 915, 916,
	916, 916, 916, 917, 917, 917, 917, 918, 918, 918,  918,	 919,  919, 919, 919, 920, 920, 920, 920, 921, 921, 921,
	921, 922, 922, 922, 922, 923, 923, 923, 923, 924,  924,	 924,  924, 925, 925, 925, 925, 926, 926, 926, 926, 926,
	927, 927, 927, 927, 928, 928, 928, 928, 929, 929,  929,	 929,  929, 930, 930, 930, 930, 931, 931, 931, 931, 932,
	932, 932, 932, 932, 933, 933, 933, 933, 934, 934,  934,	 934,  934, 935, 935, 935, 935, 936, 936, 936, 936, 936,
	937, 937, 937, 937, 938, 938, 938, 938, 938, 939,  939,	 939,  939, 939, 940, 940, 940, 940, 941, 941, 941, 941,
	941, 942, 942, 942, 942, 942, 943, 943, 943, 943,  943,	 944,  944, 944, 944, 945, 945, 945, 945, 945, 946, 946,
	946, 946, 946, 947, 947, 947, 947, 947, 948, 948,  948,	 948,  948, 949, 949, 949, 949, 949, 950, 950, 950, 950,
	950, 951, 951, 951, 951, 951, 952, 952, 952, 952,  952,	 953,  953, 953, 953, 953, 954, 954, 954, 954, 954, 954,
	955, 955, 955, 955, 955, 956, 956, 956, 956, 956,  957,	 957,  957, 957, 957, 958, 958, 958, 958, 958, 958, 959,
	959, 959, 959, 959, 960, 960, 960, 960, 960, 961,  961,	 961,  961, 961, 961, 962, 962, 962, 962, 962, 963, 963,
	963, 963, 963, 963, 964, 964, 964, 964, 964, 965,  965,	 965,  965, 965, 965, 966, 966, 966, 966, 966, 967, 967,
	967, 967, 967, 967, 968, 968, 968, 968, 968, 968,  969,	 969,  969, 969, 969, 970, 970, 970, 970, 970, 970, 971,
	971, 971, 971, 971, 971, 972, 972, 972, 972, 972,  972,	 973,  973, 973, 973, 973, 973, 974, 974, 974, 974, 974,
	974, 975, 975, 975, 975, 975, 975, 976, 976, 976,  976,	 976,  976, 977, 977, 977, 977, 977, 977, 978, 978, 978,
	978, 978, 978, 979, 979, 979, 979, 979, 979, 980,  980,	 980,  980, 980, 980, 981, 981, 981, 981, 981, 981, 982,
	982, 982, 982, 982, 982, 982, 983, 983, 983, 983,  983,	 983,  984, 984, 984, 984, 984, 984, 985, 985, 985, 985,
	985, 985, 985, 986, 986, 986, 986, 986, 986, 987,  987,	 987,  987, 987, 987, 987, 988, 988, 988, 988, 988, 988,
	989, 989, 989, 989, 989, 989, 989, 990, 990, 990,  990,	 990,  990, 991, 991, 991, 991, 991, 991, 991, 992, 992,
	992, 992, 992, 992, 992, 993, 993, 993, 993, 993,  993,	 994,  994, 994, 994, 994, 994, 994, 995, 995, 995, 995,
	995, 995, 995, 996, 996, 996, 996, 996, 996, 996,  997,	 997,  997, 997, 997, 997, 997, 998, 998, 998, 998, 998,
	998, 998, 999, 999, 999, 999, 999, 999, 999, 1000, 1000, 1000,
};


static void SetReplayExposure(CVI_U8 sID);

char *AE_GetTimestampStr(void)
{
	static char str[14];

	struct tm tm;
	struct timeval time;

	gettimeofday(&time, NULL);
	localtime_r(&time.tv_sec, &tm);

	snprintf(str, sizeof(str), "%02d:%02d:%02d.%03d ",
		tm.tm_hour,
		tm.tm_min,
		tm.tm_sec,
		(CVI_U32) (time.tv_usec / 1000));

	return str;
}

CVI_U8 AE_CheckSensorID(CVI_U8 sID)
{
	return (sID < AE_SENSOR_NUM) ? sID : AE_SENSOR_NUM - 1;
}

CVI_S32 AE_Interpolation(CVI_S32 *Data, CVI_S32 x)
{
//Data[0]: x0 , Data[1]: x1 , Data[2] : y0 , Data[3] : y1
	CVI_S32 range = Data[1] - Data[0];
	CVI_S32 result;

	if (x < Data[0])
		result = Data[2];
	else if (x > Data[1])
		result = Data[3];
	else
		result = (range) ? (Data[3] - Data[2]) * (x - Data[0]) / range + Data[2] : Data[2];

	return result;
}

CVI_S32  U16_Comp_SortBigToSmall(const void *v1, const void *v2)
{
	const CVI_U16 *p1 = v1;
	const CVI_U16 *p2 = v2;

	if (*p1 < *p2)
		return 1;
	else if (*p1 == *p2)
		return 0;
	else
		return -1;
}

CVI_S32  U8_Comp_SortBigToSmall(const void *v1, const void *v2)
{
	const CVI_U8 *p1 = v1;
	const CVI_U8 *p2 = v2;

	if (*p1 < *p2)
		return 1;
	else if (*p1 == *p2)
		return 0;
	else
		return -1;
}

static inline CVI_FLOAT AE_CalculatePow2(CVI_S16 entry)
{
	return pow(2, (CVI_FLOAT)entry / ENTRY_PER_EV);
}

static inline CVI_FLOAT AE_CalculateLog2(CVI_U32 num, CVI_U32 den)
{
	if (num == 0)
		num = 1;
	if (den == 0)
		den = 1;
	return log((CVI_FLOAT)num / den) / log(2);
}

void AE_SetFastBootExposure(VI_PIPE ViPipe, CVI_U32 expLine, CVI_U32 again, CVI_U32 dgain, CVI_U32 ispdgain)
{
	CVI_U8 sID = AE_ViPipe2sID(ViPipe);

	sID = AE_CheckSensorID(sID);
	stAeFastBootExposure[sID].u32ExpLine = expLine;
	stAeFastBootExposure[sID].stGain.u32AGain = again;
	stAeFastBootExposure[sID].stGain.u32DGain = dgain;
	stAeFastBootExposure[sID].stGain.u32ISPDGain = ispdgain;
}

void AE_GetFastBootExposure(VI_PIPE ViPipe, CVI_U32 expLine, CVI_U32 again, CVI_U32 dgain, CVI_U32 ispdgain)
{
	CVI_U8 sID = AE_ViPipe2sID(ViPipe);

	sID = AE_CheckSensorID(sID);
	stAeFastBootExposure[sID].u32ExpLine = expLine;
	stAeFastBootExposure[sID].stGain.u32AGain = again;
	stAeFastBootExposure[sID].stGain.u32DGain = dgain;
	stAeFastBootExposure[sID].stGain.u32ISPDGain = ispdgain;
}

void AE_GetFastBootExposureEntry(CVI_U8 sID, CVI_S16 *tvEntry, CVI_S16 *svEntry)
{
	CVI_U32 time, gain;

	sID = AE_CheckSensorID(sID);

	if (!stAeFastBootExposure[sID].u32ExpLine) {
		*tvEntry = 0;
		*svEntry = 0;
		return;
	}
	AAA_LIMIT(stAeFastBootExposure[sID].u32ExpLine,
		AeInfo[sID].u32ExpLineMin, AeInfo[sID].u32ExpLineMax);
	AAA_LIMIT(stAeFastBootExposure[sID].stGain.u32AGain,
		AeInfo[sID].u32AGainMin, AeInfo[sID].u32AGainMax);
	AAA_LIMIT(stAeFastBootExposure[sID].stGain.u32DGain,
		AeInfo[sID].u32DGainMin, AeInfo[sID].u32DGainMax);
	AAA_LIMIT(stAeFastBootExposure[sID].stGain.u32ISPDGain,
		AeInfo[sID].u32ISPDGainMin, AeInfo[sID].u32ISPDGainMax);

	gain = AE_CalTotalGain(stAeFastBootExposure[sID].stGain.u32AGain,
		stAeFastBootExposure[sID].stGain.u32DGain,
		stAeFastBootExposure[sID].stGain.u32ISPDGain);

	AE_GetExpTimeByLine(sID, stAeFastBootExposure[sID].u32ExpLine, &time);
	AE_GetTvEntryByTime(sID, time, tvEntry);
	AE_GetISOEntryByGain(gain, svEntry);
}

void AE_CalculateFrameLuma(CVI_U8 sID)
{
	CVI_U16 gridLuma[AE_MAX_WDR_FRAME_NUM], frameLuma[AE_MAX_WDR_FRAME_NUM];
	CVI_U32 frameGridLumaSum[AE_MAX_WDR_FRAME_NUM] = {0};
	CVI_U16 row, column;
	CVI_U16 i, j, weightSum[AE_MAX_WDR_FRAME_NUM] = {0};
	CVI_U16 tmpMaxLuma[AE_MAX_WDR_FRAME_NUM] = {0}, tmpMinLuma[AE_MAX_WDR_FRAME_NUM] = { 0xffff, 0xffff };
	CVI_U16 gridCnt[AE_MAX_WDR_FRAME_NUM] = {0};
	CVI_U32 hisSum[AE_MAX_WDR_FRAME_NUM] = {0};
	CVI_U32 frameGridLvLumaSum[AE_MAX_WDR_FRAME_NUM] = {0};
	CVI_U16 frameGridLvLuma[AE_MAX_WDR_FRAME_NUM] = {0};
	CVI_U16 gridTotal = 0;
	CVI_U32	highLightCnt[AE_MAX_WDR_FRAME_NUM][TOTAL_BASE] = {0},
			highLightBufCnt[AE_MAX_WDR_FRAME_NUM][TOTAL_BASE] = {0},
			lowLightCnt[AE_MAX_WDR_FRAME_NUM][TOTAL_BASE] = {0},
			lowLightBufCnt[AE_MAX_WDR_FRAME_NUM][TOTAL_BASE] = {0};
	CVI_U16	gridWeightRatio;
	CVI_U8	weight = 0, subVNum, subHNum;
	CVI_U16 highLightLumaThr, lowLightLumaThr, highLightBufLumaThr, lowLightBufLumaThr;
	CVI_BOOL statisticsDivide = 0;
	CVI_U16 RValue[AE_MAX_WDR_FRAME_NUM], GValue[AE_MAX_WDR_FRAME_NUM], BValue[AE_MAX_WDR_FRAME_NUM];

	sID = AE_CheckSensorID(sID);

	gridTotal = AeInfo[sID].u8GridVNum * AeInfo[sID].u8GridHNum;
	if (gridTotal == 0)
		gridTotal = 1;

	highLightLumaThr = pstAeExposureAttrInfo[sID]->stAuto.u8HighLightLumaThr << 2;
	highLightBufLumaThr = pstAeExposureAttrInfo[sID]->stAuto.u8HighLightBufLumaThr << 2;
	lowLightLumaThr = pstAeExposureAttrInfo[sID]->stAuto.u8LowLightLumaThr << 2;
	lowLightBufLumaThr = pstAeExposureAttrInfo[sID]->stAuto.u8LowLightBufLumaThr << 2;

	if (highLightBufLumaThr > highLightLumaThr * 95 / 100)
		highLightBufLumaThr = highLightLumaThr * 95 / 100;

	if (lowLightBufLumaThr < lowLightLumaThr * 105 / 100)
		lowLightBufLumaThr = lowLightLumaThr * 105 / 100;

	subVNum = AeInfo[sID].u8GridVNum / AE_WEIGHT_ZONE_ROW;
	subHNum = AeInfo[sID].u8GridHNum / AE_WEIGHT_ZONE_COLUMN;

	statisticsDivide = (subVNum > 1 || subHNum > 1) ? 1 : 0;

	for (row = 0; row < AeInfo[sID].u8GridVNum; row++) {
		for (column = 0; column < AeInfo[sID].u8GridHNum; column++) {

			if (statisticsDivide) {
				weight = pstAeStatisticsCfgInfo[sID]->au8Weight[row / subVNum][column / subHNum];
			} else {
				weight = pstAeStatisticsCfgInfo[sID]->au8Weight[row][column];
			}

			gridWeightRatio = (weight * 1000 + AeInfo[sID].u16WeightSum / 2) /
								AAA_DIV_0_TO_1(AeInfo[sID].u16WeightSum);

			for (i = 0; i < AeInfo[sID].u8AeMaxFrameNum; ++i) {
				RValue[i] = AeInfo[sID].pu16AeStatistics[i][row][column][3];
				GValue[i] = (AeInfo[sID].pu16AeStatistics[i][row][column][1] +
					  AeInfo[sID].pu16AeStatistics[i][row][column][2]) / 2;
				BValue[i] = AeInfo[sID].pu16AeStatistics[i][row][column][0];

				if (RValue[i] > AE_MAX_LUMA)
					RValue[i] = AE_MAX_LUMA;

				if (GValue[i] > AE_MAX_LUMA)
					GValue[i] = AE_MAX_LUMA;

				if (BValue[i] > AE_MAX_LUMA)
					BValue[i] = AE_MAX_LUMA;

				if (RValue[i] > GValue[i])
					gridLuma[i] = AAA_MAX(RValue[i], BValue[i]);
				else
					gridLuma[i] = AAA_MAX(GValue[i], BValue[i]);

				if (gridLuma[i] > tmpMaxLuma[i])
					tmpMaxLuma[i] = gridLuma[i];
				else if (gridLuma[i] < tmpMinLuma[i])
					tmpMinLuma[i] = gridLuma[i];

				if (gridWeightRatio >= HIGH_LIGHT_ZONE_WEIGHT_RATIO_THR) {
					if (gridLuma[i] > highLightBufLumaThr)
						highLightBufCnt[i][GRID_BASE]++;

					if (gridLuma[i] > highLightLumaThr)
						highLightCnt[i][GRID_BASE]++;

					if (gridLuma[i] < lowLightBufLumaThr)
						lowLightBufCnt[i][GRID_BASE]++;

					if (gridLuma[i] < lowLightLumaThr)
						lowLightCnt[i][GRID_BASE]++;
				}
			}

			if (AeInfo[sID].bWDRMode) {
				frameGridLumaSum[AE_LE] += (gridLuma[AE_LE] * weight);
				weightSum[AE_LE] += weight;
				gridCnt[AE_LE]++;

				frameGridLumaSum[AE_SE] += (gridLuma[AE_SE] * weight);
				weightSum[AE_SE] += weight;
				gridCnt[AE_SE]++;

				frameGridLvLumaSum[AE_LE] += gridLuma[AE_LE];
				AeInfo[sID].u16MeterLuma[AE_LE][row][column] = gridLuma[AE_LE];

				frameGridLvLumaSum[AE_SE] += gridLuma[AE_SE];
				AeInfo[sID].u16MeterLuma[AE_SE][row][column] = gridLuma[AE_SE];

			} else {
				frameGridLumaSum[AE_LE] += (gridLuma[AE_LE] * weight);
				weightSum[AE_LE] += weight;
				gridCnt[AE_LE]++;

				frameGridLvLumaSum[AE_LE] += gridLuma[AE_LE];
				AeInfo[sID].u16MeterLuma[AE_LE][row][column] = gridLuma[AE_LE];
			}
		}
	}

	for (i = 0; i < AeInfo[sID].u8AeMaxFrameNum; ++i) {

		frameLuma[i] = frameGridLumaSum[i] / AAA_DIV_0_TO_1(weightSum[i]);
		frameGridLvLuma[i] = frameGridLvLumaSum[i] / AAA_DIV_0_TO_1(gridCnt[i]);

		AeInfo[sID].u16FrameLuma[i] = frameLuma[i];
		AeInfo[sID].u16MeterMaxLuma[i] = tmpMaxLuma[i];
		AeInfo[sID].u16MeterMinLuma[i] = tmpMinLuma[i];
		AeInfo[sID].u16LvFrameLuma[i] = frameGridLvLuma[i];
		AeInfo[sID].u8MeterGridCnt[i] = gridCnt[i];

		AeInfo[sID].u16HighRatio[i][GRID_BASE] = highLightCnt[i][GRID_BASE];
		AeInfo[sID].u16HighBufRatio[i][GRID_BASE] = highLightBufCnt[i][GRID_BASE];
		AeInfo[sID].u16LowRatio[i][GRID_BASE] = lowLightCnt[i][GRID_BASE];
		AeInfo[sID].u16LowBufRatio[i][GRID_BASE] = lowLightBufCnt[i][GRID_BASE];

		for (j = 0; j < HIST_NUM; ++j) { // TODO: mason.zou
			hisSum[i] += AeInfo[sID].pu32Histogram[i][j];

			if (j < pstAeExposureAttrInfo[sID]->stAuto.u8LowLightLumaThr)
				lowLightCnt[i][HISTOGRAM_BASE] += AeInfo[sID].pu32Histogram[i][j];
			if (j < pstAeExposureAttrInfo[sID]->stAuto.u8LowLightBufLumaThr)
				lowLightBufCnt[i][HISTOGRAM_BASE] += AeInfo[sID].pu32Histogram[i][j];

			if (j > pstAeExposureAttrInfo[sID]->stAuto.u8HighLightLumaThr)
				highLightCnt[i][HISTOGRAM_BASE] += AeInfo[sID].pu32Histogram[i][j];
			if (j > pstAeExposureAttrInfo[sID]->stAuto.u8HighLightBufLumaThr)
				highLightBufCnt[i][HISTOGRAM_BASE] += AeInfo[sID].pu32Histogram[i][j];
		}

		hisSum[i] = AAA_DIV_0_TO_1(hisSum[i]);
		AeInfo[sID].u16HighRatio[i][HISTOGRAM_BASE] = highLightCnt[i][HISTOGRAM_BASE] * 1000 / hisSum[i];
		AeInfo[sID].u16HighBufRatio[i][HISTOGRAM_BASE] = highLightBufCnt[i][HISTOGRAM_BASE] * 1000 / hisSum[i];
		AeInfo[sID].u16LowRatio[i][HISTOGRAM_BASE] = lowLightCnt[i][HISTOGRAM_BASE] * 1000 / hisSum[i];
		AeInfo[sID].u16LowBufRatio[i][HISTOGRAM_BASE] = lowLightBufCnt[i][HISTOGRAM_BASE] * 1000 / hisSum[i];
	}
}

void AE_UpdateSmoothBvEntry(CVI_U8 sID, CVI_BOOL *status)
{
	CVI_U16 absBvStep = AAA_ABS(AeInfo[sID].s16BvStepEntry[AE_LE]);
	static CVI_U8 convergeStatus[AE_SENSOR_NUM];
	static CVI_U8 convergeStatusFrmCnt[AE_SENSOR_NUM];

	sID = AE_CheckSensorID(sID);

	if (AeInfo[sID].u16FrameLuma[AE_LE] > 1000) {
		if (convergeStatus[sID] == 0 && convergeStatusFrmCnt[sID] == 0) {
			convergeStatus[sID] = 1;
		}
	} else if (convergeStatus[sID] == 2 && absBvStep < ENTRY_PER_EV) {
		convergeStatusFrmCnt[sID] = 0;
		convergeStatus[sID] = 0;
	}

	if (convergeStatus[sID] == 1) {
		convergeStatusFrmCnt[sID]++;
		if (convergeStatusFrmCnt[sID] > AeInfo[sID].u8MeterFramePeriod) {
			convergeStatus[sID] = 2;
		}
		*status = convergeStatus[sID];
	}
}

void AE_CalculateBvStep(CVI_U8 sID)
{
	CVI_U8 i;
	CVI_S32 tmpBVStep = 0;
	const CVI_U16 AE_LV_TARGET = 224;
	CVI_U16 tmpFrameLuma = 0, tmpLVFrameLuma = 0;

	sID = AE_CheckSensorID(sID);

	for (i = 0; i < AeInfo[sID].u8AeMaxFrameNum; ++i) {
		tmpFrameLuma = AAA_MAX(AE_METER_MIN_LUMA, AeInfo[sID].u16FrameLuma[i]);
		tmpBVStep =
			(Log2x100Tbl[tmpFrameLuma] - Log2x100Tbl[AeInfo[sID].u16AdjustTargetLuma[i]]) *
			ENTRY_PER_BV;
		tmpBVStep = (tmpBVStep > 0) ? ((tmpBVStep + 50) / 100) : ((tmpBVStep - 50) / 100);
		AeInfo[sID].s16lumaBvStepEntry[i] = tmpBVStep;
		AeInfo[sID].s16BvStepEntry[i] = AeInfo[sID].s16lumaBvStepEntry[i];

		tmpLVFrameLuma = AAA_MAX(AE_METER_MIN_LUMA, AeInfo[sID].u16LvFrameLuma[i]);
		tmpBVStep = (Log2x100Tbl[tmpLVFrameLuma] - Log2x100Tbl[AE_LV_TARGET]) * ENTRY_PER_BV;
		tmpBVStep = (tmpBVStep > 0) ? ((tmpBVStep + 50) / 100) : ((tmpBVStep - 50) / 100);
		AeInfo[sID].s16LvBvStep[i] = tmpBVStep;
	}
}

void AE_CalculateSmoothBvStep(CVI_U8 sID)
{
	CVI_U8 i, frameIndex = 0, preFrameIndex = 0;
	CVI_BOOL updateSmoothBvStep = 0;
	CVI_BOOL upStatus[AE_MAX_WDR_FRAME_NUM] = {0};
	CVI_S32 bsDiff = 0, tmpDiff;

	sID = AE_CheckSensorID(sID);

	if (AeInfo[sID].u8ConvergeMode[AE_LE] == CONVERGE_FAST)
		return;

	for (i = 0; i < AeInfo[sID].u8AeMaxFrameNum; ++i) {
		AeInfo[sID].s16SmoothBvStepEntry[i] = AeInfo[sID].s16BvStepEntry[i];

		if (AeInfo[sID].bMeterEveryFrame) {
			if (AeInfo[sID].u8AERunInterval < AeInfo[sID].u8MeterFramePeriod &&
				AeInfo[sID].u32MeterFrameCnt >= AeInfo[sID].u8MeterFramePeriod) {
				frameIndex = AeInfo[sID].u32MeterFrameCnt % AeInfo[sID].u8MeterFramePeriod;
				preFrameIndex = (AeInfo[sID].u32MeterFrameCnt - 1) % AeInfo[sID].u8MeterFramePeriod;
				updateSmoothBvStep = 1;
			}

			if (updateSmoothBvStep) {
				if (AeInfo[sID].stSmoothApex[i][frameIndex].s16BVEntry != BV_AUTO_ENTRY &&
					AeInfo[sID].stSmoothApex[i][preFrameIndex].s16BVEntry != BV_AUTO_ENTRY) {
					AeInfo[sID].as16SmoothBv[i][0] =
						AeInfo[sID].stSmoothApex[i][preFrameIndex].s16BVEntry;
					AeInfo[sID].as16SmoothBv[i][1] =
						AeInfo[sID].stSmoothApex[i][frameIndex].s16BVEntry;
					bsDiff = (AeInfo[sID].stSmoothApex[i][preFrameIndex].s16BVEntry
						- AeInfo[sID].stSmoothApex[i][frameIndex].s16BVEntry);

					tmpDiff = AeInfo[sID].s16BvStepEntry[i] - bsDiff;
					if (AeInfo[sID].s16BvStepEntry[i] * tmpDiff >= 0) {
						bsDiff = (AAA_ABS(AeInfo[sID].s16BvStepEntry[i]) >= AAA_ABS(tmpDiff)) ?
							bsDiff : 0;
					} else {
						bsDiff = AeInfo[sID].s16BvStepEntry[i];
					}

					#if 0
					printf("fid:%d idx:%d pdx:%d mid:%d bv:%d pbv:%d bsd:%d bs:%d\n",
						AeInfo[sID].u32frmCnt, frameIndex, preFrameIndex,
						AeInfo[sID].u32MeterFrameCnt,
						AeInfo[sID].stSmoothApex[i][frameIndex].s16BVEntry,
						AeInfo[sID].stSmoothApex[i][preFrameIndex].s16BVEntry, bsDiff,
						AeInfo[sID].s16BvStepEntry[i]);
					#endif
				}
			}

			if (AeInfo[sID].u32frmCnt <= AeInfo[sID].u8BootWaitFrameNum + AeInfo[sID].u8MeterFramePeriod) {
				//for max 1/4 step
				bsDiff = AeInfo[sID].s16BvStepEntry[i] * (AeInfo[sID].u8MeterFramePeriod - 1) /
						AeInfo[sID].u8MeterFramePeriod;
			}

			if (i == AE_LE) {
				AE_UpdateSmoothBvEntry(sID, &upStatus[i]);
			}

			if (upStatus[i] == 1 && AeInfo[sID].u8MeterFramePeriod > 1) {

				bsDiff = AAA_MAX(bsDiff, AeInfo[sID].s16BvStepEntry[i]);
				//for max 1/4 step
				bsDiff = bsDiff * (AeInfo[sID].u8MeterFramePeriod - 1) /
						AeInfo[sID].u8MeterFramePeriod;
			}
		} else if (AeInfo[sID].bEnableSmoothAE) {
			if (AeInfo[sID].stSmoothApex[i][0].s16BVEntry != BV_AUTO_ENTRY &&
				AeInfo[sID].u8ConvergeMode[i] != CONVERGE_FAST) {
				bsDiff = (AeInfo[sID].stSmoothApex[i][AeInfo[sID].u8MeterFramePeriod-1].s16BVEntry
					- AeInfo[sID].stSmoothApex[i][0].s16BVEntry);
			}
		}

		AeInfo[sID].s16SmoothBvStepEntry[i] -= bsDiff;
		AeInfo[sID].s16LvBvStep[i] -= bsDiff;


	}
}

void AE_UpdateFps(CVI_U8 sID, CVI_U32 expTime)
{
	CVI_FLOAT fps = AeInfo[sID].fIspPubAttrFps;

	if (AeInfo[sID].bAeFpsChange) {
		expTime = AAA_MIN(expTime, pstAeExposureAttrInfo[sID]->stAuto.stExpTimeRange.u32Max);
		fps = (CVI_FLOAT)1000000 / AAA_DIV_0_TO_1(expTime);
		AeInfo[sID].fSlowShutterFps = AAA_MIN(fps, AeInfo[sID].fIspPubAttrFps);
		fps = AeInfo[sID].fSlowShutterFps;
	}

	AE_SetFps(sID, fps);
}

void AE_RunAlgo(CVI_U8 sID)
{
#if CHECK_AE_SIM
	AE_HLprintf("RUN____AE\n");
#endif
	sID = AE_CheckSensorID(sID);

	if (AeInfo[sID].stStitchAttr.enable && !AeInfo[sID].stStitchAttr.bMainPipe &&
			pstAeExposureAttrInfo[sID]->enOpType != OP_TYPE_MANUAL) {
		CVI_U8 mainID = AE_SENSOR_NUM, i;

		for (i = 0; i < AE_SENSOR_NUM; i++) {
			if (AeInfo[i].stStitchAttr.bMainPipe &&
					AeInfo[i].stStitchAttr.u8Group == AeInfo[sID].stStitchAttr.u8Group) {
				mainID = i;
				break;
			}
		}

		if (mainID != AE_SENSOR_NUM) {
			memcpy(AeInfo[sID].stExp, AeInfo[mainID].stExp, sizeof(AE_EXPOSURE) * AE_MAX_WDR_FRAME_NUM);
			AE_UpdateFps(sID, AeInfo[sID].stExp[AE_LE].u32ExpTime);
			return;
		}
	}
	AE_CalculateFrameLuma(sID);
	AE_AdjustTargetY(sID);
	AE_CalculateBvStep(sID);
	AE_CalFaceDetectLuma(sID);
	AE_CalFaceDetectBvStep(sID);
	AE_CalculateSmoothBvStep(sID);
	AE_CalCurrentLvX100(sID);
	if (AE_IsDCIris(sID)) {
		AE_RunDCIris(sID, pstAeExposureAttrInfo[sID]->enOpType);
	}

	if (pstAeExposureAttrInfo[sID]->enOpType != OP_TYPE_MANUAL) {
		if (!AE_IsUseDcIris(sID)) {
			AE_CalculateConverge(sID);
			AE_ConfigExposureRoute(sID);
			if (AeInfo[sID].bEnableSmoothAE && !AeInfo[sID].bMeterEveryFrame)
				AE_ConfigSmoothExposure(sID);
			AE_ConfigExposure(sID);
		}
	}
	AE_UpdateFps(sID, AeInfo[sID].stExp[AE_LE].u32ExpTime);
}

void AE_GetBVEntryLimit(CVI_U8 sID, CVI_S16 *MinEntry, CVI_S16 *MaxEntry)
{
	sID = AE_CheckSensorID(sID);

	*MinEntry = AeInfo[sID].s16MinTVEntry - AeInfo[sID].s16MaxISOEntry;
	*MaxEntry = AeInfo[sID].s16MaxTVEntry - AeInfo[sID].s16MinISOEntry;
}

void AE_SetTvEntryLimit(CVI_U8 sID, CVI_S16 MinEntry, CVI_S16 MaxEntry)
{
	sID = AE_CheckSensorID(sID);

	AeInfo[sID].s16MinTVEntry = MinEntry;
	AeInfo[sID].s16MaxTVEntry = MaxEntry;
}

void AE_GetTVEntryLimit(CVI_U8 sID, CVI_S16 *MinEntry, CVI_S16 *MaxEntry)
{
	CVI_S16 minTvEntry, maxTvEntry, minRouteTvEntry, maxRouteTvEntry;

	sID = AE_CheckSensorID(sID);

	minRouteTvEntry = (pstAeExposureAttrInfo[sID]->bAERouteExValid) ? AeInfo[sID].s16MinRouteExTVEntry :
		AeInfo[sID].s16MinRouteTVEntry;
	maxRouteTvEntry = (pstAeExposureAttrInfo[sID]->bAERouteExValid) ? AeInfo[sID].s16MaxRouteExTVEntry :
		AeInfo[sID].s16MaxRouteTVEntry;


	if (AeInfo[sID].enSlowShutterStatus == AE_ENTER_SLOW_SHUTTER) {
		AeInfo[sID].s16MinTVEntry = AAA_MAX(AeInfo[sID].s16MinRangeTVEntry,
			AeInfo[sID].s16MinFpsSensorTVEntry);
	} else {
		minTvEntry = AAA_MAX(AeInfo[sID].s16MinRangeTVEntry, minRouteTvEntry);
		AeInfo[sID].s16MinTVEntry = AAA_MAX(minTvEntry, AeInfo[sID].s16MinSensorTVEntry);
	}

	maxTvEntry = AAA_MIN(AeInfo[sID].s16MaxRangeTVEntry, maxRouteTvEntry);
	AeInfo[sID].s16MaxTVEntry = AAA_MIN(maxTvEntry, AeInfo[sID].s16MaxSensorTVEntry);
	*MinEntry = AeInfo[sID].s16MinTVEntry;
	*MaxEntry = AeInfo[sID].s16MaxTVEntry;
}

void AE_SetISOEntryLimit(CVI_U8 sID, CVI_S16 MinEntry, CVI_S16 MaxEntry)
{
	sID = AE_CheckSensorID(sID);

	AeInfo[sID].s16MinISOEntry = MinEntry;
	AeInfo[sID].s16MaxISOEntry = MaxEntry;
}

void AE_GetISOEntryLimit(CVI_U8 sID, CVI_S16 *MinEntry, CVI_S16 *MaxEntry)
{
	CVI_S16 minISOEntry, maxISOEntry, minRouteISOEntry, maxRouteISOEntry;

	sID = AE_CheckSensorID(sID);
	minRouteISOEntry = (pstAeExposureAttrInfo[sID]->bAERouteExValid) ? AeInfo[sID].s16MinRouteExISOEntry :
		AeInfo[sID].s16MinRouteISOEntry;
	maxRouteISOEntry = (pstAeExposureAttrInfo[sID]->bAERouteExValid) ? AeInfo[sID].s16MaxRouteExISOEntry :
		AeInfo[sID].s16MaxRouteISOEntry;

	minISOEntry = AAA_MAX(AeInfo[sID].s16MinRangeISOEntry, minRouteISOEntry);
	maxISOEntry = AAA_MIN(AeInfo[sID].s16MaxRangeISOEntry, maxRouteISOEntry);

	AeInfo[sID].s16MinISOEntry = AAA_MAX(minISOEntry, AeInfo[sID].s16MinSensorISOEntry);
	AeInfo[sID].s16MaxISOEntry = AAA_MIN(maxISOEntry, AeInfo[sID].s16MaxSensorISOEntry);
	*MinEntry = AeInfo[sID].s16MinISOEntry;
	*MaxEntry = AeInfo[sID].s16MaxISOEntry;
}

CVI_S16 AE_LimitBVEntry(CVI_U8 sID, CVI_S16 BvEntry)
{
	CVI_S16 minBvEntry, maxBvEntry;

	sID = AE_CheckSensorID(sID);
	AE_GetBVEntryLimit(sID, &minBvEntry, &maxBvEntry);
	AAA_LIMIT(BvEntry, minBvEntry, maxBvEntry);
	return BvEntry;
}

CVI_S16 AE_LimitTVEntry(CVI_U8 sID, CVI_S16 TvEntry)
{
	CVI_S16 minTvEntry, maxTvEntry;

	sID = AE_CheckSensorID(sID);
	AE_GetTVEntryLimit(sID, &minTvEntry, &maxTvEntry);
	AAA_LIMIT(TvEntry, minTvEntry, maxTvEntry);
	return TvEntry;
}

CVI_S16 AE_LimitISOEntry(CVI_U8 sID, CVI_S16 ISOEntry)
{
	CVI_S16 minISOEntry, maxISOEntry;

	sID = AE_CheckSensorID(sID);
	AE_GetISOEntryLimit(sID, &minISOEntry, &maxISOEntry);
	AAA_LIMIT(ISOEntry, minISOEntry, maxISOEntry);
	return ISOEntry;
}


CVI_BOOL AE_TargetParameterUpdate(CVI_U8 sID)
{
	CVI_BOOL paramUpdate = 0;
	static CVI_U8	maxHisOffset[AE_SENSOR_NUM];
	static CVI_U8	compensation[AE_SENSOR_NUM];

	if (maxHisOffset[sID] != pstAeExposureAttrInfo[sID]->stAuto.u8MaxHistOffset) {
		maxHisOffset[sID] = pstAeExposureAttrInfo[sID]->stAuto.u8MaxHistOffset;
		paramUpdate = 1;
	}

	if (compensation[sID] != pstAeExposureAttrInfo[sID]->stAuto.u8Compensation) {
		compensation[sID] = pstAeExposureAttrInfo[sID]->stAuto.u8Compensation;
		paramUpdate = 1;
	}

	return paramUpdate;
}

void AE_GetTargetRange(CVI_U8 sID, CVI_S16 LvX100, CVI_S32 *ptargetMin, CVI_S32 *ptargetMax)
{
	CVI_S32 targetMinSeg[AE_MAX_WDR_FRAME_NUM][4], targetMaxSeg[AE_MAX_WDR_FRAME_NUM][4];
	CVI_U8 targetIndexByLv;
	CVI_S16 leTargetLumaOffset = 0, seTargetLumaOffset = 0, targetLumaOffset = 0;
	CVI_S16 minLVx100, maxLVx100;

	minLVx100 = MIN_LV * 100;
	maxLVx100 = MAX_LV * 100;
	if (AeInfo[sID].bWDRMode) {
		leTargetLumaOffset = pstAeExposureAttrInfo[sID]->stAuto.u8Compensation - AE_DEFAULT_TARGET_LUMA;
		seTargetLumaOffset = pstAeWDRExposureAttrInfo[sID]->u8SECompensation - AE_DEFAULT_TARGET_LUMA;
		if (LvX100 <= minLVx100 || LvX100 >= maxLVx100) {
			targetIndexByLv = (LvX100 <= minLVx100) ? 0 : LV_TOTAL_NUM-1;
			ptargetMin[AE_LE] =
				pstAeWDRExposureAttrInfo[sID]->au8LEAdjustTargetMin[targetIndexByLv] +
				leTargetLumaOffset;
			ptargetMax[AE_LE] =
				pstAeWDRExposureAttrInfo[sID]->au8LEAdjustTargetMax[targetIndexByLv] +
				leTargetLumaOffset;
			ptargetMin[AE_SE] =
				pstAeWDRExposureAttrInfo[sID]->au8SEAdjustTargetMin[targetIndexByLv] +
				seTargetLumaOffset;
			ptargetMax[AE_SE] =
				pstAeWDRExposureAttrInfo[sID]->au8SEAdjustTargetMax[targetIndexByLv] +
				seTargetLumaOffset;
		} else {
			targetIndexByLv = (LvX100 - minLVx100) / 100;
			targetMinSeg[AE_LE][0] = AAA_ABS(LvX100) / 100 * 100;
			targetMinSeg[AE_LE][1] = AAA_ABS(LvX100) / 100 * 100 + 100;
			targetMinSeg[AE_LE][2] =
				pstAeWDRExposureAttrInfo[sID]->au8LEAdjustTargetMin[targetIndexByLv] +
				leTargetLumaOffset;
			targetMinSeg[AE_LE][3] =
				pstAeWDRExposureAttrInfo[sID]->au8LEAdjustTargetMin[targetIndexByLv + 1] +
				leTargetLumaOffset;

			targetMaxSeg[AE_LE][0] = AAA_ABS(LvX100) / 100 * 100;
			targetMaxSeg[AE_LE][1] = AAA_ABS(LvX100) / 100 * 100 + 100;
			targetMaxSeg[AE_LE][2] =
				pstAeWDRExposureAttrInfo[sID]->au8LEAdjustTargetMax[targetIndexByLv] +
				leTargetLumaOffset;
			targetMaxSeg[AE_LE][3] =
				pstAeWDRExposureAttrInfo[sID]->au8LEAdjustTargetMax[targetIndexByLv + 1] +
				leTargetLumaOffset;

			if (LvX100 < 0) {
				ptargetMin[AE_LE] = AE_Interpolation(targetMinSeg[AE_LE],
					targetMinSeg[AE_LE][1] - AAA_ABS(LvX100) + targetMinSeg[AE_LE][0]);
				ptargetMax[AE_LE] = AE_Interpolation(targetMaxSeg[AE_LE],
					targetMaxSeg[AE_LE][1] - AAA_ABS(LvX100) + targetMaxSeg[AE_LE][0]);
			} else {
				ptargetMin[AE_LE] = AE_Interpolation(targetMinSeg[AE_LE], LvX100);
				ptargetMax[AE_LE] = AE_Interpolation(targetMaxSeg[AE_LE], LvX100);
			}

			targetMinSeg[AE_SE][0] = AAA_ABS(LvX100) / 100 * 100;
			targetMinSeg[AE_SE][1] = AAA_ABS(LvX100) / 100 * 100 + 100;
			targetMinSeg[AE_SE][2] =
				pstAeWDRExposureAttrInfo[sID]->au8SEAdjustTargetMin[targetIndexByLv] +
				seTargetLumaOffset;
			targetMinSeg[AE_SE][3] =
				pstAeWDRExposureAttrInfo[sID]->au8SEAdjustTargetMin[targetIndexByLv + 1] +
				seTargetLumaOffset;

			targetMaxSeg[AE_SE][0] = AAA_ABS(LvX100) / 100 * 100;
			targetMaxSeg[AE_SE][1] = AAA_ABS(LvX100) / 100 * 100 + 100;
			targetMaxSeg[AE_SE][2] =
				pstAeWDRExposureAttrInfo[sID]->au8SEAdjustTargetMax[targetIndexByLv] +
				seTargetLumaOffset;
			targetMaxSeg[AE_SE][3] =
				pstAeWDRExposureAttrInfo[sID]->au8SEAdjustTargetMax[targetIndexByLv + 1] +
				seTargetLumaOffset;

			if (LvX100 < 0) {
				ptargetMin[AE_SE] = AE_Interpolation(targetMinSeg[AE_SE],
					targetMinSeg[AE_SE][1] - AAA_ABS(LvX100) + targetMinSeg[AE_SE][0]);
				ptargetMax[AE_SE] = AE_Interpolation(targetMaxSeg[AE_SE],
					targetMaxSeg[AE_SE][1] - AAA_ABS(LvX100) + targetMaxSeg[AE_SE][0]);
			} else {
				ptargetMin[AE_SE] = AE_Interpolation(targetMinSeg[AE_SE], LvX100);
				ptargetMax[AE_SE] = AE_Interpolation(targetMaxSeg[AE_SE], LvX100);
			}
		}
		AAA_LIMIT(ptargetMin[AE_LE], 5, 255);
		AAA_LIMIT(ptargetMax[AE_LE], 5, 255);
		AAA_LIMIT(ptargetMin[AE_SE], 1, 255);
		AAA_LIMIT(ptargetMax[AE_SE], 1, 255);
	} else {
		targetLumaOffset = pstAeExposureAttrInfo[sID]->stAuto.u8Compensation - AE_DEFAULT_TARGET_LUMA;
		if (LvX100 <= minLVx100 || LvX100 >= maxLVx100) {
			targetIndexByLv = (LvX100 <= minLVx100) ? 0 : LV_TOTAL_NUM-1;
			ptargetMin[AE_LE] =
				pstAeExposureAttrInfo[sID]->stAuto.au8AdjustTargetMin[targetIndexByLv] +
				targetLumaOffset;
			ptargetMax[AE_LE] =
				pstAeExposureAttrInfo[sID]->stAuto.au8AdjustTargetMax[targetIndexByLv] +
				targetLumaOffset;
		} else {
			targetIndexByLv = (LvX100 - minLVx100) / 100;
			targetMinSeg[AE_LE][0] = AAA_ABS(LvX100) / 100 * 100;
			targetMinSeg[AE_LE][1] = AAA_ABS(LvX100) / 100 * 100 + 100;
			targetMinSeg[AE_LE][2] =
				pstAeExposureAttrInfo[sID]->stAuto.au8AdjustTargetMin[targetIndexByLv] +
				targetLumaOffset;
			targetMinSeg[AE_LE][3] =
				pstAeExposureAttrInfo[sID]->stAuto.au8AdjustTargetMin[targetIndexByLv + 1] +
				targetLumaOffset;

			targetMaxSeg[AE_LE][0] = AAA_ABS(LvX100) / 100 * 100;
			targetMaxSeg[AE_LE][1] = AAA_ABS(LvX100) / 100 * 100 + 100;
			targetMaxSeg[AE_LE][2] =
				pstAeExposureAttrInfo[sID]->stAuto.au8AdjustTargetMax[targetIndexByLv] +
				targetLumaOffset;
			targetMaxSeg[AE_LE][3] =
				pstAeExposureAttrInfo[sID]->stAuto.au8AdjustTargetMax[targetIndexByLv + 1] +
				targetLumaOffset;

			if (LvX100 < 0) {
				ptargetMin[AE_LE] = AE_Interpolation(targetMinSeg[AE_LE],
					targetMinSeg[AE_LE][1] - AAA_ABS(LvX100) + targetMinSeg[AE_LE][0]);
				ptargetMax[AE_LE] = AE_Interpolation(targetMaxSeg[AE_LE],
					targetMaxSeg[AE_LE][1] - AAA_ABS(LvX100) + targetMaxSeg[AE_LE][0]);
			} else {
				ptargetMin[AE_LE] = AE_Interpolation(targetMinSeg[AE_LE], LvX100);
				ptargetMax[AE_LE] = AE_Interpolation(targetMaxSeg[AE_LE], LvX100);
			}
		}
		AAA_LIMIT(ptargetMin[AE_LE], 5, 255);
		AAA_LIMIT(ptargetMax[AE_LE], 5, 255);
	}
}

void AE_AdjustTargetY(CVI_U8 sID)
{
#define MAX_SLOPE	512
#define MIN_STEP	2

	CVI_U8 i;
	CVI_U16 newAeTargetY[AE_MAX_WDR_FRAME_NUM];
	CVI_U16 evBias;
	CVI_U8 curAeStrategyMode;
	CVI_S16 upLimit[AE_MAX_WDR_FRAME_NUM], downLimit[AE_MAX_WDR_FRAME_NUM];
	CVI_S32 targetMin[AE_MAX_WDR_FRAME_NUM], targetMax[AE_MAX_WDR_FRAME_NUM];
	const CVI_U16 MIN_AE_TARGET = 5;
	CVI_S16	targetStep[AE_MAX_WDR_FRAME_NUM] = {0};
	CVI_U8 paramUpdate = 0;
	CVI_S16	slope = 0, maxOffset = 0;
	CVI_U16 lowRatio[AE_MAX_WDR_FRAME_NUM], lowBufRatio[AE_MAX_WDR_FRAME_NUM],
			highRatio[AE_MAX_WDR_FRAME_NUM], highBufRatio[AE_MAX_WDR_FRAME_NUM];
	CVI_U16 tmpLVFrameLuma;
	CVI_S32 bvStep[AE_MAX_WDR_FRAME_NUM];

	static CVI_S16 lumaOffset[AE_SENSOR_NUM][AE_MAX_WDR_FRAME_NUM];
	static CVI_U8 preAeStrategyMode[AE_SENSOR_NUM] = {0xff, 0xff};
	static CVI_U16 curAeTargetY[AE_SENSOR_NUM][AE_MAX_WDR_FRAME_NUM];
	static CVI_S16 LvX100[AE_SENSOR_NUM];

	if (AeInfo[sID].u8ConvergeMode[AE_LE] == CONVERGE_FAST) {
		tmpLVFrameLuma = AAA_MAX(AE_METER_MIN_LUMA, AeInfo[sID].u16LvFrameLuma[AE_LE]);
		bvStep[AE_LE] = (Log2x100Tbl[tmpLVFrameLuma] - Log2x100Tbl[224]) * ENTRY_PER_BV;
		bvStep[AE_LE] = (bvStep[AE_LE] > 0) ? ((bvStep[AE_LE] + 50) / 100) : ((bvStep[AE_LE] - 50) / 100);
		AeInfo[sID].s16LvBvStep[AE_LE] = bvStep[AE_LE];
		AE_CalCurrentLvX100(sID);
		LvX100[sID] = AeInfo[sID].s16LvX100[AE_LE];
	} else {
		LvX100[sID] = AeInfo[sID].s16LvX100[AE_LE];
	}

	curAeStrategyMode = pstAeExposureAttrInfo[sID]->stAuto.enAEStrategyMode;
	if (curAeStrategyMode != preAeStrategyMode[sID])
		memset(lumaOffset[sID], 0, AE_MAX_WDR_FRAME_NUM * sizeof(CVI_S16));

	preAeStrategyMode[sID] = curAeStrategyMode;

	newAeTargetY[AE_LE] = pstAeExposureAttrInfo[sID]->stAuto.u8Compensation << 2; //8 bits shift to 10 bits
	newAeTargetY[AE_SE] = pstAeWDRExposureAttrInfo[sID]->u8SECompensation << 2;

	evBias = pstAeExposureAttrInfo[sID]->stAuto.u16EVBias;

	slope = pstAeExposureAttrInfo[sID]->stAuto.u16HistRatioSlope;
	slope = AAA_MIN(slope, MAX_SLOPE);

	maxOffset = (pstAeExposureAttrInfo[sID]->stAuto.u8MaxHistOffset << 2);

	for (i = 0; i < AeInfo[sID].u8AeMaxFrameNum; ++i) {

		AeInfo[sID].u16TargetLuma[i] = newAeTargetY[i];

		AE_GetTargetRange(sID, LvX100[sID], targetMin, targetMax);

		if (targetMax[i] < targetMin[i])
			targetMax[i] = targetMin[i];

		lowRatio[i] = AeInfo[sID].u16LowRatio[i][GRID_BASE];
		lowBufRatio[i] = AeInfo[sID].u16LowBufRatio[i][GRID_BASE];
		highRatio[i] = AeInfo[sID].u16HighRatio[i][GRID_BASE];
		highBufRatio[i] = AeInfo[sID].u16HighBufRatio[i][GRID_BASE];

		if (pstAeExposureAttrInfo[sID]->stAuto.bHistogramAssist) {
			lowRatio[i] = AeInfo[sID].u16LowRatio[i][HISTOGRAM_BASE];
			lowBufRatio[i] = AeInfo[sID].u16LowBufRatio[i][HISTOGRAM_BASE];
			highRatio[i] = AeInfo[sID].u16HighRatio[i][HISTOGRAM_BASE];
			highBufRatio[i] = AeInfo[sID].u16HighBufRatio[i][HISTOGRAM_BASE];
		}

		upLimit[i] = (targetMax[i] -
			pstAeExposureAttrInfo[sID]->stAuto.u8Compensation) << 2;
		downLimit[i] = (targetMin[i] -
			pstAeExposureAttrInfo[sID]->stAuto.u8Compensation) << 2;

		if (upLimit[i] < downLimit[i])
			upLimit[i] = downLimit[i];

		if (AeInfo[sID].bWDRMode) {
			if (i == AE_LE) {
				// low light prior
				upLimit[i] += maxOffset;
				if (AeInfo[sID].u32frmCnt % AeInfo[sID].u8MeterFramePeriod == 0) {
					if (lowRatio[i] >= pstAeExposureAttrInfo[sID]->stAuto.u16AEStrategyThr) {
						targetStep[i] = upLimit[i] - lumaOffset[sID][i];
					} else if (lowBufRatio[i] < pstAeExposureAttrInfo[sID]->stAuto.u16AEStrategyThr) {
						targetStep[i] = downLimit[i] - lumaOffset[sID][i];
					}
				}
			} else if (i == AE_SE) {
				// high light prior
				downLimit[i] -= maxOffset;
				if (AeInfo[sID].u32frmCnt % AeInfo[sID].u8MeterFramePeriod == 0) {
					if (highRatio[i] >= pstAeExposureAttrInfo[sID]->stAuto.u16AEStrategyThr) {
						targetStep[i] = downLimit[i] - lumaOffset[sID][i];
					} else if (highBufRatio[i] < pstAeExposureAttrInfo[sID]->stAuto.u16AEStrategyThr) {
						targetStep[i] = upLimit[i] - lumaOffset[sID][i];
					}
				}
			}
		} else {
			if (curAeStrategyMode == AE_EXP_LOWLIGHT_PRIOR) {
				upLimit[i] += maxOffset;
				if (AeInfo[sID].u32frmCnt % AeInfo[sID].u8MeterFramePeriod == 0) {
					if (lowRatio[i] >= pstAeExposureAttrInfo[sID]->stAuto.u16AEStrategyThr) {
						targetStep[i] = upLimit[i] - lumaOffset[sID][i];
					} else if (lowBufRatio[i] < pstAeExposureAttrInfo[sID]->stAuto.u16AEStrategyThr) {
						targetStep[i] = downLimit[i] - lumaOffset[sID][i];
					}
				}
			} else {
				downLimit[i] -= maxOffset;
				if (AeInfo[sID].u32frmCnt % AeInfo[sID].u8MeterFramePeriod == 0) {
					if (highRatio[i] >= pstAeExposureAttrInfo[sID]->stAuto.u16AEStrategyThr) {
						targetStep[i] = downLimit[i] - lumaOffset[sID][i];
					} else if (highBufRatio[i] < pstAeExposureAttrInfo[sID]->stAuto.u16AEStrategyThr) {
						targetStep[i] = upLimit[i] - lumaOffset[sID][i];
					}
				}
			}
		}

		targetStep[i] = (targetStep[i] * slope + 128) / (2 * 128);
		lumaOffset[sID][i] += targetStep[i];

		AAA_LIMIT(lumaOffset[sID][i], downLimit[i], upLimit[i]);

		AeInfo[sID].s16TargetlumaOffset[i] = lumaOffset[sID][i];
		AeInfo[sID].s16TargetLumaLowBound[i] = downLimit[i];
		AeInfo[sID].s16TargetLumaHighBound[i] = upLimit[i];

		newAeTargetY[i] = (newAeTargetY[i] + lumaOffset[sID][i] < MIN_AE_TARGET) ?
							MIN_AE_TARGET : newAeTargetY[i] + lumaOffset[sID][i];

		paramUpdate = AE_TargetParameterUpdate(sID);

		if (curAeTargetY[sID][i] && !paramUpdate) {
			if (newAeTargetY[i] > curAeTargetY[sID][i])
				newAeTargetY[i] = curAeTargetY[sID][i] +
					(newAeTargetY[i] - curAeTargetY[sID][i]) / 2;
			else if (newAeTargetY[i] < curAeTargetY[sID][i])
				newAeTargetY[i] = curAeTargetY[sID][i] -
					(curAeTargetY[sID][i] - newAeTargetY[i]) / 2;
		}

		curAeTargetY[sID][i] = newAeTargetY[i];
		AeInfo[sID].u16AdjustTargetLuma[i] = newAeTargetY[i];
	}

	AeInfo[sID].u16AdjustTargetLuma[AE_LE] = AeInfo[sID].u16AdjustTargetLuma[AE_LE] * evBias / AE_GAIN_BASE;

	if (AeInfo[sID].u16AdjustTargetLuma[AE_LE] > AE_MAX_LUMA)
		AeInfo[sID].u16AdjustTargetLuma[AE_LE] = AE_MAX_LUMA;

	if (AeInfo[sID].bWDRMode) {
		AeInfo[sID].u16AdjustTargetLuma[AE_SE] = AeInfo[sID].u16AdjustTargetLuma[AE_SE] * AE_GAIN_BASE /
					AAA_DIV_0_TO_1(pstAeWDRExposureAttrInfo[sID]->u16RatioBias);
		if (AeInfo[sID].u16AdjustTargetLuma[AE_SE] > AE_MAX_LUMA)
			AeInfo[sID].u16AdjustTargetLuma[AE_SE] = AE_MAX_LUMA;
	}

	if (AE_GetDebugMode(sID) == 25) {
		for (i = 0; i < AeInfo[sID].u8AeMaxFrameNum; ++i) {
			printf("(%d)TL:%u cTL:%u EvBias:%d\n", i, newAeTargetY[i],
				curAeTargetY[sID][i], pstAeExposureAttrInfo[sID]->stAuto.u16EVBias);
			printf("ATL:%u LO:%d LU:%d LD:%d LV:%d TS:%d\n", AeInfo[sID].u16AdjustTargetLuma[i],
				lumaOffset[sID][i], upLimit[i], downLimit[i], LvX100[sID], targetStep[i]);
			printf("L(%u):%u bv:%d lvbv:%d WDR_R:%u\n", sID, AeInfo[sID].u16FrameLuma[i],
				AeInfo[sID].stApex[i].s16BVEntry, AeInfo[sID].s16LvBvStep[AE_LE],
				AeInfo[sID].u32WDRExpRatio);
			printf("Ratio:%d %d %d %d %d %d %d %d\n",
				AeInfo[sID].u16HighRatio[i][GRID_BASE],
				AeInfo[sID].u16HighRatio[i][HISTOGRAM_BASE],
				AeInfo[sID].u16HighBufRatio[i][GRID_BASE],
				AeInfo[sID].u16HighBufRatio[i][HISTOGRAM_BASE],
				AeInfo[sID].u16LowRatio[i][GRID_BASE],
				AeInfo[sID].u16LowRatio[i][HISTOGRAM_BASE],
				AeInfo[sID].u16LowBufRatio[i][GRID_BASE],
				AeInfo[sID].u16LowBufRatio[i][HISTOGRAM_BASE]);
		}
	}
}

void AE_GetBvStepRangeByTolerance(CVI_U8 target, CVI_U16 tolerance, CVI_S16 *minBvStep, CVI_S16 *maxBvStep)
{
	CVI_U16 targetRangeMin, targetRangeMax;
	CVI_U16 highTolerance, lowTolerance;
	CVI_S32 bvStepLow, bvStepHigh;

	lowTolerance = AAA_MIN(tolerance, target);
	highTolerance = (target + tolerance >= 255) ? 255 - target : tolerance;

	targetRangeMin = (target > lowTolerance) ? (target - lowTolerance) * 4 - 1 : 0;
	targetRangeMax = (target + highTolerance < 256) ?  (target + highTolerance) * 4 + 1 : 256;

	bvStepLow = (Log2x100Tbl[targetRangeMin] - Log2x100Tbl[target * 4]) * ENTRY_PER_BV;
	bvStepLow = (bvStepLow > 0) ? ((bvStepLow + 50) / 100) : ((bvStepLow - 50) / 100);
	if (bvStepLow > -1)
		bvStepLow = -1;

	bvStepHigh = (Log2x100Tbl[targetRangeMax] - Log2x100Tbl[target * 4]) * ENTRY_PER_BV;
	bvStepHigh = (bvStepHigh > 0) ? ((bvStepHigh + 50) / 100) : ((bvStepHigh - 50) / 100);
	if (bvStepHigh < 1)
		bvStepHigh = 1;

	*minBvStep = bvStepLow;
	*maxBvStep = bvStepHigh;
}

void AE_CalculateStableConverge(CVI_U8 sID, CVI_BOOL mode, CVI_S16 *tmpConvBvEntry,
				CVI_S16 tmpBvEntry, CVI_S16 tmpBvStep)
{
	CVI_U16 tmpAbsBvStep = AAA_ABS(tmpBvStep);
	CVI_U16 step;
	CVI_S16 conBvEntry = *tmpConvBvEntry, diffEntry;
	CVI_S32 data[4];

	step = AAA_MAX(AeInfo[sID].u8MeterFramePeriod, AeInfo[sID].u8SensorPeriod);
	step = (AeInfo[sID].bMeterEveryFrame) ? 1 : step;

	if (mode == 0) {
		if (tmpAbsBvStep >= ENTRY_PER_EV / 20)
			*tmpConvBvEntry = (tmpBvStep > 0) ? tmpBvEntry + 1 : tmpBvEntry - 1;
		if (conBvEntry == *tmpConvBvEntry && tmpAbsBvStep > 1)
			*tmpConvBvEntry = (tmpBvStep > 0) ? tmpBvEntry + 1 : tmpBvEntry - 1;
	} else {
		diffEntry = *tmpConvBvEntry - tmpBvEntry;
		if (AAA_ABS(diffEntry) <= ENTRY_PER_EV / 10) {
			step = AAA_MIN(step, AAA_ABS(diffEntry) / 2);
			*tmpConvBvEntry = (tmpBvStep > 0) ? tmpBvEntry + step * 2 :
				tmpBvEntry - step * 2;
		} else if (AAA_ABS(diffEntry) <= ENTRY_PER_EV / 20) {
			data[0] = ENTRY_PER_EV / 10;
			data[1] = ENTRY_PER_EV / 20;
			data[2] = (tmpBvStep > 0) ? tmpBvEntry + step : tmpBvEntry - step;
			data[3] = *tmpConvBvEntry;
			*tmpConvBvEntry = AE_Interpolation(data, AAA_ABS(diffEntry));
		}
	}
}

void AE_CalculateConverge(CVI_U8 sID)
{
	CVI_U8	i;
	CVI_S16 bvEntry[AE_MAX_WDR_FRAME_NUM], convergeBvEntry[AE_MAX_WDR_FRAME_NUM];
	CVI_S16 minBvEntry, maxBvEntry;
	CVI_U16 absBvStep[AE_MAX_WDR_FRAME_NUM], absCurBvStep[AE_MAX_WDR_FRAME_NUM];
	CVI_S16 convergeSpeed[AE_MAX_WDR_FRAME_NUM] = {0, 0};
	CVI_FLOAT convergeBias = 0;
	CVI_BOOL isOverRange[AE_MAX_WDR_FRAME_NUM];
	CVI_S16 toleranceMinBvStep[AE_MAX_WDR_FRAME_NUM], toleranceMaxBvStep[AE_MAX_WDR_FRAME_NUM];
	CVI_S16 bvStep[AE_MAX_WDR_FRAME_NUM];
	CVI_BOOL	isAeStable[AE_MAX_WDR_FRAME_NUM] = {0, 0};
	CVI_S32 convergeData[4];
	CVI_BOOL highISO = 0;
	CVI_U16 blackDelay, whiteDelay, stableBvStep;
	CVI_U8	frameIndex, meterPeriod = 1;
	CVI_S32 tmpHistError = 0;

	static CVI_U8 stableFrmCnt[AE_SENSOR_NUM][AE_MAX_WDR_FRAME_NUM];
	static CVI_U16 preAbsBvStep[AE_SENSOR_NUM][AE_MAX_WDR_FRAME_NUM] = {{255, 255}, {255, 255} };
	static CVI_U8 delayFrmCnt[AE_SENSOR_NUM][2];
	static CVI_U8 finetuneFrmCnt[AE_SENSOR_NUM][AE_MAX_WDR_FRAME_NUM];
	static CVI_U8 normalConvergeFrmCnt[AE_SENSOR_NUM];

	sID = AE_CheckSensorID(sID);


	if (AeInfo[sID].bWDRMode) {
		AE_GetBvStepRangeByTolerance(AeInfo[sID].u16AdjustTargetLuma[AE_LE] >> 2,
			pstAeExposureAttrInfo[sID]->stAuto.u8Tolerance, &toleranceMinBvStep[AE_LE],
			&toleranceMaxBvStep[AE_LE]);
		AE_GetBvStepRangeByTolerance(AeInfo[sID].u16AdjustTargetLuma[AE_SE] >> 2,
			pstAeWDRExposureAttrInfo[sID]->u16Tolerance, &toleranceMinBvStep[AE_SE],
			&toleranceMaxBvStep[AE_SE]);
	} else {
		AE_GetBvStepRangeByTolerance(AeInfo[sID].u16AdjustTargetLuma[AE_LE] >> 2,
			pstAeExposureAttrInfo[sID]->stAuto.u8Tolerance, &toleranceMinBvStep[AE_LE],
			&toleranceMaxBvStep[AE_LE]);
	}

	blackDelay = pstAeExposureAttrInfo[sID]->stAuto.stAEDelayAttr.u16BlackDelayFrame;
	whiteDelay = pstAeExposureAttrInfo[sID]->stAuto.stAEDelayAttr.u16WhiteDelayFrame;

	AE_GetBVEntryLimit(sID, &minBvEntry, &maxBvEntry);

	frameIndex = AeInfo[sID].u32MeterFrameCnt % AeInfo[sID].u8MeterFramePeriod;
	highISO = (AeInfo[sID].u32ISONum[AE_LE] >= 3200) ? 1 : 0;

	meterPeriod  = (AeInfo[sID].bMeterEveryFrame) ? AeInfo[sID].u8MeterFramePeriod : 1;

	if (AeInfo[sID].u8ConvergeMode[AE_LE] == CONVERGE_FAST) {
		bvStep[AE_LE] = AeInfo[sID].s16BvStepEntry[AE_LE];
		absBvStep[AE_LE] = AAA_ABS(bvStep[AE_LE]);
		if (AeInfo[sID].u8FastConvergeFrmCnt % meterPeriod == 0) {
			if ((bvStep[AE_LE] >= toleranceMinBvStep[AE_LE] &&
				bvStep[AE_LE] <= toleranceMaxBvStep[AE_LE]) ||
				absBvStep[AE_LE] <= ENTRY_PER_EV / 3 ||
				AeInfo[sID].u8FastConvergeFrmCnt >= 5 * meterPeriod) {
				AeInfo[sID].u8ConvergeMode[AE_LE] = CONVERGE_NORMAL;
				if (((bvStep[AE_LE] >= toleranceMinBvStep[AE_LE] &&
					bvStep[AE_LE] <= toleranceMaxBvStep[AE_LE]) ||
					absBvStep[AE_LE] <= ENTRY_PER_EV / 3) &&
					AeInfo[sID].u8FastConvergeFrmCnt != 0)
					AeInfo[sID].bIsStable[AE_LE] = 1;
			}

			for (i = 0; i < AeInfo[sID].u8AeMaxFrameNum; ++i) {
				bvEntry[i] = AE_LimitBVEntry(sID, AeInfo[sID].stApex[i].s16BVEntry);
				bvEntry[i] = bvEntry[i] + bvStep[i];
				AeInfo[sID].stApex[i].s16BVEntry = AE_LimitBVEntry(sID, bvEntry[i]);
			}
		}
		AeInfo[sID].u8FastConvergeFrmCnt++;
		normalConvergeFrmCnt[sID] = 0;
		AeInfo[sID].stSmoothApex[AE_LE][frameIndex].s16BVEntry =
			AeInfo[sID].stApex[AE_LE].s16BVEntry;
	} else {
		normalConvergeFrmCnt[sID]++;
		AeInfo[sID].u8FastConvergeFrmCnt = 0;
		for (i = 0; i < AeInfo[sID].u8AeMaxFrameNum; ++i) {
			AeInfo[sID].s16PreBvEntry[i] = AeInfo[sID].stApex[i].s16BVEntry;
			bvEntry[i] = convergeBvEntry[i] = AE_LimitBVEntry(sID, AeInfo[sID].stApex[i].s16BVEntry);
			bvStep[i] = AeInfo[sID].s16SmoothBvStepEntry[i];
			absBvStep[i] = AAA_ABS(bvStep[i]);
			absCurBvStep[i] = AAA_ABS(AeInfo[sID].s16BvStepEntry[i]);

			if (AeFaceDetect[sID].bMode) {
				AeInfo[sID].bConvergeStable[i] = 1;
			}

			if (absBvStep[AE_LE] > ENTRY_PER_EV) {
				AeInfo[sID].bConvergeStable[i] = 0;
			}

			if (AeInfo[sID].bConvergeStable[i]) {
				if (absBvStep[i] > ENTRY_PER_EV / 2) {
					convergeData[0] = ENTRY_PER_EV / 2;
					convergeData[1] = ENTRY_PER_EV;
					convergeData[2] = (highISO) ? 9 : 12;
					convergeData[3] = (highISO) ? 12 : 16;
				} else if (absBvStep[i] > ENTRY_PER_EV / 4) {
					convergeData[0] = ENTRY_PER_EV / 4;
					convergeData[1] = ENTRY_PER_EV / 2;
					convergeData[2] = (highISO) ? 6 : 8;
					convergeData[3] = (highISO) ? 9 : 12;
				} else {
					convergeData[0] = ENTRY_PER_EV / 8;
					convergeData[1] = ENTRY_PER_EV / 4;
					convergeData[2] = 4;
					convergeData[3] = (highISO) ? 6 : 8;
				}
				convergeSpeed[i] = AE_Interpolation(convergeData, absBvStep[i]);
			} else {

				if (absBvStep[i] >= ENTRY_PER_EV * 20/10) {
					convergeData[0] = ENTRY_PER_EV * 2;
					convergeData[1] = ENTRY_PER_EV * 3;
					if (bvStep[i] < 0) {
						convergeData[2] = 44;
						convergeData[3] = 48;
					} else {
						convergeData[2] = 44;
						convergeData[3] = 48;
					}
				} else if (absBvStep[i] > ENTRY_PER_EV * 15 / 10) {
					convergeData[0] = ENTRY_PER_EV * 15 / 10;
					convergeData[1] = ENTRY_PER_EV * 2;
					if (bvStep[i] < 0) {
						convergeData[2] = 40;
						convergeData[3] = 44;
					} else {
						convergeData[2] = 40;
						convergeData[3] = 44;
					}
				} else if (absBvStep[i] > ENTRY_PER_EV) {
					convergeData[0] = ENTRY_PER_EV;
					convergeData[1] = ENTRY_PER_EV * 15 / 10;
					if (bvStep[i] < 0) {
						convergeData[2] = 32;
						convergeData[3] = 40;
					} else {
						convergeData[2] = 32;
						convergeData[3] = 40;
					}
				} else if (absBvStep[i] > ENTRY_PER_EV / 2) {
					convergeData[0] = ENTRY_PER_EV / 2;
					convergeData[1] = ENTRY_PER_EV;
					convergeData[2] = 24;
					convergeData[3] = 32;
				} else if (absBvStep[i] > ENTRY_PER_EV / 4) {
					convergeData[0] = ENTRY_PER_EV / 4;
					convergeData[1] = ENTRY_PER_EV / 2;
					convergeData[2] = 16;
					convergeData[3] = 24;
				} else {
					convergeData[0] = ENTRY_PER_EV / 8;
					convergeData[1] = ENTRY_PER_EV / 4;
					convergeData[2] = 8;
					convergeData[3] = 16;
				}
				convergeSpeed[i] = AE_Interpolation(convergeData, absBvStep[i]);
			}

			if (i == AE_LE) {
				if (absBvStep[i] > ENTRY_PER_EV / 10) {
					if (bvStep[i] > 0) {
						delayFrmCnt[sID][WHITE_DELAY]++;
						delayFrmCnt[sID][BLACK_DELAY] = 0;
						convergeSpeed[i] = (delayFrmCnt[sID][WHITE_DELAY] <= whiteDelay)
							? 0 : convergeSpeed[i];
					} else {
						delayFrmCnt[sID][BLACK_DELAY]++;
						delayFrmCnt[sID][WHITE_DELAY] = 0;
						convergeSpeed[i] = (delayFrmCnt[sID][BLACK_DELAY] <= blackDelay)
							? 0 : convergeSpeed[i];
					}
				} else {

					AeInfo[sID].u8ConvergeMode[i] = CONVERGE_NORMAL;

					tmpHistError = (AeInfo[sID].u16AdjustTargetLuma[i] *
						AE_CalculatePow2(absBvStep[i])) - AeInfo[sID].u16AdjustTargetLuma[i];
					tmpHistError = tmpHistError >> 2;

					if (tmpHistError <= (pstAeExposureAttrInfo[sID]->stAuto.u8Tolerance + 1)) {
						delayFrmCnt[sID][WHITE_DELAY] = delayFrmCnt[sID][BLACK_DELAY] = 0;
					}
				}
			}

			if (AeInfo[sID].bWDRMode && pstAeWDRExposureAttrInfo[sID]->enExpMainChn == ISP_CHANNEL_LE) {
				if (i == AE_SE && convergeSpeed[AE_LE] < 24 &&
					convergeSpeed[AE_SE] > convergeSpeed[AE_LE])
					convergeSpeed[AE_SE] = convergeSpeed[AE_LE];
			}
			AeInfo[sID].u8ConvergeSpeed[i] = convergeSpeed[i];

			if (pstAeSmartExposureAttrInfo[sID]->bEnable) {
				convergeBias = (CVI_FLOAT)pstAeSmartExposureAttrInfo[sID]->u8SmartSpeed / 32;
			} else if (AeInfo[sID].bWDRMode) {
				convergeBias = (CVI_FLOAT)pstAeWDRExposureAttrInfo[sID]->u16Speed / 32;
			} else {
				if (bvStep[i] > 0) {
					convergeBias = (CVI_FLOAT)pstAeExposureAttrInfo[sID]->stAuto.u8Speed / 64;
				} else {
					convergeBias =
						(CVI_FLOAT)pstAeExposureAttrInfo[sID]->stAuto.u16BlackSpeedBias / 144;
				}
			}

			convergeSpeed[i] = convergeSpeed[i] * convergeBias;
			AAA_LIMIT(convergeSpeed[i], 1, 64);

			isOverRange[i] =
				((bvEntry[i] >= maxBvEntry && bvStep[i] >= 0) ||
				(bvEntry[i] <= minBvEntry && bvStep[i] <= 0)) ?
					1 : 0;


			if (isOverRange[i]) {
				convergeBvEntry[i] = AAA_MIN(bvEntry[i], maxBvEntry);
			} else {
				if (bvStep[i] < toleranceMinBvStep[i] || bvStep[i] > toleranceMaxBvStep[i]) {
					finetuneFrmCnt[sID][i] = 0;
					AeInfo[sID].u16ConvergeFrameCnt[i]++;

					if (absBvStep[i] <= ENTRY_PER_EV / 5) {
						AE_CalculateStableConverge(sID, 0, &convergeBvEntry[i], bvEntry[i],
							bvStep[i]);
					} else {
						convergeBvEntry[i] =
							(bvStep[i] > 0) ? ((bvEntry[i] * (64 - convergeSpeed[i]) +
						    (bvEntry[i] + bvStep[i]) * convergeSpeed[i]) + 32) >> 6 :
							((bvEntry[i] * (64 - convergeSpeed[i]) +
						    (bvEntry[i] + bvStep[i]) * convergeSpeed[i]) - 32) >> 6;

						AE_CalculateStableConverge(sID, 1, &convergeBvEntry[i], bvEntry[i],
							bvStep[i]);
					}

				} else {
					finetuneFrmCnt[sID][i] = (finetuneFrmCnt[sID][i] > 10 * meterPeriod) ?
						10 * meterPeriod : finetuneFrmCnt[sID][i] + 1;
					if (finetuneFrmCnt[sID][i] > 5 * meterPeriod && absBvStep[i] > 1) {
						convergeBvEntry[i] = (bvStep[i] > 0) ? convergeBvEntry[i] + 1 :
												convergeBvEntry[i] - 1;
						finetuneFrmCnt[sID][i] = 0;
					}
				}
			}

			isOverRange[i] = ((convergeBvEntry[i] >= maxBvEntry && bvStep[i] >= 0) ||
					  (convergeBvEntry[i] <= minBvEntry && bvStep[i] <= 0)) ? 1 : 0;

			stableBvStep = AAA_MAX(ENTRY_PER_EV / 3, toleranceMaxBvStep[i]);

			if (isOverRange[i] || (absCurBvStep[i] <= stableBvStep &&
				preAbsBvStep[sID][i] <= stableBvStep)) {
				stableFrmCnt[sID][i] = (stableFrmCnt[sID][i] > 10) ? 10 : stableFrmCnt[sID][i] + 1;
				if (stableFrmCnt[sID][i] >= meterPeriod) {
					isAeStable[i] = 1;
					AeInfo[sID].bConvergeStable[i] = 1;
				}
			} else
				stableFrmCnt[sID][i] = 0;

			if (stableFrmCnt[sID][i] > 2 * meterPeriod)
				AeInfo[sID].u16ConvergeFrameCnt[i] = 0;

			bvEntry[i] = AE_LimitBVEntry(sID, convergeBvEntry[i]);
			preAbsBvStep[sID][i] = absCurBvStep[i];

			AeInfo[sID].bIsMaxExposure = (bvEntry[i] <= minBvEntry) ? 1 : 0;
			AeInfo[sID].s16ConvBvStepEntry[i] = bvEntry[i] - AeInfo[sID].stApex[i].s16BVEntry;
			AeInfo[sID].stApex[i].s16BVEntry = bvEntry[i];
			AeInfo[sID].stSmoothApex[i][frameIndex].s16BVEntry = bvEntry[i];
			if (normalConvergeFrmCnt[sID] > meterPeriod)
				AeInfo[sID].bIsStable[i] = (isOverRange[i] || isAeStable[i]) ? 1 : 0;
		}
	}


}

void AE_CheckExposureParameterRange(CVI_U8 sID, ISP_EXPOSURE_ATTR_S *pstAeAttr)
{
	sID = AE_CheckSensorID(sID);

	pstAeAttr->stAuto.stExpTimeRange.u32Min =
		AAA_MAX(AeInfo[sID].u32ExpTimeMin, pstAeAttr->stAuto.stExpTimeRange.u32Min);

	pstAeAttr->stAuto.stExpTimeRange.u32Max =
		AAA_MIN(AeInfo[sID].u32MinFpsExpTimeMax, pstAeAttr->stAuto.stExpTimeRange.u32Max);

	pstAeAttr->stAuto.stISONumRange.u32Min =
		AAA_MAX(AeInfo[sID].u32ISONumMin, pstAeAttr->stAuto.stISONumRange.u32Min);

	pstAeAttr->stAuto.stISONumRange.u32Max =
		AAA_MIN(AeInfo[sID].u32ISONumMax, pstAeAttr->stAuto.stISONumRange.u32Max);

	pstAeAttr->stAuto.stAGainRange.u32Min =
		AAA_MAX(AeInfo[sID].u32AGainMin, pstAeAttr->stAuto.stAGainRange.u32Min);

	pstAeAttr->stAuto.stAGainRange.u32Max =
		AAA_MIN(AeInfo[sID].u32AGainMax, pstAeAttr->stAuto.stAGainRange.u32Max);

	pstAeAttr->stAuto.stDGainRange.u32Min =
		AAA_MAX(AeInfo[sID].u32DGainMin, pstAeAttr->stAuto.stDGainRange.u32Min);

	pstAeAttr->stAuto.stDGainRange.u32Max =
		AAA_MIN(AeInfo[sID].u32DGainMax, pstAeAttr->stAuto.stDGainRange.u32Max);

	pstAeAttr->stAuto.stISPDGainRange.u32Min =
		AAA_MAX(AeInfo[sID].u32ISPDGainMin, pstAeAttr->stAuto.stISPDGainRange.u32Min);

	pstAeAttr->stAuto.stISPDGainRange.u32Max =
		AAA_MIN(AeInfo[sID].u32ISPDGainMax, pstAeAttr->stAuto.stISPDGainRange.u32Max);

	pstAeAttr->stAuto.stSysGainRange.u32Min =
		AAA_MAX(AeInfo[sID].u32TotalGainMin, pstAeAttr->stAuto.stSysGainRange.u32Min);

	pstAeAttr->stAuto.stSysGainRange.u32Max =
		AAA_MIN(AeInfo[sID].u32TotalGainMax, pstAeAttr->stAuto.stSysGainRange.u32Max);
}


void AE_SetManualExposure(CVI_U8 sID)
{
	sID = AE_CheckSensorID(sID);

	pstAeExposureAttrInfo[sID]->bByPass = pstAeMpiExposureAttr[sID]->bByPass;
	pstAeExposureAttrInfo[sID]->enOpType = pstAeMpiExposureAttr[sID]->enOpType;
	pstAeExposureAttrInfo[sID]->stManual = pstAeMpiExposureAttr[sID]->stManual;

	pstAeExposureAttrInfo[sID]->stAuto.stExpTimeRange = pstAeMpiExposureAttr[sID]->stAuto.stExpTimeRange;
	pstAeExposureAttrInfo[sID]->stAuto.stAGainRange = pstAeMpiExposureAttr[sID]->stAuto.stAGainRange;
	pstAeExposureAttrInfo[sID]->stAuto.stDGainRange = pstAeMpiExposureAttr[sID]->stAuto.stDGainRange;
	pstAeExposureAttrInfo[sID]->stAuto.stISPDGainRange = pstAeMpiExposureAttr[sID]->stAuto.stISPDGainRange;
	pstAeExposureAttrInfo[sID]->stAuto.stISONumRange = pstAeMpiExposureAttr[sID]->stAuto.stISONumRange;

	AE_CheckExposureParameterRange(sID, pstAeExposureAttrInfo[sID]);
	AE_ConfigManualExposure(sID);
}

void AE_ConfigDBTypeDgain(CVI_U8 sID, AE_GAIN *pstGain)
{
	CVI_U32 dGain;
	CVI_S16 i;
	AE_GAIN aeGain;

	//convert linear gain to db gain
	if (AeInfo[sID].u8DGainAccuType == AE_ACCURACY_DB) {
		aeGain = *pstGain;
		dGain = aeGain.u32DGain;
		if (AeInfo[sID].bDGainSwitchToAGain) {//sensor dgain -> sensor again
			for (i = 0; i < AeInfo[sID].u8SensorDgainNodeNum; ++i) {
				if (dGain <= AeInfo[sID].u32SensorDgainNode[i]) {
					dGain = AeInfo[sID].u32SensorDgainNode[i];
					break;
				}
			}
			if (aeGain.u32DGain != dGain) {
				aeGain.u32AGain = (CVI_U64)aeGain.u32AGain * aeGain.u32DGain / dGain;
				aeGain.u32AGain = AAA_MAX(aeGain.u32AGain, AE_GAIN_BASE);
				aeGain.u32DGain = dGain;
			}
		} else {//sensor dgain -> isp dgain
			for (i = AeInfo[sID].u8SensorDgainNodeNum-1; i >= 0 ; --i) {
				if (dGain >= AeInfo[sID].u32SensorDgainNode[i]) {
					dGain = AeInfo[sID].u32SensorDgainNode[i];
					break;
				}
			}
			if (aeGain.u32DGain != dGain) {
				aeGain.u32ISPDGain = (CVI_U64)aeGain.u32ISPDGain * aeGain.u32DGain /
					AAA_DIV_0_TO_1(dGain);
				aeGain.u32ISPDGain = AAA_MAX(aeGain.u32ISPDGain, AE_GAIN_BASE);
				aeGain.u32DGain = dGain;
			}
		}
		*pstGain = aeGain;
	}
}

void AE_GetRoutExGainByEntry(CVI_U8 sID, AE_WDR_FRAME wdrFrm, CVI_S16 SvEntry, AE_GAIN *pstGain)
{
	CVI_U8 i, routeNum;
	AE_GAIN aeGain, preNodeGain;
	CVI_S16 gainDiffEntry = 0, aGainEntry = 0, dGainEntry = 0, ispDGainEntry = 0;

	if (SvEntry < 0) {
		pstGain->u32AGain = AE_GAIN_BASE;
		pstGain->u32DGain = AE_GAIN_BASE;
		pstGain->u32ISPDGain = AE_GAIN_BASE;
		ISP_LOG_WARNING("SvEntry:%d (< 0)\n", SvEntry);
		return;
	}

	routeNum = AeInfo[sID].u8RouteNum[wdrFrm];

	for (i = 0; i < routeNum; ++i) {
		if (SvEntry < AeInfo[sID].pstAeRoute[wdrFrm][i].NodeSvEntry) {
			aeGain.u32AGain = AeInfo[sID].pstAeRoute[wdrFrm][i].stNodeGain.u32AGain;
			aeGain.u32DGain = AeInfo[sID].pstAeRoute[wdrFrm][i].stNodeGain.u32DGain;
			aeGain.u32ISPDGain = AeInfo[sID].pstAeRoute[wdrFrm][i].stNodeGain.u32ISPDGain;
			#if 0
			printf("   %d Sv:%d %d A:%d D:%d I:%d\n", i, SvEntry, AeInfo[sID].pstAeRoute[i].NodeSvEntry,
				aeGain.u32AGain, aeGain.u32DGain, aeGain.u32ISPDGain);
			#endif
			break;
		}
	}

	if (i == routeNum) {
		aeGain.u32AGain = AeInfo[sID].pstAeRoute[wdrFrm][routeNum - 1].stNodeGain.u32AGain;
		aeGain.u32DGain = AeInfo[sID].pstAeRoute[wdrFrm][routeNum - 1].stNodeGain.u32DGain;
		aeGain.u32ISPDGain = AeInfo[sID].pstAeRoute[wdrFrm][routeNum - 1].stNodeGain.u32ISPDGain;
	}

	if (i == 0) {
		preNodeGain.u32AGain = AeInfo[sID].pstAeRoute[wdrFrm][0].stNodeGain.u32AGain;
		preNodeGain.u32DGain = AeInfo[sID].pstAeRoute[wdrFrm][0].stNodeGain.u32DGain;
		preNodeGain.u32ISPDGain = AeInfo[sID].pstAeRoute[wdrFrm][0].stNodeGain.u32ISPDGain;
		gainDiffEntry = 0;
	} else if (i == routeNum) {
		preNodeGain.u32AGain = AeInfo[sID].pstAeRoute[wdrFrm][i-1].stNodeGain.u32AGain;
		preNodeGain.u32DGain = AeInfo[sID].pstAeRoute[wdrFrm][i-1].stNodeGain.u32DGain;
		preNodeGain.u32ISPDGain = AeInfo[sID].pstAeRoute[wdrFrm][i-1].stNodeGain.u32ISPDGain;
		gainDiffEntry = 0;
	} else {
		preNodeGain.u32AGain = AeInfo[sID].pstAeRoute[wdrFrm][i-1].stNodeGain.u32AGain;
		preNodeGain.u32DGain = AeInfo[sID].pstAeRoute[wdrFrm][i-1].stNodeGain.u32DGain;
		preNodeGain.u32ISPDGain = AeInfo[sID].pstAeRoute[wdrFrm][i-1].stNodeGain.u32ISPDGain;
		gainDiffEntry = SvEntry - AeInfo[sID].pstAeRoute[wdrFrm][i-1].NodeSvEntry;
	}

	#if 0
	printf("	A C:%d P:%d D C:%d P:%d I C:%d P:%d\n", aeGain.u32AGain, preNodeGain.u32AGain,
		aeGain.u32DGain, preNodeGain.u32DGain, aeGain.u32ISPDGain,
		preNodeGain.u32ISPDGain);
	#endif

	if (aeGain.u32AGain != preNodeGain.u32AGain) {
		aGainEntry = gainDiffEntry;
		aeGain.u32AGain = preNodeGain.u32AGain * AE_CalculatePow2(aGainEntry);
	} else if (aeGain.u32DGain != preNodeGain.u32DGain) {
		dGainEntry = gainDiffEntry;
		aeGain.u32DGain = preNodeGain.u32DGain * AE_CalculatePow2(dGainEntry);
	} else if (aeGain.u32ISPDGain != preNodeGain.u32ISPDGain) {
		ispDGainEntry = gainDiffEntry;
		aeGain.u32ISPDGain = preNodeGain.u32ISPDGain * AE_CalculatePow2(ispDGainEntry);
	}

	AE_ConfigDBTypeDgain(sID, &aeGain);

	AAA_LIMIT(aeGain.u32AGain, AeInfo[sID].u32AGainMin, AeInfo[sID].u32AGainMax);
	AAA_LIMIT(aeGain.u32DGain, AeInfo[sID].u32DGainMin, AeInfo[sID].u32DGainMax);
	AAA_LIMIT(aeGain.u32ISPDGain, AeInfo[sID].u32ISPDGainMin, AeInfo[sID].u32ISPDGainMax);

	pstGain->u32AGain = aeGain.u32AGain;
	pstGain->u32DGain = aeGain.u32DGain;
	pstGain->u32ISPDGain = aeGain.u32ISPDGain;

	#if 0
	printf("   SvDiff:%d A:%d D:%d I:%d\n\n", gainDiffEntry,
				aeGain.u32AGain, aeGain.u32DGain, aeGain.u32ISPDGain);
	#endif
}

void AE_GetLimitExposure(CVI_U8 sID, AE_EXPOSURE *exp, CVI_U8 *path)
{
	CVI_U32 minTime, maxTime;
	CVI_U32 minISO, maxISO, curISO, totalGain;
	CVI_U32 minAGain, maxAGain, minDGain, maxDGain, minISPDGain, maxISPDGain;
	CVI_U32 finalISO;
	AE_EXPOSURE finalExp;
	CVI_S16 ISOEntry;
	CVI_U64 finalTotalGain;
	CVI_U64 curExposure, minLimitExposure, maxLimitExposure;
	CVI_U32 tmpAGain, tmpDGain;
	AE_GAIN	minISOGain, maxISOGain;

	minTime	= pstAeExposureAttrInfo[sID]->stAuto.stExpTimeRange.u32Min;
	maxTime = pstAeExposureAttrInfo[sID]->stAuto.stExpTimeRange.u32Max;

	minISO = pstAeExposureAttrInfo[sID]->stAuto.stISONumRange.u32Min;
	maxISO = pstAeExposureAttrInfo[sID]->stAuto.stISONumRange.u32Max;

	minAGain = pstAeExposureAttrInfo[sID]->stAuto.stAGainRange.u32Min;
	maxAGain = pstAeExposureAttrInfo[sID]->stAuto.stAGainRange.u32Max;

	minDGain = pstAeExposureAttrInfo[sID]->stAuto.stDGainRange.u32Min;
	maxDGain = pstAeExposureAttrInfo[sID]->stAuto.stDGainRange.u32Max;

	minISPDGain = pstAeExposureAttrInfo[sID]->stAuto.stISPDGainRange.u32Min;
	maxISPDGain = pstAeExposureAttrInfo[sID]->stAuto.stISPDGainRange.u32Max;

	*path = 0;
	finalExp = *exp;
	totalGain = AE_CalTotalGain(exp->stExpGain.u32AGain, exp->stExpGain.u32DGain, exp->stExpGain.u32ISPDGain);
	AE_GetISONumByGain(totalGain, &curISO);
	finalISO  = curISO;
	finalTotalGain = totalGain;

	AAA_LIMIT(finalExp.u32ExpTime, minTime, maxTime);

	if (AE_GetDebugMode(sID) == 42) {
		printf("Tv:%d Sv:%d Time:%u AG:%u DG:%u IG:%u\n",
			AeInfo[sID].stApex[AE_LE].s16TVEntry, AeInfo[sID].stApex[AE_LE].s16SVEntry,
			AeInfo[sID].stExp[AE_LE].u32ExpTime, AeInfo[sID].stExp[AE_LE].stExpGain.u32AGain,
			AeInfo[sID].stExp[AE_LE].stExpGain.u32DGain, AeInfo[sID].stExp[AE_LE].stExpGain.u32ISPDGain);
	}

	if (pstAeExposureAttrInfo[sID]->stAuto.enGainType == AE_TYPE_ISO) {
		AAA_LIMIT(finalISO, minISO, maxISO);
		if (finalExp.u32ExpTime == exp->u32ExpTime && finalISO == curISO)
			return;
		AE_GetISOEntryByISONum(minISO, &ISOEntry);
		AE_GetExpGainByEntry(sID, ISOEntry, &minISOGain);
		minAGain = minISOGain.u32AGain;
		minDGain = minISOGain.u32DGain;
		minISPDGain = minISOGain.u32ISPDGain;
		AE_GetISOEntryByISONum(maxISO, &ISOEntry);
		AE_GetExpGainByEntry(sID, ISOEntry, &maxISOGain);
		maxAGain = maxISOGain.u32AGain;
		maxDGain = maxISOGain.u32DGain;
		maxISPDGain = maxISOGain.u32ISPDGain;
	} else {
		AAA_LIMIT(finalExp.stExpGain.u32AGain, minAGain, maxAGain);
		AAA_LIMIT(finalExp.stExpGain.u32DGain, minDGain, maxDGain);
		AAA_LIMIT(finalExp.stExpGain.u32ISPDGain, minISPDGain, maxISPDGain);
		if (finalExp.u32ExpTime == exp->u32ExpTime &&
			finalExp.stExpGain.u32AGain == exp->stExpGain.u32AGain &&
			finalExp.stExpGain.u32DGain == exp->stExpGain.u32DGain &&
			finalExp.stExpGain.u32ISPDGain == exp->stExpGain.u32ISPDGain)
			return;
	}

	curExposure = (CVI_U64)exp->u32ExpTime * exp->stExpGain.u32AGain *
				exp->stExpGain.u32DGain * exp->stExpGain.u32ISPDGain;

	minLimitExposure = (CVI_U64)minTime * minAGain * minDGain * minISPDGain;
	maxLimitExposure = (CVI_U64)maxTime * maxAGain * maxDGain * maxISPDGain;

	if (curExposure >= maxLimitExposure) {
		finalExp.u32ExpTime = maxTime;
		finalExp.stExpGain.u32AGain = maxAGain;
		finalExp.stExpGain.u32DGain = maxDGain;
		finalExp.stExpGain.u32ISPDGain = maxISPDGain;
		*path = 1;
	} else if (curExposure <= minLimitExposure) {
		finalExp.u32ExpTime = minTime;
		finalExp.stExpGain.u32AGain = minAGain;
		finalExp.stExpGain.u32DGain = minDGain;
		finalExp.stExpGain.u32ISPDGain = minISPDGain;
		*path = 2;
	} else {
		if (pstAeExposureAttrInfo[sID]->stAuto.enGainType == AE_TYPE_ISO) {
			if (finalExp.u32ExpTime != exp->u32ExpTime) {
				finalISO = (CVI_U64)curISO * exp->u32ExpTime / finalExp.u32ExpTime;
			}

			AAA_LIMIT(finalISO, minISO, maxISO);

			if (finalISO != curISO) {
				finalExp.u32ExpTime = (CVI_U64)exp->u32ExpTime * curISO / finalISO;
				AAA_LIMIT(finalExp.u32ExpTime, minTime, maxTime);
			}

			AE_GetISOEntryByISONum(finalISO, &ISOEntry);
			AE_GetExpGainByEntry(sID, ISOEntry, &finalExp.stExpGain);
			*path = 3;
		} else {
			tmpAGain = curExposure / ((CVI_U64)finalExp.u32ExpTime * finalExp.stExpGain.u32DGain *
				finalExp.stExpGain.u32ISPDGain);
			finalExp.stExpGain.u32AGain = tmpAGain;

			AAA_LIMIT(finalExp.stExpGain.u32AGain, minAGain, maxAGain);

			if (finalExp.stExpGain.u32AGain != tmpAGain) {
				tmpDGain = curExposure / ((CVI_U64)finalExp.u32ExpTime * finalExp.stExpGain.u32AGain *
					finalExp.stExpGain.u32ISPDGain);
				finalExp.stExpGain.u32DGain = tmpDGain;
				AAA_LIMIT(finalExp.stExpGain.u32DGain, minDGain, maxDGain);
				if (finalExp.stExpGain.u32DGain != tmpDGain) {
					finalExp.stExpGain.u32ISPDGain = curExposure / ((CVI_U64)finalExp.u32ExpTime *
						finalExp.stExpGain.u32AGain * finalExp.stExpGain.u32DGain);
					AAA_LIMIT(finalExp.stExpGain.u32ISPDGain, minISPDGain, maxISPDGain);
				}
			}
			finalExp.u32ExpTime = curExposure / ((CVI_U64)finalExp.stExpGain.u32AGain *
				finalExp.stExpGain.u32DGain * finalExp.stExpGain.u32ISPDGain);
			*path = 4;
		}
	}

	if (AE_GetDebugMode(sID) == 42) {
		finalTotalGain = AE_CalTotalGain(finalExp.stExpGain.u32AGain, finalExp.stExpGain.u32DGain,
			finalExp.stExpGain.u32ISPDGain);
		AE_GetISONumByGain(finalTotalGain, &finalISO);
		printf("T:%u-%u ISO:%u-%u AG:%u-%u DG:%u-%u IG:%u-%u P:%d\n",
			minTime, maxTime, minISO, maxISO, minAGain, maxAGain,
			minDGain, maxDGain, minISPDGain, maxISPDGain, *path);
		printf("ori T:%u ISO:%u AG:%u DG:%u IG:%u\n", exp->u32ExpTime, curISO,
			exp->stExpGain.u32AGain, exp->stExpGain.u32DGain, exp->stExpGain.u32ISPDGain);
		printf("new T:%u ISO:%u AG:%u DG:%u IG:%u\n\n", finalExp.u32ExpTime, finalISO,
			finalExp.stExpGain.u32AGain, finalExp.stExpGain.u32DGain, finalExp.stExpGain.u32ISPDGain);
	}

	exp->u32ExpTime = finalExp.u32ExpTime;
	exp->fIdealExpTime = finalExp.u32ExpTime;
	exp->stExpGain.u32AGain = finalExp.stExpGain.u32AGain;
	exp->stExpGain.u32DGain = finalExp.stExpGain.u32DGain;
	exp->stExpGain.u32ISPDGain = finalExp.stExpGain.u32ISPDGain;

}


void AE_ConfigExposure(CVI_U8 sID)
{
	CVI_U8 i;

	sID = AE_CheckSensorID(sID);

	for (i = 0; i < AeInfo[sID].u8AeMaxFrameNum; ++i) {
		AE_GetIdealExpTimeByEntry(sID, AeInfo[sID].stApex[i].s16TVEntry, &AeInfo[sID].stExp[i].fIdealExpTime);
		AeInfo[sID].stExp[i].u32ExpTime = (CVI_U32)AeInfo[sID].stExp[i].fIdealExpTime;

		if (pstAeExposureAttrInfo[sID]->bAERouteExValid)
			AE_GetRoutExGainByEntry(sID, i, AeInfo[sID].stApex[i].s16SVEntry,
				&AeInfo[sID].stExp[i].stExpGain);
		else
			AE_GetExpGainByEntry(sID, AeInfo[sID].stApex[i].s16SVEntry, &AeInfo[sID].stExp[i].stExpGain);

		AE_GetLimitExposure(sID, &AeInfo[sID].stExp[i], &AeInfo[sID].u8LimitExposurePath[i]);
	}
}

void AE_GetNodeDGain(CVI_U8 sID, CVI_U32 *dGain)
{
	CVI_S16 i;
	CVI_U32 tmpDGain = *dGain;

	for (i = AeInfo[sID].u8SensorDgainNodeNum - 1; i >= 0; i--) {
		if (tmpDGain >= AeInfo[sID].u32SensorDgainNode[i]) {
			tmpDGain = AeInfo[sID].u32SensorDgainNode[i];
			break;
		}
	}
	if (i <= 0) {
		tmpDGain = AeInfo[sID].u32DGainMin;
	}
	*dGain = tmpDGain;
}

void AE_ConfigManualExposure(CVI_U8 sID)
{
	CVI_U32 aGain, dGain, ispDGain;
	CVI_U32 manualAGain, manualDGain, manualIspDGain;
	CVI_U32 autoTime, autoISONum, manualTime, manualISONum, finalTime, finalISONum;
	CVI_U32 tmpGain, maxAGain;
	CVI_BOOL aGainFix = 0, dGainFix = 0, ispDGainFix = 0, fpsChange = 0;
	CVI_U64 autoTotalGain, finalTotalGain;
	AE_GAIN	autoGain, finalGain;
	CVI_S16 ISOEntry;
	CVI_FLOAT fps = AeInfo[sID].fFps;

	sID = AE_CheckSensorID(sID);

	if (pstAeExposureAttrInfo[sID]->bByPass)
		return;

	AE_GetExpTimeByEntry(sID, AeInfo[sID].stApex[AE_LE].s16TVEntry, &autoTime);
	AE_GetExpGainByEntry(sID, AeInfo[sID].stApex[AE_LE].s16SVEntry, &autoGain);

	maxAGain = AAA_MIN(pstAeExposureAttrInfo[sID]->stAuto.stAGainRange.u32Max, AeInfo[sID].u32AGainMax);

	autoTotalGain = (CVI_U64)autoGain.u32AGain * autoGain.u32DGain *
		autoGain.u32ISPDGain;

	tmpGain = AE_CalTotalGain(autoGain.u32AGain, autoGain.u32DGain, autoGain.u32ISPDGain);
	AE_GetISONumByGain(tmpGain, &autoISONum);

	finalTime = autoTime;
	finalISONum = autoISONum;
	finalGain = autoGain;
	finalTotalGain = autoTotalGain;

	manualTime = pstAeExposureAttrInfo[sID]->stManual.u32ExpTime;
	manualTime = AE_LimitManualTime(sID, manualTime);

	manualAGain = pstAeExposureAttrInfo[sID]->stManual.u32AGain;
	manualAGain = AE_LimitManualAGain(sID, manualAGain);

	manualDGain = pstAeExposureAttrInfo[sID]->stManual.u32DGain;
	if (AeInfo[sID].u8DGainAccuType == AE_ACCURACY_DB) {
		AE_GetNodeDGain(sID, &manualDGain);
	}
	manualDGain = AE_LimitManualDGain(sID, manualDGain);

	manualIspDGain = pstAeExposureAttrInfo[sID]->stManual.u32ISPDGain;
	manualIspDGain = AE_LimitManualISPDGain(sID, manualIspDGain);

	manualISONum = pstAeExposureAttrInfo[sID]->stManual.u32ISONum;
	manualISONum = AE_LimitManualISONum(sID, manualISONum);

	if (pstAeExposureAttrInfo[sID]->stManual.enGainType == AE_TYPE_GAIN) {
		if (pstAeExposureAttrInfo[sID]->stManual.enAGainOpType == OP_TYPE_MANUAL)
			aGainFix = 1;
		if (pstAeExposureAttrInfo[sID]->stManual.enDGainOpType == OP_TYPE_MANUAL)
			dGainFix = 1;
		if (pstAeExposureAttrInfo[sID]->stManual.enISPDGainOpType == OP_TYPE_MANUAL)
			ispDGainFix = 1;
	}

	if (pstAeExposureAttrInfo[sID]->stManual.enExpTimeOpType == OP_TYPE_MANUAL) {
		if (pstAeExposureAttrInfo[sID]->stManual.enGainType == AE_TYPE_ISO &&
			pstAeExposureAttrInfo[sID]->stManual.enISONumOpType == OP_TYPE_MANUAL) {

			AE_GetISOEntryByISONum(manualISONum, &ISOEntry);
			AE_GetExpGainByEntry(sID, ISOEntry, &finalGain);
		} else {
			finalISONum = (CVI_U64)autoTime * autoISONum / manualTime;
			finalISONum = AE_LimitISONum(sID, finalISONum);
			AE_GetISOEntryByISONum(finalISONum, &ISOEntry);
			AE_GetExpGainByEntry(sID, ISOEntry, &finalGain);
		}
		finalTime = manualTime;
	} else if (pstAeExposureAttrInfo[sID]->stManual.enGainType == AE_TYPE_ISO &&
			pstAeExposureAttrInfo[sID]->stManual.enISONumOpType == OP_TYPE_MANUAL) {
		finalTime = (CVI_U64)autoTime * autoISONum / manualISONum;
		finalTime = AE_LimitManualTime(sID, finalTime);
		AE_GetISOEntryByISONum(manualISONum, &ISOEntry);
		AE_GetExpGainByEntry(sID, ISOEntry, &finalGain);
	}

	aGain = finalGain.u32AGain;
	dGain = finalGain.u32DGain;
	ispDGain = finalGain.u32ISPDGain;
	finalTotalGain = (CVI_U64)finalGain.u32AGain * finalGain.u32DGain *
		finalGain.u32ISPDGain;

	if (aGainFix || dGainFix || ispDGainFix) {
		if (aGainFix && dGainFix && ispDGainFix) {
			aGain = manualAGain;
			dGain = manualDGain;
			ispDGain = manualIspDGain;
		} else if (aGainFix && dGainFix && (!ispDGainFix)) {
			aGain = manualAGain;
			dGain = manualDGain;
			ispDGain = finalTotalGain / ((CVI_U64)aGain * dGain);
			ispDGain = AE_LimitManualISPDGain(sID, ispDGain);
		} else if (aGainFix && (!dGainFix) && ispDGainFix) {
			aGain = manualAGain;
			ispDGain = manualIspDGain;
			dGain = finalTotalGain / ((CVI_U64)aGain * ispDGain);
			if (AeInfo[sID].u8DGainAccuType == AE_ACCURACY_DB) {
				AE_GetNodeDGain(sID, &dGain);
			}
			dGain = AE_LimitManualDGain(sID, dGain);
		} else if ((!aGainFix) && dGainFix && ispDGainFix) {
			dGain = manualDGain;
			ispDGain = manualIspDGain;
			aGain = finalTotalGain / ((CVI_U64)dGain * ispDGain);
			aGain = AE_LimitManualAGain(sID, aGain);
		} else if (aGainFix && (!dGainFix) && (!ispDGainFix)) {
			aGain = manualAGain;
			dGain = finalTotalGain / ((CVI_U64)aGain * AE_GAIN_BASE);
			if (AeInfo[sID].u8DGainAccuType == AE_ACCURACY_DB) {
				AE_GetNodeDGain(sID, &dGain);
				dGain = AE_LimitManualDGain(sID, dGain);
				ispDGain = finalTotalGain / ((CVI_U64)aGain * dGain);
				ispDGain = AE_LimitManualISPDGain(sID, ispDGain);
			} else {
				dGain = AE_LimitManualDGain(sID, dGain);
				ispDGain = finalTotalGain / ((CVI_U64)aGain * dGain);
				ispDGain = AE_LimitManualISPDGain(sID, ispDGain);
			}
		} else if ((!aGainFix) && dGainFix && (!ispDGainFix)) {
			dGain = manualIspDGain;
			aGain = finalTotalGain / ((CVI_U64)dGain * AE_GAIN_BASE);
			aGain = AE_LimitManualAGain(sID, aGain);
			ispDGain = finalTotalGain / ((CVI_U64)aGain * dGain);
			ispDGain = AE_LimitManualISPDGain(sID, ispDGain);
		} else if ((!aGainFix) && (!dGainFix) && ispDGainFix) {
			ispDGain = manualIspDGain;
			aGain = finalTotalGain / ((CVI_U64)ispDGain * AE_GAIN_BASE);
			aGain = AE_LimitManualAGain(sID, aGain);
			if (aGain <= maxAGain) {
				dGain = finalTotalGain / ((CVI_U64)aGain * ispDGain);
				dGain = AE_LimitManualDGain(sID, dGain);
			} else {
				dGain = finalTotalGain / ((CVI_U64)AeInfo[sID].u32AGainMax * ispDGain);
				if (AeInfo[sID].u8DGainAccuType == AE_ACCURACY_DB) {
					AE_GetNodeDGain(sID, &dGain);
					dGain = AE_LimitManualDGain(sID, dGain);
					aGain = finalTotalGain / ((CVI_U64)dGain * ispDGain);
					aGain = AE_LimitManualAGain(sID, aGain);
				} else {
					aGain = AeInfo[sID].u32AGainMax;
					dGain = finalTotalGain / ((CVI_U64)aGain * ispDGain);
					dGain = AE_LimitManualDGain(sID, dGain);
				}
			}
		}

		if (pstAeExposureAttrInfo[sID]->stManual.enExpTimeOpType == OP_TYPE_AUTO) {
			finalTime = (CVI_U64)autoTime * finalTotalGain / ((CVI_U64)aGain * dGain * ispDGain);
			finalTime = AE_LimitManualTime(sID, finalTime);
		}
	}

	if (!AeInfo[sID].bWDRMode) {
		fps = (CVI_FLOAT)1000000 / AAA_DIV_0_TO_1(finalTime);
		fpsChange = (fps < AeInfo[sID].fDefaultFps) ? 1 : 0;
	}

	AeInfo[sID].bAeFpsChange = fpsChange;

	AeInfo[sID].stExp[AE_LE].u32ExpTime = finalTime;
	AeInfo[sID].stExp[AE_LE].fIdealExpTime = finalTime;
	AeInfo[sID].stExp[AE_LE].stExpGain.u32AGain = aGain;
	AeInfo[sID].stExp[AE_LE].stExpGain.u32DGain = dGain;
	AeInfo[sID].stExp[AE_LE].stExpGain.u32ISPDGain = ispDGain;

	if (AeInfo[sID].bWDRMode) {
		AeInfo[sID].stExp[AE_SE].stExpGain.u32AGain = aGain;
		AeInfo[sID].stExp[AE_SE].stExpGain.u32DGain = dGain;
		AeInfo[sID].stExp[AE_SE].stExpGain.u32ISPDGain = ispDGain;
		if (AeInfo[sID].u32ManualWDRSETime) {
			AeInfo[sID].stExp[AE_SE].u32ExpTime = AeInfo[sID].u32ManualWDRSETime;
			AeInfo[sID].stExp[AE_SE].fIdealExpTime = AeInfo[sID].u32ManualWDRSETime;
		}
		if (AeInfo[sID].u32ManualWDRSEISPDgain)
			AeInfo[sID].stExp[AE_SE].stExpGain.u32ISPDGain = AeInfo[sID].u32ManualWDRSEISPDgain;
	}

	if (AE_GetDebugMode(sID) == 96) {
		printf("tv:%d sv:%d\n", AeInfo[sID].stApex[AE_LE].s16TVEntry,
			AeInfo[sID].stApex[AE_LE].s16SVEntry);
		printf("TF:%d ISOF:%d UISO:%d AF:%d DF:%d IDF:%d\n",
			pstAeExposureAttrInfo[sID]->stManual.enExpTimeOpType,
			pstAeExposureAttrInfo[sID]->stManual.enGainType,
			pstAeExposureAttrInfo[sID]->stManual.enISONumOpType,
			aGainFix, dGainFix, ispDGainFix);
		printf("Auto T:%u ISO:%d AG:%u DG:%u IG:%u\n", autoTime, autoISONum,
			autoGain.u32AGain, autoGain.u32DGain, autoGain.u32ISPDGain);
		printf("Manu T:%u ISO:%d AG:%u DG:%u IG:%u\n", finalTime, finalISONum,
			aGain, dGain, ispDGain);
	}
}


void AE_GetTVSVExposureRoute(CVI_U8 sID, AE_WDR_FRAME wdrFrm, PSEXPOSURE_TV_SV_CURVE *pPreviewTVSVExposureCurve,
	CVI_U8 *curveSize)
{
	CVI_S16 tvEntry, ISOEntry;
	CVI_U8 i, routeSize;
	CVI_U32 expTime, sysGain, expMaxTime;

	sID = AE_CheckSensorID(sID);

	expMaxTime = (wdrFrm == AE_SE) ? AeInfo[sID].u32WDRSEExpTimeMax : AeInfo[sID].u32ExpTimeMax;

	if (AeInfo[sID].bPubFpsChange) {
		routeSize = (pstAeExposureAttrInfo[sID]->bAERouteExValid) ?
			pstAeRouteExInfo[sID][wdrFrm]->u32TotalNum : pstAeRouteInfo[sID][wdrFrm]->u32TotalNum;
		for (i = 0; i < routeSize; ++i) {
			if (userDefineRoute[sID][wdrFrm][i].isMaxTime) {
				pstAeRouteExInfo[sID][wdrFrm]->astRouteExNode[i].u32IntTime = expMaxTime;
				pstAeRouteInfo[sID][wdrFrm]->astRouteNode[i].u32IntTime = expMaxTime;
			}
		}
	}

	if (pstAeExposureAttrInfo[sID]->bAERouteExValid) {
		routeSize = pstAeRouteExInfo[sID][wdrFrm]->u32TotalNum;

		for (i = 0; i < routeSize; ++i) {
			expTime = pstAeRouteExInfo[sID][wdrFrm]->astRouteExNode[i].u32IntTime;
			sysGain = AE_CalTotalGain(pstAeRouteExInfo[sID][wdrFrm]->astRouteExNode[i].u32Again,
				pstAeRouteExInfo[sID][wdrFrm]->astRouteExNode[i].u32Dgain,
				pstAeRouteExInfo[sID][wdrFrm]->astRouteExNode[i].u32IspDgain);

			AE_GetTvEntryByTime(sID, expTime, &tvEntry);
			AE_GetISOEntryByGain(sysGain, &ISOEntry);
			userDefineRoute[sID][wdrFrm][i].NodeTvEntry = tvEntry;
			userDefineRoute[sID][wdrFrm][i].NodeSvEntry = ISOEntry;
			userDefineRoute[sID][wdrFrm][i].stNodeGain.u32AGain =
				pstAeRouteExInfo[sID][wdrFrm]->astRouteExNode[i].u32Again;
			userDefineRoute[sID][wdrFrm][i].stNodeGain.u32DGain =
				pstAeRouteExInfo[sID][wdrFrm]->astRouteExNode[i].u32Dgain;
			userDefineRoute[sID][wdrFrm][i].stNodeGain.u32ISPDGain =
				pstAeRouteExInfo[sID][wdrFrm]->astRouteExNode[i].u32IspDgain;
			userDefineRoute[sID][wdrFrm][i].isMaxTime = (expTime == expMaxTime) ? 1 : 0;
		}
	} else {
		routeSize = pstAeRouteInfo[sID][wdrFrm]->u32TotalNum;

		for (i = 0; i < routeSize; ++i) {
			expTime = pstAeRouteInfo[sID][wdrFrm]->astRouteNode[i].u32IntTime;
			sysGain = pstAeRouteInfo[sID][wdrFrm]->astRouteNode[i].u32SysGain;
			AE_GetTvEntryByTime(sID, expTime, &tvEntry);
			AE_GetISOEntryByGain(sysGain, &ISOEntry);
			userDefineRoute[sID][wdrFrm][i].NodeTvEntry = tvEntry;
			userDefineRoute[sID][wdrFrm][i].NodeSvEntry = ISOEntry;
			userDefineRoute[sID][wdrFrm][i].isMaxTime = (expTime == expMaxTime) ? 1 : 0;
		}
	}

	*pPreviewTVSVExposureCurve = (PSEXPOSURE_TV_SV_CURVE)&userDefineRoute[sID][wdrFrm];
	*curveSize = routeSize;
	AeInfo[sID].u8RouteNum[wdrFrm] = routeSize;
	AeInfo[sID].pstAeRoute[wdrFrm] = userDefineRoute[sID][wdrFrm];
}

void AE_GetRouteAvEntry(CVI_U8 sID, CVI_S16 BvEntry, CVI_S16 *pAvEntry)
{
	//Todo need lens support iris
	UNUSED(sID);
	UNUSED(BvEntry);
	*pAvEntry = AE_AV_ENTRY_0;
}

void AE_GetAntiFlickerTVSVEntry(CVI_U8 sID, CVI_S16 *pTvEntry, CVI_S16 *pSvEntry)
{

	CVI_S16 tmpTvEntry = *pTvEntry, tmpSvEntry = *pSvEntry;
	CVI_S16 tvEntryDiff = 0, minAntiFlickerTvEntry;
	CVI_S16	bvStep = 0;
	CVI_U16 target;
	CVI_BOOL	isAnti50Hz = 0, routeChange = 0;

	CVI_U32 anti50HzPeriod = 10000; // us
	CVI_U32 anti60HzPeriod = 8333;
	CVI_U32 tmpExpTime = 0;
	CVI_BOOL calculateAntiFlickerTime = 1;

	sID = AE_CheckSensorID(sID);

	if (pstAeExposureAttrInfo[sID]->stAuto.stAntiflicker.enFrequency == AE_FREQUENCE_50HZ)
		isAnti50Hz = 1;
	else
		isAnti50Hz = 0;

	if (pstAeExposureAttrInfo[sID]->stAuto.stAntiflicker.enMode == ISP_ANTIFLICKER_NORMAL_MODE) {
		minAntiFlickerTvEntry = (isAnti50Hz) ? EVTT_ENTRY_1_100SEC : EVTT_ENTRY_1_120SEC;
		if (pstAeExposureAttrInfo[sID]->stAuto.stSubflicker.bEnable &&
			pstAeExposureAttrInfo[sID]->stAuto.stSubflicker.u8LumaDiff) {
			target = AeInfo[sID].u16AdjustTargetLuma[AE_LE] +
					pstAeExposureAttrInfo[sID]->stAuto.stSubflicker.u8LumaDiff * 4;
			target = AAA_MIN(target, AE_MAX_LUMA);
			if (AeInfo[sID].u16FrameLuma[AE_LE] >= target) {
				bvStep = (Log2x100Tbl[AeInfo[sID].u16FrameLuma[AE_LE]] - Log2x100Tbl[target]) *
					ENTRY_PER_BV;
				bvStep = (bvStep > 0) ? ((bvStep + 50) / 100) : ((bvStep - 50) / 100);
				minAntiFlickerTvEntry += bvStep;
			}
		}

		if (isAnti50Hz) {
			if (minAntiFlickerTvEntry >  EVTT_ENTRY_1_100SEC) {
				tvEntryDiff = bvStep;
				tmpSvEntry = (tmpSvEntry > tvEntryDiff) ? (tmpSvEntry - tvEntryDiff) : 0;
				tmpTvEntry = minAntiFlickerTvEntry;
				routeChange = 1;
				calculateAntiFlickerTime = 0;
			} else {
				if (tmpTvEntry >= EVTT_ENTRY_1_100SEC) {
					tvEntryDiff = tmpTvEntry - EVTT_ENTRY_1_100SEC;
					tmpSvEntry = (tmpSvEntry > tvEntryDiff) ? (tmpSvEntry - tvEntryDiff) : 0;
					tmpTvEntry = EVTT_ENTRY_1_100SEC;
					routeChange = 1;
					calculateAntiFlickerTime = 0;
				}
			}
		} else {
			if (minAntiFlickerTvEntry >  EVTT_ENTRY_1_120SEC) {
				tmpSvEntry += (minAntiFlickerTvEntry - tmpTvEntry);
				tmpTvEntry = minAntiFlickerTvEntry;
				routeChange = 1;
				calculateAntiFlickerTime = 0;
			} else {
				if (tmpTvEntry >= EVTT_ENTRY_1_120SEC) {
					tvEntryDiff = tmpTvEntry - EVTT_ENTRY_1_120SEC;
					tmpSvEntry = (tmpSvEntry > tvEntryDiff) ? (tmpSvEntry - tvEntryDiff) : 0;
					tmpTvEntry = EVTT_ENTRY_1_120SEC;
					routeChange = 1;
					calculateAntiFlickerTime = 0;
				}
			}
		}
		if (routeChange)
			tmpSvEntry = AE_LimitISOEntry(sID, tmpSvEntry);
	}

	if (calculateAntiFlickerTime) {
		AE_GetExpTimeByEntry(sID, *pTvEntry, &tmpExpTime);

		if (isAnti50Hz) {
			tmpExpTime = tmpExpTime / anti50HzPeriod;
		} else {
			tmpExpTime = tmpExpTime / anti60HzPeriod;
		}

		if (tmpExpTime > 0) {

			if (isAnti50Hz) {
				tmpExpTime = tmpExpTime * anti50HzPeriod;
			} else {
				tmpExpTime = tmpExpTime * anti60HzPeriod;
			}

			AE_GetTvEntryByTime(sID, tmpExpTime, &tmpTvEntry);
			tmpTvEntry = AE_LimitTVEntry(sID, tmpTvEntry);

			tmpSvEntry += (tmpTvEntry - *pTvEntry);
			tmpSvEntry = AE_LimitISOEntry(sID, tmpSvEntry);
		}
	}

	*pTvEntry = tmpTvEntry;
	*pSvEntry = tmpSvEntry;
}

void AE_GetSlowShutterTVSVEntry(CVI_U8 sID, CVI_S16 BvEntry, CVI_S16 *pTvEntry, CVI_S16 *pSvEntry)
{
	//*pTvEntry -= (*pSvEntry - AeInfo[sID].s16SlowShutterISOEntry);
	*pTvEntry = BvEntry + AeInfo[sID].s16SlowShutterISOEntry;
	*pSvEntry = AeInfo[sID].s16SlowShutterISOEntry;
	if (*pTvEntry <= AeInfo[sID].s16MinRangeTVEntry) {
		//*pSvEntry += (AeInfo[sID].s16MinRangeTVEntry - *pTvEntry);
		*pSvEntry = AeInfo[sID].s16MinRangeTVEntry - BvEntry;
		*pTvEntry = AeInfo[sID].s16MinRangeTVEntry;
		*pSvEntry = AAA_MIN(*pSvEntry, AeInfo[sID].s16MaxISOEntry);
	}
}

void AE_GetRouteTVSVEntry(CVI_U8 sID, AE_WDR_FRAME wdrFrm, CVI_S16 BvEntry, CVI_S16 *pTvEntry, CVI_S16 *pSvEntry)
{
	CVI_U8 i = 0;
	PSEXPOSURE_TV_SV_CURVE psTVSVCurve;
	CVI_U8 curveRouteNum = 0;
	CVI_S16 rangeMinBvEntry = 0, rangeMaxBvEntry = 0, maxBvEntry = 0, minBvEntry = 0;
	CVI_S16 rangeTvEntry, rangeSvEntry, tmpSvEntry;
	CVI_BOOL changeFps = 0;
	AE_WDR_FRAME tmpWdrFrm;

	sID = AE_CheckSensorID(sID);
	if (((!AeInfo[sID].bWDRUseSameGain ) ||
			pstAeWDRExposureAttrInfo[sID]->enExpMainChn == ISP_CHANNEL_SE) && wdrFrm == AE_SE) {
		tmpWdrFrm = AE_SE;
	} else {
		tmpWdrFrm = AE_LE;
	}

	AE_GetTVSVExposureRoute(sID, tmpWdrFrm, &psTVSVCurve, &curveRouteNum);

	//AE_GetBVEntryLimit(&minBvEntry, &maxBvEntry);
	if (curveRouteNum == 0) {
		ISP_LOG_ERR("%s\n", "Error!Route Num = 0");
		return;
	} else if (curveRouteNum == 1) {
		*pTvEntry = psTVSVCurve[0].NodeTvEntry;
		*pSvEntry = psTVSVCurve[0].NodeSvEntry;
		return;
	}

	minBvEntry = psTVSVCurve[curveRouteNum - 1].NodeTvEntry - psTVSVCurve[curveRouteNum - 1].NodeSvEntry;
	maxBvEntry = psTVSVCurve[0].NodeTvEntry - psTVSVCurve[0].NodeSvEntry;

	if (BvEntry >= maxBvEntry) {
		*pTvEntry = psTVSVCurve[0].NodeTvEntry;
		*pSvEntry = psTVSVCurve[0].NodeSvEntry;
	} else if (BvEntry <= minBvEntry) {
		*pTvEntry = psTVSVCurve[curveRouteNum - 1].NodeTvEntry;
		*pSvEntry = psTVSVCurve[curveRouteNum - 1].NodeSvEntry;
	} else {
		for (i = 1; i < curveRouteNum; i++) {
			rangeMaxBvEntry = psTVSVCurve[i - 1].NodeTvEntry - psTVSVCurve[i - 1].NodeSvEntry - 1;
			rangeMinBvEntry = psTVSVCurve[i].NodeTvEntry - psTVSVCurve[i].NodeSvEntry;

			if (BvEntry >= rangeMinBvEntry && BvEntry <= rangeMaxBvEntry) {
				if (psTVSVCurve[i].NodeTvEntry == psTVSVCurve[i-1].NodeTvEntry) {
					*pTvEntry = psTVSVCurve[i].NodeTvEntry;
					*pSvEntry = psTVSVCurve[i].NodeSvEntry - (BvEntry - rangeMinBvEntry);
				} else {
					*pSvEntry = psTVSVCurve[i].NodeSvEntry;
					*pTvEntry = psTVSVCurve[i].NodeTvEntry + BvEntry - rangeMinBvEntry;
				}
				break;
			}
		}
	}

	//when limit ISO,should switch tv/sv after limit for show exp time gain compensation
	rangeTvEntry = *pTvEntry;
	rangeSvEntry = tmpSvEntry = *pSvEntry;
	AAA_LIMIT(rangeTvEntry, AeInfo[sID].s16MinRangeTVEntry, AeInfo[sID].s16MaxRangeTVEntry);

	if (rangeTvEntry != *pTvEntry) {
		rangeSvEntry += (rangeTvEntry - *pTvEntry);
		tmpSvEntry = rangeSvEntry;
	}

	AAA_LIMIT(rangeSvEntry, AeInfo[sID].s16MinRangeISOEntry, AeInfo[sID].s16MaxRangeISOEntry);
	if (rangeSvEntry != tmpSvEntry) {
		rangeTvEntry += (rangeSvEntry - tmpSvEntry);
		AAA_LIMIT(rangeTvEntry, AeInfo[sID].s16MinRangeTVEntry, AeInfo[sID].s16MaxRangeTVEntry);
	}

	*pTvEntry = rangeTvEntry;
	*pSvEntry = rangeSvEntry;

	//note: WDR slow shutter, sensor driver need to alloc max buffer size(min fps) for change fps
	if (pstAeExposureAttrInfo[sID]->stAuto.enAEMode == AE_MODE_SLOW_SHUTTER && !AeInfo[sID].bWDRMode) {
		if (*pTvEntry <= psTVSVCurve[curveRouteNum-1].NodeTvEntry) {
			if (*pSvEntry >= AeInfo[sID].s16SlowShutterISOEntry) {
				AE_GetSlowShutterTVSVEntry(sID, BvEntry, pTvEntry, pSvEntry);
				changeFps = 1;
			}
		}
	}

	AeInfo[sID].enSlowShutterStatus = (changeFps) ? AE_ENTER_SLOW_SHUTTER : AE_LEAVE_SLOW_SHUTTER;
	AeInfo[sID].bAeFpsChange = changeFps;

	if (pstAeExposureAttrInfo[sID]->stAuto.stAntiflicker.bEnable)
		AE_GetAntiFlickerTVSVEntry(sID, pTvEntry, pSvEntry);

	if (AE_GetDebugMode(sID) == 47) {
		printf("wdrFrm : %d \n", wdrFrm);
		for (i = 0; i < curveRouteNum; i++) {
			printf("%d Bv:%d Tv:%d Sv:%d\n", i, psTVSVCurve[i].NodeTvEntry - psTVSVCurve[i].NodeSvEntry,
				psTVSVCurve[i].NodeTvEntry, psTVSVCurve[i].NodeSvEntry);
		}
		printf("Max Bv:%d Tv:%d Sv:%d\n", AeInfo[sID].s16MaxTVEntry - AeInfo[sID].s16MinISOEntry,
			AeInfo[sID].s16MaxTVEntry, AeInfo[sID].s16MinISOEntry);
		printf("Min Bv:%d Tv:%d Sv:%d\n", AeInfo[sID].s16MinTVEntry - AeInfo[sID].s16MaxISOEntry,
			AeInfo[sID].s16MinTVEntry, AeInfo[sID].s16MaxISOEntry);
		printf("Bv:%d rTv:%d rSv:%d Tv:%d Sv:%d\n", BvEntry, rangeTvEntry, rangeSvEntry, *pTvEntry, *pSvEntry);
		printf("SSE:%d SS:%d\n\n", AeInfo[sID].s16SlowShutterISOEntry, AeInfo[sID].enSlowShutterStatus);
	}
}

void AE_GetASMTVAVSVEntry(CVI_U8 sID, CVI_S16 *pTvEntry, CVI_S16 *pAvEntry, CVI_S16 *pSvEntry)
{
	CVI_S16 aAvEntry = *pAvEntry, aTvEntry = *pTvEntry, aSvEntry = *pSvEntry;

	sID = AE_CheckSensorID(sID);
	if (AeInfo[sID].stAssignApex[0].s16TVEntry != TV_AUTO_ENTRY ||
	    AeInfo[sID].stAssignApex[0].s16SVEntry != SV_AUTO_ENTRY) {
		aTvEntry = *pTvEntry;
		aAvEntry = *pAvEntry;
		aSvEntry = *pSvEntry;

		if (AeInfo[sID].stAssignApex[0].s16TVEntry != TV_AUTO_ENTRY) {
			if (AeInfo[sID].stAssignApex[0].s16SVEntry != SV_AUTO_ENTRY) {
				aTvEntry = AeInfo[sID].stAssignApex[0].s16TVEntry;
				aSvEntry = AeInfo[sID].stAssignApex[0].s16SVEntry;
			} else {
				aSvEntry -= (*pTvEntry - AeInfo[sID].stAssignApex[0].s16TVEntry);
				aTvEntry = AeInfo[sID].stAssignApex[0].s16TVEntry;
			}
		} else if (AeInfo[sID].stAssignApex[0].s16SVEntry != SV_AUTO_ENTRY) {
			aTvEntry -= (aSvEntry - AeInfo[sID].stAssignApex[0].s16SVEntry);
			aSvEntry = AeInfo[sID].stAssignApex[0].s16SVEntry;
		}
		AE_LimitTVEntry(sID, aTvEntry);
		AE_LimitISOEntry(sID, aSvEntry);

		*pTvEntry = aTvEntry;
		*pAvEntry = aAvEntry;
		*pSvEntry = aSvEntry;
	}
}

void AE_GetRouteTVAVSVEntry(CVI_U8 sID, AE_WDR_FRAME wdrFrm, CVI_S16 BvEntry, CVI_S16 *pTvEntry,
	CVI_S16 *pAvEntry, CVI_S16 *pSvEntry)
{
	CVI_S16 AvEntryTmp = 0, TvEntryTmp = 0, SvEntryTmp = 0;
	CVI_S16 AvEntryFinal = 0, TvEntryFinal = 0, SvEntryFinal = 0;

	sID = AE_CheckSensorID(sID);
	BvEntry = AE_LimitBVEntry(sID, BvEntry);

	AE_GetRouteAvEntry(sID, BvEntry, &AvEntryTmp);
	AE_GetRouteTVSVEntry(sID, wdrFrm, BvEntry, &TvEntryTmp, &SvEntryTmp);

	AE_GetASMTVAVSVEntry(sID, &TvEntryTmp, &AvEntryTmp, &SvEntryTmp);

	TvEntryFinal = AE_LimitTVEntry(sID, TvEntryTmp);
	AvEntryFinal = AvEntryTmp;
	SvEntryFinal = AE_LimitISOEntry(sID, SvEntryTmp);

	if (TvEntryFinal != TvEntryTmp) { // over limit range
		SvEntryFinal -= (TvEntryTmp - TvEntryFinal);
		SvEntryFinal = AE_LimitISOEntry(sID, SvEntryFinal);
	} else if (SvEntryFinal != SvEntryTmp) { // over limit range
		TvEntryFinal -= (SvEntryTmp - SvEntryFinal);
		TvEntryFinal = AE_LimitTVEntry(sID, TvEntryFinal);
	}

	*pTvEntry = TvEntryFinal;
	*pAvEntry = AvEntryFinal;
	*pSvEntry = SvEntryFinal;
}


void AE_GetWDRLESEEntry(CVI_U8 sID, AE_APEX *LE, AE_APEX *SE)
{
	CVI_S16 tvEntryDiff = 0, tvEntryNode = 0;
	AE_APEX	leExp, seExp;

	if (!AeInfo[sID].bWDRUseSameGain)
		return;

	leExp = *LE;
	seExp = *SE;

	if (pstAeWDRExposureAttrInfo[sID]->enExpMainChn == ISP_CHANNEL_LE) {
		seExp.s16TVEntry -= (seExp.s16SVEntry - leExp.s16SVEntry);
		seExp.s16SVEntry = leExp.s16SVEntry;
		seExp.s16TVEntry = AE_LimitTVEntry(sID, seExp.s16TVEntry);
		if (seExp.s16TVEntry < AeInfo[sID].s16WDRSEMinTVEntry) { // SE max long exposure
			seExp.s16SVEntry -= (seExp.s16TVEntry - AeInfo[sID].s16WDRSEMinTVEntry);
			seExp.s16SVEntry = AE_LimitISOEntry(sID, seExp.s16SVEntry);
			seExp.s16TVEntry = AeInfo[sID].s16WDRSEMinTVEntry;
			leExp.s16TVEntry -= (leExp.s16SVEntry - seExp.s16SVEntry);
			leExp.s16SVEntry = seExp.s16SVEntry;
		}
	} else {
		leExp.s16TVEntry -= (leExp.s16SVEntry - seExp.s16SVEntry);
		leExp.s16SVEntry = seExp.s16SVEntry;
		leExp.s16TVEntry = AE_LimitTVEntry(sID, leExp.s16TVEntry);

		if (leExp.s16TVEntry < AeInfo[sID].s16MinSensorTVEntry) {
			leExp.s16SVEntry -= (leExp.s16TVEntry - AeInfo[sID].s16MinSensorTVEntry);
			leExp.s16SVEntry = AE_LimitISOEntry(sID, leExp.s16SVEntry);
			leExp.s16TVEntry = AeInfo[sID].s16MinSensorTVEntry;
			seExp.s16TVEntry -= (seExp.s16SVEntry - leExp.s16SVEntry);
			seExp.s16SVEntry = leExp.s16SVEntry;
		}
	}

	if (pstAeExposureAttrInfo[sID]->stAuto.stAntiflicker.bEnable) {
		if (pstAeExposureAttrInfo[sID]->stAuto.stAntiflicker.enFrequency == AE_FREQUENCE_50HZ) {
			if (leExp.s16TVEntry <= EVTT_ENTRY_1_33SEC) {
				tvEntryDiff = leExp.s16TVEntry - EVTT_ENTRY_1_33SEC;
				tvEntryNode = EVTT_ENTRY_1_33SEC;
			} else if (leExp.s16TVEntry <= EVTT_ENTRY_1_50SEC) {
				tvEntryDiff = leExp.s16TVEntry - EVTT_ENTRY_1_50SEC;
				tvEntryNode = EVTT_ENTRY_1_50SEC;
			} else if (leExp.s16TVEntry <= EVTT_ENTRY_1_100SEC) {
				tvEntryDiff = leExp.s16TVEntry - EVTT_ENTRY_1_100SEC;
				tvEntryNode = EVTT_ENTRY_1_100SEC;
			}
		} else {
			if (leExp.s16TVEntry <= EVTT_ENTRY_1_30SEC) {
				tvEntryDiff = leExp.s16TVEntry - EVTT_ENTRY_1_30SEC;
				tvEntryNode = EVTT_ENTRY_1_30SEC;
			} else if (leExp.s16TVEntry <= EVTT_ENTRY_1_60SEC) {
				tvEntryDiff = leExp.s16TVEntry - EVTT_ENTRY_1_60SEC;
				tvEntryNode = EVTT_ENTRY_1_60SEC;
			} else if (leExp.s16TVEntry <= EVTT_ENTRY_1_120SEC) {
				tvEntryDiff = leExp.s16TVEntry - EVTT_ENTRY_1_120SEC;
				tvEntryNode = EVTT_ENTRY_1_120SEC;
			}
		}

		if (tvEntryDiff != 0) {
			leExp.s16TVEntry = tvEntryNode;
			leExp.s16SVEntry -= tvEntryDiff;
			seExp.s16TVEntry -= tvEntryDiff;
			seExp.s16SVEntry -= tvEntryDiff;
		}
	}

	if (AE_GetDebugMode(sID) == 44) {
		printf("GM:%d lBv:%d Tv:%d->%d Sv:%d->%d sBv:%d Tv:%d->%d Sv:%d->%d\n",
			AeInfo[sID].enWDRGainMode,
			LE->s16BVEntry, LE->s16TVEntry, leExp.s16TVEntry, LE->s16SVEntry, leExp.s16SVEntry,
			SE->s16BVEntry, SE->s16TVEntry, seExp.s16TVEntry, SE->s16SVEntry, seExp.s16SVEntry);
	}

	if (LE->s16TVEntry != leExp.s16TVEntry || LE->s16SVEntry != leExp.s16SVEntry) {
		*LE = leExp;
	}
	if (SE->s16TVEntry != seExp.s16TVEntry || SE->s16SVEntry != seExp.s16SVEntry) {
		*SE = seExp;
	}
}


void AE_ConfigExposureRoute(CVI_U8 sID)
{
	CVI_U8 i;
	CVI_FLOAT WDRMinRatio, WDRMaxRatio, WDRIdealMinRatio;
	CVI_S16	WDRRatioMinEntry, WDRRatioMaxEntry, WDRRatioEntry;
	CVI_S16 tmpTvEntry, tmpAvEntry, tmpSvEntry;
	CVI_S16	tvEntryDiff;
	CVI_S16	sensorMaxBvEntry, rangeMaxBvEntry, maxBvEntry;
	static CVI_BOOL bStable[AE_SENSOR_NUM];

	sID = AE_CheckSensorID(sID);

	if (AeInfo[sID].s16ManualBvEntry != BV_AUTO_ENTRY)
		AeInfo[sID].stApex[AE_LE].s16BVEntry = AeInfo[sID].s16ManualBvEntry;

	if (AeInfo[sID].bWDRMode) {

		AE_GetRouteTVAVSVEntry(sID, AE_LE, AeInfo[sID].stApex[AE_LE].s16BVEntry,
								&tmpTvEntry, &tmpAvEntry, &tmpSvEntry);

		WDRMinRatio = (CVI_FLOAT)pstAeWDRExposureAttrInfo[sID]->u32ExpRatioMin / AE_WDR_RATIO_BASE;
		WDRMaxRatio = (CVI_FLOAT)pstAeWDRExposureAttrInfo[sID]->u32ExpRatioMax / AE_WDR_RATIO_BASE;

		if (tmpTvEntry >= AeInfo[sID].s16WDRSEMinTVEntry - ENTRY_PER_EV)
			WDRIdealMinRatio = 2.0;
		else {
			tvEntryDiff = AeInfo[sID].s16WDRSEMinTVEntry - tmpTvEntry;
			WDRIdealMinRatio = pow(2, (CVI_FLOAT)tvEntryDiff/ENTRY_PER_EV);
		}

		if (WDRIdealMinRatio >= WDRMaxRatio)
			WDRMinRatio = WDRMaxRatio;
		else
			WDRMinRatio = AAA_MAX(WDRIdealMinRatio, WDRMinRatio);

		WDRRatioEntry = ENTRY_PER_EV * log(WDRMinRatio)/log(2);
		sensorMaxBvEntry = AeInfo[sID].s16MaxSensorTVEntry - AeInfo[sID].s16MinSensorISOEntry - WDRRatioEntry;
		rangeMaxBvEntry = AeInfo[sID].s16MaxRangeTVEntry - AeInfo[sID].s16MinRangeISOEntry;
		maxBvEntry = AAA_MIN(sensorMaxBvEntry, rangeMaxBvEntry);

		if (pstAeWDRExposureAttrInfo[sID]->enExpMainChn == ISP_CHANNEL_LE) {
			if (AeInfo[sID].stApex[AE_LE].s16BVEntry > maxBvEntry)
				AeInfo[sID].stApex[AE_LE].s16BVEntry = maxBvEntry;

			WDRRatioMinEntry = AeInfo[sID].stApex[AE_LE].s16BVEntry + WDRRatioEntry;
			WDRRatioMaxEntry = AeInfo[sID].stApex[AE_LE].s16BVEntry + ENTRY_PER_EV * (log(WDRMaxRatio)/log(2));
			if (AeInfo[sID].bIsStable[AE_LE]) {
				bStable[sID] = CVI_TRUE;
			}
			if (bStable[sID])
				AAA_LIMIT(AeInfo[sID].stApex[AE_SE].s16BVEntry, WDRRatioMinEntry, WDRRatioMaxEntry);
		} else {
			if (AeInfo[sID].stApex[AE_SE].s16BVEntry > maxBvEntry)
				AeInfo[sID].stApex[AE_SE].s16BVEntry = maxBvEntry;
			WDRRatioMaxEntry = AeInfo[sID].stApex[AE_SE].s16BVEntry - WDRRatioEntry;
			WDRRatioMinEntry = AeInfo[sID].stApex[AE_SE].s16BVEntry - ENTRY_PER_EV * (log(WDRMaxRatio)/log(2));
			if (AeInfo[sID].bIsStable[AE_SE]) {
				bStable[sID] = CVI_TRUE;
			}
			if (bStable[sID])
				AAA_LIMIT(AeInfo[sID].stApex[AE_LE].s16BVEntry, WDRRatioMinEntry, WDRRatioMaxEntry);
		}
	}

	for (i = 0; i < AeInfo[sID].u8AeMaxFrameNum; ++i)
		AE_GetRouteTVAVSVEntry(sID, i, AeInfo[sID].stApex[i].s16BVEntry, &AeInfo[sID].stApex[i].s16TVEntry,
				       &AeInfo[sID].stApex[i].s16AVEntry, &AeInfo[sID].stApex[i].s16SVEntry);

	if (AeInfo[sID].bWDRMode) {
		AE_GetWDRLESEEntry(sID, &AeInfo[sID].stApex[AE_LE], &AeInfo[sID].stApex[AE_SE]);
	}
}

void AE_EnableSmooth(CVI_U8 sID, CVI_BOOL enable)
{
	sID = AE_CheckSensorID(sID);
	AeInfo[sID].bEnableSmoothAE = enable;
}

void AE_ConfigSmoothExposure(CVI_U8 sID)
{
	CVI_S16 frmDiffBvEntry[AE_MAX_WDR_FRAME_NUM], tmpBv[AE_MAX_WDR_FRAME_NUM];
	CVI_S16 bvStep[AE_MAX_WDR_FRAME_NUM][AE_MAX_PERIOD_NUM];
	CVI_U8 i, j, k;
	static CVI_S16 preBvEntry[AE_SENSOR_NUM][AE_MAX_WDR_FRAME_NUM];

	sID = AE_CheckSensorID(sID);
	for (k = 0; k < AeInfo[sID].u8AeMaxFrameNum; ++k) {
		if (AeInfo[sID].stSmoothApex[k][0].s16BVEntry == BV_AUTO_ENTRY)
			preBvEntry[sID][k] = AeInfo[sID].stApex[k].s16BVEntry;

		frmDiffBvEntry[k] = AeInfo[sID].stApex[k].s16BVEntry - preBvEntry[sID][k];
		for (i = 0; i < AeInfo[sID].u8MeterFramePeriod; ++i) {
			bvStep[k][i] = frmDiffBvEntry[k] / AeInfo[sID].u8MeterFramePeriod;

			if (i < AAA_ABS(frmDiffBvEntry[k]) % AeInfo[sID].u8MeterFramePeriod)
				bvStep[k][i] = (frmDiffBvEntry[k] > 0) ? bvStep[k][i] + 1 : bvStep[k][i] - 1;

			tmpBv[k] = preBvEntry[sID][k];
			for (j = 0; j <= i; ++j)
				tmpBv[k] += bvStep[k][j];

			AeInfo[sID].stSmoothApex[k][i].s16BVEntry = (AeInfo[sID].u8ConvergeMode[k] == CONVERGE_FAST) ?
									    AeInfo[sID].stApex[k].s16BVEntry : tmpBv[k];

			AE_GetRouteTVAVSVEntry(sID, k, AeInfo[sID].stSmoothApex[k][i].s16BVEntry,
					       &AeInfo[sID].stSmoothApex[k][i].s16TVEntry,
					       &AeInfo[sID].stSmoothApex[k][i].s16AVEntry,
					       &AeInfo[sID].stSmoothApex[k][i].s16SVEntry);
		}

		if (AeInfo[sID].bWDRMode && k == AE_SE) {
			for (i = 0; i < AeInfo[sID].u8MeterFramePeriod; ++i) {
				AE_GetWDRLESEEntry(sID, &AeInfo[sID].stSmoothApex[AE_LE][i],
					&AeInfo[sID].stSmoothApex[AE_SE][i]);

				if (AeInfo[sID].bWDRUseSameGain) {
					if (AeInfo[sID].stSmoothApex[AE_SE][i].s16SVEntry !=
						AeInfo[sID].stSmoothApex[AE_LE][i].s16SVEntry) {

						AeInfo[sID].stSmoothApex[AE_SE][i].s16TVEntry -=
						(AeInfo[sID].stSmoothApex[AE_SE][i].s16SVEntry -
						 AeInfo[sID].stSmoothApex[AE_LE][i].s16SVEntry);

						AeInfo[sID].stSmoothApex[AE_SE][i].s16SVEntry =
							AeInfo[sID].stSmoothApex[AE_LE][i].s16SVEntry;

						AeInfo[sID].stSmoothApex[AE_SE][i].s16TVEntry =
							AAA_MAX(AeInfo[sID].stSmoothApex[AE_SE][i].s16TVEntry,
							AeInfo[sID].s16WDRSEMinTVEntry);
					}
				}
			}
		}

		#if 0
		AeInfo[sID].stApex[k].s16TVEntry = AeInfo[sID].stSmoothApex[k][0].s16TVEntry;
		AeInfo[sID].stApex[k].s16AVEntry = AeInfo[sID].stSmoothApex[k][0].s16AVEntry;
		AeInfo[sID].stApex[k].s16SVEntry = AeInfo[sID].stSmoothApex[k][0].s16SVEntry;
		#endif

		preBvEntry[sID][k] = AeInfo[sID].stApex[k].s16BVEntry;
	}

	for (k = 0; k < AeInfo[sID].u8AeMaxFrameNum; ++k) {
		AeInfo[sID].stApex[k].s16TVEntry = AeInfo[sID].stSmoothApex[k][0].s16TVEntry;
		AeInfo[sID].stApex[k].s16AVEntry = AeInfo[sID].stSmoothApex[k][0].s16AVEntry;
		AeInfo[sID].stApex[k].s16SVEntry = AeInfo[sID].stSmoothApex[k][0].s16SVEntry;
	}
}

void AE_GetSmoothTvAvSvEntry(CVI_U8 sID, CVI_U32 frmNo)
{
	CVI_U8 i, index;

	sID = AE_CheckSensorID(sID);

	index = frmNo % AeInfo[sID].u8MeterFramePeriod;

	if (AeInfo[sID].u8MeterFramePeriod < AeInfo[sID].u8SensorPeriod &&
		AeInfo[sID].u8MeterFramePeriod > 1)
		index = (index == 0) ?  AeInfo[sID].u8MeterFramePeriod - 1 : index;

	for (i = 0; i < AeInfo[sID].u8AeMaxFrameNum; ++i) {
		if (AeInfo[sID].stSmoothApex[i][0].s16BVEntry != BV_AUTO_ENTRY) {
			AeInfo[sID].stApex[i].s16TVEntry =
				AeInfo[sID].stSmoothApex[i][index].s16TVEntry;
			AeInfo[sID].stApex[i].s16AVEntry =
				AeInfo[sID].stSmoothApex[i][index].s16AVEntry;
			AeInfo[sID].stApex[i].s16SVEntry =
				AeInfo[sID].stSmoothApex[i][index].s16SVEntry;
		}
	}
}

CVI_U16 AE_GetFrameLuma(CVI_U8 sID)
{
	sID = AE_CheckSensorID(sID);
	return AeInfo[sID].u16FrameLuma[AE_LE];
}

CVI_U16 AE_GetBrightQuaG(CVI_U8 sID, CVI_U8 WDR_SE)
{
	sID = AE_CheckSensorID(sID);
	return (WDR_SE) ? AeInfo[sID].u16BrightQuaG[AE_SE] : AeInfo[sID].u16BrightQuaG[AE_LE];
}

void AE_GetExpTimeByEntry(CVI_U8 sID, CVI_S16 tvEntry, CVI_U32 *ptime)
{
	CVI_S16 timeIndex = 0;
	CVI_U32 time;

	sID = AE_CheckSensorID(sID);

	if (tvEntry < 0)
		timeIndex = 0;
	else
		timeIndex = tvEntry - EVTT_ENTRY_1_30SEC;
	time = 33333 / AE_CalculatePow2(timeIndex);


	switch (tvEntry) {
	case EVTT_ENTRY_1_25SEC:
		time = 40000; break;
	case EVTT_ENTRY_1_30SEC:
		time = 33333; break;
	case EVTT_ENTRY_1_33SEC:
		time = 30000; break;
	case EVTT_ENTRY_1_50SEC:
		time = 20000; break;
	case EVTT_ENTRY_1_60SEC:
		time = 16666; break;
	case EVTT_ENTRY_1_100SEC:
		time = 10000; break;
	case EVTT_ENTRY_1_120SEC:
		time = 8333; break;
	default:
		break;
	}


	if (tvEntry == AeInfo[sID].s16MinSensorTVEntry)
		time = AeInfo[sID].u32ExpTimeMax;

	*ptime = time;
}

void AE_GetIdealExpTimeByEntry(CVI_U8 sID, CVI_S16 tvEntry, CVI_FLOAT *ptime)
{
	CVI_S16 timeIndex = 0;
	CVI_FLOAT time;

	sID = AE_CheckSensorID(sID);

	if (tvEntry < 0)
		timeIndex = 0;
	else
		timeIndex = tvEntry - EVTT_ENTRY_1_30SEC;

	time = 33333 / AE_CalculatePow2(timeIndex);


	switch (tvEntry) {
	case EVTT_ENTRY_1_25SEC:
		time = 40000; break;
	case EVTT_ENTRY_1_30SEC:
		time = 33333; break;
	case EVTT_ENTRY_1_33SEC:
		time = 30000; break;
	case EVTT_ENTRY_1_50SEC:
		time = 20000; break;
	case EVTT_ENTRY_1_60SEC:
		time = 16666; break;
	case EVTT_ENTRY_1_100SEC:
		time = 10000; break;
	case EVTT_ENTRY_1_120SEC:
		time = 8333; break;
	default:
		break;
	}


	if (tvEntry == AeInfo[sID].s16MinSensorTVEntry)
		time = AeInfo[sID].u32ExpTimeMax;
	*ptime = time;
}


void AE_GetExpGainByEntry(CVI_U8 sID, CVI_S16 SvEntry, AE_GAIN *pstGain)
{
	AE_GAIN aeGain;
	CVI_S16 aGainEntry = 0, dGainEntry = 0, ispDGainEntry = 0;

	sID = AE_CheckSensorID(sID);

	if (SvEntry < 0) {
		pstGain->u32AGain = AE_GAIN_BASE;
		pstGain->u32DGain = AE_GAIN_BASE;
		pstGain->u32ISPDGain = AE_GAIN_BASE;
		ISP_LOG_WARNING("SvEntry:%d (< 0)\n", SvEntry);
		return;
	}

	aGainEntry = SvEntry;
	dGainEntry = 0;
	ispDGainEntry = 0;

	if (aGainEntry > AeInfo[sID].s16AGainMaxEntry) {
		aGainEntry = AeInfo[sID].s16AGainMaxEntry;

		if (AeInfo[sID].bISPDGainFirst) { //again -> ispdgain -> dgain
			ispDGainEntry = SvEntry - aGainEntry;
			if (ispDGainEntry > AeInfo[sID].s16ISPDGainMaxEntry) {
				ispDGainEntry = AeInfo[sID].s16ISPDGainMaxEntry;
				dGainEntry = SvEntry - aGainEntry - ispDGainEntry;
				dGainEntry = AAA_MIN(dGainEntry, AeInfo[sID].s16DGainMaxEntry);
			}
		} else { //again -> dgain -> ispdgain
			dGainEntry = SvEntry - aGainEntry;
			if (dGainEntry > AeInfo[sID].s16DGainMaxEntry) {
				dGainEntry = AeInfo[sID].s16DGainMaxEntry;
				ispDGainEntry = SvEntry - aGainEntry - dGainEntry;
				ispDGainEntry = AAA_MIN(ispDGainEntry, AeInfo[sID].s16ISPDGainMaxEntry);
			}
		}
	}

	AE_GetGainBySvEntry(sID, AE_AGAIN, aGainEntry, &aeGain.u32AGain);
	AE_GetGainBySvEntry(sID, AE_DGAIN, dGainEntry, &aeGain.u32DGain);
	AE_GetGainBySvEntry(sID, AE_ISPDGAIN, ispDGainEntry, &aeGain.u32ISPDGain);

	AE_ConfigDBTypeDgain(sID, &aeGain);

	AAA_LIMIT(aeGain.u32AGain, AeInfo[sID].u32AGainMin, AeInfo[sID].u32AGainMax);
	AAA_LIMIT(aeGain.u32DGain, AeInfo[sID].u32DGainMin, AeInfo[sID].u32DGainMax);
	AAA_LIMIT(aeGain.u32ISPDGain, AeInfo[sID].u32ISPDGainMin, AeInfo[sID].u32ISPDGainMax);

	pstGain->u32AGain = aeGain.u32AGain;
	pstGain->u32DGain = aeGain.u32DGain;
	pstGain->u32ISPDGain = aeGain.u32ISPDGain;
}


void AE_GetTvEntryByTime(CVI_U8 sID, CVI_U32 time, CVI_S16 *ptvEntry)
{
	CVI_S16 tmpTvEntry;

	UNUSED(sID);

	if (time == 0)
		time = 1;

	switch (time) {
	case 8333:
				tmpTvEntry = EVTT_ENTRY_1_120SEC;
				break;
	case 10000:
				tmpTvEntry = EVTT_ENTRY_1_100SEC;
				break;
	case 16666:
				tmpTvEntry = EVTT_ENTRY_1_60SEC;
				break;
	case 20000:
				tmpTvEntry = EVTT_ENTRY_1_50SEC;
				break;
	case 30000:
				tmpTvEntry = EVTT_ENTRY_1_33SEC;
				break;
	case 33333:
				tmpTvEntry = EVTT_ENTRY_1_30SEC;
				break;
	case 40000:
				tmpTvEntry = EVTT_ENTRY_1_25SEC;
				break;
	case 50000:
				tmpTvEntry = EVTT_ENTRY_1_20SEC;
				break;
	case 66666:
				tmpTvEntry = EVTT_ENTRY_1_15SEC;
				break;
	case 125000:
				tmpTvEntry = EVTT_ENTRY_1_08SEC;
				break;
	case 250000:
				tmpTvEntry = EVTT_ENTRY_1_04SEC;
				break;
	case 500000:
				tmpTvEntry = EVTT_ENTRY_1_02SEC;
				break;
	case 1000000:
				tmpTvEntry = EVTT_ENTRY_1_01SEC;
				break;
	default: {
				tmpTvEntry = ENTRY_PER_EV * AE_CalculateLog2(33333, time);
				tmpTvEntry += EVTT_ENTRY_1_30SEC;
				break;
			}
	}

	*ptvEntry = tmpTvEntry;
}

void AE_GetISOEntryByGain(CVI_U32 Gain, CVI_S16 *pISOEntry)
{
	*pISOEntry = ENTRY_PER_EV * AE_CalculateLog2(Gain, AE_GAIN_BASE);
}

void AE_GetISOEntryByISONum(CVI_U32 ISONum, CVI_S16 *pISOEntry)
{
	*pISOEntry = ENTRY_PER_EV * AE_CalculateLog2(ISONum, 100);
}


void AE_GetGainBySvEntry(CVI_U8 sID, AE_GAIN_TYPE gainType, CVI_S16 SvEntry, CVI_U32 *Gain)
{
	CVI_S16 maxEntry;
	CVI_U32 maxGain;

	if (gainType == AE_AGAIN) {
		maxEntry = AeInfo[sID].s16AGainMaxEntry;
		maxGain = AeInfo[sID].u32AGainMax;
	} else if (gainType == AE_DGAIN) {
		maxEntry = AeInfo[sID].s16DGainMaxEntry;
		maxGain = AeInfo[sID].u32DGainMax;
	} else if (gainType == AE_ISPDGAIN) {
		maxEntry = AeInfo[sID].s16ISPDGainMaxEntry;
		maxGain = AeInfo[sID].u32ISPDGainMax;
	} else {
		maxEntry = AeInfo[sID].s16AGainMaxEntry + AeInfo[sID].s16DGainMaxEntry +
			AeInfo[sID].s16ISPDGainMaxEntry;
		maxGain = AeInfo[sID].u32TotalGainMax;
	}

	if (SvEntry <= 0)
		*Gain =  AE_GAIN_BASE;
	else if (SvEntry >= maxEntry)
		*Gain = maxGain;
	else
		*Gain = AE_GAIN_BASE * AE_CalculatePow2(SvEntry);
}

void AE_GetGainByISONum(CVI_U32 ISONum, CVI_U32 *Gain)
{
	*Gain = (CVI_U64)ISONum * AE_GAIN_BASE / 100;
}

void AE_GetISONumByGain(CVI_U32 gain, CVI_U32 *ISONum)
{
	*ISONum = (CVI_U64)gain * 100 / AE_GAIN_BASE;
}


CVI_S32 AE_SetExposureAttr(CVI_U8 sID, const ISP_EXPOSURE_ATTR_S *pstExpAttr)
{
	sID = AE_CheckSensorID(sID);
	if (!pstAeMpiExposureAttr[sID]) {
		ISP_LOG_ERR("%s\n", "pstAeMpiExposureAttr is NULL.");
		return CVI_FAILURE;
	}
	*pstAeMpiExposureAttr[sID] = *pstExpAttr;

	AE_SetParameterUpdate(sID, AE_EXPOSURE_ATTR_UPDATE);
	return CVI_SUCCESS;
}

CVI_S32 AE_GetExposureAttr(CVI_U8 sID, ISP_EXPOSURE_ATTR_S *pstExpAttr)
{
	sID = AE_CheckSensorID(sID);
	if (!pstAeMpiExposureAttr[sID]) {
		ISP_LOG_ERR("%s\n", "pstAeMpiExposureAttr is NULL.");
		return CVI_FAILURE;
	}
	*pstExpAttr = *pstAeMpiExposureAttr[sID];
	return CVI_SUCCESS;
}

CVI_S32 AE_SetSmartExposureAttr(CVI_U8 sID, const ISP_SMART_EXPOSURE_ATTR_S *pstSmartExpAttr)
{
	sID = AE_CheckSensorID(sID);
	if (!pstAeMpiSmartExposureAttr[sID]) {
		ISP_LOG_ERR("%s\n", "pstAeMpiSmartExposureAttr is NULL.");
		return CVI_FAILURE;
	}
	*pstAeMpiSmartExposureAttr[sID] = *pstSmartExpAttr;
	AE_SetParameterUpdate(sID, AE_SMART_EXPOSURE_ATTR_UPDATE);
	return CVI_SUCCESS;
}

CVI_S32 AE_GetSmartExposureAttr(CVI_U8 sID, ISP_SMART_EXPOSURE_ATTR_S *pstSmartExpAttr)
{
	sID = AE_CheckSensorID(sID);

	if (!pstAeMpiSmartExposureAttr[sID]) {
		ISP_LOG_ERR("%s\n", "pstAeMpiSmartExposureAttr is NULL.");
		return CVI_FAILURE;
	}
	*pstSmartExpAttr = *pstAeMpiSmartExposureAttr[sID];
	return CVI_SUCCESS;
}


CVI_U32 AE_GetFrameID(CVI_U8 sID, CVI_U32 *frameID)
{
	sID = AE_CheckSensorID(sID);
	*frameID = AeInfo[sID].u32frmCnt;
	return CVI_SUCCESS;
}

void AE_GetExposureLineByTime(CVI_U8 sID, CVI_U32 time, CVI_U32 *line)
{
	CVI_U32 tmpLine;

	sID = AE_CheckSensorID(sID);
	tmpLine = time / AeInfo[sID].fExpLineTime;
	*line = tmpLine;
}

void AE_GetExposureTimeLine(CVI_U8 sID, CVI_U32 time, CVI_U32 *line)
{
	CVI_U32 tmpLine;

	sID = AE_CheckSensorID(sID);
	AE_GetExposureLineByTime(sID, time, &tmpLine);
	AAA_LIMIT(tmpLine, AeInfo[sID].u32ExpLineMin, AeInfo[sID].u32ExpLineMax);
	*line = tmpLine;
}

void AE_GetExposureTimeIdealLine(CVI_U8 sID, CVI_FLOAT time, CVI_FLOAT *line)
{
	CVI_FLOAT tmpLine;

	sID = AE_CheckSensorID(sID);
	tmpLine = time / AeInfo[sID].fExpLineTime;
	if (time == AeInfo[sID].u32ExpTimeMax)
		tmpLine = AeInfo[sID].u32ExpLineMax;
	AAA_LIMIT(tmpLine, AeInfo[sID].u32ExpLineMin, AeInfo[sID].u32ExpLineMax);
	*line = tmpLine;
}


void AE_GetExpTimeByLine(CVI_U8 sID, CVI_U32 line, CVI_U32 *time)
{
	sID = AE_CheckSensorID(sID);
	*time = line * AeInfo[sID].fExpLineTime;
}

void AE_GetCurrentInfo(CVI_U8 sID, SAE_INFO **stAeInfo)
{
	sID = AE_CheckSensorID(sID);
	*stAeInfo = &AeInfo[sID];
}

void AE_GetHistogram(CVI_U8 sID, CVI_U32 *histogram)
{
	sID = AE_CheckSensorID(sID);
	if (!AeInfo[sID].pu32Histogram[AE_LE]) {
		ISP_LOG_ERR("%s\n", "ae pu32Histogram NULL.");
		return;
	}
	memcpy(histogram, AeInfo[sID].pu32Histogram[AE_LE], HIST_NUM * sizeof(CVI_U32));
}


CVI_U32 AE_CalculateISONum(CVI_U8 sID, const AE_GAIN *pStGain)
{
	CVI_U32 isoNum = 0;
	CVI_U32 totalGain = 0, AGain, DGain, ISPDGain;

	sID = AE_CheckSensorID(sID);

	AGain = AE_LimitAGain(sID, pStGain->u32AGain);
	DGain = AE_LimitDGain(sID, pStGain->u32DGain);
	ISPDGain = AE_LimitISPDGain(sID, pStGain->u32ISPDGain);
	totalGain = AE_CalTotalGain(AGain, DGain, ISPDGain);
	AE_GetISONumByGain(totalGain, &isoNum);

	return isoNum;
}

CVI_U32 AE_CalculateBLCISONum(CVI_U8 sID, const AE_GAIN *pStGain)
{
	CVI_U32 isoNumTable[] = {100, 200, 400, 800, 1600, 3200, 6400, 12800, 25600,
							51200, 102400, 204800, 409600};
	CVI_U32 isoNum = 0;
	CVI_U32 totalGain = 0;
	CVI_U8 i, isoTableSize;

	if (AeInfo[sID].enBlcType == AE_BLC_TYPE_LADDER) {
		totalGain = AE_CalTotalGain(AeInfo[sID].u32AGainMax, pStGain->u32DGain,
			AE_GAIN_BASE);
		AE_GetISONumByGain(totalGain, &isoNum);
		isoTableSize = ARRAY_SIZE(isoNumTable);

		for (i = 0; i < isoTableSize; ++i) {
			if (isoNum <= isoNumTable[i])
				break;
		}

		isoNum = (i < isoTableSize) ? isoNumTable[i] : isoNumTable[isoTableSize-1];
	} else {
		totalGain = AE_CalTotalGain(pStGain->u32AGain, pStGain->u32DGain,
			AE_GAIN_BASE);
		AE_GetISONumByGain(totalGain, &isoNum);
	}

	return isoNum;
}


CVI_U32 AE_GetISONum(CVI_U8 sID)
{
	return AeInfo[sID].u32ISONum[AE_LE];
}


CVI_U32 AE_GetBLCISONum(CVI_U8 sID)
{
	return AeInfo[sID].u32BLCISONum[AE_LE];
}


void AE_GetISONumByEntry(CVI_U8 sID, CVI_S16 ISOEntry, CVI_U32 *ISONum)
{
	sID = AE_CheckSensorID(sID);
	*ISONum = 100 * AE_CalculatePow2(ISOEntry);
}

void AE_GetGridLuma(CVI_U8 sID, CVI_U16 ***gridLuma)
{
	sID = AE_CheckSensorID(sID);

	if (!AeInfo[sID].pu16AeStatistics[AE_LE]) {
		ISP_LOG_ERR("%s\n", "ae pu16AeStatistics NULL.");
		return;
	}

	memcpy(gridLuma, AeInfo[sID].pu16AeStatistics[AE_LE],
	       AE_CHANNEL_NUM * AE_ZONE_ROW * AE_ZONE_COLUMN * sizeof(CVI_U16));
}

void AE_GetMeterLuma(CVI_U8 sID, CVI_U16 **meterLuma)
{
	sID = AE_CheckSensorID(sID);
	memcpy(meterLuma, AeInfo[sID].u16MeterLuma[AE_LE], AE_ZONE_ROW * AE_ZONE_COLUMN * sizeof(CVI_U16));
}

void AE_SetAssignISOEntry(CVI_U8 sID, CVI_S16 ISOEntry)
{
	sID = AE_CheckSensorID(sID);
	AeInfo[sID].stAssignApex[AE_LE].s16SVEntry = ISOEntry;
}

void AE_SetAssignTvEntry(CVI_U8 sID, CVI_S16 TvEntry)
{
	sID = AE_CheckSensorID(sID);
	AeInfo[sID].stAssignApex[AE_LE].s16TVEntry = TvEntry;
}

void AE_CalculateAvX100(CVI_U8 sID, CVI_U16 *AvX100)
{
	CVI_U16 tmpFno, defaultFno;
	CVI_U16 avEntry;

	avEntry = (CVI_U16)AeInfo[sID].stApex[AE_LE].s16AVEntry;
	defaultFno = ISP_IRIS_F_NO_1_0;
	tmpFno = ISP_IRIS_F_NO_1_0;

	if (pstAeExposureAttrInfo[sID]->bAERouteExValid &&
		pstAeRouteExInfo[sID][AE_LE]->u32TotalNum > 0) {
		avEntry = AAA_MIN(avEntry, pstAeRouteExInfo[sID][AE_LE]->u32TotalNum - 1);
		tmpFno = pstAeRouteExInfo[sID][AE_LE]->astRouteExNode[avEntry].enIrisFNO;
		//avoid default is 0
		tmpFno = (tmpFno == ISP_IRIS_F_NO_32_0) ? ISP_IRIS_F_NO_1_0 : tmpFno;
	} else if (pstAeRouteInfo[sID][AE_LE]->u32TotalNum > 0) {
		avEntry = AAA_MIN(avEntry,  pstAeRouteInfo[sID][AE_LE]->u32TotalNum - 1);
		tmpFno = pstAeRouteInfo[sID][AE_LE]->astRouteNode[avEntry].enIrisFNO;
		//avoid default is 0
		tmpFno = (tmpFno == ISP_IRIS_F_NO_32_0) ? ISP_IRIS_F_NO_1_0 : tmpFno;
	}

	*AvX100 = (defaultFno - tmpFno) * 100;
	AeInfo[sID].u16AvX100 = *AvX100;
}

void AE_CalCurrentLvX100(CVI_U8 sID)
{
	CVI_U8 i, mainID;
	CVI_U16 tmpAvX100;

	static CVI_S16 tmpLVX100[AE_SENSOR_NUM][AE_MAX_WDR_FRAME_NUM];

	sID = AE_CheckSensorID(sID);
	AE_CalculateAvX100(sID, &tmpAvX100);

	for (i = 0; i < AeInfo[sID].u8AeMaxFrameNum; ++i) {
		if (AeInfo[sID].u8ConvergeMode[AE_LE] == CONVERGE_FAST) {
			if (AeInfo[sID].u8FastConvergeFrmCnt % AeInfo[sID].u8MeterFramePeriod == 0) {
				tmpLVX100[sID][i] = 700 + (AeInfo[sID].stApex[i].s16TVEntry -
					AeInfo[sID].stApex[i].s16SVEntry + AeInfo[sID].s16LvBvStep[i] -
					EVTT_ENTRY_1_30SEC) * 100 / ENTRY_PER_EV;
				tmpLVX100[sID][i] += tmpAvX100;
			}
		} else {
			tmpLVX100[sID][i] = 700 + (AeInfo[sID].stApex[i].s16TVEntry - AeInfo[sID].stApex[i].s16SVEntry +
				AeInfo[sID].s16LvBvStep[i] - EVTT_ENTRY_1_30SEC) * 100 / ENTRY_PER_EV;
			tmpLVX100[sID][i] += tmpAvX100;
		}
		tmpLVX100[sID][i] += (pstAeExposureAttrInfo[sID]->s8SensorSensitivity * 100);
		AeInfo[sID].s16LvX100[i] = tmpLVX100[sID][i];
	}
	if (AeInfo[sID].stStitchAttr.enable && !AeInfo[sID].stStitchAttr.bMainPipe) {
		mainID = sID == 0 ? 1 : 0;
		AeInfo[sID].s16LvX100[AE_LE] = AeInfo[mainID].s16LvX100[AE_LE];
	}
}

CVI_S16 AE_GetCurrentLvX100(CVI_U8 sID)
{
	CVI_FLOAT fLightValue = 0;

	sID = AE_CheckSensorID(sID);

	if (pstAeExposureAttrInfo[sID]) {
		if (pstAeExposureAttrInfo[sID]->enOpType == OP_TYPE_MANUAL) {
			memcpy((CVI_U8 *)&fLightValue,
				(CVI_U8 *)&pstAeExposureAttrInfo[sID]->stAuto.au32Reserve[0],
				sizeof(CVI_FLOAT));

			if (fLightValue != 0) {
				AeInfo[sID].s16LvX100[AE_LE] = (CVI_S16) (fLightValue * (CVI_FLOAT) 100.0);
			}
		}
	}

	return AeInfo[sID].s16LvX100[AE_LE];
}



void AE_SetISPDGainPriority(CVI_U8 sID, CVI_U8 enable)
{
	sID = AE_CheckSensorID(sID);
	AeInfo[sID].bISPDGainFirst = enable;
}

CVI_U32 AE_CalTotalGain(CVI_U32 AGain, CVI_U32 DGain, CVI_U32 ISPDGain)
{
	CVI_U32 totalGain;

	totalGain = ((CVI_FLOAT)AGain / AE_GAIN_BASE) * ((CVI_FLOAT)DGain / AE_GAIN_BASE) *
		    ((CVI_FLOAT)ISPDGain / AE_GAIN_BASE) * AE_GAIN_BASE;

	return totalGain;
}

CVI_BOOL AE_CalculateWDRExpRatio(const AE_EXPOSURE *pStExp, CVI_U32 *pExpRatio)
{
	CVI_U32 expRatio;
	//expRatioDiffRatio;
	CVI_U64	totalExp[AE_MAX_WDR_FRAME_NUM];

	totalExp[AE_LE] = (CVI_U64)pStExp[AE_LE].u32SensorExpTimeLine *
			AE_CalTotalGain(pStExp[AE_LE].stSensorExpGain.u32AGain,
			pStExp[AE_LE].stSensorExpGain.u32DGain,
			pStExp[AE_LE].stSensorExpGain.u32ISPDGain);

	totalExp[AE_SE] = (CVI_U64)pStExp[AE_SE].u32SensorExpTimeLine *
		AE_CalTotalGain(pStExp[AE_SE].stSensorExpGain.u32AGain,
		pStExp[AE_SE].stSensorExpGain.u32DGain,
		pStExp[AE_SE].stSensorExpGain.u32ISPDGain);

	expRatio = totalExp[AE_LE] * AE_WDR_RATIO_BASE / AAA_DIV_0_TO_1(totalExp[AE_SE]);
	#if 0
	if (expRatio > pstAeWDRExposureAttrInfo[sID]->u32ExpRatioMax) {
		expRatioDiffRatio = (expRatio - pstAeWDRExposureAttrInfo[sID]->u32ExpRatioMax) * 100 /
			pstAeWDRExposureAttrInfo[sID]->u32ExpRatioMax;
		if (expRatioDiffRatio < 1)
			expRatio = pstAeWDRExposureAttrInfo[sID]->u32ExpRatioMax;
	}
	#endif
	AAA_LIMIT(expRatio, MIN_WDR_RATIO, MAX_WDR_RATIO);

	*pExpRatio = expRatio;
	return CVI_SUCCESS;
}

CVI_S32 AE_SetRouteInfo(CVI_U8 sID, AE_WDR_FRAME wdrFrm, ISP_AE_ROUTE_S *routeInfo)
{
	CVI_U8 i, nodeNum, realNodeNum = 0, realNodeExNum = 0;
	CVI_U32	maxTime, minTime, maxGain, minGain;
	CVI_U32	nodeTime, nodeGain;
	CVI_U64 nodeExposure, maxExposure, minExposure;
	CVI_U32	realNodeTime = 0, realNodeGain = 0;
	CVI_U32	preNodeTime = 0, preNodeGain = 0;
	ISP_AE_ROUTE_S tmpRoute;

	memset(&tmpRoute, 0, sizeof(tmpRoute));

	if (!pstAeRouteInfo[sID][wdrFrm]) {
		ISP_LOG_ERR("%d %s\n", wdrFrm, "pstAeRouteInfo NULL.");
		return CVI_FAILURE;
	}

	if (pstAeRouteInfo[sID][wdrFrm]->u32TotalNum < 1) {
		ISP_LOG_ERR("%d %s\n", wdrFrm, "Route nodeNum is 0.");
		return CVI_FAILURE;
	}

	//Route
	nodeNum = pstAeRouteInfo[sID][wdrFrm]->u32TotalNum;
	minTime = AAA_MAX(pstAeRouteInfo[sID][wdrFrm]->astRouteNode[0].u32IntTime,
		pstAeExposureAttrInfo[sID]->stAuto.stExpTimeRange.u32Min);
	minTime = AAA_MAX(minTime, AeInfo[sID].u32ExpTimeMin);

	maxTime = AAA_MIN(pstAeRouteInfo[sID][wdrFrm]->astRouteNode[nodeNum-1].u32IntTime,
		pstAeExposureAttrInfo[sID]->stAuto.stExpTimeRange.u32Max);
	maxTime = AAA_MIN(maxTime, AeInfo[sID].u32ExpTimeMax);

	minGain = AAA_MAX(pstAeRouteInfo[sID][wdrFrm]->astRouteNode[0].u32SysGain,
		pstAeExposureAttrInfo[sID]->stAuto.stSysGainRange.u32Min);
	minGain = AAA_MAX(minGain, AeInfo[sID].u32TotalGainMin);

	maxGain = AAA_MIN(pstAeRouteInfo[sID][wdrFrm]->astRouteNode[nodeNum-1].u32SysGain,
		pstAeExposureAttrInfo[sID]->stAuto.stSysGainRange.u32Max);
	maxGain = AAA_MIN(maxGain, AeInfo[sID].u32TotalGainMax);

	minExposure = (CVI_U64)minTime * minGain;
	maxExposure = (CVI_U64)maxTime * maxGain;

	for (i = 0; i < nodeNum; ++i) {
		if (pstAeExposureAttrInfo[sID]->stAuto.stAntiflicker.bEnable &&
			pstAeExposureAttrInfo[sID]->stAuto.stAntiflicker.enMode ==
			ISP_ANTIFLICKER_NORMAL_MODE && wdrFrm == AE_LE)
			nodeTime = AeInfo[sID].stExp[AE_LE].u32SensorExpTime;
		else
			nodeTime = pstAeRouteInfo[sID][wdrFrm]->astRouteNode[i].u32IntTime;
		nodeGain = pstAeRouteInfo[sID][wdrFrm]->astRouteNode[i].u32SysGain;
		nodeExposure = (CVI_U64)nodeTime * nodeGain;

		if (nodeExposure <= minExposure) {
			realNodeTime = minTime;
			realNodeGain = minGain;
		} else if (nodeExposure >= maxExposure) {
			realNodeTime = maxTime;
			realNodeGain = maxGain;
		} else {
			realNodeTime = AE_LimitExpTime(sID, nodeTime);
			realNodeGain = nodeGain;
		}

		if (AE_GetDebugMode(sID) == 111) {
			printf("i:%d N:%d RT:%u PT:%u RG:%u PG:%u\n",
				i, realNodeNum, realNodeTime, realNodeGain,
				realNodeTime, realNodeGain);
		}

		if (realNodeExNum < ISP_AE_ROUTE_MAX_NODES &&
			(realNodeTime != preNodeTime || realNodeGain != preNodeGain)) {
			tmpRoute.astRouteNode[realNodeNum].u32IntTime = realNodeTime;
			tmpRoute.astRouteNode[realNodeNum].u32SysGain = realNodeGain;
			tmpRoute.astRouteNode[realNodeNum].enIrisFNO =
				pstAeRouteInfo[sID][wdrFrm]->astRouteNode[realNodeNum].enIrisFNO;
			tmpRoute.astRouteNode[realNodeNum].u32IrisFNOLin =
				1 << tmpRoute.astRouteNode[realNodeNum].enIrisFNO;
			realNodeNum++;
			tmpRoute.u32TotalNum = realNodeNum;
			preNodeTime = realNodeTime;
			preNodeGain = realNodeGain;
		}
	}

	if (AeInfo[sID].enSlowShutterStatus == AE_ENTER_SLOW_SHUTTER && wdrFrm == AE_LE) {
		for (i = 0; i < tmpRoute.u32TotalNum; ++i) {
			realNodeGain = tmpRoute.astRouteNode[i].u32SysGain;
			if (realNodeGain >= pstAeExposureAttrInfo[sID]->stAuto.u32GainThreshold) {
				break;
			}
		}
		tmpRoute.astRouteNode[i].u32SysGain = pstAeExposureAttrInfo[sID]->stAuto.u32GainThreshold;

		//Add 2 node
		tmpRoute.astRouteNode[i+1].u32IntTime =
			pstAeExposureAttrInfo[sID]->stAuto.stExpTimeRange.u32Max;
		tmpRoute.astRouteNode[i+1].u32SysGain =
			pstAeExposureAttrInfo[sID]->stAuto.u32GainThreshold;
		tmpRoute.astRouteNode[i+1].enIrisFNO =
			tmpRoute.astRouteNode[i].enIrisFNO;
		tmpRoute.astRouteNode[i+1].u32IrisFNOLin =
			1 << tmpRoute.astRouteNode[i].enIrisFNO;
		tmpRoute.u32TotalNum++;

		if (realNodeGain != pstAeExposureAttrInfo[sID]->stAuto.u32GainThreshold) {
			tmpRoute.astRouteNode[i+2].u32IntTime =
				pstAeExposureAttrInfo[sID]->stAuto.stExpTimeRange.u32Max;
			tmpRoute.astRouteNode[i+2].u32SysGain = realNodeGain;
			tmpRoute.astRouteNode[i+2].enIrisFNO =
				tmpRoute.astRouteNode[i].enIrisFNO;
			tmpRoute.astRouteNode[i+2].u32IrisFNOLin =
				1 << tmpRoute.astRouteNode[i].enIrisFNO;
			tmpRoute.u32TotalNum++;
		}
	}

	*routeInfo = tmpRoute;

	if (AE_GetDebugMode(sID) == 111)
		AE_SetDebugMode(sID, 0);

	return CVI_SUCCESS;
}

CVI_S32 AE_SetRouteExInfo(CVI_U8 sID, AE_WDR_FRAME wdrFrm, ISP_AE_ROUTE_EX_S *routeExInfo)
{
	CVI_U8 i, j, nodeNum, realNodeExNum = 0, ssNodeCnt = 0;
	CVI_U32	maxAGain, minAGain, maxDGain, minDGain, maxIspDgain, minIspDgain;
	CVI_U32	maxTime, minTime, maxGain, minGain;
	CVI_U32	nodeTime, nodeGain;
	CVI_U64 nodeExposure, maxExposure, minExposure;
	CVI_U32	realNodeExTime = 0, realNodeAGain = 0, realNodeDGain = 0, realNodeIspDGain = 0;
	CVI_U32	preNodeExTime = 0, preNodeAGain = 0, preNodeDGain = 0, preNodeIspDGain = 0;
	ISP_AE_ROUTE_EX_S tmpRouteEx, slowShutterEx;
	AE_ROUTE_CHANGE_ITEM	nodeExChangeItem = 0;

	memset(&tmpRouteEx, 0, sizeof(tmpRouteEx));


	if (!pstAeRouteExInfo[sID][wdrFrm]) {
		ISP_LOG_ERR("%d %s\n", wdrFrm, "pstAeRouteExInfo NULL.");
		return CVI_FAILURE;
	}

	if (pstAeRouteExInfo[sID][wdrFrm]->u32TotalNum < 1) {
		ISP_LOG_ERR("%d %s\n", wdrFrm, "RouteEx nodeNum is 0.");
		return CVI_FAILURE;
	}

	nodeNum = pstAeRouteExInfo[sID][wdrFrm]->u32TotalNum;
	minTime = AAA_MAX(pstAeRouteExInfo[sID][wdrFrm]->astRouteExNode[0].u32IntTime,
		pstAeExposureAttrInfo[sID]->stAuto.stExpTimeRange.u32Min);
	minTime = AAA_MAX(minTime, AeInfo[sID].u32ExpTimeMin);

	maxTime = AAA_MIN(pstAeRouteExInfo[sID][wdrFrm]->astRouteExNode[nodeNum-1].u32IntTime,
		pstAeExposureAttrInfo[sID]->stAuto.stExpTimeRange.u32Max);
	maxTime = AAA_MIN(maxTime, AeInfo[sID].u32ExpTimeMax);

	minAGain = AAA_MAX(pstAeRouteExInfo[sID][wdrFrm]->astRouteExNode[0].u32Again,
		pstAeExposureAttrInfo[sID]->stAuto.stAGainRange.u32Min);
	minAGain = AAA_MAX(minAGain, AeInfo[sID].u32AGainMin);

	minDGain = AAA_MAX(pstAeRouteExInfo[sID][wdrFrm]->astRouteExNode[0].u32Dgain,
		pstAeExposureAttrInfo[sID]->stAuto.stDGainRange.u32Min);
	minDGain = AAA_MAX(minDGain, AeInfo[sID].u32DGainMin);

	minIspDgain = AAA_MAX(pstAeRouteExInfo[sID][wdrFrm]->astRouteExNode[0].u32IspDgain,
		pstAeExposureAttrInfo[sID]->stAuto.stISPDGainRange.u32Min);
	minIspDgain = AAA_MAX(minIspDgain, AeInfo[sID].u32ISPDGainMin);

	maxAGain = AAA_MIN(pstAeRouteExInfo[sID][wdrFrm]->astRouteExNode[nodeNum-1].u32Again,
		pstAeExposureAttrInfo[sID]->stAuto.stAGainRange.u32Max);
	maxAGain = AAA_MIN(maxAGain, AeInfo[sID].u32AGainMax);

	maxDGain = AAA_MIN(pstAeRouteExInfo[sID][wdrFrm]->astRouteExNode[nodeNum-1].u32Dgain,
		pstAeExposureAttrInfo[sID]->stAuto.stDGainRange.u32Max);
	maxDGain = AAA_MIN(maxDGain, AeInfo[sID].u32DGainMax);

	maxIspDgain = AAA_MIN(pstAeRouteExInfo[sID][wdrFrm]->astRouteExNode[nodeNum-1].u32IspDgain,
		pstAeExposureAttrInfo[sID]->stAuto.stISPDGainRange.u32Max);
	maxIspDgain = AAA_MIN(maxIspDgain, AeInfo[sID].u32ISPDGainMax);


	minGain = AE_CalTotalGain(minAGain, minDGain, minIspDgain);
	maxGain = AE_CalTotalGain(maxAGain, maxDGain, maxIspDgain);

	minExposure = (CVI_U64)minTime * minGain;
	maxExposure = (CVI_U64)maxTime * maxGain;

	for (i = 0; i < nodeNum; ++i) {
		if (pstAeExposureAttrInfo[sID]->stAuto.stAntiflicker.bEnable &&
			pstAeExposureAttrInfo[sID]->stAuto.stAntiflicker.enMode ==
			ISP_ANTIFLICKER_NORMAL_MODE && wdrFrm == AE_LE)
			nodeTime = AeInfo[sID].stExp[AE_LE].u32SensorExpTime;
		else
			nodeTime = pstAeRouteExInfo[sID][wdrFrm]->astRouteExNode[i].u32IntTime;

		nodeGain = AE_CalTotalGain(pstAeRouteExInfo[sID][wdrFrm]->astRouteExNode[i].u32Again,
			pstAeRouteExInfo[sID][wdrFrm]->astRouteExNode[i].u32Dgain,
			pstAeRouteExInfo[sID][wdrFrm]->astRouteExNode[i].u32IspDgain);
		nodeExposure = (CVI_U64)nodeTime * nodeGain;

		if (i >= 1) {
			if (pstAeRouteExInfo[sID][wdrFrm]->astRouteExNode[i].u32IntTime !=
				pstAeRouteExInfo[sID][wdrFrm]->astRouteExNode[i-1].u32IntTime)
				nodeExChangeItem = AE_CHANGE_SHUTTER;
			else if (pstAeRouteExInfo[sID][wdrFrm]->astRouteExNode[i].u32Again !=
				pstAeMpiRouteEx[sID][wdrFrm]->astRouteExNode[i-1].u32Again)
				nodeExChangeItem = AE_CHANGE_AGAIN;
			else if (pstAeRouteExInfo[sID][wdrFrm]->astRouteExNode[i].u32Dgain !=
				pstAeRouteExInfo[sID][wdrFrm]->astRouteExNode[i-1].u32Dgain)
				nodeExChangeItem = AE_CHANGE_DGAIN;
			else if (pstAeRouteExInfo[sID][wdrFrm]->astRouteExNode[i].u32IspDgain !=
				pstAeRouteExInfo[sID][wdrFrm]->astRouteExNode[i-1].u32IspDgain)
				nodeExChangeItem = AE_CHANGE_ISPDGAIN;
			else
				nodeExChangeItem = AE_CHANGE_NONE;
		}
		if (nodeExposure <= minExposure) {
			realNodeExTime = minTime;
			realNodeAGain = minAGain;
			realNodeDGain = minDGain;
			realNodeIspDGain = minIspDgain;
		} else if (nodeExposure >= maxExposure) {
			realNodeExTime = (nodeExChangeItem == AE_CHANGE_SHUTTER) ?
				maxTime : preNodeExTime;
			realNodeAGain = (nodeExChangeItem == AE_CHANGE_AGAIN) ?
				maxAGain : preNodeAGain;
			realNodeDGain = (nodeExChangeItem == AE_CHANGE_DGAIN) ?
				maxDGain : preNodeDGain;
			realNodeIspDGain = (nodeExChangeItem == AE_CHANGE_ISPDGAIN) ?
				maxIspDgain : preNodeIspDGain;
		} else {
			if (i == 0) {
				realNodeExTime = AE_LimitExpTime(sID, nodeTime);
				realNodeAGain = AE_LimitAGain(sID,
						pstAeRouteExInfo[sID][wdrFrm]->astRouteExNode[i].u32Again);
				realNodeDGain = AE_LimitDGain(sID,
							pstAeRouteExInfo[sID][wdrFrm]->astRouteExNode[i].u32Dgain);
				realNodeIspDGain = AE_LimitISPDGain(sID,
						pstAeRouteExInfo[sID][wdrFrm]->astRouteExNode[i].u32IspDgain);
			} else {
				if (nodeExChangeItem == AE_CHANGE_SHUTTER) {
					realNodeExTime = AE_LimitExpTime(sID, nodeTime);
					realNodeAGain = preNodeAGain;
					realNodeDGain = preNodeDGain;
					realNodeIspDGain = preNodeIspDGain;
				} else if (nodeExChangeItem == AE_CHANGE_AGAIN) {
					realNodeAGain = AE_LimitAGain(sID,
						pstAeRouteExInfo[sID][wdrFrm]->astRouteExNode[i].u32Again);
					realNodeExTime = preNodeExTime;
					realNodeDGain = preNodeDGain;
					realNodeIspDGain = preNodeIspDGain;
				} else if (nodeExChangeItem == AE_CHANGE_DGAIN) {
					realNodeDGain = AE_LimitDGain(sID,
							pstAeRouteExInfo[sID][wdrFrm]->astRouteExNode[i].u32Dgain);
					realNodeExTime = preNodeExTime;
					realNodeAGain = preNodeAGain;
					realNodeIspDGain = preNodeIspDGain;
				} else if (nodeExChangeItem == AE_CHANGE_ISPDGAIN) {
					realNodeIspDGain = AE_LimitISPDGain(sID,
						pstAeRouteExInfo[sID][wdrFrm]->astRouteExNode[i].u32IspDgain);
					realNodeExTime = preNodeExTime;
					realNodeAGain = preNodeAGain;
					realNodeDGain = preNodeDGain;
				}
			}
		}

		if (AE_GetDebugMode(sID) == 112) {
			printf("i:%d N:%d T:%u PT:%u\n",
				i, realNodeExNum, realNodeExTime, preNodeExTime);
			printf("AG:%u PAG:%u DG:%u PDG:%u IG:%u PIG:%u CI:%d\n",
				realNodeAGain, preNodeAGain, realNodeDGain,
				preNodeDGain, realNodeIspDGain, preNodeIspDGain, nodeExChangeItem);
		}

		if (realNodeExNum < ISP_AE_ROUTE_EX_MAX_NODES &&
			(realNodeExTime != preNodeExTime ||
			realNodeAGain != preNodeAGain ||
			realNodeDGain != preNodeDGain ||
			realNodeIspDGain != preNodeIspDGain)) {
			tmpRouteEx.astRouteExNode[realNodeExNum].u32IntTime = realNodeExTime;
			tmpRouteEx.astRouteExNode[realNodeExNum].u32Again = realNodeAGain;
			tmpRouteEx.astRouteExNode[realNodeExNum].u32Dgain = realNodeDGain;
			tmpRouteEx.astRouteExNode[realNodeExNum].u32IspDgain = realNodeIspDGain;
			tmpRouteEx.astRouteExNode[realNodeExNum].enIrisFNO =
				pstAeRouteExInfo[sID][wdrFrm]->astRouteExNode[realNodeExNum].enIrisFNO;
			tmpRouteEx.astRouteExNode[realNodeExNum].u32IrisFNOLin = 1 <<
							tmpRouteEx.astRouteExNode[realNodeExNum].enIrisFNO;
			realNodeExNum++;

			tmpRouteEx.u32TotalNum = realNodeExNum;

			preNodeExTime = realNodeExTime;
			preNodeAGain = realNodeAGain;
			preNodeDGain = realNodeDGain;
			preNodeIspDGain = realNodeIspDGain;
		}
	}

	if (AeInfo[sID].enSlowShutterStatus == AE_ENTER_SLOW_SHUTTER && wdrFrm == AE_LE) {
		memset(&slowShutterEx, 0, sizeof(slowShutterEx));
		for (i = 0; i < tmpRouteEx.u32TotalNum; ++i) {
			slowShutterEx.astRouteExNode[i] = tmpRouteEx.astRouteExNode[i];
			nodeGain = AE_CalTotalGain(tmpRouteEx.astRouteExNode[i].u32Again,
				tmpRouteEx.astRouteExNode[i].u32Dgain,
				tmpRouteEx.astRouteExNode[i].u32IspDgain);
			if (nodeGain >= pstAeExposureAttrInfo[sID]->stAuto.u32GainThreshold) {
				realNodeAGain = tmpRouteEx.astRouteExNode[i].u32Again;
				realNodeDGain = tmpRouteEx.astRouteExNode[i].u32Dgain;
				realNodeIspDGain = tmpRouteEx.astRouteExNode[i].u32IspDgain;
				ssNodeCnt = i;
				if (i > 0) {
					if (realNodeAGain != tmpRouteEx.astRouteExNode[i-1].u32Again)
						nodeExChangeItem = AE_CHANGE_AGAIN;
					else if (realNodeDGain != tmpRouteEx.astRouteExNode[i-1].u32Dgain)
						nodeExChangeItem = AE_CHANGE_DGAIN;
					else
						nodeExChangeItem = AE_CHANGE_ISPDGAIN;
				}
				break;
			}
		}

		//Add 2 node
		if (nodeExChangeItem == AE_CHANGE_AGAIN) {
			slowShutterEx.astRouteExNode[ssNodeCnt].u32Again =
				(CVI_U64)pstAeExposureAttrInfo[sID]->stAuto.u32GainThreshold *
				AE_GAIN_BASE * AE_GAIN_BASE / (realNodeDGain * realNodeIspDGain);
		} else if (nodeExChangeItem == AE_CHANGE_DGAIN) {
			slowShutterEx.astRouteExNode[ssNodeCnt].u32Dgain =
				(CVI_U64)pstAeExposureAttrInfo[sID]->stAuto.u32GainThreshold *
				AE_GAIN_BASE * AE_GAIN_BASE / (realNodeAGain * realNodeIspDGain);
		} else {
			slowShutterEx.astRouteExNode[ssNodeCnt].u32IspDgain =
				(CVI_U64)pstAeExposureAttrInfo[sID]->stAuto.u32GainThreshold *
				AE_GAIN_BASE * AE_GAIN_BASE / (realNodeAGain * realNodeDGain);
		}

		ssNodeCnt++;
		slowShutterEx.astRouteExNode[ssNodeCnt].u32IntTime =
			pstAeExposureAttrInfo[sID]->stAuto.stExpTimeRange.u32Max;

		slowShutterEx.astRouteExNode[ssNodeCnt].u32Again = slowShutterEx.astRouteExNode[ssNodeCnt-1].u32Again;
		slowShutterEx.astRouteExNode[ssNodeCnt].u32Dgain = slowShutterEx.astRouteExNode[ssNodeCnt-1].u32Dgain;
		slowShutterEx.astRouteExNode[ssNodeCnt].u32IspDgain =
			slowShutterEx.astRouteExNode[ssNodeCnt-1].u32IspDgain;

		slowShutterEx.astRouteExNode[ssNodeCnt].enIrisFNO = tmpRouteEx.astRouteExNode[ssNodeCnt-1].enIrisFNO;
		slowShutterEx.astRouteExNode[ssNodeCnt].u32IrisFNOLin =
			1 << tmpRouteEx.astRouteExNode[ssNodeCnt-1].enIrisFNO;

		if (realNodeAGain != tmpRouteEx.astRouteExNode[realNodeExNum-1].u32Again ||
			realNodeDGain != tmpRouteEx.astRouteExNode[realNodeExNum-1].u32Dgain ||
			realNodeIspDGain != tmpRouteEx.astRouteExNode[realNodeExNum-1].u32IspDgain) {
			ssNodeCnt++;
			slowShutterEx.astRouteExNode[ssNodeCnt].u32IntTime =
				pstAeExposureAttrInfo[sID]->stAuto.stExpTimeRange.u32Max;

			slowShutterEx.astRouteExNode[ssNodeCnt].u32Again = realNodeAGain;
			slowShutterEx.astRouteExNode[ssNodeCnt].u32Dgain = realNodeDGain;
			slowShutterEx.astRouteExNode[ssNodeCnt].u32IspDgain = realNodeIspDGain;

			slowShutterEx.astRouteExNode[ssNodeCnt].enIrisFNO = tmpRouteEx.astRouteExNode[i].enIrisFNO;
			slowShutterEx.astRouteExNode[ssNodeCnt].u32IrisFNOLin =
				1 << tmpRouteEx.astRouteExNode[i].enIrisFNO;
		}

		for (j = i+1 ; j < tmpRouteEx.u32TotalNum; ++j) {
			ssNodeCnt++;
			slowShutterEx.astRouteExNode[ssNodeCnt].u32IntTime =
				pstAeExposureAttrInfo[sID]->stAuto.stExpTimeRange.u32Max;

			slowShutterEx.astRouteExNode[ssNodeCnt].u32Again = tmpRouteEx.astRouteExNode[j].u32Again;
			slowShutterEx.astRouteExNode[ssNodeCnt].u32Dgain = tmpRouteEx.astRouteExNode[j].u32Dgain;
			slowShutterEx.astRouteExNode[ssNodeCnt].u32IspDgain =
				tmpRouteEx.astRouteExNode[j].u32IspDgain;

			slowShutterEx.astRouteExNode[ssNodeCnt].enIrisFNO =
				tmpRouteEx.astRouteExNode[j].enIrisFNO;
			slowShutterEx.astRouteExNode[ssNodeCnt].u32IrisFNOLin =
				1 << tmpRouteEx.astRouteExNode[j].enIrisFNO;
		}
		slowShutterEx.u32TotalNum = ssNodeCnt + 1;
	}

	*routeExInfo = (AeInfo[sID].enSlowShutterStatus == AE_ENTER_SLOW_SHUTTER && wdrFrm == AE_LE) ?
		slowShutterEx : tmpRouteEx;

	if (AE_GetDebugMode(sID) == 112)
		AE_SetDebugMode(sID, 0);

	return CVI_SUCCESS;
}



CVI_S32 AE_GetExposureInfo(CVI_U8 sID, ISP_EXP_INFO_S *pstExpInfo)
{
	sID = AE_CheckSensorID(sID);

	if (pstAeMpiExpInfo == CVI_NULL) {
		ISP_LOG_ERR("%s\n", "pstAeMpiExpInfo is NULL.");
		return CVI_FAILURE;
	}

	*pstExpInfo = *pstAeMpiExpInfo[sID];
	return CVI_SUCCESS;
}

CVI_S32 AE_SetExposureInfo(CVI_U8 sID)
{
	CVI_U32 totalGain = 0;
	CVI_U32 expRatio;
	CVI_S32 res = CVI_SUCCESS;
	CVI_U16 frameLuma;

	sID = AE_CheckSensorID(sID);

	ISP_EXP_INFO_S *pstExpInfo = pstAeMpiExpInfo[sID];

	if (AeInfo[sID].stStitchAttr.enable && !AeInfo[sID].stStitchAttr.bMainPipe &&
			pstAeExposureAttrInfo[sID]->enOpType != OP_TYPE_MANUAL) {
		CVI_U8 mainID = AE_SENSOR_NUM, i;

		for (i = 0; i < AE_SENSOR_NUM; i++) {
			if (AeInfo[i].stStitchAttr.bMainPipe) {
				mainID = i;
				break;
			}
		}

		if (mainID != AE_SENSOR_NUM) {
			memcpy(pstExpInfo, pstAeMpiExpInfo[mainID], sizeof(ISP_EXP_INFO_S));
			return res;
		}
	}

	pstExpInfo->u32ExpTime = AE_LimitExpTime(sID, AeInfo[sID].stExp[AE_LE].u32SensorExpTime);
	pstExpInfo->u32AGain = AE_LimitAGain(sID, AeInfo[sID].stExp[AE_LE].stSensorExpGain.u32AGain);
	pstExpInfo->u32DGain = AE_LimitDGain(sID, AeInfo[sID].stExp[AE_LE].stSensorExpGain.u32DGain);
	pstExpInfo->u32ISPDGain = AE_LimitISPDGain(sID, AeInfo[sID].stExp[AE_LE].stSensorExpGain.u32ISPDGain);
	totalGain = AE_CalTotalGain(pstExpInfo->u32AGain, pstExpInfo->u32DGain,
		pstExpInfo->u32ISPDGain);

	pstExpInfo->bExposureIsMAX = AeInfo[sID].bIsMaxExposure;
	pstExpInfo->u32Fps = AeInfo[sID].fFps * 100; // frame rate * 100
	pstExpInfo->u32ISO = AeInfo[sID].u32ISONum[AE_LE];
	pstExpInfo->u32FirstStableTime = AeInfo[sID].u32FirstStableTime;

	if ((AeInfo[sID].bWDRMode && pstAeWDRExposureAttrInfo[sID]->enExpMainChn == ISP_CHANNEL_SE)) {
		frameLuma = AeInfo[sID].u16AdjustTargetLuma[AE_SE] *
			AE_CalculatePow2(AeInfo[sID].s16BvStepEntry[AE_SE]);
		pstExpInfo->s16HistError = (AeInfo[sID].u16AdjustTargetLuma[AE_SE] - frameLuma) >> 2;
	} else {
		frameLuma = AeInfo[sID].u16AdjustTargetLuma[AE_LE] *
		AE_CalculatePow2(AeInfo[sID].s16BvStepEntry[AE_LE]);

		//shift 10 bits to 8 bits
		pstExpInfo->s16HistError = (AeInfo[sID].u16AdjustTargetLuma[AE_LE] - frameLuma) >> 2;
	}

	if (AeInfo[sID].pu32Histogram[AE_LE])
		memcpy(pstExpInfo->au32AE_Hist256Value, AeInfo[sID].pu32Histogram[AE_LE],
		HIST_NUM * sizeof(CVI_U32));
	frameLuma = AAA_MIN(frameLuma, 1023);
	pstExpInfo->u8AveLum = frameLuma >> 2;

	pstExpInfo->u32LinesPer500ms = AeInfo[sID].u32LinePer500ms;

	pstExpInfo->u32Exposure = AeInfo[sID].stExp[AE_LE].u32SensorExpTimeLine * (totalGain >> 4);
	pstExpInfo->u32MedianExpTime = 0;

	pstExpInfo->u32PirisFNO = 0;

	if (AeInfo[sID].bWDRMode) {
		pstExpInfo->u32LongExpTime = pstExpInfo->u32ExpTime;
		pstExpInfo->u32ShortExpTime = AE_LimitWDRSEExpTime(sID, AeInfo[sID].stExp[AE_SE].u32SensorExpTime);
		pstExpInfo->u32AGainSF = AE_LimitAGain(sID, AeInfo[sID].stExp[AE_SE].stSensorExpGain.u32AGain);
		pstExpInfo->u32DGainSF = AE_LimitDGain(sID, AeInfo[sID].stExp[AE_SE].stSensorExpGain.u32DGain);
		pstExpInfo->u32ISPDGainSF = AE_LimitISPDGain(sID, AeInfo[sID].stExp[AE_SE].stSensorExpGain.u32ISPDGain);
		pstExpInfo->u32ISOSF = AeInfo[sID].u32ISONum[AE_SE];
		frameLuma = AeInfo[sID].u16AdjustTargetLuma[AE_SE] *
			AE_CalculatePow2(AeInfo[sID].s16BvStepEntry[AE_SE]);
		frameLuma = AAA_MIN(frameLuma, 1023);
		pstExpInfo->u8WDRShortAveLuma = frameLuma >> 2;
		AE_CalculateWDRExpRatio(AeInfo[sID].stExp, &expRatio);
		pstExpInfo->u32WDRExpRatio = expRatio;
		pstExpInfo->u32RefExpRatio = (CVI_U32)AeInfo[sID].u16MeterMaxLuma[AE_SE] *
						  expRatio / AAA_DIV_0_TO_1(AeInfo[sID].u16MeterMinLuma[AE_LE]);
		//for PQ tool parameter bAEGainSepCfg enable/disable
		pstExpInfo->bGainSepStatus = (AeInfo[sID].enWDRGainMode == SNS_GAIN_MODE_SHARE ||
			AeInfo[sID].enWDRGainMode == SNS_GAIN_MODE_ONLY_LEF) ? 0 : 1;
	} else {
		pstExpInfo->u32LongExpTime = 0;
		pstExpInfo->u32ShortExpTime = 0;
		pstExpInfo->u32AGainSF = 0;
		pstExpInfo->u32DGainSF = 0;
		pstExpInfo->u32ISPDGainSF = 0;
		pstExpInfo->u32ISOSF = 0;
		pstExpInfo->u8WDRShortAveLuma = 0;
		pstExpInfo->u32RefExpRatio = AeInfo[sID].u16MeterMaxLuma[AE_LE] * AE_WDR_RATIO_BASE /
						  AAA_DIV_0_TO_1(AeInfo[sID].u16MeterMinLuma[AE_LE]);
		pstExpInfo->bGainSepStatus = 0;
	}

	pstExpInfo->u8LEFrameAvgLuma = AeInfo[sID].u16LvFrameLuma[AE_LE] >> 2;
	pstExpInfo->u8SEFrameAvgLuma = AeInfo[sID].u16LvFrameLuma[AE_SE] >> 2;
	pstExpInfo->fLightValue = (CVI_FLOAT)AE_GetCurrentLvX100(sID) / 100;

	if (!pstAeExposureAttrInfo[sID]) {
		ISP_LOG_ERR("%s\n", "pstAeExposureAttrInfo NULL.");
		return CVI_FAILURE;
	}

	pstExpInfo->u32ISOCalibrate = pstExpInfo->u32ISO * 256 /
		AAA_DIV_0_TO_1(pstAeExposureAttrInfo[sID]->stAuto.u16ISOCalCoef);

	res = AE_SetRouteInfo(sID, AE_LE, &pstExpInfo->stAERoute);
	if (res == CVI_FAILURE)
		return res;

	res = AE_SetRouteExInfo(sID, AE_LE, &pstExpInfo->stAERouteEx);

	if (AeInfo[sID].bWDRMode) {
		if (res == CVI_FAILURE)
			return res;
		res = AE_SetRouteInfo(sID, AE_SE, &pstExpInfo->stAERouteSF);
		if (res == CVI_FAILURE)
			return res;
		res = AE_SetRouteExInfo(sID, AE_SE, &pstExpInfo->stAERouteSFEx);
	} else {
		memset(&pstExpInfo->stAERouteSF, 0, sizeof(ISP_AE_ROUTE_S));
		memset(&pstExpInfo->stAERouteSFEx, 0, sizeof(ISP_AE_ROUTE_EX_S));
	}

	return res;
}



CVI_U16 AE_GetCenterG(CVI_U8 sID, CVI_U8 WDR_SE)
{
	sID = AE_CheckSensorID(sID);
	return (WDR_SE) ? AeInfo[sID].u16CenterG[AE_SE] : AeInfo[sID].u16CenterG[AE_LE];
}

CVI_BOOL AE_IsWDRMode(CVI_U8 sID)
{
	sID = AE_CheckSensorID(sID);
	return AeInfo[sID].bWDRMode;
}


CVI_U8 AE_CheckWDRIdx(CVI_U8 WDRIdx)
{
	return (WDRIdx < AE_MAX_WDR_FRAME_NUM) ? WDRIdx : AE_MAX_WDR_FRAME_NUM - 1;
}

CVI_S32 AE_SetWDRExposureAttr(CVI_U8 sID, const ISP_WDR_EXPOSURE_ATTR_S *pstWDRExpAttr)
{
	sID = AE_CheckSensorID(sID);

	if (!pstAeMpiWDRExposureAttr[sID]) {
		ISP_LOG_ERR("%s\n", "pstAeMpiWDRExposureAttr is NULL.");
		return CVI_FAILURE;
	}
	*pstAeMpiWDRExposureAttr[sID] = *pstWDRExpAttr;
	AE_SetParameterUpdate(sID, AE_WDR_EXPOSURE_ATTR_UPDATE);
	return CVI_SUCCESS;

}

CVI_S32 AE_GetWDRExposureAttr(CVI_U8 sID, ISP_WDR_EXPOSURE_ATTR_S *pstWDRExpAttr)
{
	sID = AE_CheckSensorID(sID);

	if (!pstAeMpiWDRExposureAttr[sID]) {
		ISP_LOG_ERR("%s\n", "pstAeMpiWDRExposureAttr is NULL.");
		return CVI_FAILURE;
	}
	*pstWDRExpAttr = *pstAeMpiWDRExposureAttr[sID];
	return CVI_SUCCESS;
}

CVI_S32 AE_SetRouteAttr(CVI_U8 sID, const ISP_AE_ROUTE_S *pstRouteAttr)
{
	sID = AE_CheckSensorID(sID);

	if (!pstAeMpiRoute[sID][AE_LE]) {
		ISP_LOG_ERR("%s\n", "pstAeMpiRoute is NULL.");
		return CVI_FAILURE;
	}

	if (pstRouteAttr->u32TotalNum < ISP_AE_ROUTE_MIN_NODES
		|| pstRouteAttr->u32TotalNum > ISP_AE_ROUTE_MAX_NODES) {
		ISP_LOG_ERR("%s\n", "out of range.");
		return CVI_FAILURE;
	}

	*pstAeMpiRoute[sID][AE_LE] = *pstRouteAttr;
	AE_SetParameterUpdate(sID, AE_ROUTE_UPDATE);
	return CVI_SUCCESS;
}

CVI_S32 AE_GetRouteAttr(CVI_U8 sID, ISP_AE_ROUTE_S *pstRouteAttr)
{
	sID = AE_CheckSensorID(sID);

	if (!pstAeMpiRoute[sID][AE_LE]) {
		ISP_LOG_ERR("%s\n", "pstAeMpiRoute is NULL.");
		return CVI_FAILURE;
	}
	*pstRouteAttr = *pstAeMpiRoute[sID][AE_LE];
	return CVI_SUCCESS;
}

CVI_S32 AE_SetRouteAttrEx(CVI_U8 sID, const ISP_AE_ROUTE_EX_S *pstRouteAttrEx)
{
	sID = AE_CheckSensorID(sID);
	if (!pstAeMpiRouteEx[sID][AE_LE]) {
		ISP_LOG_ERR("%s\n", "pstAeMpiRouteEx is NULL.");
		return CVI_FAILURE;
	}

	if (pstRouteAttrEx->u32TotalNum < ISP_AE_ROUTE_EX_MIN_NODES
		|| pstRouteAttrEx->u32TotalNum > ISP_AE_ROUTE_EX_MAX_NODES) {
		ISP_LOG_ERR("%s\n", "out of range.");
		return CVI_FAILURE;
	}

	*pstAeMpiRouteEx[sID][AE_LE] = *pstRouteAttrEx;
	AE_SetParameterUpdate(sID, AE_ROUTE_EX_UPDATE);
	return CVI_SUCCESS;
}

CVI_S32 AE_GetRouteAttrEx(CVI_U8 sID, ISP_AE_ROUTE_EX_S *pstRouteAttrEx)
{
	sID = AE_CheckSensorID(sID);
	if (!pstAeMpiRouteEx[sID][AE_LE]) {
		ISP_LOG_ERR("%s\n", "pstAeMpiRouteEx is NULL.");
		return CVI_FAILURE;
	}
	*pstRouteAttrEx = *pstAeMpiRouteEx[sID][AE_LE];
	return CVI_SUCCESS;
}

CVI_S32 AE_SetRouteSFAttr(CVI_U8 sID, const ISP_AE_ROUTE_S *pstRouteSFAttr)
{
	sID = AE_CheckSensorID(sID);

	if (!AeInfo[sID].bWDRMode) {
		return CVI_SUCCESS;
	}

	if (!pstAeMpiRoute[sID][AE_SE]) {
		ISP_LOG_ERR("%s\n", "pstAeMpiRoute SF is NULL.");
		return CVI_FAILURE;
	}

	if (pstRouteSFAttr->u32TotalNum < ISP_AE_ROUTE_MIN_NODES
		|| pstRouteSFAttr->u32TotalNum > ISP_AE_ROUTE_MAX_NODES) {
		ISP_LOG_ERR("%s\n", "out of range.");
		return CVI_FAILURE;
	}

	*pstAeMpiRoute[sID][AE_SE] = *pstRouteSFAttr;
	AE_SetParameterUpdate(sID, AE_ROUTE_SF_UPDATE);
	return CVI_SUCCESS;
}

CVI_S32 AE_GetRouteSFAttr(CVI_U8 sID, ISP_AE_ROUTE_S *pstRouteSFAttr)
{
	sID = AE_CheckSensorID(sID);

	if (!AeInfo[sID].bWDRMode) {
		memset(pstRouteSFAttr, 0, sizeof(ISP_AE_ROUTE_S));
		return CVI_SUCCESS;
	}

	if (!pstAeMpiRoute[sID][AE_SE]) {
		ISP_LOG_ERR("%s\n", "pstAeMpiRoute SF is NULL.");
		return CVI_FAILURE;
	}
	*pstRouteSFAttr = *pstAeMpiRoute[sID][AE_SE];
	return CVI_SUCCESS;
}

CVI_S32 AE_SetRouteSFAttrEx(CVI_U8 sID, const ISP_AE_ROUTE_EX_S *pstRouteSFAttrEx)
{
	sID = AE_CheckSensorID(sID);

	if (!AeInfo[sID].bWDRMode) {
		return CVI_SUCCESS;
	}

	if (!pstAeMpiRouteEx[sID][AE_SE]) {
		ISP_LOG_ERR("%s\n", "pstRouteAttrEx SE is NULL.");
		return CVI_FAILURE;
	}

	if (pstRouteSFAttrEx->u32TotalNum < ISP_AE_ROUTE_EX_MIN_NODES
		|| pstRouteSFAttrEx->u32TotalNum > ISP_AE_ROUTE_EX_MAX_NODES) {
		ISP_LOG_ERR("%s\n", "out of range.");
		return CVI_FAILURE;
	}

	*pstAeMpiRouteEx[sID][AE_SE] = *pstRouteSFAttrEx;
	AE_SetParameterUpdate(sID, AE_ROUTE_SF_EX_UPDATE);
	return CVI_SUCCESS;
}

CVI_S32 AE_GetRouteSFAttrEx(CVI_U8 sID, ISP_AE_ROUTE_EX_S *pstRouteSFAttrEx)
{
	sID = AE_CheckSensorID(sID);

	if (!AeInfo[sID].bWDRMode) {
		memset(pstRouteSFAttrEx, 0, sizeof(ISP_AE_ROUTE_EX_S));
		return CVI_SUCCESS;
	}

	if (!pstAeMpiRouteEx[sID][AE_SE]) {
		ISP_LOG_ERR("%s\n", "pstRouteAttrEx SE is NULL.");
		return CVI_FAILURE;
	}
	*pstRouteSFAttrEx = *pstAeMpiRouteEx[sID][AE_SE];
	return CVI_SUCCESS;
}


CVI_S32 AE_SetStatisticsConfig(CVI_U8 sID, const ISP_AE_STATISTICS_CFG_S *pstAeStatCfg)
{
	sID = AE_CheckSensorID(sID);

	if (!pstAeStatisticsCfg[sID]) {
		ISP_LOG_ERR("%s\n", "pstAeStatisticsCfg is NULL.");
		return CVI_FAILURE;
	}

	*pstAeStatisticsCfg[sID] = *pstAeStatCfg;

	AE_SetParameterUpdate(sID, AE_STATISTICS_CONFIG_UPDATE);
	return CVI_SUCCESS;
}

CVI_S32 AE_GetStatisticsConfig(CVI_U8 sID, ISP_AE_STATISTICS_CFG_S *pstAeStatCfg)
{
	sID = AE_CheckSensorID(sID);
	if (!pstAeStatisticsCfg[sID]) {
		ISP_LOG_ERR("%s\n", "pstAeStatisticsCfg is NULL.");
		return CVI_FAILURE;
	}
	*pstAeStatCfg = *pstAeStatisticsCfg[sID];
	return CVI_SUCCESS;
}


void AE_SetWDRSETime(CVI_U8 sID, CVI_U32 time)
{
	sID = AE_CheckSensorID(sID);
	AeInfo[sID].u32ManualWDRSETime = time;
}

CVI_U32 AE_GetWDRSEMaxTime(CVI_U8 sID)
{
	return AeInfo[sID].u32WDRSEExpTimeMax;
}

CVI_U32 AE_GetWDRSEMinTime(CVI_U8 sID)
{
	return AeInfo[sID].u32ExpTimeMin;
}

void AE_GetWDRSETimeRange(CVI_U8 sID, CVI_U32 *minTime, CVI_U32 *maxTime)
{
	*minTime = AE_GetWDRSEMinTime(sID);
	*maxTime = AE_GetWDRSEMaxTime(sID);
}

void AE_SetWDRSEISPDGain(CVI_U8 sID, CVI_U32 ispDgain)
{
	sID = AE_CheckSensorID(sID);
	AeInfo[sID].u32ManualWDRSEISPDgain = ispDgain;
}


void AE_BracketingSetSimple(CVI_BOOL bEnable)
{
	bisSimpleAEB = bEnable;
}

void AE_BracketingStart(CVI_U8 sID)
{
	ISP_EXPOSURE_ATTR_S tmpExpAttr;

	AeInfo[sID].s16AEBracketing0EvBv[AE_LE] = AeInfo[sID].stApex[AE_LE].s16BVEntry;
	AeInfo[sID].s16AEBracketing0EvBv[AE_SE] = AeInfo[sID].stApex[AE_SE].s16BVEntry;
	AE_GetExposureAttr(sID, &tmpExpAttr);
	tmpExpAttr.enOpType = OP_TYPE_MANUAL;
	AE_SetExposureAttr(sID, &tmpExpAttr);
	u8SimpleAEB_Cnt = 0;
}

CVI_S32 AE_BracketingSetExposure(CVI_U8 sID, CVI_S16 leEv, CVI_S16 seEv)
{
	AE_APEX tmpExp[AE_MAX_WDR_FRAME_NUM], bracketingExp[AE_MAX_WDR_FRAME_NUM];
	CVI_S32 result;
	CVI_S16 minBvEntry[AE_MAX_WDR_FRAME_NUM], maxBvEntry[AE_MAX_WDR_FRAME_NUM];
	CVI_S16	tmpMinBvEntry, tmpMaxBvEntry;
	CVI_U8 i;
	CVI_U32 expRatio;
	AE_EXPOSURE bracketing[AE_MAX_WDR_FRAME_NUM];

	if (bisSimpleAEB) {
		AE_BracketingSetIndex(sID, u8SimpleAEB_Cnt);
		u8SimpleAEB_Cnt++;
		return 0;
	}

	tmpExp[AE_LE].s16BVEntry = AeInfo[sID].s16AEBracketing0EvBv[AE_LE] - leEv * ENTRY_PER_EV / 10;
	tmpExp[AE_SE].s16BVEntry = AeInfo[sID].s16AEBracketing0EvBv[AE_LE] - seEv * ENTRY_PER_EV / 10;

	tmpMinBvEntry = AeInfo[sID].s16MinSensorTVEntry - AeInfo[sID].s16MaxSensorISOEntry;
	tmpMaxBvEntry = AeInfo[sID].s16MaxSensorTVEntry - AeInfo[sID].s16MinSensorISOEntry;

	minBvEntry[AE_LE] = tmpMinBvEntry;
	maxBvEntry[AE_LE] = tmpMaxBvEntry - 2 * ENTRY_PER_EV;
	minBvEntry[AE_SE] = tmpMinBvEntry + 2 * ENTRY_PER_EV;
	maxBvEntry[AE_SE] = tmpMaxBvEntry;

	bracketingExp[AE_LE].s16BVEntry = tmpExp[AE_LE].s16BVEntry;
	bracketingExp[AE_SE].s16BVEntry = tmpExp[AE_SE].s16BVEntry;

	for (i = 0; i < AeInfo[sID].u8AeMaxFrameNum; ++i) {
		AAA_LIMIT(bracketingExp[i].s16BVEntry, minBvEntry[i], maxBvEntry[i]);
		AE_GetRouteTVAVSVEntry(sID, i, bracketingExp[i].s16BVEntry, &bracketingExp[i].s16TVEntry,
		&bracketingExp[i].s16AVEntry, &bracketingExp[i].s16SVEntry);
	}

	if (AeInfo[sID].bWDRMode)
		AE_GetWDRLESEEntry(sID, &bracketingExp[AE_LE], &bracketingExp[AE_SE]);

	for (i = 0; i < AeInfo[sID].u8AeMaxFrameNum; ++i) {
		AE_GetIdealExpTimeByEntry(sID, bracketingExp[i].s16TVEntry, &bracketing[i].fIdealExpTime);
		AE_GetExpGainByEntry(sID, bracketingExp[i].s16SVEntry, &bracketing[i].stExpGain);
		AE_GetExposureTimeIdealLine(sID, bracketing[i].fIdealExpTime, &bracketing[i].fIdealExpTimeLine);
		bracketing[i].u32ExpTime = (CVI_U32)bracketing[i].fIdealExpTime;
		bracketing[i].u32ExpTimeLine = (CVI_U32)bracketing[i].fIdealExpTimeLine;
	}

	expRatio = bracketing[AE_LE].u32ExpTimeLine * AE_WDR_RATIO_BASE /
		AAA_DIV_0_TO_1(bracketing[AE_SE].u32ExpTimeLine);

	AeInfo[sID].stExp[AE_LE] = bracketing[AE_LE];
	AeInfo[sID].stExp[AE_SE] = bracketing[AE_SE];
	AE_GetWDRExpLineRange(sID, expRatio, &AeInfo[sID].stExp[AE_LE].u32ExpTimeLine,
		&AeInfo[sID].stExp[AE_SE].u32ExpTimeLine, 0);

	if (bracketingExp[AE_LE].s16BVEntry != tmpExp[AE_LE].s16BVEntry ||
		bracketing[AE_LE].u32ExpTimeLine >  AeInfo[sID].stExp[AE_LE].u32ExpTimeLine)
		result = 1;
	else if (bracketingExp[AE_SE].s16BVEntry != tmpExp[AE_SE].s16BVEntry ||
		bracketing[AE_SE].u32ExpTimeLine >  AeInfo[sID].stExp[AE_SE].u32ExpTimeLine)
		result = 2;
	else
		result = 0;

	AeInfo[sID].u32ManualWDRSETime = AeInfo[sID].stExp[AE_SE].u32ExpTime;

	AE_SetExposure(sID, AeInfo[sID].stExp);

	if (AE_GetDebugMode(sID) == 61) {
		printf(">>>>leEv:%d seEv:%d\n", leEv, seEv);
		printf("LE:%d %d %d SE:%d %d %d\n",
			bracketingExp[AE_LE].s16BVEntry, bracketingExp[AE_LE].s16TVEntry,
			bracketingExp[AE_LE].s16SVEntry, bracketingExp[AE_SE].s16BVEntry,
			bracketingExp[AE_SE].s16TVEntry, bracketingExp[AE_SE].s16SVEntry);
		printf("exp LE T:%d L:%d-%d SE T:%d L:%d-%d AG:%d DG:%d R:%d\n", bracketing[AE_LE].u32ExpTime,
			bracketing[AE_LE].u32ExpTimeLine, AeInfo[sID].stExp[AE_LE].u32SensorExpTimeLine,
			bracketing[AE_SE].u32ExpTime,
			bracketing[AE_SE].u32ExpTimeLine, AeInfo[sID].stExp[AE_SE].u32SensorExpTimeLine,
			AeInfo[sID].stExp[AE_LE].stExpGain.u32AGain, AeInfo[sID].stExp[AE_LE].stExpGain.u32DGain,
			result);

		for (i = 0; i < AeInfo[sID].u8AeMaxFrameNum; ++i) {
			printf("%d %u FL:%d\n", i, AeInfo[sID].u32frmCnt, AeInfo[sID].u16FrameAvgLuma[i]);
			printf("Set expL:%u %u Sensor ExpL:%u\n",
			bracketing[i].u32ExpTimeLine, AeInfo[sID].stExp[i].u32ExpTimeLine,
			AeInfo[sID].stExp[i].u32SensorExpTimeLine);
			printf("Set AG:%u DG:%u IG:%u Sensor AG:%u DG:%u IG:%u\n",
			AeInfo[sID].stExp[i].stExpGain.u32AGain,  AeInfo[sID].stExp[i].stExpGain.u32DGain,
			AeInfo[sID].stExp[i].stExpGain.u32ISPDGain,
			AeInfo[sID].stExp[i].stSensorExpGain.u32AGain,
			AeInfo[sID].stExp[i].stSensorExpGain.u32DGain,
			AeInfo[sID].stExp[i].stSensorExpGain.u32ISPDGain);
		}
	}

	return result;
}




CVI_S32 AE_BracketingSetIndex(CVI_U8 sID, CVI_S16 index)
{
	AE_APEX tmpExp[AE_MAX_WDR_FRAME_NUM], bracketingExp[AE_MAX_WDR_FRAME_NUM];
	CVI_S32 result;
	CVI_S16 minBvEntry[AE_MAX_WDR_FRAME_NUM], maxBvEntry[AE_MAX_WDR_FRAME_NUM];
	CVI_S16	tmpMinBvEntry, tmpMaxBvEntry;
	CVI_U8 i;
	CVI_U32 expRatio;
	AE_EXPOSURE bracketing[AE_MAX_WDR_FRAME_NUM];

#ifndef AAA_PC_PLATFORM

	AAA_LIMIT(index, 0, AE_TEST_RAW_NUM - 1);

	tmpExp[AE_LE].s16BVEntry = aeb_ev[index].tv - aeb_ev[index].sv;
	tmpExp[AE_SE].s16BVEntry = tmpExp[AE_LE].s16BVEntry + 6 * ENTRY_PER_BV;

	//512 1745
	//1523 0
	tmpMinBvEntry = AeInfo[sID].s16MinSensorTVEntry - AeInfo[sID].s16MaxSensorISOEntry;
	tmpMaxBvEntry = AeInfo[sID].s16MaxSensorTVEntry - AeInfo[sID].s16MinSensorISOEntry;
	minBvEntry[AE_LE] = tmpMinBvEntry;
	maxBvEntry[AE_LE] = tmpMaxBvEntry - 2 * ENTRY_PER_EV;
	minBvEntry[AE_SE] = tmpMinBvEntry + 2 * ENTRY_PER_EV;
	maxBvEntry[AE_SE] = tmpMaxBvEntry;

	bracketingExp[AE_LE].s16BVEntry = tmpExp[AE_LE].s16BVEntry;
	bracketingExp[AE_SE].s16BVEntry = tmpExp[AE_SE].s16BVEntry;

	AAA_LIMIT(bracketingExp[AE_LE].s16BVEntry, minBvEntry[AE_LE], maxBvEntry[AE_SE]);
	AAA_LIMIT(bracketingExp[AE_SE].s16BVEntry, minBvEntry[AE_SE], maxBvEntry[AE_SE]);
	AE_GetRouteTVAVSVEntry(sID, AE_LE, bracketingExp[AE_LE].s16BVEntry, &bracketingExp[AE_LE].s16TVEntry,
		&bracketingExp[AE_LE].s16AVEntry, &bracketingExp[AE_LE].s16SVEntry);

	AE_GetRouteTVAVSVEntry(sID, AE_SE, bracketingExp[AE_SE].s16BVEntry, &bracketingExp[AE_SE].s16TVEntry,
		&bracketingExp[AE_SE].s16AVEntry, &bracketingExp[AE_SE].s16SVEntry);
	AE_GetWDRLESEEntry(sID, &bracketingExp[AE_LE], &bracketingExp[AE_SE]);


	for (i = 0; i < AeInfo[sID].u8AeMaxFrameNum; ++i) {
		AE_GetIdealExpTimeByEntry(sID, bracketingExp[i].s16TVEntry, &bracketing[i].fIdealExpTime);
		AE_GetExpGainByEntry(sID, bracketingExp[i].s16SVEntry, &bracketing[i].stExpGain);
		AE_GetExposureTimeIdealLine(sID, bracketing[i].fIdealExpTime, &bracketing[i].fIdealExpTimeLine);
		bracketing[i].u32ExpTime = (CVI_U32)bracketing[i].fIdealExpTime;
		bracketing[i].u32ExpTimeLine = (CVI_U32)bracketing[i].fIdealExpTimeLine;
	}

	expRatio = bracketing[AE_LE].u32ExpTimeLine * AE_WDR_RATIO_BASE /
		AAA_DIV_0_TO_1(bracketing[AE_SE].u32ExpTimeLine);

	AeInfo[sID].stExp[AE_LE] = bracketing[AE_LE];
	AeInfo[sID].stExp[AE_SE] = bracketing[AE_SE];
	AE_GetWDRExpLineRange(sID, expRatio, &AeInfo[sID].stExp[AE_LE].u32ExpTimeLine,
		&AeInfo[sID].stExp[AE_SE].u32ExpTimeLine, 0);

	if (bracketingExp[AE_LE].s16BVEntry != tmpExp[AE_LE].s16BVEntry ||
		bracketing[AE_LE].u32ExpTimeLine >  AeInfo[sID].stExp[AE_LE].u32ExpTimeLine)
		result = 1;
	else if (bracketingExp[AE_SE].s16BVEntry != tmpExp[AE_SE].s16BVEntry ||
		bracketing[AE_SE].u32ExpTimeLine >  AeInfo[sID].stExp[AE_SE].u32ExpTimeLine)
		result = 2;
	else
		result = 0;

	AeInfo[sID].u32ManualWDRSETime = AeInfo[sID].stExp[AE_SE].u32ExpTime;

	AE_SetExposure(sID, AeInfo[sID].stExp);

	if (AE_GetDebugMode(sID) == 61) {
		printf(">>>>ind:%d\n", index);
		printf("LE:%d %d %d SE:%d %d %d\n",
			bracketingExp[AE_LE].s16BVEntry, bracketingExp[AE_LE].s16TVEntry,
			bracketingExp[AE_LE].s16SVEntry, bracketingExp[AE_SE].s16BVEntry,
			bracketingExp[AE_SE].s16TVEntry, bracketingExp[AE_SE].s16SVEntry);
		printf("exp LE T:%d L:%d-%d SE T:%d L:%d-%d AG:%d DG:%d R:%d\n", bracketing[AE_LE].u32ExpTime,
			bracketing[AE_LE].u32ExpTimeLine, AeInfo[sID].stExp[AE_LE].u32SensorExpTimeLine,
			bracketing[AE_SE].u32ExpTime,
			bracketing[AE_SE].u32ExpTimeLine, AeInfo[sID].stExp[AE_SE].u32SensorExpTimeLine,
			AeInfo[sID].stExp[AE_LE].stExpGain.u32AGain, AeInfo[sID].stExp[AE_LE].stExpGain.u32DGain,
			result);

		for (i = 0; i < AeInfo[sID].u8AeMaxFrameNum; ++i) {
			printf("%d %u\n", i, AeInfo[sID].u32frmCnt);
			printf("Set expL:%u %u Sensor ExpL:%u\n",
			bracketing[i].u32ExpTimeLine, AeInfo[sID].stExp[i].u32ExpTimeLine,
			AeInfo[sID].stExp[i].u32SensorExpTimeLine);
			printf("Set AG:%u DG:%u IG:%u Sensor AG:%u DG:%u IG:%u\n",
			AeInfo[sID].stExp[i].stExpGain.u32AGain,  AeInfo[sID].stExp[i].stExpGain.u32DGain,
			AeInfo[sID].stExp[i].stExpGain.u32ISPDGain,
			AeInfo[sID].stExp[i].stSensorExpGain.u32AGain,
			AeInfo[sID].stExp[i].stSensorExpGain.u32DGain,
			AeInfo[sID].stExp[i].stSensorExpGain.u32ISPDGain);
		}
	}
#endif

	return result;
}

void AE_BracketingSetExposureFinish(CVI_U8 sID)
{
	ISP_EXPOSURE_ATTR_S tmpExpAttr;

	AE_BracketingSetExposure(sID, 0, 0);
	AeInfo[sID].s16AEBracketing0EvBv[AE_LE] = BV_AUTO_ENTRY;
	AeInfo[sID].u32ManualWDRSETime = 0;
	AE_Delay1ms(500);
	AE_GetExposureAttr(sID, &tmpExpAttr);
	tmpExpAttr.enOpType = OP_TYPE_AUTO;
	AE_SetExposureAttr(sID, &tmpExpAttr);
}


void AE_InitPara(VI_PIPE ViPipe, CVI_BOOL isInit)
{
	CVI_U8 j;
	CVI_S16 minISOEntry, maxISOEntry, ISOMaxEntryByOB;
	CVI_U16 maxOBValue, OB1, OB2;
	ISP_BLACK_LEVEL_ATTR_S *pAttr;
	CVI_S32 s32Ret = 0;
	CVI_U8 sID;
	static CVI_U8	preWdrMode[AE_SENSOR_NUM] = {255, 255};

	sID = AE_CheckSensorID(ViPipe);
	AeInfo[sID].u8AeMaxFrameNum = (AeInfo[sID].bWDRMode) ? AE_MAX_WDR_FRAME_NUM : 1;

	if (!isInit) {
		AeInfo[sID].bEnableSmoothAE = ENABLE_SMOOTH_AE;
		AeInfo[sID].bISOLimitByBLC = ISO_LIMIT_BY_BLC;
		AeInfo[sID].bISPDGainFirst = ISPDGAIN_FIRST;
		AeInfo[sID].u32ISPDGainMin = MIN_ISPDGAIN;
		AeInfo[sID].u32ISPDGainMax = MAX_ISPDGAIN;
		AeInfo[sID].bEnableISPDgainCompensation = ENABLE_ISPDGAIN_COMPENSATION;
		AeInfo[sID].bMeterEveryFrame = ENABLE_METER_EVERY_FRAME;
		AeInfo[sID].u32TotalGainMin =	AE_CalTotalGain(AeInfo[sID].u32AGainMin,
			AeInfo[sID].u32DGainMin, AeInfo[sID].u32ISPDGainMin);
		AeInfo[sID].u32TotalGainMax = AE_CalTotalGain(AeInfo[sID].u32AGainMax,
			AeInfo[sID].u32DGainMax, AeInfo[sID].u32ISPDGainMax);

		AE_GetISONumByGain(AeInfo[sID].u32TotalGainMin, &AeInfo[sID].u32ISONumMin);
		AE_GetISONumByGain(AeInfo[sID].u32TotalGainMax, &AeInfo[sID].u32ISONumMax);

		AE_GetISOEntryByGain(AeInfo[sID].u32TotalGainMin, &minISOEntry);
		AE_GetISOEntryByGain(AeInfo[sID].u32TotalGainMax, &maxISOEntry);
		AeInfo[sID].s16MinSensorISOEntry = minISOEntry;
		AeInfo[sID].s16MaxSensorISOEntry = maxISOEntry;
		AeInfo[sID].s16MinISOEntry = minISOEntry;
		AeInfo[sID].s16MaxISOEntry = maxISOEntry;
		AeInfo[sID].s16MinTVEntry = AeInfo[sID].s16MinSensorTVEntry;
		AeInfo[sID].s16MaxTVEntry = AeInfo[sID].s16MaxSensorTVEntry;

		AE_GetISOEntryByGain(AeInfo[sID].u32AGainMax, &AeInfo[sID].s16AGainMaxEntry);
		AE_GetISOEntryByGain(AeInfo[sID].u32DGainMax, &AeInfo[sID].s16DGainMaxEntry);
		AE_GetISOEntryByGain(AeInfo[sID].u32ISPDGainMax, &AeInfo[sID].s16ISPDGainMaxEntry);

		AeInfo[sID].u32ManualWDRSETime = 0;

		if (u8SensorNum < sID + 1)
			u8SensorNum = sID + 1;

		ISOMaxEntryByOB = maxISOEntry;

		if (AeInfo[sID].bISOLimitByBLC) {
			s32Ret = isp_sensor_updateBlc(ViPipe, &pAttr);
			if (s32Ret != CVI_SUCCESS)
				ISP_LOG_WARNING("%s\n", "AE Sensor get blc fail. Use API blc value.");
			else {
				for (j = 0; j < ISP_AUTO_ISO_STRENGTH_NUM; ++j) {
					OB1 = AAA_MAX(pAttr->stAuto.OffsetR[j], pAttr->stAuto.OffsetB[j]);
					OB2 = AAA_MAX(pAttr->stAuto.OffsetGr[j], pAttr->stAuto.OffsetGb[j]);
					maxOBValue = AAA_MAX(OB1, OB2);
					//printf("%d mOB:%d\n",j,maxOBValue);
					if (maxOBValue >= 800 && j > 1) {
						ISOMaxEntryByOB = (j-1) * ENTRY_PER_EV;
						ISP_LOG_NOTICE("MaxOBE:%d\n", ISOMaxEntryByOB);
						break;
					}
				}
			}
		}

		AeInfo[sID].s16BLCMaxISOEntry = ISOMaxEntryByOB;

		if (AeInfo[sID].bISOLimitByBLC)
			AeInfo[sID].s16BLCMaxISOEntry = AAA_MIN(maxISOEntry, ISOMaxEntryByOB);

		AE_GetISONumByEntry(sID, AeInfo[sID].s16BLCMaxISOEntry, &AeInfo[sID].u32BLCISONumMax);
		AE_GetGainBySvEntry(sID, AE_GAIN_TOTAL, AeInfo[sID].s16BLCMaxISOEntry, &AeInfo[sID].u32BLCSysGainMax);
		AE_GetFastBootExposureEntry(sID, &AeInfo[sID].stInitApex[AE_LE].s16TVEntry,
			&AeInfo[sID].stInitApex[AE_LE].s16SVEntry);

		for (j = 0; j < AeInfo[sID].u8AeMaxFrameNum; ++j) {
			if (AeInfo[sID].bWDRMode || !AeInfo[sID].stInitApex[j].s16TVEntry) {
				AeInfo[sID].stInitApex[j].s16TVEntry =
					(j == 0) ? BOOT_TV_ENTRY : BOOT_TV_ENTRY + 2 * ENTRY_PER_EV;
				AeInfo[sID].stInitApex[j].s16SVEntry = BOOT_ISO_ENTRY;
			}

			AeInfo[sID].stInitApex[j].s16AVEntry = 0;
			AeInfo[sID].stInitApex[j].s16BVEntry = AeInfo[sID].stInitApex[j].s16TVEntry -
					AeInfo[sID].stInitApex[j].s16SVEntry;

			AeInfo[sID].stApex[j].s16TVEntry = AeInfo[sID].stInitApex[j].s16TVEntry;
			AeInfo[sID].stApex[j].s16AVEntry = AeInfo[sID].stInitApex[j].s16AVEntry;
			AeInfo[sID].stApex[j].s16SVEntry = AeInfo[sID].stInitApex[j].s16SVEntry;
			AeInfo[sID].stApex[j].s16BVEntry = AeInfo[sID].stInitApex[j].s16BVEntry;

			AeInfo[sID].stAssignApex[j].s16BVEntry = BV_AUTO_ENTRY;
			AeInfo[sID].stAssignApex[j].s16TVEntry = TV_AUTO_ENTRY;
			AeInfo[sID].stAssignApex[j].s16AVEntry = AV_AUTO_ENTRY;
			AeInfo[sID].stAssignApex[j].s16SVEntry = SV_AUTO_ENTRY;

			AE_GetExpTimeByEntry(sID, AeInfo[sID].stApex[j].s16TVEntry,
										&AeInfo[sID].stExp[j].u32ExpTime);
			AE_GetExpGainByEntry(sID, AeInfo[sID].stApex[j].s16SVEntry,
										&AeInfo[sID].stExp[j].stExpGain);
			AeInfo[sID].stExp[j].fIdealExpTime = AeInfo[sID].stExp[j].u32ExpTime;
		}

		AeInfo[sID].s16LvX100[AE_LE] = 700 + (AeInfo[sID].stApex[AE_LE].s16BVEntry - EVTT_ENTRY_1_30SEC) *
			100 / ENTRY_PER_EV;
	}

	if (isInit && AeInfo[sID].bWDRMode != preWdrMode[sID]) {
		if (AeInfo[sID].bWDRMode) {
			if (AeInfo[sID].stExp[AE_LE].u32ExpTime > AeInfo[sID].u32ExpTimeMax) {
				AeInfo[sID].stExp[AE_LE].stExpGain.u32ISPDGain *=
					(CVI_FLOAT)AeInfo[sID].stExp[AE_LE].u32ExpTime /
					AAA_DIV_0_TO_1(AeInfo[sID].u32ExpTimeMax);
				AeInfo[sID].stExp[AE_LE].u32ExpTime = AeInfo[sID].u32ExpTimeMax;
			}
			AeInfo[sID].stExp[AE_SE].u32ExpTime = AeInfo[sID].stExp[AE_LE].u32ExpTime / 4;
			if (AeInfo[sID].stExp[AE_SE].u32ExpTime > AeInfo[sID].u32WDRSEExpTimeMax)
				AeInfo[sID].stExp[AE_SE].u32ExpTime = AeInfo[sID].u32WDRSEExpTimeMax;
			AeInfo[sID].stExp[AE_SE].stExpGain =  AeInfo[sID].stExp[AE_LE].stExpGain;
		}
	}

	for (j = 0; j < AeInfo[sID].u8AeMaxFrameNum; ++j) {
		AeInfo[sID].stSmoothApex[j][0].s16BVEntry = BV_AUTO_ENTRY;
		AeInfo[sID].stSmoothApex[j][0].s16TVEntry = TV_AUTO_ENTRY;
		AeInfo[sID].stSmoothApex[j][0].s16AVEntry = AV_AUTO_ENTRY;
		AeInfo[sID].stSmoothApex[j][0].s16SVEntry = SV_AUTO_ENTRY;
		AeInfo[sID].bIsStable[j] = 0;
	}
	preWdrMode[sID] = AeInfo[sID].bWDRMode;

	AeInfo[sID].pu32Histogram = CVI_NULL;
	AeInfo[sID].u16RawNum = AeInfo[sID].u16RawBufCnt = 0;
	AeInfo[sID].u8ConvergeMode[AE_LE] = CONVERGE_FAST;
}

void AE_ExposureAttr_Init(CVI_U8 sID)
{
	sID = AE_CheckSensorID(sID);
	if (!pstAeMpiExposureAttr[sID]) {
		ISP_LOG_ERR("%s\n", "pstAeMpiExposureAttr is NULL.");
		return;
	}
	//Exposure
	pstAeMpiExposureAttr[sID]->bByPass = 0;
	pstAeMpiExposureAttr[sID]->enOpType = OP_TYPE_AUTO;
	pstAeMpiExposureAttr[sID]->u8AERunInterval = 1;
	pstAeMpiExposureAttr[sID]->bHistStatAdjust = 0;
	pstAeMpiExposureAttr[sID]->bAERouteExValid = 0;
	pstAeMpiExposureAttr[sID]->u8DebugMode = 0;

	pstAeMpiExposureAttr[sID]->stManual.enExpTimeOpType = OP_TYPE_AUTO;
	pstAeMpiExposureAttr[sID]->stManual.enAGainOpType = OP_TYPE_AUTO;
	pstAeMpiExposureAttr[sID]->stManual.enDGainOpType = OP_TYPE_AUTO;
	pstAeMpiExposureAttr[sID]->stManual.enISPDGainOpType = OP_TYPE_AUTO;
	pstAeMpiExposureAttr[sID]->stManual.enISONumOpType = OP_TYPE_AUTO;
	pstAeMpiExposureAttr[sID]->stManual.u32ExpTime = 16384;
	pstAeMpiExposureAttr[sID]->stManual.u32AGain = AE_GAIN_BASE;
	pstAeMpiExposureAttr[sID]->stManual.u32DGain = AE_GAIN_BASE;
	pstAeMpiExposureAttr[sID]->stManual.u32ISPDGain = AE_GAIN_BASE;

	pstAeMpiExposureAttr[sID]->stManual.enGainType = AE_TYPE_GAIN;
	pstAeMpiExposureAttr[sID]->stManual.u32ISONum = AeInfo[sID].u32ISONumMin;

	pstAeMpiExposureAttr[sID]->stAuto.stExpTimeRange.u32Min = AeInfo[sID].u32ExpTimeMin;
	pstAeMpiExposureAttr[sID]->stAuto.stExpTimeRange.u32Max = 100000;//AeInfo[sID].u32ExpTimeMax;

	pstAeMpiExposureAttr[sID]->stAuto.stAGainRange.u32Min = AeInfo[sID].u32AGainMin;
	pstAeMpiExposureAttr[sID]->stAuto.stAGainRange.u32Max = AeInfo[sID].u32AGainMax;

	pstAeMpiExposureAttr[sID]->stAuto.stDGainRange.u32Min = AeInfo[sID].u32DGainMin;
	pstAeMpiExposureAttr[sID]->stAuto.stDGainRange.u32Max = AeInfo[sID].u32DGainMax;

	pstAeMpiExposureAttr[sID]->stAuto.stISPDGainRange.u32Min = AeInfo[sID].u32ISPDGainMin;
	pstAeMpiExposureAttr[sID]->stAuto.stISPDGainRange.u32Max = AeInfo[sID].u32ISPDGainMax;

	pstAeMpiExposureAttr[sID]->stAuto.stSysGainRange.u32Min = AeInfo[sID].u32TotalGainMin;
	pstAeMpiExposureAttr[sID]->stAuto.stSysGainRange.u32Max = AeInfo[sID].u32BLCSysGainMax;

	pstAeMpiExposureAttr[sID]->stAuto.enGainType = AE_TYPE_GAIN;
	pstAeMpiExposureAttr[sID]->stAuto.stISONumRange.u32Min = AeInfo[sID].u32ISONumMin;
	pstAeMpiExposureAttr[sID]->stAuto.stISONumRange.u32Max = AeInfo[sID].u32BLCISONumMax;

	pstAeMpiExposureAttr[sID]->stAuto.u8Speed = 64;
	pstAeMpiExposureAttr[sID]->stAuto.u16BlackSpeedBias = 144;

	pstAeMpiExposureAttr[sID]->stAuto.u8Tolerance = 2;
	pstAeMpiExposureAttr[sID]->stAuto.u32GainThreshold = AeInfo[sID].u32TotalGainMax;
	pstAeMpiExposureAttr[sID]->stAuto.u8Compensation = 56;
	pstAeMpiExposureAttr[sID]->stAuto.u16EVBias = AE_EVBIAS_BASE;
	pstAeMpiExposureAttr[sID]->stAuto.enAEStrategyMode = AE_EXP_HIGHLIGHT_PRIOR;
	pstAeMpiExposureAttr[sID]->stAuto.enAEMode = AE_MODE_FIX_FRAME_RATE;

	pstAeMpiExposureAttr[sID]->stAuto.stAntiflicker.bEnable = 0;
	pstAeMpiExposureAttr[sID]->stAuto.stAntiflicker.enMode = ISP_ANTIFLICKER_AUTO_MODE;
	pstAeMpiExposureAttr[sID]->stAuto.stAntiflicker.enFrequency = AE_FREQUENCE_50HZ;
	pstAeMpiExposureAttr[sID]->stAuto.stSubflicker.bEnable = 0;
	pstAeMpiExposureAttr[sID]->stAuto.stSubflicker.u8LumaDiff = 50;

	pstAeMpiExposureAttr[sID]->stAuto.bManualExpValue = 0;
	pstAeMpiExposureAttr[sID]->stAuto.u32ExpValue = 0;

	pstAeMpiExposureAttr[sID]->stAuto.enFSWDRMode = ISP_FSWDR_NORMAL_MODE;
	pstAeMpiExposureAttr[sID]->stAuto.bWDRQuick = 0;
	pstAeMpiExposureAttr[sID]->stAuto.u16ISOCalCoef = 256;

	pstAeMpiExposureAttr[sID]->stAuto.bEnableFaceAE = 0;
	pstAeMpiExposureAttr[sID]->stAuto.u8FaceTargetLuma = 46;
	pstAeMpiExposureAttr[sID]->stAuto.u8FaceWeight = 80;

	pstAeMpiExposureAttr[sID]->stAuto.u8HighLightLumaThr = 224;
	pstAeMpiExposureAttr[sID]->stAuto.u8HighLightBufLumaThr = 176;
	pstAeMpiExposureAttr[sID]->stAuto.u8LowLightLumaThr = 16;
	pstAeMpiExposureAttr[sID]->stAuto.u8LowLightBufLumaThr = 48;

	pstAeMpiExposureAttr[sID]->bAEGainSepCfg = 1;
	pstAeMpiExposureAttr[sID]->stAuto.bHistogramAssist = 0;
	pstAeMpiExposureAttr[sID]->stAuto.u16AEStrategyThr = 2;
	pstAeMpiExposureAttr[sID]->s8SensorSensitivity = 0;
}

void AE_WDRAttr_Init(CVI_U8 sID)
{
	sID = AE_CheckSensorID(sID);
	if (!pstAeMpiWDRExposureAttr[sID]) {
		ISP_LOG_ERR("%s\n", "pstAeMpiWDRExposureAttr is NULL.");
		return;
	}
	pstAeMpiWDRExposureAttr[sID]->enExpRatioType = OP_TYPE_AUTO;
	memset(pstAeMpiWDRExposureAttr[sID]->au32ExpRatio, MIN_WDR_RATIO,
		WDR_EXP_RATIO_NUM * sizeof(CVI_U32));
	pstAeMpiWDRExposureAttr[sID]->u32ExpRatioMax = MAX_WDR_RATIO;
	pstAeMpiWDRExposureAttr[sID]->u32ExpRatioMin = MIN_WDR_RATIO;
	pstAeMpiWDRExposureAttr[sID]->u16Tolerance = 6;
	pstAeMpiWDRExposureAttr[sID]->u16Speed = 32;
	pstAeMpiWDRExposureAttr[sID]->u16RatioBias = AE_GAIN_BASE;
	pstAeMpiWDRExposureAttr[sID]->u8SECompensation = 56;

	memset(pstAeMpiWDRExposureAttr[sID]->au8LEAdjustTargetMin, 40, LV_TOTAL_NUM * sizeof(CVI_U8));
	memset(pstAeMpiWDRExposureAttr[sID]->au8LEAdjustTargetMax, 60, LV_TOTAL_NUM * sizeof(CVI_U8));
	memset(pstAeMpiWDRExposureAttr[sID]->au8SEAdjustTargetMin, 20, LV_TOTAL_NUM * sizeof(CVI_U8));
	memset(pstAeMpiWDRExposureAttr[sID]->au8SEAdjustTargetMax, 60, LV_TOTAL_NUM * sizeof(CVI_U8));
	pstAeMpiWDRExposureAttr[sID]->enExpMainChn = ISP_CHANNEL_LE;

}

void AE_Route_Init(CVI_U8 sID)
{
	CVI_U8 i;
	CVI_U32 maxTime;

	sID = AE_CheckSensorID(sID);

	for (i = 0 ; i < AeInfo[sID].u8AeMaxFrameNum; ++i) {
		if (!pstAeMpiRoute[sID][i]) {
			ISP_LOG_ERR("%d %s\n", i, "pstAeMpiRoute is NULL.");
			return;
		}
		maxTime = (i == AE_LE) ? AeInfo[sID].u32ExpTimeMax : AeInfo[sID].u32WDRSEExpTimeMax;
		pstAeMpiRoute[sID][i]->u32TotalNum = 3;
		pstAeMpiRoute[sID][i]->astRouteNode[0].u32IntTime = AeInfo[sID].u32ExpTimeMin;
		pstAeMpiRoute[sID][i]->astRouteNode[0].u32SysGain = AeInfo[sID].u32TotalGainMin;
		pstAeMpiRoute[sID][i]->astRouteNode[0].enIrisFNO = ISP_IRIS_F_NO_1_0;
		pstAeMpiRoute[sID][i]->astRouteNode[0].u32IrisFNOLin = 1 << ISP_IRIS_F_NO_1_0;

		pstAeMpiRoute[sID][i]->astRouteNode[1].u32IntTime = maxTime;
		pstAeMpiRoute[sID][i]->astRouteNode[1].u32SysGain = AeInfo[sID].u32TotalGainMin;
		pstAeMpiRoute[sID][i]->astRouteNode[1].enIrisFNO = ISP_IRIS_F_NO_1_0;
		pstAeMpiRoute[sID][i]->astRouteNode[1].u32IrisFNOLin = 1 << ISP_IRIS_F_NO_1_0;

		pstAeMpiRoute[sID][i]->astRouteNode[2].u32IntTime = maxTime;
		pstAeMpiRoute[sID][i]->astRouteNode[2].u32SysGain = AeInfo[sID].u32TotalGainMax;
		pstAeMpiRoute[sID][i]->astRouteNode[2].enIrisFNO = ISP_IRIS_F_NO_1_0;
		pstAeMpiRoute[sID][i]->astRouteNode[2].u32IrisFNOLin = 1 << ISP_IRIS_F_NO_1_0;

		if (i == AE_LE) {
			AE_GetTvEntryByTime(sID, AeInfo[sID].u32ExpTimeMin, &AeInfo[sID].s16MaxRouteTVEntry);
			AE_GetTvEntryByTime(sID, AeInfo[sID].u32ExpTimeMax, &AeInfo[sID].s16MinRouteTVEntry);
			AE_GetISOEntryByGain(AeInfo[sID].u32TotalGainMin, &AeInfo[sID].s16MinRouteISOEntry);
			AE_GetISOEntryByGain(AeInfo[sID].u32TotalGainMax, &AeInfo[sID].s16MaxRouteISOEntry);
		}

		if (!pstAeMpiRouteEx[sID][i]) {
			ISP_LOG_ERR("%d %s\n", i, "pstAeMpiRouteEx is NULL.");
			return;
		}

		pstAeMpiRouteEx[sID][i]->u32TotalNum = 5;
		pstAeMpiRouteEx[sID][i]->astRouteExNode[0].u32IntTime = AeInfo[sID].u32ExpTimeMin;
		pstAeMpiRouteEx[sID][i]->astRouteExNode[0].u32Again = AeInfo[sID].u32AGainMin;
		pstAeMpiRouteEx[sID][i]->astRouteExNode[0].u32Dgain = AeInfo[sID].u32DGainMin;
		pstAeMpiRouteEx[sID][i]->astRouteExNode[0].u32IspDgain = AeInfo[sID].u32ISPDGainMin;
		pstAeMpiRouteEx[sID][i]->astRouteExNode[0].enIrisFNO = ISP_IRIS_F_NO_1_0;
		pstAeMpiRouteEx[sID][i]->astRouteExNode[0].u32IrisFNOLin = 1 << ISP_IRIS_F_NO_1_0;

		pstAeMpiRouteEx[sID][i]->astRouteExNode[1].u32IntTime = maxTime;
		pstAeMpiRouteEx[sID][i]->astRouteExNode[1].u32Again = AeInfo[sID].u32AGainMin;
		pstAeMpiRouteEx[sID][i]->astRouteExNode[1].u32Dgain = AeInfo[sID].u32DGainMin;
		pstAeMpiRouteEx[sID][i]->astRouteExNode[1].u32IspDgain = AeInfo[sID].u32ISPDGainMin;
		pstAeMpiRouteEx[sID][i]->astRouteExNode[1].enIrisFNO = ISP_IRIS_F_NO_1_0;
		pstAeMpiRouteEx[sID][i]->astRouteExNode[1].u32IrisFNOLin = 1 << ISP_IRIS_F_NO_1_0;

		pstAeMpiRouteEx[sID][i]->astRouteExNode[2].u32IntTime = maxTime;
		pstAeMpiRouteEx[sID][i]->astRouteExNode[2].u32Again = AeInfo[sID].u32AGainMax;
		pstAeMpiRouteEx[sID][i]->astRouteExNode[2].u32Dgain = AeInfo[sID].u32DGainMin;
		pstAeMpiRouteEx[sID][i]->astRouteExNode[2].u32IspDgain = AeInfo[sID].u32ISPDGainMin;
		pstAeMpiRouteEx[sID][i]->astRouteExNode[2].enIrisFNO = ISP_IRIS_F_NO_1_0;
		pstAeMpiRouteEx[sID][i]->astRouteExNode[2].u32IrisFNOLin = 1 << ISP_IRIS_F_NO_1_0;

		pstAeMpiRouteEx[sID][i]->astRouteExNode[3].u32IntTime = maxTime;
		pstAeMpiRouteEx[sID][i]->astRouteExNode[3].u32Again = AeInfo[sID].u32AGainMax;
		pstAeMpiRouteEx[sID][i]->astRouteExNode[3].u32Dgain = AeInfo[sID].u32DGainMax;
		pstAeMpiRouteEx[sID][i]->astRouteExNode[3].u32IspDgain = AeInfo[sID].u32ISPDGainMin;
		pstAeMpiRouteEx[sID][i]->astRouteExNode[3].enIrisFNO = ISP_IRIS_F_NO_1_0;
		pstAeMpiRouteEx[sID][i]->astRouteExNode[3].u32IrisFNOLin = 1 << ISP_IRIS_F_NO_1_0;

		pstAeMpiRouteEx[sID][i]->astRouteExNode[4].u32IntTime = maxTime;
		pstAeMpiRouteEx[sID][i]->astRouteExNode[4].u32Again = AeInfo[sID].u32AGainMax;
		pstAeMpiRouteEx[sID][i]->astRouteExNode[4].u32Dgain = AeInfo[sID].u32DGainMax;
		pstAeMpiRouteEx[sID][i]->astRouteExNode[4].u32IspDgain = AeInfo[sID].u32ISPDGainMax;
		pstAeMpiRouteEx[sID][i]->astRouteExNode[4].enIrisFNO = ISP_IRIS_F_NO_1_0;
		pstAeMpiRouteEx[sID][i]->astRouteExNode[4].u32IrisFNOLin = 1 << ISP_IRIS_F_NO_1_0;

		if (i == AE_LE) {
			AE_GetTvEntryByTime(sID, AeInfo[sID].u32ExpTimeMin, &AeInfo[sID].s16MaxRouteExTVEntry);
			AE_GetTvEntryByTime(sID, AeInfo[sID].u32ExpTimeMax, &AeInfo[sID].s16MinRouteExTVEntry);
			AE_GetISOEntryByGain(AeInfo[sID].u32TotalGainMin, &AeInfo[sID].s16MinRouteExISOEntry);
			AE_GetISOEntryByGain(AeInfo[sID].u32TotalGainMax, &AeInfo[sID].s16MaxRouteExISOEntry);
		}
	}
}

void AE_StatisticsConfig_Init(CVI_U8 sID)
{
	CVI_U8 i;
	sID = AE_CheckSensorID(sID);

	if (!pstAeStatisticsCfg[sID]) {
		ISP_LOG_ERR("%s\n", "pstAeStatisticsCfg is NULL.");
		return;
	}
	for (i = 0; i < AE_MAX_NUM; ++i) {
		pstAeStatisticsCfg[sID]->stCrop[i].bEnable = 0;
		pstAeStatisticsCfg[sID]->stCrop[i].u16X = 0;
		pstAeStatisticsCfg[sID]->stCrop[i].u16Y = 0;
		pstAeStatisticsCfg[sID]->stCrop[i].u16W = 1920;
		pstAeStatisticsCfg[sID]->stCrop[i].u16H = 1080;
	}

	memset(pstAeStatisticsCfg[sID]->au8Weight, 1,
		AE_WEIGHT_ZONE_ROW * AE_WEIGHT_ZONE_COLUMN * sizeof(CVI_U8));
}

void AE_SmartExposureAttr_Init(CVI_U8 sID)
{
	sID = AE_CheckSensorID(sID);

	if (!pstAeMpiSmartExposureAttr[sID]) {
		ISP_LOG_ERR("%s\n", "pstAeMpiSmartExposureAttr is NULL.");
		return;
	}
	pstAeMpiSmartExposureAttr[sID]->bEnable = 0;
	pstAeMpiSmartExposureAttr[sID]->bIRMode = 0;
	pstAeMpiSmartExposureAttr[sID]->enSmartExpType = OP_TYPE_AUTO;
	pstAeMpiSmartExposureAttr[sID]->u8LumaTarget = 46;
	pstAeMpiSmartExposureAttr[sID]->u16ExpCoef = 1024;
	pstAeMpiSmartExposureAttr[sID]->u16ExpCoefMax = 4096;
	pstAeMpiSmartExposureAttr[sID]->u16ExpCoefMin = 256;
	pstAeMpiSmartExposureAttr[sID]->u8SmartInterval = 1;
	pstAeMpiSmartExposureAttr[sID]->u8SmartSpeed = 32;
	pstAeMpiSmartExposureAttr[sID]->u16SmartDelayNum = 5;
	pstAeMpiSmartExposureAttr[sID]->u8Weight = 80;
	pstAeMpiSmartExposureAttr[sID]->u8NarrowRatio = 75;
}


void AE_Function_Init(CVI_U8 sID)
{
	sID = AE_CheckSensorID(sID);
	AE_MemoryAlloc(sID, AE_TOOL_PARAMETER_ITEM);
	AE_ExposureAttr_Init(sID);
	AE_WDRAttr_Init(sID);
	AE_Route_Init(sID);
	AE_StatisticsConfig_Init(sID);
	AE_SmartExposureAttr_Init(sID);
	AE_IrisAttr_Init(sID);

	AeInfo[sID].bParameterUpdate[AE_UPDATE_ALL] = 1;
}


void AE_GetBootInfo(CVI_U8 sID, SAE_BOOT_INFO *bootInfo)
{
	sID = AE_CheckSensorID(sID);
	if (AeBootInfo[sID])
		*bootInfo = *AeBootInfo[sID];
}


void AE_SetDebugMode(CVI_U8 sID, CVI_U8 mode)
{
	sID = AE_CheckSensorID(sID);
	if (mode && u8AeDebugPrintMode[sID] != mode)
		printf("AE debugM:%d\n", mode);
	u8AeDebugPrintMode[sID] = mode;
}

CVI_U8 AE_GetDebugMode(CVI_U8 sID)
{
	return u8AeDebugPrintMode[sID];
}

CVI_U32 AE_LimitExpTime(CVI_U8 sID, CVI_U32 expTime)
{
	CVI_U32 minExpTime, maxExpTime;

	sID = AE_CheckSensorID(sID);
	minExpTime = AAA_MAX(AeInfo[sID].u32ExpTimeMin,
		pstAeExposureAttrInfo[sID]->stAuto.stExpTimeRange.u32Min);

	if (AeInfo[sID].enSlowShutterStatus == AE_ENTER_SLOW_SHUTTER) {
		maxExpTime = pstAeExposureAttrInfo[sID]->stAuto.stExpTimeRange.u32Max;
	} else {
		maxExpTime = AAA_MIN(AeInfo[sID].u32ExpTimeMax,
		pstAeExposureAttrInfo[sID]->stAuto.stExpTimeRange.u32Max);
	}

	AAA_LIMIT(expTime, minExpTime, maxExpTime);
	return expTime;
}

CVI_U32 AE_LimitWDRSEExpTime(CVI_U8 sID, CVI_U32 expTime)
{
	AAA_LIMIT(expTime, AeInfo[sID].u32ExpTimeMin, AeInfo[sID].u32WDRSEExpTimeMax);
	return expTime;
}


CVI_U32 AE_LimitAGain(CVI_U8 sID, CVI_U32 AGain)
{
	CVI_U32 minAGain, maxAGain;
	CVI_BOOL isLimitISONum;

	sID = AE_CheckSensorID(sID);
	isLimitISONum = (pstAeExposureAttrInfo[sID]->enOpType == OP_TYPE_MANUAL) ?
		pstAeExposureAttrInfo[sID]->stManual.enGainType == AE_TYPE_ISO :
		pstAeExposureAttrInfo[sID]->stAuto.enGainType == AE_TYPE_ISO;
	if (isLimitISONum) {
		minAGain = AeInfo[sID].u32AGainMin;
		maxAGain = AeInfo[sID].u32AGainMax;
	} else {
		minAGain = AAA_MAX(AeInfo[sID].u32AGainMin,
			pstAeExposureAttrInfo[sID]->stAuto.stAGainRange.u32Min);
		maxAGain = AAA_MIN(AeInfo[sID].u32AGainMax,
			pstAeExposureAttrInfo[sID]->stAuto.stAGainRange.u32Max);
	}

	AAA_LIMIT(AGain, minAGain, maxAGain);
	return AGain;
}

CVI_U32 AE_LimitDGain(CVI_U8 sID, CVI_U32 DGain)
{
	CVI_U32 minDGain, maxDGain;
	CVI_BOOL isLimitISONum;

	sID = AE_CheckSensorID(sID);
	isLimitISONum = (pstAeExposureAttrInfo[sID]->enOpType == OP_TYPE_MANUAL) ?
		pstAeExposureAttrInfo[sID]->stManual.enGainType == AE_TYPE_ISO :
		pstAeExposureAttrInfo[sID]->stAuto.enGainType == AE_TYPE_ISO;
	if (isLimitISONum) {
		minDGain = AeInfo[sID].u32DGainMin;
		maxDGain = AeInfo[sID].u32DGainMax;
	} else {
		minDGain = AAA_MAX(AeInfo[sID].u32DGainMin,
			pstAeExposureAttrInfo[sID]->stAuto.stDGainRange.u32Min);
		maxDGain = AAA_MIN(AeInfo[sID].u32DGainMax,
			pstAeExposureAttrInfo[sID]->stAuto.stDGainRange.u32Max);
	}

	AAA_LIMIT(DGain, minDGain, maxDGain);
	return DGain;
}

CVI_U32 AE_LimitISPDGain(CVI_U8 sID, CVI_U32 ISPDGain)
{
	CVI_U32 minISPDGain, maxISPDGain;
	CVI_BOOL isLimitISONum;

	sID = AE_CheckSensorID(sID);
	isLimitISONum = (pstAeExposureAttrInfo[sID]->enOpType == OP_TYPE_MANUAL) ?
		pstAeExposureAttrInfo[sID]->stManual.enGainType == AE_TYPE_ISO :
		pstAeExposureAttrInfo[sID]->stAuto.enGainType == AE_TYPE_ISO;
	if (isLimitISONum) {
		minISPDGain = AeInfo[sID].u32ISPDGainMin;
		maxISPDGain = AeInfo[sID].u32ISPDGainMax;
	} else {
		minISPDGain = AAA_MAX(AeInfo[sID].u32ISPDGainMin,
			pstAeExposureAttrInfo[sID]->stAuto.stISPDGainRange.u32Min);
		maxISPDGain = AAA_MIN(AeInfo[sID].u32ISPDGainMax,
			pstAeExposureAttrInfo[sID]->stAuto.stISPDGainRange.u32Max);
	}
	AAA_LIMIT(ISPDGain, minISPDGain, maxISPDGain);
	return ISPDGain;
}

CVI_U32 AE_LimitTimeRange(CVI_U8 sID, CVI_U32 expTime)
{
	sID = AE_CheckSensorID(sID);
	AAA_LIMIT(expTime, pstAeExposureAttrInfo[sID]->stAuto.stExpTimeRange.u32Min,
		pstAeExposureAttrInfo[sID]->stAuto.stExpTimeRange.u32Max);
	return expTime;
}

CVI_U32 AE_LimitISONum(CVI_U8 sID, CVI_U32 ISONum)
{
	CVI_U32 minISONum, maxISONum;
	CVI_U32 minTotalGain, maxTotalGain;
	CVI_BOOL isLimitISONum;

	sID = AE_CheckSensorID(sID);
	isLimitISONum = (pstAeExposureAttrInfo[sID]->enOpType == OP_TYPE_MANUAL) ?
		pstAeExposureAttrInfo[sID]->stManual.enGainType == AE_TYPE_ISO :
		pstAeExposureAttrInfo[sID]->stAuto.enGainType == AE_TYPE_ISO;

	if (isLimitISONum) {
		minISONum = pstAeExposureAttrInfo[sID]->stAuto.stISONumRange.u32Min;
		maxISONum = pstAeExposureAttrInfo[sID]->stAuto.stISONumRange.u32Max;
	} else {
		minTotalGain = AE_CalTotalGain(pstAeExposureAttrInfo[sID]->stAuto.stAGainRange.u32Min,
			pstAeExposureAttrInfo[sID]->stAuto.stDGainRange.u32Min,
			pstAeExposureAttrInfo[sID]->stAuto.stISPDGainRange.u32Min);
		maxTotalGain = AE_CalTotalGain(pstAeExposureAttrInfo[sID]->stAuto.stAGainRange.u32Max,
			pstAeExposureAttrInfo[sID]->stAuto.stDGainRange.u32Max,
			pstAeExposureAttrInfo[sID]->stAuto.stISPDGainRange.u32Max);
		minTotalGain = AAA_MAX(minTotalGain, pstAeExposureAttrInfo[sID]->stAuto.stSysGainRange.u32Min);
		maxTotalGain = AAA_MIN(maxTotalGain, pstAeExposureAttrInfo[sID]->stAuto.stSysGainRange.u32Max);
		AE_GetISONumByGain(minTotalGain, &minISONum);
		AE_GetISONumByGain(maxTotalGain, &maxISONum);
	}
	minISONum = AAA_MAX(minISONum, AeInfo[sID].u32ISONumMin);
	maxISONum = AAA_MIN(maxISONum, AeInfo[sID].u32ISONumMax);
	AAA_LIMIT(ISONum, minISONum, maxISONum);
	return ISONum;
}

CVI_U32 AE_LimitISORange(CVI_U8 sID, CVI_U32 ISONum)
{
	CVI_U32 minISONum, maxISONum;

	sID = AE_CheckSensorID(sID);
	if (pstAeExposureAttrInfo[sID]->stAuto.enGainType == AE_TYPE_ISO) {
		minISONum = pstAeExposureAttrInfo[sID]->stAuto.stISONumRange.u32Min;
		maxISONum = pstAeExposureAttrInfo[sID]->stAuto.stISONumRange.u32Max;
	} else {
		AE_GetISONumByGain(pstAeExposureAttrInfo[sID]->stAuto.stSysGainRange.u32Min, &minISONum);
		AE_GetISONumByGain(pstAeExposureAttrInfo[sID]->stAuto.stSysGainRange.u32Max, &maxISONum);
	}
	AAA_LIMIT(ISONum, minISONum, maxISONum);
	return ISONum;
}


CVI_U32 AE_LimitAGainRange(CVI_U8 sID, CVI_U32 AGain)
{
	sID = AE_CheckSensorID(sID);
	AAA_LIMIT(AGain, pstAeExposureAttrInfo[sID]->stAuto.stAGainRange.u32Min,
		pstAeExposureAttrInfo[sID]->stAuto.stAGainRange.u32Max);
	return AGain;
}

CVI_U32 AE_LimitDGainRange(CVI_U8 sID, CVI_U32 DGain)
{
	sID = AE_CheckSensorID(sID);
	AAA_LIMIT(DGain, pstAeExposureAttrInfo[sID]->stAuto.stDGainRange.u32Min,
		pstAeExposureAttrInfo[sID]->stAuto.stDGainRange.u32Max);
	return DGain;
}

CVI_U32 AE_LimitISPDGainRange(CVI_U8 sID, CVI_U32 ISPDGain)
{
	sID = AE_CheckSensorID(sID);
	AAA_LIMIT(ISPDGain, pstAeExposureAttrInfo[sID]->stAuto.stISPDGainRange.u32Min,
		pstAeExposureAttrInfo[sID]->stAuto.stISPDGainRange.u32Max);
	return ISPDGain;
}

void AE_GetTvRangeEntryLimit(CVI_U8 sID, CVI_S16 *minEntry, CVI_S16 *maxEntry)
{
	sID = AE_CheckSensorID(sID);
	*minEntry = AeInfo[sID].s16MinRangeTVEntry;
	*maxEntry = AeInfo[sID].s16MaxRangeTVEntry;
}

void AE_GetISORangeEntryLimit(CVI_U8 sID, CVI_S16 *minEntry, CVI_S16 *maxEntry)
{
	sID = AE_CheckSensorID(sID);
	*minEntry = AeInfo[sID].s16MinRangeISOEntry;
	*maxEntry = AeInfo[sID].s16MaxRangeISOEntry;
}

CVI_U32 AE_LimitManualTime(CVI_U8 sID, CVI_U32 expTime)
{
	sID = AE_CheckSensorID(sID);
	expTime = AE_LimitTimeRange(sID, expTime);
	expTime = AAA_MAX(expTime, AeInfo[sID].u32ExpTimeMin);
	//expTime = AE_LimitExpTime(sID, expTime);
	return expTime;
}

CVI_U32 AE_LimitManualISONum(CVI_U8 sID, CVI_U32 ISONum)
{
	CVI_U32 minISONum, maxISONum;

	sID = AE_CheckSensorID(sID);
	minISONum = pstAeExposureAttrInfo[sID]->stAuto.stISONumRange.u32Min;
	maxISONum = pstAeExposureAttrInfo[sID]->stAuto.stISONumRange.u32Max;
	AAA_LIMIT(ISONum, minISONum, maxISONum);
	return ISONum;
}

CVI_U32 AE_LimitManualAGain(CVI_U8 sID, CVI_U32 AGain)
{
	sID = AE_CheckSensorID(sID);
	AGain = AE_LimitAGainRange(sID, AGain);
	AGain = AE_LimitAGain(sID, AGain);
	return AGain;
}

CVI_U32 AE_LimitManualDGain(CVI_U8 sID, CVI_U32 DGain)
{
	sID = AE_CheckSensorID(sID);
	DGain = AE_LimitDGainRange(sID, DGain);
	DGain = AE_LimitDGain(sID, DGain);
	return DGain;
}

CVI_U32 AE_LimitManualISPDGain(CVI_U8 sID, CVI_U32 ISPDGain)
{
	sID = AE_CheckSensorID(sID);
	ISPDGain = AE_LimitISPDGainRange(sID, ISPDGain);
	ISPDGain = AE_LimitISPDGain(sID, ISPDGain);
	return ISPDGain;
}



void AE_SetWDRLEOnly(CVI_U8 sID, CVI_BOOL wdrLEOnly)
{
	sID = AE_CheckSensorID(sID);
	if (AeInfo[sID].bWDRLEOnly != wdrLEOnly)
		printf("wdrLEOnly:%d\n", wdrLEOnly);
	AeInfo[sID].bWDRLEOnly = wdrLEOnly;
}


void AE_ResetFaceDetectInfo(CVI_U8 sID)
{
	sID = AE_CheckSensorID(sID);
	AeFaceDetect[sID].bMode = 0;
	AeFaceDetect[sID].u16FDPosX = AeFaceDetect[sID].u16FDPosY = 0;
	AeFaceDetect[sID].u16FDWidth = AeFaceDetect[sID].u16FDHeight = 0;
	AeFaceDetect[sID].u16AEFDPosX = AeFaceDetect[sID].u16AEFDPosY = 0;
	AeFaceDetect[sID].u16AEFDWidth = AeFaceDetect[sID].u16AEFDHeight = 0;

	memset(&AeFaceDetect[sID].stSmartInfo, 0, sizeof(ISP_SMART_ROI_S));
}


CVI_U8 AE_SetFaceInfo(CVI_U8 sID, const ISP_SMART_ROI_S *pstSmartInfo)
{
#define STABLE_FRAME_NUM	5
#define NARROW_FACE_RATIO	75
#define RATIO_X_100			100

	CVI_U16 gridPerSizeX, gridPerSizeY;
	CVI_U16 fdPosX = 0, fdPosY = 0, fdWidth = 0, fdHeight = 0;
	CVI_U16 frameWidth, frameHeight, aeFdPosX, aeFdPosY, aeFdWidth, aeFdHeight;
	CVI_U8 i, maxIdx = 0;
	CVI_U32	faceSize, maxFaceSize = 0;
	CVI_U16 ratioX100 = 0, delayFrmNum = 0;
	static CVI_U8 fdFrmStatusCnt[AE_SENSOR_NUM];

	sID = AE_CheckSensorID(sID);

	delayFrmNum = (pstAeSmartExposureAttrInfo[sID]->bEnable) ?
		pstAeSmartExposureAttrInfo[sID]->u16SmartDelayNum : STABLE_FRAME_NUM;

	if (AE_GetDebugMode(sID) == 43) {
		if (pstSmartInfo->bEnable) {
			for (i = 0; i < pstSmartInfo->u8Num; ++i) {
				printf("AE sID:%d fid:%u %d N:%d X:%d Y:%d W:%d H:%d FW:%d FH:%d\n",
					sID, AeInfo[sID].u32frmCnt, i,  pstSmartInfo->u8Num,
					pstSmartInfo->u16PosX[i], pstSmartInfo->u16PosY[i],
					pstSmartInfo->u16Width[i], pstSmartInfo->u16Height[i],
					pstSmartInfo->u16FrameWidth, pstSmartInfo->u16FrameHeight);
			}
		} else {
			printf("AE sID:%d fid:%u N:%d\n", sID, AeInfo[sID].u32frmCnt,
				pstSmartInfo->u8Num);
		}
	}

	if (pstSmartInfo->u8Num == 0 || !pstSmartInfo->bEnable ||
		(!pstAeSmartExposureAttrInfo[sID]->bEnable && !pstAeExposureAttrInfo[sID]->stAuto.bEnableFaceAE)) {
		fdFrmStatusCnt[sID] = (fdFrmStatusCnt[sID] >= 1) ? fdFrmStatusCnt[sID] - 1 : 0;

		if (fdFrmStatusCnt[sID] == 0)
			AE_ResetFaceDetectInfo(sID);

		return CVI_FALSE;
	}

	fdFrmStatusCnt[sID] = delayFrmNum;

	AeFaceDetect[sID].stSmartInfo.u8Num = pstSmartInfo->u8Num;
	for (i = 0; i < pstSmartInfo->u8Num; ++i) {
		AeFaceDetect[sID].stSmartInfo.u16PosX[i] = pstSmartInfo->u16PosX[i];
		AeFaceDetect[sID].stSmartInfo.u16PosY[i] = pstSmartInfo->u16PosY[i];
		AeFaceDetect[sID].stSmartInfo.u16Width[i] = pstSmartInfo->u16Width[i];
		AeFaceDetect[sID].stSmartInfo.u16Height[i] = pstSmartInfo->u16Height[i];
	}

	AeFaceDetect[sID].stSmartInfo.u16FrameWidth = pstSmartInfo->u16FrameWidth;
	AeFaceDetect[sID].stSmartInfo.u16FrameHeight = pstSmartInfo->u16FrameHeight;

	if (AeFaceDetect[sID].stSmartInfo.u8Num) {
		for (i = 0; i < AeFaceDetect[sID].stSmartInfo.u8Num; ++i) {
			faceSize = AeFaceDetect[sID].stSmartInfo.u16Width[i] *
				AeFaceDetect[sID].stSmartInfo.u16Height[i];
			if (faceSize > maxFaceSize) {
				maxIdx = i;
				maxFaceSize = faceSize;
			}
		}
		fdPosX = AeFaceDetect[sID].stSmartInfo.u16PosX[maxIdx];
		fdPosY = AeFaceDetect[sID].stSmartInfo.u16PosY[maxIdx];
		fdWidth = AeFaceDetect[sID].stSmartInfo.u16Width[maxIdx];
		fdHeight = AeFaceDetect[sID].stSmartInfo.u16Height[maxIdx];
	}

	frameWidth = AeFaceDetect[sID].stSmartInfo.u16FrameWidth;
	frameHeight = AeFaceDetect[sID].stSmartInfo.u16FrameHeight;

	gridPerSizeX = (frameWidth / AE_ZONE_COLUMN) ? frameWidth / AE_ZONE_COLUMN : 1;
	gridPerSizeY = (frameHeight / AE_ZONE_ROW) ? frameHeight / AE_ZONE_ROW : 1;

	if ((fdPosX + fdWidth > frameWidth) || (fdPosY + fdHeight > frameHeight) ||
		(!fdWidth) || (!fdHeight) || (!gridPerSizeX) || (!gridPerSizeY)) {
		AE_ResetFaceDetectInfo(sID);
		return CVI_FALSE;
	}

	AeFaceDetect[sID].bMode = 1;
	AeFaceDetect[sID].u16FrameWidth = frameWidth;
	AeFaceDetect[sID].u16FrameHeight = frameHeight;
	AeFaceDetect[sID].u16FDPosX = fdPosX;
	AeFaceDetect[sID].u16FDPosY = fdPosY;
	AeFaceDetect[sID].u16FDWidth = fdWidth;
	AeFaceDetect[sID].u16FDHeight = fdHeight;
	AeFaceDetect[sID].u8GridSizeX = gridPerSizeX;
	AeFaceDetect[sID].u8GridSizeY = gridPerSizeY;

	if (pstSmartInfo->bAvailable) {
		AeFaceDetect[sID].bUpdateInfo = CVI_TRUE;
	}

	#if 0
	if (fdWidth > fdHeight) {
		printf("FD  W > H , FdWidth:%d FdHeight:%d\n", fdWidth, fdHeight);
		fdWidth = fdHeight;
	}
	#endif


	ratioX100 = (pstAeSmartExposureAttrInfo[sID]->bEnable) ?
		100 - pstAeSmartExposureAttrInfo[sID]->u8NarrowRatio : 100 - NARROW_FACE_RATIO;

	if (ratioX100 == 0)
		ratioX100 = 1;
	ratioX100 = (sqrt(ratioX100) * RATIO_X_100 + 10 / 2) / 10;

	aeFdWidth = fdWidth * ratioX100 / RATIO_X_100;
	aeFdHeight = fdHeight * ratioX100 / RATIO_X_100;
	aeFdPosX = fdPosX + fdWidth / 2  - aeFdWidth / 2;
	aeFdPosY = fdPosY + fdHeight / 2 - aeFdHeight / 2;


	AeFaceDetect[sID].u16AEFDPosX = aeFdPosX;
	AeFaceDetect[sID].u16AEFDPosY = aeFdPosY;
	AeFaceDetect[sID].u16AEFDWidth = aeFdWidth;
	AeFaceDetect[sID].u16AEFDHeight = aeFdHeight;

	#if 0
	printf("Face X:%d Y:%d W:%d H:%d SX:%d SY:%d\n", AeFaceDetect.FdPosX, AeFaceDetect.FdPosY,
			AeFaceDetect.FdFdWidth, AeFaceDetect.FdFdHeight, AeFaceDetect.FullScreenX,
			AeFaceDetect.FullScreenY);
	printf("AE  X:%d Y:%d W:%d H:%d\n", AeFaceDetect.PosX, AeFaceDetect.PosY, AeFaceDetect.FdWidth,
			AeFaceDetect.FdHeight);
	#endif

	return CVI_TRUE;
}


void AE_CalFaceDetectLuma(CVI_U8 sID)
{
	CVI_U16 faceGridCount = 0;
	CVI_U16 fdGridStartIndex, fdGridEndIndex;
	CVI_U16 tmpFaceLuma = 0, tmpGridLuma = 0;
	CVI_U32 tmpFaceLumaSum = 0;
	CVI_U8 rowStartIdx, columnStartIdx, rowEndIdx, columnEndIdx,
			row, column;
	CVI_U16	tmpAEFdGridLuma[AE_FD_GRID_LUMA_SIZE];
	CVI_U16	tmpAEFDPosX, tmpAEFDPosY, tmpAEFDWidth, tmpAEFDHeight;
	CVI_S32 tmpBvStepSum = 0, tmpBvStep;

	if (!pstAeExposureAttrInfo[sID]->stAuto.bEnableFaceAE &&
		!pstAeSmartExposureAttrInfo[sID]->bEnable)
		return;

	if (!AeFaceDetect[sID].bMode)
		return;

	memset(tmpAEFdGridLuma, 0, sizeof(tmpAEFdGridLuma));

	tmpAEFDPosX = AeFaceDetect[sID].u16AEFDPosX;
	tmpAEFDPosY = AeFaceDetect[sID].u16AEFDPosY;
	tmpAEFDWidth = AeFaceDetect[sID].u16AEFDWidth;
	tmpAEFDHeight = AeFaceDetect[sID].u16AEFDHeight;

	if (!AeFaceDetect[sID].u8GridSizeX)
		AeFaceDetect[sID].u8GridSizeX = 1;

	if (!AeFaceDetect[sID].u8GridSizeY)
		AeFaceDetect[sID].u8GridSizeY = 1;

	fdGridStartIndex = tmpAEFDPosY / AeFaceDetect[sID].u8GridSizeY * AE_ZONE_COLUMN +
						tmpAEFDPosX / AeFaceDetect[sID].u8GridSizeX;

	fdGridEndIndex = (tmpAEFDPosY + tmpAEFDHeight) / AeFaceDetect[sID].u8GridSizeY * AE_ZONE_COLUMN +
						(tmpAEFDPosX + tmpAEFDWidth) / AeFaceDetect[sID].u8GridSizeX;

	rowStartIdx = tmpAEFDPosY / AeFaceDetect[sID].u8GridSizeY;
	columnStartIdx = tmpAEFDPosX / AeFaceDetect[sID].u8GridSizeX;

	rowEndIdx = (tmpAEFDPosY + tmpAEFDHeight) / AeFaceDetect[sID].u8GridSizeY;
	columnEndIdx = (tmpAEFDPosX + tmpAEFDWidth) / AeFaceDetect[sID].u8GridSizeX;


	if (pstAeSmartExposureAttrInfo[sID]->bEnable) {
		AeFaceDetect[sID].u16FDTargetLuma = pstAeSmartExposureAttrInfo[sID]->u8LumaTarget << 2;
		if (pstAeSmartExposureAttrInfo[sID]->enSmartExpType == OP_TYPE_MANUAL) {
			AeFaceDetect[sID].u16FDTargetLuma = AeFaceDetect[sID].u16FDTargetLuma *
				pstAeSmartExposureAttrInfo[sID]->u16ExpCoef / EXP_COEF_BASE;
		}
	} else {
		AeFaceDetect[sID].u16FDTargetLuma = pstAeExposureAttrInfo[sID]->stAuto.u8FaceTargetLuma << 2;
	}

	AAA_LIMIT(AeFaceDetect[sID].u16FDTargetLuma, AE_METER_MIN_LUMA, AE_MAX_LUMA);

	for (row = rowStartIdx; row <= rowEndIdx; ++row) {
		for (column = columnStartIdx; column <= columnEndIdx; ++column) {
			tmpGridLuma = AeInfo[sID].u16MeterLuma[AE_LE][row][column];
			tmpGridLuma = AAA_MAX(tmpGridLuma, AE_METER_MIN_LUMA);
			tmpFaceLumaSum += tmpGridLuma;
			tmpBvStep = (Log2x100Tbl[tmpGridLuma] -
				Log2x100Tbl[AeFaceDetect[sID].u16FDTargetLuma]) * ENTRY_PER_BV;
			tmpBvStep = (tmpBvStep > 0) ? ((tmpBvStep + 50) / 100) : ((tmpBvStep - 50) / 100);
			tmpBvStepSum += tmpBvStep;
			++faceGridCount;
		}
	}

	tmpFaceLuma = (faceGridCount) ? tmpFaceLumaSum/faceGridCount : tmpFaceLumaSum;
	tmpBvStep = tmpBvStepSum / AAA_DIV_0_TO_1(faceGridCount);

	AeFaceDetect[sID].u16AEFDGridStart = fdGridStartIndex;
	AeFaceDetect[sID].u16AEFDGridEnd = fdGridEndIndex;//fdGridIndex;

	AAA_LIMIT(tmpFaceLuma, 60, AE_MAX_LUMA);
	AeFaceDetect[sID].u16FDLuma = tmpFaceLuma;
	AeFaceDetect[sID].s16FDEVStep = tmpBvStep;
	#if 0
	AeFaceDetect[sID].s16FDEVStep = (Log2x100Tbl[AeFaceDetect[sID].u16FDLuma] -
					Log2x100Tbl[AeFaceDetect[sID].u16FDTargetLuma]) * ENTRY_PER_BV / 100;
	#endif
	AeFaceDetect[sID].u8AEFDGridWidthCount = columnEndIdx - columnStartIdx + 1;
	AeFaceDetect[sID].u8AEFDGridHeightCount = rowEndIdx - rowStartIdx + 1;
	AeFaceDetect[sID].u16AEFDGridCount = faceGridCount;

	if (AE_GetDebugMode(sID) == 3) {
		printf("GS:%d GE:%d GW:%d GH:%d GC:%d\n", AeFaceDetect[sID].u16AEFDGridStart,
			AeFaceDetect[sID].u16AEFDGridEnd, AeFaceDetect[sID].u8AEFDGridWidthCount,
			AeFaceDetect[sID].u8AEFDGridHeightCount, AeFaceDetect[sID].u16AEFDGridCount);
		AE_SetDebugMode(sID, 0);
	}
}

void AE_CalFaceDetectBvStep(CVI_U8 sID)
{
	CVI_U8 faceWeight, envWeight;
	CVI_S16	minBvStep, maxBvStep;
	CVI_U16 expCoefMin = 0, expCoefMax = 0;

	if (!pstAeExposureAttrInfo[sID]->stAuto.bEnableFaceAE &&
		!pstAeSmartExposureAttrInfo[sID]->bEnable)
		return;

	if (!AeFaceDetect[sID].bMode)
		return;

	if (pstAeSmartExposureAttrInfo[sID]->bEnable) {
		if (AeInfo[sID].u32frmCnt % pstAeSmartExposureAttrInfo[sID]->u8SmartInterval)
			return;
	}

	faceWeight = (pstAeSmartExposureAttrInfo[sID]->bEnable) ?
		pstAeSmartExposureAttrInfo[sID]->u8Weight : pstAeExposureAttrInfo[sID]->stAuto.u8FaceWeight;
	AeFaceDetect[sID].s16EnvBvStep = AeInfo[sID].s16BvStepEntry[AE_LE];
	envWeight = 100 - faceWeight;

	if (AeFaceDetect[sID].bUpdateInfo || AeFaceDetect[sID].s16CurTargetBv == 0) {
		AeFaceDetect[sID].s16FinalBvStep = (AeFaceDetect[sID].s16EnvBvStep * envWeight +
			AeFaceDetect[sID].s16FDEVStep * faceWeight) / 100;
		AeFaceDetect[sID].s16CurTargetBv = AeInfo[sID].stApex[AE_LE].s16BVEntry +
					AeFaceDetect[sID].s16FinalBvStep;
		AeFaceDetect[sID].bUpdateInfo = CVI_FALSE;
	} else {
		AeFaceDetect[sID].s16FinalBvStep = AeFaceDetect[sID].s16CurTargetBv -
					AeInfo[sID].stApex[AE_LE].s16BVEntry;
	}

	if (pstAeSmartExposureAttrInfo[sID]->bEnable) {
		if (pstAeSmartExposureAttrInfo[sID]->enSmartExpType == OP_TYPE_AUTO) {
			expCoefMin = pstAeSmartExposureAttrInfo[sID]->u16ExpCoefMin;
			expCoefMax = pstAeSmartExposureAttrInfo[sID]->u16ExpCoefMax;

			if (expCoefMin > expCoefMax)
				expCoefMin = expCoefMax;

			maxBvStep = -ENTRY_PER_EV * AE_CalculateLog2(expCoefMin, EXP_COEF_BASE);
			minBvStep = -ENTRY_PER_EV * AE_CalculateLog2(expCoefMax, EXP_COEF_BASE);

			if (AeFaceDetect[sID].s16FinalBvStep - AeInfo[sID].s16BvStepEntry[AE_LE] > maxBvStep)
				AeFaceDetect[sID].s16FinalBvStep = AeInfo[sID].s16BvStepEntry[AE_LE] + maxBvStep;
			else if (AeFaceDetect[sID].s16FinalBvStep - AeInfo[sID].s16BvStepEntry[AE_LE] < minBvStep)
				AeFaceDetect[sID].s16FinalBvStep = AeInfo[sID].s16BvStepEntry[AE_LE] + minBvStep;

			#if 0
				printf("Fd fid:%d Bs:%d %d m:%d M:%d F:%d\n", AeInfo[sID].u32frmCnt,
						AeInfo[sID].s16BvStepEntry[AE_LE], AeFaceDetect[sID].s16FDEVStep,
						minBvStep, maxBvStep, AeFaceDetect[sID].s16FinalBvStep);
			#endif
		}
	} else {
		maxBvStep = ENTRY_PER_EV * 3;
		minBvStep = -ENTRY_PER_EV * 3;

		if (AeFaceDetect[sID].s16FinalBvStep - AeInfo[sID].s16BvStepEntry[AE_LE] > maxBvStep)
			AeFaceDetect[sID].s16FinalBvStep = AeInfo[sID].s16BvStepEntry[AE_LE] + maxBvStep;
		else if (AeFaceDetect[sID].s16FinalBvStep - AeInfo[sID].s16BvStepEntry[AE_LE] < minBvStep)
			AeFaceDetect[sID].s16FinalBvStep = AeInfo[sID].s16BvStepEntry[AE_LE] + minBvStep;
	}

	AeInfo[sID].s16BvStepEntry[AE_LE] = AeFaceDetect[sID].s16FinalBvStep;
}

CVI_U32 AE_GetFrameLine(CVI_U8 sID)
{
	sID = AE_CheckSensorID(sID);
	return AeInfo[sID].u32FrameLine;
}

void AE_GetStatisticsData(VI_PIPE ViPipe, const ISP_AE_INFO_S *pstAeInfo)
{
	CVI_U16 row, column, centerCnt = 0, i, j;
	CVI_U16 RValue, GValue, BValue, maxValue;
	CVI_U8 centerRowStart, centerRowEnd, centerColumnStart, centerColumnEnd;
	CVI_U32 centerG = 0, frameLumaSum[AE_MAX_WDR_FRAME_NUM] = {0, 0};
	CVI_U32	hisTotalCnt[AE_MAX_WDR_FRAME_NUM] = {0, 0};
	CVI_U8 sID;
	CVI_U64 hisBrightQuaSum[AE_MAX_WDR_FRAME_NUM] = {0, 0};
	CVI_U32 hisBrightQuaCnt[AE_MAX_WDR_FRAME_NUM] = {0, 0};

	sID = AE_ViPipe2sID(ViPipe);

	centerRowStart = (AeInfo[sID].u8GridVNum) / 2 - AeInfo[sID].u8GridVNum / 4;
	centerRowEnd = (AeInfo[sID].u8GridVNum) / 2 + AeInfo[sID].u8GridVNum / 4;
	centerColumnStart = (AeInfo[sID].u8GridHNum) / 2 - AeInfo[sID].u8GridHNum / 4;
	centerColumnEnd = (AeInfo[sID].u8GridHNum) / 2 + AeInfo[sID].u8GridHNum / 4;

	for (i = 0; i < AeInfo[sID].u8AeMaxFrameNum; ++i) {
		for (row = 0; row < AeInfo[sID].u8GridVNum; row++) {
			for (column = 0; column < AeInfo[sID].u8GridHNum; column++) {
				RValue = pstAeInfo->pstFEAeStat3[0]->au16ZoneAvg[i][row][column][AE_CHANNEL_R];
				GValue = (pstAeInfo->pstFEAeStat3[0]->au16ZoneAvg[i][row][column][AE_CHANNEL_GR] +
					  pstAeInfo->pstFEAeStat3[0]->au16ZoneAvg[i][row][column][AE_CHANNEL_GB]) / 2;
				BValue = pstAeInfo->pstFEAeStat3[0]->au16ZoneAvg[i][row][column][AE_CHANNEL_B];
				#if CHECK_AE_SIM
				if (i == 0) {//LE
					RValue = myR2[column + row * AeInfo[sID].u8GridHNum];
					GValue = myG2[column + row * AeInfo[sID].u8GridHNum];
					BValue = myB2[column + row * AeInfo[sID].u8GridHNum];
				} else {
					RValue = myR[column + row * AeInfo[sID].u8GridHNum];
					GValue = myG[column + row * AeInfo[sID].u8GridHNum];
					BValue = myB[column + row * AeInfo[sID].u8GridHNum];
				}
				#endif

				if ((row >= centerRowStart && row <= centerRowEnd) &&
				    (column >= centerColumnStart && column <= centerColumnEnd)) {
					centerCnt++;
					centerG += GValue;
				}

				maxValue = AAA_MAX(RValue, GValue);
				maxValue = AAA_MAX(maxValue, BValue);
				frameLumaSum[i] += maxValue;
			}
		}

		AeInfo[sID].u16CenterG[i] = centerG / AAA_DIV_0_TO_1(centerCnt);

		centerCnt = centerG = 0;
		AeInfo[sID].u16FrameAvgLuma[i] = frameLumaSum[i] / AE_ZONE_NUM;
		for (j = 0; j < HIST_NUM; ++j) {
			#if CHECK_AE_SIM
			if (i == 0) {//LE
				AeInfo[sID].u32Histogram[i][j] = myHis2[j];
			} else {
				AeInfo[sID].u32Histogram[i][j] = myHis[j];
			}
			#endif
			hisTotalCnt[i] += pstAeInfo->pstFEAeStat1[0]->au32HistogramMemArray[i][j];
		}

		AeInfo[sID].u32HistogramTotalCnt[i] = hisTotalCnt[i];

		for (j = HIST_NUM - 1 ; j > 0; --j) {
			hisBrightQuaCnt[i] += pstAeInfo->pstFEAeStat1[0]->au32HistogramMemArray[i][j];
			hisBrightQuaSum[i] += pstAeInfo->pstFEAeStat1[0]->au32HistogramMemArray[i][j] * j * 4;
			if (hisBrightQuaCnt[i] >= hisTotalCnt[i] / 4) {
				AeInfo[sID].u16BrightQuaG[i] = hisBrightQuaSum[i] / AAA_DIV_0_TO_1(hisBrightQuaCnt[i]);
				break;
			}
		}
	}

	AeInfo[sID].pu32Histogram = pstAeInfo->pstFEAeStat1[0]->au32HistogramMemArray;
	AeInfo[sID].pu16AeStatistics = pstAeInfo->pstFEAeStat3[0]->au16ZoneAvg;
}

CVI_S32 AE_InitSensorFpsPara(CVI_U8 sID, CVI_FLOAT fps, AE_CTX_S *pstAeCtx)
{
	CVI_U32 uMinLineTime;
	CVI_U8 offset = 0;
	VI_PIPE viPipe;
	CVI_U16 manual = 1;
	CVI_U32 ratio[3] = { 256, 64, 64 }; //max 256x
	CVI_U32 IntTimeMax[4], IntTimeMin[4], LFMaxIntTime[4];
	CVI_S32	result;
	static CVI_BOOL isInit[AE_SENSOR_NUM];

	sID = AE_CheckSensorID(sID);
	viPipe = AE_sID2ViPipe(sID);

	AAA_LIMIT(fps, AeInfo[sID].fMinFps, AeInfo[sID].fMaxFps);

#ifndef AAA_PC_PLATFORM
	/*set fps.*/
	if (pstAeCtx->stSnsRegister.stAeExp.pfn_cmos_fps_set) {
		result = pstAeCtx->stSnsRegister.stAeExp.pfn_cmos_fps_set(viPipe, fps, &pstAeCtx->stSnsDft);
		if (result == CVI_FAILURE) {
			ISP_LOG_ERR("%s\n", "fps range over NULL!\n");
			return CVI_FAILURE;
		}
	}

	AeInfo[sID].fExpLineTime = 500000 / (CVI_FLOAT)(pstAeCtx->stSnsDft.u32LinesPer500ms);
	AeInfo[sID].u32FrameLine = pstAeCtx->stSnsDft.u32FullLinesStd;
	AeInfo[sID].fFps = fps;

	if (AeInfo[sID].fExpTimeAccu < 1) {
		AeInfo[sID].fExpLineTime = AeInfo[sID].fExpLineTime * AeInfo[sID].fExpTimeAccu;
	}

	uMinLineTime = AeInfo[sID].fExpLineTime * 1000000;
	//min line time = 29.62963us , min line time int should be 30us
	if (uMinLineTime % 1000000)
		offset = 1;

	if (AeInfo[sID].bWDRMode) {
		if (pstAeCtx->stSnsRegister.stAeExp.pfn_cmos_get_inttime_max)
			pstAeCtx->stSnsRegister.stAeExp.pfn_cmos_get_inttime_max(viPipe, manual, ratio, IntTimeMax,
											 IntTimeMin, LFMaxIntTime);

		//SE Time range
		if (AeInfo[sID].fFps >= 25 || !isInit[sID]) {
			AeInfo[sID].u32WDRSEExpLineMax = IntTimeMax[0];
			AeInfo[sID].u32ExpLineMin = IntTimeMin[0];
			AeInfo[sID].u32ExpLineMax = pstAeCtx->stSnsDft.u32FullLinesStd - IntTimeMax[0];
			AeInfo[sID].u32WDRSEExpTimeMax = AeInfo[sID].u32WDRSEExpLineMax * AeInfo[sID].fExpLineTime;
			AE_GetTvEntryByTime(sID, AeInfo[sID].u32WDRSEExpTimeMax, &AeInfo[sID].s16WDRSEMinTVEntry);
		}

		ISP_LOG_DEBUG("WDR Line:%d Time:%d E:%d\n", AeInfo[sID].u32WDRSEExpLineMax,
			AeInfo[sID].u32WDRSEExpTimeMax, AeInfo[sID].s16WDRSEMinTVEntry);
	} else {
		AeInfo[sID].u32ExpLineMin = pstAeCtx->stSnsDft.u32MinIntTime;
		AeInfo[sID].u32ExpLineMax = pstAeCtx->stSnsDft.u32MaxIntTime;
	}

	AeInfo[sID].u32ExpTimeMin = AeInfo[sID].u32ExpLineMin * AeInfo[sID].fExpLineTime + offset;
	AeInfo[sID].u32ExpTimeMax = AeInfo[sID].u32ExpLineMax * AeInfo[sID].fExpLineTime;
	AeInfo[sID].u32MinFpsExpTimeMax = 1000000 / AAA_DIV_0_TO_1(AeInfo[sID].fMinFps);
	AeInfo[sID].u32LinePer500ms = pstAeCtx->stSnsDft.u32LinesPer500ms;
	isInit[sID] = 1;

	AE_GetTvEntryByTime(sID, AeInfo[sID].u32ExpTimeMin, &AeInfo[sID].s16MaxSensorTVEntry);
	AE_GetTvEntryByTime(sID, AeInfo[sID].u32ExpTimeMax, &AeInfo[sID].s16MinSensorTVEntry);
	AE_GetTvEntryByTime(sID, AeInfo[sID].u32MinFpsExpTimeMax, &AeInfo[sID].s16MinFpsSensorTVEntry);

	AE_SetParameterUpdate(sID, AE_ROUTE_UPDATE);
	AE_SetParameterUpdate(sID, AE_ROUTE_EX_UPDATE);
	AE_SetParameterUpdate(sID, AE_ROUTE_SF_UPDATE);
	AE_SetParameterUpdate(sID, AE_ROUTE_SF_EX_UPDATE);

#endif
	return CVI_SUCCESS;
}

void AE_GetSensorDGainNode(CVI_U8 sID, const AE_CTX_S *pstAeCtx)
{
#ifndef AAA_PC_PLATFORM

#define GAIN_CALCULATE_NODE	(20)

	CVI_U32 i, gainGap;
	CVI_U32	dGain, dGainDb;

	static CVI_U32 preDGain[AE_SENSOR_NUM] = {0xffffffff, 0xffffffff};
	VI_PIPE ViPipe = AE_sID2ViPipe(sID);

	if (AeInfo[sID].u8DGainAccuType == AE_ACCURACY_DB) {
		gainGap = (AeInfo[sID].u32DGainMax - AeInfo[sID].u32DGainMin) / GAIN_CALCULATE_NODE;
		for (i = AeInfo[sID].u32DGainMin; i <= AeInfo[sID].u32DGainMax; i += gainGap) {
			dGain = i;
			if (pstAeCtx->stSnsRegister.stAeExp.pfn_cmos_dgain_calc_table)
				pstAeCtx->stSnsRegister.stAeExp.pfn_cmos_dgain_calc_table(ViPipe, &dGain, &dGainDb);
			if (dGain != preDGain[sID]) {
				AeInfo[sID].u32SensorDgainNode[AeInfo[sID].u8SensorDgainNodeNum] =
					AAA_MAX(dGain, AE_GAIN_BASE);
				ISP_LOG_DEBUG("%d(%d)\n",
					AeInfo[sID].u32SensorDgainNode[AeInfo[sID].u8SensorDgainNodeNum],
					AeInfo[sID].u8SensorDgainNodeNum);
				AeInfo[sID].u8SensorDgainNodeNum++;
			}
			preDGain[sID] = dGain;
			if (dGain >= pstAeCtx->stSnsDft.u32MaxDgain)
				break;
		}
	}
#endif
}

CVI_S32 AE_InitSensorPara(VI_PIPE ViPipe, const ISP_AE_PARAM_S *pstAeParam)
{
	AE_CTX_S *pstAeCtx;
	ISP_PUB_ATTR_S pubAttr;
	CVI_U8 sID;
	CVI_S32 result;
	CVI_BOOL setDefaultFps = 0;

#ifndef AAA_PC_PLATFORM

	pstAeCtx = AE_GET_CTX(ViPipe);
	sID = AE_CheckSensorID(ViPipe);
	CVI_ISP_GetPubAttr(ViPipe, &pubAttr);

	if (pstAeCtx->stSnsRegister.stAeExp.pfn_cmos_get_ae_default)
		pstAeCtx->stSnsRegister.stAeExp.pfn_cmos_get_ae_default(ViPipe, &pstAeCtx->stSnsDft);

	AeInfo[sID].fMaxFps = pstAeCtx->stSnsDft.f32Fps;
	AeInfo[sID].fMinFps = AAA_MAX(1, pstAeCtx->stSnsDft.f32MinFps);
	AeInfo[sID].fExpTimeAccu = pstAeCtx->stSnsDft.stIntTimeAccu.f32Accuracy;
	AeInfo[sID].bWDRMode = (pstAeParam->u8WDRMode == WDR_MODE_NONE) ? 0 : 1;

	if (pubAttr.f32FrameRate < AeInfo[sID].fMinFps || pubAttr.f32FrameRate > AeInfo[sID].fMaxFps) {
		setDefaultFps = 1;
		pubAttr.f32FrameRate = (pstAeParam->u8WDRMode == WDR_MODE_NONE) ? LINEAR_MODE_FPS : WDR_MODE_FPS;
	}

	AeInfo[sID].fDefaultFps = pubAttr.f32FrameRate;
	result = AE_InitSensorFpsPara(ViPipe, pubAttr.f32FrameRate, pstAeCtx);

	if (result == CVI_SUCCESS && setDefaultFps) {
		CVI_ISP_SetPubAttr(ViPipe, &pubAttr);
	}

	AeInfo[sID].u8GridHNum = pstAeParam->aeLEWinConfig[0].winXNum;
	AeInfo[sID].u8GridVNum = pstAeParam->aeLEWinConfig[0].winYNum;
	AeInfo[sID].u32AGainMin = pstAeCtx->stSnsDft.u32MinAgain;
	AeInfo[sID].u32AGainMax = pstAeCtx->stSnsDft.u32MaxAgain;
	AeInfo[sID].u32DGainMin = pstAeCtx->stSnsDft.u32MinDgain;
	AeInfo[sID].u32DGainMax = pstAeCtx->stSnsDft.u32MaxDgain;
	AeInfo[sID].u8DGainAccuType = AE_ACCURACY_TABLE; //pstAeCtx->stSnsDft.stDgainAccu.enAccuType;

	AeInfo[sID].u8SensorPeriod = pstAeCtx->stSnsDft.u32AEResponseFrame;
	AAA_LIMIT(AeInfo[sID].u8SensorPeriod, 1, AE_MAX_PERIOD_NUM);

	AeInfo[sID].u8BootWaitFrameNum = pstAeCtx->stSnsDft.u32SnsStableFrame;
	AeInfo[sID].bDGainSwitchToAGain = (AeInfo[sID].u8DGainAccuType == AE_ACCURACY_DB)
		? DGAIN_TO_AGAIN : 0;
	AeInfo[sID].enBlcType = pstAeCtx->stSnsDft.enBlcType;

	AeInfo[sID].u8SensorRunInterval = AAA_MAX(1, pstAeCtx->stSnsDft.u8AERunInterval);
	AeInfo[sID].enWDRGainMode = pstAeCtx->stSnsDft.enWDRGainMode;
	memcpy(&AeInfo[sID].stStitchAttr, &pstAeParam->stStitchAttr, sizeof(ISP_STITCH_ATTR_S));

	AE_GetSensorDGainNode(sID, pstAeCtx);
#endif

	return CVI_SUCCESS;
}

CVI_S32 aeInit(VI_PIPE ViPipe, const ISP_AE_PARAM_S *pstAeParam)
{
	CVI_U8 sID;
	CVI_S32 res;
	static CVI_BOOL isAeInit[AE_SENSOR_NUM];

	ISP_LOG_NOTICE("Func : %s line %d ver %d.%d\n", __func__, __LINE__, AE_LIB_VER, AE_LIB_SUBVER);
	sID = AE_ViPipe2sID(ViPipe);
	res = AE_InitSensorPara(ViPipe, pstAeParam);
	if (res != CVI_SUCCESS)
		return CVI_FAILURE;

	AE_InitPara(ViPipe, isAeInit[sID]);

	AE_Function_Init(sID);
	AE_SetExposure(sID, AeInfo[sID].stExp);
	isAeInit[sID] = 1;

	return CVI_SUCCESS;
}

void AE_CalculateWeightSum(CVI_U8 sID)
{
	CVI_U8 i, j;
	CVI_U16 weightSum = 0;

	for (i = 0; i < AE_WEIGHT_ZONE_ROW; ++i) {
		for (j = 0; j < AE_WEIGHT_ZONE_COLUMN; ++j) {
			weightSum += pstAeStatisticsCfgInfo[sID]->au8Weight[i][j];
		}
	}

	AeInfo[sID].u16WeightSum = (weightSum) ? weightSum : 1;
}

void AE_CheckRouteRange(CVI_U8 sID, AE_WDR_FRAME wdrFrm, ISP_AE_ROUTE_S *pstRoute)
{
	CVI_U8 i;
	CVI_U32 maxTime;

	maxTime = (wdrFrm == AE_SE) ? AeInfo[sID].u32WDRSEExpTimeMax : AeInfo[sID].u32ExpTimeMax;
	for (i = 0; i < pstRoute->u32TotalNum; ++i) {
		AAA_LIMIT(pstRoute->astRouteNode[i].u32IntTime, AeInfo[sID].u32ExpTimeMin, maxTime);
		AAA_LIMIT(pstRoute->astRouteNode[i].u32SysGain, AeInfo[sID].u32TotalGainMin,
			AeInfo[sID].u32TotalGainMax);
	}
}


void AE_CheckRouteExRange(CVI_U8 sID, AE_WDR_FRAME wdrFrm, ISP_AE_ROUTE_EX_S *pstRouteEx)
{
	CVI_U8 i;
	CVI_U32 maxTime;

	maxTime = (wdrFrm == AE_SE) ? AeInfo[sID].u32WDRSEExpTimeMax : AeInfo[sID].u32ExpTimeMax;
	for (i = 0; i < pstRouteEx->u32TotalNum; ++i) {
		AAA_LIMIT(pstRouteEx->astRouteExNode[i].u32IntTime, AeInfo[sID].u32ExpTimeMin, maxTime);
		AAA_LIMIT(pstRouteEx->astRouteExNode[i].u32Again, AeInfo[sID].u32AGainMin, AeInfo[sID].u32AGainMax);
		AAA_LIMIT(pstRouteEx->astRouteExNode[i].u32Dgain, AeInfo[sID].u32DGainMin, AeInfo[sID].u32DGainMax);
		AAA_LIMIT(pstRouteEx->astRouteExNode[i].u32IspDgain, AeInfo[sID].u32ISPDGainMin,
			AeInfo[sID].u32ISPDGainMax);
	}
}

void AE_CalculateManualBvEntry(CVI_U8 sID, CVI_U32 exposure, CVI_S16 *bvEntry)
{
	CVI_S16 tvEntry, svEntry;
	CVI_U32 manualGain, manualTime;
	CVI_U32	routeExp, routeTime, routeGain, routeLine;
	CVI_U8 i, nodeNum;
	CVI_U64	manualExp;
	static CVI_U32 preRotueTime[AE_SENSOR_NUM];
	//Initializes variable of routeLine
	routeLine = 0;

	//note: exposure = expLine * gain (1x = 64 , not 1024)
	if (!pstAeExposureAttrInfo[sID]->stAuto.bManualExpValue) {
		*bvEntry = BV_AUTO_ENTRY;
		return;
	}

	nodeNum = (pstAeExposureAttrInfo[sID]->bAERouteExValid) ?
		pstAeRouteExInfo[sID][AE_LE]->u32TotalNum : pstAeRouteInfo[sID][AE_LE]->u32TotalNum;
	routeTime = AeInfo[sID].u32ExpTimeMin;
	routeGain = AeInfo[sID].u32TotalGainMin;

	for (i = 0 ; i < nodeNum; ++i) {
		if (pstAeExposureAttrInfo[sID]->bAERouteExValid) {
			routeTime = pstAeRouteExInfo[sID][AE_LE]->astRouteExNode[i].u32IntTime;
			routeGain = AE_CalTotalGain(pstAeRouteExInfo[sID][AE_LE]->astRouteExNode[i].u32Again,
				pstAeRouteExInfo[sID][AE_LE]->astRouteExNode[i].u32Dgain,
				pstAeRouteExInfo[sID][AE_LE]->astRouteExNode[i].u32IspDgain);
		} else {
			routeTime = pstAeRouteInfo[sID][AE_LE]->astRouteNode[i].u32IntTime;
			routeGain = pstAeRouteInfo[sID][AE_LE]->astRouteNode[i].u32SysGain;
		}
		routeLine = routeTime / AAA_DIV_0_TO_1(AeInfo[sID].fExpLineTime);
		routeExp = routeLine * (routeGain >> 4);
		if (routeExp >= exposure) {
			break;
		}
		preRotueTime[sID] = routeTime;
	}

	if (i == 0 || i == nodeNum) {
		manualTime = routeTime;
		manualGain = routeGain;
	} else {
		if (routeTime == preRotueTime[sID]) {
			manualTime = routeTime;
			manualExp = ((CVI_U64)exposure) << 4;
			manualGain = manualExp / AAA_DIV_0_TO_1(routeLine);
		} else {
			manualGain = routeGain;
			routeGain = (routeGain >> 4);
			manualTime = (exposure / AAA_DIV_0_TO_1(routeGain)) * AeInfo[sID].fExpLineTime;
		}
	}
	AE_GetTvEntryByTime(sID, manualTime, &tvEntry);
	AE_GetISOEntryByGain(manualGain, &svEntry);
	*bvEntry = tvEntry - svEntry;
}

void AE_UpdateParameter(VI_PIPE ViPipe)
{
	CVI_U32 minGain, maxGain, weightSum = 0;
	CVI_U32 minTime, maxTime, tmpMinTime, tmpMaxTime;
	CVI_U32	minISONum, maxISONum, tmpMinISONum, tmpMaxISONum;
	CVI_U8	maxNodeNum, meterPeriod;
	CVI_U8 sID;
	CVI_U8 i, j;

	sID = AE_ViPipe2sID(ViPipe);

	if (AeInfo[sID].bParameterUpdate[AE_EXPOSURE_ATTR_UPDATE] ||
		AeInfo[sID].bParameterUpdate[AE_UPDATE_ALL]) {
		*pstAeExposureAttrInfo[sID] = *pstAeMpiExposureAttr[sID];

		AE_SetDebugMode(sID, pstAeExposureAttrInfo[sID]->u8DebugMode);

		if (pstAeMpiExposureAttr[sID]->enOpType == OP_TYPE_MANUAL)
			AE_SetManualExposure(sID); // for manual exposure control

		AE_CheckExposureParameterRange(sID, pstAeExposureAttrInfo[sID]);

		minTime = pstAeExposureAttrInfo[sID]->stAuto.stExpTimeRange.u32Min;
		maxTime = pstAeExposureAttrInfo[sID]->stAuto.stExpTimeRange.u32Max;

		AE_CalculateManualBvEntry(sID, pstAeExposureAttrInfo[sID]->stAuto.u32ExpValue,
			&AeInfo[sID].s16ManualBvEntry);

		AE_GetTvEntryByTime(sID, maxTime, &AeInfo[sID].s16MinRangeTVEntry);
		AE_GetTvEntryByTime(sID, minTime, &AeInfo[sID].s16MaxRangeTVEntry);

		AE_GetExpTimeByEntry(sID, AeInfo[sID].s16MinRangeTVEntry, &tmpMaxTime);
		AE_GetExpTimeByEntry(sID, AeInfo[sID].s16MaxRangeTVEntry, &tmpMinTime);

		//for rounding
		if (AeInfo[sID].s16MaxRangeTVEntry - AeInfo[sID].s16MinRangeTVEntry > 1) {
			if (tmpMaxTime > maxTime)
				AeInfo[sID].s16MinRangeTVEntry += 1;
			if (tmpMinTime < minTime)
				AeInfo[sID].s16MaxRangeTVEntry -= 1;
		}
		if (pstAeExposureAttrInfo[sID]->stAuto.enGainType == AE_TYPE_ISO) {
			minISONum = AAA_MAX(pstAeExposureAttrInfo[sID]->stAuto.stISONumRange.u32Min,
				AeInfo[sID].u32ISONumMin);
			maxISONum = AAA_MIN(pstAeExposureAttrInfo[sID]->stAuto.stISONumRange.u32Max,
				AeInfo[sID].u32ISONumMax);

			AE_GetISOEntryByISONum(maxISONum, &AeInfo[sID].s16MaxRangeISOEntry);
			AE_GetISOEntryByISONum(minISONum, &AeInfo[sID].s16MinRangeISOEntry);

			AE_GetISONumByEntry(sID, AeInfo[sID].s16MaxRangeISOEntry, &tmpMaxISONum);
			AE_GetISONumByEntry(sID, AeInfo[sID].s16MinRangeISOEntry, &tmpMinISONum);
			//for rounding
			if (AeInfo[sID].s16MaxRangeISOEntry - AeInfo[sID].s16MinRangeISOEntry > 1) {
				if (tmpMaxISONum > maxISONum)
					AeInfo[sID].s16MaxRangeISOEntry -= 1;
				if (tmpMinISONum < minISONum)
					AeInfo[sID].s16MinRangeISOEntry += 1;
			}
		} else {

			minGain = AE_CalTotalGain(pstAeExposureAttrInfo[sID]->stAuto.stAGainRange.u32Min,
				pstAeExposureAttrInfo[sID]->stAuto.stDGainRange.u32Min,
				pstAeExposureAttrInfo[sID]->stAuto.stISPDGainRange.u32Min);
			maxGain = AE_CalTotalGain(pstAeExposureAttrInfo[sID]->stAuto.stAGainRange.u32Max,
				pstAeExposureAttrInfo[sID]->stAuto.stDGainRange.u32Max,
				pstAeExposureAttrInfo[sID]->stAuto.stISPDGainRange.u32Max);

			minGain = AAA_MAX(minGain, pstAeExposureAttrInfo[sID]->stAuto.stSysGainRange.u32Min);
			maxGain = AAA_MIN(maxGain, pstAeExposureAttrInfo[sID]->stAuto.stSysGainRange.u32Max);


			minGain = AAA_MAX(minGain, AeInfo[sID].u32TotalGainMin);
			maxGain = AAA_MIN(maxGain, AeInfo[sID].u32TotalGainMax);

			AE_GetISOEntryByGain(maxGain, &AeInfo[sID].s16MaxRangeISOEntry);
			AE_GetISOEntryByGain(minGain, &AeInfo[sID].s16MinRangeISOEntry);
		}

		AE_GetISOEntryByGain(pstAeExposureAttrInfo[sID]->stAuto.u32GainThreshold,
			&AeInfo[sID].s16SlowShutterISOEntry);

		AeInfo[sID].u8AERunInterval = AAA_MAX(pstAeExposureAttrInfo[sID]->u8AERunInterval,
			AeInfo[sID].u8SensorRunInterval);

		meterPeriod = AeInfo[sID].u8SensorPeriod / AAA_DIV_0_TO_1(AeInfo[sID].u8AERunInterval);
		AeInfo[sID].u8MeterFramePeriod = AAA_MAX(1, meterPeriod);
		AeInfo[sID].bEnableSmoothAE = (AeInfo[sID].u8MeterFramePeriod <= 1) ? 0 : 1;

		if (AeInfo[sID].enWDRGainMode == SNS_GAIN_MODE_SHARE ||
		((AeInfo[sID].enWDRGainMode == SNS_GAIN_MODE_WDR_2F ||
		AeInfo[sID].enWDRGainMode == SNS_GAIN_MODE_WDR_3F) &&
		!pstAeExposureAttrInfo[sID]->bAEGainSepCfg))
			AeInfo[sID].bWDRUseSameGain = 1;
		else
			AeInfo[sID].bWDRUseSameGain = 0;

		AeInfo[sID].bParameterUpdate[AE_EXPOSURE_ATTR_UPDATE] = 0;
	}

	if (AeInfo[sID].bParameterUpdate[AE_WDR_EXPOSURE_ATTR_UPDATE] ||
		AeInfo[sID].bParameterUpdate[AE_UPDATE_ALL]) {
		*pstAeWDRExposureAttrInfo[sID] = *pstAeMpiWDRExposureAttr[sID];

		AeInfo[sID].bParameterUpdate[AE_WDR_EXPOSURE_ATTR_UPDATE] = 0;
	}

	if (AeInfo[sID].bParameterUpdate[AE_ROUTE_UPDATE] ||
		AeInfo[sID].bParameterUpdate[AE_ROUTE_EX_UPDATE] ||
		AeInfo[sID].bParameterUpdate[AE_ROUTE_SF_UPDATE] ||
		AeInfo[sID].bParameterUpdate[AE_ROUTE_SF_EX_UPDATE] ||
		AeInfo[sID].bParameterUpdate[AE_UPDATE_ALL]) {
		if (AeInfo[sID].bParameterUpdate[AE_ROUTE_EX_UPDATE] ||
			AeInfo[sID].bParameterUpdate[AE_UPDATE_ALL]) {
			*pstAeRouteExInfo[sID][AE_LE] = *pstAeMpiRouteEx[sID][AE_LE];
			AE_CheckRouteExRange(sID, AE_LE, pstAeRouteExInfo[sID][AE_LE]);

			maxNodeNum = pstAeRouteExInfo[sID][AE_LE]->u32TotalNum - 1;
			minTime = pstAeRouteExInfo[sID][AE_LE]->astRouteExNode[0].u32IntTime;
			maxTime = pstAeRouteExInfo[sID][AE_LE]->astRouteExNode[maxNodeNum].u32IntTime;
			minGain = AE_CalTotalGain(pstAeRouteExInfo[sID][AE_LE]->astRouteExNode[0].u32Again,
						pstAeRouteExInfo[sID][AE_LE]->astRouteExNode[0].u32Dgain,
						pstAeRouteExInfo[sID][AE_LE]->astRouteExNode[0].u32IspDgain);
			maxGain = AE_CalTotalGain(pstAeRouteExInfo[sID][AE_LE]->astRouteExNode[maxNodeNum].u32Again,
						pstAeRouteExInfo[sID][AE_LE]->astRouteExNode[maxNodeNum].u32Dgain,
						pstAeRouteExInfo[sID][AE_LE]->astRouteExNode[maxNodeNum].u32IspDgain);

			AE_GetTvEntryByTime(sID, minTime, &AeInfo[sID].s16MaxRouteExTVEntry);
			AE_GetTvEntryByTime(sID, maxTime, &AeInfo[sID].s16MinRouteExTVEntry);
			AE_GetISOEntryByGain(minGain, &AeInfo[sID].s16MinRouteExISOEntry);
			AE_GetISOEntryByGain(maxGain, &AeInfo[sID].s16MaxRouteExISOEntry);

			AeInfo[sID].bParameterUpdate[AE_ROUTE_EX_UPDATE] = 0;
		}
		if (AeInfo[sID].bParameterUpdate[AE_ROUTE_UPDATE] ||
			AeInfo[sID].bParameterUpdate[AE_UPDATE_ALL]) {
			*pstAeRouteInfo[sID][AE_LE] = *pstAeMpiRoute[sID][AE_LE];
			AE_CheckRouteRange(sID, AE_LE, pstAeRouteInfo[sID][AE_LE]);
			maxNodeNum = pstAeRouteInfo[sID][AE_LE]->u32TotalNum - 1;
			minTime = pstAeRouteInfo[sID][AE_LE]->astRouteNode[0].u32IntTime;
			maxTime = pstAeRouteInfo[sID][AE_LE]->astRouteNode[maxNodeNum].u32IntTime;
			minGain = pstAeRouteInfo[sID][AE_LE]->astRouteNode[0].u32SysGain;
			maxGain = pstAeRouteInfo[sID][AE_LE]->astRouteNode[maxNodeNum].u32SysGain;

			AE_GetTvEntryByTime(sID, minTime, &AeInfo[sID].s16MaxRouteTVEntry);
			AE_GetTvEntryByTime(sID, maxTime, &AeInfo[sID].s16MinRouteTVEntry);
			AE_GetISOEntryByGain(minGain, &AeInfo[sID].s16MinRouteISOEntry);
			AE_GetISOEntryByGain(maxGain, &AeInfo[sID].s16MaxRouteISOEntry);

			AeInfo[sID].bParameterUpdate[AE_ROUTE_UPDATE] = 0;
		}
		if (AeInfo[sID].bWDRMode && (AeInfo[sID].bParameterUpdate[AE_ROUTE_SF_EX_UPDATE] ||
			AeInfo[sID].bParameterUpdate[AE_UPDATE_ALL])) {
			*pstAeRouteExInfo[sID][AE_SE] = *pstAeMpiRouteEx[sID][AE_SE];
			AE_CheckRouteExRange(sID, AE_SE, pstAeRouteExInfo[sID][AE_SE]);
			AeInfo[sID].bParameterUpdate[AE_ROUTE_SF_EX_UPDATE] = 0;
		}
		if (AeInfo[sID].bWDRMode && (AeInfo[sID].bParameterUpdate[AE_ROUTE_SF_UPDATE] ||
			AeInfo[sID].bParameterUpdate[AE_UPDATE_ALL])) {
			*pstAeRouteInfo[sID][AE_SE] = *pstAeMpiRoute[sID][AE_SE];
			AE_CheckRouteRange(sID, AE_SE, pstAeRouteInfo[sID][AE_SE]);
			AeInfo[sID].bParameterUpdate[AE_ROUTE_SF_UPDATE] = 0;
		}
		AE_CalculateManualBvEntry(sID, pstAeExposureAttrInfo[sID]->stAuto.u32ExpValue,
			&AeInfo[sID].s16ManualBvEntry);
	}

	if (AeInfo[sID].bParameterUpdate[AE_STATISTICS_CONFIG_UPDATE] ||
		AeInfo[sID].bParameterUpdate[AE_UPDATE_ALL]) {

		*pstAeStatisticsCfgInfo[sID] = *pstAeStatisticsCfg[sID];
		for (i = 0; i < AE_WEIGHT_ZONE_ROW; ++i) {
			for (j = 0; j < AE_WEIGHT_ZONE_COLUMN; ++j) {
				weightSum += pstAeStatisticsCfgInfo[sID]->au8Weight[i][j];
			}
		}
		if (weightSum == 0)
			memset(pstAeStatisticsCfgInfo[sID]->au8Weight, 1,
				AE_WEIGHT_ZONE_ROW * AE_WEIGHT_ZONE_COLUMN * sizeof(CVI_U8));
		AeInfo[sID].bParameterUpdate[AE_STATISTICS_CONFIG_UPDATE] = 0;
		AeInfo[sID].bRegWeightUpdate = 1;
		AE_CalculateWeightSum(sID);
	}

	if (AeInfo[sID].bParameterUpdate[AE_SMART_EXPOSURE_ATTR_UPDATE] ||
		AeInfo[sID].bParameterUpdate[AE_UPDATE_ALL]) {
		*pstAeSmartExposureAttrInfo[sID] = *pstAeMpiSmartExposureAttr[sID];

		AeInfo[sID].bParameterUpdate[AE_SMART_EXPOSURE_ATTR_UPDATE] = 0;
	}

	if (AeInfo[sID].bParameterUpdate[AE_IRIS_ATTR_UPDATE] ||
		AeInfo[sID].bParameterUpdate[AE_DC_IRIS_ATTR_UPDATE] ||
		AeInfo[sID].bParameterUpdate[AE_UPDATE_ALL]) {
		if (AeInfo[sID].bParameterUpdate[AE_IRIS_ATTR_UPDATE] ||
			AeInfo[sID].bParameterUpdate[AE_UPDATE_ALL]) {
			AE_UpdateIrisAttr(sID);
			AeInfo[sID].bParameterUpdate[AE_IRIS_ATTR_UPDATE] = 0;
		}
		if (AeInfo[sID].bParameterUpdate[AE_DC_IRIS_ATTR_UPDATE] ||
			AeInfo[sID].bParameterUpdate[AE_UPDATE_ALL]) {
			AE_UpdateDcIrisAttr(ViPipe);
			AeInfo[sID].bParameterUpdate[AE_DC_IRIS_ATTR_UPDATE] = 0;
		}
	}

	AeInfo[sID].bParameterUpdate[AE_UPDATE_ALL] = 0;
}

static void AE_RunAeDebugMode(CVI_U8 sID)
{
	switch (AE_GetDebugMode(sID)) {
	case 255:
		AE_DumpLog();
		AE_SetDebugMode(sID, 0);
		break;
	case 254:
		AE_DumpBootLog(sID);
		AE_SetDebugMode(sID, 0);
		break;
	case AE_DEBUG_ENABLE_LOG_TO_FILE:
		AE_MemoryAlloc(sID, AE_LOG_ITEM);
		break;
	default:
		break;
	}
}

CVI_S32 aeRun(VI_PIPE ViPipe, const ISP_AE_INFO_S *pstAeInfo, ISP_AE_RESULT_S *pstAeResult, CVI_S32 s32Rsv)
{
	AE_CTX_S *pstAeCtx;
	static struct timeval firstAeTime[AE_SENSOR_NUM], curAeTime[AE_SENSOR_NUM], preAeTime[AE_SENSOR_NUM],
		proAeTime[AE_SENSOR_NUM];
	static CVI_BOOL isFirstAe[AE_SENSOR_NUM];
	static CVI_U8 debugFrmCnt[AE_SENSOR_NUM];
	CVI_U32 periodT[AE_SENSOR_NUM], processT[AE_SENSOR_NUM];
	CVI_U8 sID = AE_ViPipe2sID(ViPipe);
	CVI_U16 aePeriodN = 0;
	ISP_PUB_ATTR_S pubAttr;
	CVI_U32 curFps;
	static CVI_U32 preFps[AE_SENSOR_NUM];

	pstAeCtx = AE_GET_CTX(ViPipe);
	ViPipe = pstAeCtx->IspBindDev;

	AeInfo[sID].u32frmCnt = pstAeInfo->u32FrameCnt;

	if (!isFirstAe[sID]) {
		isFirstAe[sID] = 1;
		#ifndef ARCH_RTOS_CV181X // TODO@CV181X
		gettimeofday(&firstAeTime[sID], NULL);
		#endif
		preAeTime[sID].tv_sec = preAeTime[sID].tv_usec = 0;
	}

	#ifndef ARCH_RTOS_CV181X // TODO@CV181X
	gettimeofday(&curAeTime[sID], NULL);
	#endif
	periodT[sID] = AE_GetMSTimeDiff(&preAeTime[sID], &curAeTime[sID]);
	preAeTime[sID] = curAeTime[sID];
	AeInfo[sID].u16FramePeriodTime = periodT[sID];

	CVI_ISP_GetPubAttr(ViPipe, &pubAttr);
	AeInfo[sID].fIspPubAttrFps = pubAttr.f32FrameRate;

	AE_CheckParamUpdateFlag(ViPipe);
	AE_UpdateParameter(ViPipe);

	AE_RunAeDebugMode(sID);

	AE_GetStatisticsData(ViPipe, pstAeInfo);

	aePeriodN = AAA_MAX(AeInfo[sID].u8AERunInterval, AeInfo[sID].u8SensorPeriod);

	AE_DoFlickDetect(sID, AeInfo[sID].stApex[AE_LE].s16TVEntry);

	#if CHECK_AE_SIM
		AE_HLprintf("======= i:%d TV:%d SV:%d =======\n",
		AeInfo[sID].u32frmCnt, AeInfo[sID].stApex[0].s16TVEntry, AeInfo[sID].stApex[0].s16SVEntry);
	#endif

	if (AE_IsAeSimMode()) {
		AeInfo[sID].bEnableSmoothAE = 0;
		AeInfo[sID].bMeterEveryFrame = 0;
		if (sAe_sim.genRawCntLast == sAe_sim.genRawCnt) {
			sAe_sim.waitCnt = 0;
		} else {
			sAe_sim.waitCnt++;
			//sAe_sim.genRawCntLast = sAe_sim.genRawCnt;
		}
		//printf("CNT %d %d %d\n", sAe_sim.genRawCntLast, sAe_sim.genRawCnt, sAe_sim.waitCnt);

		if (sAe_sim.waitCnt > aePeriodN) {
			sAe_sim.genRawCntLast = sAe_sim.genRawCnt;
			AeInfo[sID].u32frmCnt = (sAe_sim.genRawCnt/aePeriodN)*aePeriodN;//do AE
		} else {
			AE_SetExpToResult(sID, pstAeResult, AeInfo[sID].stExp);
			return CVI_SUCCESS;
		}
	}

	if (AeInfo[sID].bMeterEveryFrame) {
		if (pstAeExposureAttrInfo[sID]->bByPass ||
			AeInfo[sID].u32frmCnt % AeInfo[sID].u8AERunInterval != 0 ||
			AeInfo[sID].u32frmCnt <= AeInfo[sID].u8BootWaitFrameNum) {
			AE_SetExpToResult(sID, pstAeResult, AeInfo[sID].stExp);
			return CVI_SUCCESS;
		}
	} else {
		if (pstAeExposureAttrInfo[sID]->bByPass || AeInfo[sID].u32frmCnt % aePeriodN != 0 ||
			AeInfo[sID].u32frmCnt < AeInfo[sID].u8BootWaitFrameNum + AeInfo[sID].u8SensorPeriod) {
			if (!pstAeExposureAttrInfo[sID]->bByPass) {
				if (AeInfo[sID].bEnableSmoothAE) {
					if (pstAeExposureAttrInfo[sID]->enOpType != OP_TYPE_MANUAL) {
						AE_GetSmoothTvAvSvEntry(sID, AeInfo[sID].u32frmCnt);
						AE_ConfigExposure(sID);
					}
					AE_SetExposure(sID, AeInfo[sID].stExp);
				}
			}
			AE_SetExpToResult(sID, pstAeResult, AeInfo[sID].stExp);
			AE_SaveSmoothExpSettingLog(sID);
			return CVI_SUCCESS;
		}
	}

	//write here for route max time update
	curFps = AeInfo[sID].fIspPubAttrFps * FLOAT_TO_INT_NUMBER;
	AeInfo[sID].bPubFpsChange = (curFps != preFps[sID]) ? 1 : 0;
	preFps[sID] = curFps;
	AE_SetFaceInfo(sID, &pstAeInfo->stSmartInfo);
	AE_RunAlgo(sID);
	AE_SetExposure(sID, AeInfo[sID].stExp);
#ifdef ENABLE_DUMP_BOOT_LOG
	AE_SaveBootLog(sID);
#endif

	if (AE_isRawReplayMode(sID)) {
		SetReplayExposure(sID);
	}
	AE_SetExpToResult(sID, pstAeResult, AeInfo[sID].stExp);
	AE_SetExposureInfo(sID);
	AE_SaveLog(sID, pstAeResult);
	AE_ShowDebugInfo(sID);
	AeInfo[sID].u32MeterFrameCnt++;
	if (AeInfo[sID].bIsStable[AE_LE] && !AeInfo[sID].u32FirstStableTime)
		AeInfo[sID].u32FirstStableTime = AE_GetUSTimeDiff(&firstAeTime[sID], &curAeTime[sID]);

	if (AE_IsAeSimMode())
		AE_SetAeSimEv(sID);

	#ifndef ARCH_RTOS_CV181X // TODO@CV181X
	gettimeofday(&proAeTime[sID], NULL);
	#endif
	processT[sID] = AE_GetMSTimeDiff(&curAeTime[sID], &proAeTime[sID]);

	if (AE_GetDebugMode(sID) == 1) {
		printf("Func : %s\n", __func__);
		printf("sId:%u Fc:%u PN:%u V:%u H:%u\n", sID, AeInfo[sID].u32frmCnt,
		       pstAeExposureAttrInfo[sID]->u8AERunInterval, AeInfo[sID].u8GridVNum, AeInfo[sID].u8GridHNum);
		printf("AE CTv:%d %d PTv:%d %d\n", (CVI_S32)curAeTime[sID].tv_sec, (CVI_S32)curAeTime[sID].tv_usec,
			(CVI_S32)preAeTime[sID].tv_sec, (CVI_S32)preAeTime[sID].tv_usec);
		printf("PeriodT:%u fps:%d ProT:%u\n", periodT[sID], (CVI_U16)AeInfo[sID].fFps, processT[sID]);
		printf("aeL:%u time:%u gain:%u %u s32Rsv:%d\n", AeInfo[sID].u16FrameLuma[AE_LE],
		       AeInfo[sID].stExp[AE_LE].u32ExpTime, AeInfo[sID].stExp[AE_LE].stExpGain.u32AGain,
		       AeInfo[sID].stExp[AE_LE].stExpGain.u32DGain, s32Rsv);
		printf("stEn:%d stMain:%d grp:%d cbs:%d cbsS:%d cbsC:%d stCal:%d LR:%d RR:%d BR:%d\n",
			AeInfo[sID].stStitchAttr.enable, AeInfo[sID].stStitchAttr.bMainPipe,
			AeInfo[sID].stStitchAttr.u8Group, AeInfo[sID].stStitchAttr.bCombineSts,
			AeInfo[sID].stStitchAttr.u8CombChnSum, AeInfo[sID].stStitchAttr.u8CombChn,
			AeInfo[sID].stStitchAttr.bCalibEnable, AeInfo[sID].stStitchAttr.u32CalibLumaRatio,
			AeInfo[sID].stStitchAttr.u32CalibRGainRatio, AeInfo[sID].stStitchAttr.u32CalibBGainRatio);
		debugFrmCnt[sID]++;
		if (debugFrmCnt[sID] > 5) {
			debugFrmCnt[sID] = 0;
			AE_SetDebugMode(sID, 0);
		}
	}
	return CVI_SUCCESS;
}

CVI_S32 aeCtrl(VI_PIPE ViPipe, CVI_U32 u32Cmd, CVI_VOID *pValue)
{
	CVI_U8 sID = AE_ViPipe2sID(ViPipe);

	if (pValue == CVI_NULL) {
		return CVI_FAILURE;
	}

	switch (u32Cmd) {
	default:
		break;
	}

	if (AE_GetDebugMode(sID) == 21) {
		printf("Func : %s ViPipe:%d cmd:%d value:%d\n", __func__, ViPipe, u32Cmd, *(CVI_S32 *)pValue);
	}

	return CVI_SUCCESS;
}

CVI_S32 aeExit(VI_PIPE ViPipe)
{
	CVI_U8 sID = AE_ViPipe2sID(ViPipe);

	ISP_LOG_NOTICE("Func : %s ViPipe:%d\n", __func__, sID);
	AE_MemoryFree(sID, AE_BOOT_ITEM);
	AE_MemoryFree(sID, AE_LOG_ITEM);
	AE_MemoryFree(sID, AE_TOOL_PARAMETER_ITEM);

	if (u8MallocCnt[sID])
		ISP_LOG_ERR("vi:%d %s:%d\n", ViPipe, "AE Malloc cnt not equal to free!", u8MallocCnt[sID]);

	u8SensorNum = 1;
	AeInfo[sID].u32frmCnt = 0;
	AeInfo[sID].pu16AeStatistics = CVI_NULL;
	AeInfo[sID].pu32Histogram = CVI_NULL;
	return CVI_SUCCESS;
}

void AE_GetIspDgainCompensationRange(CVI_U8 sID, AE_EXPOSURE *pStExp, CVI_U32 *minRange, CVI_U32 *maxRange)
{
	CVI_U32	minISONum, maxISONum;
	CVI_U32	isoMinGain, isoMaxGain, sensorGain, manualISONum;
	CVI_U32	tmpMinRange, tmpMaxRange;

	if ((pstAeExposureAttrInfo[sID]->enOpType == OP_TYPE_MANUAL &&
		pstAeExposureAttrInfo[sID]->stManual.enISONumOpType == OP_TYPE_MANUAL &&
		pstAeExposureAttrInfo[sID]->stManual.enGainType == AE_TYPE_ISO) ||
		(pstAeExposureAttrInfo[sID]->enOpType == OP_TYPE_AUTO &&
		pstAeExposureAttrInfo[sID]->stAuto.enGainType == AE_TYPE_ISO)) {
		minISONum = AAA_MAX(AeInfo[sID].u32ISONumMin,
			pstAeExposureAttrInfo[sID]->stAuto.stISONumRange.u32Min);
		maxISONum = AAA_MIN(AeInfo[sID].u32ISONumMax,
			pstAeExposureAttrInfo[sID]->stAuto.stISONumRange.u32Max);
		AE_GetGainByISONum(minISONum, &isoMinGain);
		if (pstAeExposureAttrInfo[sID]->enOpType == OP_TYPE_MANUAL) {
			manualISONum = AAA_MIN(pstAeExposureAttrInfo[sID]->stManual.u32ISONum,
				maxISONum);
			maxISONum = manualISONum;
		}
		AE_GetGainByISONum(maxISONum, &isoMaxGain);
		sensorGain = AE_CalTotalGain(pStExp[AE_LE].stSensorExpGain.u32AGain,
			pStExp[AE_LE].stSensorExpGain.u32DGain,
			AE_GAIN_BASE);
		tmpMinRange = (CVI_U64)isoMinGain * AE_GAIN_BASE / AAA_DIV_0_TO_1(sensorGain);
		tmpMaxRange = (CVI_U64)isoMaxGain * AE_GAIN_BASE / AAA_DIV_0_TO_1(sensorGain);
	} else {
		tmpMinRange = AAA_MAX(AeInfo[sID].u32ISPDGainMin,
			pstAeExposureAttrInfo[sID]->stAuto.stISPDGainRange.u32Min);
		tmpMaxRange = AAA_MIN(pstAeExposureAttrInfo[sID]->stAuto.stSysGainRange.u32Max,
			pstAeExposureAttrInfo[sID]->stAuto.stISPDGainRange.u32Max);
		tmpMaxRange = AAA_MIN(tmpMaxRange, AeInfo[sID].u32ISPDGainMax);
	}
	AAA_LIMIT(tmpMinRange, AeInfo[sID].u32ISPDGainMin, AeInfo[sID].u32ISPDGainMax);
	AAA_LIMIT(tmpMaxRange, AeInfo[sID].u32ISPDGainMin, AeInfo[sID].u32ISPDGainMax);

	*minRange = tmpMinRange;
	*maxRange = tmpMaxRange;
}

void AE_CalculateISPDGainCompensate(CVI_U8 sID, AE_EXPOSURE *pStExp)
{
	CVI_BOOL manualIspDgainMode = 0;
	CVI_U32 minIspDgain, maxIspDgain;
	CVI_S16 i;

	static CVI_U32	ispDgainCompTimeThr;

	if (!ispDgainCompTimeThr) {
		AE_GetExpTimeByEntry(sID, TV_ENTRY_WITH_ISPDGAIN_COMPENSATION, &ispDgainCompTimeThr);
	}

	AE_GetIspDgainCompensationRange(sID, pStExp, &minIspDgain, &maxIspDgain);

	if (pstAeExposureAttrInfo[sID]->enOpType == OP_TYPE_MANUAL &&
		pstAeExposureAttrInfo[sID]->stManual.enISPDGainOpType == OP_TYPE_MANUAL &&
		pstAeExposureAttrInfo[sID]->stManual.enGainType == AE_TYPE_GAIN)
		manualIspDgainMode = 1;

	if (AeInfo[sID].bEnableISPDgainCompensation && !AeInfo[sID].u32ManualWDRSEISPDgain &&
		!manualIspDgainMode && !AE_isRawReplayMode(sID)) {
		for (i = 0; i < AeInfo[sID].u8AeMaxFrameNum; ++i) {
			if (pStExp[i].u32ExpTime < ispDgainCompTimeThr &&
				pStExp[i].fIdealExpTimeLine > pStExp[i].u32SensorExpTimeLine) {
				pStExp[i].stSensorExpGain.u32ISPDGain = pStExp[i].stSensorExpGain.u32ISPDGain *
					pStExp[i].fIdealExpTimeLine /
					AAA_DIV_0_TO_1(pStExp[i].u32SensorExpTimeLine);
			}

			if (pStExp[i].stSensorExpGain.u32AGain != pStExp[i].stExpGain.u32AGain) {
				pStExp[i].stSensorExpGain.u32ISPDGain = (pStExp[i].stSensorExpGain.u32ISPDGain *
					pStExp[i].stExpGain.u32AGain) /
					AAA_DIV_0_TO_1(pStExp[i].stSensorExpGain.u32AGain);
			}

			if (pStExp[i].stSensorExpGain.u32DGain != pStExp[i].stExpGain.u32DGain) {
				pStExp[i].stSensorExpGain.u32ISPDGain = (pStExp[i].stSensorExpGain.u32ISPDGain *
					pStExp[i].stExpGain.u32DGain) /
					AAA_DIV_0_TO_1(pStExp[i].stSensorExpGain.u32DGain);
			}
			AAA_LIMIT(pStExp[i].stSensorExpGain.u32ISPDGain, minIspDgain, maxIspDgain);
		}
	}

	if (AE_GetDebugMode(sID) == 95) {
		printf("AG:%u %u DG:%u %u IG:%u->%u R:%u-%u\n", pStExp[0].stExpGain.u32AGain,
			pStExp[0].stSensorExpGain.u32AGain, pStExp[0].stExpGain.u32DGain,
		pStExp[0].stSensorExpGain.u32DGain, pStExp[0].stExpGain.u32ISPDGain,
		pStExp[0].stSensorExpGain.u32ISPDGain, minIspDgain, maxIspDgain);
	}
}


CVI_S32 AE_SetExposure(CVI_U8 sID, AE_EXPOSURE *pStExp)
{
	AE_CTX_S *pstAeCtx;
	CVI_S16 i;
	CVI_U32 time[AE_MAX_WDR_FRAME_NUM] = { 0 }, again[AE_MAX_WDR_FRAME_NUM] = { 0 },
		dgain[AE_MAX_WDR_FRAME_NUM] = { 0 };

	VI_PIPE ViPipe;
	CVI_U32 againDbResult[AE_MAX_WDR_FRAME_NUM] = {1024, 1024},
		dgainDbResult[AE_MAX_WDR_FRAME_NUM] = {1024, 1024};

	CVI_U16 manual = 1;
	CVI_U32 ratio[3] = { 1024, 64, 64 };
	CVI_U32 IntTimeMax[4], IntTimeMin[4], LFMaxIntTime[4];
	CVI_U32 WDRIdealRatio, WDRRealRatio = 0;
	CVI_U32 sensorAGain, sensorDGain;
	CVI_BOOL	bSensorPortingCheck = 0;

	static CVI_U16 debugCnt[AE_SENSOR_NUM];
	static CVI_BOOL lscStatus[AE_SENSOR_NUM];
	/*inttime max used parameter.*/

#ifndef AAA_PC_PLATFORM

	sID = AE_CheckSensorID(sID);
	pstAeCtx = AE_GET_CTX(sID);
	ViPipe = AE_sID2ViPipe(sID);


	for (i = 0; i < AeInfo[sID].u8AeMaxFrameNum; ++i) {
		if (!pStExp[i].fIdealExpTime)
			pStExp[i].fIdealExpTime = pStExp[i].u32ExpTime;
		//AE_GetExposureTimeLine(sID, pStExp[i].u32ExpTime, &pStExp[i].u32ExpTimeLine);
		AE_GetExposureTimeIdealLine(sID, pStExp[i].fIdealExpTime, &pStExp[i].fIdealExpTimeLine);
		pStExp[i].u32ExpTimeLine = (CVI_U32)pStExp[i].fIdealExpTimeLine;
		pStExp[i].u32SensorExpTimeLine = pStExp[i].u32ExpTimeLine;
		pStExp[i].u32SensorExpTime = pStExp[i].u32ExpTime;
		pStExp[i].stSensorExpGain = pStExp[i].stExpGain;
	}

	if (AeInfo[sID].bWDRMode) {
		//0: WDR SE 1: WDR LE, same with Hisi
		time[0] = pStExp[AE_SE].u32ExpTimeLine;
		time[1] = pStExp[AE_LE].u32ExpTimeLine;
	} else {
		time[0] = pStExp[AE_LE].u32ExpTimeLine;
	}

	if (AeInfo[sID].bWDRMode) {
		WDRIdealRatio = (pstAeWDRExposureAttrInfo[sID]->enExpRatioType == OP_TYPE_MANUAL) ?
					pstAeWDRExposureAttrInfo[sID]->au32ExpRatio[0] :
					pStExp[AE_LE].u32ExpTimeLine * AE_WDR_RATIO_BASE /
						AAA_DIV_0_TO_1(pStExp[AE_SE].u32ExpTimeLine);

		AAA_LIMIT(WDRIdealRatio, MIN_WDR_RATIO, MAX_WDR_RATIO);
		ratio[0] = WDRIdealRatio;

		if (!AeInfo[sID].u32ManualWDRSETime) {
			time[0] = (time[1] * AE_WDR_RATIO_BASE) / AAA_DIV_0_TO_1(WDRIdealRatio); //64 = 1x

			if (pstAeCtx->stSnsRegister.stAeExp.pfn_cmos_get_inttime_max)
				pstAeCtx->stSnsRegister.stAeExp.pfn_cmos_get_inttime_max(ViPipe, manual,
					ratio, IntTimeMax, IntTimeMin, LFMaxIntTime);

			AAA_LIMIT(time[0], IntTimeMin[0], IntTimeMax[0]);
			AAA_LIMIT(time[1], IntTimeMin[1], IntTimeMax[1]);
		}
	}

	if (AE_GetDebugMode(sID) >= SENSOR_PORTING_START_ITEM &&
		// +1 for sensor test check work frame
		AE_GetDebugMode(sID) <= SENSOR_PORTING_END_ITEM + 1) {
		if (debugCnt[sID] == 0) {
			AE_SetManual(sID, OP_TYPE_MANUAL, u8AeDebugPrintMode[sID]);
		}
		AE_ShutterGainTest(sID, pStExp);
		if (AeInfo[sID].bWDRMode) {
			time[0] = pStExp[AE_SE].u32SensorExpTimeLine;
			time[1] = pStExp[AE_LE].u32SensorExpTimeLine;
		} else {
			time[0] = pStExp[AE_LE].u32SensorExpTimeLine;
		}
		bSensorPortingCheck = 1;
	}


	for (i = 0; i < AeInfo[sID].u8AeMaxFrameNum; ++i) {
		sensorAGain = pStExp[i].stSensorExpGain.u32AGain;
		sensorDGain = pStExp[i].stSensorExpGain.u32DGain;

		if (pstAeCtx->stSnsRegister.stAeExp.pfn_cmos_again_calc_table)
			pstAeCtx->stSnsRegister.stAeExp.pfn_cmos_again_calc_table(ViPipe, &sensorAGain,
				&againDbResult[i]);

		if (pstAeCtx->stSnsRegister.stAeExp.pfn_cmos_dgain_calc_table)
			pstAeCtx->stSnsRegister.stAeExp.pfn_cmos_dgain_calc_table(ViPipe, &sensorDGain,
				&dgainDbResult[i]);

		pStExp[i].stSensorExpGain.u32AGain = sensorAGain;
		pStExp[i].stSensorExpGain.u32DGain = sensorDGain;
	}

	if (AeInfo[sID].bWDRMode) {
		//0: WDR SE 1: WDR LE, same with Hisi
		again[0] = againDbResult[AE_SE];
		again[1] = againDbResult[AE_LE];

		dgain[0] = dgainDbResult[AE_SE];
		dgain[1] = dgainDbResult[AE_LE];
	} else {
		again[0] = againDbResult[AE_LE];
		dgain[0] = dgainDbResult[AE_LE];
	}

	if (pstAeCtx->stSnsRegister.stAeExp.pfn_cmos_inttime_update)
		pstAeCtx->stSnsRegister.stAeExp.pfn_cmos_inttime_update(ViPipe, time);

	if (pstAeCtx->stSnsRegister.stAeExp.pfn_cmos_gains_update)
		pstAeCtx->stSnsRegister.stAeExp.pfn_cmos_gains_update(ViPipe, again, dgain);

	if (AeInfo[sID].bWDRMode) {
		pStExp[AE_LE].u32SensorExpTimeLine = time[1];
		pStExp[AE_LE].u32SensorExpTime = time[1] * AeInfo[sID].fExpLineTime;
		pStExp[AE_SE].u32SensorExpTimeLine = time[0];
		pStExp[AE_SE].u32SensorExpTime = time[0] * AeInfo[sID].fExpLineTime;
		WDRRealRatio = time[1] * AE_WDR_RATIO_BASE / AAA_DIV_0_TO_1(time[0]);
	} else {
		pStExp[AE_LE].u32SensorExpTimeLine = time[0];
		pStExp[AE_LE].u32SensorExpTime = time[0] * AeInfo[sID].fExpLineTime;
	}

	AE_CalculateISPDGainCompensate(sID, pStExp);

	if (AE_GetDebugMode(sID)) {
		if (bSensorPortingCheck) {
			if (debugCnt[sID] == 0) {
				AE_GetLSC(sID, &lscStatus[sID]);
				AE_SetIspDgainCompensation(sID, 0);
				AE_SetLSC(sID, 0);
			}
			for (i = 0; i < AeInfo[sID].u8AeMaxFrameNum; ++i) {
				printf("fid(%d):%u T:%d Luma:%d time:%u expLine:%u AG:%u DG:%u IG:%u\n",
					i, AeInfo[sID].u32frmCnt, AeInfo[sID].u16FramePeriodTime,
					AeInfo[sID].u16CenterG[i],
					pStExp[i].u32SensorExpTime, pStExp[i].u32SensorExpTimeLine,
					pStExp[i].stSensorExpGain.u32AGain,
					pStExp[i].stSensorExpGain.u32DGain,
					pStExp[i].stSensorExpGain.u32ISPDGain);
			}
			printf("\n");
		} else if (AE_GetDebugMode(sID) == 19) {
			printf("fid:%u\n", AeInfo[sID].u32frmCnt);
			for (i = 0; i < AeInfo[sID].u8AeMaxFrameNum; ++i) {
				printf("i:%d Tv E:%d T:%u(%u) %f(%f) ST:%u(%u)\n", i, AeInfo[sID].stApex[i].s16TVEntry,
					pStExp[i].u32ExpTime, pStExp[i].u32ExpTimeLine,
					pStExp[i].fIdealExpTime, pStExp[i].fIdealExpTimeLine,
					pStExp[i].u32SensorExpTime, pStExp[i].u32SensorExpTimeLine);
				printf("Sv E:%d AG:%u(%u) DG:%u(%u) IG:%u(%u)\n",
					AeInfo[sID].stApex[i].s16SVEntry, pStExp[i].stExpGain.u32AGain,
					pStExp[i].stSensorExpGain.u32AGain, pStExp[i].stExpGain.u32DGain,
					pStExp[i].stSensorExpGain.u32DGain, pStExp[i].stExpGain.u32ISPDGain,
					pStExp[i].stSensorExpGain.u32ISPDGain);
			}
			if (AeInfo[sID].bWDRMode) {
				printf("R:%u\n", WDRRealRatio);
					printf("LE:%u->%u(%u-%u) SE:%u->%u(%u-%u)---Diff:%d %d\n\n",
					   pStExp[AE_LE].u32ExpTimeLine, time[1], IntTimeMin[1], IntTimeMax[1],
				       pStExp[AE_SE].u32ExpTimeLine, time[0], IntTimeMin[0], IntTimeMax[0],
				       pStExp[AE_LE].u32ExpTimeLine - time[1], pStExp[AE_SE].u32ExpTimeLine - time[0]);
			}
		} else if (AE_GetDebugMode(sID) == 11 || AE_GetDebugMode(sID) == 12) {
			i = (AE_GetDebugMode(sID) == 11) ? AE_LE : AE_SE;
			printf("fid:%u CG:%u FL:%u TL:%u Bs:%d T:%u AG:%u DG:%u ISPDG:%u\n",
				AeInfo[sID].u32frmCnt,
				AeInfo[sID].u16CenterG[i], AeInfo[sID].u16FrameLuma[i],
				AeInfo[sID].u16AdjustTargetLuma[i],
				AeInfo[sID].s16BvStepEntry[i],
				AeInfo[sID].stExp[i].u32ExpTime,
				AeInfo[sID].stExp[i].stExpGain.u32AGain,
				AeInfo[sID].stExp[i].stExpGain.u32DGain,
				AeInfo[sID].stExp[i].stExpGain.u32ISPDGain);
		}
		debugCnt[sID]++;
		if (debugCnt[sID] >= 100) {
			debugCnt[sID] = 0;
			if (AE_GetDebugMode(sID) >= SENSOR_PORTING_START_ITEM &&
				AE_GetDebugMode(sID) <= SENSOR_PORTING_END_ITEM) {
				AE_SetIspDgainCompensation(sID, 1);
				AE_SetLSC(sID, lscStatus[sID]);
				AE_SetManual(sID, OP_TYPE_AUTO, 0);
			}
			AE_SetDebugMode(sID, 0);
		}
	}

#endif

	return CVI_SUCCESS;
}

void AE_CalculateDCFInfo(CVI_U8 sID, ISP_AE_RESULT_S *pstAeResult)
{
	CVI_U16 expTimeDenominator, expTimeNumerator;
	CVI_U16 evBiasDenominator, evBiasNumerator;
	CVI_U16 fnoDenominator, fnoNumerator;
	CVI_U16 maxAperDenominator, maxAperNumerator;

	expTimeDenominator = (pstAeResult->u32IntTime[AE_LE]) ?
				(1000000 / pstAeResult->u32IntTime[AE_LE]) : 1;
	expTimeNumerator = 1;

	evBiasDenominator = 10;
	evBiasNumerator = 10 * AE_CalculateLog2(pstAeExposureAttrInfo[sID]->stAuto.u16EVBias, 1024);

	fnoDenominator = 18;
	fnoNumerator = 10;

	maxAperDenominator = 18;
	maxAperNumerator = 10;

	pstAeResult->stUpdateInfo.u32ISOSpeedRatings = pstAeResult->u32Iso;
	pstAeResult->stUpdateInfo.u32ExposureTime = (expTimeNumerator << 16) + expTimeDenominator;
	pstAeResult->stUpdateInfo.u8ExposureProgram = NORMAL_PROGRAM;
	pstAeResult->stUpdateInfo.u32ExposureBiasValue = (evBiasNumerator << 16) + evBiasDenominator;
	pstAeResult->stUpdateInfo.u32FNumber = (fnoNumerator << 16) + fnoDenominator;
	pstAeResult->stUpdateInfo.u32MaxApertureValue = (maxAperNumerator << 16) + maxAperDenominator;
	if (pstAeExposureAttrInfo[sID]->enOpType == OP_TYPE_MANUAL)
		pstAeResult->stUpdateInfo.u8ExposureMode = MANUAL_MODE;
	else
		pstAeResult->stUpdateInfo.u8ExposureMode = AUTO_MODE;
}

void AE_SetExpToResult(CVI_U8 sID, ISP_AE_RESULT_S *pstAeResult, const AE_EXPOSURE *pStExp)
{
	CVI_U32 wdrAutoExpRatio;
	CVI_U16 i, tableIndex;
	CVI_U64	curFrameEv[AE_MAX_WDR_FRAME_NUM];

	static CVI_U64 preFrameEv[AE_SENSOR_NUM][AE_MAX_WDR_FRAME_NUM];

	sID = AE_CheckSensorID(sID);

	for (i = 0; i < AeInfo[sID].u8AeMaxFrameNum; ++i) {
		AeInfo[sID].u32ISONum[i] =  AE_CalculateISONum(sID, &pStExp[i].stSensorExpGain);
		AeInfo[sID].u32BLCISONum[i] = AE_CalculateBLCISONum(sID, &pStExp[i].stSensorExpGain);
	}

	pstAeResult->u32IntTime[AE_LE] = AE_LimitExpTime(sID, pStExp[AE_LE].u32SensorExpTime);
	pstAeResult->u32Again = AE_LimitAGain(sID, pStExp[AE_LE].stSensorExpGain.u32AGain);
	pstAeResult->u32Dgain = AE_LimitDGain(sID, pStExp[AE_LE].stSensorExpGain.u32DGain);
	pstAeResult->u32IspDgain = AE_LimitISPDGain(sID, pStExp[AE_LE].stSensorExpGain.u32ISPDGain);
	pstAeResult->u32Iso = AeInfo[sID].u32ISONum[AE_LE];
	pstAeResult->s16CurrentLV = AE_GetCurrentLvX100(sID);
	pstAeResult->u32AvgLuma = AeInfo[sID].u16FrameLuma[AE_LE] >> 2;
	pstAeResult->u8MeterFramePeriod = AeInfo[sID].u8SensorPeriod;
	pstAeResult->bStable = AeInfo[sID].bIsStable[AE_LE];
	pstAeResult->fBvStep = (CVI_FLOAT)AeInfo[sID].s16BvStepEntry[AE_LE] / ENTRY_PER_EV;
	pstAeResult->u32BlcIso = AeInfo[sID].u32BLCISONum[AE_LE];

	curFrameEv[AE_LE] = (CVI_U64)pStExp[AE_LE].u32SensorExpTimeLine *
		AE_CalTotalGain(pStExp[AE_LE].stSensorExpGain.u32AGain,
		pStExp[AE_LE].stSensorExpGain.u32DGain, pStExp[AE_LE].stSensorExpGain.u32ISPDGain);
	pstAeResult->fEvRatio[AE_LE] = (preFrameEv[sID][AE_LE]) ? (CVI_FLOAT)preFrameEv[sID][AE_LE] /
		AAA_DIV_0_TO_1(curFrameEv[AE_LE]) : 1;

	if (AeInfo[sID].bWDRMode) {
		pstAeResult->u32AgainSF = AE_LimitAGain(sID, pStExp[AE_SE].stSensorExpGain.u32AGain);
		pstAeResult->u32DgainSF = AE_LimitDGain(sID, pStExp[AE_SE].stSensorExpGain.u32DGain);
		pstAeResult->u32IspDgainSF = AE_LimitISPDGain(sID, pStExp[AE_SE].stSensorExpGain.u32ISPDGain);
		pstAeResult->u32IsoSF = AeInfo[sID].u32ISONum[AE_SE];
		pstAeResult->u32BlcIsoSF = AeInfo[sID].u32BLCISONum[AE_SE];
		pstAeResult->u32IntTime[AE_SE] = AE_LimitExpTime(sID, pStExp[AE_SE].u32SensorExpTime);
		AE_CalculateWDRExpRatio(pStExp, &wdrAutoExpRatio);
		pstAeResult->u32ExpRatio = (pstAeWDRExposureAttrInfo[sID]->enExpRatioType == OP_TYPE_MANUAL) ?
			pstAeWDRExposureAttrInfo[sID]->au32ExpRatio[0] : wdrAutoExpRatio;
		AeInfo[sID].u32WDRExpRatio = pstAeResult->u32ExpRatio;
		curFrameEv[AE_SE] = (CVI_U64)pStExp[AE_SE].u32SensorExpTimeLine *
			AE_CalTotalGain(pStExp[AE_SE].stSensorExpGain.u32AGain, pStExp[AE_SE].stSensorExpGain.u32DGain,
			pStExp[AE_SE].stSensorExpGain.u32ISPDGain);
		pstAeResult->fEvRatio[AE_SE] = (preFrameEv[sID][AE_SE]) ? (CVI_FLOAT)preFrameEv[sID][AE_SE] /
			AAA_DIV_0_TO_1(curFrameEv[AE_SE]) : 1;
	} else {
		pstAeResult->u32AgainSF = AE_GAIN_BASE;
		pstAeResult->u32DgainSF = AE_GAIN_BASE;
		pstAeResult->u32IspDgainSF = AE_GAIN_BASE;
		pstAeResult->u32IsoSF = 100;
		pstAeResult->u32BlcIsoSF = 100;
		pstAeResult->u32IntTime[AE_SE] = 0;
		pstAeResult->u32ExpRatio = 0;
	}

	if (AeInfo[sID].bRegWeightUpdate) {
		memcpy(pstAeResult->stStatAttr.au8WeightTable,
			pstAeStatisticsCfgInfo[sID]->au8Weight,
			AE_WEIGHT_ZONE_ROW * AE_WEIGHT_ZONE_COLUMN);
		pstAeResult->stStatAttr.bWightTableUpdate = 1;
		AeInfo[sID].bRegWeightUpdate = 0;
	} else {
		pstAeResult->stStatAttr.bWightTableUpdate = 0;
	}

	AE_CalculateDCFInfo(sID, pstAeResult);

	if (AE_GetDebugMode(sID) == 10) {
		printf("Ae Result fid:%u\n", AeInfo[sID].u32frmCnt);
		for (i = 0; i < AeInfo[sID].u8AeMaxFrameNum; ++i) {
			printf("%d T:%u(%u) ST:%u(%u) RT:%u AG:%u(%u) DG:%u(%u) IG:%u(%u) ISO:%u %u\n", i,
				pStExp[i].u32ExpTime, pStExp[i].u32ExpTimeLine,
				pStExp[i].u32SensorExpTime, pStExp[i].u32SensorExpTimeLine,
				pstAeResult->u32IntTime[i],
				pStExp[i].stExpGain.u32AGain, pStExp[i].stSensorExpGain.u32AGain,
				pStExp[i].stExpGain.u32DGain, pStExp[i].stSensorExpGain.u32DGain,
				pStExp[i].stExpGain.u32ISPDGain, pStExp[i].stSensorExpGain.u32ISPDGain,
				AeInfo[sID].u32ISONum[i], AeInfo[sID].u32BLCISONum[i]);
			//printf("Ev:%lu pEv:%lu R:%f\n", curFrameEv[i], preFrameEv[i], pstAeResult->fEvRatio[i]);
		}

		printf("Lv:%d Bs:%d stable:%d\n", pstAeResult->s16CurrentLV, AeInfo[sID].s16BvStepEntry[AE_LE],
				pstAeResult->bStable);
	}

	if (AE_isRawReplayMode(sID)) {
		pstAeResult->bStable = 1;
		#if 0
		if (AeInfo[sID].bWDRMode) {
			printf("fid:%u L:%d %d ExpR:%d TnrR:%f %f\n", AeInfo[sID].u32frmCnt,
				AeInfo[sID].u16FrameAvgLuma[0], AeInfo[sID].u16FrameAvgLuma[1],
				pstAeResult->u32ExpRatio, pstAeResult->fEvRatio[AE_LE],
				pstAeResult->fEvRatio[AE_SE]);
			printf("T0:%u AG:%u DG:%u IG:%u T1:%u AG:%u DG:%u IG:%u ISO:%u Lv:%d\n",
				pstAeResult->u32IntTime[AE_LE], pstAeResult->u32Again,
				pstAeResult->u32Dgain, pstAeResult->u32IspDgain,
				pstAeResult->u32IntTime[AE_SE], pstAeResult->u32AgainSF, pstAeResult->u32DgainSF,
				pstAeResult->u32IspDgainSF, pstAeResult->u32Iso, pstAeResult->s16CurrentLV);
		} else {
			printf("fid:%u L:%d TnrR:%f\n", AeInfo[sID].u32frmCnt, AeInfo[sID].u16FrameAvgLuma[0],
				pstAeResult->fEvRatio[AE_LE]);
			printf("T0:%u AG:%u DG:%u IG:%u ISO:%u Lv:%d\n", pstAeResult->u32IntTime[AE_LE],
				pstAeResult->u32Again, pstAeResult->u32Dgain, pstAeResult->u32IspDgain,
				pstAeResult->u32Iso, pstAeResult->s16CurrentLV);
		}
		#endif
	}


	for (i = 0; i < AeInfo[sID].u8AeMaxFrameNum; ++i) {
		preFrameEv[sID][i] = curFrameEv[i];
	}


	if (bTableUpdateEnable[sID] && (pstRawReplayExpInfo[sID] != CVI_NULL)) {
		tableIndex = AeInfo[sID].u32frmCnt % aeRawReplayTableSize[sID];
		pstRawReplayExpInfo[sID][tableIndex].u32FrameID = AeInfo[sID].u32frmCnt;
		pstRawReplayExpInfo[sID][tableIndex].u32Time[AE_LE] = pStExp[AE_LE].u32SensorExpTime;
		pstRawReplayExpInfo[sID][tableIndex].u32AGain[AE_LE] = pStExp[AE_LE].stSensorExpGain.u32AGain;
		pstRawReplayExpInfo[sID][tableIndex].u32DGain[AE_LE] = pStExp[AE_LE].stSensorExpGain.u32DGain;
		pstRawReplayExpInfo[sID][tableIndex].u32ISPDGain[AE_LE] = pStExp[AE_LE].stSensorExpGain.u32ISPDGain;
		pstRawReplayExpInfo[sID][tableIndex].u32ISO = AeInfo[sID].u32ISONum[AE_LE];
		pstRawReplayExpInfo[sID][tableIndex].s16LvX100 = AE_GetCurrentLvX100(sID);
		if (AeInfo[sID].bWDRMode) {
			pstRawReplayExpInfo[sID][tableIndex].u32Time[AE_SE] = pStExp[AE_SE].u32SensorExpTime;
			pstRawReplayExpInfo[sID][tableIndex].u32AGain[AE_SE] = pStExp[AE_SE].stSensorExpGain.u32AGain;
			pstRawReplayExpInfo[sID][tableIndex].u32DGain[AE_SE] = pStExp[AE_SE].stSensorExpGain.u32DGain;
			pstRawReplayExpInfo[sID][tableIndex].u32ISPDGain[AE_SE] = pStExp[AE_SE].stSensorExpGain.u32ISPDGain;
		} else {
			pstRawReplayExpInfo[sID][tableIndex].u32Time[AE_SE] = 0;
			pstRawReplayExpInfo[sID][tableIndex].u32AGain[AE_SE] = 0;
			pstRawReplayExpInfo[sID][tableIndex].u32DGain[AE_SE] = 0;
			pstRawReplayExpInfo[sID][tableIndex].u32ISPDGain[AE_SE] = 0;
		}
		#ifdef FRAME_ID_CHECK
		printf("tableIndex:%d fid:%u Exp0:%u %u %u %u Exp1:%u %u %u %u ISO:%u LV:%u\n",
			tableIndex,
			pstRawReplayExpInfo[sID][tableIndex].u32FrameID,
			pstRawReplayExpInfo[sID][tableIndex].u32Time[AE_LE],
			pstRawReplayExpInfo[sID][tableIndex].u32AGain[AE_LE],
			pstRawReplayExpInfo[sID][tableIndex].u32DGain[AE_LE],
			pstRawReplayExpInfo[sID][tableIndex].u32ISPDGain[AE_LE],
			pstRawReplayExpInfo[sID][tableIndex].u32Time[AE_SE],
			pstRawReplayExpInfo[sID][tableIndex].u32AGain[AE_SE],
			pstRawReplayExpInfo[sID][tableIndex].u32DGain[AE_SE],
			pstRawReplayExpInfo[sID][tableIndex].u32ISPDGain[AE_SE],
			pstRawReplayExpInfo[sID][tableIndex].u32ISO,
			pstRawReplayExpInfo[sID][tableIndex].s16LvX100);
		#endif
	}

	if (AeInfo[sID].u16RawNum) {
		AeInfo[sID].u16RawBufCnt++;
		if (AeInfo[sID].u16RawBufCnt >= AeInfo[sID].u16RawNum)
			bTableUpdateEnable[sID] = 0;
	}


	#if CHECK_AE_SIM
	AE_SaveSnapLog(sID);
	#endif
}

#ifdef ENABLE_DUMP_BOOT_LOG
void AE_SaveBootLog(CVI_U8 sID)
{
	CVI_U8 i;
	static CVI_U32 bootFrmCnt[AE_SENSOR_NUM];

	sID = AE_CheckSensorID(sID);

	AE_MemoryAlloc(sID, AE_BOOT_ITEM);

	if (!AeBootInfo[sID]) {
		return;
	}
	if (bootFrmCnt[sID] < AE_BOOT_MAX_FRAME) {
		for (i = 0; i < AeInfo[sID].u8AeMaxFrameNum; ++i) {
			AeBootInfo[sID]->u8FrmID[bootFrmCnt[sID]] = AeInfo[sID].u32frmCnt;
			AeBootInfo[sID]->u16FrmLuma[i][bootFrmCnt[sID]] = AeInfo[sID].u16FrameLuma[i];
			AeBootInfo[sID]->stApex[i][bootFrmCnt[sID]] = AeInfo[sID].stApex[i];
			AeBootInfo[sID]->u16AdjustTargetLuma[i][bootFrmCnt[sID]] = AeInfo[sID].u16AdjustTargetLuma[i];
			AeBootInfo[sID]->s16FrmBvStep[i][bootFrmCnt[sID]] = AeInfo[sID].s16BvStepEntry[i];
			AeBootInfo[sID]->bFrmConvergeMode[i][bootFrmCnt[sID]] = AeInfo[sID].u8ConvergeMode[i];
			AeBootInfo[sID]->bStable[i][bootFrmCnt[sID]] = AeInfo[sID].bIsStable[i];
			AeBootInfo[sID]->s16LvX100[bootFrmCnt[sID]] = AeInfo[sID].s16LvX100[AE_LE];
		}
		bootFrmCnt[sID]++;
	}
	#if 0
	if (bootFrmCnt[sID] == AE_BOOT_MAX_FRAME) {
		for (i = 0; i < AE_BOOT_MAX_FRAME; ++i) {
			printf("fid:%d L:%d TL:%d bv:%d tv:%d sv:%d bs:%d lv:%d cm:%d s:%d\n",
				AeBootInfo[sID]->u8FrmID[i], AeBootInfo[sID]->u16FrmLuma[AE_LE][i],
				AeBootInfo[sID]->u16AdjustTargetLuma[AE_LE][i],
				AeBootInfo[sID]->stApex[AE_LE][i].s16BVEntry,
				AeBootInfo[sID]->stApex[AE_LE][i].s16TVEntry,
				AeBootInfo[sID]->stApex[AE_LE][i].s16SVEntry,
				AeBootInfo[sID]->s16FrmBvStep[AE_LE][i], AeBootInfo[sID]->s16LvX100[i],
				AeBootInfo[sID]->bFrmConvergeMode[AE_LE][i], AeBootInfo[sID]->bStable[AE_LE][i]);
		}
		printf("\n");
		if (AeInfo[sID].bWDRMode) {
			for (i = 0; i < AE_BOOT_MAX_FRAME; ++i) {
				printf("fid:%d L:%d TL:%d bv:%d tv:%d sv:%d bs:%d lv:%d cm:%d s:%d\n",
					AeBootInfo[sID]->u8FrmID[i], AeBootInfo[sID]->u16FrmLuma[AE_SE][i],
					AeBootInfo[sID]->u16AdjustTargetLuma[AE_SE][i],
					AeBootInfo[sID]->stApex[AE_SE][i].s16BVEntry,
					AeBootInfo[sID]->stApex[AE_SE][i].s16TVEntry,
					AeBootInfo[sID]->stApex[AE_SE][i].s16SVEntry,
					AeBootInfo[sID]->s16FrmBvStep[AE_SE][i], AeBootInfo[sID]->s16LvX100[i],
					AeBootInfo[sID]->bFrmConvergeMode[AE_SE][i],
					AeBootInfo[sID]->bStable[AE_SE][i]);
			}
		}
		bootFrmCnt[sID]++;
	}
	#endif
}
#endif

#ifdef ARCH_RTOS_CV181X // TODO@CV181X
#include "FreeRTOS.h"
#endif

void AE_Delay1ms(CVI_S32 ms)
{
#ifdef ARCH_RTOS_CV181X // TODO@CV181X
	vTaskDelay(pdMS_TO_TICKS(ms));
#else
	usleep(ms * 1000);
#endif
}

void AE_Delay1s(CVI_S32 s)
{
#ifdef ARCH_RTOS_CV181X // TODO@CV181X
	vTaskDelay(pdMS_TO_TICKS(s * 1000));
#else
	sleep(s);
#endif
}

CVI_U32 AE_GetUSTimeDiff(const struct timeval *before, const struct timeval *after)
{
	CVI_U32 CVI_timeDiff = 0;

	CVI_timeDiff = (after->tv_sec - before->tv_sec) * 1000000 + (after->tv_usec - before->tv_usec);

	return CVI_timeDiff;
}

CVI_U32 AE_GetMSTimeDiff(const struct timeval *before, const struct timeval *after)
{
	CVI_U32 CVI_timeDiff = 0;

	CVI_timeDiff = AE_GetUSTimeDiff(before, after) / 1000;

	return CVI_timeDiff;
}

void AE_SaveSmoothExpSettingLog(CVI_U8 sID)
{
	CVI_U16 i;

	sID = AE_CheckSensorID(sID);
	for (i = 0; i < AeInfo[sID].u8AeMaxFrameNum; ++i) {
		AE_LogPrintf("%s", AE_GetTimestampStr());
		AE_LogPrintf("fid(%d, %d):%10d Luma:%4d Lv:%5d Time:%5d ExpL:%4d ",
				sID, i, AeInfo[sID].u32frmCnt, AeInfo[sID].u16CenterG[i], AeInfo[sID].s16LvX100[AE_LE],
				AeInfo[sID].stExp[i].u32ExpTime, AeInfo[sID].stExp[i].u32ExpTimeLine);
		AE_LogPrintf("ISO:%10d AG:%6d DG:%6d IspDG:%5d ",
			AeInfo[sID].u32ISONum[AE_LE], AeInfo[sID].stExp[i].stExpGain.u32AGain,
			AeInfo[sID].stExp[i].stExpGain.u32DGain, AeInfo[sID].stExp[i].stExpGain.u32ISPDGain);
		AE_LogPrintf("STime:%5d SExpL:%4d BlcISO:%u SAG:%6u SDG:%6u SIG:%5u\n",
			AeInfo[sID].stExp[i].u32SensorExpTime, AeInfo[sID].stExp[i].u32SensorExpTimeLine,
			AeInfo[sID].u32BLCISONum[AE_LE], AeInfo[sID].stExp[i].stSensorExpGain.u32AGain,
			AeInfo[sID].stExp[i].stSensorExpGain.u32DGain,
			AeInfo[sID].stExp[i].stSensorExpGain.u32ISPDGain);
	}
}

static void AE_SaveLog(CVI_U8 sID, const ISP_AE_RESULT_S *pstAeResult)
{
	CVI_U16 i, j;

	if (AE_LogBuf == CVI_NULL) {
		return;
	}

	if (AE_Dump2File)
		return;

	for (i = 0; i < AeInfo[sID].u8AeMaxFrameNum; ++i) {
		AE_LogPrintf("%s", AE_GetTimestampStr());
		AE_LogPrintf("fid(%d, %d):%6d Luma:%4d Lv:%5d T:%5d L:%4d ",
			sID, i, AeInfo[sID].u32frmCnt, AeInfo[sID].u16CenterG[i], AeInfo[sID].s16LvX100[AE_LE],
			AeInfo[sID].stExp[i].u32ExpTime, AeInfo[sID].stExp[i].u32ExpTimeLine);
		AE_LogPrintf("ISO:%6d AG:%6d DG:%6d IG:%5d ",
			AeInfo[sID].u32ISONum[AE_LE], AeInfo[sID].stExp[i].stExpGain.u32AGain,
			AeInfo[sID].stExp[i].stExpGain.u32DGain, AeInfo[sID].stExp[i].stExpGain.u32ISPDGain);
		AE_LogPrintf("ST:%5d SL:%4d SAG:%6u SDG:%6u SIG:%5u\n",
			AeInfo[sID].stExp[i].u32SensorExpTime, AeInfo[sID].stExp[i].u32SensorExpTimeLine,
			AeInfo[sID].stExp[i].stSensorExpGain.u32AGain,
			AeInfo[sID].stExp[i].stSensorExpGain.u32DGain,
			AeInfo[sID].stExp[i].stSensorExpGain.u32ISPDGain);

		AE_LogPrintf("AeL(%d, %d):%d %d TL:%d ATL:%d LO:%d LB:%d HB:%d\n", sID, i,
			AeInfo[sID].u16FrameLuma[i], AeInfo[sID].u16CenterG[i], AeInfo[sID].u16TargetLuma[i],
			AeInfo[sID].u16AdjustTargetLuma[i], AeInfo[sID].s16TargetlumaOffset[i],
			AeInfo[sID].s16TargetLumaLowBound[i], AeInfo[sID].s16TargetLumaHighBound[i]);
		AE_LogPrintf("lBs:%d Bs:%d(%d %d) sBs:%d cBs:%d\n",
			AeInfo[sID].s16lumaBvStepEntry[i],
			AeInfo[sID].s16BvStepEntry[i], AeInfo[sID].as16SmoothBv[i][0],
			AeInfo[sID].as16SmoothBv[i][1], AeInfo[sID].s16SmoothBvStepEntry[i],
			AeInfo[sID].s16ConvBvStepEntry[i]);
		AE_LogPrintf("Bv(%d, %d):%d pBv:%d Tv:%d Sv:%d\n", sID, i,
			AeInfo[sID].stApex[i].s16BVEntry, AeInfo[sID].s16PreBvEntry[i],
			AeInfo[sID].stApex[i].s16TVEntry, AeInfo[sID].stApex[i].s16SVEntry);
		AE_LogPrintf("CM:%d S:%d CS:%d %d LV:%d P:%d LR:%d\n", AeInfo[sID].u8ConvergeMode[i],
			AeInfo[sID].bIsStable[i], AeInfo[sID].bConvergeStable[i], AeInfo[sID].u8ConvergeSpeed[i],
			AeInfo[sID].s16LvX100[AE_LE], AeInfo[sID].u8LimitExposurePath[i], AeInfo[sID].u8His255Ratio[i]);

		if (AeInfo[sID].bEnableSmoothAE && !AeInfo[sID].bMeterEveryFrame) {
			for (j = 0; j < AeInfo[sID].u8MeterFramePeriod; ++j) {
				AE_LogPrintf("%d sBv:%d %d %d\n", AeInfo[sID].u32frmCnt + j,
				AeInfo[sID].stSmoothApex[i][j].s16BVEntry,
				AeInfo[sID].stSmoothApex[i][j].s16TVEntry, AeInfo[sID].stSmoothApex[i][j].s16SVEntry);
			}
		}
	}

	if ((pstAeSmartExposureAttrInfo[sID]->bEnable || pstAeExposureAttrInfo[sID]->stAuto.bEnableFaceAE)
		&& AeFaceDetect[sID].bMode) {
		AE_LogPrintf("\nFD(%d) N:%d L:%d TL:%d W:%d Bs:%d EnBs:%d FBs:%d\n", sID,
			AeFaceDetect[sID].stSmartInfo.u8Num,
			AeFaceDetect[sID].u16FDLuma, AeFaceDetect[sID].u16FDTargetLuma,
			pstAeSmartExposureAttrInfo[sID]->u8Weight, AeFaceDetect[sID].s16FDEVStep,
			AeFaceDetect[sID].s16EnvBvStep, AeFaceDetect[sID].s16FinalBvStep);
		AE_LogPrintf("X:%d Y:%d W:%d H:%d FW:%d FH:%d\n", AeFaceDetect[sID].u16FDPosX,
			AeFaceDetect[sID].u16FDPosY, AeFaceDetect[sID].u16FDWidth, AeFaceDetect[sID].u16FDHeight,
			AeFaceDetect[sID].u16FrameWidth, AeFaceDetect[sID].u16FrameHeight);
		AE_LogPrintf("AE X:%d Y:%d W:%d H:%d\n", AeFaceDetect[sID].u16AEFDPosX, AeFaceDetect[sID].u16AEFDPosY,
			AeFaceDetect[sID].u16AEFDWidth, AeFaceDetect[sID].u16AEFDHeight);
		AE_LogPrintf("GS:%d GE:%d GC:%d WC:%d HC:%d\n", AeFaceDetect[sID].u16AEFDGridStart,
			AeFaceDetect[sID].u16AEFDGridEnd, AeFaceDetect[sID].u16AEFDGridCount,
			AeFaceDetect[sID].u8AEFDGridWidthCount, AeFaceDetect[sID].u8AEFDGridHeightCount);
	} else
		AE_LogPrintf("FD(%d):%d %d %d\n", sID, pstAeSmartExposureAttrInfo[sID]->bEnable,
			pstAeExposureAttrInfo[sID]->stAuto.bEnableFaceAE, AeFaceDetect[sID].bMode);

	if (pstAeExposureAttrInfo[sID]->stAuto.enAEMode == AE_MODE_SLOW_SHUTTER)
		AE_LogPrintf("enSS:%d\n", AeInfo[sID].enSlowShutterStatus);

	AE_LogPrintf("Res T:%d %d G:%d %d %d Iso:%u BlcIso:%u ExpR:%d stb:%d\n", pstAeResult->u32IntTime[0],
		pstAeResult->u32IntTime[1], pstAeResult->u32Again, pstAeResult->u32Dgain,
		pstAeResult->u32IspDgain, pstAeResult->u32Iso, pstAeResult->u32BlcIso, pstAeResult->u32ExpRatio,
		pstAeResult->bStable);

	AE_LogPrintf("L/S:%d S:%d Off:%d HR:%d %d HBR:%d %d LR:%d %d LBR:%d %d\n\n",
		pstAeExposureAttrInfo[sID]->stAuto.enAEStrategyMode,
		pstAeExposureAttrInfo[sID]->stAuto.u16HistRatioSlope,
		pstAeExposureAttrInfo[sID]->stAuto.u8MaxHistOffset,
		AeInfo[sID].u16HighRatio[AE_LE][GRID_BASE], AeInfo[sID].u16HighRatio[AE_LE][HISTOGRAM_BASE],
		AeInfo[sID].u16HighBufRatio[AE_LE][GRID_BASE], AeInfo[sID].u16HighBufRatio[AE_LE][HISTOGRAM_BASE],
		AeInfo[sID].u16LowRatio[AE_LE][GRID_BASE], AeInfo[sID].u16LowRatio[AE_LE][HISTOGRAM_BASE],
		AeInfo[sID].u16LowBufRatio[AE_LE][GRID_BASE], AeInfo[sID].u16LowBufRatio[AE_LE][HISTOGRAM_BASE]);
}

void AE_SaveSnapLog(CVI_U8 sID)
{//dbg cmd: 110 , 16 0 0 0 0

	CVI_U16 i, j, k, n;

	CVI_BOOL statisticsDivide = 0;

	statisticsDivide = (AeInfo[sID].u8GridVNum / AE_WEIGHT_ZONE_ROW > 1 ||
		AeInfo[sID].u8GridHNum / AE_WEIGHT_ZONE_COLUMN > 1) ? 1 : 0;

	#ifndef AAA_PC_PLATFORM
	#if CHECK_AE_SIM == 0
	//Don't modify ==>
	AE_SnapLogPrintf("\nfrmNum:%d\n", AeInfo[sID].u8AeMaxFrameNum);
	for (i = 0; i < AeInfo[sID].u8AeMaxFrameNum; ++i) {
		for (n = 0; n < AE_CHANNEL_NUM; ++n) {
			if (n == AE_CHANNEL_GR)//GR = GB
				continue;
			for (j = 0; j < AE_WEIGHT_ZONE_ROW; ++j) {
				for (k = 0; k < AE_WEIGHT_ZONE_COLUMN; ++k) {
					if (statisticsDivide) {
						AE_SnapLogPrintf("%d\t", (AeInfo[sID].pu16AeStatistics[i][2*j][2*k][n] +
							AeInfo[sID].pu16AeStatistics[i][2*j][2*k+1][n] +
							AeInfo[sID].pu16AeStatistics[i][2*j+1][2*k][n] +
							AeInfo[sID].pu16AeStatistics[i][2*j+1][2*k+1][n]) / 4);
					} else {
						AE_SnapLogPrintf("%d\t", AeInfo[sID].pu16AeStatistics[i][j][k][n]);
					}
				}
				AE_SnapLogPrintf("\n");
			}
			AE_SnapLogPrintf("\n");
		}

		for (j = 0; j < HIST_NUM; ++j) {
			AE_SnapLogPrintf("%d\t", AeInfo[sID].pu32Histogram[i][j]);
			if ((j+1) % 16 == 0)
				AE_SnapLogPrintf("\n");
		}
		AE_SnapLogPrintf("\n");
	}
	//Don't modify <==
	#else
	UNUSED(j);
	UNUSED(k);
	UNUSED(n);
	#endif
	#endif

	for (i = 0; i < AeInfo[sID].u8AeMaxFrameNum; ++i) {
		AE_SnapLogPrintf("AeL(%d, %d):%d %d TL:%d ATL:%d LO:%d LB:%d HB:%d\n", sID, i,
			AeInfo[sID].u16FrameLuma[i], AeInfo[sID].u16CenterG[i], AeInfo[sID].u16TargetLuma[i],
			AeInfo[sID].u16AdjustTargetLuma[i], AeInfo[sID].s16TargetlumaOffset[i],
			AeInfo[sID].s16TargetLumaLowBound[i], AeInfo[sID].s16TargetLumaHighBound[i]);
		AE_SnapLogPrintf("lBs:%d\n", AeInfo[sID].s16lumaBvStepEntry[i]);
		AE_SnapLogPrintf("Bv(%d, %d):%d pBv:%d Bs:%d cBs:%d  Tv:%d Sv:%d\n", sID, i,
			AeInfo[sID].stApex[i].s16BVEntry, AeInfo[sID].s16PreBvEntry[i],
			AeInfo[sID].s16BvStepEntry[i],  AeInfo[sID].s16ConvBvStepEntry[i],
			AeInfo[sID].stApex[i].s16TVEntry, AeInfo[sID].stApex[i].s16SVEntry);
		AE_SnapLogPrintf("CM:%d S:%d LV:%d ISO:%d\n", AeInfo[sID].u8ConvergeMode[i],
			AeInfo[sID].bIsStable[i], AeInfo[sID].s16LvX100[AE_LE], AeInfo[sID].u32ISONum[i]);
		AE_SnapLogPrintf("T:%u(%u) L:%u(%u) AG:%u(%u) DG:%u(%u) IG:%u(%u)\n",
			AeInfo[sID].stExp[i].u32ExpTime, AeInfo[sID].stExp[i].u32SensorExpTime,
			AeInfo[sID].stExp[i].u32ExpTimeLine, AeInfo[sID].stExp[i].u32SensorExpTimeLine,
			AeInfo[sID].stExp[i].stExpGain.u32AGain, AeInfo[sID].stExp[i].stSensorExpGain.u32AGain,
			AeInfo[sID].stExp[i].stExpGain.u32DGain, AeInfo[sID].stExp[i].stSensorExpGain.u32DGain,
			AeInfo[sID].stExp[i].stExpGain.u32ISPDGain, AeInfo[sID].stExp[i].stSensorExpGain.u32ISPDGain);
		if (i == 1)
			AE_SnapLogPrintf("expR:%d\n", AeInfo[sID].u32WDRExpRatio);
	}
}

void AE_SaveParameterSetting(void)
{
	CVI_U8 i, j, sID;
	CVI_U8 aeFps, pubFps, ssFps;

	for (sID = 0; sID < u8SensorNum; ++sID) {
		aeFps = (CVI_U8)AeInfo[sID].fFps;
		pubFps = (CVI_U8)AeInfo[sID].fIspPubAttrFps;
		ssFps = (CVI_U8)AeInfo[sID].fSlowShutterFps;

		AE_SnapLogPrintf("\nMPI:%d\n", sID);
		AE_SnapLogPrintf("WDR:%d blc:%d P:%d %d %d\n", AeInfo[sID].bWDRMode, AeInfo[sID].enBlcType,
				AeInfo[sID].u8SensorPeriod, AeInfo[sID].u8SensorRunInterval,
				AeInfo[sID].u8MeterFramePeriod);

		AE_SnapLogPrintf("SS E:%d\n", AeInfo[sID].s16SlowShutterISOEntry);
		AE_SnapLogPrintf("fps:%d %d %d\n", pubFps, aeFps, ssFps);
		AE_SnapLogPrintf("Ss:\n");
		AE_SnapLogPrintf("L m:%u M:%u\n", AeInfo[sID].u32ExpLineMin, AeInfo[sID].u32ExpLineMax);
		AE_SnapLogPrintf("T m:%u M:%u\n", AeInfo[sID].u32ExpTimeMin, AeInfo[sID].u32ExpTimeMax);
		if (AeInfo[sID].bWDRMode)
			AE_SnapLogPrintf("SE L:%u T:%u\n", AeInfo[sID].u32WDRSEExpLineMax,
				AeInfo[sID].u32WDRSEExpTimeMax);
		AE_SnapLogPrintf("A m:%u M:%u\n", AeInfo[sID].u32AGainMin, AeInfo[sID].u32AGainMax);
		AE_SnapLogPrintf("D m:%u M:%u\n", AeInfo[sID].u32DGainMin, AeInfo[sID].u32DGainMax);

		AE_SnapLogPrintf("Rg:\n");
		AE_SnapLogPrintf("BV m:%d M:%d\n", AeInfo[sID].s16MinTVEntry - AeInfo[sID].s16MaxISOEntry,
					AeInfo[sID].s16MaxTVEntry - AeInfo[sID].s16MinISOEntry);
		AE_SnapLogPrintf("TV m:%d M:%d SE:%d\n", AeInfo[sID].s16MinTVEntry, AeInfo[sID].s16MaxTVEntry,
			AeInfo[sID].s16WDRSEMinTVEntry);
		AE_SnapLogPrintf("Ss m:%d M:%d ss:%d\n", AeInfo[sID].s16MinSensorTVEntry,
			AeInfo[sID].s16MaxSensorTVEntry, AeInfo[sID].s16MinFpsSensorTVEntry);
		AE_SnapLogPrintf("Rt m:%d M:%d\n", AeInfo[sID].s16MinRouteTVEntry, AeInfo[sID].s16MaxRouteTVEntry);
		AE_SnapLogPrintf("RtEx m:%d M:%d\n", AeInfo[sID].s16MinRouteExTVEntry,
			AeInfo[sID].s16MaxRouteExTVEntry);
		AE_SnapLogPrintf("Rg m:%d M:%d\n\n", AeInfo[sID].s16MinRangeTVEntry, AeInfo[sID].s16MaxRangeTVEntry);
		AE_SnapLogPrintf("ISO: m:%d M:%d\n", AeInfo[sID].s16MinISOEntry, AeInfo[sID].s16MaxISOEntry);
		AE_SnapLogPrintf("Ss m:%d M:%d\n", AeInfo[sID].s16MinSensorISOEntry,
			AeInfo[sID].s16MaxSensorISOEntry);
		AE_SnapLogPrintf("Rt m:%d M:%d\n", AeInfo[sID].s16MinRouteISOEntry, AeInfo[sID].s16MaxRouteISOEntry);
		AE_SnapLogPrintf("RtEx m:%d M:%d\n", AeInfo[sID].s16MinRouteExISOEntry,
			AeInfo[sID].s16MaxRouteExISOEntry);
		AE_SnapLogPrintf("Rg m:%d M:%d\n", AeInfo[sID].s16MinRangeISOEntry, AeInfo[sID].s16MaxRangeISOEntry);
		AE_SnapLogPrintf("BLC M:%d\n", AeInfo[sID].s16BLCMaxISOEntry);
		AE_SnapLogPrintf("\n");

		//ExposureAttr
		if (pstAeMpiExposureAttr[sID] != CVI_NULL) {
			AE_SnapLogPrintf("BP:%d Op:%d RI:%d ExV:%d His:%d GS:%d\n",
				pstAeMpiExposureAttr[sID]->bByPass, pstAeMpiExposureAttr[sID]->enOpType,
				pstAeMpiExposureAttr[sID]->u8AERunInterval, pstAeMpiExposureAttr[sID]->bAERouteExValid,
				pstAeMpiExposureAttr[sID]->bHistStatAdjust, pstAeMpiExposureAttr[sID]->bAEGainSepCfg);
			AE_SnapLogPrintf("Op T:%d uISO:%d ISO:%d\n",
				pstAeMpiExposureAttr[sID]->stManual.enExpTimeOpType,
				pstAeMpiExposureAttr[sID]->stManual.enGainType,
				pstAeMpiExposureAttr[sID]->stManual.enISONumOpType);
			AE_SnapLogPrintf("AG:%d DG:%d IG:%d\n",
				pstAeMpiExposureAttr[sID]->stManual.enAGainOpType,
				pstAeMpiExposureAttr[sID]->stManual.enDGainOpType,
				pstAeMpiExposureAttr[sID]->stManual.enISPDGainOpType);
			AE_SnapLogPrintf("T:%u ISO:%u AG:%u DG:%u IG:%u\n",
				pstAeMpiExposureAttr[sID]->stManual.u32ExpTime,
				pstAeMpiExposureAttr[sID]->stManual.u32ISONum,
				pstAeMpiExposureAttr[sID]->stManual.u32AGain,
				pstAeMpiExposureAttr[sID]->stManual.u32DGain,
				pstAeMpiExposureAttr[sID]->stManual.u32ISPDGain);
			AE_SnapLogPrintf("TRg:%u-%u AE:%u-%u\n",
				pstAeMpiExposureAttr[sID]->stAuto.stExpTimeRange.u32Min,
				pstAeMpiExposureAttr[sID]->stAuto.stExpTimeRange.u32Max,
				pstAeExposureAttrInfo[sID]->stAuto.stExpTimeRange.u32Min,
				pstAeExposureAttrInfo[sID]->stAuto.stExpTimeRange.u32Max);
			AE_SnapLogPrintf("AGRg:%u-%u AE:%u-%u\n",
				pstAeMpiExposureAttr[sID]->stAuto.stAGainRange.u32Min,
				pstAeMpiExposureAttr[sID]->stAuto.stAGainRange.u32Max,
				pstAeExposureAttrInfo[sID]->stAuto.stAGainRange.u32Min,
				pstAeExposureAttrInfo[sID]->stAuto.stAGainRange.u32Max);
			AE_SnapLogPrintf("DGRg:%u-%u AE:%u-%u\n",
				pstAeMpiExposureAttr[sID]->stAuto.stDGainRange.u32Min,
				pstAeMpiExposureAttr[sID]->stAuto.stDGainRange.u32Max,
				pstAeExposureAttrInfo[sID]->stAuto.stDGainRange.u32Min,
				pstAeExposureAttrInfo[sID]->stAuto.stDGainRange.u32Max);
			AE_SnapLogPrintf("IGRg:%u-%u AE:%u-%u\n",
				pstAeMpiExposureAttr[sID]->stAuto.stISPDGainRange.u32Min,
				pstAeMpiExposureAttr[sID]->stAuto.stISPDGainRange.u32Max,
				pstAeExposureAttrInfo[sID]->stAuto.stISPDGainRange.u32Min,
				pstAeExposureAttrInfo[sID]->stAuto.stISPDGainRange.u32Max);
			AE_SnapLogPrintf("SGRg:%u-%u AE:%u-%u\n",
				pstAeMpiExposureAttr[sID]->stAuto.stSysGainRange.u32Min,
				pstAeMpiExposureAttr[sID]->stAuto.stSysGainRange.u32Max,
				pstAeExposureAttrInfo[sID]->stAuto.stSysGainRange.u32Min,
				pstAeExposureAttrInfo[sID]->stAuto.stSysGainRange.u32Max);
			AE_SnapLogPrintf("uISO:%d ISORg:%u-%u AE:%u-%u\n",
				pstAeMpiExposureAttr[sID]->stAuto.enGainType,
				pstAeMpiExposureAttr[sID]->stAuto.stISONumRange.u32Min,
				pstAeMpiExposureAttr[sID]->stAuto.stISONumRange.u32Max,
				pstAeExposureAttrInfo[sID]->stAuto.stISONumRange.u32Min,
				pstAeExposureAttrInfo[sID]->stAuto.stISONumRange.u32Max);
			AE_SnapLogPrintf("GaThr%u Sp:%d BSp:%d Tol:%d\n",
				pstAeMpiExposureAttr[sID]->stAuto.u32GainThreshold, pstAeMpiExposureAttr[sID]->stAuto.u8Speed,
				pstAeMpiExposureAttr[sID]->stAuto.u16BlackSpeedBias,
				pstAeMpiExposureAttr[sID]->stAuto.u8Tolerance);

			AE_SnapLogPrintf("Comp:%d EvBias:%d\n",
				pstAeMpiExposureAttr[sID]->stAuto.u8Compensation, pstAeMpiExposureAttr[sID]->stAuto.u16EVBias);

			AE_SnapLogPrintf("StraM:%d RS:%d Off:%d M:%d\n",
				pstAeMpiExposureAttr[sID]->stAuto.enAEStrategyMode,
				pstAeMpiExposureAttr[sID]->stAuto.u16HistRatioSlope,
				pstAeMpiExposureAttr[sID]->stAuto.u8MaxHistOffset, pstAeMpiExposureAttr[sID]->stAuto.enAEMode);

			AE_SnapLogPrintf("AFk M:%d En:%d Hz:%d SFlicker en:%d luDiff:%d\n",
				pstAeMpiExposureAttr[sID]->stAuto.stAntiflicker.enMode,
				pstAeMpiExposureAttr[sID]->stAuto.stAntiflicker.bEnable,
				pstAeMpiExposureAttr[sID]->stAuto.stAntiflicker.enFrequency,
				pstAeMpiExposureAttr[sID]->stAuto.stSubflicker.bEnable,
				pstAeMpiExposureAttr[sID]->stAuto.stSubflicker.u8LumaDiff);

			AE_SnapLogPrintf("D B:%d W:%d\n",
				pstAeMpiExposureAttr[sID]->stAuto.stAEDelayAttr.u16BlackDelayFrame,
				pstAeMpiExposureAttr[sID]->stAuto.stAEDelayAttr.u16WhiteDelayFrame);

			if (pstAeMpiExposureAttr[sID]->stAuto.bManualExpValue) {
				AE_SnapLogPrintf("MExpV:%d ExpV:%d FSW:%d\n",
				pstAeMpiExposureAttr[sID]->stAuto.bManualExpValue,
				pstAeMpiExposureAttr[sID]->stAuto.u32ExpValue,
				pstAeMpiExposureAttr[sID]->stAuto.enFSWDRMode);
			}

			AE_SnapLogPrintf("TGMin:");
			for (i = 0; i < LV_TOTAL_NUM; ++i)
				AE_SnapLogPrintf("%d ", pstAeMpiExposureAttr[sID]->stAuto.au8AdjustTargetMin[i]);
			AE_SnapLogPrintf("\n");

			AE_SnapLogPrintf("TGMax:");
			for (i = 0; i < LV_TOTAL_NUM; ++i)
				AE_SnapLogPrintf("%d ", pstAeMpiExposureAttr[sID]->stAuto.au8AdjustTargetMax[i]);
			AE_SnapLogPrintf("\n");
			AE_SnapLogPrintf("fd:%d TL:%d w:%d\n", pstAeMpiExposureAttr[sID]->stAuto.bEnableFaceAE,
				pstAeMpiExposureAttr[sID]->stAuto.u8FaceTargetLuma,
				pstAeMpiExposureAttr[sID]->stAuto.u8FaceWeight);
			//WDR ExposureAttr
			if (AeInfo[sID].bWDRMode) {
				AE_SnapLogPrintf("GM:%d SG:%d\n", AeInfo[sID].enWDRGainMode, AeInfo[sID].bWDRUseSameGain);
				AE_SnapLogPrintf("RT:%d R:%d Rm:%d  RM:%d\n",
				pstAeMpiWDRExposureAttr[sID]->enExpRatioType, pstAeMpiWDRExposureAttr[sID]->au32ExpRatio[0],
				pstAeMpiWDRExposureAttr[sID]->u32ExpRatioMin, pstAeMpiWDRExposureAttr[sID]->u32ExpRatioMax);

				AE_SnapLogPrintf("Tol:%d Sp:%d B:%d SEComp:%d\n",
					pstAeMpiWDRExposureAttr[sID]->u16Tolerance, pstAeMpiWDRExposureAttr[sID]->u16Speed,
					pstAeMpiWDRExposureAttr[sID]->u16RatioBias,
					pstAeMpiWDRExposureAttr[sID]->u8SECompensation);

				AE_SnapLogPrintf("LE TGMin:\n");
				for (i = 0; i < LV_TOTAL_NUM; ++i)
					AE_SnapLogPrintf("%d ", pstAeMpiWDRExposureAttr[sID]->au8LEAdjustTargetMin[i]);
				AE_SnapLogPrintf("\n");

				AE_SnapLogPrintf("LE TGMax:\n");
				for (i = 0; i < LV_TOTAL_NUM; ++i)
					AE_SnapLogPrintf("%d ", pstAeMpiWDRExposureAttr[sID]->au8LEAdjustTargetMax[i]);

				AE_SnapLogPrintf("\n");

				AE_SnapLogPrintf("SE TGMin:\n");
				for (i = 0; i < LV_TOTAL_NUM; ++i)
					AE_SnapLogPrintf("%d ", pstAeMpiWDRExposureAttr[sID]->au8SEAdjustTargetMin[i]);
				AE_SnapLogPrintf("\n");

				AE_SnapLogPrintf("SE TGMax:\n");
				for (i = 0; i < LV_TOTAL_NUM; ++i)
					AE_SnapLogPrintf("%d ", pstAeMpiWDRExposureAttr[sID]->au8SEAdjustTargetMax[i]);
				AE_SnapLogPrintf("\n");

			}

			if (pstAeMpiExposureAttr[sID]->bAERouteExValid) {
				//RouteEx
				AE_SnapLogPrintf("RouteEx:\n");
				AE_SnapLogPrintf("T AGa DG IG iris\n");
				for (j = 0; j < AeInfo[sID].u8AeMaxFrameNum; ++j) {
					for (i = 0; i < pstAeMpiRouteEx[sID][j]->u32TotalNum; ++i) {
						AE_SnapLogPrintf("%u %u %u %u %d\n",
							pstAeMpiRouteEx[sID][j]->astRouteExNode[i].u32IntTime,
							pstAeMpiRouteEx[sID][j]->astRouteExNode[i].u32Again,
							pstAeMpiRouteEx[sID][j]->astRouteExNode[i].u32Dgain,
							pstAeMpiRouteEx[sID][j]->astRouteExNode[i].u32IspDgain,
							pstAeMpiRouteEx[sID][j]->astRouteExNode[i].enIrisFNO);
					}
					AE_SnapLogPrintf("\n");
				}
				AE_SnapLogPrintf("\n");
			} else {
				//Route
				AE_SnapLogPrintf("Route:\n");
				AE_SnapLogPrintf("T G iris\n");
				for (j = 0; j < AeInfo[sID].u8AeMaxFrameNum; ++j) {
					for (i = 0; i < pstAeMpiRoute[sID][j]->u32TotalNum; ++i) {
						AE_SnapLogPrintf("%u %u %d\n",
							pstAeMpiRoute[sID][j]->astRouteNode[i].u32IntTime,
							pstAeMpiRoute[sID][j]->astRouteNode[i].u32SysGain,
							pstAeMpiRoute[sID][j]->astRouteNode[i].enIrisFNO);
					}
					AE_SnapLogPrintf("\n");
				}
			}

			if (pstAeMpiSmartExposureAttr[sID]->bEnable) {
				AE_SnapLogPrintf("StAttr:\n");
				AE_SnapLogPrintf("b:%d Ir:%d T:%d TL:%d C:%d Cm:%d CM:%d I:%d S:%d D:%d W:%d R:%d\n",
					pstAeMpiSmartExposureAttr[sID]->bEnable,
					pstAeMpiSmartExposureAttr[sID]->bIRMode,
					pstAeMpiSmartExposureAttr[sID]->enSmartExpType,
					pstAeMpiSmartExposureAttr[sID]->u8LumaTarget,
					pstAeMpiSmartExposureAttr[sID]->u16ExpCoef,
					pstAeMpiSmartExposureAttr[sID]->u16ExpCoefMin,
					pstAeMpiSmartExposureAttr[sID]->u16ExpCoefMax,
					pstAeMpiSmartExposureAttr[sID]->u8SmartInterval,
					pstAeMpiSmartExposureAttr[sID]->u8SmartSpeed,
					pstAeMpiSmartExposureAttr[sID]->u16SmartDelayNum,
					pstAeMpiSmartExposureAttr[sID]->u8Weight,
					pstAeMpiSmartExposureAttr[sID]->u8NarrowRatio);
			} else {
				AE_SnapLogPrintf("StExpAtrr:%d\n", pstAeMpiSmartExposureAttr[sID]->bEnable);
			}

			//weight
			AE_SnapLogPrintf("Wt:\n");
			for (i = 0; i < AE_WEIGHT_ZONE_ROW; ++i) {
				for (j = 0; j < AE_WEIGHT_ZONE_COLUMN; ++j)
					AE_SnapLogPrintf("%d\t", pstAeStatisticsCfg[sID]->au8Weight[i][j]);
				AE_SnapLogPrintf("\n");
			}
			AE_SnapLogPrintf("\n");
		}
	}
}

CVI_U8 AE_GetMeterPeriod(CVI_U8 sID)
{
	sID = AE_CheckSensorID(sID);
	return AeInfo[sID].u8MeterFramePeriod;
}

CVI_U8 AE_GetSensorPeriod(CVI_U8 sID)
{
	sID = AE_CheckSensorID(sID);
	return AeInfo[sID].u8SensorPeriod;
}

CVI_U8 AE_ViPipe2sID(VI_PIPE ViPipe)
{
	CVI_U8 sID = AE_CheckSensorID(ViPipe);

	return sID;
}

VI_PIPE AE_sID2ViPipe(CVI_U8 sID)
{
//TODO@Lenny need check
	return sID;
}

void AE_GetBootExpousreInfo(CVI_U8 sID, AE_APEX *bootApex)
{
	sID = AE_CheckSensorID(sID);
	*bootApex = AeInfo[sID].stInitApex[AE_LE];
}

CVI_U8 AE_GetSensorDGainType(CVI_U8 sID)
{
	sID = AE_CheckSensorID(sID);
	return AeInfo[sID].u8DGainAccuType;
}

CVI_U32 AE_GetDGainNodeValue(CVI_U8 sID, CVI_U8 nodeNum)
{
	sID = AE_CheckSensorID(sID);
	nodeNum = AAA_MIN(nodeNum, AeInfo[sID].u8SensorDgainNodeNum - 1);
	return AeInfo[sID].u32SensorDgainNode[nodeNum];
}

CVI_S32 AE_SetFps(CVI_U8 sID, CVI_FLOAT fps)
{
#ifndef AAA_PC_PLATFORM
	AE_CTX_S *pstAeCtx;
	CVI_U32 setFps, aeFps;

	sID = AE_CheckSensorID(sID);
	AAA_LIMIT(fps, AeInfo[sID].fMinFps, AeInfo[sID].fMaxFps);
	setFps = fps * FLOAT_TO_INT_NUMBER;
	aeFps = AeInfo[sID].fFps * FLOAT_TO_INT_NUMBER;

	if (setFps != aeFps) {
		pstAeCtx = AE_GET_CTX(sID);
		ISP_LOG_NOTICE("AE fps:%f->%f\n", AeInfo[sID].fFps, fps);
		AE_InitSensorFpsPara(sID, fps, pstAeCtx);
	}
#endif
	return CVI_SUCCESS;
}

void AE_GetFps(CVI_U8 sID, CVI_FLOAT *pFps)
{
	sID = AE_CheckSensorID(sID);
	*pFps = AeInfo[sID].fFps;
}


void AE_GetSensorExposureSetting(CVI_U8 sID, AE_EXPOSURE *expSet)
{
	CVI_U8 i;

	sID = AE_CheckSensorID(sID);
	for (i = 0; i < AeInfo[sID].u8AeMaxFrameNum; ++i) {
		expSet[i].u32SensorExpTime = AeInfo[sID].stExp[i].u32SensorExpTime;
		expSet[i].u32SensorExpTimeLine = AeInfo[sID].stExp[i].u32SensorExpTimeLine;
		expSet[i].stSensorExpGain = AeInfo[sID].stExp[i].stSensorExpGain;
	}
}

void AE_GetWDRSEMinTvEntry(CVI_U8 sID, CVI_S16 *tvEntry)
{
	sID = AE_CheckSensorID(sID);
	*tvEntry = AeInfo[sID].s16WDRSEMinTVEntry;
}

CVI_BOOL AE_GetWDRExpLineRange(CVI_U8 sID, CVI_U32 expRatio, CVI_U32 *leExpLine, CVI_U32 *seExpLine,
	CVI_BOOL getMaxLine)
{
#ifndef AAA_PC_PLATFORM


	AE_CTX_S *pstAeCtx;
	VI_PIPE ViPipe;
	CVI_U16 manual = 1;
	CVI_U32 ratio[3] = { 256, 64, 64 }; //max 256x
	CVI_U32 IntTimeMax[4], IntTimeMin[4], LFMaxIntTime[4];

	ViPipe = AE_sID2ViPipe(sID);
	pstAeCtx = AE_GET_CTX(sID);
	ratio[0] = expRatio;

	if (pstAeCtx->stSnsRegister.stAeExp.pfn_cmos_get_inttime_max)
		pstAeCtx->stSnsRegister.stAeExp.pfn_cmos_get_inttime_max(ViPipe, manual, ratio, IntTimeMax,
										 IntTimeMin, LFMaxIntTime);
	#if 0
	printf("\nLE:%d(%d-%d) SE:%d(%d-%d)\n", *leExpLine, IntTimeMin[1], IntTimeMax[1],
			*seExpLine, IntTimeMin[0], IntTimeMax[0]);
	#endif
	if (getMaxLine) {
		IntTimeMin[0] = IntTimeMax[0];
		IntTimeMin[1] = IntTimeMax[1];
	}

	AAA_LIMIT(*seExpLine, IntTimeMin[0], IntTimeMax[0]);
	AAA_LIMIT(*leExpLine, IntTimeMin[1], IntTimeMax[1]);
#endif

	return CVI_SUCCESS;
}

void AE_GetExpGainInfo(CVI_U8 sID, AE_WDR_FRAME wdrFrm, AE_GAIN *expGain)
{
	sID = AE_CheckSensorID(sID);
	*expGain = AeInfo[sID].stExp[wdrFrm].stExpGain;
}

void AE_SetAPEXExposure(CVI_U8 sID, AE_WDR_FRAME wdrFrm, const AE_APEX *papex)
{
	sID = AE_CheckSensorID(sID);
	AeInfo[sID].stApex[wdrFrm] = *papex;
}

void AE_GetAPEXExposure(CVI_U8 sID, AE_WDR_FRAME wdrFrm, AE_APEX *papex)
{
	sID = AE_CheckSensorID(sID);
	*papex = AeInfo[sID].stApex[wdrFrm];
}


void AE_GetAlgoVer(CVI_U16 *pVer, CVI_U16 *pSubVer)
{
	*pVer = AE_LIB_VER;
	*pSubVer =  AE_LIB_SUBVER;
}

void AE_SetIspDgainCompensation(CVI_U8 sID, CVI_BOOL enable)
{
	sID = AE_CheckSensorID(sID);
	AeInfo[sID].bEnableISPDgainCompensation = enable;
	if (AE_GetDebugMode(sID) <= 60 || AE_GetDebugMode(sID) >= 90)
		printf("sID:%d enableIspDgainCompensation:%d\n", sID, enable);
}

void AE_ConfigExposureTime_ISO(CVI_U8 sID,  AE_WDR_FRAME wdrFrm, CVI_FLOAT time, CVI_U32 ISO)
{
	CVI_S16	isoEntry;
	AE_GAIN aeGain;

	sID = AE_CheckSensorID(sID);

	AAA_LIMIT(ISO, AeInfo[sID].u32ISONumMin, AeInfo[sID].u32ISONumMax);

	AE_GetISOEntryByISONum(ISO, &isoEntry);
	AE_GetExpGainByEntry(sID, isoEntry, &aeGain);
	AE_ConfigExposureTime_Gain(sID, wdrFrm, time, &aeGain);
}

void AE_ConfigExposureTime_Gain(CVI_U8 sID, AE_WDR_FRAME wdrFrm, CVI_FLOAT time, AE_GAIN *pstGain)
{
	CVI_U32 maxTime;

	sID = AE_CheckSensorID(sID);
	wdrFrm = AE_CheckWDRIdx(wdrFrm);

	maxTime = (wdrFrm == AE_SE) ? AeInfo[sID].u32WDRSEExpTimeMax : AeInfo[sID].u32ExpTimeMax;
	AAA_LIMIT(time, AeInfo[sID].u32ExpTimeMin, maxTime);
	AAA_LIMIT(pstGain->u32AGain, AeInfo[sID].u32AGainMin, AeInfo[sID].u32AGainMax);
	AAA_LIMIT(pstGain->u32DGain, AeInfo[sID].u32DGainMin, AeInfo[sID].u32DGainMax);
	AAA_LIMIT(pstGain->u32ISPDGain, AeInfo[sID].u32ISPDGainMin, AeInfo[sID].u32ISPDGainMax);

	AeInfo[sID].stExp[wdrFrm].fIdealExpTime = time;
	AeInfo[sID].stExp[wdrFrm].u32ExpTime = (CVI_U32)time;
	AeInfo[sID].stExp[wdrFrm].stExpGain.u32AGain = pstGain->u32AGain;
	AeInfo[sID].stExp[wdrFrm].stExpGain.u32DGain = pstGain->u32DGain;
	AeInfo[sID].stExp[wdrFrm].stExpGain.u32ISPDGain = pstGain->u32ISPDGain;

	if (AeInfo[sID].bWDRMode) {
		if (wdrFrm == AE_SE) {
			time *= 4;
			AAA_LIMIT(time, AeInfo[sID].u32ExpTimeMin, AeInfo[sID].u32ExpTimeMax);
			AeInfo[sID].stExp[AE_LE].fIdealExpTime = time;
			AeInfo[sID].stExp[AE_LE].u32ExpTime = (CVI_U32)time;
			if (AeInfo[sID].enWDRGainMode == SNS_GAIN_MODE_SHARE) {
				AeInfo[sID].stExp[AE_LE].stExpGain.u32AGain = pstGain->u32AGain;
				AeInfo[sID].stExp[AE_LE].stExpGain.u32DGain = pstGain->u32DGain;
				AeInfo[sID].stExp[AE_LE].stExpGain.u32ISPDGain = pstGain->u32ISPDGain;
			} else {
				AeInfo[sID].stExp[AE_LE].stExpGain.u32AGain = AE_GAIN_BASE;
				AeInfo[sID].stExp[AE_LE].stExpGain.u32DGain = AE_GAIN_BASE;
				AeInfo[sID].stExp[AE_LE].stExpGain.u32ISPDGain = AE_GAIN_BASE;
			}
		} else {
			time /= 4;
			AAA_LIMIT(time, AeInfo[sID].u32ExpTimeMin, AeInfo[sID].u32WDRSEExpTimeMax);
			AeInfo[sID].stExp[AE_SE].fIdealExpTime = time;
			AeInfo[sID].stExp[AE_SE].u32ExpTime = (CVI_U32)time;
			if (AeInfo[sID].enWDRGainMode == SNS_GAIN_MODE_SHARE) {
				AeInfo[sID].stExp[AE_SE].stExpGain.u32AGain = pstGain->u32AGain;
				AeInfo[sID].stExp[AE_SE].stExpGain.u32DGain = pstGain->u32DGain;
				AeInfo[sID].stExp[AE_SE].stExpGain.u32ISPDGain = pstGain->u32ISPDGain;
			} else {
				AeInfo[sID].stExp[AE_SE].stExpGain.u32AGain = AE_GAIN_BASE;
				AeInfo[sID].stExp[AE_SE].stExpGain.u32DGain = AE_GAIN_BASE;
				AeInfo[sID].stExp[AE_SE].stExpGain.u32ISPDGain = AE_GAIN_BASE;
			}
		}
	}
}

void AE_SetLumaBase(CVI_U8 sID, LUMA_BASE_E type)
{
	sID = AE_CheckSensorID(sID);
	AeInfo[sID].enLumaBase = (type == HISTOGRAM_BASE) ? HISTOGRAM_BASE : GRID_BASE;
}


void AE_SetParameterUpdate(CVI_U8 sID, AE_PARAMETER_UPDATE paraItem)
{
	sID = AE_CheckSensorID(sID);
	AE_SetParamUpdateFlag(sID, paraItem);
}


CVI_S32 AE_SetRawDumpFrameID(CVI_U8 sID, CVI_U32 fid, CVI_U16 frmNum)
{
	sID = AE_CheckSensorID(sID);
	AeInfo[sID].u32RawId = fid;
	AeInfo[sID].u16RawNum = frmNum;
	return CVI_SUCCESS;
}

void AE_SetRawReplayMode(CVI_U8 sID, CVI_BOOL mode)
{
	AeInfo[sID].bIsRawReplay = mode;
}

CVI_BOOL AE_isRawReplayMode(CVI_U8 sID)
{
	sID = AE_CheckSensorID(sID);
	return AeInfo[sID].bIsRawReplay;
}

static void SetReplayExposure(CVI_U8 sID)
{
	AeInfo[sID].u32AGainMin = 1024;
	AeInfo[sID].u32AGainMax = 32768 * 1024;
	AeInfo[sID].u32DGainMin = 1024;
	AeInfo[sID].u32DGainMax = 32768 * 1024;
	AeInfo[sID].u32ISPDGainMin = 1024;
	AeInfo[sID].u32ISPDGainMax = 32768 * 1024;
	AeInfo[sID].u32ExpTimeMax = 100000;
	AeInfo[sID].u32ExpTimeMin = 0;
	AeInfo[sID].u32WDRSEExpTimeMax = 100000;
	AeInfo[sID].fFps = AeInfo[sID].fDefaultFps;
	AeInfo[sID].stExp[AE_LE].stSensorExpGain.u32AGain = stReplayExpInfo.u32AGain;
	AeInfo[sID].stExp[AE_LE].stSensorExpGain.u32DGain = stReplayExpInfo.u32DGain;
	AeInfo[sID].stExp[AE_LE].stSensorExpGain.u32ISPDGain = stReplayExpInfo.u32ISPDGain;
	AeInfo[sID].stExp[AE_LE].stExpGain.u32AGain = stReplayExpInfo.u32AGain;
	AeInfo[sID].stExp[AE_LE].stExpGain.u32DGain = stReplayExpInfo.u32DGain;
	AeInfo[sID].stExp[AE_LE].stExpGain.u32ISPDGain = stReplayExpInfo.u32ISPDGain;
	AeInfo[sID].stExp[AE_LE].u32SensorExpTime = stReplayExpInfo.u32ExpTime;
	AeInfo[sID].stExp[AE_LE].u32ExpTime = stReplayExpInfo.u32ExpTime;
	AeInfo[sID].stExp[AE_LE].u32SensorExpTimeLine = stReplayExpInfo.u32ExpTime;
	if (AeInfo[sID].bWDRMode) {
		AeInfo[sID].stExp[AE_SE].stSensorExpGain.u32AGain = stReplayExpInfo.u32AGainSF;
		AeInfo[sID].stExp[AE_SE].stSensorExpGain.u32DGain = stReplayExpInfo.u32DGainSF;
		AeInfo[sID].stExp[AE_SE].stSensorExpGain.u32ISPDGain = stReplayExpInfo.u32ISPDGainSF;
		AeInfo[sID].stExp[AE_SE].stExpGain.u32AGain = stReplayExpInfo.u32AGainSF;
		AeInfo[sID].stExp[AE_SE].stExpGain.u32DGain = stReplayExpInfo.u32DGainSF;
		AeInfo[sID].stExp[AE_SE].stExpGain.u32ISPDGain = stReplayExpInfo.u32ISPDGainSF;
		AeInfo[sID].stExp[AE_SE].u32SensorExpTime = stReplayExpInfo.u32ShortExpTime;
		AeInfo[sID].stExp[AE_SE].u32ExpTime = stReplayExpInfo.u32ShortExpTime;
		AeInfo[sID].stExp[AE_SE].u32SensorExpTimeLine = stReplayExpInfo.u32ShortExpTime;
	}
}

CVI_S32 AE_SetRawReplayExposure(CVI_U8 sID, const ISP_EXP_INFO_S *pstExpInfo)
{
	sID = AE_CheckSensorID(sID);

	stReplayExpInfo = *pstExpInfo;

	SetReplayExposure(sID);

	return CVI_SUCCESS;
}

CVI_S32 AE_MallocRawReplayExpBuf(CVI_U8 sID, CVI_U16 frmNum)
{
	sID = AE_CheckSensorID(sID);

	if (frmNum < AeInfo[sID].u8SensorPeriod) {
		return CVI_FAILURE;
	}


	pstRawReplayExpInfo[sID] = (AE_RAW_REPLAY_EXP_INFO *)AE_Malloc(sID, sizeof(AE_RAW_REPLAY_EXP_INFO) * frmNum);
	if (pstRawReplayExpInfo[sID] == CVI_NULL) {
		return CVI_FAILURE;
	}

	aeRawReplayTableSize[sID] = frmNum;
	bTableUpdateEnable[sID] = 1;

	return CVI_SUCCESS;
}

CVI_S32 AE_GetRawReplayExpBuf(CVI_U8 sID, CVI_U8 *buf, CVI_U32 *bufSize)
{
	CVI_U16 i, ispDgainIndex = 0, tableIdex = 0;

	if (pstRawReplayExpInfo[sID] == CVI_NULL) {
		*bufSize = 0;
		return CVI_FAILURE;
	}

	sID = AE_CheckSensorID(sID);

	if (AE_LogBuf == CVI_NULL || AE_SnapLogBuf == CVI_NULL) {
		AE_MemoryAlloc(sID, AE_LOG_ITEM);
	}

	AE_SnapLogInit();

	for (i = 0; i < aeRawReplayTableSize[sID]; ++i) {
		//raw dump frame id is 1 early than ae
		if (AeInfo[sID].u32RawId - AeInfo[sID].u8SensorPeriod == pstRawReplayExpInfo[sID][i].u32FrameID + 1) {
			ispDgainIndex = i;
			#ifdef FRAME_ID_CHECK
			printf("find %d %d %d\n", AeInfo[sID].u32RawId, ispDgainIndex, AeInfo[sID].u16RawNum);
			#endif
			break;
		}
	}

	if (i == aeRawReplayTableSize[sID]) {
		bTableUpdateEnable[sID] = 0;
		*bufSize = 0;
		AE_Free(sID, pstRawReplayExpInfo[sID]);
		pstRawReplayExpInfo[sID] = CVI_NULL;
		ISP_LOG_ERR("can't find fid:%d Ispdgain!\n", AeInfo[sID].u32RawId);
		AeInfo[sID].u16RawNum = 0;
		return CVI_FAILURE;
	}

	AE_SnapLogPrintf("Time0\tAG0\tDG0\tIG0\tTime1\tAG1\tDG1\tIG1\tISO\tLV\n");

	for (i = 0; i < AeInfo[sID].u16RawNum; ++i) {
		tableIdex = (ispDgainIndex + i) % aeRawReplayTableSize[sID];
		AE_SnapLogPrintf("%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%d\n",
			pstRawReplayExpInfo[sID][tableIdex].u32Time[AE_LE],
			pstRawReplayExpInfo[sID][tableIdex].u32AGain[AE_LE],
			pstRawReplayExpInfo[sID][tableIdex].u32DGain[AE_LE],
			pstRawReplayExpInfo[sID][tableIdex].u32ISPDGain[AE_LE],
			pstRawReplayExpInfo[sID][tableIdex].u32Time[AE_SE],
			pstRawReplayExpInfo[sID][tableIdex].u32AGain[AE_SE],
			pstRawReplayExpInfo[sID][tableIdex].u32DGain[AE_SE],
			pstRawReplayExpInfo[sID][tableIdex].u32ISPDGain[AE_SE],
			pstRawReplayExpInfo[sID][tableIdex].u32ISO,
			pstRawReplayExpInfo[sID][tableIdex].s16LvX100);
		#ifdef FRAME_ID_CHECK
		printf("%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%d\t%d\n", tableIdex,
			pstRawReplayExpInfo[sID][tableIdex].u32Time[AE_LE],
			pstRawReplayExpInfo[sID][tableIdex].u32AGain[AE_LE],
			pstRawReplayExpInfo[sID][tableIdex].u32DGain[AE_LE],
			pstRawReplayExpInfo[sID][tableIdex].u32ISPDGain[AE_LE],
			pstRawReplayExpInfo[sID][tableIdex].u32Time[AE_SE],
			pstRawReplayExpInfo[sID][tableIdex].u32AGain[AE_SE],
			pstRawReplayExpInfo[sID][tableIdex].u32DGain[AE_SE],
			pstRawReplayExpInfo[sID][tableIdex].u32ISPDGain[AE_SE],
			pstRawReplayExpInfo[sID][tableIdex].u32ISO,
			pstRawReplayExpInfo[sID][tableIdex].s16LvX100);
		#endif
	}
	memcpy(buf, AE_SnapLogBuf, AE_SnapLogTailIndex);
	*bufSize = AE_SnapLogTailIndex;
	AeInfo[sID].u16RawNum = AeInfo[sID].u16RawBufCnt = 0;
	bTableUpdateEnable[sID] = 0;
	AE_Free(sID, pstRawReplayExpInfo[sID]);
	pstRawReplayExpInfo[sID] = CVI_NULL;
	aeRawReplayTableSize[sID] = 0;

	return CVI_SUCCESS;
}

void AE_SetAeSimMode(CVI_BOOL bMode)
{
	sAe_sim.bIsSimAe = bMode;
}

CVI_BOOL AE_IsAeSimMode(void)
{
	return sAe_sim.bIsSimAe;
}

void AE_SetAeSimEv(CVI_U8 sID)
{
	CVI_FLOAT v1, v2, v3;

	v1 = 33333.0/(CVI_FLOAT)AeInfo[sID].stExp[AE_LE].u32SensorExpTime;
	v2 = log(v1)/log(2) * 100 + 512;
	sAe_sim.ae_wantTv[AE_LE] = v2;

	v1 = 33333.0/(CVI_FLOAT)AeInfo[sID].stExp[AE_SE].u32SensorExpTime;
	v2 = log(v1)/log(2) * 100 + 512;
	sAe_sim.ae_wantTv[AE_SE] = v2;

	v1 = AeInfo[sID].stExp[AE_LE].stExpGain.u32AGain/(CVI_FLOAT)(1024);
	v2 = AeInfo[sID].stExp[AE_LE].stExpGain.u32DGain/(CVI_FLOAT)(1024);
	v3 = log(v1*v2)/log(2) * 100;
	sAe_sim.ae_wantSv[AE_LE] = v3;

	v1 = AeInfo[sID].stExp[AE_SE].stExpGain.u32AGain/(CVI_FLOAT)(1024);
	v2 = AeInfo[sID].stExp[AE_SE].stExpGain.u32DGain/(CVI_FLOAT)(1024);
	v3 = log(v1*v2)/log(2) * 100;
	sAe_sim.ae_wantSv[AE_SE] = v3;
#if 0
	printf("T:%d %d\n", AeInfo[sID].stExp[AE_LE].u32SensorExpTime,
	AeInfo[sID].stExp[AE_SE].u32SensorExpTime);
	printf("G:%d %d\n", AeInfo[sID].stExp[AE_LE].stExpGain.u32AGain,
		AeInfo[sID].stExp[AE_SE].stExpGain.u32AGain);
	printf("____id:%d %d %d\n", AeInfo[sID].u32frmCnt, AeInfo[sID].u16FrameLuma[0],
		AeInfo[sID].u16FrameLuma[1]);
	printf("____tv:%d %d %d %d\n", sAe_sim.ae_wantTv[0], sAe_sim.ae_wantTv[1], sAe_sim.ae_wantSv[0],
		sAe_sim.ae_wantSv[1]);
#endif
}

