#pragma once
#include <mutex>
#include <atomic>
#include "EventBase.h"
#include "SourceInterface.h"
#include "SinkInterface.h"
#include "CodecInterface.h"
#include "HttpClient.h"
#include "ShareBlk.h"
#include "SmartPtr.h"
#include "SmartNal.h"
#include "Byte.h"
#include "xsystem.h"
#include "json/value.h"

class CChannel : public ISourceProxyCallback, IDecodeProxyCallback {
public:
	CChannel(const xtstring& moniker);
	~CChannel();

public:
	bool Startup(ISourceProxy* proxy);
	const xtstring& GetMoniker() const { return m_strMoniker; }
	ISinkProxyCallback* AddSink(ISinkProxy* client);
	bool PTZControl(CHttpClient* client, const int action, const int speed);
	bool VideoEffect(CHttpClient* client, const int bright, const int contrast, const int saturation, const int hue);
	void GetStatus(Json::Value& status);

protected:
	//IDecodeProxyCallback
	virtual void OnVideoFormat(const CodecID codec, const int width, const int height, const double fps);
	virtual void OnVideoConfig(const CodecID codec, const uint8_t* header, const int size);
	virtual void OnVideoFrame(const uint8_t* const data, const int size, const uint32_t pts);
	virtual void OnAudioFormat(const CodecID codec, const int channel, const int bitwidth, const int samplerate);
	virtual void OnAudioConfig(const CodecID codec, const uint8_t* header, const int size);
	virtual void OnAudioFrame(const uint8_t* const data, const int size, const uint32_t pts);

protected:
	//ISourceCallback
	virtual void OnHeaderData(const uint8_t* const data, const int size);
	virtual void OnVideoData(const uint8_t* const data, const int size, const bool key = true);
	virtual void OnAudioData(const uint8_t* const data, const int size);
	virtual void OnCustomData(const uint8_t* const data, const int size);
	virtual void OnError(LPCTSTR reason);
	virtual void OnStatistics(LPCTSTR statistics);
	virtual void OnControlResult(const unsigned int token, bool result, LPCTSTR reason);
	virtual void OnDiscarded();

protected:
	void WantKeyFrame();
	void OnMediaData(ISinkProxy::PackType type, const uint8_t* const data, const int size, const bool key = true);

protected:
	bool CreateDecoder();

protected:
	static void timer_callback(evutil_socket_t fd, short events, void* context);
	void timer_callback(evutil_socket_t fd, short events);

protected:
	bool onInited();
	bool onFetching();
	bool onDiscard();
	bool onWaiting();
	bool onSinkClosing();
	bool onDecodeClosing();
	bool onClosed();

protected:
	enum SourceStage { SS_Inited, SS_Fetching, SS_Discard, SS_Waiting, SS_SinkClosing, SS_DecodeClosing, SS_Closed };
	const char* getStage();
	void SetStage(SourceStage stage);

protected:
	friend class CSinkProxyCallback;
	enum ProxyStatus{PS_Pending, PS_Raw, PS_Compressed, PS_Baseband, PS_Discarded, PS_Closed};
	const char* getStatus(ProxyStatus status);

protected:
	static void on_error(struct bufferevent* event, short what, void* context);

protected:
	class CSinkProxy : public ISinkProxyCallback {
	public:
		CSinkProxy(std::recursive_mutex& mutex, CChannel* channel, ISinkProxy* proxy);
	protected:
		//ISinkProxyCallback
		virtual void WantKeyFrame();
		virtual void OnDiscarded();
	protected:
		friend class CChannel;
		void Discard();
	private:
		std::recursive_mutex& m_lkProxy;
		ProxyStatus m_eProxyStatus;
		CChannel* const m_lpChannel;
		ISinkProxy* const m_lpSourcer;
		bool m_bDiscarded;
	};
	typedef CSmartHdr<bufferevent*, void> bevptr;
	enum ControlState { pending, finished, failed, over, closed };
	class CControlClient {
	public:
		CControlClient(const int action, const int speed);
		CControlClient(const int bright, const int contrast, const int saturation, const int hue);
	protected:
		friend class CChannel;
	private:
		bevptr bev;
		uint32_t invalidAt;
		bool mode;	//0==>PTZControl	1==>VideoEffect
		ControlState state;
		union {
			struct {
				const int action;
				const int speed;
			};
			struct {
				const int bright;
				const int contrast;
				const int saturation;
				const int hue;
			};
		};
	};
	typedef std::shared_ptr<CControlClient> cliptr;

protected:
	void handleControlRequest();
	void handleControl(cliptr cli);
	void sendResponse(cliptr cli, int code = 200, const char* status = "OK");
	
private:
	const time_t m_startupTime;
	const xtstring m_strMoniker;
	volatile SourceStage m_stage;
	std::recursive_mutex m_lkProxy;
	ISourceProxy* m_lpSourcer;
	bool m_bNeedDecoder;
	IDecodeProxy* m_lpDecoder;
	CodecID m_videoCodec;
	CodecID m_audioCodec;

	std::recursive_mutex m_lkSink;
	std::vector<CSmartPtr<CSinkProxy>> m_spSinks;

	event_base* m_base;
	event* m_timer;
	unsigned int m_idleBeginAt;
	std::mutex m_lockCache;
	std::vector<CSmartNal<uint8_t>> m_rawHeaders;
	CByte m_rawCache;	//Length(4Bytes)+Type(1Byte)+Key(1Byte)+Data(NBytes)+...
	uint32_t m_latestSendData;
	uint64_t m_rawDataSize;
	xtstring m_error;
	
	std::recursive_mutex m_lockClient;
	std::map<evutil_socket_t, cliptr> m_pendingClient;
	std::map<unsigned int, cliptr> m_activedClient;
	unsigned int m_token;
};

