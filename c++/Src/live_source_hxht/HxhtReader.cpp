#include "HxhtReader.h"
#include "HxhtServer.h"
#include "Markup.h"
#include "Writelog.h"
#include "ConsoleUI.h"
#include "xaid.h"
#include "net.h"

#define SHOW_MESSAGE	1
#define MESSAGE(x)		if(SHOW_MESSAGE) cpm(_T("[%s] %s"), m_name.c_str(), x)

CHxhtReader::CHxhtReader(CHxhtServer* server, ISourceProxyCallback* callback, const xtstring& host, int port, const xtstring& usr, const xtstring& pwd, const xtstring moniker, const xtstring& name)
	: m_server(server)
	, m_host(host)
	, m_port(port)
	, m_user(usr)
	, m_pwd(pwd)
	, m_moniker(moniker)
	, m_name(name)

	, m_lpCallback(callback)
	, m_base(nullptr)
	, m_connection(evhttp_connection_free)

	, m_sessionId()
	, m_privilege()
	, m_naming()
	, m_ticket()
	, m_bev(bufferevent_free)
	, m_workStage(Inited)

	, m_uIndex(1)
	, m_byX11Package()
	, m_responseSkipBytes(0)

	, m_byFirstFrame()
	, m_nVersion(-1) {
	m_byX11Package.EnsureSize(1024);
}
CHxhtReader::~CHxhtReader() {
	if (m_lpCallback)m_lpCallback->OnDiscarded();
	m_lpCallback = NULL;
}

bool CHxhtReader::StartFetch(event_base* base) {
	xtstring url;
	m_localIp = _T("192.168.2.10");
	url.Format(_T("http://%s:%d/web_xml_interface/user_logon.xml?logonName=%s&password=%s&IP=%s"),
		m_host.c_str(), m_port, m_user.c_str(), m_pwd.c_str(), m_localIp.c_str());
	m_base = base;
	m_workStage = Login;
	if (!httpGet(url.toString().c_str())) return false;
	return true;
}
void CHxhtReader::WantKeyFrame() {

}
ISourceProxy::ControlResult CHxhtReader::PTZControl(const unsigned int token, const unsigned int action, const int speed) {
	return ISourceProxy::Failed;
}
ISourceProxy::ControlResult CHxhtReader::VideoEffect(const unsigned int token, const int bright, const int contrast, const int saturation, const int hue) {
	return ISourceProxy::Failed;
}
bool CHxhtReader::Discard() {
	delete this;
	return true;
}

bool CHxhtReader::httpGet(const char* url) {
	m_connection = nullptr;
	CSmartHdr<evhttp_uri*, void> uri(evhttp_uri_parse_with_flags(url, EVHTTP_URI_NONCONFORMANT), evhttp_uri_free);
	if (uri == nullptr) return wlet(false, _T("[%s] Parse url [%s] failed."), m_moniker.c_str(), xtstring(url).c_str());
	evhttp_connection* connection = nullptr;
	evhttp_request* request = nullptr;
	auto finalize(std::destruct_invoke([&request, &connection]() {
		if (request != nullptr) {
			evhttp_request_free(request);
		}
		if (connection != nullptr) {
			evhttp_connection_free(connection);
		}
	}));
	request = evhttp_request_new(http_callback, this);
	if (uri == nullptr) return wlet(false, _T("[%s] Create http request failed."), m_moniker.c_str());
	const char* host = evhttp_uri_get_host(uri);
	int port = evhttp_uri_get_port(uri);
	const char* path = evhttp_uri_get_path(uri);
	const char* query = evhttp_uri_get_query(uri);
	if (port == -1) port = 80;
	connection = evhttp_connection_base_new(m_base, nullptr, host, port);
	if (connection == nullptr) return wlet(false, _T("[%s] Connect to [%s:%d] failed for url."), m_moniker.c_str(), host, port, xtstring(url).c_str());
	std::string query_path = std::string(path != nullptr ? path : "");
	if (!query_path.empty()) {
		if (query != nullptr) {
			query_path += "?";
			query_path += query;
		}
	}
	else {
		query_path = "/";
	}
	int result = evhttp_make_request(connection, request, EVHTTP_REQ_GET, query_path.c_str());
	if (result != 0) return wlet(false, _T("[%s] Request http[%s] failed."), m_moniker.c_str(), xtstring(url).c_str());
	xstring<char> host_port(xstring<char>::format("%s:%d", host, port));
	evhttp_add_header(request->output_headers, "Host", host_port);
	finalize.cancel();
	m_connection = connection;
	return true;
}
void CHxhtReader::http_callback(evhttp_request* req, void* context) {
	CHxhtReader* thiz(reinterpret_cast<CHxhtReader*>(context));
	thiz->http_callback(req);
}
void CHxhtReader::http_callback(evhttp_request* req) {
	if (req == nullptr) {
		m_connection = nullptr;
		onError(_T("登录服务器失败"));
	}
	else {
		if (req->response_code == HTTP_MOVEPERM || req->response_code == HTTP_MOVETEMP) {
			m_connection = nullptr;
			const char* newLocation = evhttp_find_header(req->input_headers, "Location");
			if (newLocation == nullptr || strlen(newLocation) == 0) {
				onError(_T("服务器错误"));
			}
			else if (!httpGet(newLocation)) {
				onError(_T("连接服务器错误"));
			}
		} else if (req->response_code != HTTP_OK) {
			m_connection = nullptr;
			onError(xtstring(req->response_code_line));
		} else {
			size_t len = evbuffer_get_length(req->input_buffer);
			std::shared_ptr<char> text(new char[len + 1], [](char* ptr) {delete[] ptr; });
			evbuffer_remove(req->input_buffer, text.get(), len);
			text.get()[len] = '\0';
			xtstring response = xtstring::convert(text.get(), len, CodePage_GB2312);
			m_connection = nullptr;
			if (!response.IsEmpty()) {
				CMarkup xml;
				bool result(false);
				xtstring accessIp;
				int accessPort(0);
				if (xml.SetDoc(response.c_str())) {
					if (xml.FindElem(_T("logon_success"))) {
						m_sessionId = xml.GetAttrib(_T("sessionId"));
						m_privilege = xml.GetAttrib(_T("priority"));
						m_naming = xml.GetAttrib(_T("naming"));
						if (xml.FindChildElem(_T("Access"))) {
							xml.IntoElem();
							accessIp = xml.GetAttrib(_T("IP"));
							accessPort = _ttoi(xml.GetAttrib(_T("Port")).c_str());
							result = true;
						}
					}
				} 
				if (result && !accessIp.IsEmpty() && !m_sessionId.IsEmpty()) {
					connectAccess(accessIp, accessPort);
				} else {
					wle(_T("[%s] server return an invalid xml: %s"), m_naming.c_str(), response.c_str());
					onError(_T("服务器返回的XML无效"));
				}
			}
		}
	}
}
void CHxhtReader::connectAccess(const xtstring& ip, int port) {
	m_bev = bufferevent_socket_new(m_base, -1, BEV_OPT_CLOSE_ON_FREE);
	m_workStage = HandShark;
	if (!m_bev.isValid() || bufferevent_socket_connect_hostname(m_bev.GetHdr(), nullptr, AF_INET, ip.toString().c_str(), port) < 0) {
		wle(_T("[%s] Connect to access server [%s:%d] failed."), m_moniker.c_str(), ip.c_str(), port);
		onError(_T("连接接入服务器失败"));
	} else {
		wli(_T("[%s] Connect to access server [%s:%d] succeed."), m_moniker.c_str(), ip.c_str(), port);
		evutil_socket_t fd(bufferevent_getfd(m_bev));
		setkeepalive(fd, 5, 5);
		bufferevent_setcb(m_bev.GetHdr(), on_read, nullptr, on_event, this);
		bufferevent_enable(m_bev.GetHdr(), EV_READ | EV_WRITE);
	}
}
void CHxhtReader::handShark() {
	CMarkup xml;
	xml.AddElem(_T("Message"));
	xml.IntoElem();
	xml.AddElem(_T("Ver"));
	xml.SetElemContent(_T("1.0"));
	xml.AddElem(_T("Naming"));
	xml.SetElemContent(m_naming);
	xml.AddElem(_T("IP"));
	xml.SetElemContent(m_localIp);
	xml.OutOfElem();
	xtstring message(xml.GetDoc());
	char arrResource[31] = { 0x00, 0x6C, 0x0B, 0x10 };
	const CByte& data(x11Pack(eSharkHand, ++m_uIndex, m_sessionId.toString(), arrResource, message.toString()));
	MESSAGE(_T("发送握手命令"));
	m_workStage = HandShark;
	if (bufferevent_write(m_bev, data.GetData(), data.GetSize()) < 0) {
		return onError(_T("与服务器握手失败"));
	}
	m_responseSkipBytes = -1;
	m_byX11Package.Clear();
}
void CHxhtReader::openLive() {
	CMarkup xml;
	xml.AddElem(_T("Message"));
	xml.SetAttrib(_T("privilege"), m_privilege);
	xml.SetAttrib(_T("uuid"), m_user);
	xml.IntoElem();
	xml.AddElem(_T("Naming"));
	xml.SetElemContent(m_moniker);
	xml.AddElem(_T("DevName"));
	xml.SetElemContent(m_name);
	xml.AddElem(_T("Version"));
	xml.SetElemContent(_T("2"));
	xml.AddElem(_T("Dispatch"));
	xml.OutOfElem();
	xtstring message(xml.GetDoc());
	char arrResource[31];
	strncpy(arrResource, m_moniker.toString().c_str(), 31);
	const CByte& data(x11Pack(eOpenLive, ++m_uIndex, m_sessionId.toString(), arrResource, message.toString(CodePage_GB2312)));
	MESSAGE(_T("发送请求视频服务器命令"));
	m_workStage = OpenLive;
	if (bufferevent_write(m_bev, data.GetData(), data.GetSize()) < 0) {
		return onError(_T("请求视频服务器失败"));
	}
	m_responseSkipBytes = -1;
	m_byX11Package.Clear();
}
void CHxhtReader::connectVideo(const xtstring& svrIp, int svrPort) {
	m_bev = nullptr;
	m_bev = bufferevent_socket_new(m_base, -1, BEV_OPT_CLOSE_ON_FREE);
	m_workStage = OpenStream;
	if (!m_bev.isValid() || bufferevent_socket_connect_hostname(m_bev.GetHdr(), nullptr, AF_INET, svrIp.toString().c_str(), svrPort) < 0) {
		wle(_T("[%s] Connect to video server [%s:%d] failed."), m_moniker.c_str(), svrIp.c_str(), svrPort);
		onError(_T("连接视频服务器失败"));
	} else {
		wli(_T("[%s] Connect to video server [%s:%d] succeed."), m_moniker.c_str(), svrIp.c_str(), svrPort);
		evutil_socket_t fd(bufferevent_getfd(m_bev));
		setkeepalive(fd, 5, 5);
		bufferevent_setcb(m_bev.GetHdr(), on_read, nullptr, on_event, this);
		bufferevent_enable(m_bev.GetHdr(), EV_READ | EV_WRITE);
	}
}
void CHxhtReader::openStream() {
	CMarkup xml;
	xml.AddElem(_T("Message"));
	xml.IntoElem();
	xml.AddElem(_T("Ticket"));
	xml.SetElemContent(m_ticket);
	xml.OutOfElem();
	xtstring message(xml.GetDoc());
	char arrResource[31];
	strncpy(arrResource, m_moniker.toString().c_str(), 31);
	m_workStage = OpenStream;
	const CByte& data(x11Pack(eGetStream, ++m_uIndex, m_sessionId.toString(), arrResource, message.toString(CodePage_GB2312)));
	MESSAGE(_T("发送播放实时流命令"));
	if (bufferevent_write(m_bev, data.GetData(), data.GetSize()) < 0) {
		return onError(_T("请求播放实时流失败"));
	}
	m_responseSkipBytes = -1;
	m_byX11Package.Clear();
}

void CHxhtReader::on_read(struct bufferevent* bev, void* ctx) {
	CHxhtReader* thiz(reinterpret_cast<CHxhtReader*>(ctx));
	thiz->on_read(bev);
}
void CHxhtReader::on_read(struct bufferevent* bev) {
	do {
		char* data(reinterpret_cast<char*>(m_byX11Package.GetData()));
		int offset(m_byX11Package.GetSize());
		int limit(m_byX11Package.GetLimit() - offset);
		if (limit < 512) {
			m_byX11Package.EnsureSize(m_byX11Package.GetLimit() + 1024, true);
			data = reinterpret_cast<char*>(m_byX11Package.GetData());
			limit = m_byX11Package.GetLimit() - offset;
		}
		int read = static_cast<int>(bufferevent_read(bev, data + offset, limit));
		if (read <= 0) break;
		m_byX11Package.GrowSize(read);
		do {
			if (!x11Unpack(reinterpret_cast<uint8_t*>(m_byX11Package.GetData()), m_byX11Package.GetSize())) {
				break;
			}
		} while (m_workStage != Corrupted);
	} while (m_workStage != Corrupted);
}
void CHxhtReader::on_event(struct bufferevent* bev, short what, void* ctx) {
	CHxhtReader* thiz(reinterpret_cast<CHxhtReader*>(ctx));
	thiz->on_event(bev, what);
}
void CHxhtReader::on_event(struct bufferevent* bev, short what) {
	if (m_workStage == OpenStream) {
		if (what & BEV_EVENT_CONNECTED) {
			MESSAGE(_T("连接视频服务器成功"));
			openStream();
		} else {
			onError(_T("连接视频服务器失败"));
		}
	} else {
		if (what & BEV_EVENT_CONNECTED) {
			MESSAGE(_T("连接接入服务器成功"));
			handShark();
		} else {
			onError(_T("连接接入服务器失败"));
		}
	}
}

const CByte& CHxhtReader::x11Pack(unsigned int action, unsigned int index, const std::string& session, const char* resource, const std::string& message) {
	const static uint8_t header[] = { 0x01, 0x01, 0x00, 0x00 };
	const unsigned int length(static_cast<int>(message.length()) + 1);
	const unsigned char zero(0);
	m_byX11Package.Clear();
	m_byX11Package.AppendData(header, sizeof(header));
	m_byX11Package.AppendData(&length, sizeof(length));
	m_byX11Package.AppendData(&action, sizeof(action));
	m_byX11Package.AppendData(&index, sizeof(index));
	m_byX11Package.AppendData(session.c_str(), static_cast<int>(session.length()));
	m_byX11Package.AppendData(&zero, sizeof(zero));
	m_byX11Package.AppendData(resource, 31);
	m_byX11Package.AppendData(&zero, sizeof(zero));
	m_byX11Package.AppendData(message.c_str(), static_cast<int>(message.length()));
	m_byX11Package.AppendData(&zero, sizeof(zero));
	return m_byX11Package;
}
bool CHxhtReader::x11Unpack(const uint8_t* data, int size) {
	if (m_responseSkipBytes == -1) {
		const static uint8_t header[] = { 0x01, 0x01, 0x00, 0x00 };
		for (int i = 0; i < size - 4; i++) {
			if (memcmp(data + i, header, 4) == 0) {
				m_responseSkipBytes = 0;
				break;
			}
		}
	}
	if (m_responseSkipBytes >= 0 && size >= 20 + m_responseSkipBytes && m_workStage != Corrupted) {
		data += m_responseSkipBytes;
		size -= m_responseSkipBytes;
		int bodySize(*(uint32_t*)(data + 4));
		if (size >= m_responseSkipBytes + 20 + bodySize) {
			if (m_workStage != Fetching) {
				uint32_t result(*(uint32_t*)(data + 8));
				//uint32_t index(*(uint32_t*)(data + 12));
				std::string message;
				if (bodySize > 0) {
					message.append(reinterpret_cast<const char*>(data + 20), bodySize);
				}
				switch (m_workStage) {
				case CHxhtReader::HandShark:
					onHandShark(result);
					break;
				case CHxhtReader::OpenLive:
					onOpenLive(result, message);
					break;
				case CHxhtReader::OpenStream:
					onOpenStream(result, message);
					break;
				default:
					break;
				}
			} else {
				uint32_t type(*(uint32_t*)(data + 8));
				onFetching(type, data + 20, bodySize);
			}
			m_byX11Package.LeftShift(m_responseSkipBytes + 20 + bodySize);
			m_responseSkipBytes = -1;
			return true;
		}
	}
	return false;
}
void CHxhtReader::onHandShark(uint32_t result) {
	if (result != rSharkHand) {
		wle(_T("[%s] shark hand failed, result = 0x%08x"), m_moniker.c_str(), result);
		onError(_T("与接入服务器握手失败"));		
	} else {
		MESSAGE(_T("与接入服务器握手成功"));
		openLive();
	}
}
void CHxhtReader::onOpenLive(uint32_t result, const std::string& message) {
	if (result != rOpenLive) {
		wle(_T("[%s] open live failed, result = 0x%08x"), m_moniker.c_str(), result);
		onError(_T("打开视频服务器失败"));
	} else if (message.empty()) {
		onError(_T("该摄像头不在线"));
	} else {
		CMarkup xml;	
		xtstring body = xtstring::convert(message, CodePage_GB2312);
		if (!xml.SetDoc(body) || !xml.FindElem(_T("Message"))) {
			wle(_T("[%s] read invalid message from access server: %s"), m_moniker.c_str(), body.c_str());
			onError(_T("服务器返回无效消息"));
		} else {
			xtstring ticket(xmlNodeValue(xml, _T("Ticket")));
			xtstring svrIp(xmlNodeValue(xml, _T("SvrIP")));
			int svrPort(_ttoi(xmlNodeValue(xml, _T("VideoPort"))));
			if (svrIp.IsEmpty() || svrPort <= 0 || ticket.IsEmpty()) {
				wle(_T("[%s] read invalid message from access server: %s"), m_moniker.c_str(), body.c_str());
				onError(_T("服务器返回无效消息"));
			} else {
				cpm(_T("[%s] 获取视频服务器成功: %s:%d"), m_name.c_str(), svrIp.c_str(), svrPort);
				m_ticket = ticket;
				connectVideo(svrIp, svrPort);
			}
		}
	}
}
void CHxhtReader::onOpenStream(uint32_t result, const std::string& message) {
	if (result != rGetStream) {
		wle(_T("[%s] get stream failed, result = 0x%08x"), m_moniker.c_str(), result);
		onError(_T("请求实时视频流失败"));
	} else {
		m_workStage = Fetching;
		cpj(_T("[%s] 请求实时视频流成功"), m_name.c_str());
		if (m_server) m_server->SetOffline(m_moniker, false);
	}
}
void CHxhtReader::onFetching(unsigned int type, const uint8_t* data, int size) {
	if (type == Header) {
		m_lpCallback->OnHeaderData(data, size);
	} else if (type == Body) {
		if (m_nVersion == -1 && size < 8) {
			return;
		} else if (m_nVersion == -1 && m_byFirstFrame.GetSize() < 1) {
			m_byFirstFrame.FillData(data, size);
		} else if (m_nVersion == -1) {
			if (memcmp((uint8_t*)m_byFirstFrame.GetData() + 4, data + 4, 4) == 0) {
				m_nVersion = 2;
			} else {
				m_nVersion = 1;
			}
			cpm(_T("[%s] 确认版本号：%d"), m_name.c_str(), m_nVersion);
			if (m_nVersion > 1)m_lpCallback->OnVideoData((uint8_t*)m_byFirstFrame.GetData() + 12, m_byFirstFrame.GetSize() - 12, true);
			else m_lpCallback->OnVideoData((uint8_t*)m_byFirstFrame.GetData(), m_byFirstFrame.GetSize(), true);
			m_byFirstFrame.Clear();
		}
		if (m_nVersion > 1 && size > 12) {
			return m_lpCallback->OnVideoData(data + 12, size - 12, true);
		} else {
			return m_lpCallback->OnVideoData(data, size, true);
		}
	}
}
xtstring CHxhtReader::xmlNodeValue(CMarkup& xml, LPCTSTR node) {
	xtstring value;
	xml.ResetChildPos();
	if (xml.FindChildElem(node)) {
		xml.IntoElem();
		value = xml.GetElemContent();
		xml.OutOfElem();
	}
	return value;
}
void CHxhtReader::onError(LPCTSTR reason) {
	wle(_T("[%s] onError(%s)"), m_moniker.c_str(), reason);
	if (m_workStage != Fetching && m_server != nullptr) {
		m_server->SetOffline(m_moniker, true);
	}
	m_workStage = Corrupted;
}
