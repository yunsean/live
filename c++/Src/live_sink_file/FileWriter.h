#pragma once
#include <mutex>
#include <memory>
#include <event2/bufferevent.h>
#include "HttpClient.h"
#include "SinkInterface.h"
#include "H264Utility.h"
#include "json/value.h"
#include "xstring.h"
#include "Byte.h"
#include "xsystem.h"
#include "SmartHdr.h"

struct event;
struct event_base;
class CHttpClient;
class CFileWriter {
public:
	CFileWriter(const xtstring& path);
	~CFileWriter();

public:
	bool				Open();
	bool				Startup(ISinkHttpRequest* request);
	const xtstring&		GetMoniker() const { return m_strMoniker; }

protected:
	std::string			GetHttpSucceedResponse(ISinkHttpRequest* request, const xtstring& contentType = _T("text/html"));

protected:
	static void			on_write(struct bufferevent* event, void* context);
	static void			on_error(struct bufferevent* event, short what, void* context);
	void				on_write(struct bufferevent* event);
	void				on_error(struct bufferevent* event, short what);

private:
	const xtstring m_strMoniker;
	CSmartHdr<bufferevent*, void> m_bev;
	CSmartHdr<FILE*, int> m_file;
	std::unique_ptr<uint8_t[]> m_cache;
};

