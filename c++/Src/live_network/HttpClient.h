#pragma once
#include <map>
#include <atomic>
#include <memory>
#include "xstring.h"
#include "SmartHdr.h"

#ifndef evutil_socket_t
#ifdef WIN32
#define evutil_socket_t intptr_t
#else
#define evutil_socket_t int
#endif
#endif

struct bufferevent;
struct event_base;
struct evbuffer;
class CHttpClient {
protected:
	CHttpClient();
	virtual ~CHttpClient();

public:
	virtual void set(evutil_socket_t fd, struct sockaddr* addr);
	virtual bool run(event_base* base);
	virtual event_base* base();
	virtual evutil_socket_t fd(bool detach = true);
	virtual void close();
	virtual void release();

protected:
	LPCTSTR name() const { return m_clientName; }
	const xtstring& operation() const { return m_operation; }
	const xtstring& version() const { return m_version; }
	const xtstring& url() const { return m_url; }
	const xtstring& path() const { return m_path; }
	const xtstring& ip() const { return m_clientIp; }
	const int port() const { return m_clientPort; }
	const std::map<xtstring, xtstring>& params() const { return m_params; }
	const std::map<xtstring, xtstring>& payloads() const { return m_payloads; }
	LPCTSTR param(LPCTSTR key) const;
	LPCTSTR payload(LPCTSTR key) const;
	const xtstring& action() const { return m_action; }
	evbuffer* input();
	evbuffer* output();
	bufferevent* bev(bool detach = true);

public:
	const static std::map<xtstring, xtstring>	s_mapNull;
	bool sendHttpRedirectResponse(const xtstring& location, const int code = 302, const xtstring& status = _T("Found"));
	void sendHTTPFailedResponse(const xtstring& reason = _T(""), const int statusCode = 400);
	bool sendHttpSucceedResponse(const xtstring& contentType = _T("text/html"), const int64_t contentLen = -1, const std::map<xtstring, xtstring>& mapPayload = s_mapNull);
	bool sendHttpContent(const void* data, const size_t size);
	std::string getHttpSucceedResponse(const xtstring& contentType = _T("text/html"), const int64_t contentLen = -1, const std::map<xtstring, xtstring>& mapPayload = s_mapNull);
	
protected:
	virtual void handle() = 0;

private:
	static void on_read(struct bufferevent* event, void* context);
	void on_read(struct bufferevent* event);
	static void on_error(struct bufferevent* bev, short what, void* context);
	void on_error(struct bufferevent* bev, short what);

private:
	char asciiHexToChar(char c);
	void unescapeURI(char* str);
	bool tryParseRequet(struct bufferevent* event);
	bool parseHeader();

private:
	static std::atomic<int> m_instanceCount;

private:
	int m_instanceIndex;
	evutil_socket_t m_fd;
	xtstring m_clientIp;
	int m_clientPort;
	bufferevent* m_bev;
	xtstring m_clientName;

	std::shared_ptr<char> m_cache;
	int m_cacheOffset;
	bool m_hasHeader;

	xtstring m_operation;
	xtstring m_version;
	xtstring m_url;
	xtstring m_path;
	std::map<xtstring, xtstring> m_params;
	std::map<xtstring, xtstring> m_payloads;
	xtstring m_action;
};


