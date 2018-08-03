#pragma once
#include <map>
#include "os.h"
#include "xchar.h"

#ifndef MKFCC
#define MKFCC(a, b, c, d) (((uint32_t)a << 0) + ((uint32_t)b << 8) + ((uint32_t)c << 16) + ((uint32_t)d << 24))
#endif

interface ISinkProxyCallback {
	virtual ~ISinkProxyCallback() {}
	virtual void				WantKeyFrame() = 0;
	virtual void				OnDiscarded() = 0;
};

#ifndef evutil_socket_t
#ifdef WIN32
#define evutil_socket_t intptr_t
#else
#define evutil_socket_t int
#endif
#endif
struct event_base;
interface ISinkProxy {
	enum CodecType { Raw = 0, Compressed, Baseband };
	enum CodecID { LPCM = MKFCC('M', 'C', 'P', 'L'), YV12 = MKFCC('2', '1', 'V', 'Y'), AAC = MKFCC(' ', 'C', 'A', 'A'), AVC = MKFCC(' ', 'C', 'V', 'A') };
	enum PackType { Header = 0, Video, Audio, Error, Custom };
	virtual ~ISinkProxy() {}
	virtual CodecType			PreferCodec() = 0;
	virtual bool				StartWrite(event_base* base) = 0;
	virtual void				OnVideoFormat(const CodecID codec, const int width, const int height, const double fps) = 0;
	virtual void				OnVideoConfig(const CodecID codec, const uint8_t* header, const int size) = 0;
	virtual void				OnVideoFrame(const uint8_t* const data, const int size, const uint32_t pts) = 0;
	virtual void				OnAudioFormat(const CodecID codec, const int channel, const int bitwidth, const int samplerate) = 0;
	virtual void				OnAudioConfig(const CodecID codec, const uint8_t* header, const int size) = 0;
	virtual void				OnAudioFrame(const uint8_t* const data, const int size, const uint32_t pts) = 0;
	virtual void				OnRawPack(PackType type, const uint8_t* const data, const int size, const bool key = true) = 0;
	virtual void				OnError(LPCTSTR reason) = 0;
	virtual bool				GetStatus(LPSTR* json, void(**free)(LPSTR)) = 0;
	virtual bool				Discard() = 0;
};

interface ISinkFactoryCallback {
	virtual ~ISinkFactoryCallback() {}
	virtual event_base*			GetPreferBase() = 0;	//根据当前线程获取event_base，如果当前线程不在loop中，则将返回nullptr
	virtual event_base**		GetAllBase(int& count) = 0;
	virtual ISinkProxyCallback*	AddSink(ISinkProxy* proxy, LPCTSTR moniker, LPCTSTR device, LPCTSTR param = NULL) = 0;
	virtual ISinkProxyCallback*	AddSink(ISinkProxy* proxy, LPCTSTR moniker, LPCTSTR device, LPCTSTR param, uint64_t beginTime, uint64_t endTime = 0) = 0;
};

interface ISinkHttpRequest {
	virtual LPCTSTR				GetOperation() const = 0;
	virtual LPCTSTR				GetVersion() const = 0;
	virtual LPCTSTR				GetUrl() const = 0;
	virtual LPCTSTR				GetPath() const = 0;
	virtual LPCTSTR				GetAction() const = 0;
	virtual LPCTSTR				GetIp() const = 0;
	virtual int					GetPort() const = 0;
	virtual LPCTSTR				GetParam(LPCTSTR key) const = 0;
	virtual LPCTSTR				GetPayload(LPCTSTR key) const = 0;
	virtual evutil_socket_t		GetSocket(bool detach = true) = 0;
	virtual event_base*			GetBase() = 0;
};

struct event_base;
interface ISinkFactory {
	enum SupportState { unsupport, supported, maybesupport };
	virtual ~ISinkFactory() {}
	virtual LPCTSTR				FactoryName() const = 0;
	virtual bool				Initialize(ISinkFactoryCallback* callback) = 0;
	virtual int					HandleSort() = 0;
	virtual bool				HandleRequest(ISinkHttpRequest* request) = 0;
	virtual bool				GetStatusInfo(LPSTR* json, void(**free)(LPSTR)) = 0;
	virtual void				Uninitialize() = 0;
	virtual void				Destroy() = 0;
};

ISinkFactory*					GetSinkFactory();