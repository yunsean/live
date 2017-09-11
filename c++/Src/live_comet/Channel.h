#pragma once
#include <string>
#include <map>
#include <memory>
#include <list>
#include "Constant.h"
#include "Subscriber.h"
#include "QuickList.h"
#include "xstring.h"
#include "json/value.h"

class COneClient;
class CChannel {
public:
	CChannel(const std::string& cname);
	~CChannel();
	
public:
	bool addSubscriber(std::shared_ptr<CSubscriber> subscriber);
	int sendMessage(const std::string& message);
	int sendMessage(const Json::Value& message);

	const std::string& name() const { return m_name; }
	size_t messageCount() const { return m_messages.count(); }
	size_t subscriberCount() const { return m_subscribers.size(); }

protected:
	void cacheMessage(unsigned long seq, const std::string& msg);
	int sendChunkedMessage(bool noop, const int seq, const std::string& message);

protected:
	struct CMessage : QuickItem<CMessage> {
		unsigned long seq;
		std::string msg;
	};

private:
	const std::string m_name;
	unsigned long m_sequence;
	std::vector<std::weak_ptr<CSubscriber>> m_subscribers;
	QuickList<CMessage*> m_messages;
};

