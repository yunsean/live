#pragma once
#include <mutex>
#include <queue>
#include <map>
#include <memory>
#include <event2/bufferevent.h>
#include <event2/thread.h>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/http.h>
#include <event2/http_struct.h>
#include <event2/keyvalq_struct.h>
#include "xstring.h"
#include "SmartPtr.h"
#include "json/value.h"

class CChannel;
enum Mode;
class COneClient {
public:
	COneClient();
	~COneClient();

public:
	static COneClient* create();

public:
	bool open(event_base* base, evutil_socket_t fd, struct sockaddr* addr);
	void close();
	void release();
	
private:
	static void on_read(struct bufferevent* event, void* context);
	void on_read(struct bufferevent* event);
	static void on_error(struct bufferevent* bev, short what, void* context);
	void on_error(struct bufferevent* bev, short what);

protected:
	bool parse_header(bufferevent* bev, size_t length);
	void parse_query(const char* begin, const char* end);
	void send_error(const int code, const std::string& reason);
	void send_header(bool chunked, const std::string& contentType);
	void send_result(const std::string& contentType, const std::string& contennt);
	void onHeader();
	void handle();

protected:
	void onSub(Json::Value& body);
	void onPub(Json::Value& body);
	void onCall(Json::Value& body);
	void onPull(Json::Value& body);
	void onInspect(Json::Value& body);
	void onReturn();
	void onInfo();

protected:
	static std::mutex m_mutex;
	static std::queue<COneClient*> m_clients;

protected:
	enum WorkStage{ReadHeader, ReadBody, Closing};

private:
	std::string m_clientIp;
	int m_clientPort;
	bufferevent* m_bev;
	size_t m_cacheSize;
	std::unique_ptr<char, std::function<void(char*)>> m_cache;
	std::map<std::string, std::string> m_headers;
	std::map<std::string, std::string> m_params;
	size_t m_contentLength;
	size_t m_bodySize;
	WorkStage m_workStage;
};

