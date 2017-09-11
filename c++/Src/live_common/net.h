#pragma once
#include <map>
#include <stdio.h>
#include <stdlib.h>
#if defined _WIN32
#	pragma comment(lib, "ws2_32.lib")
#	include <winsock2.h>
#	include <ws2tcpip.h>
#	include <mstcpip.h>
#else
#	include <sys/types.h>
#	include <sys/socket.h>
#	include <netinet/tcp.h>
#	include <netinet/in.h>
#	include <arpa/inet.h>
#	include <netdb.h>
#	include <unistd.h>
#	include <errno.h>
#	include <fcntl.h>
#endif
#include "xstring.h"

#if defined _WIN32
#define net_error() WSAGetLastError()
#else
typedef int SOCKET;
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close(s)
#define net_error() errno
#endif

#if defined _WIN32
#define init_sockets()\
do {\
	WSADATA wsaData;\
	if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0){\
		printf("Error at WSAStartup()\n");\
		exit(1);\
	}\
} while(0)
#else
#define init_sockets() (void)0
#endif

#if defined _WIN32
#define uninit_sockets()\
do {\
	WSACleanup();\
} while(0)
#else
#define uninit_sockets() (void)0
#endif

int setnonblock(SOCKET fd);	//evutil_make_socket_nonblocking
int setkeepalive(SOCKET fd, int time, int interval);

bool resolveUrl(const xtstring& url, xtstring& path, std::map<xtstring, xtstring>& queries);
xtstring composeQuery(const std::map<xtstring, xtstring>& queries);
xtstring composeUrl(const xtstring& path, const std::map<xtstring, xtstring>& queries = std::map<xtstring, xtstring>());
int resolveHeader(const char* const lpszHeader, std::map<xtstring, xtstring>& payloads);
std::string encodeUrl(const xtstring& url);
xtstring decodeUrl(const std::string& url);