#include "Publisher.h"
#include "Rtmp2Publisher.h"

CPublisher::CPublisher(const std::function<void(long sendBlocks, long lostBlocks, int bitrate)>& callback /*= nullptr*/)
	: m_publisher(new CRtmp2Publisher(callback)){
}
CPublisher::~CPublisher() {
	delete m_publisher;
}

void CPublisher::setupVideo(const int width, const int height, const unsigned char* sps, const int szSps, const unsigned char* pps, const int szPps) {
	return m_publisher->setupVideo(width, height, sps, szSps, pps, szPps);
}
void CPublisher::setupAudio(int channels, int sampleRate, const unsigned char* esds, const int szEsds) {
	return m_publisher->setupAudio(channels, sampleRate, esds, szEsds);
}
void CPublisher::appendVideo(const unsigned char* datas, int length, long timecode, bool isKey, bool isNal /*= false*/, bool waitBlock /*= false*/) {
	return m_publisher->appendVideo(datas, length, timecode, isKey, isNal, waitBlock);
}
void CPublisher::appendAudio(const unsigned char* datas, int length, long timecode, bool waitBlock /*= false*/) {
	return m_publisher->appendAudio(datas, length, timecode, waitBlock);
}
bool CPublisher::connect(const char* url) {
	return m_publisher->connect(url);
}
void CPublisher::disconnect() {
	return m_publisher->disconnect();
}
void CPublisher::cleanup() {
	return m_publisher->cleanup();
}
