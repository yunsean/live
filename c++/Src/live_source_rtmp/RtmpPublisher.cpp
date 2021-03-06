#include "RtmpPublisher.h"
#include "RtmpChunk.h"
#include "CommandMessage.h"
#include "ProtocolAmf0.h"
#include "ConsoleUI.h"
#include "AVCConfig.h"
#include "Writelog.h"
#include "xutils.h"
using namespace amf0;

CRtmpPublisher::CRtmpPublisher(uint32_t outputChunkSize)
	: m_outputChunkSize(outputChunkSize)
	, m_streamId(0)
	, m_streamName()
	, m_firstAudioPacket(true)
	, m_vConfigs()

	, m_lkCallback()
	, m_lpCallback(nullptr)
	, m_bFetching(false)
	, m_timer(event_free)

	, m_outputCache() {
}
CRtmpPublisher::~CRtmpPublisher() {
	m_lkCallback.lock();
	if (m_lpCallback != nullptr) {
		m_lpCallback->OnDiscarded();
		m_lkCallback.unlock();
	}
	cpm(_T("[%s] RTMP对象释放"), m_streamName.c_str());
}

bool CRtmpPublisher::SetCallback(ISourceProxyCallback* callback) {
	std::lock_guard<std::recursive_mutex> lock(m_lkCallback);
	if (m_lpCallback != nullptr) return false;
	m_lpCallback = callback;
	m_bFetching = false;
	return true;
}

LPCTSTR CRtmpPublisher::SourceName() const {
	return m_streamName.c_str();
}
bool CRtmpPublisher::StartFetch(event_base* base) {
	std::lock_guard<std::recursive_mutex> lock(m_lkCallback);
	m_bFetching = true;
	for (auto header : m_vConfigs) {
		m_lpCallback->OnHeaderData(header.GetArr(), header.GetSize());
	}
	return true;
}
void CRtmpPublisher::WantKeyFrame() {

}
ISourceProxy::ControlResult CRtmpPublisher::PTZControl(const unsigned int token, const unsigned int action, const int speed) {
	return ISourceProxy::Failed;
}
ISourceProxy::ControlResult CRtmpPublisher::VideoEffect(const unsigned int token, const int bright, const int contrast, const int saturation, const int hue) {
	return ISourceProxy::Failed;
}
bool CRtmpPublisher::Discard() {
	std::unique_lock<std::recursive_mutex> lock(m_lkCallback, std::try_to_lock);
	if (lock) {
		m_lpCallback->OnDiscarded();
		m_lpCallback = nullptr;
		m_bFetching = false;
		return true;
	}
	return false;
}

bool CRtmpPublisher::isStream(const std::string& streamName) const {
	xtstring name(streamName);
	return m_streamName.Compare(name) == 0;
}
bool CRtmpPublisher::onMessage(evbuffer* output, CRtmpChunk* chunk) {
	switch (chunk->type()) {
	case RTMP_MSG_AudioMessage:
		return onAudio(chunk);
	case RTMP_MSG_VideoMessage:
		return onVideo(chunk);
	}
	CSmartPtr<CRtmpMessage> message(nullptr);
	if (!chunk->buildMessage(message.GetPtr())) {
		return wlet(false, _T("decode packet failed: messageType=%d, length=%d"), (int)chunk->type(), chunk->length());
	} else {
		return onMessage(output, message);
	}
}
bool CRtmpPublisher::onMessage(evbuffer* output, CRtmpMessage* message) {
	switch (message->type()) {
	case PublishStreamRequest:
		return onPublishStream(output, dynamic_cast<CPublishStreamRequest*>(message));
	case OnMetadataRequest:
		return onOnMetadata(output, dynamic_cast<COnMetadataRequest*>(message));
	case FCUnpublishStreamRequest:
		return onUnpublishStream(output, dynamic_cast<CFCUnpublishStreamRequest*>(message));
	default:
		return true;
	}
}

bool CRtmpPublisher::onPublishStream(evbuffer* output, CPublishStreamRequest* request) {
	m_streamName = request->streamName;
	m_streamId = request->streamId();
	if (m_streamName.Find('?') > 0) {
		m_streamName = m_streamName.Left(m_streamName.Find('?'));
	}
	cpj(_T("[%s] 开始流发布"), m_streamName.c_str());
	CSmartPtr<CPublishStreamResponse> pkt(new CPublishStreamResponse(request->transactionId));
	pkt->props = Amf0Any::null();
	pkt->info = Amf0Any::object();
	pkt->info->set(StatusLevel, Amf0Any::string(StatusLevelStatus));
	pkt->info->set(StatusCode, Amf0Any::string(StatusCodePublishStart));
	pkt->info->set(StatusDescription, Amf0Any::string("Start publishing"));
	return CRtmpChunk::sendMessage(output, m_outputChunkSize, 0, m_streamId, pkt);
}
bool CRtmpPublisher::onUnpublishStream(evbuffer* output, CFCUnpublishStreamRequest* request) {
	CSmartPtr<CUnpublishStreamResponse> pkt(new CUnpublishStreamResponse(request->transactionId));
	pkt->props = Amf0Any::null();
	pkt->info = Amf0Any::object();
	pkt->info->set(StatusLevel, Amf0Any::string(StatusLevelStatus));
	pkt->info->set(StatusCode, Amf0Any::string(StatusCodeUnpublishSuccess));
	pkt->info->set(StatusDescription, Amf0Any::string("Stop publishing"));
	cpj(_T("[%s] 停止流发布"), m_streamName.c_str());
	return CRtmpChunk::sendMessage(output, m_outputChunkSize, 0, m_streamId, pkt);
}
bool CRtmpPublisher::onOnMetadata(evbuffer* output, COnMetadataRequest* request) {
	Amf0EcmaArray* metadata(request->metadata->toEcmaArray());
	if (metadata != nullptr) {
		Amf0Any* awidth(metadata->ensurePropertyNumber("width"));
		Amf0Any* aheight(metadata->ensurePropertyNumber("height"));
		Amf0Any* avcodec(metadata->ensurePropertyNumber("videocodecid"));
		Amf0Any* aacodec(metadata->ensurePropertyNumber("audiocodecid"));
		Amf0Any* asampleRate(metadata->ensurePropertyNumber("audiosamplerate"));
		Amf0Any* asampleSize(metadata->ensurePropertyNumber("audiosamplesize"));
		int width(awidth != nullptr ? (int)awidth->toNumber() : 0);
		int height(aheight != nullptr ? (int)aheight->toNumber() : 0);
		int vcodec(awidth != nullptr ? (int)avcodec->toNumber() : 0);
		int acodec(aheight != nullptr ? (int)aacodec->toNumber() : 0);
		int sampleRate(awidth != nullptr ? (int)asampleRate->toNumber() : 0);
		int sampleSize(aheight != nullptr ? (int)asampleSize->toNumber() : 0);
		cpm(_T("[%s] 视频：%dx%d@%s 音频：%dx%d@%s"), m_streamName.c_str(),
			width, height, getVideoCodec(vcodec),
			sampleRate, sampleSize, getAudioCodec(acodec));
	}
	return true;
}
LPCTSTR CRtmpPublisher::getVideoCodec(int codecId) {
	switch (codecId) {
	case JPEG: return _T("Jpeg");
	case H263: return _T("H263");
	case Screen:
	case ScreenV2: return _T("Screen");
	case VP6:
	case VP6A: return _T("VP6");
	case Avc: return _T("AVC");
	default: return _T("Unknown");
	}
}
LPCTSTR CRtmpPublisher::getAudioCodec(int codecId) {
	switch (codecId) {
	case PCM:
	case LPCM: return _T("LPCM");
	case Mp3: return _T("MP3");
	case Alaw: return _T("Alaw");
	case Mlaw: return _T("Mulaw");
	case Aac: return _T("AAC");
	default: return _T("Unknown");
	}
}

bool CRtmpPublisher::onError(LPCTSTR reason) {
	cpe(_T("%s"), reason);
	std::lock_guard<std::recursive_mutex> lock(m_lkCallback);
	if (m_lpCallback != nullptr) {
		m_lpCallback->OnError(_T("客户端断开连接"));
	}
	return false;
}
bool CRtmpPublisher::onVideo(CRtmpChunk* chunk) {
	//cpm(_T("video=%d"), chunk->timestamp());
	uint8_t* payload(chunk->payload());
	uint32_t size(chunk->length());
	if (size < 1) return onError(_T("数据流出现错误"));
	uint8_t codecId(payload[0] & 0x0f);
	std::lock_guard<std::recursive_mutex> lock(m_lkCallback);
	if (codecId == Avc) {
		if (size < 5) return onError(_T("数据流出现错误"));
		uint8_t packetType(payload[1]);
		if (packetType == 0) {	//sequence header
			if (size > 5) {		//Video Tag: VideoType[4B] + CodecID[4B] + AVCPacketType[UI8] + CompositionTime[UI24]
				CAVCConfig config;
				config.Unserialize(payload + 5, size - 5);
				CByte nal;
				for (int i = 0; i < config.SpsCount(); i++) {
					CSmartNal<unsigned char> sps;
					config.GetSps(i, sps);
					nal.AppendData(sps.GetArr(), sps.GetSize());
				}
				for (int i = 0; i < config.PpsCount(); i++) {
					CSmartNal<unsigned char> pps;
					config.GetPps(i, pps);
					nal.AppendData(pps.GetArr(), pps.GetSize());
				}
				CSmartNal<uint8_t> header(4 + 1 + 4 + 2 + nal.GetSize());
				*(reinterpret_cast<uint32_t*>(header.GetArr())) = MKFCC('1', 'L', 'A', 'N');
				*(reinterpret_cast<uint8_t*>(header.GetArr() + 4)) = 'V';
				*(reinterpret_cast<uint32_t*>(header.GetArr() + 5)) = MKFCC(' ', 'C', 'V', 'A');
				*(reinterpret_cast<uint16_t*>(header.GetArr() + 9)) = static_cast<uint16_t>(nal.GetSize());
				if (nal.GetSize() > 0) memcpy(reinterpret_cast<uint8_t*>(header.GetArr()) + 11, nal.GetData(), nal.GetSize());
				header.SetSize(4 + 1 + 4 + 2 + nal.GetSize());
				m_vConfigs.push_back(header);
				if (m_lpCallback)m_lpCallback->OnHeaderData(header.GetArr(), header.GetSize());
			}
		} else if (m_lpCallback) {
			*(reinterpret_cast<uint32_t*>(payload + 5 - 4)) = static_cast<uint32_t>(chunk->timestamp());
			uint8_t* ptr(payload + 5);
			uint8_t* end(ptr + size - 5);
			while (end - ptr > 4) {
				uint32_t len(utils::uintFrom4BytesBE(ptr));
				static const uint8_t NalHeader[] = { 0x00, 0x00, 0x00, 0x01 };
				memcpy(ptr, NalHeader, 4);
				ptr += len + 4;
			}
			m_lpCallback->OnVideoData(payload + 5 - 4, size - 5 + 4, (payload[0] & 0x10) != 0);
		}
	}
	return true;
}
bool CRtmpPublisher::onAudio(CRtmpChunk* chunk) {
	//cpw(_T("audio=%d"), chunk->timestamp());
	uint8_t* payload(chunk->payload());
	uint32_t size(chunk->length());
	if (size < 1) return onError(_T("数据流出现错误"));
	uint8_t codecId((payload[0] >> 4) & 0x0f);
	std::lock_guard<std::recursive_mutex> lock(m_lkCallback);
	if (codecId == Aac) {
		if (size < 2) return onError(_T("数据流出现错误"));
		uint8_t packetType(payload[1]);
		if (packetType == 0) {	//sequence header
			if (size > 2) {		//SoundFormat[4B] + SoundRate[2B] + SoundSize[1B] + SoundType[1B] + AACPacketType[UI8]
				CSmartNal<uint8_t> header(4 + 1 + 4 + 2 + (size - 2));
				*(reinterpret_cast<uint32_t*>(header.GetArr())) = MKFCC('1', 'L', 'A', 'N');
				*(reinterpret_cast<uint8_t*>(header.GetArr() + 4)) = 'A';
				*(reinterpret_cast<uint32_t*>(header.GetArr() + 5)) = MKFCC(' ', 'C', 'A', 'A');
				*(reinterpret_cast<uint16_t*>(header.GetArr() + 9)) = static_cast<uint16_t>(size - 2);
				if (size - 2 > 0) memcpy(reinterpret_cast<uint8_t*>(header.GetArr()) + 11, payload + 2, size - 2);
				header.SetSize(4 + 1 + 4 + 2 + (size - 2));
				m_vConfigs.push_back(header);
				if (m_lpCallback)m_lpCallback->OnHeaderData(header.GetArr(), header.GetSize());
			}
		} else if (m_lpCallback) {
			m_outputCache.EnsureSize(size - 2 + 4);
			*(reinterpret_cast<uint32_t*>(m_outputCache.GetData())) = static_cast<uint32_t>(chunk->timestamp());
			memcpy(m_outputCache.GetData() + 4, payload + 2, size - 2);
			m_lpCallback->OnAudioData(m_outputCache.GetData(), size - 2 + 4);
		}
	} else if (codecId == Mp3) {
		if (m_firstAudioPacket) {
			m_firstAudioPacket = false;
			CSmartNal<uint8_t> header(4 + 1 + 4 + 2);
			*(reinterpret_cast<uint32_t*>(header.GetArr())) = MKFCC('1', 'L', 'A', 'N');
			*(reinterpret_cast<uint8_t*>(header.GetArr() + 4)) = 'A';
			*(reinterpret_cast<uint32_t*>(header.GetArr() + 5)) = MKFCC(' ', '3', 'P', 'M');
			*(reinterpret_cast<uint16_t*>(header.GetArr() + 9)) = 0;
			header.SetSize(4 + 1 + 4 + 2);
			m_vConfigs.push_back(header);
			if (m_lpCallback)m_lpCallback->OnHeaderData(header.GetArr(), header.GetSize());
		}
		m_outputCache.EnsureSize(size + 4);
		*(reinterpret_cast<uint32_t*>(m_outputCache.GetData())) = static_cast<uint32_t>(chunk->timestamp());
		memcpy(m_outputCache.GetData() + 4, payload, size);
		m_lpCallback->OnAudioData(m_outputCache.GetData(), size + 4);
	}
	return true;
}
