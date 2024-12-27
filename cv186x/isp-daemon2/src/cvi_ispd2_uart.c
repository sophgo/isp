/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2022. All rights reserved.
 *
 * File Name: cvi_ispd2_uart.c
 * Description:
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "cvi_ispd2_uart.h"
#include "cvi_ispd2_event_server.h"
#include "cvi_ispd2_callback_funcs_apps.h"
#include "cvi_ispd2_callback_funcs_dump.h"

#define UART_RECV_LEN 32

TISPDaemon2Info		gtObject_uart = {0};
int			guart_run_state;

// -----------------------------------------------------------------------------
static void isp_daemon2_uart_set_fd(TISPDaemon2Info *ptObject, CVI_S32 fd)
{
	ptObject->uart_fd = fd;
}

// -----------------------------------------------------------------------------
void isp_daemon2_uart_init(int fd)
{
	CVI_ISPD2_InitialDaemonInfo(&gtObject_uart);
	isp_daemon2_uart_set_fd(&gtObject_uart, fd);
	CVI_ISPD2_ConfigMessageHandler(&(gtObject_uart.tHandlerInfo));
	CVI_ISPD2_ES_RunService_Uart(&gtObject_uart);
	guart_run_state = CVI_TRUE;
}

// -----------------------------------------------------------------------------
void isp_daemon2_uart_uninit(void)
{
	CVI_ISPD2_ES_DestoryService_Uart(&gtObject_uart);
	CVI_ISPD2_ReleaseDaemonInfo(&gtObject_uart);
}

// -----------------------------------------------------------------------------
void isp_daemon2_enable_device_bind_control_uart(int enable)
{
	TISPDeviceInfo	*ptDeviceInfo = &(gtObject_uart.tDeviceInfo);

	ptDeviceInfo->bVPSSBindCtrl = (enable) ? CVI_TRUE : CVI_FALSE;
}

// -----------------------------------------------------------------------------

TISPDaemon2Info *CVI_ISPD2_Uart_GetDaemon2Info(void)
{
	return &gtObject_uart;
}

int isp_daemon2_get_uart_run_state(void)
{
	return guart_run_state;
}

int isp_daemon2_set_uart_run_state(int state)
{
	guart_run_state = state;
	return CVI_SUCCESS;
}

CVI_U32 uart_recv(CVI_U32 fd, char *buf, CVI_U32 len)
{
	char recv_data[UART_RECV_LEN + 1];
	CVI_U32 recv_total = 0;
	CVI_S32 recv_len;
	fd_set fs_read;
	struct timeval time;

	FD_ZERO(&fs_read);
	FD_SET(fd, &fs_read);

	time.tv_sec = 1;
	time.tv_usec = 0;

	while (select(fd + 1, &fs_read, NULL, NULL, &time) > 0) {
		memset(recv_data, 0, UART_RECV_LEN + 1);
		recv_len = read(fd, recv_data, UART_RECV_LEN);

		if (recv_len > 0) {
			memcpy(buf + recv_total, recv_data, recv_len);
			recv_total += recv_len;
			// printf("recv_len:%d\n%s\n", recv_len, recv_data);
		}
		if (recv_total + UART_RECV_LEN >= len) {
			break;
		}
	}
	return recv_total;
}
