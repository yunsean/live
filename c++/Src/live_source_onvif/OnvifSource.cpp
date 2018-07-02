#include <event2/event.h>
#include <event2/thread.h>
#include <event2/event_struct.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/http.h>
#include <event2/listener.h>
#include "net.h"
#include "OnvifSource.h"
#include "OnvifPattern.h"
#include "Writelog.h"
#include "ConsoleUI.h"
#include "pugixml/pugixml.hpp"
#include "OnvifPattern.h"
#include "xmember.h"
#include "xsystem.h"
#include "xaid.h"

#define MAX_UINT32	(uint32_t)(~0)


//////////////////////////////////////////////////////////////////////////
//COnvifSource
static void evdns_base_free(evdns_base* base) {
	evdns_base_free(base, 0);
}

COnvifSource::COnvifSource(const xtstring& ip, const std::string& usr, const std::string& pwd, const int width)
	: m_moniker(ip)
	, m_preferWidth(width)
	, m_username(usr)
	, m_password(pwd)
	, m_lpCallback(nullptr)

	, m_evbase(nullptr)
	, m_dns(evdns_base_free)
	, m_bev(bufferevent_free)
	, m_timer(event_free)
	, m_connection(evhttp_connection_free)
	, m_rtspClient(nullptr)
	, m_messageId()

	, m_onvifStage(Ready)
	, m_onvifControl(this, ip, usr, pwd) {
	cpm(_T("[%s] 创建Onvif Source"), m_moniker.c_str());
}
COnvifSource::~COnvifSource(void) {
	cpm(_T("[%s] 释放File Source"), m_moniker.c_str());
	if (m_lpCallback)m_lpCallback->OnDiscarded();
	m_lpCallback = NULL;
}

bool COnvifSource::Open(ISourceProxyCallback* callback) {
	m_lpCallback = callback;
	return true;
}

void COnvifSource::timer_callback(evutil_socket_t fd, short event, void* arg) {
	COnvifSource* thiz(reinterpret_cast<COnvifSource*>(arg));
	if (thiz != nullptr) thiz->timer_callback(fd, event);
}
void COnvifSource::timer_callback(evutil_socket_t fd, short event) {
	onError(_T("OnVif发现设备超时"));
}

void COnvifSource::on_read(struct bufferevent* event, void* context) {
	COnvifSource* thiz(reinterpret_cast<COnvifSource*>(context));
	thiz->on_read(event);
}
void COnvifSource::on_read(struct bufferevent* event) {
	if (m_onvifStage == Probe) return probeResponse(event);
}

void COnvifSource::on_error(struct bufferevent* bev, short what, void* context) {
	COnvifSource* thiz(reinterpret_cast<COnvifSource*>(context));
	thiz->on_error(bev, what);
}
void COnvifSource::on_error(struct bufferevent* bev, short what) {
	
}

void COnvifSource::http_callback(evhttp_request* req, void* context) {
	COnvifSource* thiz(reinterpret_cast<COnvifSource*>(context));
	thiz->http_callback(req);
}
void COnvifSource::http_callback(evhttp_request* req) {
	if (req == nullptr) return onError(_T("访问Onvif失败"));
	if (req->response_code != HTTP_OK) return onError(xtstring(req->response_code_line));
	size_t len = evbuffer_get_length(req->input_buffer);
	std::unique_ptr<char[]> buf(new char[len + 1]);
	size_t read = evbuffer_remove(req->input_buffer, buf.get(), len);
	buf.get()[len] = '\0';
	pugi::xml_document doc;
	if (!doc.load_buffer_inplace(buf.get(), read)) return onError(_T("Onvif无效响应"));
	switch (m_onvifStage) {
	case COnvifSource::GetCapabilities: return capabilitiesResponse(doc);
	case COnvifSource::GetProfiles: return profilesResponse(doc);
	case GetStreamUri: return streamUriResponse(doc);
	default: break;
	}
}

void COnvifSource::probeResponse(struct bufferevent* bufev) {
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
					if (!capabilitiesRequest()) return onError(_T("Onvif请求失败"));
					m_onvifStage = GetCapabilities;
				} else {
					onError(_T("Onvif返回地址无效"));
					wle(_T("onvif device return a invalid response: %s"), xtstring(buf.get()).c_str());
				}
			}
		}
	}
}
std::string COnvifSource::findValidUrl(const xstring<char>& addr) {
	int pos(addr.Find(" "));
	do {
		if (pos < 0) pos = static_cast<int>(addr.length());
		std::string url = addr.Left(pos);
		return url;
	} while (pos > 0);
}
void COnvifSource::capabilitiesResponse(pugi::xml_document& doc) {
	auto media = doc.select_single_node("//*[local-name()='Media']").node();
	if (media == nullptr) return onError(_T("Onvif无效响应"));
	m_mediaUrl = media.select_single_node("//*[local-name()='XAddr']").node().first_child().value();
	if (m_mediaUrl.empty()) return onError(_T("Onvif无效响应"));
	if (!profilesRequest()) return onError(_T("Onvif请求失败"));
	m_onvifStage = GetProfiles;
}
void COnvifSource::profilesResponse(pugi::xml_document& doc) {
	auto profiles = doc.select_nodes("//*[local-name()='GetProfilesResponse']/*[local-name()='Profiles']");
	int delta(999999999);
	int validWidth(0);
	int validHeight(0);
	delta;
	validHeight;
	std::string validToken;
	cpw(_T("[%s] Profile列表："), m_moniker.c_str());
	for (auto profile : profiles) {
		auto node = profile.node();
		std::string token = node.attribute("token").as_string();
		pugi::xml_node resolution = node.select_node("*[local-name()='VideoEncoderConfiguration']/*[local-name()='Resolution']").node();
		if (resolution == nullptr) continue;
		int width = atoi(resolution.select_node("*[local-name()='Width']").node().first_child().value());
		int height = atoi(resolution.select_node("*[local-name()='Height']").node().first_child().value());
		if (validToken.empty() || (m_preferWidth > 0 && (abs(width - m_preferWidth) < abs(validWidth - m_preferWidth)))) {
			validWidth = width;
			validHeight = height;
			validToken = token;
		}
		pugi::xml_node videoEncoding = node.select_node("*[local-name()='VideoEncoderConfiguration']/*[local-name()='Encoding']").node();
		pugi::xml_node audioEncoding = node.select_node("*[local-name()='AudioEncoderConfiguration']/*[local-name()='Encoding']").node();
		std::string videoCodec = videoEncoding == nullptr ? "" : videoEncoding.first_child().value();
		std::string audioCodec = audioEncoding == nullptr ? "" : audioEncoding.first_child().value();
		CConsoleUI::Singleton().Print(CConsoleUI::Message, false, _T("\t\t%s: %s-%dx%d | %s"), xtstring(token.c_str()).c_str(), 
			xtstring(videoCodec).c_str(), width, height, xtstring(audioCodec).c_str());
	}
	if (validToken.empty()) return onError(_T("读取媒体配置失败"));
	cpm(_T("[%s] User Profile %s"), m_moniker.c_str(), xtstring(validToken).c_str());
	if (!streamUriRequest(validToken.c_str())) return onError(_T("Onvif请求失败"));
	m_onvifStage = GetStreamUri;
}
void COnvifSource::streamUriResponse(pugi::xml_document& doc) {
	auto uriNode = doc.select_node("//*[local-name()='GetStreamUriResponse']/*[local-name()='MediaUri']/*[local-name()='Uri']").node();
	if (uriNode == nullptr) return onError(_T("获取Onvif播放地址失败"));
	std::string uri = uriNode.first_child().value();
	m_rtspClient = new CRtspClient(m_evbase, this, m_username.c_str(), m_password.c_str());
	if (!m_rtspClient->feed(uri.c_str())) return onError(_T("打开媒体流失败"));
}

bool COnvifSource::dnsRequest() {
	m_dns = evdns_base_new(m_evbase, 1);
	if (!m_dns.isValid()) return wlet(false, _T("create dns failed."));
	struct evutil_addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_flags = EVUTIL_AI_CANONNAME;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	evdns_getaddrinfo(m_dns, m_moniker.toString().c_str(), NULL, &hints, getaddrinfo_callback, this);
	return true;
}
void COnvifSource::getaddrinfo_callback(int result, struct evutil_addrinfo *res, void *arg) {
	COnvifSource* thiz(reinterpret_cast<COnvifSource*>(arg));
	thiz->getaddrinfo_callback(result, res);
}
void COnvifSource::getaddrinfo_callback(int result, struct evutil_addrinfo *addr) {
	if (result != 0) return onError(_T("域名解析失败"));
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

bool COnvifSource::probeRequest(const char* ip) {
	evutil_socket_t fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd == -1) return wlet(false, _T("Create socket failed."));
	if (evutil_make_listen_socket_reuseable(fd) < 0) {
		evutil_closesocket(fd);
		return wlet(false, _T("Setup socket failed."));
	}
	m_bev = bufferevent_socket_new(m_evbase, fd, BEV_OPT_CLOSE_ON_FREE);
	if (m_bev == NULL) {
		evutil_closesocket(fd);
		return wlet(false, _T("Create buffer event failed."));
	}
	bufferevent_setcb(m_bev, on_read, NULL, on_error, this);
	bufferevent_enable(m_bev, EV_READ | EV_WRITE);
	m_timer = evtimer_new(m_evbase, &COnvifSource::timer_callback, this);
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
		return wlet(false, _T("send broadcast message failed."));
	}
	static struct timeval tv = { 5, 0 };
	evtimer_add(m_timer, &tv);
	m_onvifStage = Probe;
	return true;
}
bool COnvifSource::capabilitiesRequest() {
	m_timer = nullptr;
	std::string message = COnvifPattern::getCapabilities(m_username.c_str(), m_password.c_str(), "Media");
	auto request = httpRequest(m_serviceAddr.c_str(), EVHTTP_REQ_POST, message.length());
	if (request == nullptr) return false;
	if (evbuffer_add(request->output_buffer, message.c_str(), message.length()) != 0) return wlet(false, _T("[%s] Send http body failed."));
	return true;
}
bool COnvifSource::profilesRequest() {
	std::string message = COnvifPattern::getProfiles(m_username.c_str(), m_password.c_str());
	auto request = httpRequest(m_mediaUrl.c_str(), EVHTTP_REQ_POST, message.length());
	if (request == nullptr) return false;
	if (evbuffer_add(request->output_buffer, message.c_str(), message.length()) != 0) return wlet(false, _T("[%s] Send http body failed."));
	return true;
}
bool COnvifSource::streamUriRequest(const char* token) {
	std::string message = COnvifPattern::getStreamUri(m_username.c_str(), m_password.c_str(), token);
	auto request = httpRequest(m_mediaUrl.c_str(), EVHTTP_REQ_POST, message.length());
	if (request == nullptr) return false;
	if (evbuffer_add(request->output_buffer, message.c_str(), message.length()) != 0) return wlet(false, _T("[%s] Send http body failed."));
	return true;
}

//////////////////////////////////////////////////////////////////////////
bool COnvifSource::StartFetch(event_base* base) {
	m_evbase = base;
	for (auto header : m_vConfigs) {
		m_lpCallback->OnHeaderData(header.GetArr(), header.GetSize());
	}
	return dnsRequest();
}
void COnvifSource::WantKeyFrame() {
}
ISourceProxy::ControlResult COnvifSource::PTZControl(const unsigned int token, const unsigned int action, const int speed) {
	std::lock_guard<std::recursive_mutex> lock(m_lockCommands);
	m_ptzCommands.push(std::shared_ptr<ControlCommand>(new ControlCommand(token, action, speed)));
	DoControl();
	return ISourceProxy::Pending;
}
ISourceProxy::ControlResult COnvifSource::VideoEffect(const unsigned int token, const int bright, const int contrast, const int saturation, const int hue) {
	std::lock_guard<std::recursive_mutex> lock(m_lockCommands);
	m_ptzCommands.push(std::shared_ptr<ControlCommand>(new ControlCommand(token, bright, contrast, saturation, hue)));
	DoControl();
	return ISourceProxy::Pending;
}
bool COnvifSource::Discard() {
	m_onvifControl.Discard();
	if (m_onvifControl.IsWorking()) return false;
	delete this;
	return true;
}

void COnvifSource::onError(LPCTSTR reason) {
	if (m_onvifStage == Corrupted) return;
	m_onvifStage = Corrupted;
	if (m_lpCallback != nullptr) {
		m_lpCallback->OnError(reason);
	}
}

void COnvifSource::OnControlComplete(const unsigned int token, bool result) {
	m_lpCallback->OnControlResult(token, result, nullptr);
	DoControl();
}
void COnvifSource::DoControl() {
	std::lock_guard<std::recursive_mutex> lock(m_lockCommands);
	if (m_ptzCommands.size() < 1) return;
	if (m_onvifControl.IsWorking()) return;
	cmdptr cmd(m_ptzCommands.front());
	m_ptzCommands.pop();
	m_onvifControl.DoCommand(cmd);
}

evhttp_request* COnvifSource::httpRequest(const char* url, evhttp_cmd_type method, size_t contentLen) {
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

//////////////////////////////////////////////////////////////////////////
void COnvifSource::onVideoConfig(uint32_t fourCC, const uint8_t* data, int size) {
	CSmartNal<uint8_t> header(4 + 1 + 4 + 2 + size);
	*(reinterpret_cast<uint32_t*>(header.GetArr())) = MKFCC('1', 'L', 'A', 'N');
	*(reinterpret_cast<uint8_t*>(header.GetArr() + 4)) = 'V';
	*(reinterpret_cast<uint32_t*>(header.GetArr() + 5)) = fourCC;
	*(reinterpret_cast<uint16_t*>(header.GetArr() + 9)) = static_cast<uint16_t>(size);
	if (size > 0) memcpy(reinterpret_cast<uint8_t*>(header.GetArr()) + 11, data, size);
	header.SetSize(4 + 1 + 4 + 2 + size);
	m_vConfigs.push_back(header);
	if (m_lpCallback != nullptr) m_lpCallback->OnHeaderData(header.GetArr(), header.GetSize());
}
void COnvifSource::onAudioConfig(uint32_t fourCC, const uint8_t* data, int size) {
	CSmartNal<uint8_t> header(4 + 1 + 4 + 2 + size);
	*(reinterpret_cast<uint32_t*>(header.GetArr())) = MKFCC('1', 'L', 'A', 'N');
	*(reinterpret_cast<uint8_t*>(header.GetArr() + 4)) = 'A';
	*(reinterpret_cast<uint32_t*>(header.GetArr() + 5)) = fourCC;
	*(reinterpret_cast<uint16_t*>(header.GetArr() + 9)) = static_cast<uint16_t>(size);
	if (size > 0) memcpy(reinterpret_cast<uint8_t*>(header.GetArr()) + 11, data, size);
	header.SetSize(4 + 1 + 4 + 2 + size);
	m_vConfigs.push_back(header);
	if (m_lpCallback != nullptr) m_lpCallback->OnHeaderData(header.GetArr(), header.GetSize());
}
void COnvifSource::onVideoFrame(uint32_t timestamp, const uint8_t* data, int size) {
	*(reinterpret_cast<uint32_t*>(const_cast<uint8_t*>(data) - 4)) = timestamp;
	if (m_lpCallback != nullptr) m_lpCallback->OnVideoData(data - 4, size + 4);
}
void COnvifSource::onAudioFrame(uint32_t timestamp, const uint8_t* data, int size) {
	*(reinterpret_cast<uint32_t*>(const_cast<uint8_t*>(data) - 4)) = timestamp;
	if (m_lpCallback != nullptr) m_lpCallback->OnAudioData(data - 4, size + 4);
}
