#include <event2/event.h>
#include <event2/thread.h>
#include <event2/event_struct.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/http.h>
#include <event2/listener.h>
#include "zlib.h"
#include "HxhtServer.h"
#include "HxhtFactory.h"
#include "HxhtReader.h"
#include "Writelog.h"
#include "ConsoleUI.h"
#include "Markup.h"
#include "xaid.h"
#include "file.h"

CHxhtServer::CHxhtServer(const xtstring& host, int port, const xtstring& usr, const xtstring& pwd)
	: m_host(host)
	, m_port(port)
	, m_user(usr)
	, m_pwd(pwd)
	, m_name(xtstring::format(_T("%s:%d"), host.c_str(), port).c_str())

	, m_base(nullptr)
	, m_stage(Inited)
	, m_connection(evhttp_connection_free)

	, m_cameras() {
}
CHxhtServer::~CHxhtServer() {
}

bool CHxhtServer::StartEnum(event_base* base) {
	xtstring url;
	xtstring localIp(_T("192.168.2.10"));
	url.Format(_T("http://%s:%d/web_xml_interface/user_logon.xml?logonName=%s&password=%s&IP=%s"),
		m_host.c_str(), m_port, m_user.c_str(), m_pwd.c_str(), localIp.c_str());
	m_base = base;
	m_stage = Login;
	if (!httpGet(url.toString().c_str())) return false;
	return true;
}
void CHxhtServer::EnumCamera(CMarkup& xml, bool onlineOnly) const {
	if (m_stage < EnumOver) return;
	for (auto it : m_cameras) {
		if (onlineOnly && it.second->offline) continue;
		xml.AddElem(_T("signal"));
		xml.AddAttrib(_T("name"), it.second->name);
		xml.AddAttrib(_T("moniker"), m_name + _T("|") + it.first);
		xml.AddAttrib(_T("device"), _T("sky"));
		if (it.second->offline) xml.AddAttrib(_T("offline"), _T("true"));
	}
}
bool CHxhtServer::HasCamera(LPCTSTR moniker) {
	xtstring naming(moniker);
	xtstring server;
	int pos(naming.Find('|'));
	if (pos < 0) pos = naming.Find('~');
	if (pos > 0) {
		server = naming.Left(pos);
		naming = naming.Mid(pos + 1);
	}
	if (server.CompareNoCase(m_name) != 0) return false;
	auto found = m_cameras.find(naming);
	return found != m_cameras.end();
}
ISourceProxy* CHxhtServer::CreateProxy(ISourceProxyCallback* callback, LPCTSTR moniker) {
	xtstring naming(moniker);
	xtstring server;
	int pos(naming.Find('|'));
	if (pos < 0) pos = naming.Find('~'); if (pos > 0) {
		server = naming.Left(pos);
		naming = naming.Mid(pos + 1);
	}
	if (server.CompareNoCase(m_name) != 0) return nullptr;
	auto found = m_cameras.find(naming);
	if (found == m_cameras.end()) {
		return nullptr;
	}
	return new CHxhtReader(this, callback, m_host, m_port, m_user, m_pwd, found->first, found->second->name);
}
void CHxhtServer::SetOffline(const xtstring& naming, bool offline) {
	auto found = m_cameras.find(naming);
	if (found != m_cameras.end()) {
		found->second->offline = offline;
	}
}

bool CHxhtServer::httpGet(const char* url) {
	m_connection = nullptr;
	CSmartHdr<evhttp_uri*, void> uri(evhttp_uri_parse_with_flags(url, EVHTTP_URI_NONCONFORMANT), evhttp_uri_free);
	if (uri == nullptr) return wlet(false, _T("Parse url [%s] failed."), xtstring(url).c_str());
	evhttp_connection* connection = nullptr;
	evhttp_request* request = nullptr;
	auto finalize(std::destruct_invoke([request, connection]() {
		if (request != nullptr) {
			evhttp_request_free(request);
		}
		if (connection != nullptr) {
			evhttp_connection_free(connection);
		}
	}));
	request = evhttp_request_new(http_callback, this);
	if (uri == nullptr) return wlet(false, _T("Create http request failed."));
	const char* host = evhttp_uri_get_host(uri);
	int port = evhttp_uri_get_port(uri);
	const char* path = evhttp_uri_get_path(uri);
	const char* query = evhttp_uri_get_query(uri);
	if (port == -1) port = 80;
	connection = evhttp_connection_base_new(m_base, nullptr, host, port);
	if (connection == nullptr) return wlet(false, _T("Connect to [%s:%d] failed for url."), host, port, xtstring(url).c_str());
	std::string query_path = std::string(path != nullptr ? path : "");
	if (!query_path.empty()) {
		if (query != nullptr) {
			query_path += "?";
			query_path += query;
		}
	} else {
		query_path = "/";
	}
	int result = evhttp_make_request(connection, request, EVHTTP_REQ_GET, query_path.c_str());
	if (result != 0) return wlet(false, _T("Request http[%s] failed."), xtstring(url).c_str());
	xstring<char> host_port(xstring<char>::format("%s:%d", host, port));
	evhttp_add_header(request->output_headers, "Host", host_port);
	finalize.cancel();
	m_connection = connection;
	return true;
}

void CHxhtServer::http_callback(evhttp_request* req, void* context) {
	CHxhtServer* thiz(reinterpret_cast<CHxhtServer*>(context));
	thiz->http_callback(req);
}
void CHxhtServer::http_callback(evhttp_request* req) {
	if (req == nullptr) {
		m_connection = nullptr;
		onEnumFailed(_T("加载摄像头列表失败：无法访问"));
	} else {
		if (req->response_code == HTTP_MOVEPERM || req->response_code == HTTP_MOVETEMP) {
			const char* newLocation = evhttp_find_header(req->input_headers, "Location");
			m_connection = nullptr;
			if (newLocation == nullptr || strlen(newLocation) == 0) {
				onEnumFailed(_T("加载摄像头列表失败：服务器错误"));
			} else if (!httpGet(newLocation)) {
				onEnumFailed(_T("加载摄像头列表失败：连接服务器错误"));
			}
		} else if (req->response_code != HTTP_OK) {
			m_connection = nullptr;
			onEnumFailed(_T("加载摄像头列表失败") + xtstring(req->response_code_line));
		} else {
			const char* contentType = evhttp_find_header(req->input_headers, "Content-Type");
			size_t len = evbuffer_get_length(req->input_buffer);
			std::shared_ptr<char> text(new char[len + 1], [](char* ptr) {delete[] ptr; });
			evbuffer_remove(req->input_buffer, text.get(), len);
			text.get()[len] = '\0';
			xtstring response;
			m_connection = nullptr;
			if (contentType != nullptr && strstr(contentType, "gzip") != nullptr) {
				std::string unziped;
				if (!unzip(text.get(), static_cast<int>(len), unziped)) {
					onEnumFailed(_T("加载摄像头列表失败：解压缩数据错误"));
				} else {
					response = xtstring::convert(unziped, CodePage_GB2312);
				}
			} else {
				response = xtstring::convert(text.get(), len, CodePage_GB2312);
			}
			if (!response.IsEmpty()) {
				CMarkup xml;
				if (!xml.SetDoc(response.c_str())) {
					onEnumFailed(_T("加载摄像头列表失败：服务器返回的XML无效"));
				} else {
					switch (m_stage) {
					case CHxhtServer::Login:
						onLogin(xml);
						break;
					case CHxhtServer::EnumDevice:
						onDevice(xml);
						break;
					default:
						break;
					}
				}
			}
		}
	}
}
bool CHxhtServer::unzip(char* data, int size, std::string& result) {
	const static int windowBits = 15;
	const static int ENABLE_ZLIB_GZIP = 32;
	const static int CHUNK = 4096;
	z_stream strm = { 0 };
	auto finalize(std::destruct_invoke([&strm]() {
		inflateEnd(&strm);
	}));
	unsigned char out[CHUNK];
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.next_in = reinterpret_cast<unsigned char*>(data);
	strm.avail_in = size;
	int status(inflateInit2(&strm, windowBits | ENABLE_ZLIB_GZIP));
	if (status < 0) return wlet(false, _T("unzip gzip data failed."));
	result.clear();
	do {
		unsigned have;
		strm.avail_out = CHUNK;
		strm.next_out = out;
		status = inflate(&strm, Z_NO_FLUSH);
		if (status < 0) return wlet(false, _T("unzip gzip data failed."));
		have = CHUNK - strm.avail_out;
		result.append(reinterpret_cast<char*>(out), have);
	} while (strm.avail_out == 0);
	return true;
}

void CHxhtServer::onLogin(CMarkup& xml) {
	xtstring sessionId;
	if (xml.FindElem(_T("logon_success"))) {
		sessionId = xml.GetAttrib(_T("sessionId"));
	}
	if (!sessionId.IsEmpty()) {
		cpm(_T("[%s] 登录服务器成功"), m_name.c_str());
		CConsoleUI::Singleton().Print(CConsoleUI::Message, false, _T("\t\tSessionID:%s"), sessionId.c_str());
		xtstring url;
		url.Format(_T("http://%s:%d/web_xml_interface/device_hiberarchy_info.xml?sessionId=%s"), 
			m_host.c_str(), m_port, sessionId.c_str());
		m_stage = EnumDevice;
		if (!httpGet(url.toString().c_str())) {
			onEnumFailed(_T("加载摄像头列表失败：访问服务器失败"));
		}
	} else {
		onEnumFailed(_T("加载摄像头列表失败：登录服务器失败"));
	}
}
void CHxhtServer::onDevice(CMarkup& xml) {
	if (xml.FindElem(_T("organ"))) {
		xml.IntoElem();
		if (xml.FindElem()) {
			enumOrgan(xml);
		}
	}
	CMarkup cameras;
	cameras.SetDoc(_T("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"));
	cameras.AddElem(_T("signals"));
	cameras.IntoElem();
	for (auto it : m_cameras) {
		cameras.AddElem(_T("signal"));
		cameras.AddAttrib(_T("name"), it.second->name);
		cameras.AddAttrib(_T("naming"), it.first);
		cameras.AddAttrib(_T("device"), _T("sky"));
	}
	xtstring file;
	file.Format(_T("%s/%s_%d_%s.hxht.clist"), file::parentDirectory(file::exeuteFilePath()).c_str(), m_host.c_str(), m_port, m_user.c_str());
	cameras.Save(file);
	m_stage = EnumOver;
	cpj(_T("[%s] 加载设备列表完成：共%d个摄像头"), m_name.c_str(), m_cameras.size());
}
void CHxhtServer::enumOrgan(CMarkup& xml) {
	do {
		xtstring tag = xml.GetTagName();
		xtstring name = xml.GetAttrib(_T("name"));
		if (tag.Compare(_T("organ")) == 0) {
			xml.IntoElem();
			if (xml.FindElem()) {
				enumOrgan(xml);
			}
			xml.OutOfElem();
		} else if (tag.Compare(_T("video_input_channel")) == 0) {
			xtstring naming = xml.GetAttrib(_T("naming"));
			std::shared_ptr<CameraVic> sp(new CameraVic(name));
			m_cameras.insert(std::make_pair(naming, sp));
		}
	} while (xml.FindElem());
}
void CHxhtServer::onEnumFailed(const xtstring& message) {
	xtstring file;
	bool result(false);
	file.Format(_T("%s/%s_%d_%s.hxht.clist"), file::parentDirectory(file::exeuteFilePath()).c_str(), m_host.c_str(), m_port, m_user.c_str());
	if (_taccess(file, 0) == 0) {
		CMarkup xml;
		if (xml.Load(file) && xml.FindElem(_T("signals"))) {
			xml.IntoElem();
			while (xml.FindElem(_T("signal"))) {
				xtstring name(xml.GetAttrib(_T("name")));
				xtstring naming(xml.GetAttrib(_T("naming")));
				std::shared_ptr<CameraVic> sp(new CameraVic(name));
				m_cameras.insert(std::make_pair(naming, sp));
			}
			cpm(_T("[%s] 从缓存加载设备列表完成：共%d个摄像头"), m_name.c_str(), m_cameras.size());
			result = true;
			m_stage = EnumOver;
		}
	} 
	if (!result) {
		cpe(_T("[%s] %s"), m_name.c_str(), message.c_str());
		CHxhtFactory::Singleton().RemoveServer(this);
	}
}