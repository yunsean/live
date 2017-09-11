#include <event2/event.h>
#include <event2/thread.h>
#include <event2/event_struct.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/http.h>
#include <event2/listener.h>
#include "net.h"
#include "VodFactory.h"
#include "Profile.h"
#include "Writelog.h"
#include "os.h"
#include "xaid.h"

CVodFactory::CVodFactory()
	: m_lpszFactoryName(_T("Vod Source"))
	, m_lpCallback(nullptr)
	, m_listeners() {
}

CVodFactory::~CVodFactory() {
}

CVodFactory& CVodFactory::Singleton() {
	static CVodFactory instance;
	return instance;
}

LPCTSTR CVodFactory::FactoryName() const {
	return m_lpszFactoryName;
}
bool CVodFactory::Initialize(ISourceFactoryCallback* callback) {
#ifdef _WIN32
	if (evthread_use_windows_threads() != 0) {
#else 
	if (evthread_use_pthreads() != 0) {
#endif
		return wlet(false, _T("init multiple thread failed."));
	}
	m_lpCallback = callback;
	CProfile profile(CProfile::Default());
	int port(profile(_T("Vod"), _T("Listen port"), _T("99")));
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	int status = inet_aton("0.0.0.0", &server_addr.sin_addr);
	if (status == 0) {
		return wlet(false, _T("Convert ip address failed."));
	}
	int family = server_addr.sin_family;
	evutil_socket_t fd = socket(family, SOCK_STREAM, 0);
	if (fd == -1) {
		return wlet(false, _T("Create socket failed."));
	}
	if (evutil_make_socket_nonblocking(fd) < 0) {
		evutil_closesocket(fd);
		return wlet(false, _T("Setup socket failed."));
	}
	int on = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (const char*)&on, sizeof(on)) < 0) {
		evutil_closesocket(fd);
		return wlet(false, _T("Setup socket failed."));
	}
	if (evutil_make_listen_socket_reuseable(fd) < 0) {
		evutil_closesocket(fd);
		return wlet(false, _T("Setup socket failed."));
	}
	if (bind(fd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(sockaddr)) < 0) {
		evutil_closesocket(fd);
		return wlet(false, _T("Setup socket failed."));
	}

	int count(0);
	event_base** bases(callback->GetAllBase(count));
	if (bases == NULL || count < 1) {
		evutil_closesocket(fd);
		return wlet(false, _T("No event base."));
	}
	for (int i = 0; i < count; i++) {
		CSmartHdr<evconnlistener*, void> listen(evconnlistener_new(bases[i], listen_callback, this, LEV_OPT_THREADSAFE | LEV_OPT_REUSEABLE, 100, fd), evconnlistener_free);
		if (!listen.isValid()) {
			evutil_closesocket(fd);
			return wlet(false, _T("Create listen socket failed."));
		}
		m_listeners.push_back(listen);
	}
	return true;
}
void CVodFactory::listen_callback(struct evconnlistener* conn, evutil_socket_t fd, struct sockaddr* addr, int addrlen, void* context) {
	CVodFactory* thiz(reinterpret_cast<CVodFactory*>(context));
	thiz->listen_callback(conn, fd, addr, addrlen);
}
void CVodFactory::listen_callback(struct evconnlistener* conn, evutil_socket_t fd, struct sockaddr* addr, int addrlen) {
	
}


ISourceFactory::SupportState CVodFactory::DidSupport(LPCTSTR device, LPCTSTR moniker, LPCTSTR param) {
	if (device != NULL && _tcslen(device) > 0 && _tcscmp(device, _T("vod")) != 0) {
		return unsupport;
	}
	return unsupport;
}
ISourceProxy* CVodFactory::CreateLiveProxy(ISourceProxyCallback* callback, LPCTSTR device, LPCTSTR moniker, LPCTSTR param) {
	if (device != NULL && _tcslen(device) > 0 && _tcscmp(device, _T("vod")) != 0) {
		return nullptr;
	}
	return nullptr;
}
ISourceProxy* CVodFactory::CreatePastProxy(ISourceProxyCallback* callback, LPCTSTR device, LPCTSTR moniker, LPCTSTR param, uint64_t beginTime, uint64_t endTime /*= 0*/) {
	return nullptr;
}
bool CVodFactory::GetSourceList(LPTSTR* _xml, void(**free)(LPTSTR), bool onlineOnly, LPCTSTR device /*= NULL*/) {
	if (!_xml)return false;
	if (device != NULL && _tcslen(device) > 0 && _tcsicmp(device, _T("vod")) != 0)return false;
	return true;
}
bool CVodFactory::GetStatusInfo(LPSTR* json, void(**free)(LPSTR)) {
	return false;
}
void CVodFactory::Uninitialize() {
	m_listeners.clear();
}
void CVodFactory::Destroy() {
}