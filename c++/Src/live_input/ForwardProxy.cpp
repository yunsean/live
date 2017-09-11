#include <memory>
#include <functional>
#include "event2/http.h"
#include "event2/http_struct.h"
#include "ForwardProxy.h"

CForwardProxy::CForwardProxy(const xtstring& device, const xtstring& moniker, const xtstring& param, std::map<xtstring, xtstring> extend)
	: m_strDevice(device)
	, m_strMoniker(moniker)
	, m_strParam(param)
	, m_strPath()
	, m_strHost()
	, m_vod(false) {
	m_strDevice.Trim();
	m_strMoniker.Trim();
	m_strParam.Trim();
	if (!m_strDevice.IsEmpty()) extend.insert(std::make_pair(_T("device"), m_strDevice));
	if (!m_strMoniker.IsEmpty()) extend.insert(std::make_pair(_T("moniker"), m_strMoniker));
	if (!m_strParam.IsEmpty()) extend.insert(std::make_pair(_T("param"), m_strParam));
	std::string query = buildQuery(extend);
	std::unique_ptr<evhttp_uri, std::function<void(evhttp_uri*)>> uri(evhttp_uri_new(), [](evhttp_uri* uri) { evhttp_uri_free(uri); });
}
CForwardProxy::~CForwardProxy() {
}

bool CForwardProxy::openSuperior(const xtstring& superior) {
	return true;
}

std::string CForwardProxy::buildQuery(const std::map<xtstring, xtstring>& query) {
	xstring<char> result;
	for (auto it : query) {
		result.AppendFormat("%s=%s&", it.first.toString().c_str(), uriEncode(it.second).c_str());
	}
	result.TrimRight("&");
	return result;
}
std::string CForwardProxy::uriEncode(const xtstring& uri) {
	std::string utf8(uri.toString());
	const unsigned char* from = reinterpret_cast<const unsigned char*>(utf8.c_str());
	const unsigned char* end = from + utf8.length();
	std::shared_ptr<char> result(new char[utf8.length() * 3], [](char* ptr) { delete[] ptr; });
	char* to = result.get();
	const unsigned char hexchars[] = "0123456789ABCDEF";
	while (from < end) {
		char c = *from++;
		if (c == ' ') {
			*to++ = '+';
		} else if ((c < '0' && c != '-' && c != '.') || (c < 'A' && c > '9') || (c > 'Z' && c < 'a' && c != '_') || (c > 'z')) {
			to[0] = '%';
			to[1] = hexchars[c >> 4];
			to[2] = hexchars[c & 15];
			to += 3;
		} else {
			*to++ = c;
		}
	}
	*to = 0;
	return std::string(result.get(), to - result.get());
}
