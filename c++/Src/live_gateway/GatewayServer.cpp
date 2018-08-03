#include "event2/listener.h"
#include "EventBase.h"
#include "GatewayServer.h"
#include "GatewayClient.h"
#include "Writelog.h"
#include "ConsoleUI.h"
#include "SmartPtr.h"
#include "xaid.h"
#include "net.h"
#include "Byte.h"

CGatewayServer::CGatewayServer()
	: m_listenPort(0){
}
CGatewayServer::~CGatewayServer() {
}

CGatewayServer& CGatewayServer::singleton() {
	static CGatewayServer instance;
	return instance;
}

bool CGatewayServer::startup(const int port, const xtstring& ip /* = _T("0.0.0.0") */) {
	if (!CEventBase::singleton().addListen(ip.toString().c_str(), port, &CGatewayServer::listen_callback, this)) {
		return wlet(false, _T("Initialize socket failed."));
	}
	m_listenPort = port;
	return true;
}
void CGatewayServer::shutdown() {
	struct sockaddr_in sock_addr;
	evutil_socket_t sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	evutil_make_socket_nonblocking(sock);
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_port = htons(m_listenPort);
	inet_aton("127.0.0.1", &sock_addr.sin_addr);
	memset(sock_addr.sin_zero, 0x00, 8);
	connect(sock, (struct sockaddr*)&sock_addr, sizeof(sock_addr));
}

void CGatewayServer::listen_callback(struct evconnlistener* conn, evutil_socket_t fd, struct sockaddr* addr, int addrlen, void* context) {
	CGatewayServer* thiz(reinterpret_cast<CGatewayServer*>(context));
	thiz->listen_callback(conn, fd, addr, addrlen);
}
void CGatewayServer::listen_callback(struct evconnlistener* conn, evutil_socket_t fd, struct sockaddr* addr, int addrlen) {
	xtstring ip(inet_ntoa(((sockaddr_in*)addr)->sin_addr));
	int port(ntohs(((sockaddr_in*)addr)->sin_port));
	CGatewayClient* client(CGatewayClient::newInstance(fd, addr));
	setkeepalive(fd, 5, 5);
	evutil_make_listen_socket_reuseable(fd);
	auto finalize(std::destruct_invoke([client, fd]() {
		client->release();
		evutil_closesocket(fd);
	}));
	if (!client->run(evconnlistener_get_base(conn))) {
		return wle(_T("Set up client [%s:%d] failed."), ip.c_str(), port);
	}
	finalize.cancel();
}