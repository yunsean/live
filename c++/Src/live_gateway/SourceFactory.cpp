#include "EventBase.h"
#include "SourceFactory.h"
#include "InputFactory.h"
#include "SystemInfo.h"
#include "HttpClient.h"
#include "Writelog.h"
#include "ConsoleUI.h"
#include "xaid.h"

CSourceFactory::CSourceFactory() {
}
CSourceFactory::~CSourceFactory() {
}

CSourceFactory& CSourceFactory::singleton() {
	static CSourceFactory instance;
	return instance;
}

bool CSourceFactory::initialize() {
	if (!CInputFactory::singleton().initialize(this)) {
		return wlet(false, _T("Initialize input factory failed."));
	}
	return true;
}
ISourceProxy* CSourceFactory::createLiveSource(ISourceProxyCallback* callback, LPCTSTR device, LPCTSTR moniker, LPCTSTR param) {
	return CInputFactory::singleton().createLiveSource(callback, device, moniker, param);
}
ISourceProxy* CSourceFactory::createPastSource(ISourceProxyCallback* callback, LPCTSTR device, LPCTSTR moniker, LPCTSTR param, uint64_t beginTime, uint64_t endTime /*= 0*/) {
	return CInputFactory::singleton().createPastSource(callback, device, moniker, param, beginTime, endTime);
}
bool CSourceFactory::streamList(xtstring& xml, const xtstring& device, bool onlineOnly) {
	return CInputFactory::singleton().streamList(xml, device, onlineOnly);
}
void CSourceFactory::status(Json::Value& status) {
	CInputFactory::singleton().statusInfo(status);
}
void CSourceFactory::uninitialize() {
	CInputFactory::singleton().uninitialize();
}

event_base** CSourceFactory::GetAllBase(int& count) {
	return CEventBase::singleton().allBase(count);
}
event_base* CSourceFactory::GetPreferBase() {
	return CEventBase::singleton().preferBase();
}
void CSourceFactory::OnNewSource(LPCTSTR lpszDevice, LPCTSTR lpszMoniker, LPCTSTR lpszName, LPCTSTR lpszMeta /* = NULL */) {
	cpw(_T("[%s] NewSource[%s] Coming"), lpszDevice, lpszName);
}
void CSourceFactory::OnSourceOver(LPCTSTR lpszDevice, LPCTSTR lpszMoniker) {
	cpw(_T("[%s] Source Disconnected"), lpszDevice);
}