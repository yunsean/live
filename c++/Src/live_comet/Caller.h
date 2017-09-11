#pragma once
#include <atomic>
#include <memory>
#include <map>
#include <functional>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/http.h>
#include <event2/http_struct.h>

class CCaller {
public:
	CCaller();
	CCaller(const std::string& stub);
	~CCaller();

public:
	void start(bufferevent* bev, const std::map<std::string, std::string>& headers);
	void reply(bufferevent* bev, const std::map<std::string, std::string>& headers);

	const std::string& stub() const { return m_stub; }
	bool replied() const { return m_callee != nullptr; }

protected:
	static void on_read(bufferevent* event, void* userdata);
	void on_read(bufferevent* event);
	static void on_write(bufferevent* event, void* userdata);
	void on_write(bufferevent* event);
	static void on_error(bufferevent* event, short what, void* userdata);
	void on_error(bufferevent* event, short what);

protected:
	void responseCaller();
	void responseCallee();

private:
	static std::atomic<unsigned int> m_callerIndex;

private:
	std::string m_stub;
	std::unique_ptr<bufferevent, std::function<void(bufferevent*)>> m_caller;
	std::unique_ptr<bufferevent, std::function<void(bufferevent*)>> m_callee;
	std::map<std::string, std::string> m_callerHeader;
	std::map<std::string, std::string> m_calleeHeader;
	uint64_t m_calleeContentLength;
	uint64_t m_calleeReceived;
};

