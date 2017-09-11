#pragma once
#include <vector>
#include <mutex>
#include <atomic>
#include <functional>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/http.h>
#include <event2/http_struct.h>
#include "net.h"
#include "xthread.h"
#include "Byte.h"
#include "SmartHdr.h"

class CEventBase {
public:
	CEventBase();
	~CEventBase();

public:
	static CEventBase& singleton();

public:
	bool initialize(int threadCount = -1);
	event_base* nextBase();
	event_base** allBase(int& count);
	bool addListen(const char* ip, unsigned int port, void(*callback)(struct evconnlistener*, evutil_socket_t, struct sockaddr*, int, void*), void* context = NULL);
	bool startup();
	void addTrigger(const std::function<void()>& trigger);
	void shutdown();

protected:
	bool addSocket(evutil_socket_t fd, short events, void(*callback)(evutil_socket_t, short, void*), void* context = NULL);
	bool addTimer(unsigned int timeout, void (*callback)(evutil_socket_t, short, void*), void* context = NULL);
	bool addHttp(char* ip, unsigned int port, void (*callback)(struct evhttp_request*, void*), void* context = NULL);
	bufferevent* createBuffer(evutil_socket_t fd, void(*on_read)(struct bufferevent*, void*), void(*on_write)(struct bufferevent*, void*), void(*on_error)(struct bufferevent*, short, void*), void* context = NULL);
	evhttp_request* httpGet(const char* url, void(*callback)(struct evhttp_request*, void*), void* context = NULL);
	evhttp_request* httpRequest(const char* url, evhttp_cmd_type method, const char* contentType, size_t contentLen, void(*callback)(struct evhttp_request*, void*), void* context = NULL);

protected:
	static void libevent_log_cb(int severity, const char *msg);
	static void libevent_fatal_cb(int err);
		
private:
	std::vector<event_base*> m_eventBases;
	std::atomic<int> m_threadCount;
	std::vector<CSmartHdr<evconnlistener*, void>> m_listeners;
	std::atomic<unsigned int> m_nextIndex;
	bool m_willQuit;
};

