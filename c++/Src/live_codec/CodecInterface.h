#pragma once
#include <map>
#include "os.h"
#include "xchar.h"

#ifndef MKFCC
#define MKFCC(a, b, c, d) (((uint32_t)a << 0) + ((uint32_t)b << 8) + ((uint32_t)c << 16) + ((uint32_t)d << 24))
#endif

interface IDecodeProxyCallback {
	virtual ~IDecodeProxyCallback() {}
	enum CodecID { LPCM = MKFCC('M', 'C', 'P', 'L'), YV12 = MKFCC('2', '1', 'V', 'Y'), AAC = MKFCC(' ', 'C', 'A', 'A'), AVC = MKFCC(' ', 'C', 'V', 'A') };
	virtual void				OnVideoFormat(const CodecID codec, const int width, const int height, const double fps) = 0;
	virtual void				OnVideoConfig(const CodecID codec, const uint8_t* header, const int size) = 0;
	virtual void				OnVideoFrame(const uint8_t* const data, const int size, const uint32_t pts) = 0;
	virtual void				OnAudioFormat(const CodecID codec, const int channel, const int bitwidth, const int samplerate) = 0;
	virtual void				OnAudioConfig(const CodecID codec, const uint8_t* header, const int size) = 0;
	virtual void				OnAudioFrame(const uint8_t* const data, const int size, const uint32_t pts) = 0;
};

interface IDecodeProxy {
	virtual ~IDecodeProxy() {}
	virtual bool				Start() = 0;
	virtual bool				CrossThread() = 0;
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