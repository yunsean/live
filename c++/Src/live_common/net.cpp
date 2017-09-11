#include "net.h"


#ifdef _WIN32
int setnonblock(SOCKET fd) {
	unsigned long val = 1;
	return ioctlsocket(fd, FIONBIO, &val);
}
#else 
int setnonblock(int fd) {
	int f_old;
	f_old = fcntl(fd, F_GETFL, 0);
	if (f_old < 0) return -1;
	f_old |= O_NONBLOCK;
	return fcntl(fd, F_SETFL, f_old);
}
#endif

#ifdef _WIN32
int setkeepalive(SOCKET fd, int time, int interval) {
	int keepalive = 1;
	int result = ::setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (char*)&keepalive, sizeof(keepalive));
	if (result == SOCKET_ERROR) {
		return result;
	}
	tcp_keepalive alive_in = { 0 };
	tcp_keepalive alive_out = { 0 };
	alive_in.keepalivetime = time * 1000;
	alive_in.keepaliveinterval = interval * 1000;
	alive_in.onoff = true;
	unsigned long ulBytesReturn = 0;
	result = WSAIoctl(fd, SIO_KEEPALIVE_VALS, &alive_in, sizeof(alive_in), &alive_out, sizeof(alive_out), &ulBytesReturn, NULL, NULL);
	return result;
}
#else 
int setkeepalive(SOCKET fd, int time, int interval) {
	int keepalive = 1;
	int keepidle = time;
	int keepinterval = interval;
	int keepcount = 3;
	int result = 0;
	result = setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void*)&keepalive, sizeof(keepalive));
	result = result != 0 ? result : setsockopt(fd, SOL_TCP, TCP_KEEPIDLE, (void*)&keepidle, sizeof(keepidle));
	result = result != 0 ? result : setsockopt(fd, SOL_TCP, TCP_KEEPINTVL, (void*)&keepinterval, sizeof(keepinterval));
	result = result != 0 ? result : setsockopt(fd, SOL_TCP, TCP_KEEPCNT, (void*)&keepcount, sizeof(keepcount));
	return result;
}
#endif

bool resolveUrl(const xtstring& url, xtstring& path, std::map<xtstring, xtstring>& queries) {
	xtstring uri(url);
	uri.Trim();
	if (uri.IsEmpty())return false;
	int pos(uri.Find(_T("?")));
	xtstring query;
	queries.clear();
	if (pos > 0) {
		query = uri.Mid(pos + 1);
		path = uri.Left(pos);
	} else {
		path = uri;
		return true;
	}
	while (!query.IsEmpty()) {
		if (query.Right(1) != _T("&"))
			query += _T("&");
		int equal(query.Find(_T("=")));
		int quato(query.Find(_T("\"")));
		int _and;
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
			xtstring value(query.Mid(equal + 1, _and -(equal + 1)));
			key.Trim();
			value.Trim(_T(" \""));
			queries.insert(std::make_pair(key, value));
		} else {
			break;
		}
		query = query.Mid(_and +1);
	}
	return true;
}
xtstring composeQuery(const std::map<xtstring, xtstring>& queries) {
	xtstring url;
	for (std::map<xtstring, xtstring>::const_iterator it = queries.begin(); it != queries.end(); it++) {
		xtstring strTmp;
		if (it->second.Find(_T("&")) >= 0 || it->second.Find(_T("=")) >= 0) {
			strTmp.Format(_T("%s=\"%s\""), it->first.c_str(), it->second.c_str());
		}
		else {
			strTmp.Format(_T("%s=%s"), it->first.c_str(), it->second.c_str());
		}
		url += strTmp;
		url += _T("&");
	}
	url.TrimRight(_T("&"));
	return url;
}
xtstring composeUrl(const xtstring& path, const std::map<xtstring, xtstring>& queries /* = std::map<xtstring, xtstring>() */) {
	xtstring url(path);
	if (url.Right(1) != _T("?") && queries.size() > 0) {
		url += _T("?");
	}
	for (std::map<xtstring, xtstring>::const_iterator it = queries.begin(); it != queries.end(); it++) {
		xtstring strTmp;
		if (it->second.Find(_T("&")) >= 0 || it->second.Find(_T("=")) >= 0) {
			strTmp.Format(_T("%s=\"%s\""), it->first.c_str(), it->second.c_str());
		} else {
			strTmp.Format(_T("%s=%s"), it->first.c_str(), it->second.c_str());
		}
		url += strTmp;
		url += _T("&");
	}
	url.TrimRight(_T("&"));
	return url;
}
int resolveHeader(const char* const lpszHeader, std::map<xtstring, xtstring>& queries) {
	xstring<char> header(lpszHeader);
	int length(header.Find("\r\n\r\n"));
	if (length < 0)return 0;
	length += 4;
	xstring<char> param(header.Left(length - 2));
	int pos(param.Find("\r\n"));
	if (pos < 0)return 0;
	param = param.Mid(pos + 2);
	pos = param.Find("\r\n");
	while (pos > 0) {
		xstring<char> line(param.Left(pos));
		param = param.Mid(pos + 2);
		pos = line.Find(":");
		if (pos > 0) {
			xtstring key(line.Left(pos));
			xtstring value(line.Mid(pos + 1));
			key.Trim();
			value.Trim();
			key.MakeLower();
			queries.insert(std::make_pair(key, value));
		}
		pos = param.Find("\r\n");
	}
	return length;
}
std::string encodeUrl(const xtstring& url) {
	xstring<char> urlA(url);
	xstring<char> out;
	const unsigned char* from(reinterpret_cast<const unsigned char*>(urlA.GetString()));
	const unsigned char* end(from + urlA.GetLength());
	unsigned char* to(reinterpret_cast<unsigned char*>(out.GetBuffer(urlA.GetLength() * 3)));
	const unsigned char hexchars[] = "0123456789ABCDEF";
	while (from < end) {
		unsigned char   c(*from++);
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
	out.ReleaseBuffer();
	return out;
}
xtstring decodeUrl(const std::string& url) {
	const static char flag[128] = {
		-1, -1, -1, -1, -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
		-1, -1, -1, -1, -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
		-1, -1, -1, -1, -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
		0 , 1,  2,  3,  4 ,5 ,6 ,7 ,8 ,9 ,-1,-1,-1,-1,-1,-1,
		-1, 10, 11, 12, 13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,
		-1, -1, -1, -1, -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
		-1, 10, 11, 12, 13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,
		-1, -1, -1, -1, -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
	};
	const char* cp(url.c_str());
	xstring<char> strOutA;
	while (*cp != '\0') {
		if (*cp == '%') {
			char h(*(++cp));
			char l(*(++cp));
			if ((h > 0) && (l > 0) && (flag[(unsigned char)h] >= 0) && (flag[(unsigned char)l] >= 0)) {
				strOutA.AppendFormat("%c", ((flag[(unsigned char)h] << 4) | flag[(unsigned char)l]));
			} else {
				break;
			}
		} else if (*cp == '+') {
			strOutA += ' ';
		} else {
			strOutA += *cp;
		}
		++cp;
	}
	return strOutA;
}
