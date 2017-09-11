#pragma once
#include <map>
#include <memory>
#include <signal.h>
#include <event2/event.h>
#include <event2/thread.h>
#include <event2/event_struct.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/http.h>
#include <event2/listener.h>
#include <event2/http_struct.h>
#include "Channel.h"
#include "Subscriber.h"
#include "Caller.h"
#include "SmartPtr.h"

#define Channel_NotExist	((CChannel*)-1)
#define Channel_TooMany		nullptr

class CEngine {
public:
	CEngine();
	~CEngine();

public:
	static CEngine& singleton();

public:
	bool startup(const int port = 88);
	void shutdown();

public:
	std::shared_ptr<CChannel> getChannel(const std::string& cname);
	bool removeChannel(const std::string& cname);
	std::shared_ptr<CSubscriber> regSubscriber(const std::string& token);
	std::shared_ptr<CSubscriber> getSubscriber(const std::string& token);
	void removeSubscriber(const std::string& token);
	std::shared_ptr<CCaller> regCaller();
	std::shared_ptr<CCaller> regCaller(const std::string& stub);
	std::shared_ptr<CCaller> getCaller(const std::string& stub);
	void removeCaller(const std::string& stub);
	void generateInfo(Json::Value& info);

	unsigned int currentTick() const { return m_currentTick; }

protected:
	static void signal_cb(evutil_socket_t sig, short events, void *userdata);
	void signal_cb(evutil_socket_t sig, short events);
	static void timer_cb(evutil_socket_t sig, short events, void *userdata);
	void timer_cb(evutil_socket_t sig, short events);

protected:
	static void libevent_log_cb(int severity, const char* msg);
	static void libevent_fatal_cb(int err);

private:
	static void listen_callback(struct evconnlistener* evcon, evutil_socket_t fd, struct sockaddr* addr, int addrlen, void* userdata);
	void listen_callback(struct evconnlistener* evcon, evutil_socket_t fd, struct sockaddr* addr, int addrlen);

private:
	unsigned int m_currentTick;
	event_base* m_evbase;
	event* m_timerEvent;
	event* m_sigintEvent;
	event* m_sigtermEvent;
	evconnlistener* m_listener;

	std::map<std::string, std::shared_ptr<CChannel>> m_channels;
	std::map<std::string, std::shared_ptr<CSubscriber>> m_subscribers;
	std::map<std::string, std::shared_ptr<CCaller>> m_callers;
};

