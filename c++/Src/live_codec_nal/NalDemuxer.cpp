#include "NalDemuxer.h"
#include "ConsoleUI.h"
#include "Writelog.h"
#include "xutils.h"
#include "SmartNal.h"

#define CLOSE_AFTER_INVALID_MS	(10 * 1000)
#define CHECK_INTERVAL			(1 * 1000)
#define BUFFER_THRESHOLD		2
#define RELEASE_AFTER_IDEL_MS	(5 * 1000)

CNalDemuxer::CNalDemuxer(IDecodeProxyCallback* callback)
	: m_lpCallback(callback) {
}
CNalDemuxer::~CNalDemuxer() {
}

bool CNalDemuxer::Start() {
	return true;
}
bool CNalDemuxer::AddHeader(const uint8_t* const data, const int size) {
	if (size < 11) return false;
	if (data[3] != 'N' || data[2] != 'A' || data[1] != 'L' || data[0] != '1') return false;
	uint32_t videoFourCC = 0;
	CSmartNal<uint8_t> videoConfig;
	uint32_t audioFourCC = 0;
	CSmartNal<uint8_t> audioConfig;
	const uint8_t* ptr(data + 4);
	const uint8_t* end(data + size);
	while (end - ptr >= 7) {
		uint8_t type = *ptr++;
		if (type == 'V') {
			videoFourCC = *reinterpret_cast<const uint32_t*>(ptr);
			ptr += 4;
			uint16_t size = *reinterpret_cast<const uint16_t*>(ptr);
			ptr += 2;
			if (size > 0) {
				if (end - ptr < size) return wlet(false, _T("Invalid [NAL1] header."));
				videoConfig.Copy(ptr, size);
				ptr += size;
			}
		} else if (type == 'A') {
			audioFourCC = *reinterpret_cast<const uint32_t*>(ptr);
			ptr += 4;
			uint16_t size = *reinterpret_cast<const uint16_t*>(ptr);
			ptr += 2;
			if (size > 0) {
				if (end - ptr < size) return wlet(false, _T("Invalid [NAL1] header."));
				audioConfig.Copy(ptr, size);
				ptr += size;
			}
		}
	} 
	if (videoFourCC == IDecodeProxyCallback::AVC) {
		m_lpCallback->OnVideoConfig(IDecodeProxyCallback::AVC, videoConfig.GetArr(), videoConfig.GetSize());
	} else if (videoFourCC != 0) {
	}
	if (audioFourCC == IDecodeProxyCallback::AAC) {
		m_lpCallback->OnAudioConfig(IDecodeProxyCallback::AAC, audioConfig.GetArr(), audioConfig.GetSize());
	} else if (audioFourCC != 0) {
	}
	return true;
}
bool CNalDemuxer::AddVideo(const uint8_t* const data, const int size) {
	if (size < 8) return false;
	uint32_t pts = *(uint32_t*)data;
	m_lpCallback->OnVideoFrame(data + 4, size - 4, pts);
	return true;
}
bool CNalDemuxer::AddAudio(const uint8_t* const data, const int size) {
	if (size < 8) return false;
	uint32_t pts = *(uint32_t*)data;
	m_lpCallback->OnAudioFrame(data + 4, size - 4, pts);
	return true;
}
bool CNalDemuxer::AddCustom(const uint8_t* const data, const int size) {
	return true;
}
void CNalDemuxer::Stop() {

}
bool CNalDemuxer::Discard() {
	delete this;
	return true;
}
