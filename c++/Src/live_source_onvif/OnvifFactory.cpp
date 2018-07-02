#include <regex>
#include <event2/event.h>
#include <event2/thread.h>
#include <event2/event_struct.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/http.h>
#include <event2/listener.h>
#include "net.h"
#include "OnvifFactory.h"
#include "OnvifSource.h"
#include "Profile.h"
#include "ConsoleUI.h"
#include "Writelog.h"
#include "pugixml/pugixml.hpp"
#include "OnvifPattern.h"
#include "Markup.h"

COnvifFactory::COnvifFactory()
	: m_lpszFactoryName(_T("Onvif Input Source"))
	, m_lpCallback(nullptr)
	, m_messageId()
	, m_fd(-1)
	, m_timer(nullptr) {
}
COnvifFactory::~COnvifFactory() {
	if (m_timer != nullptr) {
		event_free(m_timer);
		m_timer = nullptr;
	}
	if (m_fd != -1) {
		evutil_closesocket(m_fd);
		m_fd = -1;
	}
}

COnvifFactory& COnvifFactory::Singleton() {
	static COnvifFactory instance;
	return instance;
}

LPCTSTR COnvifFactory::FactoryName() const {
	return m_lpszFactoryName;
}

static int avio_decode_interrupt_cb(void* flag) {
	int* iflag = (int*)flag;
	if (*iflag == 0) return 1;
	else return 0;
}
int read_packet(void *opaque, uint8_t *buf, int buf_size) {
	return 0;
}
int write_packet(void *opaque, uint8_t *buf, int buf_size) {
	return 0;
}
int64_t seek(void *opaque, int64_t offset, int whence) {
	return offset;
}

bool COnvifFactory::Initialize(ISourceFactoryCallback* callback) {
	if (!discovery(callback)) return false;
	m_lpCallback = callback;
	return true;
}
ISourceFactory::SupportState COnvifFactory::DidSupport(LPCTSTR device, LPCTSTR moniker, LPCTSTR param) {
	if (device != NULL && _tcslen(device) > 0 && _tcscmp(device, _T("onvif")) == 0) {
		return ISourceFactory::supported;
	}
	return unsupport;
}
ISourceProxy* COnvifFactory::CreateLiveProxy(ISourceProxyCallback* callback, LPCTSTR device, LPCTSTR moniker, LPCTSTR param) {
	if (device != NULL && _tcslen(device) > 0 && _tcscmp(device, _T("onvif")) != 0) {
		return nullptr;
	}
	xtstring smoniker(moniker);
	int pos(smoniker.Find(_T("~")));
	int width(0);
	if (pos > 0) {
		width = _ttoi(smoniker.Mid(pos + 1));
		smoniker = smoniker.Left(pos);
	}

	std::string username;
	std::string password;
	xtstring userpass;
	if (param != nullptr) userpass = param;
	if (userpass.Find(_T("@")) <= 0) {
		CProfile profile(CProfile::Default());
		userpass = (LPCTSTR)profile(_T("Onvif"), smoniker);
	}
	pos = userpass.Find(_T("@"));
	if (pos > 0) {
		password = userpass.Left(pos).toString();
		username = userpass.Mid(pos + 1).toString();
		cpm(_T("[%s] 找到内置用户： %s"), smoniker.c_str(), userpass.Mid(pos + 1).c_str());
	}
	CSmartPtr<COnvifSource> spSource(new COnvifSource(smoniker, username, password, width));
	if (!spSource->Open(callback))return wlet(nullptr, _T("Open Onvif[%s] failed."));
	return spSource.Detach();
}
ISourceProxy* COnvifFactory::CreatePastProxy(ISourceProxyCallback* callback, LPCTSTR device, LPCTSTR moniker, LPCTSTR param, uint64_t beginTime, uint64_t endTime /*= 0*/) {
	return nullptr;
}
bool COnvifFactory::GetSourceList(LPTSTR* _xml, void(**free)(LPTSTR), bool onlineOnly, LPCTSTR device /*= NULL*/) {
	if (!_xml)return false;
	if (device != NULL && _tcslen(device) > 0 && _tcsicmp(device, _T("ugc")) != 0)return false;
	CMarkup xml;
	xml.AddElem(_T("onvif"));
	xml.AddAttrib(_T("name"), _T("Onvif设备"));
	xml.IntoElem();
	for (auto device : m_devices) {
		xml.AddElem(_T("signal"));
		xml.AddAttrib(_T("name"), device);
		xml.AddAttrib(_T("moniker"), device);
		xml.AddAttrib(_T("device"), _T("onvif"));
	}
	xtstring text(xml.GetDoc());
	const TCHAR* lpsz(text.c_str());
	const size_t len(text.length());
	TCHAR* result = new TCHAR[len + 1];
	_tcscpy(result, lpsz);
	*_xml = result;
	*free = [](LPTSTR ptr) {
		delete[] reinterpret_cast<TCHAR*>(ptr);
	};
	return true;
}
bool COnvifFactory::GetStatusInfo(LPSTR* json, void(**free)(LPSTR)) {
	return false;
}
void COnvifFactory::Uninitialize() {

}
void COnvifFactory::Destroy() {
}

bool COnvifFactory::discovery(ISourceFactoryCallback* callback) {
	evutil_socket_t fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd == -1) return wlet(false, _T("Create socket failed."));
	int flag = 1;
	if (evutil_make_listen_socket_reuseable(fd) < 0) {
		evutil_closesocket(fd);
		return wlet(false, _T("Setup socket failed."));
	}
	int count(0);
	event_base** allBase(callback->GetAllBase(count));
	if (allBase == nullptr || count < 1) {
		evutil_closesocket(fd);
		return wlet(false, _T("No event base."));
	}
	event_base* evbase(allBase[0]);
	event* ev(event_new(evbase, fd, EV_READ | EV_PERSIST, &COnvifFactory::read_callback, this));
	int result(event_add(ev, nullptr));
	if (result != 0) {
		evutil_closesocket(fd);
		return wlet(false, _T("Add event to loop failed: %d."), result);
	}
	m_fd = fd;
	m_timer = evtimer_new(evbase, &COnvifFactory::timer_callback, this);
	m_messageId = COnvifPattern::messageID();
	broadcast();
	return true;
}

void COnvifFactory::read_callback(evutil_socket_t fd, short event, void* arg) {
	COnvifFactory* thiz(reinterpret_cast<COnvifFactory*>(arg));
	thiz->read_callback(fd, event);
}
void COnvifFactory::read_callback(evutil_socket_t fd, short event) {
	struct sockaddr_in client_addr;
	socklen_t size(sizeof(struct sockaddr));
	int len(recvfrom(fd, m_udpCache, sizeof(m_udpCache), 0, reinterpret_cast<struct sockaddr *>(&client_addr), reinterpret_cast<socklen_t*>(&size)));
	if (len > 0) {
		m_udpCache[len] = '\0';
		pugi::xml_document doc;
		if (doc.load_buffer_inplace(m_udpCache, sizeof(m_udpCache))) {
			auto probeMatchs = doc.select_nodes("//*[local-name()='ProbeMatch']");
			for (pugi::xpath_node probeMatch : probeMatchs) {
				auto node(probeMatch.node());
				auto types(node.select_single_node("//*[local-name()='Types']").node().first_child());
				auto xAddrs(node.select_single_node("//*[local-name()='XAddrs']").node().first_child());
				std::regex pattern("(((25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])\\.){3}(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9]))");
				std::match_results<const char*> result;
				bool valid = std::regex_search(xAddrs.value(), result, pattern);
				if (valid) {
					xtstring ip(result[0].first, result[0].length());
					auto found(std::find_if(m_devices.begin(), m_devices.end(), [ip](xtstring lrs) {
						return lrs.compare(ip) == 0;
					}));
					if (found == m_devices.end()) {
						m_devices.push_back(ip);
						cpm(_T("Find Onvif device: %s"), ip.c_str());
					}
				}
			}
		}
	}
}

void COnvifFactory::timer_callback(evutil_socket_t fd, short event, void* arg) {
	COnvifFactory* thiz(reinterpret_cast<COnvifFactory*>(arg));
	thiz->timer_callback(fd, event);
}
void COnvifFactory::timer_callback(evutil_socket_t fd, short event) {
	broadcast();
}

void COnvifFactory::broadcast() {
	std::string message(COnvifPattern::probe("dn:NetworkVideoTransmitter", "", m_messageId.c_str()));
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(3702);
	inet_aton("239.255.255.250", &addr.sin_addr);
	int result(sendto(m_fd, message.c_str(), static_cast<int>(message.length()), 0, (struct sockaddr*)&addr, sizeof(addr)));
	if (result == -1) return wle(_T("send broadcast message failed."));
	else wli(_T("send onvif discovery broadcast ok."));
	static struct timeval tv = { 30, 0 };
	evtimer_add(m_timer, &tv);
}