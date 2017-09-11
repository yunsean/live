#include "CodecFactory.h"
#include "DecodeFactory.h"
#include "HttpClient.h"
#include "Writelog.h"
#include "ConsoleUI.h"
#include "xaid.h"

CCodecFactory::CCodecFactory() {
}
CCodecFactory::~CCodecFactory() {
}

CCodecFactory& CCodecFactory::singleton() {
	static CCodecFactory instance;
	return instance;
}

bool CCodecFactory::initialize() {
	if (!CDecodeFactory::singleton().initialize()) {
		return wlet(false, _T("Initialize input factory failed."));
	}
	return true;
}
void CCodecFactory::status(Json::Value& status) {
	CDecodeFactory::singleton().statusInfo(status);
}
void CCodecFactory::uninitialize() {
	CDecodeFactory::singleton().uninitialize();
}
