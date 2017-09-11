#include <event2/event.h>
#include "GatewayClient.h"
#include "SourceFactory.h"
#include "SystemInfo.h"
#include "Writelog.h"
#include "ConsoleUI.h"

#define ACTION_STREAMLIST	_T("streamlist")
#define ACTION_PLAYSTREAM	_T("playstream")
#define ACTION_PLAYSOURCE	_T("playsource")
#define ACTION_SYSTEMINFO	_T("systeminfo")

std::mutex CGatewayClient::m_mutex;
std::queue<CGatewayClient*> CGatewayClient::m_clients;
CGatewayClient::CGatewayClient()
	: CHttpClient() {
}
CGatewayClient::~CGatewayClient() {
}

CGatewayClient* CGatewayClient::newInstance(evutil_socket_t fd, struct sockaddr* addr) {
	if (m_clients.size() > 0) {
		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_clients.size() > 0) {
			auto client(m_clients.front());
			m_clients.pop();
			client->set(fd, addr);
			return client;
		}
	}
	auto client = new CGatewayClient;
	client->set(fd, addr);
	return client;
}
void CGatewayClient::release() {
	std::lock_guard<std::mutex> lock(m_mutex);
	close();
	m_clients.push(this);
}

void CGatewayClient::handle() {
	if (action() == nullptr) return;
	if (action().CompareNoCase(ACTION_STREAMLIST) == 0) {
		onStreamList();
	} else if (action().CompareNoCase(ACTION_SYSTEMINFO) == 0) {
		onSystemInfo();
	}
}
void CGatewayClient::onSystemInfo() {
	cpm(_T("[%s] 获取系统状态"), name());
	xtstring detail(param(_T("detail")));
	bool noDetail(detail.CompareNoCase(_T("false")) == 0 || detail.CompareNoCase(_T("no")) == 0 || detail.CompareNoCase(_T("0")) == 0);
	std::string status(CSystemInfo::singleton().status(!noDetail));
#ifdef _WIN32
	sendHttpSucceedResponse(_T("application/json;charset=GB2312"), status.length());
#else 
	sendHttpSucceedResponse(_T("application/json;charset=UTF-8"), status.length());
#endif
	sendHttpContent(reinterpret_cast<const unsigned char*>(status.c_str()), static_cast<int>(status.length()));
}
void CGatewayClient::onStreamList() {
	cpm(_T("[%s] 获取流列表"), name());
	xtstring xml;
	xtstring device(param(_T("device")));
	xtstring online(param(_T("online")));
	bool onlineOnly(online.CompareNoCase(_T("true")) == 0 || online.CompareNoCase(_T("yes")) == 0 || online.CompareNoCase(_T("1")) == 0);
	if (!CSourceFactory::singleton().streamList(xml, device, onlineOnly)) {
		sendHTTPFailedResponse(_T("Build stream's xml failed."));
	} else {
		std::string _xml(xml.toString());
		sendHttpSucceedResponse(_T("text/xml"), _xml.length());
		sendHttpContent(reinterpret_cast<const unsigned char*>(_xml.c_str()), static_cast<int>(_xml.length()));
	}
}