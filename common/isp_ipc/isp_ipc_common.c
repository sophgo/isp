
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/epoll.h>

#include "clog.h"
#include "isp_debug.h"
#include "isp_ipc_common.h"

#ifdef ENABLE_ISP_IPC

#ifndef UNUSED
#define UNUSED(x) ((void)(x))
#endif

//#ifdef ISP_LOG_ERR
//#undef ISP_LOG_ERR
//#define ISP_LOG_ERR printf
//#endif
//
//#ifdef ISP_LOG_INFO
//#undef ISP_LOG_INFO
//#define ISP_LOG_INFO printf
//#endif

#define ISP_IPC_TYPE_RET  0
#define ISP_IPC_TYPE_CMD  1

#define __USE_OPEN_WRITE 0

#define __MAX_CLIENT 8
#define __MAX_FIFO_NAME 32
#define __SERVER_FIFO_NAME "/tmp/ispfifo_0"

#define __SERVER_CMD_REG_CLIENT       0xFFFFFFF0
#define __SERVER_CMD_UNREG_CLIENT     0xFFFFFFF1
#define __SERVER_CMD_EXIT_SERVER_LOOP 0xFFFFFFF2

#define __CLIENT_CMD_REG_CLIENT       0xFFFFFFF3
#define __CLIENT_CMD_EXIT_CLIENT_LOOP 0xFFFFFFF4
#define __CLIENT_CMD_SERVER_EXIT      0xFFFFFFF5

typedef struct {
	CVI_BOOL valid;
	CVI_S32 pid;
	int fifo_s2c_fd;
	int fifo_c2s_fd;
	char fifo_s2c_name[__MAX_FIFO_NAME];
	char fifo_c2s_name[__MAX_FIFO_NAME];
	pthread_mutex_t lock;
	pthread_mutex_t cond_lock;
	pthread_condattr_t cond_attr;
	pthread_cond_t cond;
	CVI_S32 recv_ret;
	CVI_U8 *recv_res;
	CVI_U32 recv_res_len;
} CLIENT_INFO_S;

static CLIENT_INFO_S g_client_info_list[__MAX_CLIENT];
static CLIENT_INFO_S g_client_info;

static CVI_S32 isp_ipc_start_server_loop(void);
static CVI_S32 isp_ipc_stop_server_loop(void);

static CVI_S32 isp_ipc_start_client_loop(void);
static CVI_S32 isp_ipc_stop_client_loop(void);

static CVI_U32 g_server_cmd_list_len;
static ISP_IPC_CMD_ITEM_S *g_server_cmd_list;
static CVI_U32 g_client_cmd_list_len;
static ISP_IPC_CMD_ITEM_S *g_client_cmd_list;

CVI_S32 isp_ipc_reg_server_cmd(ISP_IPC_CMD_ITEM_S *cmd_list, CVI_U32 cmd_len)
{
	g_server_cmd_list = cmd_list;
	g_server_cmd_list_len = cmd_len;
	return CVI_SUCCESS;
}

CVI_S32 isp_ipc_reg_client_cmd(ISP_IPC_CMD_ITEM_S *cmd_list, CVI_U32 cmd_len)
{
	g_client_cmd_list = cmd_list;
	g_client_cmd_list_len = cmd_len;
	return CVI_SUCCESS;
}

// cmd
static void server_send_reg_client_cmd(CVI_U8 client_id);
static void server_send_server_exit_cmd(void);
static CVI_S32 client_cmd_reg_client(CVI_U8 *arg, CVI_U32 arg_len, CVI_U8 *res, CVI_U32 res_len);
static CVI_S32 client_cmd_server_exit(CVI_U8 *arg, CVI_U32 arg_len, CVI_U8 *res, CVI_U32 res_len);

// test
//extern void isp_ipc_server_reg_cmd(void);
//extern void isp_ipc_test_client_to_server(void);

static CVI_U8 g_server_init_cnt;
static CVI_U8 g_client_init_cnt;

CVI_S32 isp_ipc_server_init(void)
{
	CVI_S32 ret = CVI_SUCCESS;

	g_server_init_cnt++;

	if (g_server_init_cnt > 1) {
		return ret;
	}

	if (access(__SERVER_FIFO_NAME, F_OK) != 0) {
		if (mkfifo(__SERVER_FIFO_NAME, 0666) != 0) {
			ISP_LOG_ERR("mkfifo fail, %s\n", __SERVER_FIFO_NAME);
			return CVI_FAILURE;
		}
	} else {
		ISP_LOG_WARNING("%s exist...\n", __SERVER_FIFO_NAME);
	}

	ret = isp_ipc_start_server_loop();

	//isp_ipc_server_reg_cmd();

	return ret;
}

CVI_S32 isp_ipc_server_deinit(void)
{
	if (g_server_init_cnt > 1) {
		g_server_init_cnt--;
	} else if (g_server_init_cnt == 1) {
		g_server_init_cnt = 0;
		isp_ipc_stop_server_loop();
		unlink(__SERVER_FIFO_NAME);
	} else if (g_server_init_cnt == 0) {
		// not init
	}

	return CVI_SUCCESS;
}

CVI_S32 isp_ipc_client_init(void)
{
	CVI_S32 ret = CVI_SUCCESS;

	g_client_init_cnt++;

	if (g_client_info.valid) {
		return ret;
	}

	g_client_info.valid = CVI_TRUE;
	g_client_info.pid = getpid();
	snprintf(g_client_info.fifo_s2c_name, __MAX_FIFO_NAME, "/tmp/ispfifo_%d_s2c", g_client_info.pid);
	snprintf(g_client_info.fifo_c2s_name, __MAX_FIFO_NAME, "/tmp/ispfifo_%d_c2s", g_client_info.pid);

	if (mkfifo(g_client_info.fifo_s2c_name, 0666) != 0) {
		ISP_LOG_ERR("mkfifo fail, %s\n", g_client_info.fifo_s2c_name);
		ret = CVI_FAILURE;
	}

	g_client_info.fifo_s2c_fd = open(g_client_info.fifo_s2c_name, O_RDWR);

	if (g_client_info.fifo_s2c_fd < 0) {
		ISP_LOG_ERR("open %s fail\n", g_client_info.fifo_s2c_name);
		ret = CVI_FAILURE;
	}

	if (mkfifo(g_client_info.fifo_c2s_name, 0666) != 0) {
		ISP_LOG_ERR("mkfifo fail, %s\n", g_client_info.fifo_c2s_name);
		ret = CVI_FAILURE;
	}

#if __USE_OPEN_WRITE

#else
	//g_client_info.fifo_c2s_fd = open(g_client_info.fifo_c2s_name, O_RDWR);
	g_client_info.fifo_c2s_fd = open(g_client_info.fifo_c2s_name, O_RDWR | O_NONBLOCK);
	//g_client_info.fifo_c2s_fd = open(g_client_info.fifo_c2s_name, O_WRONLY | O_NONBLOCK);

	if (g_client_info.fifo_c2s_fd < 0) {
		ISP_LOG_ERR("open %s fail\n", g_client_info.fifo_c2s_name);
		ret = CVI_FAILURE;
	}
#endif
	if (ret == CVI_SUCCESS) {
		pthread_mutex_init(&g_client_info.lock, NULL);
		pthread_mutex_init(&g_client_info.cond_lock, NULL);
		pthread_condattr_init(&g_client_info.cond_attr);
		pthread_condattr_setclock(&g_client_info.cond_attr, CLOCK_MONOTONIC);
		pthread_cond_init(&g_client_info.cond, &g_client_info.cond_attr);
		ret = isp_ipc_start_client_loop();
	} else {

		if (g_client_info.fifo_s2c_fd > 0) {
			close(g_client_info.fifo_s2c_fd);
		}

		if (g_client_info.fifo_c2s_fd > 0) {
			close(g_client_info.fifo_c2s_fd);
		}

		unlink(g_client_info.fifo_s2c_name);
		unlink(g_client_info.fifo_c2s_name);
	}

	//isp_ipc_test_client_to_server();

	return ret;
}

CVI_S32 isp_ipc_client_deinit(void)
{
	if (g_client_init_cnt > 0) {
		g_client_init_cnt--;
	}

	if (g_client_init_cnt != 0) {
		return CVI_SUCCESS;
	}

	if (!g_client_info.valid) {
		return CVI_SUCCESS;
	}

	isp_ipc_stop_client_loop();

	if (g_client_info.fifo_s2c_fd > 0) {
		close(g_client_info.fifo_s2c_fd);
	}

	if (g_client_info.fifo_c2s_fd > 0) {
		close(g_client_info.fifo_c2s_fd);
	}

	unlink(g_client_info.fifo_s2c_name);
	unlink(g_client_info.fifo_c2s_name);

	pthread_mutex_destroy(&g_client_info.lock);
	pthread_mutex_destroy(&g_client_info.cond_lock);
	pthread_cond_destroy(&g_client_info.cond);
	memset(&g_client_info, 0, sizeof(CLIENT_INFO_S));

	return CVI_SUCCESS;
}

/*-------------------------------------- common -------------------------------------------*/
static CVI_S32 open_write_fifo(char *name, CVI_U8 *data, CVI_U32 len)
{
	CVI_S32 ret = CVI_SUCCESS;

	int fifo_fd = open(name, O_WRONLY | O_NONBLOCK);

	if (fifo_fd < 0) {
		ISP_LOG_ERR("open %s fail, ret = %d\n", name, fifo_fd);
		ret = CVI_FAILURE;
		goto open_write_fail;
	}

	ret = write(fifo_fd, data, len);
	if (ret != (CVI_S32) len) {
		ISP_LOG_ERR("write %s fail\n", name);
		ret = CVI_FAILURE;
		goto open_write_fail;
	}

open_write_fail:

	if (fifo_fd > 0) {
		close(fifo_fd);
	}

	return ret;
}

static CVI_S32 write_fifo(int fd, CVI_U8 *data, CVI_U32 len)
{
#define __WRITE_FIFO_RETRY_TIMES  1000 // 1000 ms
#define __WRITE_FIFO_WAIT_TIME_MS (1 * 1000)

	CVI_S32 retry_times = __WRITE_FIFO_RETRY_TIMES;
	CVI_S32 ret = CVI_SUCCESS;

	CVI_U32 write_count = 0;

	do {
		ret = write(fd, data + write_count, len - write_count);
		if (ret >= 0) {
			write_count += ret;
			if (write_count == len) {
				ret = CVI_SUCCESS;
				break;
			} else if (write_count > len) {
				ISP_LOG_ERR("write fifo unkonw error...\n");
				ret = CVI_FAILURE;
				break;
			}
			retry_times = __WRITE_FIFO_RETRY_TIMES;
			continue;
		} else {
			//ISP_LOG_ERR("write fifo error..., %d\n", ret);
			//ret = CVI_FAILURE;
			//break;
		}

		usleep(__WRITE_FIFO_WAIT_TIME_MS);

	} while (retry_times--);

	if (retry_times <= 0) {
		ISP_LOG_ERR("write fifo timeout...\n");
		ret = CVI_FAILURE;
	}

	return ret;
}

static CVI_S32 read_fifo(int fd, CVI_U8 *data, CVI_U32 len)
{
#define __READ_FIFO_RETRY_TIMES  1000 // 1000 ms
#define __READ_FIFO_WAIT_TIME_MS (1 * 1000)

	CVI_S32 retry_times = __READ_FIFO_RETRY_TIMES;
	CVI_S32 ret = CVI_SUCCESS;

	CVI_U32 read_count = 0;

	do {
		ret = read(fd, data + read_count, len - read_count);
		if (ret >= 0) {
			read_count += ret;
			if (read_count == len) {
				ret = CVI_SUCCESS;
				break;
			} else if (read_count > len) {
				ISP_LOG_ERR("read fifo unkonw error...\n");
				ret = CVI_FAILURE;
				break;
			}
			retry_times = __READ_FIFO_RETRY_TIMES;
			continue;
		} else {
			//ISP_LOG_ERR("read fifo error..., %d\n", ret);
			//ret = CVI_FAILURE;
			//break;
		}

		usleep(__READ_FIFO_WAIT_TIME_MS);

	} while (retry_times--);

	if (retry_times <= 0) {
		ISP_LOG_ERR("read fifo timeout...\n");
		ret = CVI_FAILURE;
	}

	return ret;
}

#define WAIT_COND_TIMEOUT_MS (1000)

static CVI_S32 wait_cond_timeout(CLIENT_INFO_S *client, CVI_S32 timeout_ms)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct timespec tv;

	clock_gettime(CLOCK_MONOTONIC, &tv);

	tv.tv_nsec += timeout_ms * 1000 * 1000;
	tv.tv_sec += tv.tv_nsec / (1000 * 1000 * 1000);
	tv.tv_nsec = tv.tv_nsec % (1000 * 1000 * 1000);

	pthread_mutex_lock(&client->cond_lock);
	ret = pthread_cond_timedwait(&client->cond, &client->cond_lock, &tv);
	pthread_mutex_unlock(&client->cond_lock);

	if (ret != 0) {
		ISP_LOG_ERR("wait cond timeout...\n");
		ret = CVI_FAILURE;
	}

	return ret;
}

static CVI_S32 cond_signal(CLIENT_INFO_S *client)
{
	CVI_S32 ret = CVI_SUCCESS;

	pthread_mutex_lock(&client->cond_lock);
	ret = pthread_cond_signal(&client->cond);
	pthread_mutex_unlock(&client->cond_lock);

	return ret;
}

/*-------------------------------------- server -------------------------------------------*/
static int g_max_fd;
static fd_set g_all_fds;
static CVI_S32 server_exec_reg_client_cmd(ISP_IPC_MSG_S *msg)
{
	CVI_S32 ret = CVI_SUCCESS;

	for (int i = 0; i < __MAX_CLIENT; i++) {
		if (g_client_info_list[i].valid &&
			g_client_info_list[i].pid == msg->pid) {
			ISP_LOG_ERR("already reg, %d\n", msg->pid);
			return CVI_FAILURE;
		}
	}

	for (int i = 0; i < __MAX_CLIENT; i++) {
		if (!g_client_info_list[i].valid) {
			snprintf(g_client_info_list[i].fifo_s2c_name, __MAX_FIFO_NAME, "/tmp/ispfifo_%d_s2c", msg->pid);
			snprintf(g_client_info_list[i].fifo_c2s_name, __MAX_FIFO_NAME, "/tmp/ispfifo_%d_c2s", msg->pid);

			g_client_info_list[i].fifo_c2s_fd = open(g_client_info_list[i].fifo_c2s_name, O_RDWR);

			if (g_client_info_list[i].fifo_c2s_fd < 0) {
				ISP_LOG_ERR("reg open %s, error\n", g_client_info_list[i].fifo_c2s_name);
				ret = CVI_FAILURE;
			}
#if __USE_OPEN_WRITE

#else
			//g_client_info_list[i].fifo_s2c_fd = open(g_client_info_list[i].fifo_s2c_name,
			//	O_RDWR);
			g_client_info_list[i].fifo_s2c_fd = open(g_client_info_list[i].fifo_s2c_name,
				O_RDWR | O_NONBLOCK);
			//g_client_info_list[i].fifo_s2c_fd = open(g_client_info_list[i].fifo_s2c_name,
			//	O_WRONLY | O_NONBLOCK);

			if (g_client_info_list[i].fifo_s2c_fd < 0) {
				ISP_LOG_ERR("reg open %s, error\n", g_client_info_list[i].fifo_s2c_name);
				ret = CVI_FAILURE;
			}
#endif
			if (ret == CVI_SUCCESS) {

				FD_SET(g_client_info_list[i].fifo_c2s_fd, &g_all_fds);
				if (g_client_info_list[i].fifo_c2s_fd > g_max_fd) {
					g_max_fd = g_client_info_list[i].fifo_c2s_fd;
				}

				if (ret == CVI_SUCCESS) {
					g_client_info_list[i].valid = CVI_TRUE;
					g_client_info_list[i].pid = msg->pid;
					pthread_mutex_init(&g_client_info_list[i].lock, NULL);
					pthread_mutex_init(&g_client_info_list[i].cond_lock, NULL);
					pthread_condattr_init(&g_client_info_list[i].cond_attr);
					pthread_condattr_setclock(&g_client_info_list[i].cond_attr,
						CLOCK_MONOTONIC);
					pthread_cond_init(&g_client_info_list[i].cond,
						&g_client_info_list[i].cond_attr);
					server_send_reg_client_cmd(i);
					ISP_LOG_INFO("reg success, %d, id: %d\n", msg->pid, i);
				}
			}

			if (ret != CVI_SUCCESS) {
				ISP_LOG_INFO("reg fail, %d, id: %d\n", msg->pid, i);

				if (g_client_info_list[i].fifo_c2s_fd > 0) {
					close(g_client_info_list[i].fifo_c2s_fd);
				}

				if (g_client_info_list[i].fifo_s2c_fd > 0) {
					close(g_client_info_list[i].fifo_s2c_fd);
				}
			}

			return CVI_SUCCESS;
		}
	}

	return CVI_SUCCESS;
}

static CVI_S32 server_exec_unreg_client_cmd(ISP_IPC_MSG_S *msg)
{
	CVI_S32 ret = CVI_SUCCESS;
	int i = 0;

	for (i = 0; i < __MAX_CLIENT; i++) {
		if (g_client_info_list[i].valid &&
			g_client_info_list[i].pid == msg->pid) {
			ISP_LOG_INFO("unreg, %d\n", msg->pid);

			FD_CLR(g_client_info_list[i].fifo_c2s_fd, &g_all_fds);

			if (g_client_info_list[i].fifo_s2c_fd > 0) {
				close(g_client_info_list[i].fifo_s2c_fd);
			}

			if (g_client_info_list[i].fifo_c2s_fd > 0) {
				close(g_client_info_list[i].fifo_c2s_fd);
			}

			pthread_mutex_destroy(&g_client_info_list[i].lock);
			pthread_mutex_destroy(&g_client_info_list[i].cond_lock);
			pthread_cond_destroy(&g_client_info_list[i].cond);
			memset(&g_client_info_list[i], 0, sizeof(CLIENT_INFO_S));
			break;
		}
	}

	if (i >= __MAX_CLIENT) {
		ISP_LOG_ERR("unreg fail, %d\n", msg->pid);
		ret = CVI_FAILURE;
	}

	return ret;
}

static pthread_t g_server_loop_t;
static int g_server_fifo_fd;

static CVI_S32 server_handle_cmd(CVI_BOOL *is_exit);
static CVI_S32 server_handle_client_cmd(int client_id);

static CVI_S32 lookup_client_id(int fd)
{
	for (int i = 0; i < __MAX_CLIENT; i++) {
		if (g_client_info_list[i].valid) {
			if (g_client_info_list[i].fifo_c2s_fd == fd) {
				return i;
			}
		}
	}

	ISP_LOG_ERR("lookup client id fail, %d\n", fd);

	return -1;
}

static void *server_loop(void *param)
{
	UNUSED(param);

	CVI_S32 ret = CVI_SUCCESS;
	fd_set read_fds;

	g_server_fifo_fd = open(__SERVER_FIFO_NAME, O_RDWR);

	if (g_server_fifo_fd < 0) {
		ISP_LOG_ERR("open %s fail\n", __SERVER_FIFO_NAME);
		goto server_loop_fail;
	}

	FD_ZERO(&g_all_fds);
	FD_SET(g_server_fifo_fd, &g_all_fds);
	g_max_fd = g_server_fifo_fd;

	CVI_BOOL is_exit = CVI_FALSE;

	while (!is_exit) {
		read_fds = g_all_fds;
		ret = select(g_max_fd + 1, &read_fds, NULL, NULL, NULL);
		if (ret < 0) {
			ISP_LOG_ERR("select wait fail\n");
			goto server_loop_fail;
		}

		for (int i = 0; i < g_max_fd + 1; i++) {
			if (FD_ISSET(i, &read_fds)) {
				if (i == g_server_fifo_fd) {
					server_handle_cmd(&is_exit);
				} else {
					server_handle_client_cmd(lookup_client_id(i));
				}
			}
		}
	}

server_loop_fail:

	if (g_server_fifo_fd > 0) {
		close(g_server_fifo_fd);
	}

	ISP_LOG_INFO("exit server loop finish...\n");

	return NULL;
}

static CVI_S32 isp_ipc_start_server_loop(void)
{
	CVI_S32 ret = CVI_SUCCESS;

	ret = pthread_create(&g_server_loop_t, NULL, server_loop, NULL);

	if (ret != 0) {
		ISP_LOG_ERR("create serve loop fail\n");
	}

	return ret;
}

static void server_exit_loop(void)
{
	ISP_IPC_MSG_S msg;

	memset(&msg, 0, sizeof(msg));

	ISP_LOG_INFO("server exit loop++\n");

	msg.pid = 0;
	msg.type = 0;
	msg.id = 0;
	msg.cmd = __SERVER_CMD_EXIT_SERVER_LOOP;
	msg.arg_len = 0;
	msg.res_len = 0;

	open_write_fifo(__SERVER_FIFO_NAME, (CVI_U8 *)&msg, sizeof(msg));

	ISP_LOG_INFO("server exit loop--\n");
}

static CVI_S32 isp_ipc_stop_server_loop(void)
{
	ISP_LOG_INFO("stop server loop++\n");

	server_send_server_exit_cmd();
	server_exit_loop();
	pthread_join(g_server_loop_t, NULL);

	ISP_LOG_INFO("stop server loop--\n");

	return CVI_SUCCESS;
}

static CVI_S32 server_handle_cmd(CVI_BOOL *is_exit)
{
	int fd;
	CVI_S32 read_size;
	ISP_IPC_MSG_S msg;

	fd = open(__SERVER_FIFO_NAME, O_RDONLY | O_NONBLOCK);

	if (fd < 0) {
		ISP_LOG_ERR("open %s fail\n", __SERVER_FIFO_NAME);
		return CVI_FAILURE;
	}

read_again:
	memset(&msg, 0, sizeof(msg));

	read_size = read(fd, &msg, sizeof(msg));

	if (read_size != sizeof(msg)) {
		if (read_size > 0) {
			ISP_LOG_ERR("read server fifo error, read_size: %d, %d\n", read_size, (CVI_S32) sizeof(msg));
		}
		close(fd);
		return read_size;
	}

	ISP_LOG_INFO("recv pid: %d, type: %d, id: %d, cmd: %d\n",
		msg.pid, msg.type, msg.id, msg.cmd);

	switch (msg.cmd) {
	case __SERVER_CMD_REG_CLIENT:
		{
			server_exec_reg_client_cmd(&msg);
			break;
		}
	case __SERVER_CMD_UNREG_CLIENT:
		{
			server_exec_unreg_client_cmd(&msg);
			break;
		}
	case __SERVER_CMD_EXIT_SERVER_LOOP:
		{
			*is_exit = CVI_TRUE;
			ISP_LOG_INFO("exit server loop...\n");
			break;
		}
	default:
		{
			ISP_LOG_ERR("server handle unkonw cmd...\n");
			break;
		}
	}

	goto read_again;
}

CVI_S32 server_send_cmd_to_client(ISP_IPC_MSG_S *msg, CVI_U8 *arg, CVI_U8 *res)
{
	CVI_S32 ret = CVI_SUCCESS;

	ISP_LOG_INFO("server send cmd to client, pid: %d, id: %d, cmd: %d\n",
		msg->pid, msg->id, msg->cmd);

	if (msg->id < __MAX_CLIENT &&
		g_client_info_list[msg->id].valid) {
		pthread_mutex_lock(&g_client_info_list[msg->id].lock);

		msg->pid = g_client_info_list[msg->id].pid;
		msg->type = ISP_IPC_TYPE_CMD;

		if (msg->res_len > 0 && res != NULL) {
			g_client_info_list[msg->id].recv_ret = CVI_SUCCESS;
			g_client_info_list[msg->id].recv_res = res;
			g_client_info_list[msg->id].recv_res_len = msg->res_len;
		}

		ISP_LOG_INFO("server send cmd to client, pid: %d, id: %d, cmd: %d\n",
			msg->pid, msg->id, msg->cmd);

		ISP_LOG_INFO("server write msg...\n");

#if __USE_OPEN_WRITE
		ret = open_write_fifo(g_client_info_list[msg->id].fifo_s2c_name, (CVI_U8 *)msg, sizeof(ISP_IPC_MSG_S));
		if (ret != sizeof(ISP_IPC_MSG_S)) {
			ISP_LOG_ERR("server write msg error...\n");
			goto send_fail;
		}
#else
		ret = write_fifo(g_client_info_list[msg->id].fifo_s2c_fd, (CVI_U8 *)msg, sizeof(ISP_IPC_MSG_S));
		if (ret != CVI_SUCCESS) {
			ISP_LOG_ERR("server write msg error...\n");
			goto send_fail;
		}
#endif

		if (msg->arg_len > 0 && arg != NULL) {
			ISP_LOG_INFO("server write arg, %d\n", msg->arg_len);
#if __USE_OPEN_WRITE
			ret = open_write_fifo(g_client_info_list[msg->id].fifo_s2c_name, arg, msg->arg_len);
			if (ret != (CVI_S32) msg->arg_len) {
				ISP_LOG_ERR("server write arg error...\n");
				goto send_fail;
			}
#else
			ret = write_fifo(g_client_info_list[msg->id].fifo_s2c_fd, arg, msg->arg_len);
			if (ret != CVI_SUCCESS) {
				ISP_LOG_ERR("server write arg error...\n");
				goto send_fail;
			}
#endif
		}

		if (msg->res_len > 0 && res != NULL) {
			ISP_LOG_INFO("server wait res++, %d\n", msg->res_len);
			ret = wait_cond_timeout(&g_client_info_list[msg->id], WAIT_COND_TIMEOUT_MS);
			if (ret == CVI_SUCCESS) {
				ret = g_client_info_list[msg->id].recv_ret;
			}
			ISP_LOG_INFO("server wait res--, %d\n", ret);
		}

send_fail:
		pthread_mutex_unlock(&g_client_info_list[msg->id].lock);
	} else {
		ISP_LOG_ERR("unkonw client...\n");
		ret = CVI_FAILURE;
	}

	return ret;
}

static CVI_S32 server_handle_client_cmd(int client_id)
{
	CVI_S32 ret;
	CVI_S32 read_size;
	ISP_IPC_MSG_S msg;

	CVI_U8 *arg = NULL;
	CVI_U8 *res = NULL;
	ISP_IPC_FUN fun = NULL;

	if (client_id < 0 || client_id > __MAX_CLIENT) {
		return CVI_FAILURE;
	}

	int fd = open(g_client_info_list[client_id].fifo_c2s_name, O_RDONLY | O_NONBLOCK);

read_again:
	memset(&msg, 0, sizeof(msg));

	read_size = read(fd, &msg, sizeof(msg));

	if (read_size != sizeof(msg)) {
		if (read_size > 0) {
			ISP_LOG_ERR("read server fifo error, read_size: %d, %d\n", read_size, (CVI_S32) sizeof(msg));
		}
		close(fd);
		return read_size;
	}

	ISP_LOG_INFO("recv pid: %d, type: %d, id: %d, cmd: %d, arg_len: %d, res_len: %d\n",
		msg.pid, msg.type, msg.id, msg.cmd, msg.arg_len, msg.res_len);

	if (msg.type == ISP_IPC_TYPE_RET) {
		ret = read_fifo(fd, g_client_info_list[msg.id].recv_res,
			g_client_info_list[msg.id].recv_res_len);
		if (ret != CVI_SUCCESS) {
			g_client_info_list[msg.id].recv_ret = CVI_FAILURE;
			ISP_LOG_ERR("server read res error...\n");
		}

		cond_signal(&g_client_info_list[msg.id]);

		goto read_again;
	}

	if (g_server_cmd_list != CVI_NULL &&
		msg.cmd < g_server_cmd_list_len &&
		msg.cmd == g_server_cmd_list[msg.cmd].cmd &&
		msg.id < __MAX_CLIENT &&
		g_client_info_list[msg.id].valid &&
		msg.pid == g_client_info_list[msg.id].pid) {
		fun = g_server_cmd_list[msg.cmd].fun;
	} else {
		ISP_LOG_ERR("cmd error\n");
		if (msg.id < __MAX_CLIENT &&
			(!g_client_info_list[msg.id].valid ||
			 msg.pid != g_client_info_list[msg.id].pid)) {
			ISP_LOG_ERR("unkonw client, reg first...\n");
		}
		goto read_again;
	}

	if (msg.type == ISP_IPC_TYPE_CMD) {

		// read arg
		if (msg.arg_len > 0) {
			arg = calloc(1, msg.arg_len);
			ret = read_fifo(fd, arg, msg.arg_len);
			if (ret != CVI_SUCCESS) {
				ISP_LOG_ERR("server read arg error...\n");
				goto read_again;
			}
		}

		if (msg.res_len > 0) {
			res = calloc(1, msg.res_len);
		} else {
			res = (CVI_U8 *) &msg; // !!!
		}

		// exec cmd
		ret = fun(arg, msg.arg_len, res, msg.res_len);

		// write res
		if (msg.res_len > 0) {
			msg.type = ISP_IPC_TYPE_RET;
#if __USE_OPEN_WRITE
			ret = open_write_fifo(g_client_info_list[msg.id].fifo_s2c_name, (CVI_U8 *)&msg, sizeof(msg));
			if (ret != sizeof(msg)) {
				ISP_LOG_ERR("server write msg error...\n");
			}

			ret = open_write_fifo(g_client_info_list[msg.id].fifo_s2c_name, res, msg.res_len);
			if (ret != (CVI_S32) msg.res_len) {
				ISP_LOG_ERR("server write res error...\n");
			}
#else
			ret = write_fifo(g_client_info_list[msg.id].fifo_s2c_fd, (CVI_U8 *)&msg, sizeof(msg));
			if (ret != CVI_SUCCESS) {
				ISP_LOG_ERR("server write msg error...\n");
			}

			ret = write_fifo(g_client_info_list[msg.id].fifo_s2c_fd, res, msg.res_len);
			if (ret != CVI_SUCCESS) {
				ISP_LOG_ERR("server write res error...\n");
			}
#endif
		}

		if (arg != NULL) {
			free(arg);
		}

		if (msg.res_len > 0 && res != NULL) {
			free(res);
		}

		goto read_again;
	}

	goto read_again;
}

/*-------------------------------------- clinet -----------------------------------------*/
static CVI_U8 g_client_id;
static CVI_BOOL g_client_is_ready;

static CVI_S32 client_handle_server_cmd(CVI_BOOL *is_exit);

static pthread_t g_client_loop_t;

static void *client_loop(void *param)
{
	UNUSED(param);

	CVI_S32 ret = CVI_SUCCESS;
	int max_fd = -1;
	fd_set read_fds, all_fds;

	FD_ZERO(&all_fds);
	FD_SET(g_client_info.fifo_s2c_fd, &all_fds);
	max_fd = g_client_info.fifo_s2c_fd;

	CVI_BOOL is_exit = CVI_FALSE;

	while (!is_exit) {

		read_fds = all_fds;

		ret = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
		if (ret < 0) {
			ISP_LOG_ERR("select wait fail\n");
			goto client_loop_fail;
		}

		for (int i = 0; i < max_fd + 1; i++) {
			if (FD_ISSET(i, &read_fds)) {
				if (i == g_client_info.fifo_s2c_fd) {
					client_handle_server_cmd(&is_exit);
				}
			}
		}
	}

client_loop_fail:

	g_client_is_ready = CVI_FALSE;

	ISP_LOG_INFO("client loop finish...\n");

	return NULL;
}

CVI_S32 client_send_cmd_to_server(ISP_IPC_MSG_S *msg, CVI_U8 *arg, CVI_U8 *res)
{
	CVI_S32 ret = CVI_SUCCESS;

	pthread_mutex_lock(&g_client_info.lock);

	if (!g_client_is_ready) {
		pthread_mutex_unlock(&g_client_info.lock);
		ISP_LOG_ERR("client not ready...\n");
		return CVI_FAILURE;
	}

	msg->pid = getpid();
	msg->type = ISP_IPC_TYPE_CMD;
	msg->id = g_client_id;

	ISP_LOG_INFO("client send cmd, pid: %d, id: %d, cmd: %d\n",
		msg->pid, msg->id, msg->cmd);

	if (msg->res_len > 0 && res != NULL) {
		g_client_info.recv_ret = CVI_SUCCESS;
		g_client_info.recv_res = res;
		g_client_info.recv_res_len = msg->res_len;
	}

	ISP_LOG_INFO("client write msg...\n");
#if __USE_OPEN_WRITE
	ret = open_write_fifo(g_client_info.fifo_c2s_name, (CVI_U8 *)msg, sizeof(ISP_IPC_MSG_S));
	if (ret != sizeof(ISP_IPC_MSG_S)) {
		ISP_LOG_ERR("client write msg error...\n");
		goto send_fail;
	}
#else
	ret = write_fifo(g_client_info.fifo_c2s_fd, (CVI_U8 *)msg, sizeof(ISP_IPC_MSG_S));
	if (ret != CVI_SUCCESS) {
		ISP_LOG_ERR("client write msg error...\n");
		goto send_fail;
	}
#endif

	if (msg->arg_len > 0 && arg != NULL) {
		ISP_LOG_INFO("client write arg, %d\n", msg->arg_len);
#if __USE_OPEN_WRITE
		ret = open_write_fifo(g_client_info.fifo_c2s_name, arg, msg->arg_len);
		if (ret != (CVI_S32) msg->arg_len) {
			ISP_LOG_ERR("client write arg error...\n");
			goto send_fail;
		}
#else
		ret = write_fifo(g_client_info.fifo_c2s_fd, arg, msg->arg_len);
		if (ret != CVI_SUCCESS) {
			ISP_LOG_ERR("client write arg error...\n");
			goto send_fail;
		}
#endif
	}

	if (msg->res_len > 0 && res != NULL) {
		ISP_LOG_INFO("client wait res++, %d\n", msg->res_len);
		ret = wait_cond_timeout(&g_client_info, WAIT_COND_TIMEOUT_MS);
		if (ret == CVI_SUCCESS) {
			ret = g_client_info.recv_ret;
		}
		ISP_LOG_INFO("client wait res--, %d\n", ret);
	}

send_fail:
	pthread_mutex_unlock(&g_client_info.lock);

	return ret;
}


static CVI_S32 client_handle_server_cmd(CVI_BOOL *is_exit)
{
	CVI_S32 ret;
	CVI_S32 read_size;
	ISP_IPC_MSG_S msg;

	CVI_U8 *arg = NULL;
	CVI_U8 *res = NULL;
	ISP_IPC_FUN fun = NULL;

	int fd = open(g_client_info.fifo_s2c_name, O_RDONLY | O_NONBLOCK);

	if (fd < 0) {
		ISP_LOG_ERR("open %s fail!\n", g_client_info.fifo_s2c_name);
		return CVI_FAILURE;
	}

read_again:
	memset(&msg, 0, sizeof(msg));

	read_size = read(fd, &msg, sizeof(msg));

	if (read_size != sizeof(msg)) {
		if (read_size > 0) {
			ISP_LOG_ERR("read server fifo error, read_size: %d, %d\n", read_size, (CVI_S32) sizeof(msg));
		}
		close(fd);
		return read_size;
	}

	if (msg.pid != g_client_info.pid) {
		ISP_LOG_ERR("unkonw msg..., %d, %d\n", msg.pid, g_client_info.pid);
		goto read_again;
	}

	ISP_LOG_INFO("recv pid: %d, type: %d, id: %d, cmd: %d, arg_len: %d, res_len: %d\n",
		msg.pid, msg.type, msg.id, msg.cmd, msg.arg_len, msg.res_len);

	if (msg.type == ISP_IPC_TYPE_RET) {
		ret = read_fifo(fd, g_client_info.recv_res,
			g_client_info.recv_res_len);
		if (ret != CVI_SUCCESS) {
			g_client_info.recv_ret = CVI_FAILURE;
			ISP_LOG_ERR("client read res error...\n");
		}

		cond_signal(&g_client_info);

		goto read_again;
	}

	switch (msg.cmd) {
	case __CLIENT_CMD_REG_CLIENT:
		fun = client_cmd_reg_client;
		break;
	case __CLIENT_CMD_EXIT_CLIENT_LOOP:
		*is_exit = CVI_TRUE;
		ISP_LOG_INFO("exit client loop\n");
		return CVI_SUCCESS;
	case __CLIENT_CMD_SERVER_EXIT:
		fun = client_cmd_server_exit;
		break;
	default:
		{
			if (g_client_cmd_list != CVI_NULL &&
				msg.cmd < g_client_cmd_list_len &&
				msg.cmd == g_client_cmd_list[msg.cmd].cmd) {
				fun = g_client_cmd_list[msg.cmd].fun;
			} else {
				ISP_LOG_ERR("cmd error\n");
				goto read_again;
			}
		}
		break;
	}

	if (msg.type == ISP_IPC_TYPE_CMD) {

		// read arg
		if (msg.arg_len > 0) {
			arg = calloc(1, msg.arg_len);
			ret = read_fifo(fd, arg, msg.arg_len);
			if (ret != CVI_SUCCESS) {
				ISP_LOG_ERR("client read arg error...\n");
				goto read_again;
			}
		}

		if (msg.res_len > 0) {
			res = calloc(1, msg.res_len);
		} else {
			res = (CVI_U8 *) &msg; // !!!
		}

		// exec cmd
		ret = fun(arg, msg.arg_len, res, msg.res_len);

		// write res
		if (msg.res_len > 0) {
			msg.type = ISP_IPC_TYPE_RET;
#if __USE_OPEN_WRITE
			ret = open_write_fifo(g_client_info.fifo_c2s_name, (CVI_U8 *)&msg, sizeof(msg));
			if (ret != sizeof(msg)) {
				ISP_LOG_ERR("client write msg error...\n");
			}

			ret = open_write_fifo(g_client_info.fifo_c2s_name, res, msg.res_len);
			if (ret != (CVI_S32) msg.res_len) {
				ISP_LOG_ERR("client write res error...\n");
			}
#else
			ret = write_fifo(g_client_info.fifo_c2s_fd, (CVI_U8 *)&msg, sizeof(msg));
			if (ret != CVI_SUCCESS) {
				ISP_LOG_ERR("client write msg error...\n");
			}

			ret = write_fifo(g_client_info.fifo_c2s_fd, res, msg.res_len);
			if (ret != CVI_SUCCESS) {
				ISP_LOG_ERR("client write res error...\n");
			}
#endif
		}

		if (arg != NULL) {
			free(arg);
		}

		if (msg.res_len > 0 && res != NULL) {
			free(res);
		}

		goto read_again;
	}

	goto read_again;
}

static CVI_S32 client_reg(void);
static CVI_S32 client_unreg(void);
static CVI_S32 client_exit_loop(void);

//static void *__reg_thread(void *param)
//{
//	UNUSED(param);
//
//	while (1) {
//		if (access(__SERVER_FIFO_NAME, F_OK) == 0) {
//			client_reg();
//			break;
//		} else {
//			sleep(1);
//		}
//	}
//
//	return NULL;
//}

static CVI_S32 isp_ipc_start_client_loop(void)
{
	CVI_S32 ret = CVI_SUCCESS;
	//pthread_t tid;

	ret = pthread_create(&g_client_loop_t, NULL, client_loop, NULL);

	if (ret != 0) {
		ISP_LOG_ERR("create client loop fail\n");
	} else {
		if (access(__SERVER_FIFO_NAME, F_OK) != 0) {
			//ISP_LOG_ERR("isp server not ready, start reg thread...\n");
			//pthread_create(&tid, NULL, __reg_thread, NULL);
			ISP_LOG_ERR("isp server not ready....\n");
			ret = CVI_FAILURE;
		} else {
			ret = client_reg();

			if (ret == CVI_SUCCESS) {
				ISP_LOG_INFO("client wait reg ready...\n");
				ret = wait_cond_timeout(&g_client_info, WAIT_COND_TIMEOUT_MS);
			}
		}
	}

	return ret;
}

static CVI_S32 isp_ipc_stop_client_loop(void)
{
	if (access(__SERVER_FIFO_NAME, F_OK) == 0) {
		client_unreg();
	}
	client_exit_loop();
	pthread_join(g_client_loop_t, NULL);

	return CVI_SUCCESS;
}

static CVI_S32 client_reg(void)
{
	ISP_IPC_MSG_S msg;
	CVI_S32 ret = CVI_SUCCESS;

	memset(&msg, 0, sizeof(msg));

	ISP_LOG_INFO("client reg++\n");

	msg.pid = getpid();
	msg.type = ISP_IPC_TYPE_CMD;
	msg.id = 0;
	msg.cmd = __SERVER_CMD_REG_CLIENT;
	msg.arg_len = 0;
	msg.res_len = 0;

	ret = open_write_fifo(__SERVER_FIFO_NAME, (CVI_U8 *)&msg, sizeof(msg));

	if (ret != CVI_FAILURE) {
		ret = CVI_SUCCESS;
	}

	ISP_LOG_INFO("client reg--, ret: %d\n", ret);

	return ret;
}

static CVI_S32 client_unreg(void)
{
	ISP_IPC_MSG_S msg;
	CVI_S32 ret = CVI_SUCCESS;

	memset(&msg, 0, sizeof(msg));

	ISP_LOG_INFO("client unreg++\n");

	msg.pid = getpid();
	msg.type = ISP_IPC_TYPE_CMD;
	msg.id = 0;
	msg.cmd = __SERVER_CMD_UNREG_CLIENT;
	msg.arg_len = 0;
	msg.res_len = 0;

	ret = open_write_fifo(__SERVER_FIFO_NAME, (CVI_U8 *)&msg, sizeof(msg));

	if (ret != CVI_FAILURE) {
		ret = CVI_SUCCESS;
	}

	ISP_LOG_INFO("client unreg--\n");

	return ret;
}

static CVI_S32 client_exit_loop(void)
{
	ISP_IPC_MSG_S msg;
	CVI_S32 ret = CVI_SUCCESS;

	memset(&msg, 0, sizeof(msg));

	ISP_LOG_INFO("client exit loop++\n");

	pthread_mutex_lock(&g_client_info.lock);
	g_client_is_ready = CVI_FALSE;
	pthread_mutex_unlock(&g_client_info.lock);

	msg.pid = getpid();
	msg.type = ISP_IPC_TYPE_CMD;
	msg.id = g_client_id;
	msg.cmd = __CLIENT_CMD_EXIT_CLIENT_LOOP;
	msg.arg_len = 0;
	msg.res_len = 0;

	ret = open_write_fifo(g_client_info.fifo_s2c_name, (CVI_U8 *)&msg, sizeof(msg));

	if (ret != CVI_FAILURE) {
		ret = CVI_SUCCESS;
	}

	ISP_LOG_INFO("client exit loop--\n");

	return ret;
}

/*----------------------------------------- cmd ---------------------------------------------*/
static void *__server_send_reg_client_cmd(void *param)
{
#if __SIZEOF_POINTER__ == 8
	CVI_U8 client_id = (CVI_U8) (CVI_U64) param;
#else
	CVI_U8 client_id = (CVI_U8) (CVI_U32) param;
#endif

	CVI_S32 res = CVI_SUCCESS;
	ISP_IPC_MSG_S msg;

	ISP_LOG_INFO("server send reg client cmd++\n");

	memset(&msg, 0, sizeof(msg));

	msg.id = client_id;
	msg.cmd = __CLIENT_CMD_REG_CLIENT;
	msg.arg_len = sizeof(msg);
	msg.res_len = sizeof(CVI_S32);

	server_send_cmd_to_client(&msg, (CVI_U8 *)&msg, (CVI_U8 *)&res);

	ISP_LOG_INFO("server send reg client cmd--\n");

	return NULL;
}

static void server_send_reg_client_cmd(CVI_U8 client_id)
{
	pthread_t tid;

#if __SIZEOF_POINTER__ == 8
	pthread_create(&tid, NULL, __server_send_reg_client_cmd, (void *) (CVI_U64) client_id);
#else
	pthread_create(&tid, NULL, __server_send_reg_client_cmd, (void *) (CVI_U32) client_id);
#endif
}

static void server_send_server_exit_cmd(void)
{
	int i = 0;
	ISP_IPC_MSG_S msg;
	CVI_S32 res = CVI_SUCCESS;

	memset(&msg, 0, sizeof(msg));

	for (i = 0; i < __MAX_CLIENT; i++) {
		if (g_client_info_list[i].valid) {
			msg.id = i;
			msg.cmd = __CLIENT_CMD_SERVER_EXIT;
			msg.arg_len = sizeof(msg);
			msg.res_len = sizeof(CVI_S32);
			ISP_LOG_INFO("notify client exit, %d\n", i);
			server_send_cmd_to_client(&msg, (CVI_U8 *)&msg, (CVI_U8 *)&res);
			ISP_LOG_INFO("notify client exit, %d, res: %d\n", i, res);

			if (g_client_info_list[i].fifo_s2c_fd > 0) {
				close(g_client_info_list[i].fifo_s2c_fd);
			}

			if (g_client_info_list[i].fifo_c2s_fd > 0) {
				close(g_client_info_list[i].fifo_c2s_fd);
			}

			pthread_mutex_destroy(&g_client_info_list[i].lock);
			pthread_mutex_destroy(&g_client_info_list[i].cond_lock);
			pthread_cond_destroy(&g_client_info_list[i].cond);
			memset(&g_client_info_list[i], 0, sizeof(CLIENT_INFO_S));
		}
	}
}

static CVI_S32 client_cmd_reg_client(CVI_U8 *arg, CVI_U32 arg_len, CVI_U8 *res, CVI_U32 res_len)
{
	ISP_LOG_INFO("client cmd reg client++, %d, %d\n", arg_len, res_len);

	ISP_IPC_MSG_S *msg = (ISP_IPC_MSG_S *) arg;

	g_client_id = msg->id;
	g_client_is_ready = CVI_TRUE;

	*((CVI_S32 *) res) = CVI_SUCCESS;

	cond_signal(&g_client_info);

	ISP_LOG_INFO("client cmd reg client--, id: %d\n", g_client_id);

	return CVI_SUCCESS;
}

static ISP_IPC_FUN g_server_exit_callback;

CVI_S32 client_reg_server_exit_callback(ISP_IPC_FUN fun)
{
	g_server_exit_callback = fun;
	return CVI_SUCCESS;
}

static CVI_S32 client_cmd_server_exit(CVI_U8 *arg, CVI_U32 arg_len, CVI_U8 *res, CVI_U32 res_len)
{
	UNUSED(arg);
	UNUSED(arg_len);
	UNUSED(res_len);

	ISP_LOG_INFO("server exit...\n");

	g_client_is_ready = CVI_FALSE;
	*((CVI_S32 *) res) = CVI_SUCCESS;

	if (g_server_exit_callback != NULL) {
		g_server_exit_callback(NULL, 0, NULL, 0);
	}

	return CVI_SUCCESS;
}

#endif
