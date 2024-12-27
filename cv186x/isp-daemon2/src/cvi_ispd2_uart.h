/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2022. All rights reserved.
 *
 * File Name: cvi_ispd2_uart.h
 * Description:
 */

#ifndef _CVI_ISPD2_UART_H_
#define _CVI_ISPD2_UART_H_

#include "cvi_ispd2_local.h"

void CVI_ISPD2_InitialDaemonInfo(TISPDaemon2Info *ptObject);
void CVI_ISPD2_ReleaseDaemonInfo(TISPDaemon2Info *ptObject);
TISPDaemon2Info *CVI_ISPD2_Uart_GetDaemon2Info(void);
CVI_U32 uart_recv(CVI_U32 fd, char *buf, CVI_U32 len);

#endif // _CVI_ISPD2_UART_H_
