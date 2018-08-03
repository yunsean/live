#pragma once
#include <mutex>
#include <map>
#include <list>
#include <vector>
#include "SinkInterface.h"
#include "Channel.h"
#include "SmartPtr.h"
#include "json/value.h"
#include "xstring.h"

class CHttpClient;
struct event_base;
class CChannelFactory {
public:
	CChannelFactory();
	~CChannelFactory();

public:
	static CChannelFactory& singleton();

public:
	bool initialize();
	void status(Json::Value& status);
	void uninitialize();

public:
	CChannel* getChannel(const xtstring& device, const xtstring& moniker, const xtstring& params);
	ISinkProxyCallback* bindChannel(ISinkProxy* proxy, const xtstring& device, const xtstring& moniker, const xtstring& params);
	void unbindChannel(CChannel* source);

protected:
	typedef CSmartPtr<CChannel> CChannelPtr;

private:
	std::recursive_mutex m_lkChannels;
	std::map<xtstring, CChannelPtr> m_liveChannels;
	std::vector<CChannelPtr> m_corruptedChannels;
};

