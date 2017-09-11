#pragma once
#include <map>
#include "os.h"
#include "xchar.h"

interface IDecodeProxyCallback {
	virtual ~IDecodeProxyCallback() {}
	enum CodecID { LPCM = 1, YV12 = 2, MP3 = 3, AAC = 4, H264 = 5 };
	virtual void				OnVideoFormat(const CodecID codec, const int width, const int height) = 0;
	virtual void				OnVideoFrame(const uint8_t* const data, const int size, const uint32_t pts) = 0;
	virtual void				OnAudioFormat(const CodecID codec, const int channel, const int bitwidth, const int samplerate) = 0;
	virtual void				OnAudioFrame(const uint8_t* const data, const int size, const uint32_t pts) = 0;
};

interface IDecodeProxy {
	enum CodecID { LPCM = 1, YV12 = 2, MP3 = 3, AAC = 4, H264 = 5 };
	virtual ~IDecodeProxy() {}
	virtual bool				Start() = 0;
	virtual bool				AddHeader(const uint8_t* const data, const int size) = 0;
	virtual bool				AddVideo(const uint8_t* const data, const int size) { return false; }
	virtual bool				AddAudio(const uint8_t* const data, const int size) { return false; }
	virtual bool				AddCustom(const uint8_t* const data, const int size) { return false; }
	virtual void				Stop() = 0;
	virtual bool				Discard() = 0;
};

interface IDecodeFactory {
	virtual ~IDecodeFactory() {}
	virtual LPCTSTR				FactoryName() const = 0;
	virtual bool				Initialize() = 0;
	virtual bool				DidSupport(const uint8_t* const header, const int length) = 0;
	virtual IDecodeProxy*		CreateDecoder(IDecodeProxyCallback* callback, const uint8_t* const header, const int length) = 0;
	virtual bool				GetStatusInfo(LPSTR* json, void(**free)(LPSTR)) = 0;
	virtual void				Uninitialize() = 0;
	virtual void				Destroy() = 0;
};

IDecodeFactory*					GetDecodeFactory();