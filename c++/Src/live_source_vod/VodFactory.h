#pragma once
#include <vector>
#include <mutex>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/http.h>
#include <event2/http_struct.h>
#include "SourceInterface.h"
#include "xstring.h"
#include "SmartPtr.h"
#include "SmartHdr.h"

class CVodFactory : public ISourceFactory {
public:
	CVodFactory();
	~CVodFactory();

public:
	static CVodFactory& Singleton();

public:
	virtual LPCTSTR FactoryName() const;
	virtual bool Initialize(ISourceFactoryCallback* callback);
	virtual SupportState DidSupport(LPCTSTR device, LPCTSTR moniker, LPCTSTR param);
	virtual ISourceProxy* CreateLiveProxy(ISourceProxyCallback* callback, LPCTSTR device, LPCTSTR moniker, LPCTSTR param);
	virtual ISourceProxy* CreatePastProxy(ISourceProxyCallback* callback, LPCTSTR device, LPCTSTR moniker, LPCTSTR param, uint64_t beginTime, uint64_t endTime = 0);
	virtual bool GetSourceList(LPTSTR* xml, void(**free)(LPTSTR), bool onlineOnly, LPCTSTR device = NULL);
	virtual bool GetStatusInfo(LPSTR* json, void(**free)(LPSTR));
	virtual void Uninitialize();
	virtual void Destroy();

private:
	static void listen_callback(struct evconnlistener* conn, evutil_socket_t fd, struct sockaddr* addr, int addrlen, void* context);
	void listen_callback(struct evconnlistener* conn, evutil_socket_t fd, struct sockaddr* addr, int addrlen);

private:
	const LPCTSTR m_lpszFactoryName;
	ISourceFactoryCallback* m_lpCallback;
	std::vector<CSmartHdr<evconnlistener*, void>> m_listeners;
};

