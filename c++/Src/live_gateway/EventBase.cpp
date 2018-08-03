#include <event2/event.h>
#include <event2/thread.h>
#include <event2/event_struct.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/http.h>
#include <event2/listener.h>
#include <signal.h>
#include "EventBase.h"
#include "SystemInfo.h"
#include "xstring.h"
#include "xaid.h"
#include "xsystem.h"
#include "ConsoleUI.h"
#include "Writelog.h"

CEventBase::CEventBase() 
	: m_eventBases()
	, m_baseThreads()
	, m_threadCount(0)
	, m_listeners()
	, m_willQuit(false) {
}

CEventBase::~CEventBase() {
	shutdown();
}
CEventBase& CEventBase::singleton() {
	static CEventBase instance;
	return instance;
}
void CEventBase::libevent_log_cb(int severity, const char* msg) {
	xtstring message(msg);
	switch (severity) {
	case EVENT_LOG_MSG:
		wli(_T("%s"), message.c_str());
		break;
	case EVENT_LOG_WARN:
		wlw(_T("%s"), message.c_str());
		break;
	case EVENT_LOG_ERR:
		wle(_T("%s"), message.c_str());
		break;
	}
#if defined(UNICODE) || defined(_UNICODE)
	cpm(_T("event:{%d}: %S"), severity, msg);
#else 
	cpm(_T("event:{%d}: %s"), severity, msg);
#endif
}
void CEventBase::libevent_fatal_cb(int err) {
	cpe(_T("核心引擎错误！"));
	wle(_T("event failed: %d"), err);
}

bool CEventBase::initialize(int threadCount /* = -1 */) {
// #ifdef _WIN32
// 	event_enable_debug_mode();
// 	evthread_enable_lock_debuging();
// #endif
	event_set_log_callback(libevent_log_cb);
	event_set_fatal_callback(libevent_fatal_cb);
// #ifdef _WIN32
// 	if (evthread_use_windows_threads() != 0) {
// #else 
// 	if (evthread_use_pthreads() != 0) {
// #endif
// 		return wlet(false, _T("init multiple thread failed."));
// 	}

	if (m_eventBases.size() > 0) {
		return wlet(false, _T("The object has initialized."));
	}
	if (threadCount <= 0) {
		threadCount = std::cpuCount() * 2;
	}
	if (threadCount <= 0) {
		threadCount = 1;
	}
	for (int i = 0; i < threadCount; i++) {
		m_eventBases.push_back(event_base_new());
	}
	return true;
}
event_base* CEventBase::preferBase() {
	auto found(m_baseThreads.find(std::tid()));
	if (found == m_baseThreads.end()) return nullptr;
	return found->second;
}
event_base* CEventBase::nextBase() {
	return m_eventBases.at(m_nextIndex++ % m_eventBases.size());
}
event_base** CEventBase::allBase(int& count) {
	if (m_eventBases.size() < 1) {
		count = 0;
		return NULL;
	}
	else {
		count = static_cast<int>(m_eventBases.size());
		return &m_eventBases[0];
	}
}

bool CEventBase::addListen(const char* ip, unsigned int port, void(*callback)(struct evconnlistener *, evutil_socket_t, struct sockaddr *, int socklen, void *), void* context /*= NULL*/) {
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	int status = inet_aton(ip, &server_addr.sin_addr);
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
	for (auto base : m_eventBases) {
		CSmartHdr<evconnlistener*, void> listen(evconnlistener_new(base, callback, context, LEV_OPT_THREADSAFE | LEV_OPT_REUSEABLE, 100, fd), evconnlistener_free);
		if (!listen.isValid()) {
			return wlet(false, _T("Create listen socket failed."));
		}	
		m_listeners.push_back(listen);
	}
	return true;
}

bool CEventBase::addSocket(evutil_socket_t fd, short events, void(*callback)(evutil_socket_t, short, void *), void* context) {
	struct event* ev(event_new(nextBase(), fd, events, callback, context));
	auto finalize(std::destruct_invoke([&ev]() { if (ev) event_free(ev); }));
	if (ev == NULL) {
		return wlet(false, _T("Assign event failed."));
	}
	int result(event_add(ev, NULL));
	if (result != 0) {
		return wlet(false, _T("Add event to loop failed: %d."), result);
	}
	finalize.cancel();
	return true;
}
bool CEventBase::addTimer(unsigned int timeout, void(*callback)(evutil_socket_t, short, void *), void* context /* = NULL */) {
	struct event* ev(evtimer_new(nextBase(), callback, context));
	struct timeval tv = {(int)(timeout / 1000), (timeout % 1000) * 1000};
	auto finalize(std::destruct_invoke([&ev]() { if (ev) event_free(ev); }));
	if (ev == NULL) {
		return wlet(false, _T("Assign event failed."));
	}
	int result(evtimer_add(ev, &tv));
	if (result != 0) {
		return wlet(false, _T("Add event to loop failed: %d."), result);
	}
	finalize.cancel();
	return true;
}
bool CEventBase::addHttp(char* ip, unsigned int port, void (*callback)(struct evhttp_request*, void*), void* context /* = NULL */) {
	struct evhttp* http = evhttp_new(nextBase());
	auto finalize(std::destruct_invoke([&http]() { if (http) evhttp_free(http); }));
	if (!http) {
		return wlet(false, _T("Assign http event failed."));
	}
	static char self[] = "0.0.0.0";
	if (ip == NULL)ip = self;
	int result(evhttp_bind_socket(http, ip, port));
	if (result != 0) {
		return wlet(false, _T("Listen http at port[%d] failed."), port);
	}
	evhttp_set_gencb(http, callback, NULL);
	finalize.cancel();
	return true;
}

bufferevent* CEventBase::createBuffer(evutil_socket_t fd, void(*on_read)(struct bufferevent*, void*), void(*on_write)(struct bufferevent*, void*), void(*on_error)(struct bufferevent*, short, void*), void* context /* = NULL */) {
	bufferevent* buf = bufferevent_socket_new(nextBase(), fd, 0);
	if (buf == NULL) {
		return wlet(nullptr, _T("Create buffer event failed."));
	}
	bufferevent_setcb(buf, on_read, on_write, on_error, context);
	bufferevent_enable(buf, EV_READ|EV_WRITE);
	return buf;
}
evhttp_request* CEventBase::httpGet(const char* url, void(*callback)(struct evhttp_request*, void*), void* context /* = NULL */) {
	return httpRequest(url, EVHTTP_REQ_GET, nullptr, 0, callback, context);
}
evhttp_request* CEventBase::httpRequest(const char* url, evhttp_cmd_type method, const char* contentType, size_t contentLen, void(*callback)(struct evhttp_request*, void*), void* context /* = NULL */) {
	evhttp_uri* uri = evhttp_uri_parse_with_flags(url, EVHTTP_URI_NONCONFORMANT);
	if (uri == nullptr) {
		return wlet(nullptr, _T("Parse url [%s] failed."), xtstring(url).c_str());
	}
	evhttp_request* request = evhttp_request_new(callback, context);
	auto finalize(std::destruct_invoke([request](){
		evhttp_request_free(request);
	}));
	if (uri == nullptr) {
		return wlet(nullptr, _T("Create http request failed."));
	}
	const char* host = evhttp_uri_get_host(uri);
	int port = evhttp_uri_get_port(uri);
	const char* path = evhttp_uri_get_path(uri);
	const char* query = evhttp_uri_get_query(uri);
	if (port == -1) port = 80;
	evhttp_connection* conn = evhttp_connection_base_new(nextBase(), NULL, host, port);
	if (conn == nullptr) {
		return wlet(nullptr, _T("Connect to [%s:%d] failed for url."), host, port, xtstring(url).c_str());
	}
	std::string query_path = std::string(path != nullptr ? path : "");
	if (!query_path.empty()) {
		if (query != nullptr) {
			query_path += "?";
			query_path += query;
		}
	}
	else {
		query_path = "/";
	}
	int result = evhttp_make_request(conn, request, method, query_path.c_str());
	if (result != 0) {
		return wlet(nullptr, _T("Request http[%s] failed."), xtstring(url).c_str());
	}
	xstring<char> host_port(xstring<char>::format("%s:%d", host, port));
	evhttp_add_header(request->output_headers, "Host", host_port);
	if (contentType != NULL) {
		evhttp_add_header(request->output_headers, "ContentType", contentType);
	}
	finalize.cancel();
	return request;
}

bool CEventBase::startup() {
	if (m_eventBases.size() < 1) return false;
	m_threadCount = 0;
	CSystemInfo::SetThreadCount(static_cast<unsigned int>(m_eventBases.size()));
	for (auto base : m_eventBases) {
		new xthread([this, base]() {
			while (!m_willQuit) {
				unsigned int tid(std::tid());
				m_baseThreads.insert(std::make_pair(tid, base));
				int result(event_base_loop(base, 0));
				if (result < 0) {
					cpe(_T("启动事件循环失败！"));
					wle(_T("Start event dispatch loop failed, result=%d"), result);
					break;
				}
				std::sleep(15);
			}
			cpm(_T("事件循环退出"));
			CSystemInfo::WorkThreadExit();
			m_threadCount--;
		});
		m_threadCount++;
	}
	return true;
}
void CEventBase::shutdown() {
	m_willQuit = true;
	for (auto listener : m_listeners) {
		evconnlistener_disable(listener);
	}
	for (auto base : m_eventBases) {
		event_base_loopbreak(base);
	}
	while (m_threadCount > 0) {
		std::sleep(100);
	}
	std::sleep(100);
	m_listeners.clear();
}
