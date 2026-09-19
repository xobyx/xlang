#include "xsocket.h"
#include "functions.h"
#include "func_stack.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int s_sockets_initialized = 0;

static void ensure_sockets_initialized(void)
{
#ifdef _WIN32
	if (!s_sockets_initialized)
	{
		WSADATA wsa;
		if (WSAStartup(MAKEWORD(2, 2), &wsa) == 0)
			s_sockets_initialized = 1;
	}
#else
	s_sockets_initialized = 1;
#endif
}

static int get_int_arg(fcall* fc, int index, int default_val)
{
	if (fc == NULL || index >= fc->parm_count_c) return default_val;
	var* p = &fc->func_parmeters[index];
	if (p == NULL) return default_val;

	if (p->type_define == T_INT || (p->type_define != NULL && p->type_define->type_id == 3))
	{
		if (p->value_int != NULL) return *p->value_int;
	}
	else if (p->type_define == T_LONG || (p->type_define != NULL && p->type_define->type_id == 0))
	{
		if (p->value_long != NULL) return (int)*p->value_long;
	}
	else if (p->type_define == T_FLOAT || (p->type_define != NULL && p->type_define->type_id == 5))
	{
		if (p->value_float != NULL) return (int)*p->value_float;
	}
	else if (p->type_define == T_STRING || (p->type_define != NULL && p->type_define->type_id == 1))
	{
		if (p->value_str_ptr != NULL && *p->value_str_ptr != NULL)
			return atoi(*p->value_str_ptr);
	}
	return default_val;
}

static const char* get_str_arg(fcall* fc, int index, const char* default_val)
{
	if (fc == NULL || index >= fc->parm_count_c) return default_val;
	var* p = &fc->func_parmeters[index];
	if (p == NULL) return default_val;

	if (p->type_define == T_STRING || (p->type_define != NULL && p->type_define->type_id == 1))
	{
		if (p->value_str_ptr != NULL && *p->value_str_ptr != NULL)
			return *p->value_str_ptr;
	}
	else if (p->type_define == T_CHAR || (p->type_define != NULL && p->type_define->type_id == 2))
	{
		if (p->value_char_ptr != NULL)
			return p->value_char_ptr;
	}
	return default_val;
}

static int get_fd(fcall* fc, int arg_index)
{
	if (fc != NULL && fc->context != NULL && fc->context->value_type_instsance != NULL)
	{
		var* fd_var = get_var_by_name_on_stack("fd", &fc->context->value_type_instsance->propertys);
		if (fd_var != NULL && fd_var->value_int != NULL)
			return *fd_var->value_int;
	}
	return get_int_arg(fc, arg_index, -1);
}

static const char* get_str_param(fcall* fc, int standalone_idx, int method_idx, const char* def)
{
	int idx = (fc != NULL && fc->context != NULL) ? method_idx : standalone_idx;
	return get_str_arg(fc, idx, def);
}

static int get_int_param(fcall* fc, int standalone_idx, int method_idx, int def)
{
	int idx = (fc != NULL && fc->context != NULL) ? method_idx : standalone_idx;
	return get_int_arg(fc, idx, def);
}

void x_socket_create(fcall* fc)
{
	ensure_sockets_initialized();
	const char* type_str = get_str_arg(fc, 0, "tcp");
	int sock_type = SOCK_STREAM;
	int proto = IPPROTO_TCP;

	if (type_str != NULL && (strcmp(type_str, "udp") == 0 || strcmp(type_str, "UDP") == 0))
	{
		sock_type = SOCK_DGRAM;
		proto = IPPROTO_UDP;
	}

	SOCKET fd = socket(AF_INET, sock_type, proto);
	int res_fd = (int)fd;
	if (fd == XSOCKET_INVALID)
	{
		res_fd = -1;
	}

	fc->_return.value_int = new_int(1, res_fd);
	fc->_return.type_define = T_INT;
}

void x_socket_connect(fcall* fc)
{
	int fd = get_fd(fc, 0);
	const char* host = get_str_param(fc, 1, 0, "127.0.0.1");
	int port = get_int_param(fc, 2, 1, 0);

	if (fd < 0 || host == NULL || port <= 0)
	{
		fc->_return.value_int = new_int(1, -1);
		fc->_return.type_define = T_INT;
		return;
	}

	char port_str[16];
	snprintf(port_str, sizeof(port_str), "%d", port);

	struct addrinfo hints, *res = NULL, *p;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	int status = getaddrinfo(host, port_str, &hints, &res);
	if (status != 0 || res == NULL)
	{
		fc->_return.value_int = new_int(1, -1);
		fc->_return.type_define = T_INT;
		return;
	}

	int ret = -1;
	for (p = res; p != NULL; p = p->ai_next)
	{
		if (connect(fd, p->ai_addr, p->ai_addrlen) == 0)
		{
			ret = 0;
			break;
		}
	}
	freeaddrinfo(res);

	fc->_return.value_int = new_int(1, ret);
	fc->_return.type_define = T_INT;
}

void x_socket_bind(fcall* fc)
{
	int fd = get_fd(fc, 0);
	const char* host = get_str_param(fc, 1, 0, "0.0.0.0");
	int port = get_int_param(fc, 2, 1, 0);

	if (fd < 0 || port < 0)
	{
		fc->_return.value_int = new_int(1, -1);
		fc->_return.type_define = T_INT;
		return;
	}

	int opt = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

	int ret = -1;
	if (host == NULL || host[0] == '\0' || strcmp(host, "0.0.0.0") == 0)
	{
		struct sockaddr_in sin;
		memset(&sin, 0, sizeof(sin));
		sin.sin_family = AF_INET;
		sin.sin_addr.s_addr = htonl(INADDR_ANY);
		sin.sin_port = htons((unsigned short)port);
		ret = bind(fd, (struct sockaddr*)&sin, sizeof(sin));
	}
	else
	{
		char port_str[16];
		snprintf(port_str, sizeof(port_str), "%d", port);

		struct addrinfo hints, *res = NULL, *p;
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = AI_PASSIVE;

		int status = getaddrinfo(host, port_str, &hints, &res);
		if (status == 0 && res != NULL)
		{
			for (p = res; p != NULL; p = p->ai_next)
			{
				if (bind(fd, p->ai_addr, p->ai_addrlen) == 0)
				{
					ret = 0;
					break;
				}
			}
			freeaddrinfo(res);
		}
	}

	fc->_return.value_int = new_int(1, ret == 0 ? 0 : -1);
	fc->_return.type_define = T_INT;
}

void x_socket_listen(fcall* fc)
{
	int fd = get_fd(fc, 0);
	int backlog = get_int_param(fc, 1, 0, 10);
	if (backlog <= 0) backlog = 10;

	int ret = -1;
	if (fd >= 0)
	{
		ret = listen(fd, backlog);
	}

	fc->_return.value_int = new_int(1, ret == 0 ? 0 : -1);
	fc->_return.type_define = T_INT;
}

void x_socket_accept(fcall* fc)
{
	int fd = get_fd(fc, 0);
	int client_fd = -1;

	if (fd >= 0)
	{
		struct sockaddr_in caddr;
		socklen_t clen = sizeof(caddr);
		SOCKET s = accept(fd, (struct sockaddr*)&caddr, &clen);
		if (s != XSOCKET_INVALID)
		{
			client_fd = (int)s;
		}
	}

	fc->_return.value_int = new_int(1, client_fd);
	fc->_return.type_define = T_INT;
}

void x_socket_send(fcall* fc)
{
	int fd = get_fd(fc, 0);
	const char* data = get_str_param(fc, 1, 0, "");
	int bytes_sent = -1;

	if (fd >= 0 && data != NULL)
	{
		int flags = 0;
#ifdef MSG_NOSIGNAL
		flags = MSG_NOSIGNAL;
#endif
		bytes_sent = send(fd, data, (int)strlen(data), flags);
	}

	fc->_return.value_int = new_int(1, bytes_sent);
	fc->_return.type_define = T_INT;
}

void x_socket_recv(fcall* fc)
{
	int fd = get_fd(fc, 0);
	int max_bytes = get_int_param(fc, 1, 0, 4096);
	if (max_bytes <= 0) max_bytes = 4096;
	if (max_bytes > 1024 * 1024) max_bytes = 1024 * 1024;

	if (fd < 0)
	{
		char* empty = (char*)calloc(1, 1);
		fc->_return.value_str_ptr = get_pptr_string(empty);
		fc->_return.type_define = T_STRING;
		return;
	}

	char* buf = (char*)malloc(max_bytes + 1);
	if (!buf)
	{
		char* empty = (char*)calloc(1, 1);
		fc->_return.value_str_ptr = get_pptr_string(empty);
		fc->_return.type_define = T_STRING;
		return;
	}

	int n = recv(fd, buf, max_bytes, 0);
	if (n <= 0)
	{
		buf[0] = '\0';
	}
	else
	{
		buf[n] = '\0';
	}

	fc->_return.value_str_ptr = get_pptr_string(buf);
	fc->_return.type_define = T_STRING;
}

void x_socket_close(fcall* fc)
{
	int fd = get_fd(fc, 0);
	int ret = -1;

	if (fd >= 0)
	{
		ret = xsocket_close_fd(fd);
	}

	fc->_return.value_int = new_int(1, ret == 0 ? 0 : -1);
	fc->_return.type_define = T_INT;
}

void x_socket_set_timeout(fcall* fc)
{
	int fd = get_fd(fc, 0);
	int sec = get_int_param(fc, 1, 0, 5);
	int ret = -1;

	if (fd >= 0)
	{
#ifdef _WIN32
		DWORD timeout = (DWORD)(sec * 1000);
		setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
		ret = setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
#else
		struct timeval tv;
		tv.tv_sec = sec;
		tv.tv_usec = 0;
		setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
		ret = setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof(tv));
#endif
	}

	fc->_return.value_int = new_int(1, ret == 0 ? 0 : -1);
	fc->_return.type_define = T_INT;
}

void x_socket_set_reuseaddr(fcall* fc)
{
	int fd = get_fd(fc, 0);
	int opt = get_int_param(fc, 1, 0, 1);
	int ret = -1;

	if (fd >= 0)
	{
		ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
	}

	fc->_return.value_int = new_int(1, ret == 0 ? 0 : -1);
	fc->_return.type_define = T_INT;
}

void x_socket_sendto(fcall* fc)
{
	int fd = get_fd(fc, 0);
	const char* data = get_str_param(fc, 1, 0, "");
	const char* host = get_str_param(fc, 2, 1, "127.0.0.1");
	int port = get_int_param(fc, 3, 2, 0);
	int bytes_sent = -1;

	if (fd >= 0 && data != NULL && host != NULL && port > 0)
	{
		struct sockaddr_in dest;
		memset(&dest, 0, sizeof(dest));
		dest.sin_family = AF_INET;
		dest.sin_port = htons((unsigned short)port);
		inet_pton(AF_INET, host, &dest.sin_addr);

		bytes_sent = sendto(fd, data, (int)strlen(data), 0, (struct sockaddr*)&dest, sizeof(dest));
	}

	fc->_return.value_int = new_int(1, bytes_sent);
	fc->_return.type_define = T_INT;
}

void x_socket_recvfrom(fcall* fc)
{
	int fd = get_fd(fc, 0);
	int max_bytes = get_int_param(fc, 1, 0, 4096);
	if (max_bytes <= 0) max_bytes = 4096;
	if (max_bytes > 1024 * 1024) max_bytes = 1024 * 1024;

	if (fd < 0)
	{
		char* empty = (char*)calloc(1, 1);
		fc->_return.value_str_ptr = get_pptr_string(empty);
		fc->_return.type_define = T_STRING;
		return;
	}

	char* buf = (char*)malloc(max_bytes + 1);
	if (!buf)
	{
		char* empty = (char*)calloc(1, 1);
		fc->_return.value_str_ptr = get_pptr_string(empty);
		fc->_return.type_define = T_STRING;
		return;
	}

	struct sockaddr_in src_addr;
	socklen_t addr_len = sizeof(src_addr);
	int n = recvfrom(fd, buf, max_bytes, 0, (struct sockaddr*)&src_addr, &addr_len);
	if (n <= 0)
	{
		buf[0] = '\0';
	}
	else
	{
		buf[n] = '\0';
	}

	fc->_return.value_str_ptr = get_pptr_string(buf);
	fc->_return.type_define = T_STRING;
}

static int parse_http_url(const char* url, char* host, int host_sz, int* port, char* path, int path_sz)
{
	if (url == NULL || *url == '\0') return -1;
	const char* p = url;
	if (strncmp(p, "http://", 7) == 0) p += 7;
	else if (strncmp(p, "https://", 8) == 0) p += 8;

	*port = 80;
	path[0] = '/';
	path[1] = '\0';

	const char* slash = strchr(p, '/');
	int hostport_len = slash ? (int)(slash - p) : (int)strlen(p);
	char hostport[256];
	if (hostport_len >= (int)sizeof(hostport)) hostport_len = (int)sizeof(hostport) - 1;
	memcpy(hostport, p, hostport_len);
	hostport[hostport_len] = '\0';

	char* colon = strchr(hostport, ':');
	if (colon)
	{
		*colon = '\0';
		*port = atoi(colon + 1);
	}
	snprintf(host, host_sz, "%s", hostport);

	if (slash)
	{
		snprintf(path, path_sz, "%s", slash);
	}
	return 0;
}

void x_http_get(fcall* fc)
{
	const char* url = get_str_arg(fc, 0, "");
	char host[256] = {0};
	char path[512] = {0};
	int port = 80;

	if (parse_http_url(url, host, sizeof(host), &port, path, sizeof(path)) != 0)
	{
		char* err_res = (char*)calloc(1, 1);
		fc->_return.value_str_ptr = get_pptr_string(err_res);
		fc->_return.type_define = T_STRING;
		return;
	}

	ensure_sockets_initialized();
	SOCKET fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == XSOCKET_INVALID)
	{
		char* err_res = (char*)calloc(1, 1);
		fc->_return.value_str_ptr = get_pptr_string(err_res);
		fc->_return.type_define = T_STRING;
		return;
	}

#ifdef _WIN32
	DWORD timeout = 5000;
	setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
	setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
#else
	struct timeval tv;
	tv.tv_sec = 5;
	tv.tv_usec = 0;
	setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
	setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof(tv));
#endif

	char port_str[16];
	snprintf(port_str, sizeof(port_str), "%d", port);
	struct addrinfo hints, *res = NULL, *p;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	int status = getaddrinfo(host, port_str, &hints, &res);
	if (status != 0 || res == NULL)
	{
		xsocket_close_fd(fd);
		char* err_res = (char*)calloc(1, 1);
		fc->_return.value_str_ptr = get_pptr_string(err_res);
		fc->_return.type_define = T_STRING;
		return;
	}

	int connected = 0;
	for (p = res; p != NULL; p = p->ai_next)
	{
		if (connect(fd, p->ai_addr, p->ai_addrlen) == 0)
		{
			connected = 1;
			break;
		}
	}
	freeaddrinfo(res);

	if (!connected)
	{
		xsocket_close_fd(fd);
		char* err_res = (char*)calloc(1, 1);
		fc->_return.value_str_ptr = get_pptr_string(err_res);
		fc->_return.type_define = T_STRING;
		return;
	}

	char req[1024];
	snprintf(req, sizeof(req),
	         "GET %s HTTP/1.1\r\n"
	         "Host: %s\r\n"
	         "User-Agent: xlang\r\n"
	         "Accept: */*\r\n"
	         "Connection: close\r\n\r\n",
	         path, host);

	int send_flags = 0;
#ifdef MSG_NOSIGNAL
	send_flags = MSG_NOSIGNAL;
#endif
	send(fd, req, (int)strlen(req), send_flags);

	int cap = 4096;
	int len = 0;
	char* resp = (char*)malloc(cap);
	if (!resp)
	{
		xsocket_close_fd(fd);
		char* err_res = (char*)calloc(1, 1);
		fc->_return.value_str_ptr = get_pptr_string(err_res);
		fc->_return.type_define = T_STRING;
		return;
	}

	while (1)
	{
		int n = recv(fd, resp + len, cap - len - 1, 0);
		if (n <= 0) break;
		len += n;
		if (len + 1024 >= cap)
		{
			cap *= 2;
			char* new_resp = (char*)realloc(resp, cap);
			if (!new_resp) break;
			resp = new_resp;
		}
	}
	resp[len] = '\0';
	xsocket_close_fd(fd);

	fc->_return.value_str_ptr = get_pptr_string(resp);
	fc->_return.type_define = T_STRING;
}
