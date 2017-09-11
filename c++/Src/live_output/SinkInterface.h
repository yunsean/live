#pragma once
#include <map>
#include "os.h"
#include "xchar.h"

interface ISinkProxyCallback {
	virtual ~ISinkProxyCallback() {}
	virtual void				WantKeyFrame() = 0;
	virtual void				OnDiscarded() = 0;
};

struct event_base;
interface ISinkProxy {
	enum CodecType { Raw = 0, Compressed, Baseband };
	enum CodecID { LPCM = 1, YV12 = 2, MP3 = 3, AAC = 4, H264 = 5 };
	enum PackType { Header = 0, Video, Audio, Error, Custom };
	virtual ~ISinkProxy() {}
	virtual CodecType			PreferCodec() = 0;
	virtual bool				StartWrite(event_base* base) = 0;
	virtual void				OnVideoFormat(const CodecID codec, const int width, const int height, const double fps) = 0;
	virtual void				OnVideoFrame(const uint8_t* const data, const int size, const uint32_t pts) = 0;
	virtual void				OnAudioFormat(const CodecID codec, const int channel, const int bitwidth, const int samplerate) = 0;
	virtual void				OnAudioFrame(const uint8_t* const data, const int size, const uint32_t pts) = 0;
	virtual void				OnRawPack(PackType type, const uint8_t* const data, const int size, const bool key = true) = 0;
	virtual void				OnError(LPCTSTR reason) = 0;
	virtual bool				GetStatus(LPSTR* json, void(**free)(LPSTR)) = 0;
	virtual bool				Discard() = 0;
};

interface ISinkFactoryCallback {
	virtual ~ISinkFactoryCallback() {}
	virtual event_base*			GetPreferBase() = 0;
	virtual event_base**		GetAllBase(int& count) = 0;
	virtual ISinkProxyCallback*	AddSink(ISinkProxy* proxy, LPCTSTR moniker, LPCTSTR device, LPCTSTR param = NULL) = 0;
	virtual ISinkProxyCallback*	AddSink(ISinkProxy* proxy, LPCTSTR moniker, LPCTSTR device, LPCTSTR param, uint64_t beginTime, uint64_t endTime = 0) = 0;
};

struct event_base;
interface ISinkFactory {
	enum SupportState { unsupport, supported, maybesupport };
	virtual ~ISinkFactory() {}
	virtual LPCTSTR				FactoryName() const = 0;
	virtual bool				Initialize(ISinkFactoryCallback* callback) = 0;
	virtual bool				GetStatusInfo(LPSTR* json, void(**free)(LPSTR)) = 0;
	virtual void				Uninitialize() = 0;
	virtual void				Destroy() = 0;
};

ISinkFactory*					GetSinkFactory();