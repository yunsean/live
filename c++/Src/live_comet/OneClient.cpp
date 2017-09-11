#include <iostream>  
#include <sstream> 
#include "net.h"
#include "Channel.h"
#include "Caller.h"
#include "OneClient.h"
#include "Engine.h"
#include "Writelog.h"
#include "ConsoleUI.h"
#include "json/value.h"
#include "json/reader.h"

#define MAX_CONTENT_LENGTH	(100 * 1024)

std::mutex COneClient::m_mutex;
std::queue<COneClient*> COneClient::m_clients;
COneClient::COneClient()
	: m_clientIp()
	, m_clientPort(0)
	, m_bev(nullptr)
	, m_cacheSize(512)
	, m_cache(new(std::nothrow) char[m_cacheSize], [](char* ptr) {delete[] ptr; })
	, m_headers()
	, m_contentLength(0)
	, m_bodySize(0)
	, m_workStage(ReadHeader) {
}
COneClient::~COneClient() {
}

COneClient* COneClient::create() {
	if (m_clients.size() > 0) {
		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_clients.size() > 0) {
			auto client(m_clients.front());
			m_clients.pop();
			return client;
		}
	}
	auto client = new COneClient;
	return client;
}
void COneClient::release() {
	std::lock_guard<std::mutex> lock(m_mutex);
	close();
	m_clients.push(this);
}

bool COneClient::open(event_base* base, evutil_socket_t fd, struct sockaddr* addr) {
	close();

	char ip[1024];
	m_clientIp = inet_ntop(AF_INET, &(((sockaddr_in*)addr)->sin_addr), ip, sizeof(ip));
	m_clientPort = ntohs(((sockaddr_in*)addr)->sin_port);
	m_bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
	if (m_bev == NULL) return wlet(false, _T("Create buffer event failed."));
	bufferevent_setcb(m_bev, on_read, NULL, on_error, this);
	bufferevent_enable(m_bev, EV_READ | EV_WRITE);

	return true;
}
void COneClient::close() {
	if (m_bev != nullptr) {
		bufferevent_free(m_bev);
		m_bev = nullptr;
	}
	if (m_cacheSize > 4096) {
		m_cacheSize = 512;
		m_cache.reset(new char[m_cacheSize]);
	}
	m_workStage = ReadHeader;
	m_bodySize = 0;
	m_headers.clear();
	m_params.clear();
}

void COneClient::on_read(struct bufferevent* bufev, void* context) {
	COneClient* thiz = reinterpret_cast<COneClient*>(context);
	thiz->on_read(bufev);
}
void COneClient::on_read(struct bufferevent* bufev) {
	if (m_workStage == ReadHeader) {
		evbuffer* input = bufferevent_get_input(bufev);
		const static char eof_flag[] = "\r\n\r\n";
		evbuffer_ptr eof = evbuffer_search(input, eof_flag, 4, NULL);
		if (eof.pos > 0) {
			if (parse_header(bufev, eof.pos + 4)) {
				onHeader();
			} else {
				release();
			}
		} else if (evbuffer_get_length(input) > 20 * 1024) {
			release();
		}
	} 
	if (m_workStage == ReadBody) {
		evbuffer* input = bufferevent_get_input(bufev);
		size_t size = evbuffer_get_length(input);
		if (size > 0) {
			if (size > m_contentLength - m_bodySize) size = m_contentLength - m_bodySize;
			size_t read = bufferevent_read(bufev, m_cache.get() + m_bodySize, size);
			if (read == 0) return release();
			m_bodySize += read;
			if (m_bodySize >= m_contentLength) return handle();
		}
	}
}
void COneClient::on_error(struct bufferevent* bev, short what, void* context) {
	COneClient* thiz = reinterpret_cast<COneClient*>(context);
	thiz->on_error(bev, what);
}
void COneClient::on_error(struct bufferevent* bev, short what) {
	if (what & (BEV_EVENT_ERROR | BEV_EVENT_EOF | BEV_EVENT_TIMEOUT)) {
		bufferevent_disable(bev, EV_READ | EV_WRITE);
		release();
	}
}

bool COneClient::parse_header(bufferevent* bev, size_t length) {
	if (length > m_cacheSize) {
		m_cacheSize = (length / 512) * 512 + 512;
		m_cache.reset(new char[m_cacheSize]);
	}
	size_t read = bufferevent_read(bev, m_cache.get(), length);
	if (read == 0) return false;
	const char* head = m_cache.get();
	const char* end = m_cache.get() + read;
	const char* tail = head;
	
	while (tail != end && *tail != ' ') ++tail;
	m_headers["Method"] = std::string(head, tail);
	while (tail != end && *tail == ' ') ++tail;
	head = tail;
	while (tail != end && *tail != ' ') ++tail;
	const char* question = reinterpret_cast<const char*>(::memchr(head, '?', tail - head));
	if (question == NULL) {
		m_headers["Path"] = std::string(head, tail);
	} else {
		m_headers["Path"] = std::string(head, question);
		parse_query(question + 1, tail);
	}
	while (tail != end && *tail == ' ') ++tail;
	head = tail;
	while (tail != end && *tail != '\r') ++tail;
	m_headers["Version"] = std::string(head, tail);
	if (tail != end) ++tail;
	head = tail;
	if (head != end && *head == '\n') head = (tail += 1);
	while (head != end && *head != '\r') {
		while (tail != end && *tail != '\r') ++tail;
		const char* colon = reinterpret_cast<const char*>(::memchr(head, ':', tail - head));
		if (colon == NULL) break;
		const char *value = colon + 1;
		while (value != tail && *value == ' ') ++value;
		m_headers[std::string(head, colon)] = std::string(value, tail);
		if (*tail == '\r') head = (tail += 1);
		else head = tail;
		if (head != end && *head == '\n') head = (tail += 1);
	}
	return true;
}
void COneClient::parse_query(const char* head, const char* end) {
	const char* tail = head;
	while (head != end) {
		while (tail != end && *tail != '&') ++tail;
		const char* equal = reinterpret_cast<const char*>(::memchr(head, '=', tail - head));
		if (equal != NULL) m_params[std::string(head, equal)] = std::string(equal + 1, tail);
		if (*tail == '&')head = (tail += 1);
		else head = tail;
	}
}
void COneClient::send_error(const int code, const std::string& reason) {
	if (m_bev == nullptr) return;
	std::string version = m_headers["Version"];
	std::ostringstream oss;
	oss << version << " " << code << " " << reason << std::endl
		<< "Server: comet" << std::endl
		<< "Connection: close" << std::endl
		<< "Content-Length: 0" << std::endl
		<< std::endl;
	std::string response = oss.str();
	bufferevent_write(m_bev, response.c_str(), response.length());
	bufferevent_flush(m_bev, EV_WRITE, BEV_FLUSH);
	m_workStage = Closing;
}
void COneClient::send_header(bool chunked, const std::string& contentType) {
	std::string version = m_headers["Version"];
	std::ostringstream oss;
	oss << version << " 200 OK" << std::endl
		<< "Server: live.comet" << std::endl
		<< "Connection: keep-alive" << std::endl
		<< "Content-Type: " << contentType << std::endl;
	if (chunked) oss << "Transfer-Encoding: chunked" << std::endl;
	oss << std::endl;
	std::string response = oss.str();
	bufferevent_write(m_bev, response.c_str(), response.length());
}
void COneClient::send_result(const std::string& contentType, const std::string& content) {
	std::string version = m_headers["Version"];
	std::ostringstream oss;
	oss << version << " 200 OK" << std::endl
		<< "Server: live.comet" << std::endl
		<< "Connection: closed" << std::endl
		<< "Content-Length: " << content.length() << std::endl
		<< "Content-Type: " << contentType << std::endl
		<< std::endl;
	std::string response = oss.str();
	bufferevent_write(m_bev, response.c_str(), response.length());
	bufferevent_write(m_bev, content.c_str(), content.length());
	m_workStage = Closing;
}

void COneClient::onHeader() {
	std::string path = m_headers["Path"];
	std::string method = m_headers["Method"];
	if (path == "/return" || path == "/push") {
		onReturn();
	} else if (method == "POST") {
		m_contentLength = static_cast<size_t>(::strtol(m_headers["Content-Length"].c_str(), NULL, 10));
		if (m_contentLength > MAX_CONTENT_LENGTH) return send_error(400, "Bad Request");
		m_workStage = ReadBody;
		if (m_cacheSize < m_contentLength) {
			m_cacheSize = (m_contentLength / 512) * 512 + 512;
			m_cache.reset(new char[m_cacheSize]);
		}
	} else if (method == "GET") {
		handle();
	} else {
		send_error(400, "Bad Request");
	}
}
void COneClient::handle() {
	std::string path = m_headers["Path"];
	Json::Value body;
	if (m_bodySize > 0) {
		Json::Reader reader;
		if (!reader.parse(m_cache.get(), m_cache.get() + m_bodySize, body, false)) return send_error(400, "Bad Request");
	}
	if (path == "/sub") return onSub(body);
	else if (path == "/pub") return onPub(body);
	else if (path == "/call") return onCall(body);
	else if (path == "/pull") return onPull(body);
	else if (path == "/inspect") return onInspect(body);
	else if (path == "/info") return onInfo();
	else return send_error(404, "Not Found");
}
void COneClient::onSub(Json::Value& body) {
	std::string token = m_params["token"];
	if (token.empty()) return send_error(400, "Bad Request");
	auto subscriber = CEngine::singleton().regSubscriber(token);
	if (subscriber == nullptr) return send_error(401, "Token exist");
	send_header(true, "application/json; charset=utf-8");
	cpj("[%p] %s:%d subscribe(token=%s)", subscriber.get(), m_clientIp.c_str(), m_clientPort, token.c_str());
	subscriber->start(subscriber, m_bev, body);
	m_bev = nullptr;
	release();
}
void COneClient::onPub(Json::Value& body) {
	std::string cname = m_params["cname"];
	std::string content = m_params["content"];
	if (cname.empty()) return send_error(400, "Bad Request");
	auto channel = CEngine::singleton().getChannel(cname);
	if (channel == nullptr) return send_error(500, "Internal Error");
	int subscribers = 0;
	if (!body.empty()) subscribers = channel->sendMessage(body);
	else if (!content.empty()) subscribers = channel->sendMessage(content);
	else return send_error(400, "Bad Request");
	Json::Value value;
	value["result"] = "ok";
	value["subscriber"] = subscribers;
	send_result("application/json; charset=utf-8", value.toStyledString());
	cpj("[%p] %s:%d notify(cname=%s)", channel.get(), m_clientIp.c_str(), m_clientPort, cname.c_str());
}
void COneClient::onCall(Json::Value& body) {
	std::string token = m_params["token"];
	std::string method = m_params["method"];
	if (token.empty() || method.empty()) return send_error(400, "Bad Request");
	auto subscriber = CEngine::singleton().getSubscriber(token);
	if (subscriber == nullptr) return send_error(401, "No Subscriber");
	auto sp = CEngine::singleton().regCaller();
	if (!subscriber->call(sp->stub(), m_params, body)) {
		CEngine::singleton().removeCaller(sp->stub());
		return send_error(501, "Call Subscriber Error");
	}
	cpj("[%p] %s:%d %s(token=%s, stub=%s)", sp.get(), m_clientIp.c_str(), m_clientPort, method.c_str(), token.c_str(), sp->stub().c_str());
	sp->start(m_bev, m_headers);
	m_bev = nullptr;
	release();
}
void COneClient::onPull(Json::Value& body) {
	std::string stub = m_params["stub"];
	if (stub.empty()) return send_error(400, "Bad Request");
	auto sp = CEngine::singleton().regCaller(stub);
	cpj("[%p] %s:%d pull(stub=%s)", sp.get(), m_clientIp.c_str(), m_clientPort, sp->stub().c_str());
	sp->start(m_bev, m_headers);
	m_bev = nullptr;
	release();
}
void COneClient::onInspect(Json::Value& body) {
	std::string stub = m_params["stub"];
	if (stub.empty()) return send_error(400, "Bad Request");
	auto caller = CEngine::singleton().getCaller(stub);
	if (caller == nullptr) return send_error(401, "No Caller");
	Json::Value value;
	value["result"] = "ok";
	value["status"] = caller->replied() ? "replied" : "waiting";
	send_result("application/json; charset=utf-8", value.toStyledString());
	cpj("[%p] %s:%d inspect(stub=%s)", this, m_clientIp.c_str(), m_clientPort, stub.c_str());
}
void COneClient::onReturn() {
	std::string stub = m_params["stub"];
	if (stub.empty()) return send_error(400, "Bad Request");
	auto caller = CEngine::singleton().getCaller(stub);
	if (caller == nullptr) return send_error(401, "No Caller");
	if (caller->replied()) return send_error(402, "Caller Has Replied");
	cpj("[%p] %s:%d return(stub=%s)", caller.get(), m_clientIp.c_str(), m_clientPort, stub.c_str());
	caller->reply(m_bev, m_headers);
	m_bev = nullptr;
	release();
}
void COneClient::onInfo() {
	Json::Value value;
	CEngine::singleton().generateInfo(value);
	send_result("application/json; charset=utf-8", value.toStyledString());
	cpj("[%p] %s:%d info()", this, m_clientIp.c_str(), m_clientPort);
}
