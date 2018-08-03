#include <sstream>
#include "Caller.h"
#include "Engine.h"
#include "ConsoleUI.h"

std::atomic<unsigned int> CCaller::m_callerIndex(0);
CCaller::CCaller()
	: CCaller(std::to_string(m_callerIndex++)) {
}
CCaller::CCaller(const std::string& stub)
	: m_stub(stub)
	, m_caller(nullptr, [](bufferevent* p) {
		if (p != nullptr) {
			bufferevent_setcb(p, nullptr, nullptr, nullptr, nullptr);
			bufferevent_free(p);
		}
	})
	, m_callee(nullptr, [](bufferevent* p) {
		if (p != nullptr) {
			bufferevent_setcb(p, nullptr, nullptr, nullptr, nullptr);
			bufferevent_free(p);
		}
	})
	, m_callerHeader()
	, m_calleeHeader()
	, m_calleeContentLength(0)
	, m_calleeReceived(0) {
}
CCaller::~CCaller() {
	cpj(w"[%p] Call[%s] finished.", this, m_stub.c_str());
}

void CCaller::start(bufferevent* bev, const std::map<std::string, std::string>& headers) {
	m_callerHeader = headers;
	m_caller.reset(bev);
	bufferevent_setcb(bev, NULL, on_write, on_error, this);
	bufferevent_enable(bev, EV_READ | EV_WRITE);
}
void CCaller::reply(bufferevent* bev, const std::map<std::string, std::string>& headers) {
	m_calleeHeader = headers;
	if (m_calleeHeader.find("Content-Length") != m_calleeHeader.end()) {
		m_calleeContentLength = std::strtoull(m_calleeHeader["Content-Length"].c_str(), nullptr, 10);
	} else {
		m_calleeContentLength = 0;
	}
	responseCaller();

	m_callee.reset(bev);
	bufferevent_setcb(bev, on_read, NULL, on_error, this);
	bufferevent_enable(bev, EV_READ | EV_WRITE);
	evbuffer* input = bufferevent_get_input(m_callee.get());
	size_t size = evbuffer_get_length(input);
	if (size > 0) on_read(bev);
}

void CCaller::on_read(bufferevent* bev, void* userdata) {
	CCaller* thiz(reinterpret_cast<CCaller*>(userdata));
	thiz->on_read(bev);
}
void CCaller::on_read(bufferevent* bev) {
	evbuffer* input = bufferevent_get_input(m_callee.get());
	evbuffer* output = bufferevent_get_output(m_caller.get());
	m_calleeReceived += evbuffer_get_length(input);
	evbuffer_add_buffer(output, input);
	if (m_calleeContentLength != 0 && m_calleeReceived >= m_calleeContentLength) responseCallee();
}
void CCaller::on_write(bufferevent* event, void* userdata) {
	CCaller* thiz(reinterpret_cast<CCaller*>(userdata));
	thiz->on_write(event);
}
void CCaller::on_write(bufferevent* event) {

}
void CCaller::on_error(bufferevent* event, short what, void* userdata) {
	CCaller* thiz(reinterpret_cast<CCaller*>(userdata));
	thiz->on_error(event, what);
}
void CCaller::on_error(bufferevent* event, short what) {
	if (event == m_callee.get()) {
		bufferevent_setcb(m_callee.get(), NULL, nullptr, nullptr, this);
		m_callee.reset(nullptr);
	}
	if (event == m_caller.get()) {
		bufferevent_setcb(m_caller.get(), NULL, nullptr, nullptr, this);
		m_caller.reset(nullptr);
	}
	if (m_caller == nullptr) {
		CEngine::singleton().removeCaller(m_stub);
	}
}

void CCaller::responseCaller() {
	std::string version = m_callerHeader["Version"];
	std::ostringstream oss;
	oss << version << " 200 OK" << std::endl
		<< "Server: comet" << std::endl
		<< "Access-Control-Allow-Origin: *" << std::endl
		<< "Connection: close" << std::endl;
	for (auto kv : m_calleeHeader) {
		auto key = kv.first;
		if (key == "Path" || key == "Host" || key == "Version") continue;
		if (key == "Accept" || key == "Method" || key == "User-Agent") continue;
		oss << kv.first << ": " << kv.second << std::endl;
	}
	oss << std::endl;
	std::string response = oss.str();
	bufferevent_write(m_caller.get(), response.c_str(), response.length());
	bufferevent_flush(m_caller.get(), EV_WRITE, BEV_FLUSH);
}
void CCaller::responseCallee() {
	const static char* message = "{\"result\": \"ok\"}\r\n";
	const static size_t length = strlen(message);
	std::string version = m_calleeHeader["Version"];
	std::ostringstream oss;
	oss << version << " 200 OK" << std::endl
		<< "Server: comet" << std::endl
		<< "Access-Control-Allow-Origin: *" << std::endl
		<< "Connection: close" << std::endl
		<< "Content-Length: " << length << std::endl
		<< "Content-Type: " << "application / json; charset = utf - 8" << std::endl
		<< std::endl;
	std::string response = oss.str();
	bufferevent_write(m_callee.get(), response.c_str(), response.length());
	bufferevent_write(m_callee.get(), message, length);
	bufferevent_flush(m_callee.get(), EV_WRITE, BEV_FLUSH);
}
