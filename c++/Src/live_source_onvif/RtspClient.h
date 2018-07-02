#pragma once
#include <list>
#include <functional>
#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/http.h>
#include <event2/http_struct.h>
#include "RtpH264Demuxer.h"
#include "RtpAacDemuxer.h"
#include "SmartHdr.h"

class CRtspClient {
public:
	interface ICallback {
		virtual void onVideoConfig(uint32_t fourCC, const uint8_t* data, int size) = 0;
		virtual void onAudioConfig(uint32_t fourCC, const uint8_t* data, int size) = 0;
		virtual void onVideoFrame(uint32_t timestamp, const uint8_t* data, int size) = 0;
		virtual void onAudioFrame(uint32_t timestamp, const uint8_t* data, int size) = 0;
		virtual void onError(LPCTSTR reason) = 0;
	};

public:
	CRtspClient(event_base* base, ICallback* callback, const char* username, const char* password);
	~CRtspClient();

public:
	bool feed(const char* url);
	
protected:
	void onConnect();
	void onOptions();
	void onDescribe();
	void onSetupVideo();
	void onSetupAudio();
	void onPlay();
	void onRtp();

protected:
	void onError(LPCTSTR reason);
	bool onUnauthorized();

protected:
	std::string format(const char* fmt, ...);
	std::string authorize(const char* method, const std::string& url);
	bool parse_header(size_t length);
	bool parse_sdp(const char* sdp);
	bool parse_packet(const uint8_t* buf, int len);

protected:
	enum RtspStage { Connecting, Options, Describe, SetupVideo, SetupAudio, Play, Streaming };

protected:
	static void on_read(struct bufferevent* event, void* context);
	void on_read(struct bufferevent* event);
	static void on_event(struct bufferevent* bev, short what, void* context);
	void on_event(struct bufferevent* bev, short what);

private:
	ICallback* m_callback;
	event_base* m_evbase;
	std::string m_username;
	std::string m_password;
	std::string m_basic;
	std::string m_realm;
	std::string m_nonce;
	std::string m_session;
	uint32_t m_seqId;
	CSmartHdr<bufferevent*, void> m_bev;
	std::string m_url;
	RtspStage m_rtspStage;
	bool m_readRtspWhileStreaming;
	int m_contentLength;
	size_t m_cacheSize;
	std::unique_ptr<char[]> m_cache;
	std::map<std::string, std::string> m_headers;

	std::unique_ptr<CRtpH264Demuxer> m_videoTrack;
	std::unique_ptr<CRtpAacDemuxer> m_audioTrack;

	uint32_t m_latestHeartbeat;
	uint8_t m_rtpChannel;
	int m_rtpLength;
};

