#pragma once
#include <vector>
#include <mutex>
#include <atomic>
#include <list>
#include <functional>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/http.h>
#include <event2/http_struct.h>
#include "SourceInterface.h"
#include "xstring.h"
#include "SmartPtr.h"
#include "xthread.h"

class COnvifFactory : public ISourceFactory {
public:
	COnvifFactory();
	~COnvifFactory();

public:
	static COnvifFactory& Singleton();

public:
	event_base* GetPreferBase() { return m_lpCallback->GetPreferBase(); }

protected:
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
	bool discovery(ISourceFactoryCallback* callback);
	void broadcast();

protected:
	static void read_callback(evutil_socket_t fd, short event, void* arg);
	void read_callback(evutil_socket_t fd, short event);
	static void timer_callback(evutil_socket_t fd, short event, void* arg);
	void timer_callback(evutil_socket_t fd, short event);
	
private:
	const LPCTSTR m_lpszFactoryName;
	ISourceFactoryCallback* m_lpCallback;
	std::string m_messageId;
	char m_udpCache[10240];
	evutil_socket_t m_fd;
	event* m_timer;
	std::list<xtstring> m_devices;
};

