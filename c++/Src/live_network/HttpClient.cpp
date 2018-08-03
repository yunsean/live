#include <event2/bufferevent.h>
#include <event2/thread.h>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/http.h>
#include <event2/http_struct.h>
#include "net.h"
#include "HttpClient.h"
#include "Writelog.h"
#include "ConsoleUI.h"

const std::map<xtstring, xtstring> CHttpClient::s_mapNull;
std::atomic<int> CHttpClient::m_instanceCount(0);
CHttpClient::CHttpClient()
	: m_instanceIndex(0)
	, m_fd(-1)
	, m_clientIp()
	, m_clientPort(0)
	, m_bev(nullptr)
	, m_clientName()

	, m_cache(new(std::nothrow) char[20 * 1024], [](char* ptr) {delete[] ptr; })
	, m_cacheOffset(0)
	, m_hasHeader(false)

	, m_operation()
	, m_version()
	, m_url()
	, m_path()
	, m_params()
	, m_payloads()
	, m_action() {
	cpm(_T("创建客户端对象: %s"), name());
}
CHttpClient::~CHttpClient() {
	cpm(_T("析构客户端对象: %s"), name());
	close();
}

void CHttpClient::set(evutil_socket_t fd, struct sockaddr* addr) {
	close();

	m_instanceIndex = m_instanceCount++;
	m_fd = fd;
	char ip[1024];
	m_clientIp = inet_ntop(AF_INET, &(((sockaddr_in*)addr)->sin_addr), ip, sizeof(ip));
	m_clientPort = ntohs(((sockaddr_in*)addr)->sin_port);
	m_bev = nullptr;
	m_clientName.format(_T("#%d@%s:%d"), m_instanceIndex, m_clientIp.c_str(), m_clientPort);

	m_cacheOffset = 0;
	m_hasHeader = false;

	m_operation.clear();
	m_version.clear();
	m_url.clear();
	m_path.clear();
	m_params.clear();
	m_payloads.clear();
	m_action.clear();
}
bool CHttpClient::run(event_base* base) {
	if (m_bev != nullptr) return false;
	m_bev = bufferevent_socket_new(base, m_fd, 0);
	if (m_bev == NULL) {
		return wlet(false, _T("Create buffer event failed."));
	}
	bufferevent_setcb(m_bev, &CHttpClient::on_read, NULL, &CHttpClient::on_error, this);
	bufferevent_enable(m_bev, EV_READ | EV_WRITE);
	return true;
}
event_base* CHttpClient::base() {
	if (m_bev == nullptr) return nullptr;
	return bufferevent_get_base(m_bev);
}
void CHttpClient::close() {
	if (m_bev != nullptr) {
		bufferevent_free(m_bev);
		m_bev = nullptr;
	}
	if (m_fd != -1) {
		evutil_closesocket(m_fd);
		m_fd = -1;
	}
}
void CHttpClient::release() {
	delete this;
}

LPCTSTR CHttpClient::param(LPCTSTR key) const {
	auto found(m_params.find(key));
	if (found == m_params.end()) {
		return nullptr;
	} else {
		return found->second.c_str();
	}
}
LPCTSTR CHttpClient::payload(LPCTSTR key) const {
	auto found(m_payloads.find(key));
	if (found == m_payloads.end()) {
		return nullptr;
	} else {
		return found->second.c_str();
	}
}
evbuffer* CHttpClient::input() { 
	return bufferevent_get_input(m_bev); 
}
evbuffer* CHttpClient::output() {
	return bufferevent_get_output(m_bev); 
}

void CHttpClient::on_read(struct bufferevent* event, void* context) {
	CHttpClient* thiz = reinterpret_cast<CHttpClient*>(context);
	thiz->on_read(event);
}
void CHttpClient::on_error(struct bufferevent* bev, short what, void* context) {
	CHttpClient* thiz = reinterpret_cast<CHttpClient*>(context);
	thiz->on_error(bev, what);
}

void CHttpClient::on_read(struct bufferevent* event) {
	if (!m_hasHeader) {
		if (!tryParseRequet(event)) {
			return close();
		}
		if (m_hasHeader) {
			handle();
		}
	}
}
void CHttpClient::on_error(struct bufferevent* bev, short what) {
	if (what & (BEV_EVENT_ERROR | BEV_EVENT_EOF | BEV_EVENT_TIMEOUT)) {
		bufferevent_disable(bev, EV_READ | EV_WRITE);
		release();
	}
}

char CHttpClient::asciiHexToChar(char c) {
	return  c >= '0' && c <= '9' ? c - '0'
		: c >= 'A' && c <= 'F' ? c - 'A' + 10
		: c - 'a' + 10;
}
void CHttpClient::unescapeURI(char* str) {
	char* p(str);
	char* q(str);
	while (*p) {
		if (*p == '%') {
			p++;
			if (*p)*q = asciiHexToChar(*p++) * 16;
			if (*p)*q = *q + asciiHexToChar(*p);
			p++;
			q++;
		}
		else {
			*q++ = *p++;
		}
	}
	*q++ = 0;
}
bool CHttpClient::tryParseRequet(struct bufferevent* event) {
	while (20 * 1024 - 4 - m_cacheOffset > 1024) {
		int read = static_cast<int>(bufferevent_read(event, m_cache.get() + m_cacheOffset, 1024));
		if (read <= 0) break;
		m_cacheOffset += read;
	}
	char* lpString(m_cache.get());
	int nCLCRPos(-1);
	for (int i = 0; i < m_cacheOffset - 3; i++, lpString++) {
		if (*(unsigned int*)lpString == 0x0a0d0a0d) {	//'\r\n\r\n'
			nCLCRPos = static_cast<int>(lpString - m_cache.get());
			break;
		}
	}
	if (nCLCRPos == -1) {
		if (20 * 1024 - 4 - m_cacheOffset <= 1024) {
			return wlet(false, _T("The http header is more than 20 * 1024 bytes."));
		}
		else {
			return true;
		}
	}
	nCLCRPos += 4;

	char byTemp(m_cache.get()[nCLCRPos]);
	m_cache.get()[nCLCRPos] = '\0';
	if (!parseHeader()) {
		return wlet(false, _T("Parse http header failed."));
	}
	m_cache.get()[nCLCRPos] = byTemp;
	m_cacheOffset -= nCLCRPos;
	if (m_cacheOffset > 0) {
		memcpy(m_cache.get(), m_cache.get() + nCLCRPos, m_cacheOffset);
	}
	m_hasHeader = true;
	return true;
}
bool CHttpClient::parseHeader() {
	m_operation.Empty();
	m_version.Empty();
	m_path.Empty();
	m_params.clear();
	m_payloads.clear();
	m_action.Empty();
	unescapeURI(m_cache.get());
	xtstring header((char*)m_cache.get(), static_cast<int>(strlen(m_cache.get())));
	header.TrimLeft(_T("\r\n"));

	xtstring payload;
	int pos(header.Find(_T("\r\n")));
	if (pos > 0) {
		payload = header.Mid(pos + 2);
		header = header.Left(pos);
	}
	header.TrimLeft();

	xtstring operation(header.SpanExcluding(_T(" ")));
	if (operation.IsEmpty()) {
		return wlet(false, _T("Operation is empty. header: %s"), header.c_str());
	}
	m_operation = operation;
	header = header.Mid(operation.GetLength() + 1);
	header.TrimLeft();

	pos = header.ReverseFind(' ');
	xtstring path(header.Left(pos + 1));
	path.Trim();
	if (path.IsEmpty()) {
		return wlet(false, _T("Path is empty. header: %s"), header.c_str());
	}
	header = header.Mid(path.GetLength() + 1);
	header.Trim();
	m_version = header;
	m_url = path;

	xtstring query;
	pos = path.Find(_T("?"));
	if (pos > 0) {
		query = path.Mid(pos + 1);
		path = path.Left(pos);
	}
	m_path = path;
	pos = path.ReverseFind('/');
	if (pos < 0) {
		m_action = path;
	}
	else {
		m_action = path.Mid(pos + 1);
	}
	m_action.Trim();
	while (!query.IsEmpty()) {
		if (query.Right(1) != _T("&")) {
			query += _T("&");
		}
		int equal;
		int _and;
		int quato;
		equal = query.Find(_T("="));
		quato = query.Find(_T("\""));
		if (quato < equal && quato >= 0) {
			quato = query.Find(_T("\""), quato + 1);
			equal = query.Find(_T("="), quato + 1);
			quato = query.Find(_T("\""), equal + 1);
		}
		_and = query.Find(_T("&"), equal);
		if (quato < _and && quato >= 0) {
			quato = query.Find(_T("\""), quato + 1);
			_and = query.Find(_T("&"), quato);
		}
		if (equal > 0 && _and > 0) {
			xtstring key(query.Left(equal).MakeLower());
			xtstring value(query.Mid(equal + 1, _and - (equal + 1)));
			key.Trim();
			value.Trim(_T(" \""));
			m_params.insert(std::make_pair(key, value));
		}
		else {
			break;
		}
		query = query.Mid(_and + 1);
	}

	payload.TrimRight();
	payload.TrimRight(_T("\r\n"));
	while (!payload.IsEmpty()) {
		xtstring line(payload.SpanExcluding(_T("\r\n")));
		int quato(line.Find(_T(":")));
		xtstring key;
		xtstring value;
		if (quato < 1) {
			key = line;
		}
		else {
			key = line.Left(quato);
			value = line.Mid(quato + 1);
		}
		key.Trim();
		value.Trim(_T(" \""));
		m_payloads.insert(std::make_pair(key, value));
		payload.Delete(0, line.GetLength());
		payload.TrimLeft(_T("\r\n"));
	}

#if 0
	cpm(_T("operation=%s"), m_operation.c_str());
	cpm(_T("url=%s"), m_url.c_str());
	cpm(_T("streamName=%s"), m_action.c_str());
	cpm(_T("payloads:"));
	for (auto it = m_payloads.begin(); it != m_payloads.end(); it++) {
		cpm(_T("%s=%s"), it->first.c_str(), it->second.c_str());
	}
	cpm(_T("params:"));
	for (auto it = m_params.begin(); it != m_params.end(); it++) {
		cpm(_T("%s=%s"), it->first.c_str(), it->second.c_str());
	}
#endif
	return true;
}

bool CHttpClient::sendHttpRedirectResponse(const xtstring& location, const int code /*= 302*/, const xtstring& status /*= _T("Found")*/) {
	xstring<char> strHeader;
	strHeader.Format("%s %d %s\r\n", m_version.toString().c_str(), code, status.toString().c_str());
	strHeader += "Server: simplehttp\r\n";
	strHeader += "Connection: close\r\n";
	strHeader += "Cache-Control: no-cache\r\n";
	strHeader += "Location: " + location.toString() + "\r\n";
	strHeader += "\r\n";
	const char* data(strHeader.c_str());
	const size_t size(strHeader.length());
	int result(bufferevent_write(m_bev, data, size));
	if (result != 0) {
		return wlet(false, _T("Send redirect response to [%s:%d] failed, reason = %d."), m_clientIp.c_str(), m_clientPort, evutil_socket_geterror(m_fd));
	}
	bufferevent_flush(m_bev, EV_WRITE, BEV_FLUSH);
	return true;
}
void CHttpClient::sendHTTPFailedResponse(const xtstring& reason /* = L"" */, const int statusCode /*= 400*/) {
	xstring<char> strHeader;
	strHeader.Format("%s %d %s\r\n", m_version.toString().c_str(), statusCode, reason.toString().c_str());
	strHeader += "Server: simplehttp\r\n";
	strHeader += "Connection: close\r\n";
	strHeader += "Content-Length: 0\r\n";
	strHeader += "\r\n";
	const char* data(strHeader.c_str());
	const size_t size(strHeader.length());
	int result(bufferevent_write(m_bev, data, size));
	if (result != 0) {
		return wle(_T("Send failed response to [%s:%d] failed, reason = %d."), m_clientIp.c_str(), m_clientPort, evutil_socket_geterror(m_fd));
	}
	bufferevent_flush(m_bev, EV_WRITE, BEV_FLUSH);
}
bool CHttpClient::sendHttpSucceedResponse(const xtstring& contentType /* = _T("text/html") */, const int64_t contentLen /* = -1 */, const std::map<xtstring, xtstring>& mapPayload /* = s_mapNull */) {
	std::string strHeader(getHttpSucceedResponse(contentType, contentLen, mapPayload));
	const char* data(strHeader.c_str());
	const size_t size(strHeader.length());
	int result(bufferevent_write(m_bev, data, size));
	if (result != 0) {
		return wlet(false, _T("Send succeed response to [%s:%d] failed, reason = %d."), m_clientIp.c_str(), m_clientPort, evutil_socket_geterror(m_fd));
	}
	bufferevent_flush(m_bev, EV_WRITE, BEV_FLUSH);
	return true;
}
bool CHttpClient::sendHttpContent(const void* data, const size_t size) {
	int result(bufferevent_write(m_bev, data, size)); 
	if (result != 0) {
		return false;
	}
	return true;
}
std::string CHttpClient::getHttpSucceedResponse(const xtstring& contentType /* = _T("text/html") */, const int64_t contentLen /* = -1 */, const std::map<xtstring, xtstring>& mapPayload /* = s_mapNull */) {
	xstring<char> strHeader;
	strHeader.Format("%s 200 OK\r\n", m_version.toString().c_str());
	strHeader += "Server: simplehttp\r\n";
	strHeader += "Connection: close\r\n";
	strHeader += "Cache-Control: no-cache\r\n";
	std::for_each(mapPayload.begin(), mapPayload.end(),
		[&strHeader](const std::pair<xtstring, xtstring> payload) {
		strHeader.AppendFormat("%s: %s\r\n", payload.first.toString().c_str(), payload.second.toString().c_str());
	});
	if (!contentType.IsEmpty())
		strHeader += "Content-Type: " + contentType + "\r\n";
	if (contentLen >= 0) {
		strHeader.AppendFormat("Content-Length: %lld\r\n", contentLen);
	}
	strHeader += "\r\n";
	return strHeader.c_str();
}

bufferevent* CHttpClient::bev(bool detach /* = true */) {
	if (detach) {
		bufferevent* bev = m_bev;
		m_bev = nullptr;
		return bev;
	} else {
		return m_bev;
	}
}
evutil_socket_t CHttpClient::fd(bool detach /* = true */) {
	if (detach) {
		if (m_bev != nullptr) {
			bufferevent_disable(m_bev, EV_WRITE | EV_READ);
			bufferevent_free(m_bev);
			m_bev = nullptr;
		}
		evutil_socket_t fd = m_fd;
		m_fd = -1;
		return fd;
	} else {
		return m_fd;
	}
}