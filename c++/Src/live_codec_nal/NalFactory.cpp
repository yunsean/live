#include "NalFactory.h"
#include "NalDemuxer.h"
#include "Writelog.h"
#include "ConsoleUI.h"
#include "json/value.h"
#include "xaid.h"

CNalFactory::CNalFactory()
	: m_lpszFactoryName(_T("Nal Demuxer")) {
}
CNalFactory::~CNalFactory() {
}

CNalFactory& CNalFactory::singleton() {
	static CNalFactory instance;
	return instance;
}

LPCTSTR CNalFactory::FactoryName() const {
	return m_lpszFactoryName;
}
bool CNalFactory::Initialize() {
	return true;
}
bool CNalFactory::DidSupport(const uint8_t* const header, const int length) {
	if (length < 4) return false;
	if (header[3] != 'N' || header[2] != 'A' || header[1] != 'L' || header[0] != '1') return false;
	return true;
}
IDecodeProxy* CNalFactory::CreateDecoder(IDecodeProxyCallback* callback, const uint8_t* const header, const int length) {
	if (length < 4) return nullptr;
	if (header[3] != 'N' || header[2] != 'A' || header[1] != 'L' || header[0] != '1') return nullptr;
	return new CNalDemuxer(callback);
}
bool CNalFactory::GetStatusInfo(LPSTR* json, void(**free)(LPSTR)) {
	return false;
}
void CNalFactory::Uninitialize() {

}
void CNalFactory::Destroy() {

}