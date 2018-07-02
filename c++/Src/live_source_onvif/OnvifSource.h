#pragma once
#include <list>
#include <queue>
#include <mutex>
#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/http.h>
#include <event2/http_struct.h>
#include <event2/dns.h>
#include <mutex>
#include "SourceInterface.h"
#include "OnvifControl.h"
#include "xstring.h"
#include "RtspClient.h"
#include "SmartPtr.h"
#include "SmartNal.h"
#include "SmartHdr.h"
#include "xthread.h"

namespace pugi {
	class xml_document;
}

class COnvifSource : public ISourceProxy, CRtspClient::ICallback {
public:
	COnvifSource(const xtstring& ip, const std::string& usr, const std::string& pwd, const int width);
	~COnvifSource(void);

public:
	bool Open(ISourceProxyCallback* callback);

protected:
	//ISourceProxy
	virtual LPCTSTR SourceName() const { return m_moniker; }
	virtual bool StartFetch(event_base* base);
	virtual void WantKeyFrame();
	virtual ControlResult PTZControl(const unsigned int token, const unsigned int action, const int speed);
	virtual ControlResult VideoEffect(const unsigned int token, const int bright, const int contrast, const int saturation, const int hue);
	virtual bool Discard();

protected:
	//CRtspClient::ICallback
	void onVideoConfig(uint32_t fourCC, const uint8_t* data, int size);
	void onAudioConfig(uint32_t fourCC, const uint8_t* data, int size);
	void onVideoFrame(uint32_t timestamp, const uint8_t* data, int size);
	void onAudioFrame(uint32_t timestamp, const uint8_t* data, int size);
	void onError(LPCTSTR reason);

protected:
	friend class COnvifControl;
	void OnControlComplete(const unsigned int token, bool result);
	void DoControl();

protected:
	static void getaddrinfo_callback(int result, struct evutil_addrinfo *res, void *arg);
	void getaddrinfo_callback(int result, struct evutil_addrinfo *res);

	static void timer_callback(evutil_socket_t fd, short event, void* arg);
	void timer_callback(evutil_socket_t fd, short event);

	static void on_read(struct bufferevent* event, void* context);
	void on_read(struct bufferevent* event);
	static void on_error(struct bufferevent* bev, short what, void* context);
	void on_error(struct bufferevent* bev, short what);

	static void http_callback(evhttp_request* req, void* context);
	void http_callback(evhttp_request* req);

protected:
	bool dnsRequest();
	bool probeRequest(const char* ip);
	bool capabilitiesRequest();
	bool profilesRequest();
	bool streamUriRequest(const char* token);

protected:
	void probeResponse(struct bufferevent* event);
	void capabilitiesResponse(pugi::xml_document& xml);
	void profilesResponse(pugi::xml_document& xml);
	void streamUriResponse(pugi::xml_document& xml);

protected:
	std::string findValidUrl(const xstring<char>& addr);

protected:
	evhttp_request* httpRequest(const char* url, evhttp_cmd_type method, size_t contentLen);

protected:
	enum OnvifStage {
		Ready, Probe, GetCapabilities, GetProfiles, GetStreamUri,
		Options, Describe, SetupVideo, SetupAudio, Play,
		Corrupted
	};
	typedef COnvifControl::ControlCommand ControlCommand;
	typedef std::shared_ptr<ControlCommand> cmdptr;
	
private:
	xtstring m_moniker;
	int m_preferWidth;
	std::string m_username;
	std::string m_password;
	ISourceProxyCallback* m_lpCallback;
	std::vector<CSmartNal<uint8_t>> m_vConfigs;

	event_base* m_evbase;
	CSmartHdr<evdns_base*, void> m_dns;
	CSmartHdr<bufferevent*, void> m_bev;
	CSmartHdr<event*, void> m_timer;
	CSmartHdr<evhttp_connection*, void> m_connection;
	CSmartPtr<CRtspClient> m_rtspClient;
	std::string m_messageId;
	std::string m_serviceAddr;
	std::string m_mediaUrl;
	OnvifStage m_onvifStage;

	std::recursive_mutex m_lockCommands;
	std::queue<cmdptr> m_ptzCommands;
	COnvifControl m_onvifControl;
};

