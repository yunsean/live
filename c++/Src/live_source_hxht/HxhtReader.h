#pragma once
#include <memory>
#include <atomic>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/http.h>
#include <event2/http_struct.h>
#include "SourceInterface.h"
#include "SmartHdr.h"
#include "Byte.h"
#include "Markup.h"

class CHxhtServer;
class CHxhtReader : public ISourceProxy {
public:
	CHxhtReader(CHxhtServer* server, ISourceProxyCallback* callback, const xtstring& host, int port, const xtstring& usr, const xtstring& pwd, const xtstring moniker, const xtstring& name);
	~CHxhtReader();

protected:
	//ISourceProxy
	virtual LPCTSTR SourceName() const { return m_moniker; }
	virtual bool StartFetch(event_base* base);
	virtual void WantKeyFrame();
	virtual bool PTZControl(const PTZAction eAction, const int nValue);
	virtual bool VideoEffect(const int nBright, const int nContrast, const int nSaturation, const int nHue);
	virtual bool Discard();

protected:
	bool httpGet(const char* url);
	xtstring xmlNodeValue(CMarkup& xml, LPCTSTR node);
	void onError(LPCTSTR reason);
	void connectAccess(const xtstring& ip, int port);
	void handShark();
	void openLive();
	void connectVideo(const xtstring& svrIp, int svrPort);
	void openStream();
	void onFetching(unsigned int type, const uint8_t* data, int size);

protected:
	void onHandShark(uint32_t result);
	void onOpenLive(uint32_t result, const std::string& message);
	void onOpenStream(uint32_t result, const std::string& message);

protected:
	static void http_callback(evhttp_request* req, void* context);
	void http_callback(evhttp_request* req);
	static void on_read(struct bufferevent* bev, void* ctx);
	void on_read(struct bufferevent* bev);
	static void on_event(struct bufferevent* bev, short what, void* ctx);
	void on_event(struct bufferevent* bev, short what);

protected:
	enum {
		eSharkHand = 0x138a, eOpenLive = 0x1392, eGetStream = 0x1f40,
		ePTZControl = 0x138e, eShiftTime = 0x138c
	};
	enum {
		rSharkHand = 0x138b, rOpenLive = 0x1393, rGetStream = 0x1f41,
		rPTZControl = 0x138f, rShiftTime = 0x138d
	};
	const CByte& x11Pack(unsigned int action, unsigned int index, const std::string& session, const char* resource, const std::string& message);
	bool x11Unpack(const uint8_t* data, int size);

protected:
	enum WorkStage {
		Inited, Login, HandShark, OpenLive, OpenStream, Fetching, Corrupted
	};
	enum MediaType { 
		Header = 0x1f42, Body = 0x1f44 
	};

protected:
	CHxhtServer* const m_server;
	xtstring m_host;
	int m_port;
	xtstring m_user;
	xtstring m_pwd;
	xtstring m_moniker;
	xtstring m_name;
	xtstring m_localIp;

	ISourceProxyCallback* m_lpCallback;
	event_base* m_base;
	CSmartHdr<evhttp_connection*, void> m_connection;

	xtstring m_sessionId;
	xtstring m_privilege;
	xtstring m_naming;
	xtstring m_ticket;
	CSmartHdr<bufferevent*, void> m_bev;
	int m_workStage;

	unsigned int m_uIndex;
	CByte m_byX11Package;
	int m_responseSkipBytes;

	CByte m_byFirstFrame;
	int m_nVersion;
};

