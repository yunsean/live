#pragma once
#include "xstring.h"

class CGatewayServer {
public:
	CGatewayServer();
	~CGatewayServer();

public:
	static CGatewayServer& singleton();

public:
	bool startup(const int port, const xtstring& ip = _T("0.0.0.0"));
	void shutdown();

private:
	static void listen_callback(struct evconnlistener* conn, evutil_socket_t sock, struct sockaddr* addr, int addrlen, void* context);
	void listen_callback(struct evconnlistener* conn, evutil_socket_t sock, struct sockaddr* addr, int addrlen);

private:
	int m_listenPort;
};

