#include "Channel.h"
#include "Engine.h"
#include "OneClient.h"
#include "json/value.h"
#include "json/writer.h"
#include "ConsoleUI.h"
#include "Writelog.h"
#include "xsystem.h"

#define MAX_MESSAGE_COUNT		10

CChannel::CChannel(const std::string& cname)
	: m_name(cname)
	, m_sequence(0)
	, m_subscribers()
	, m_messages() {
}
CChannel::~CChannel() {
	auto it = m_messages.iterator();
	while (auto* message = it.next()) {
		delete message;
	}
}

bool CChannel::addSubscriber(std::shared_ptr<CSubscriber> subscriber) {
	m_subscribers.push_back(std::weak_ptr<CSubscriber>(subscriber));
	auto it = m_messages.iterator();
	while (auto* message = it.next()) {
		subscriber->notify(m_name, message->seq, message->msg);
	}
	return true;
}
int CChannel::sendMessage(const std::string& message) {
	Json::Value value;
	unsigned long seq = m_sequence++;
	value["type"] = "notify";
	value["seq"] = std::to_string(seq);
	value["message"] = message;
	Json::FastWriter writer;
	std::string msg = writer.write(value);
	return sendChunkedMessage(false, seq, msg);
}
int CChannel::sendMessage(const Json::Value& message) {
	Json::Value value;
	unsigned long seq = m_sequence++;
	value["type"] = "notify";
	value["seq"] = std::to_string(seq);
	value["message"] = message;
	Json::FastWriter writer;
	std::string msg = writer.write(value);
	return sendChunkedMessage(false, seq, msg);
}

void CChannel::cacheMessage(unsigned long seq, const std::string& msg) {
	CMessage* message = nullptr;
	if (m_messages.count() >= MAX_MESSAGE_COUNT) {
		message = m_messages.pop_front();
	} else{
		message = new CMessage();
	}
	message->seq = seq;
	message->msg = msg;
	m_messages.push_back(message);
}
int CChannel::sendChunkedMessage(bool noop, const int seq, const std::string& message) {
	std::stringstream ss;
	ss << std::hex << (message.length()) << "\r\n" << message << "\r\n";
	std::string content = ss.str();
	int subscribers = 0;
	m_subscribers.erase(std::remove_if(m_subscribers.begin(), m_subscribers.end(), 
		[&](std::weak_ptr<CSubscriber> wp) {
		auto sp = wp.lock();
		if (sp == nullptr) return true;
		if (sp->notify(m_name, seq, content)) subscribers++;
		return false;
	}), m_subscribers.end());
	cacheMessage(seq, content);
	return subscribers;
}
