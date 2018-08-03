#include <event2/event.h>
#include <event2/thread.h>
#include <event2/event_struct.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/http.h>
#include <event2/listener.h>
#include "net.h"
#include "RtmpFactory.h"
#include "RtmpPublisher.h"
#include "RtmpConnection.h"
#include "ConsoleUI.h"
#include "Markup.h"
#include "Profile.h"
#include "Writelog.h"
#include "os.h"
#include "xaid.h"

CRtmpFactory::CRtmpFactory()
	: m_lpszFactoryName(_T("RTMP Source"))
	, m_lpCallback(nullptr)
	, m_listeners() 
	
	, m_lkPublishers()
	, m_vPublishers() {
}
CRtmpFactory::~CRtmpFactory() {
}

CRtmpFactory& CRtmpFactory::Singleton() {
	static CRtmpFactory instance;
	return instance;
}

void CRtmpFactory::RegistPublisher(CRtmpPublisher* publisher) {
	std::lock_guard<std::recursive_mutex> lock(m_lkPublishers);
	m_vPublishers.push_back(publisher);
}
void CRtmpFactory::DeletePublisher(CRtmpPublisher* publisher) {
	std::lock_guard<std::recursive_mutex> lock(m_lkPublishers);
	auto found(std::find_if(m_vPublishers.begin(), m_vPublishers.end(),
		[publisher](CRtmpPublisher* sp) -> bool {
		return sp == publisher;
	}));
	if (found != m_vPublishers.end()) {
		m_vPublishers.erase(found);
	}
}
void CRtmpFactory::DeleteConnection(CRtmpConnection* connection) {
	std::lock_guard<std::recursive_mutex> lock(m_lkPublishers);
	auto found(std::find_if(m_vConnections.begin(), m_vConnections.end(),
		[connection](CSmartPtr<CRtmpConnection> sp) -> bool {
		return sp.GetPtr() == connection;
	}));
	if (found != m_vConnections.end()) {
		m_vConnections.erase(found);
	} else {
		wle(_T("Can not find the rtmp connection: [0x%08x]"), connection);
	}
}

LPCTSTR CRtmpFactory::FactoryName() const {
	return m_lpszFactoryName;
}
bool CRtmpFactory::Initialize(ISourceFactoryCallback* callback) {
#ifdef _WIN32
	if (evthread_use_windows_threads() != 0) {
#else 
	if (evthread_use_pthreads() != 0) {
#endif
		return wlet(false, _T("init multiple thread failed."));
	}
	m_lpCallback = callback;
	CProfile profile(CProfile::Default());
	int port(profile(_T("Rtmp"), _T("Listen port"), _T("1935")));
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
void CRtmpFactory::listen_callback(struct evconnlistener* conn, evutil_socket_t fd, struct sockaddr* addr, int addrlen, void* context) {
	CRtmpFactory* thiz(reinterpret_cast<CRtmpFactory*>(context));
	thiz->listen_callback(conn, fd, addr, addrlen);
}
void CRtmpFactory::listen_callback(struct evconnlistener* conn, evutil_socket_t fd, struct sockaddr* addr, int addrlen) {
	CSmartPtr<CRtmpConnection> spConnection(new CRtmpConnection, &CRtmpConnection::Release);
	xtstring ip(inet_ntoa(((sockaddr_in*)addr)->sin_addr));
	int port(ntohs(((sockaddr_in*)addr)->sin_port));
	setkeepalive(fd, 5, 5);
	evutil_make_listen_socket_reuseable(fd);
	auto finalize(std::destruct_invoke([fd]() {
		evutil_closesocket(fd);
	}));
	if (!spConnection->Startup(evconnlistener_get_base(conn), fd, addr)) {
		return wle(_T("Set up client [%s:%d] failed."), ip.c_str(), port);
	}
	finalize.cancel();
	std::lock_guard<std::recursive_mutex> lock(m_lkPublishers);
	m_vConnections.push_back(spConnection);
}

ISourceFactory::SupportState CRtmpFactory::DidSupport(LPCTSTR device, LPCTSTR moniker, LPCTSTR param) {
	if (device != NULL && _tcslen(device) > 0 && _tcscmp(device, _T("ugc")) != 0) {
		return unsupport;
	}
	std::lock_guard<std::recursive_mutex> lock(m_lkPublishers);
	for (auto publisher : m_vPublishers) {
		if (publisher->GetName() == moniker) {
			return supported;
		}
	}
	return unsupport;
}
ISourceProxy* CRtmpFactory::CreateLiveProxy(ISourceProxyCallback* callback, LPCTSTR device, LPCTSTR moniker, LPCTSTR param) {
	if (device != NULL && _tcslen(device) > 0 && _tcscmp(device, _T("ugc")) != 0) {
		return nullptr;
	}
	std::lock_guard<std::recursive_mutex> lock(m_lkPublishers);
	for (auto publisher : m_vPublishers) {
		if (publisher->GetName() == moniker) {
			if (publisher->SetCallback(callback)) {
				return publisher;
			} else {
				break;
			}
		}
	}
	return nullptr;
}
ISourceProxy* CRtmpFactory::CreatePastProxy(ISourceProxyCallback* callback, LPCTSTR device, LPCTSTR moniker, LPCTSTR param, uint64_t beginTime, uint64_t endTime /*= 0*/) {
	return nullptr;
}
bool CRtmpFactory::GetSourceList(LPTSTR* _xml, void(**free)(LPTSTR), bool onlineOnly, LPCTSTR device /*= NULL*/) {
	if (!_xml)return false;
	if (device != NULL && _tcslen(device) > 0 && _tcsicmp(device, _T("ugc")) != 0)return false;
	CMarkup xml;
	xml.AddElem(_T("ugc"));
	xml.AddAttrib(_T("name"), _T("直播回传"));
	xml.IntoElem();
	std::lock_guard<std::recursive_mutex> lock(m_lkPublishers);
	for (auto publisher : m_vPublishers) {
		xml.AddElem(_T("signal"));
		xml.AddAttrib(_T("name"), publisher->GetName());
		xml.AddAttrib(_T("moniker"), publisher->GetName());
		xml.AddAttrib(_T("device"), _T("ugc"));
	}
	xtstring text(xml.GetDoc());
	const TCHAR* lpsz(text.c_str());
	const size_t len(text.length());
	TCHAR* result = new TCHAR[len + 1];
	_tcscpy(result, lpsz);
	*_xml = result;
	*free = [](LPTSTR ptr) {
		delete[] reinterpret_cast<TCHAR*>(ptr);
	};
	return true;
}
bool CRtmpFactory::GetStatusInfo(LPSTR* json, void(**free)(LPSTR)) {
	return false;
}
void CRtmpFactory::Uninitialize() {
	m_vPublishers.clear();
	m_listeners.clear();
}
void CRtmpFactory::Destroy() {
}