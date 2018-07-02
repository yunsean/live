#pragma once
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
class COnvifSource;
class COnvifControl {
public:
	COnvifControl(COnvifSource* source, const xtstring& ip, const std::string& usr, const std::string& pwd);
	~COnvifControl();

public:
	struct ControlCommand {
		ControlCommand(const unsigned int token, const int action, const int speed);
		ControlCommand(const unsigned int token, const int bright, const int contrast, const int saturation, const int hue);
		const unsigned int token;
		bool mode;	//0==>PTZControl	1==>VideoEffect
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

public:
	bool IsWorking();
	void DoCommand(const std::shared_ptr<ControlCommand>& command);
	void Discard();

protected:
	static void getaddrinfo_callback(int result, struct evutil_addrinfo *res, void *arg);
	void getaddrinfo_callback(int result, struct evutil_addrinfo *res);
	
	static void timer_callback(evutil_socket_t fd, short event, void* arg);
	void timer_callback(evutil_socket_t fd, short event);
	static void on_read(struct bufferevent* event, void* context);
	void on_read(struct bufferevent* event);

	static void http_callback(evhttp_request* req, void* context);
	void http_callback(evhttp_request* req);

protected:
	void dnsRequest();
	void probeRequest(const char* ip);
	void ptzCapabilitiesRequest();
	void mediaCapabilitiesRequest();
	void profilesRequest(const char* mediaUrl);
	void ptzRequest();
	void effectRequest();

protected:
	void onError(LPCTSTR reason);
	unsigned int reset();

protected:
	void probeResponse(struct bufferevent* event);
	void ptzCapabilitiesResponse(pugi::xml_document& xml);
	void mediaCapabilitiesResponse(pugi::xml_document& xml);
	void profilesResponse(pugi::xml_document& xml);
	void ptzResponse(pugi::xml_document& xml);
	void effectResponse(pugi::xml_document& xml);

protected:
	std::string findValidUrl(const xstring<char>& addr);

protected:
	evhttp_request* httpRequest(const char* url, evhttp_cmd_type method, size_t contentLen);

protected:
	enum OnvifStage { None, Probe, GetPtzCapabilities, GetMediaCapabilities, GetProfiles, Ptz, Effect };

private:
	xtstring m_moniker;
	std::string m_username;
	std::string m_password;
	COnvifSource* m_source;
	event_base* m_evbase;
	std::string m_messageId;
	CSmartHdr<evdns_base*, void> m_dns;
	CSmartHdr<bufferevent*, void> m_bev;
	CSmartHdr<event*, void> m_timer;
	CSmartHdr<evhttp_connection*, void> m_connection;
	OnvifStage m_onvifStage;

	std::string m_serviceAddr;
	std::string m_ptzUrl;
	std::string m_profileToken;

	std::shared_ptr<ControlCommand> m_command;
};

