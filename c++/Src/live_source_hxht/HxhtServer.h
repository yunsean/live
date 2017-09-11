#pragma once
#include <memory>
#include <map>
#include <xstring.h>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/http.h>
#include <event2/http_struct.h>
#include "SmartHdr.h"
#include "Markup.h"
#include "Byte.h"

struct event_base;
interface ISourceProxy;
interface ISourceProxyCallback;
class CHxhtServer {
public:
	CHxhtServer(const xtstring& host, int port, const xtstring& usr, const xtstring& pwd);
	~CHxhtServer();

public:
	bool StartEnum(event_base* base);
	void EnumCamera(CMarkup& xml, bool onlineOnly) const;
	bool HasCamera(LPCTSTR moniker);
	ISourceProxy* CreateProxy(ISourceProxyCallback* callback, LPCTSTR moniker);

protected:
	friend class CHxhtReader;
	void SetOffline(const xtstring& naming, bool offline);

protected:
	bool httpGet(const char* url);

protected:
	static void http_callback(evhttp_request* req, void* context);
	void http_callback(evhttp_request* req);

protected:
	void onLogin(CMarkup& xml);
	void onDevice(CMarkup& xml);
	void onEnumFailed(const xtstring& message);

protected:
	bool unzip(char* data, int size, std::string& result);
	void enumOrgan(CMarkup& xml);

protected:
	enum WorkStage{Inited, Login, EnumDevice, EnumOver};

protected:
	struct CameraVic {
		CameraVic(const xtstring& n) : name(n), offline(false) {}
		xtstring name;
		bool offline;
	};
	
private:
	xtstring m_host;
	int m_port;
	xtstring m_user;
	xtstring m_pwd;
	xtstring m_name;

	event_base* m_base;
	WorkStage m_stage;
	CSmartHdr<evhttp_connection*, void> m_connection;

	std::map<xtstring, std::shared_ptr<CameraVic>> m_cameras;
};

