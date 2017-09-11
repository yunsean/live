#pragma once
#include <mutex>
#include <atomic>
#include "EventBase.h"
#include "SourceInterface.h"
#include "SinkInterface.h"
#include "ShareBlk.h"
#include "SmartPtr.h"
#include "Byte.h"
#include "xsystem.h"
#include "json/value.h"

class CChannel : public ISourceProxyCallback{
public:
	CChannel(const xtstring& moniker);
	~CChannel();

public:
	bool Startup(ISourceProxy* proxy);
	const xtstring& GetMoniker() const { return m_strMoniker; }
	ISinkProxyCallback* AddSink(ISinkProxy* client);
	void GetStatus(Json::Value& status);

protected:
	//ISourceCallback
	virtual void OnHeaderData(const uint8_t* const data, const int size);
	virtual void OnVideoData(const uint8_t* const data, const int size, const bool key = true);
	virtual void OnAudioData(const uint8_t* const data, const int size);
	virtual void OnCustomData(const uint8_t* const data, const int size);
	virtual void OnError(LPCTSTR reason);
	virtual void OnStatistics(LPCTSTR statistics);
	virtual void OnDiscarded();

protected:
	void WantKeyFrame();
	void OnMediaData(ISinkProxy::PackType type, const uint8_t* const data, const int size, const bool key = true);

protected:
	static void timer_callback(evutil_socket_t fd, short events, void* context);
	void timer_callback(evutil_socket_t fd, short events);

protected:
	bool onInited();
	bool onFetching();
	bool onDiscard();
	bool onWaiting();
	bool onClosing();
	bool onClosed();

protected:
	enum SourceStage { SS_Inited, SS_Fetching, SS_Discard, SS_Waiting, SS_Closing, SS_Closed };
	const char* getStage();
	void SetStage(SourceStage stage);

protected:
	friend class CSinkProxyCallback;
	enum ProxyStatus{PS_Pending, PS_Raw, PS_Compressed, PS_Baseband, PS_Discarded, PS_Closed};
	const char* getStatus(ProxyStatus status);

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
		ISinkProxy* const m_lpProxy;
		bool m_bDiscarded;
	};
	
private:
	const time_t m_startupTime;
	const xtstring m_strMoniker;
	volatile SourceStage m_stage;
	std::recursive_mutex m_lkProxy;
	ISourceProxy* m_lpProxy;

	std::recursive_mutex m_lkSink;
	std::vector<CSmartPtr<CSinkProxy>> m_spSinks;

	event_base* m_base;
	event* m_timer;
	unsigned int m_idleBeginAt;
	std::mutex m_lockCache;
	CByte m_rawHeader;
	CByte m_rawCache;	//Length(4Bytes)+Type(1Byte)+Key(1Byte)+Data(NBytes)+...
	uint32_t m_latestSendData;
	uint64_t m_rawDataSize;
	xtstring m_error;
};

