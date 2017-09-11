#pragma once
#include <mutex>
#include <queue>
#include "HttpClient.h"

class CGatewayClient : public CHttpClient {
protected:
	CGatewayClient();
	virtual ~CGatewayClient();

public:
	static CGatewayClient* newInstance(evutil_socket_t fd, struct sockaddr* addr);
public:
	virtual void release();

protected:
	static std::mutex m_mutex;
	static std::queue<CGatewayClient*> m_clients;

protected:
	virtual void handle();

protected:
	void onSystemInfo();
	void onStreamList();
};

