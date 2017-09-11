#pragma once
#include <mutex>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/http.h>
#include <event2/http_struct.h>
#include "RtmpActor.h"
#include "SourceInterface.h"
#include "HandshakeBytes.h"
#include "RtmpHandshake.h"
#include "RtmpChunk.h"
#include "RtmpMessage.h"
#include "CommandMessage.h"
#include "DataMessage.h"
#include "SmartHdr.h"
#include "SmartNal.h"

class CRtmpPublisher : public CRtmpActor, public ISourceProxy{
public:
	CRtmpPublisher(uint32_t outputChunkSize);
	~CRtmpPublisher();

public:
	const xtstring& GetName() const { return m_streamName; }
	bool SetCallback(ISourceProxyCallback* callback);

protected:
	virtual Type getType() const { return Publisher; }
	virtual bool isStream(const std::string& streamName) const;
	virtual bool onMessage(evbuffer* output, CRtmpChunk* chunk);
	virtual bool onMessage(evbuffer* output, CRtmpMessage* message);

protected:
	virtual LPCTSTR			SourceName() const;
	virtual unsigned int    MaxStartDelay(const unsigned int def) { return def; }
	virtual unsigned int    MaxFrameDelay(const unsigned int def) { return def; }
	virtual bool			StartFetch(event_base* base);
	virtual void			WantKeyFrame();
	virtual bool			PTZControl(const PTZAction action, const int value);
	virtual bool			VideoEffect(const int bright, const int contrast, const int saturation, const int hue);
	virtual bool			Discard();

protected:
	bool onUnpublishStream(evbuffer* output, CFCUnpublishStreamRequest* request);
	bool onOnMetadata(evbuffer* output, COnMetadataRequest* request);
	bool onPublishStream(evbuffer* output, CPublishStreamRequest* request);

	bool onVideo(CRtmpChunk* chunk);
	bool onAudio(CRtmpChunk* chunk);
	bool onError(LPCTSTR reason);

protected:
	enum VideoCodec { JPEG = 1, H263, Screen, VP6, VP6A, ScreenV2, Avc };
	enum AudioCodec { PCM = 1, Mp3, LPCM, Alaw = 7, Mlaw, Aac = 10 };
	static LPCTSTR getVideoCodec(int codecId);
	static LPCTSTR getAudioCodec(int codecId);

private:
	uint32_t m_outputChunkSize;
	uint32_t m_streamId;
	xtstring m_streamName;

	std::vector<CSmartNal<uint8_t>> m_vHeaders;

	std::recursive_mutex m_lkCallback;
	ISourceProxyCallback* m_lpCallback;
	bool m_bFetching;
	CSmartHdr<event*, void> m_timer;
};

