#include <event2/event.h>
#include <event2/thread.h>
#include <event2/event_struct.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/http.h>
#include <event2/listener.h>
#include "net.h"
#include "FlvFactory.h"
#include "Profile.h"
#include "Writelog.h"
#include "ConsoleUI.h"
#include "json/value.h"
#include "xaid.h"

#define ACTION_PLAYSTREAM	_T("playflv")

unsigned int CFlvFactory::m_lLicenseCount(0);
std::atomic<unsigned int> CFlvFactory::m_lInvokeCount(0);
std::atomic<unsigned int> CFlvFactory::m_lPlayingCount(0);
std::atomic<uint64_t> CFlvFactory::m_lTotalReceived(0);
std::atomic<uint64_t> CFlvFactory::m_lTotalSend(0);
CFlvFactory::CFlvFactory()
	: m_lpszFactoryName(_T("Flv Sender"))
	, m_lpCallback(nullptr)

	, m_lkWriters()
	, m_liveWriter(){
}
CFlvFactory::~CFlvFactory() {
}

CFlvFactory& CFlvFactory::singleton() {
	static CFlvFactory instance;
	return instance;
}

LPCTSTR CFlvFactory::FactoryName() const {
	return m_lpszFactoryName;
}
bool CFlvFactory::Initialize(ISinkFactoryCallback* callback) {
#ifdef _WIN32
	if (evthread_use_windows_threads() != 0) {
#else 
	if (evthread_use_pthreads() != 0) {
#endif
		return wlet(false, _T("init multiple thread failed."));
	}
	m_lpCallback = callback;
	return true;
}
bool CFlvFactory::HandleRequest(ISinkHttpRequest* request) {
	xtstring action(request->GetAction());
	if (action.CompareNoCase(ACTION_PLAYSTREAM) == 0) {
		xtstring device(request->GetParam(_T("device")));
		xtstring moniker(request->GetParam(_T("moniker")));
		xtstring param(request->GetParam(_T("param")));
		moniker.Trim();
		device.Trim();
		param.Trim();
		if (moniker.IsEmpty()) {
			xtstring ip(request->GetParam(_T("ip")));
			xtstring sport(request->GetParam(_T("port")));
			int port(sport.empty() ? 80 : _ttoi(sport));
			xtstring stream(request->GetParam(_T("stream")));
			if (ip.IsEmpty() || port <= 0 || port > 65535) return false;
			stream.Trim();
			if (stream.IsEmpty())moniker.Format(_T("%s:%d"), ip.c_str(), port);
			else moniker.Format(_T("%s:%d~%s"), ip.c_str(), port, stream.c_str());
		}
		if (param.IsEmpty()) {
			xtstring user(request->GetParam(_T("user")));
			xtstring pwd(request->GetParam(_T("pwd")));
			if (!user.IsEmpty() || !pwd.IsEmpty())param.Format(L"%s@%s", pwd.c_str(), user.c_str());
		}
		if (moniker.IsEmpty()) return false;
		cpw(_T("[%s] 播放流请求"), moniker.c_str());
		bool result(CFlvFactory::singleton().bindWriter(request, device, moniker, param));
		if (!result) return false;
		else return true;
	}
	return false;
}
bool CFlvFactory::GetStatusInfo(LPSTR* json, void(**free)(LPSTR)) {
	auto number2Size([](uint64_t size) -> xstring<char> {
		if (size > 1024LL * 1024 * 1024 * 512) {
			return xstring<char>::format("%.2f TB", size / (1024 * 1024 * 1024 * 1024.f));
		} else if (size > 1024 * 1024 * 512) {
			return xstring<char>::format("%.2f GB", size / (1024 * 1024 * 1024.f));
		} else if (size > 1024 * 512) {
			return xstring<char>::format("%.2f MB", size / (1024 * 1024.f));
		} else {
			return xstring<char>::format("%.2f KB", size / (1024.f));
		}
	});

	std::lock_guard<std::recursive_mutex> lock(m_lkWriters);
	Json::Value value;
	value["插件名称"] = "源流转发";
	value["授权数量"] = m_lLicenseCount;
	value["连接总数"] = (unsigned int)m_lInvokeCount;
	value["当前连接"] = (unsigned int)m_lPlayingCount;
	value["总发送量"] = number2Size(m_lTotalSend);
	value["总接收量"] = number2Size(m_lTotalReceived);
	value["对象数量"] = (unsigned int)m_liveWriter.size();
	if (m_liveWriter.size() > 0) {
		Json::Value sources;
		for (auto it : m_liveWriter) {
			Json::Value item;
			item[it.first.toString()] = it.second->GetStatus();
			sources.append(item);
		}
		value["活跃对象"] = sources;
	}
	std::string text(value.toStyledString());
	const CHAR* lpsz(text.c_str());
	const size_t len(text.length() + 1);
	CHAR* result = new CHAR[len];
	strcpy_s(result, len, lpsz);
	*json = result;
	*free = [](LPSTR ptr) {
		delete[] reinterpret_cast<CHAR*>(ptr);
	};
	return true;
}
void CFlvFactory::Uninitialize() {
}
void CFlvFactory::Destroy() {
}

bool CFlvFactory::bindWriter(ISinkHttpRequest* client, const xtstring& device, const xtstring& moniker, const xtstring& params) {
	std::unique_lock<std::recursive_mutex> lock(m_lkWriters);
	auto source(m_liveWriter.find(moniker));
	if (source != m_liveWriter.end()) {
		if (source->second->AddClient(client)) {
			return true;
		}
		return false;
	}
	CWriterPtr spWriter(new(std::nothrow) CFlvWriter(moniker, true));
	if (spWriter == NULL) {
		return wlet(false, _T("Create CWriter() failed."));
	}
	ISinkProxyCallback* callback(m_lpCallback->AddSink(spWriter, moniker, device, params));
	if (callback == NULL) {
		return wlet(false, _T("Add Sink to manager failed."));
	}
	if (!spWriter->Startup(callback)) {
		callback->OnDiscarded();
		return wlet(false, _T("Startup Writer failed."));
	}
	if (!spWriter->AddClient(client)) {
		callback->OnDiscarded();
		return false;
	}
	m_liveWriter.insert(std::make_pair(moniker, spWriter));
	return true;
}
void CFlvFactory::unbindWriter(CFlvWriter* source) {
	std::lock_guard<std::recursive_mutex> lock(m_lkWriters);
	xtstring moniker(source->GetMoniker());
	auto found(m_liveWriter.find(moniker));
	if (found != m_liveWriter.end() && found->second.GetPtr() == source) {
		m_liveWriter.erase(found);
	} else {
		wlw(_T("Internal eror: not found %s in factory."), moniker.c_str());
	}
}

void CFlvFactory::AddPlayingCount() {
	++m_lPlayingCount;
	++m_lInvokeCount;
	UpdateTitle();
}
void CFlvFactory::DelPlayingCount() {
	--m_lPlayingCount;
	UpdateTitle();
}
void CFlvFactory::AddTotalDataSize(uint64_t received, uint64_t send) {
	m_lTotalReceived += received;
	m_lTotalSend += send;
}
void CFlvFactory::UpdateTitle() {
	CConsoleUI::Singleton().SetTitle(_T("数据转发网关 - 总连接:%u, 正在播放:%u"), (unsigned int)m_lInvokeCount, (unsigned int)m_lPlayingCount);
}