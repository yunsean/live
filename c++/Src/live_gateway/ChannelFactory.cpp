#include "EventBase.h"
#include "ChannelFactory.h"
#include "Channel.h"
#include "SourceFactory.h"
#include "Writelog.h"
#include "ConsoleUI.h"

CChannelFactory::CChannelFactory()
	: m_lkChannels()
	, m_liveChannels()
	, m_corruptedChannels() {
}
CChannelFactory::~CChannelFactory() {
}

CChannelFactory& CChannelFactory::singleton() {
	static CChannelFactory instance;
	return instance;
}

bool CChannelFactory::initialize() {
	return true;
}
void CChannelFactory::status(Json::Value& status) {
	std::unique_lock<std::recursive_mutex> lock(m_lkChannels);
	for (auto sp : m_liveChannels) {
		Json::Value value;
		sp.second->GetStatus(value);
		status.append(value);
	}
	for (auto sp : m_corruptedChannels) {
		Json::Value value;
		sp->GetStatus(value);
		status.append(value);
	}
}
void CChannelFactory::uninitialize() {
	m_liveChannels.clear();
	m_corruptedChannels.clear();
}

ISinkProxyCallback* CChannelFactory::bindChannel(ISinkProxy* proxy, const xtstring& device, const xtstring& moniker, const xtstring& params) {
	std::unique_lock<std::recursive_mutex> lock(m_lkChannels);
	auto found(m_liveChannels.find(moniker));
	if (found != m_liveChannels.end()) {
		ISinkProxyCallback* callback(found->second->AddSink(proxy));
		if (callback != nullptr) {
			return callback;
		}
	}
	CChannelPtr spChannel(new(std::nothrow) CChannel(moniker));
	if (spChannel == NULL) {
		return wlet(nullptr, _T("Create CChannel() failed."));
	}
	ISourceProxy* source(CSourceFactory::singleton().createLiveSource(spChannel, device, moniker, params));
	if (source == nullptr) {
		return wlet(nullptr, _T("Create Channel Proxy failed."));
	}
	if (!spChannel->Startup(source)) {
		source->Discard();
		return wlet(nullptr, _T("Startup Channel failed."));
	}
	ISinkProxyCallback* callback(spChannel->AddSink(proxy));
	if (callback == nullptr) {
		m_corruptedChannels.push_back(spChannel);
		return wlet(nullptr, _T("AddSink() failed"));
	}
	found = m_liveChannels.find(moniker);
	if (found != m_liveChannels.end()) {
		m_corruptedChannels.push_back(found->second);
		m_liveChannels.erase(found);
	}
	m_liveChannels.insert(std::make_pair(moniker, spChannel));
	return callback;
}
void CChannelFactory::unbindChannel(CChannel* source) {
	std::lock_guard<std::recursive_mutex> lock(m_lkChannels);
	xtstring moniker(source->GetMoniker());
	auto found(m_liveChannels.find(moniker));
	if (found != m_liveChannels.end() && found->second.GetPtr() == source) {
		m_liveChannels.erase(found);
	} else {
		auto found(std::find_if(m_corruptedChannels.begin(), m_corruptedChannels.end(),
			[source](CChannelPtr sp) {
			return sp.GetPtr() == source;
		}));
		if (found != m_corruptedChannels.end()) {
			m_corruptedChannels.erase(found);
		} else {
			wlw(_T("Internal eror: not found %s in factory."), moniker.c_str());
		}
	}
}