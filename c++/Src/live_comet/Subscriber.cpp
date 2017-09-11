#include <sstream>
#include <string>
#include <event2/event.h>
#include <event2/thread.h>
#include <event2/event_struct.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include "Subscriber.h"
#include "ConsoleUI.h"
#include "Writelog.h"
#include "OneClient.h"
#include "Engine.h"
#include "Channel.h"
#include "json/value.h"
#include "json/writer.h"

CSubscriber::CSubscriber(const std::string& token)
	: m_token(token)
	, m_latestSend(CEngine::singleton().currentTick())
	, m_channels()
	, m_bev(nullptr, [](bufferevent* p) {
		if (p != nullptr) {
			bufferevent_setcb(p, nullptr, nullptr, nullptr, nullptr);
			bufferevent_free(p);
		}
	}) {
}
CSubscriber::~CSubscriber() {
	cpj(w"[%p] unsubscribed(token=%s).", this, m_token.c_str());
}

int CSubscriber::start(std::shared_ptr<CSubscriber> self, bufferevent* bev, const Json::Value& subscribes) {
	m_bev.reset(bev);
	bufferevent_setcb(bev, NULL, on_write, on_error, this);
	bufferevent_enable(bev, EV_READ | EV_WRITE);

	int count = 0;
	if (subscribes.isArray()) {
		Json::FastWriter writer;
		for (int i = 0; i < static_cast<int>(subscribes.size()); i++) {
			std::string cname = subscribes[i]["cname"].asString();
			unsigned int seq = subscribes[i]["seq"].asUInt();
			auto channel = CEngine::singleton().getChannel(cname);
			if (channel == nullptr) continue;
			m_channels.insert(std::make_pair(cname, seq));
			channel->addSubscriber(self);
			count++;
		}
	}
	return count;
}

bool CSubscriber::notify(const std::string& cname, unsigned int seq, const std::string& chunk) {
	auto found = m_channels.find(cname);
	if (found == m_channels.end()) return false;
	if (found->second > seq) return false;
	bufferevent_write(m_bev.get(), chunk.c_str(), chunk.length());
	bufferevent_flush(m_bev.get(), EV_WRITE, BEV_FLUSH);
	m_latestSend = CEngine::singleton().currentTick();
	return true;
}
void CSubscriber::noop(const unsigned int shouldAfter, const unsigned int now) {
	static const char* content = "3\r\n{}\n\r\n";
	static const size_t length = strlen(content);
	if (m_latestSend < shouldAfter) {
		bufferevent_write(m_bev.get(), content, length);
		bufferevent_flush(m_bev.get(), EV_WRITE, BEV_FLUSH);
		m_latestSend = now;
	}
}
bool CSubscriber::call(const std::string& stub, const std::map<std::string, std::string>& params, const Json::Value& body) {
	Json::Value value;
	value["type"] = "call";
	value["stub"] = stub;
	for (auto kv : params) {
		value[kv.first] = kv.second;
	}
	if (!body.empty()) value["body"] = body;
	Json::FastWriter writer;
	std::string msg = writer.write(value);
	std::stringstream ss;
	ss << std::hex << (msg.length()) << "\r\n" << msg << "\r\n";
	std::string content = ss.str();
	bufferevent_write(m_bev.get(), content.c_str(), content.length());
	bufferevent_flush(m_bev.get(), EV_WRITE, BEV_FLUSH);
	m_latestSend = CEngine::singleton().currentTick();
	return true;
}

void CSubscriber::on_write(bufferevent* event, void* userdata) {
	CSubscriber* thiz(reinterpret_cast<CSubscriber*>(userdata));
	thiz->on_write(event);
}
void CSubscriber::on_write(bufferevent* event) {

}
void CSubscriber::on_error(bufferevent* event, short what, void* userdata) {
	CSubscriber* thiz(reinterpret_cast<CSubscriber*>(userdata));
	thiz->on_error(event, what);	
}
void CSubscriber::on_error(bufferevent* event, short what) {
	bufferevent_setcb(m_bev.get(), NULL, nullptr, nullptr, this);
	CEngine::singleton().removeSubscriber(m_token);
}

