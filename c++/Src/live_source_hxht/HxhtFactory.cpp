#include <event2/event.h>
#include <event2/thread.h>
#include <event2/event_struct.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include "HxhtFactory.h"
#include "Profile.h"
#include "ConsoleUI.h"
#include "Writelog.h"

COnvifFactory::COnvifFactory()
	: m_lpszFactoryName(_T("Hxht Input Source"))
	, m_lpCallback(nullptr)
	, m_lkHxhtServers()
	, m_vHxhtServers() {
}
COnvifFactory::~COnvifFactory() {
	for (auto server : m_vHxhtServers) {
		delete server;
	}
	m_vHxhtServers.clear();
}

COnvifFactory& COnvifFactory::Singleton() {
	static COnvifFactory instance;
	return instance;
}

LPCTSTR COnvifFactory::FactoryName() const {
	return m_lpszFactoryName;
}
bool COnvifFactory::Initialize(ISourceFactoryCallback* callback) {
#ifdef _WIN32
	if (evthread_use_windows_threads() != 0) {
#else 
	if (evthread_use_pthreads() != 0) {
#endif
		return wlet(false, _T("init multiple thread failed."));
	}

	m_lpCallback = callback;
	CProfile profile(CProfile::Default());
	int nCount(profile(_T("Hxht"), _T("ServerCount"), _T("-1")));
	if (nCount < 0) {
		profile(_T("Hxht"), _T("ServerCount")) = _T("0");
		profile(_T("Hxht"), _T("Server0")) = _T("[user:pwd]@[0.0.0.0:0]");
		nCount = 0;
	}
	if (nCount < 1) {
		cpw(_T("没有配置任何天网设备..."));
	} else {
		for (int iServer = 0; iServer < nCount; iServer++) {
			xtstring strTmp;
			strTmp.Format(_T("Server%d"), iServer);
			xtstring strValue((LPCTSTR)profile(_T("Hxht"), strTmp));
			if (strValue.Left(1) != _T("[") || strValue.Right(1) != _T("]"))continue;
			strValue.Trim(_T("[]"));
			int nPos(strValue.Find(_T(":")));
			if (nPos <= 0)continue;
			xtstring strUser(strValue.Left(nPos));
			strValue = strValue.Mid(nPos + 1);
			nPos = strValue.Find(_T("]@["));
			if (nPos <= 0)continue;
			xtstring strPwd(strValue.Left(nPos));
			strValue = strValue.Mid(nPos + 3);
			nPos = strValue.Find(_T(":"));
			if (nPos <= 0)continue;
			xtstring strIP(strValue.Left(nPos));
			strValue = strValue.Mid(nPos + 1);
			int nPort(_ttoi(strValue));
			std::lock_guard<std::mutex> lock(m_lkHxhtServers);
			std::unique_ptr<CHxhtServer> spServer(new CHxhtServer(strIP, nPort, strUser, strPwd));
			if (!spServer->StartEnum(callback->GetPreferBase())) {
				cpe(_T("连接天网服务器[%s:%d]失败！"), strIP.c_str(), nPort);
			} else {
				m_vHxhtServers.push_back(spServer.release());
			}
		}
		cpj(_T("加载天网服务器配置完成"));
	}

	return true;
}
ISourceFactory::SupportState COnvifFactory::DidSupport(LPCTSTR device, LPCTSTR moniker, LPCTSTR param) {
	if (device != NULL && _tcslen(device) > 0 && _tcscmp(device, _T("sky")) != 0) {
		return unsupport;
	}
	for (auto server : m_vHxhtServers) {
		if (server->HasCamera(moniker)) {
			return ISourceFactory::supported;
		}
	}
	return ISourceFactory::unsupport;
}
ISourceProxy* COnvifFactory::CreateLiveProxy(ISourceProxyCallback* callback, LPCTSTR device, LPCTSTR moniker, LPCTSTR param) {
	if (device != NULL && _tcslen(device) > 0 && _tcscmp(device, _T("sky")) != 0) {
		return nullptr;
	}
	for (auto server : m_vHxhtServers) {
		ISourceProxy* proxy(server->CreateProxy(callback, moniker));
		if (proxy != nullptr) return proxy;
	}
	return nullptr;
}
ISourceProxy* COnvifFactory::CreatePastProxy(ISourceProxyCallback* callback, LPCTSTR device, LPCTSTR moniker, LPCTSTR param, uint64_t beginTime, uint64_t endTime /*= 0*/) {
	return nullptr;
}
bool COnvifFactory::GetSourceList(LPTSTR* _xml, void(**free)(LPTSTR), bool onlineOnly, LPCTSTR device /*= NULL*/) {
	if (!_xml)return false;
	if (device != NULL && _tcslen(device) > 0 && _tcsicmp(device, _T("sky")) != 0)return false;
	CMarkup xml;
	xml.AddElem(_T("sky"));
	xml.AddAttrib(_T("name"), _T("天网设备"));
	xml.IntoElem();
	std::lock_guard<std::mutex> lock(m_lkHxhtServers);
	for (auto server : m_vHxhtServers) {
		server->EnumCamera(xml, onlineOnly);
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

void COnvifFactory::RemoveServer(CHxhtServer* server) {
	std::lock_guard<std::mutex> lock(m_lkHxhtServers);
	auto found(std::find(m_vHxhtServers.begin(), m_vHxhtServers.end(), server));
	if (found != m_vHxhtServers.end()) m_vHxhtServers.erase(found);
}