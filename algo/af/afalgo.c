/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: afalgo.c
 * Description:
 *
 */

#define AF_LIB_VER     (1)//U8
#define AF_LIB_SUBVER  (1)//U8

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <sys/time.h> //for gettimeofday()
#include <unistd.h> //for usleep()
#include "cvi_isp.h"
#include "cvi_comm_3a.h"
#include "isp_main.h"
#include "isp_debug.h"
#include "cvi_base.h"

#include "afalgo.h"
void AF_GetAlgoVer(CVI_U16 *pVer, CVI_U16 *pSubVer)
{
	*pVer = AF_LIB_VER;
	*pSubVer =  AF_LIB_SUBVER;
}

