#pragma once
#include <mutex>
#include <queue>
#include "HttpClient.h"
#include "SinkInterface.h"

class CGatewayClient : public CHttpClient, public ISinkHttpRequest {
protected:
	CGatewayClient();
	virtual ~CGatewayClient();

public:
	static CGatewayClient* newInstance(evutil_socket_t fd, struct sockaddr* addr);
public:
	virtual void release();

protected:
	//ISinkHttpRequest
	virtual LPCTSTR GetOperation() const { return operation().c_str(); }
	virtual LPCTSTR GetVersion() const { return version().c_str(); }
	virtual LPCTSTR GetUrl() const { return url().c_str(); }
	virtual LPCTSTR GetPath() const { return path().c_str(); }
	virtual LPCTSTR GetAction() const { return action().c_str(); }
	virtual LPCTSTR GetIp() const { return ip().c_str(); }
	virtual int GetPort() const { return port(); }
	virtual LPCTSTR GetParam(LPCTSTR key) const { return param(key); }
	virtual LPCTSTR GetPayload(LPCTSTR key) const { return payload(key); }
	virtual evutil_socket_t GetSocket(bool detach /* = true */) { return fd(detach); }
	virtual event_base* GetBase() { return base(); }

protected:
	static std::mutex m_mutex;
	static std::queue<CGatewayClient*> m_clients;

protected:
	virtual void handle();

protected:
	void onSystemInfo();
	void onStreamList();
	void onPtzControl();
};

