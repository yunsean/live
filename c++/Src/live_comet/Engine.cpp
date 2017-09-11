#include "net.h"
#include "json/value.h"
#include "xstring.h"
#include "Engine.h"
#include "Channel.h"
#include "Writelog.h"
#include "ConsoleUI.h"
#include "OneClient.h"
#include "xaid.h"
#include "xutils.h"
#include "xsystem.h"

#define CHANNEL_CHECK_INTERVAL	10			//s
#define CHANNEL_MAX_IDEL		(30 * 1000)	//ms

CEngine::CEngine()
	: m_currentTick(std::tickCount())
	, m_evbase(nullptr)
	, m_timerEvent(nullptr)
	, m_sigintEvent(nullptr)
	, m_sigtermEvent(nullptr)
	, m_listener(nullptr)
{
}
CEngine::~CEngine() {
	shutdown();
}

CEngine& CEngine::singleton() {
	static CEngine instance;
	return instance;
}

void CEngine::libevent_log_cb(int severity, const char* msg) {
	cpe(msg);
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
void CEngine::libevent_fatal_cb(int err) {
	cpe(_T("ºËÐÄÒýÇæ´íÎó£¡"));
	wle(_T("event failed: %d"), err);
}

bool CEngine::startup(const int port /*= 88*/) {
	event_set_log_callback(libevent_log_cb);
	event_set_fatal_callback(libevent_fatal_cb);
#ifdef _WIN32
	if (evthread_use_windows_threads() != 0) {
#else 
	if (evthread_use_pthreads() != 0) {
#endif
		return wlet(false, _T("init multiple thread failed."));
	}

	m_evbase = event_base_new();
	if (!m_evbase) return wlet(false, "create evbase error!\n");

	m_sigintEvent = evsignal_new(m_evbase, SIGINT, signal_cb, this);
	if (!m_sigintEvent || event_add(m_sigintEvent, NULL) < 0) return wlet(false, "Could not create/add a signal event!\n");
	m_sigtermEvent = evsignal_new(m_evbase, SIGTERM, signal_cb, this);
	if (!m_sigtermEvent || event_add(m_sigtermEvent, NULL) < 0) return wlet(false, "Could not create/add a signal event!\n");
	m_timerEvent = event_new(m_evbase, -1, EV_PERSIST, timer_cb, this);
	struct timeval tv;
	tv.tv_sec = CHANNEL_CHECK_INTERVAL;
	tv.tv_usec = 0;
	if (!m_timerEvent || evtimer_add(m_timerEvent, &tv) < 0) return wlet(false, "Could not create/add a timer event!\n");

	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	int status = inet_aton("0.0.0.0", &server_addr.sin_addr);
	if (status == 0) return wlet(false, _T("Convert ip address failed."));
	int family = server_addr.sin_family;
	evutil_socket_t fd = socket(family, SOCK_STREAM, 0);
	if (fd == -1) return wlet(false, _T("Create socket failed."));
	auto finalize(std::destruct_invoke([fd]() { evutil_closesocket(fd); }));
	if (evutil_make_socket_nonblocking(fd) < 0) return wlet(false, _T("Setup socket failed."));
	int on = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (const char*)&on, sizeof(on)) < 0) return wlet(false, _T("Setup socket failed."));
	if (evutil_make_listen_socket_reuseable(fd) < 0) return wlet(false, _T("Setup socket failed."));
	if (bind(fd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(sockaddr)) < 0) return wlet(false, _T("Setup socket failed."));
	m_listener = evconnlistener_new(m_evbase, listen_callback, this, LEV_OPT_THREADSAFE | LEV_OPT_REUSEABLE, 100, fd);
	if (m_listener == nullptr) return wlet(false, _T("Create listen socket failed."));
	finalize.cancel();
	int result(event_base_dispatch(m_evbase));
	if (result < 0) return wlet(false, _T("Start event dispatch loop failed, result=%d"), result);
	return true;
}
void CEngine::shutdown() {
	if (m_evbase != nullptr) {
		event_base_loopbreak(m_evbase);
	}
	if (m_timerEvent != nullptr) {
		event_free(m_timerEvent);
		m_timerEvent = nullptr;
	}
	if (m_sigintEvent != nullptr) {
		event_free(m_sigintEvent);
		m_sigintEvent = nullptr;
	}
	if (m_sigtermEvent != nullptr) {
		event_free(m_sigtermEvent);
		m_sigtermEvent = nullptr;
	}
	if (m_listener != nullptr) {
		evconnlistener_free(m_listener);
		m_listener = nullptr;
	}
	cpw(w"Network engine exited.");
}

void CEngine::signal_cb(evutil_socket_t sig, short events, void* userdata) {
	CEngine* thiz(reinterpret_cast<CEngine*>(userdata));
	thiz->signal_cb(sig, events);
}
void CEngine::signal_cb(evutil_socket_t sig, short events) {
	event_base_loopbreak(m_evbase);
}
void CEngine::timer_cb(evutil_socket_t sig, short events, void* userdata) {
	CEngine* thiz(reinterpret_cast<CEngine*>(userdata));
	thiz->timer_cb(sig, events);
}
void CEngine::timer_cb(evutil_socket_t sig, short events) {
	m_currentTick = std::tickCount();
	unsigned int shouldAfter = m_currentTick - CHANNEL_MAX_IDEL;
	for (auto sp : m_subscribers) {
		sp.second->noop(shouldAfter, m_currentTick);
	}
}

void CEngine::listen_callback(struct evconnlistener* evcon, evutil_socket_t fd, struct sockaddr* addr, int addrlen, void* userdata) {
	CEngine* thiz(reinterpret_cast<CEngine*>(userdata));
	thiz->listen_callback(evcon, fd, addr, addrlen);
}
void CEngine::listen_callback(struct evconnlistener* evcon, evutil_socket_t fd, struct sockaddr* addr, int addrlen) {
	xtstring ip(inet_ntoa(((sockaddr_in*)addr)->sin_addr));
	int port(ntohs(((sockaddr_in*)addr)->sin_port));
	event_base* base(evconnlistener_get_base(evcon));
	COneClient* client(COneClient::create());
	auto finalize(std::destruct_invoke([client, fd]() {
		cpe(w"Can not accept client.");
		if (client != nullptr) client->release();
		if (fd != -1) evutil_closesocket(fd);
	}));
	if (client == nullptr) return wle(_T("Create http proxy failed."));
	setkeepalive(fd, 2, 2);
	evutil_make_listen_socket_reuseable(fd);
	if (!client->open(base, fd, addr)) return wle(_T("Set up client [%s:%d] failed."), ip.c_str(), port);
	finalize.cancel();
}

std::shared_ptr<CChannel> CEngine::getChannel(const std::string& cname) {
	auto found = m_channels.find(cname);
	if (found != m_channels.end()) return found->second;
	auto sp = std::make_shared<CChannel>(cname);
	m_channels.insert(std::make_pair(cname, sp));
	return sp;
}
bool CEngine::removeChannel(const std::string& cname) {
	auto found = m_channels.find(cname);
	if (found == m_channels.end()) return false;
	m_channels.erase(found);
	return true;
}

std::shared_ptr<CSubscriber> CEngine::regSubscriber(const std::string& token) {
	auto found = m_subscribers.find(token);
	if (found != m_subscribers.end()) cpw(w"The Subscriber[%s] exist, overwrite it.", token.c_str());
	auto sp = std::make_shared<CSubscriber>(token);
	m_subscribers[token] = sp;
	return sp;
}
std::shared_ptr<CSubscriber> CEngine::getSubscriber(const std::string& token) {
	auto found = m_subscribers.find(token);
	if (found == m_subscribers.end()) return nullptr;
	return found->second;
}
void CEngine::removeSubscriber(const std::string& token) {
	auto found = m_subscribers.find(token);
	if (found == m_subscribers.end()) return;
	m_subscribers.erase(found);
}
std::shared_ptr<CCaller> CEngine::regCaller() {
	auto sp = std::make_shared<CCaller>();
	m_callers.insert(std::make_pair(sp->stub(), sp));
	return sp;
}
std::shared_ptr<CCaller> CEngine::regCaller(const std::string& stub) {
	auto found = m_callers.find(stub);
	if (found != m_callers.end()) cpw(w"The Caller[%s] exist, overwrite it.", stub.c_str());
	auto sp = std::make_shared<CCaller>(stub);
	m_callers.insert(std::make_pair(sp->stub(), sp));
	return sp;
}
std::shared_ptr<CCaller> CEngine::getCaller(const std::string& stub) {
	auto found = m_callers.find(stub);
	if (found == m_callers.end()) return nullptr;
	return found->second;
}
void CEngine::removeCaller(const std::string& stub) {
	auto found = m_callers.find(stub);
	if (found == m_callers.end()) return;
	m_callers.erase(found);
}
void CEngine::generateInfo(Json::Value& info) {
	for (auto channel : m_channels) {
		Json::Value item;
		item["name"] = channel.second->name();
		item["subscriber"] = std::to_string(channel.second->subscriberCount());
		item["message"] = std::to_string(channel.second->messageCount());
		info["channels"].append(item);
	}
	for (auto subscriber : m_subscribers) {
		Json::Value item;
		item["token"] = subscriber.second->token();
		for (auto kv : subscriber.second->channels()) {
			Json::Value channel;
			channel["name"] = kv.first;
			channel["seq"] = std::to_string(kv.second);
			item["channels"].append(channel);
		}
		info["subscribers"].append(item);
	}
	for (auto caller : m_callers) {
		Json::Value item;
		item["stub"] = caller.second->stub();
		item["replied"] = caller.second->replied() ? "yes" : "no";
		info["callers"].append(item);
	}
}