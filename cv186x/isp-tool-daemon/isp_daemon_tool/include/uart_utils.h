/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2022. All rights reserved.
 *
 * File Name: uart_utils.h
 * Description:
 */

#ifndef _UART_UTILS_H_
#define _UART_UTILS_H_

int isp_daemon2_init_uart(char *uart_name, int single_output);
void isp_daemon2_uninit_uart(void);
void console_recover(void);

#endif // _UART_UTILS_H_
