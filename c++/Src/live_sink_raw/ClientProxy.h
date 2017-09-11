#pragma once
#include <mutex>
#include <queue>
#include "HttpClient.h"

class CClientProxy : public CHttpClient {
public:
	CClientProxy();
	~CClientProxy();

public:
	static CClientProxy* newInstance(evutil_socket_t fd, struct sockaddr* addr);

public:
	virtual void release();

protected:
	static std::mutex m_mutex;
	static std::queue<CClientProxy*> m_clients;

protected:
	virtual void handle();

protected:
	void onSystemInfo();
	void onStreamList();
	void onPlayStream();
};

