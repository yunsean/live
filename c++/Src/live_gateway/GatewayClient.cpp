#include <event2/event.h>
#include "GatewayClient.h"
#include "ChannelFactory.h"
#include "SourceFactory.h"
#include "SinkFactory.h"
#include "SystemInfo.h"
#include "Writelog.h"
#include "ConsoleUI.h"

#define ACTION_STREAMLIST	_T("streamlist")
#define ACTION_SYSTEMINFO	_T("systeminfo")
#define ACTION_PTZCONTROL	_T("ptz")

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
	} else if (action().CompareNoCase(ACTION_PTZCONTROL) == 0) {
		onPtzControl();
	} else if (!CSinkFactory::singleton().handleRequest(this)) {
		sendHTTPFailedResponse(_T("Unsupport method"), 401);
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
		sendHTTPFailedResponse(_T("Build stream's xml failed."), 404);
	} else {
		std::string _xml(xml.toString());
		std::map<xtstring, xtstring> payload;
		payload.insert(std::make_pair(_T("Access-Control-Allow-Origin"), _T("*")));
		sendHttpSucceedResponse(_T("text/xml"), _xml.length(), payload);
		sendHttpContent(reinterpret_cast<const unsigned char*>(_xml.c_str()), static_cast<int>(_xml.length()));
	}
}
void CGatewayClient::onPtzControl() {
	xtstring device(param(_T("device")));
	xtstring moniker(param(_T("moniker")));
	xtstring params(param(_T("param")));
	xtstring action(param(_T("action")));
	xtstring speed(param(_T("speed")));
	if (moniker.IsEmpty() || action.IsEmpty()) return sendHTTPFailedResponse(_T("Invalid request."), 404);
	CChannel* channel(CChannelFactory::singleton().getChannel(device, moniker, params));
	if (channel == nullptr) return sendHTTPFailedResponse(_T("Open source failed."), 404);
	int ptzAction(ISourceProxy::Stop);
	auto convertAction([](const xtstring& action) -> ISourceProxy::PTZAction {
		if (action.CompareNoCase(_T("ZoomIn")) == 0 || action.CompareNoCase(_T("zi")) == 0) return ISourceProxy::ZoomIn;
		else if (action.CompareNoCase(_T("ZoomOut")) == 0 || action.CompareNoCase(_T("zo")) == 0) return ISourceProxy::ZoomOut;
		else if (action.CompareNoCase(_T("FocusNear")) == 0 || action.CompareNoCase(_T("fn")) == 0) return ISourceProxy::FocusNear;
		else if (action.CompareNoCase(_T("FocusFar")) == 0 || action.CompareNoCase(_T("ff")) == 0) return ISourceProxy::FocusFar;
		else if (action.CompareNoCase(_T("IrisOpen")) == 0 || action.CompareNoCase(_T("io")) == 0) return ISourceProxy::IrisOpen;
		else if (action.CompareNoCase(_T("IrisClose")) == 0 || action.CompareNoCase(_T("ic")) == 0) return ISourceProxy::IrisClose;
		else if (action.CompareNoCase(_T("TiltUp")) == 0 || action.CompareNoCase(_T("tu")) == 0) return ISourceProxy::TiltUp;
		else if (action.CompareNoCase(_T("TiltDown")) == 0 || action.CompareNoCase(_T("td")) == 0) return ISourceProxy::TiltDown;
		else if (action.CompareNoCase(_T("PanLeft")) == 0 || action.CompareNoCase(_T("pl")) == 0) return ISourceProxy::PanLeft;
		else if (action.CompareNoCase(_T("PanRight")) == 0 || action.CompareNoCase(_T("pr")) == 0) return ISourceProxy::PanRight;
		else if (action.CompareNoCase(_T("Stop")) == 0 || action.CompareNoCase(_T("s")) == 0) return ISourceProxy::Stop;
		else return ISourceProxy::Stop;
	});
	int pos(static_cast<int>(action.find(_T(","))));
	while (pos > 0) {
		ptzAction |= convertAction(action.Left(pos));
		action = action.Mid(pos + 1);
		pos = static_cast<int>(action.find(_T(",")));
	}
	if (!action.IsEmpty()) ptzAction |= convertAction(action);
	int ptzSpeed(1);
	if (!speed.IsEmpty()) ptzSpeed = _ttoi(speed);
	if (!channel->PTZControl(this, ptzAction, ptzSpeed)) {
		sendHTTPFailedResponse(_T("Send PTZ Control failed."), 401);
	}
}