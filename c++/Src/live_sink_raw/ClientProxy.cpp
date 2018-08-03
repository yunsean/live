#include "RawFactory.h"
#include "ClientProxy.h"
#include "ConsoleUI.h"
#include "Writelog.h"
#include "net.h"
#include "xaid.h"

#define ACTION_STREAMLIST	_T("streamlist")
#define ACTION_PLAYSTREAM	_T("playstream")
#define ACTION_PLAYSOURCE	_T("playsource")

std::mutex CClientProxy::m_mutex;
std::queue<CClientProxy*> CClientProxy::m_clients;
CClientProxy::CClientProxy() {
}
CClientProxy::~CClientProxy() {
}

CClientProxy* CClientProxy::newInstance(evutil_socket_t fd, struct sockaddr* addr) {
	if (m_clients.size() > 0) {
		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_clients.size() > 0) {
			auto client(m_clients.front());
			m_clients.pop();
			client->set(fd, addr);
			return client;
		}
	}
	auto client = new CClientProxy;
	client->set(fd, addr);
	return client;
}

evhttp_request* httpRequest(event_base* base, const char* url, evhttp_cmd_type method, const char* contentType, size_t contentLen, void(*callback)(struct evhttp_request*, void*), void* context /* = NULL */) {
	evhttp_uri* uri = evhttp_uri_parse_with_flags(url, EVHTTP_URI_NONCONFORMANT);
	if (uri == nullptr) {
		return wlet(nullptr, _T("Parse url [%s] failed."), xtstring(url).c_str());
	}
	evhttp_request* request = evhttp_request_new(callback, context);
	auto finalize(std::destruct_invoke([request]() {
		evhttp_request_free(request);
	}));
	if (uri == nullptr) {
		return wlet(nullptr, _T("Create http request failed."));
	}
	const char* host = evhttp_uri_get_host(uri);
	int port = evhttp_uri_get_port(uri);
	const char* path = evhttp_uri_get_path(uri);
	const char* query = evhttp_uri_get_query(uri);
	if (port == -1) port = 80;
	evhttp_connection* conn = evhttp_connection_base_new(base, NULL, host, port);
	if (conn == nullptr) {
		return wlet(nullptr, _T("Connect to [%s:%d] failed for url."), host, port, xtstring(url).c_str());
	}
	std::string query_path = std::string(path != nullptr ? path : "");
	if (!query_path.empty()) {
		if (query != nullptr) {
			query_path += "?";
			query_path += query;
		}
	} else {
		query_path = "/";
	}
	int result = evhttp_make_request(conn, request, method, query_path.c_str());
	if (result != 0) {
		return wlet(nullptr, _T("Request http[%s] failed."), xtstring(url).c_str());
	}
	xstring<char> host_port(xstring<char>::format("%s:%d", host, port));
	evhttp_add_header(request->output_headers, "Host", host_port);
	if (contentType != NULL) {
		evhttp_add_header(request->output_headers, "ContentType", contentType);
	}
	finalize.cancel();
	return request;
}
evhttp_request* httpGet(event_base* base, const char* url, void(*callback)(struct evhttp_request*, void*), void* context /* = NULL */) {
	return httpRequest(base, url, EVHTTP_REQ_GET, nullptr, 0, callback, context);
}

void CClientProxy::handle() {
	xtstring _path(path());
	xtstring device;
	xtstring moniker;
	xtstring query;
	_path.Trim(_T("/"));
	if (_path.CompareNoCase(ACTION_PLAYSTREAM) == 0 ||
		_path.CompareNoCase(ACTION_PLAYSOURCE) == 0) {
		moniker = param(_T("moniker"));
		device = param(_T("device"));
		query = param(_T("param"));
		moniker.Trim();
		device.Trim();
		query.Trim();
		if (moniker.IsEmpty()) {
			xtstring ip(param(_T("ip")));
			int port(_ttoi(param(_T("port"))));
			xtstring stream(param(_T("stream")));
			if (ip.IsEmpty() || port <= 0 || port > 65535) {
				sendHTTPFailedResponse(_T("Invalid Params."));
				return cpw(_T("[%s] 无效的播放流请求！"), name());
			}
			stream.Trim();
			if (stream.IsEmpty())moniker.Format(_T("%s:%d"), ip.c_str(), port);
			else moniker.Format(_T("%s:%d~%s"), ip.c_str(), port, stream.c_str());
		}
		if (query.IsEmpty()) {
			xtstring user(param(_T("user")));
			xtstring pwd(param(_T("pwd")));
			if (!user.IsEmpty() || !pwd.IsEmpty())query.Format(L"%s@%s", pwd.c_str(), user.c_str());
		}
	} else if (_path.CompareNoCase(ACTION_STREAMLIST) == 0) {
#ifdef _WIN32
		xtstring url(composeUrl(_T("http://127.0.0.1:89/streamlist"), params()));
#else
		xtstring url(composeUrl(_T("http://127.0.0.1:8089/streamlist"), params()));
#endif
		httpGet(base(), url.toString().c_str(), 
			[](evhttp_request* req, void* context) {
				size_t len = evbuffer_get_length(req->input_buffer);
				std::shared_ptr<char> xml(new char[len + 1], [](char* ptr) {delete[] ptr; });
				evbuffer_remove(req->input_buffer, xml.get(), len);
				CClientProxy* thiz = reinterpret_cast<CClientProxy*>(context);
				thiz->sendHttpSucceedResponse(_T("text/xml"), len);
				thiz->sendHttpContent(xml.get(), len);
			}, this);
		return;
	} else {
		auto param(params());
		int posDevice(_path.Find(_T("/")));
		int posMoniker(_path.ReverseFind(_T('/')));
		query = composeQuery(param);
		if (posDevice > 0) device = _path.Left(posDevice);
		if (posMoniker > 0) moniker = _path.Mid(posMoniker + 1);
		else moniker = _path;
	}
	if (moniker.IsEmpty()) {
		sendHTTPFailedResponse(_T("Invalid request: need moniker."));
		return cpw(_T("无效的播放流请求！"));
	}
	cpw(_T("[%s] 播放流请求"), moniker.c_str());
	bool result(CNalFactory::singleton().bindWriter(this, device, moniker, query));
	if (!result) {
		sendHTTPFailedResponse(L"Connect to device failed.");
		return cpe(_T("[%s] 获取设备失败！"), moniker.c_str());
	}
	release();
}
void CClientProxy::release() {
	std::lock_guard<std::mutex> lock(m_mutex);
	close();
	m_clients.push(this);
}