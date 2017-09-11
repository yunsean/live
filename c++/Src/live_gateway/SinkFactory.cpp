#include "EventBase.h"
#include "SinkFactory.h"
#include "OutputFactory.h"
#include "ChannelFactory.h"
#include "ConsoleUI.h"
#include "Writelog.h"

CSinkFactory::CSinkFactory() {
}
CSinkFactory::~CSinkFactory() {
}

CSinkFactory& CSinkFactory::singleton() {
	static CSinkFactory instance;
	return instance;
}

bool CSinkFactory::initialize() {
	if (!COutputFactory::singleton().initialize(this)) {
		return wlet(false, _T("Initialize output factory failed."));
	}
	return true;
}
void CSinkFactory::status(Json::Value& status) {
	COutputFactory::singleton().statusInfo(status);
}

event_base* CSinkFactory::GetPreferBase() {
	return CEventBase::singleton().nextBase();
}
event_base** CSinkFactory::GetAllBase(int& count) {
	return CEventBase::singleton().allBase(count);
}
ISinkProxyCallback* CSinkFactory::AddSink(ISinkProxy* proxy, LPCTSTR moniker, LPCTSTR device, LPCTSTR param /*= NULL*/) {
	return CChannelFactory::singleton().bindChannel(proxy, device, moniker, param);
}
ISinkProxyCallback* CSinkFactory::AddSink(ISinkProxy* proxy, LPCTSTR moniker, LPCTSTR device, LPCTSTR param, uint64_t beginTime, uint64_t endTime /*= 0*/) {
	return NULL;
}

