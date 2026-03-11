
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
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

#define CLOG_OUTPUT_LVL CLOG_LVL_WARN
#define CLOG_TAG "ISP_IPC"
#include "clog.h"
#include "isp_ipc_common.h"

#ifdef ENABLE_ISP_IPC

#ifndef UNUSED
#define UNUSED(x) ((void)(x))
#endif

static int __write_data(int fd, const void *buf, int len)
{
	int written = 0;
	while (written < len) {
		int ret = write(fd, (const char *)buf + written, len - written);
		if (ret < 0) {
			if (errno == EINTR) {
				continue; // Interrupted by signal, retry
			}
			return -1; // Write error
		}
		written += ret;
	}
	return 0; // Success
}

static int __read_data(int fd, void *buf, int len)
{
	int read_bytes = 0;
	while (read_bytes < len) {
		int ret = read(fd, (char *)buf + read_bytes, len - read_bytes);
		if (ret < 0) {
			if (errno == EINTR) {
				continue; // Interrupted by signal, retry
			}
			return -1; // Read error
		} else if (ret == 0) { // client disconnected
			return 0;
		}
		read_bytes += ret;
	}
	return read_bytes; // Success
}

/*----------------------------------server--------------------------------------*/
static CVI_U32 g_server_cmd_list_len;
static ISP_IPC_CMD_ITEM_S *g_server_cmd_list;

CVI_S32 isp_ipc_reg_server_cmd(ISP_IPC_CMD_ITEM_S *cmd_list, CVI_U32 cmd_len)
{
	g_server_cmd_list = cmd_list;
	g_server_cmd_list_len = cmd_len;
	return CVI_SUCCESS;
}

#define __MAX_CLIENT 8
#define __SOCKET_PATH "/tmp/isp_ipc.sock"
#define __MIGIC_NUMBER 0x55AA

static int g_server_running;
static pthread_t g_server_thread;

static int server_handle_client_cmd(int client_fd)
{
	int ret = 0;
	ISP_IPC_MSG_S msg;

	ret = __read_data(client_fd, &msg, sizeof(msg));
	if (ret < 0) {
		clog_e("Failed to read message from client fd=%d: %s\n", client_fd, strerror(errno));
		return -1; // Read error or client disconnected
	} if (ret == 0) {
		clog_i("Client fd=%d disconnected\n", client_fd);
		return -1; // Client disconnected
	}

	if (msg.magic != __MIGIC_NUMBER) {
		clog_e("Invalid magic number: expected 0x%04X, got 0x%04X\n", __MIGIC_NUMBER, msg.magic);
		return -1; // Invalid magic number
	}

	if (msg.cmd >= g_server_cmd_list_len) {
		clog_e("Invalid command: %d, max command is %d\n", msg.cmd, g_server_cmd_list_len - 1);
		return -1; // Invalid command
	}

	CVI_U8 *arg = NULL;
	CVI_U8 *res = NULL;
	ISP_IPC_FUN fun = g_server_cmd_list[msg.cmd].fun;

	clog_i("Received command: %d, arg_len: %u, res_len: %u\n", msg.cmd, msg.arg_len, msg.res_len);

	if (!fun) {
		clog_e("No function registered for command: %d\n", msg.cmd);
		ret = -1;
		goto fail;
	}

	// read argument data
	if (msg.arg_len > 0) {
		arg = calloc(1, msg.arg_len);
		if (!arg) {
			clog_e("Failed to allocate memory for argument data\n");
			ret = -1;
			goto fail;
		}
		ret = __read_data(client_fd, arg, msg.arg_len);
		if (ret <= 0) {
			clog_e("Failed to read argument data: %s\n", strerror(errno));
			goto fail;
		}
	}

	if (msg.res_len > 0) {
		res = calloc(1, msg.res_len);
		if (!res) {
			clog_e("Failed to allocate memory for response data\n");
			ret = -1;
			goto fail;
		}
	}

	// Call the registered function
	fun(arg, msg.arg_len, res, msg.res_len);

	if (res && msg.res_len > 0) {
		ret = __write_data(client_fd, res, msg.res_len);
		if (ret < 0) {
			clog_e("Failed to send response data: %s\n", strerror(errno));
			goto fail;
		}
	}

fail:
	if (arg) {
		free(arg);
	}
	if (res) {
		free(res);
	}

	return ret;
}

static void *isp_ipc_server_thread(void *arg)
{
	int ret = 0;
    int listen_fd, conn_fd, max_fd, i;
    int client_fds[__MAX_CLIENT] = {0};
    struct sockaddr_un addr;
    fd_set read_fds, all_fds;

	UNUSED(arg);

    listen_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (listen_fd < 0) {
		clog_e("socket error: %s\n", strerror(errno));
		return NULL;
    }

    unlink(__SOCKET_PATH);

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, __SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		clog_e("bind error: %s\n", strerror(errno));
		close(listen_fd);
		return NULL;
    }

    if (listen(listen_fd, __MAX_CLIENT) < 0) {
        clog_e("listen error: %s\n", strerror(errno));
		close(listen_fd);
		return NULL;
    }

    FD_ZERO(&all_fds);
    FD_SET(listen_fd, &all_fds);
    max_fd = listen_fd;

	clog_i("Server listening on %s\n", __SOCKET_PATH);

	while (g_server_running) {
		struct timeval timeout = {0, 100 * 1000};
		read_fds = all_fds;

		ret = select(max_fd + 1, &read_fds, NULL, NULL, &timeout);
		if (ret < 0) {
			if (errno == EINTR) {
				continue; // Interrupted by signal, retry
			}
			clog_e("select error: %s\n", strerror(errno));
			break;
		} else if (ret == 0) {
			continue; // Timeout, no activity
		}

		if (FD_ISSET(listen_fd, &read_fds)) {
			// New connection
			conn_fd = accept(listen_fd, NULL, NULL);
			if (conn_fd < 0) {
				clog_e("accept error: %s\n", strerror(errno));
				continue;
			}

			for (i = 0; i < __MAX_CLIENT; i++) {
				if (client_fds[i] == 0) {
					client_fds[i] = conn_fd;
					break;
				}
			}

			if (i == __MAX_CLIENT) {
				clog_e("Too many clients connected, rejecting new connection\n");
				close(conn_fd);
				continue;
			}

			FD_SET(conn_fd, &all_fds);
			if (conn_fd > max_fd) {
				max_fd = conn_fd;
			}

			clog_i("New client connected: fd=%d\n", conn_fd);
		} else {
			// Handle client requests
			for (i = 0; i < __MAX_CLIENT; i++) {
				if (client_fds[i] > 0 && FD_ISSET(client_fds[i], &read_fds)) {
					if (server_handle_client_cmd(client_fds[i]) < 0) {
						// Client disconnected or error occurred
						FD_CLR(client_fds[i], &all_fds);
						close(client_fds[i]);
						client_fds[i] = 0;
					}
				}
			}
		}
	}
	close(listen_fd);
	for (i = 0; i < __MAX_CLIENT; i++) {
		if (client_fds[i] > 0) {
			close(client_fds[i]);
			clog_i("Closed client fd=%d\n", client_fds[i]);
		}
	}
	unlink(__SOCKET_PATH);
	return NULL;
}

static CVI_U8 g_server_init_cnt;

CVI_S32 isp_ipc_server_init(void)
{
	CVI_S32 ret = CVI_SUCCESS;

	g_server_init_cnt++;
	if (g_server_init_cnt > 1) {
		return ret;
	}
	
	g_server_running = 1;
	ret = pthread_create(&g_server_thread, NULL, isp_ipc_server_thread, NULL);
	if (ret != 0) {	
		clog_e("Failed to create server thread: %s\n", strerror(ret));
		g_server_running = 0;
		return CVI_FAILURE;
	}

	return ret;
}

CVI_S32 isp_ipc_server_deinit(void)
{
	if (g_server_init_cnt > 1) {
		g_server_init_cnt--;
	} else if (g_server_init_cnt == 1) {
		g_server_init_cnt = 0;
		g_server_running = 0;
		pthread_join(g_server_thread, NULL);
	} else if (g_server_init_cnt == 0) {
		// not init
	}

	return CVI_SUCCESS;
}

/*----------------------------------client--------------------------------------*/
CVI_S32 isp_ipc_client_init(void)
{	
	return CVI_SUCCESS;
}

CVI_S32 isp_ipc_client_deinit(void)
{
	return CVI_SUCCESS;
}

CVI_S32 client_send_cmd_to_server(ISP_IPC_MSG_S *msg, CVI_U8 *arg, CVI_U8 *res)
{
	int ret;
	int sockfd;
	struct sockaddr_un addr;

	sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sockfd < 0) {
		clog_e("socket error: %s\n", strerror(errno));
		return CVI_FAILURE;
	}
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, __SOCKET_PATH, sizeof(addr.sun_path) - 1);
	ret = connect(sockfd, (struct sockaddr*)&addr, sizeof(addr));
	if (ret < 0) {
		clog_e("connect error: %s\n", strerror(errno));
		close(sockfd);
		return CVI_FAILURE;
	}
	msg->magic = __MIGIC_NUMBER;
	clog_i("Sending command: %d, arg_len: %u, res_len: %u\n", msg->cmd, msg->arg_len, msg->res_len);
	ret = __write_data(sockfd, msg, sizeof(*msg));
	if (ret < 0) {
		clog_e("Failed to send message header: %s\n", strerror(errno));
		close(sockfd);
		return CVI_FAILURE;
	}
	if (arg && msg->arg_len > 0) {
		ret = __write_data(sockfd, arg, msg->arg_len);
		if (ret < 0) {
			clog_e("Failed to send argument data: %s\n", strerror(errno));
			close(sockfd);
			return CVI_FAILURE;
		}
	}
	if (res && msg->res_len > 0) {
		ret = __read_data(sockfd, res, msg->res_len);
		if (ret <= 0) {
			clog_e("Failed to read response data: %s\n", strerror(errno));
			close(sockfd);
			return CVI_FAILURE;
		}
	}
	close(sockfd);
	return CVI_SUCCESS;
}

#endif // end of #ifdef ENABLE_ISP_IPC
