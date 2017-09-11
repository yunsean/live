#include <event2/event.h>
#include <event2/thread.h>
#include <event2/event_struct.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/http.h>
#include <event2/listener.h>
#include "net.h"
#include "RawFactory.h"
#include "ClientProxy.h"
#include "Profile.h"
#include "Writelog.h"
#include "ConsoleUI.h"
#include "json/value.h"
#include "xaid.h"

unsigned int CRawFactory::m_lLicenseCount(0);
std::atomic<unsigned int> CRawFactory::m_lInvokeCount(0);
std::atomic<unsigned int> CRawFactory::m_lPlayingCount(0);
std::atomic<uint64_t> CRawFactory::m_lTotalReceived(0);
std::atomic<uint64_t> CRawFactory::m_lTotalSend(0);
CRawFactory::CRawFactory()
	: m_lpszFactoryName(_T("Raw Sender"))
	, m_lpCallback(nullptr)
	, m_listeners()

	, m_lkWriters()
	, m_liveWriter(){
}
CRawFactory::~CRawFactory() {
}

CRawFactory& CRawFactory::singleton() {
	static CRawFactory instance;
	return instance;
}

LPCTSTR CRawFactory::FactoryName() const {
	return m_lpszFactoryName;
}
bool CRawFactory::Initialize(ISinkFactoryCallback* callback) {
#ifdef _WIN32
	if (evthread_use_windows_threads() != 0) {
#else 
	if (evthread_use_pthreads() != 0) {
#endif
		return wlet(false, _T("init multiple thread failed."));
	}
	m_lpCallback = callback;

	CProfile profile(CProfile::Default());
	int port(profile(_T("Raw"), _T("Listen port"), _T("88")));
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	int status = inet_aton("0.0.0.0", &server_addr.sin_addr);
	if (status == 0) {
		return wlet(false, _T("Convert ip address failed."));
	}
	int family = server_addr.sin_family;
	evutil_socket_t fd = socket(family, SOCK_STREAM, 0);
	if (fd == -1) {
		return wlet(false, _T("Create socket failed."));
	}
	if (evutil_make_socket_nonblocking(fd) < 0) {
		evutil_closesocket(fd);
		return wlet(false, _T("Setup socket failed."));
	}
	int on = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (const char*)&on, sizeof(on)) < 0) {
		evutil_closesocket(fd);
		return wlet(false, _T("Setup socket failed."));
	}
	if (evutil_make_listen_socket_reuseable(fd) < 0) {
		evutil_closesocket(fd);
		return wlet(false, _T("Setup socket failed."));
	}
	if (bind(fd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(sockaddr)) < 0) {
		evutil_closesocket(fd);
		return wlet(false, _T("Setup socket failed."));
	}

	int count(0);
	event_base** bases(callback->GetAllBase(count));
	if (bases == NULL || count < 1) {
		evutil_closesocket(fd);
		return wlet(false, _T("No event base."));
	}
	for (int i = 0; i < count; i++) {
		CSmartHdr<evconnlistener*, void> listen(evconnlistener_new(bases[i], listen_callback, this, LEV_OPT_THREADSAFE | LEV_OPT_REUSEABLE, 100, fd), evconnlistener_free);
		if (!listen.isValid()) {
			evutil_closesocket(fd);
			return wlet(false, _T("Create listen socket failed."));
		}
		m_listeners.push_back(listen);
	}
	return true;
}
bool CRawFactory::GetStatusInfo(LPSTR* json, void(**free)(LPSTR)) {
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
void CRawFactory::Uninitialize() {
	m_listeners.clear();
}
void CRawFactory::Destroy() {
}

void CRawFactory::listen_callback(struct evconnlistener* conn, evutil_socket_t fd, struct sockaddr* addr, int addrlen, void* context) {
	CRawFactory* thiz(reinterpret_cast<CRawFactory*>(context));
	thiz->listen_callback(conn, fd, addr, addrlen);
}
void CRawFactory::listen_callback(struct evconnlistener* conn, evutil_socket_t fd, struct sockaddr* addr, int addrlen) {
	xtstring ip(inet_ntoa(((sockaddr_in*)addr)->sin_addr));
	int port(ntohs(((sockaddr_in*)addr)->sin_port));
	CClientProxy* client(CClientProxy::newInstance(fd, addr));
	setkeepalive(fd, 5, 5);
	evutil_make_listen_socket_reuseable(fd);
	auto finalize(std::destruct_invoke([client, fd]() {
		client->release();
		evutil_closesocket(fd);
	}));
	if (!client->run(evconnlistener_get_base(conn))) {
		return wle(_T("Set up client [%s:%d] failed."), ip.c_str(), port);
	}
	finalize.cancel();
}

bool CRawFactory::bindWriter(CHttpClient* client, const xtstring& device, const xtstring& moniker, const xtstring& params) {
	std::unique_lock<std::recursive_mutex> lock(m_lkWriters);
	auto source(m_liveWriter.find(moniker));
	if (source != m_liveWriter.end()) {
		if (source->second->AddClient(client)) {
			return true;
		}
		return false;
	}
	CWriterPtr spWriter(new(std::nothrow) CRawWriter(moniker, true));
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
void CRawFactory::unbindWriter(CRawWriter* source) {
	std::lock_guard<std::recursive_mutex> lock(m_lkWriters);
	xtstring moniker(source->GetMoniker());
	auto found(m_liveWriter.find(moniker));
	if (found != m_liveWriter.end() && found->second.GetPtr() == source) {
		m_liveWriter.erase(found);
	} else {
		wlw(_T("Internal eror: not found %s in factory."), moniker.c_str());
	}
}

void CRawFactory::AddPlayingCount() {
	++m_lPlayingCount;
	++m_lInvokeCount;
	UpdateTitle();
}
void CRawFactory::DelPlayingCount() {
	--m_lPlayingCount;
	UpdateTitle();
}
void CRawFactory::AddTotalDataSize(uint64_t received, uint64_t send) {
	m_lTotalReceived += received;
	m_lTotalSend += send;
}
void CRawFactory::UpdateTitle() {
	CConsoleUI::Singleton().SetTitle(_T("数据转发网关 - 总连接:%u, 正在播放:%u"), (unsigned int)m_lInvokeCount, (unsigned int)m_lPlayingCount);
}