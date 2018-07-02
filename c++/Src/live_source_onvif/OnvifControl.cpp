#include <event2/event.h>
#include <event2/thread.h>
#include <event2/event_struct.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/http.h>
#include <event2/listener.h>
#include "net.h"
#include "OnvifSource.h"
#include "OnvifControl.h"
#include "OnvifPattern.h"
#include "OnvifFactory.h"
#include "Writelog.h"
#include "ConsoleUI.h"
#include "pugixml/pugixml.hpp"
#include "OnvifPattern.h"
#include "xmember.h"
#include "xsystem.h"
#include "xaid.h"

//////////////////////////////////////////////////////////////////////////
//ControlCommand
COnvifControl::ControlCommand::ControlCommand(const unsigned int t, const int a, const int s)
	: token(t)
	, mode(false)
	, action(a)
	, speed(s) {
}
COnvifControl::ControlCommand::ControlCommand(const unsigned int t, const int b, const int c, const int s, const int h)
	: token(t)
	, mode(true)
	, bright(b)
	, contrast(c)
	, saturation(s)
	, hue(h) {
}


//////////////////////////////////////////////////////////////////////////
static void evdns_base_free(evdns_base* base) {
	evdns_base_free(base, 0);
}
COnvifControl::COnvifControl(COnvifSource* source, const xtstring& ip, const std::string& usr, const std::string& pwd)
	: m_moniker(ip)
	, m_username(usr)
	, m_password(pwd)
	, m_source(source)

	, m_evbase(nullptr)
	, m_dns(evdns_base_free)
	, m_bev(bufferevent_free)
	, m_timer(event_free)
	, m_connection(evhttp_connection_free)
	, m_onvifStage(None) {
}
COnvifControl::~COnvifControl() {
}

bool COnvifControl::IsWorking() {
	return m_onvifStage != None;
}
void COnvifControl::DoCommand(const std::shared_ptr<ControlCommand>& command) {
	m_command = command;
	if (m_evbase == nullptr) m_evbase = COnvifFactory::Singleton().GetPreferBase();
	if (m_serviceAddr.empty()) dnsRequest();
	else if (m_ptzUrl.empty()) ptzCapabilitiesRequest();
	else if (m_profileToken.empty()) mediaCapabilitiesRequest();
	else if (m_command->mode) effectRequest();
	else ptzRequest();
}
void COnvifControl::Discard() {

}

//////////////////////////////////////////////////////////////////////////
//DNS
void COnvifControl::dnsRequest() {
	m_onvifStage = Probe;
	m_dns = evdns_base_new(m_evbase, 1);
	if (!m_dns.isValid()) return onError(_T("解析域名错误")), wle(_T("create dns failed."));
	struct evutil_addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_flags = EVUTIL_AI_CANONNAME;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	evdns_getaddrinfo(m_dns, m_moniker.toString().c_str(), NULL, &hints, getaddrinfo_callback, this);
}
void COnvifControl::getaddrinfo_callback(int result, struct evutil_addrinfo *res, void *arg) {
	COnvifControl* thiz(reinterpret_cast<COnvifControl*>(arg));
	thiz->getaddrinfo_callback(result, res);
}
void COnvifControl::getaddrinfo_callback(int result, struct evutil_addrinfo *addr) {
	if (result != 0) return onError(_T("域名解析失败")), wle(_T("Parse domain failed, result = %d"), result);
	struct evutil_addrinfo *ai;
	for (ai = addr; ai; ai = ai->ai_next) {
		char buf[128];
		const char *s = NULL;
		if (ai->ai_family == AF_INET) {
			struct sockaddr_in *sin = (struct sockaddr_in *)ai->ai_addr;
			s = evutil_inet_ntop(AF_INET, &sin->sin_addr, buf, 128);
		} else if (ai->ai_family == AF_INET6) {
			struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)ai->ai_addr;
			s = evutil_inet_ntop(AF_INET6, &sin6->sin6_addr, buf, 128);
		}
		probeRequest(s);
	}
	evutil_freeaddrinfo(addr);
}

//////////////////////////////////////////////////////////////////////////
//Probe
void COnvifControl::probeRequest(const char* ip) {
	m_onvifStage = Probe;
	evutil_socket_t fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd == -1) return onError(_T("查找设备失败")), wle(_T("Create socket failed."));
	int flag = 1;
	if (evutil_make_listen_socket_reuseable(fd) < 0) {
		evutil_closesocket(fd);
		onError(_T("查找设备失败"));
		return wle(_T("Setup socket failed."));
	}
	m_bev = bufferevent_socket_new(m_evbase, fd, BEV_OPT_CLOSE_ON_FREE);
	if (m_bev == NULL) {
		evutil_closesocket(fd);
		onError(_T("查找设备失败"));
		return wle(_T("Create buffer event failed."));
	}
	bufferevent_setcb(m_bev, on_read, NULL, NULL, this);
	bufferevent_enable(m_bev, EV_READ | EV_WRITE);
	m_timer = evtimer_new(m_evbase, &COnvifControl::timer_callback, this);
	m_messageId = COnvifPattern::messageID();
	std::string message(COnvifPattern::probe("dn:NetworkVideoTransmitter", "", m_messageId.c_str()));
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(3702);
	inet_aton(ip, &addr.sin_addr);
	int result(sendto(fd, message.c_str(), static_cast<int>(message.length()), 0, (struct sockaddr*)&addr, sizeof(addr)));
	if (result == -1) {
		m_bev = nullptr;
		onError(_T("查找设备失败"));
		return wle(_T("send broadcast message failed."));
	}
	static struct timeval tv = { 5, 0 };
	evtimer_add(m_timer, &tv);
}
void COnvifControl::timer_callback(evutil_socket_t fd, short event, void* arg) {
	COnvifControl* thiz(reinterpret_cast<COnvifControl*>(arg));
	if (thiz != nullptr) thiz->timer_callback(fd, event);
}
void COnvifControl::timer_callback(evutil_socket_t fd, short event) {
	onError(_T("OnVif发现设备超时"));
	wle(_T("probe onvif device timeout."));
}
void COnvifControl::on_read(struct bufferevent* event, void* context) {
	COnvifControl* thiz(reinterpret_cast<COnvifControl*>(context));
	thiz->on_read(event);
}
void COnvifControl::on_read(struct bufferevent* event) {
	return probeResponse(event);
}
void COnvifControl::probeResponse(struct bufferevent* bufev) {
	evbuffer* input = bufferevent_get_input(bufev);
	size_t size = evbuffer_get_length(input);
	if (size > 0) {
		std::unique_ptr<char[]> buf(new char[size + 1]);
		size_t read = bufferevent_read(bufev, buf.get(), size);
		buf.get()[read] = '\0';
		pugi::xml_document doc;
		if (doc.load_buffer_inplace(buf.get(), read)) {
			auto probeMatchs = doc.select_nodes("//*[local-name()='ProbeMatch']");
			for (pugi::xpath_node probeMatch : probeMatchs) {
				auto node(probeMatch.node());
				m_serviceAddr = node.select_single_node("//*[local-name()='XAddrs']").node().first_child().value();
				if (!m_serviceAddr.empty()) {
					m_serviceAddr = findValidUrl(m_serviceAddr);
					ptzCapabilitiesRequest();
				} else {
					onError(_T("Onvif返回地址无效"));
					wle(_T("onvif device return a invalid response: %s"), xtstring(buf.get()).c_str());
				}
			}
		}
	}
}
std::string COnvifControl::findValidUrl(const xstring<char>& addr) {
	int pos(addr.Find(" "));
	do {
		if (pos < 0) pos = static_cast<int>(addr.length());
		std::string url = addr.Left(pos);
		return url;
	} while (pos > 0);
}

//////////////////////////////////////////////////////////////////////////
//Capability
void COnvifControl::ptzCapabilitiesRequest() {
	m_onvifStage = GetPtzCapabilities;
	m_timer = nullptr;
	std::string message = COnvifPattern::getCapabilities(m_username.c_str(), m_password.c_str(), "PTZ");
	auto request = httpRequest(m_serviceAddr.c_str(), EVHTTP_REQ_POST, message.length());
	if (request == nullptr ||
		evbuffer_add(request->output_buffer, message.c_str(), message.length()) != 0) {
		onError(_T("访问Onvif设备失败"));
		wle(_T("send getCapabilities('ptz') failed"));
	}
}
void COnvifControl::ptzCapabilitiesResponse(pugi::xml_document& doc) {
	auto media = doc.select_single_node("//*[local-name()='PTZ']").node();
	if (media == nullptr) return onError(_T("不支持PTZ操作")), wle(_T("onvif return an invalid response"));
	m_ptzUrl = media.select_single_node("//*[local-name()='XAddr']").node().first_child().value();
	if (m_ptzUrl.empty()) return onError(_T("不支持PTZ操作")), wle(_T("onvif return an invalid response"));
	mediaCapabilitiesRequest();
}

//////////////////////////////////////////////////////////////////////////
//Media
void COnvifControl::mediaCapabilitiesRequest() {
	m_onvifStage = GetMediaCapabilities;
	std::string message = COnvifPattern::getCapabilities(m_username.c_str(), m_password.c_str(), "Media");
	auto request = httpRequest(m_serviceAddr.c_str(), EVHTTP_REQ_POST, message.length());
	if (request == nullptr ||
		evbuffer_add(request->output_buffer, message.c_str(), message.length()) != 0) {
		onError(_T("访问Onvif设备失败"));
		return wle(_T("send getCapabilities('media') failed."));
	}
}
void COnvifControl::mediaCapabilitiesResponse(pugi::xml_document& doc) {
	auto media = doc.select_single_node("//*[local-name()='Media']").node();
	if (media == nullptr) return onError(_T("Onvif无效响应")), wle(_T("onvif return an invalid response"));
	std::string mediaUrl = media.select_single_node("//*[local-name()='XAddr']").node().first_child().value();
	if (mediaUrl.empty()) return onError(_T("Onvif无效响应")), wle(_T("onvif return an invalid response"));
	profilesRequest(mediaUrl.c_str());
}

//////////////////////////////////////////////////////////////////////////
//Profile
void COnvifControl::profilesRequest(const char* mediaUrl) {
	m_onvifStage = GetProfiles;
	std::string message = COnvifPattern::getProfiles(m_username.c_str(), m_password.c_str());
	auto request = httpRequest(mediaUrl, EVHTTP_REQ_POST, message.length());
	if (request == nullptr ||
		evbuffer_add(request->output_buffer, message.c_str(), message.length()) != 0) {
		onError(_T("访问Onvif设备失败"));
		return wle(_T("send getProfiles() failed."));
	}
}
void COnvifControl::profilesResponse(pugi::xml_document& doc) {
	auto profiles = doc.select_nodes("//*[local-name()='GetProfilesResponse']/*[local-name()='Profiles']");
	struct Profile {
		int width;
		int height;
		std::string token;
	};
	std::list<Profile> params;
	for (auto profile : profiles) {
		Profile item;
		auto node = profile.node();
		item.token = node.attribute("token").as_string();
		pugi::xml_node resolution = node.select_node("//*[local-name()='VideoEncoderConfiguration']/*[local-name()='Resolution']").node();
		if (resolution == nullptr) continue;
		item.width = atoi(resolution.select_node("//*[local-name()='Width']").node().first_child().value());
		item.height = atoi(resolution.select_node("//*[local-name()='Height']").node().first_child().value());
		params.push_back(item);
	}
	if (params.size() < 1) return onError(_T("读取媒体配置失败")), wle(_T("onvif return an invalid response"));
	m_profileToken = params.begin()->token;
	if (m_profileToken.empty()) return onError(_T("Onvif请求失败")), wle(_T("onvif return an invalid response"));
	if (m_command->mode) effectRequest();
	else ptzRequest();
}

//////////////////////////////////////////////////////////////////////////
//PTZ
void COnvifControl::ptzRequest() {
	m_onvifStage = Ptz;
	std::string message;
	auto action = m_command->action;
	auto speed = m_command->speed > 0 ? m_command->speed : 1;
	if (action == 0) {
		message = COnvifPattern::getStop(m_username.c_str(), m_password.c_str(), m_profileToken.c_str());
	} else if (action & (ISourceProxy::ZoomIn | ISourceProxy::ZoomOut)) {
		int x = (action & ISourceProxy::ZoomIn) ? speed : -speed;
		message = COnvifPattern::getZoom(m_username.c_str(), m_password.c_str(), m_profileToken.c_str(), x);
	} else if (action & (ISourceProxy::PanLeft | ISourceProxy::PanRight | ISourceProxy::TiltUp | ISourceProxy::TiltDown)) {
		int x = 0;
		int y = 0;
		if ((action & ISourceProxy::PanLeft)) x = -speed;
		else if ((action & ISourceProxy::PanRight)) x = speed;
		if ((action & ISourceProxy::TiltDown)) y = -speed;
		else if ((action & ISourceProxy::TiltUp)) y = speed;
		message = COnvifPattern::getPanTilt(m_username.c_str(), m_password.c_str(), m_profileToken.c_str(), x, y);
	} else {
		return onError(_T("不支持的云台操作"));
	}
	auto request = httpRequest(m_ptzUrl.c_str(), EVHTTP_REQ_POST, message.length());
	if (request == nullptr ||
		evbuffer_add(request->output_buffer, message.c_str(), message.length()) != 0) {
		onError(_T("访问Onvif设备失败"));
		return wle(_T("send getProfiles() failed."));
	}
}
void COnvifControl::ptzResponse(pugi::xml_document& xml) {
	if (m_command == nullptr) return;
	m_source->OnControlComplete(reset(), true);
}

//////////////////////////////////////////////////////////////////////////
//Video Effect
void COnvifControl::effectRequest() {
	m_onvifStage = Effect;
	onError(_T("暂不支持"));
}
void COnvifControl::effectResponse(pugi::xml_document& xml) {
	if (m_command == nullptr) return;
	m_source->OnControlComplete(reset(), true);
}

//////////////////////////////////////////////////////////////////////////
//Error
void COnvifControl::onError(LPCTSTR reason) {
	cpe(_T("%s"), reason);
	if (m_command == nullptr) return;
	m_source->OnControlComplete(reset(), false);
}
unsigned int COnvifControl::reset() {
	auto token(m_command == nullptr ? 0 : m_command->token);
	m_onvifStage = None;
	m_bev = nullptr;
	m_dns = nullptr;
	m_timer = nullptr;
	m_connection = nullptr;
	m_command = nullptr;
	return token;
}

//////////////////////////////////////////////////////////////////////////
//HTTP
evhttp_request* COnvifControl::httpRequest(const char* url, evhttp_cmd_type method, size_t contentLen) {
	CSmartHdr<evhttp_uri*, void> uri(evhttp_uri_parse_with_flags(url, EVHTTP_URI_NONCONFORMANT), evhttp_uri_free);
	if (uri == nullptr) return wlet(nullptr, _T("[%s] Parse url [%s] failed."), m_moniker.c_str(), xtstring(url).c_str());
	evhttp_connection* connection = nullptr;
	evhttp_request* request = nullptr;
	auto finalize(std::destruct_invoke([&request, &connection]() {
		if (request != nullptr) evhttp_request_free(request);
		if (connection != nullptr) evhttp_connection_free(connection);
	}));
	request = evhttp_request_new(http_callback, this);
	if (uri == nullptr) return wlet(nullptr, _T("[%s] Create http request failed."), m_moniker.c_str());
	const char* host = evhttp_uri_get_host(uri);
	int port = evhttp_uri_get_port(uri);
	const char* path = evhttp_uri_get_path(uri);
	const char* query = evhttp_uri_get_query(uri);
	if (port == -1) port = 80;
	connection = evhttp_connection_base_new(m_evbase, nullptr, host, port);
	if (connection == nullptr) return wlet(nullptr, _T("[%s] Connect to [%s:%d] failed for url."), m_moniker.c_str(), host, port, xtstring(url).c_str());
	std::string query_path = std::string(path != nullptr ? path : "");
	if (query_path.empty()) query_path = "/";
	if (query != nullptr) {
		query_path += "?";
		query_path += query;
	}
	int result = evhttp_make_request(connection, request, method, query_path.c_str());
	if (result != 0) return wlet(nullptr, _T("[%s] Request http[%s] failed."), m_moniker.c_str(), xtstring(url).c_str());
	xstring<char> host_port(xstring<char>::format("%s:%d", host, port));
	evhttp_add_header(request->output_headers, "Host", host_port);
	evhttp_add_header(request->output_headers, "Content-Type", "application/soap+xml; charset=utf-8");
	if (contentLen > 0) evhttp_add_header(request->output_headers, "Content-Length", xstring<char>::format("%d", contentLen).c_str());
	finalize.cancel();
	m_connection = connection;
	return request;
}
void COnvifControl::http_callback(evhttp_request* req, void* context) {
	COnvifControl* thiz(reinterpret_cast<COnvifControl*>(context));
	thiz->http_callback(req);
}
void COnvifControl::http_callback(evhttp_request* req) {
	if (req == nullptr) return onError(_T("连接Onvif设备失败"));
	if (req == nullptr || req->response_code != HTTP_OK) return onError(xtstring(req->response_code_line));
	size_t len = evbuffer_get_length(req->input_buffer);
	std::unique_ptr<char[]> buf(new char[len + 1]);
	size_t read = evbuffer_remove(req->input_buffer, buf.get(), len);
	buf.get()[len] = '\0';
	pugi::xml_document doc;
	if (!doc.load_buffer_inplace(buf.get(), read)) return onError(_T("Onvif无效响应"));
	switch (m_onvifStage) {
	case COnvifControl::GetPtzCapabilities: return ptzCapabilitiesResponse(doc);
	case COnvifControl::GetMediaCapabilities: return mediaCapabilitiesResponse(doc);
	case COnvifControl::GetProfiles: return profilesResponse(doc);
	case COnvifControl::Ptz: return ptzResponse(doc);
	case COnvifControl::Effect: return effectResponse(doc);
	default: break;
	}
}