#pragma once
#include <memory>
#include <functional>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/http.h>
#include <event2/http_struct.h>
#include "xstring.h"
#include "Constant.h"
#include "QuickList.h"
#include "json/value.h"

class COneClient;
class CChannel;
class CSubscriber : public QuickItem<CSubscriber> {
public:
	CSubscriber(const std::string& token);
	~CSubscriber();

public:
	int start(std::shared_ptr<CSubscriber> self, bufferevent* bev, const Json::Value& subscribe);
	bool notify(const std::string& cname, unsigned int seq, const std::string& chunk);
	void noop(const unsigned int shouldAfter, const unsigned int now);
	bool call(const std::string& stub, const std::map<std::string, std::string>& params, const Json::Value& body);

	const std::string& token() const { return m_token; }
	const std::map<std::string, unsigned int>& channels() const { return m_channels; }

protected:
	static void on_write(bufferevent* event, void* userdata);
	void on_write(bufferevent* event);
	static void on_error(bufferevent* event, short what, void* userdata);
	void on_error(bufferevent* event, short what);

private:
	friend class CChannel;
	const std::string m_token;
	unsigned int m_latestSend;
	std::map<std::string, unsigned int> m_channels;
	std::unique_ptr<bufferevent, std::function<void(bufferevent*)>> m_bev;
};

