#include <stdlib.h>
#include <stdio.h>
//#include <conio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include <windows.h>
#include <sys/time.h>//for gettimeofday()

#include "cvi_comm_video.h"
#include "cvi_type.h"
#include "cvi_isp.h"

#include "cvi_comm_isp.h"
#include "cvi_comm_3a.h"
#include "cvi_common.h"
#include "isp_main.h"
#include "isp_debug.h"
#include "cvi_awb.h"
#include "awbalgo.h"

#include "misc.h"
#include "bmp.h"
#include "simple_jpg.h"
#include "awb_platform.h"

int parseJudeTxt(char *judgeTxt, struct AWB_JUDGE_ST *stJudge)
{
	struct stat statbuf;
	int ret;
	char buf[1024];
	char *p = NULL;
	char *pjudge = buf;
	int idx = 0;

	memset(buf, 0, 1024);
	ret = stat(judgeTxt, &statbuf);
	if (ret != 0)
		return -1;

	FILE *fp = fopen(judgeTxt, "r");

	fread(buf, statbuf.st_size, 1, fp);
	fclose(fp);

	printf("%s\n", buf);

	do {
		if (pjudge == NULL)
			break;
		p = strsep(&pjudge, ",");

		if (idx == 0)
			stJudge->u16IdealR = atoi(p);
		else if (idx == 1)
			stJudge->u16IdealB = atoi(p);
		else if (idx == 2)
			stJudge->fIdealRThresh = atof(p);
		else
			stJudge->fIdealBThresh = atof(p);
		idx++;
	} while (p != NULL);
	return 0;
}

int main(int argc, char *argv[])
{
	if (argc < 4) {
		printf("arguments should be[judgeTxt, wbin, json, outputPath]\n");
		printf("*****************************************************\n");
		printf("judgeTxt -> idealRgain,idealBgain,idealRgain threshold,idealBgain threshold\n");
		printf("outputPath is optional, if not set, we output result to curPath/output\n");
		printf("*****************************************************\n");
	} else {
		if (argc == 5 || argc == 4) {
			struct AWB_JUDGE_ST stJudge;

			memset(&stJudge, 0, sizeof(struct AWB_JUDGE_ST));
			if (parseJudeTxt(argv[1], &stJudge) != -1) {
				if (argc == 5) {
					if ((access(argv[2], 0) == 0) &&
						(access(argv[3], 0) == 0) &&
						(access(argv[4], 0) == 0))
						AWB_DLL_Main(&stJudge, argv[2], argv[3], argv[4]);
					else if (access(argv[2], 0) != 0)
						printf("wbin not exist\n");
					else if (access(argv[3], 0) != 0)
						printf("json not exist\n");
				} else {
					AWB_DLL_Main(&stJudge, argv[2], argv[3], NULL);
				}
			} else {
				printf("judgeTxt not exist\n");
			}
		}
	}

	return 0;
}

