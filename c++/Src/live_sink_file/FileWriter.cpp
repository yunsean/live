#include <memory>
#include "event2/event.h"
#include "event2/bufferevent.h"
#include "event2/buffer.h"
#include "event2/http.h" 
#include "event2/http_struct.h"
#include "FileWriter.h"
#include "FileFactory.h"
#include "ByteStream.h"
#include "AVCConfig.h"
#include "H264Utility.h"
#include "ConsoleUI.h"
#include "Writelog.h"
#include "xutils.h"

#define BLOCKSIZE	(64 * 1024)

CFileWriter::CFileWriter(const xtstring& moniker)
	: m_strMoniker(moniker)
	, m_bev(bufferevent_free, nullptr)
	, m_file(fclose, nullptr)
	, m_cache(new uint8_t[BLOCKSIZE]) {
	wli(_T("[%s] Build File Writer."), m_strMoniker.c_str());
}
CFileWriter::~CFileWriter() {
	wli(_T("[%s] Remove File Writer."), m_strMoniker.c_str());
}

bool CFileWriter::Open() {
	m_file = _tfopen(m_strMoniker.c_str(), _T("rb"));
	if (!m_file.isValid()) {
		return wlet(false, _T("Open file [%s] failed."), m_strMoniker.c_str());
	}
	return true;
}
bool CFileWriter::Startup(ISinkHttpRequest* request) {
	event_base* base(request->GetBase());
	evutil_socket_t fd(request->GetSocket(true));
	m_bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
	if (!m_bev.isValid()) {
		evutil_closesocket(fd);
		return wlet(false, _T("Create buffer event for client failed."));
	}
	
	fseek(m_file, 0L, SEEK_END);
	long size(ftell(m_file));
	fseek(m_file, 0L, SEEK_SET);
	xtstring mime(CFileFactory::singleton().getMime(m_strMoniker));
	std::string header(GetHttpSucceedResponse(request, mime));
	bufferevent_setcb(m_bev, NULL, on_write, on_error, this);
	if (bufferevent_enable(m_bev, EV_READ | EV_WRITE) != 0 || bufferevent_write(m_bev, header.c_str(), header.length()) != 0) {
		return wlet(false, _T("Create buffer event for client failed."));
	}
	return true;
}

std::string CFileWriter::GetHttpSucceedResponse(ISinkHttpRequest* request, const xtstring& contentType /* = _T("text/html") */) {
	xtstring strHeader;
	strHeader.Format(_T("%s 200 OK\r\n"), request->GetVersion());
	strHeader += _T("Server: simplehttp\r\n");
	strHeader += _T("Connection: close\r\n");
	strHeader += _T("Cache-Control: no-cache\r\n");
	if (!contentType.IsEmpty()) strHeader += "Content-Type: " + contentType + "\r\n";
	strHeader += _T("\r\n");
	return strHeader.toString().c_str();
}

void CFileWriter::on_error(struct bufferevent* bev, short what, void* context) {
	CFileWriter* thiz(reinterpret_cast<CFileWriter*>(context));
	thiz->on_error(bev, what);
}
void CFileWriter::on_write(struct bufferevent* bev, void* context) {
	CFileWriter* thiz(reinterpret_cast<CFileWriter*>(context));
	thiz->on_write(bev);
}
void CFileWriter::on_error(struct bufferevent* event, short what) {
	if (what & (BEV_EVENT_ERROR | BEV_EVENT_EOF | BEV_EVENT_TIMEOUT)) {
		wle(_T("[%s] on_error(%d)"), m_strMoniker.c_str(), what);
		CFileFactory::singleton().removeWriter(this);
	}
}
void CFileWriter::on_write(struct bufferevent* event) {
	if (feof(m_file) != 0) {
		CFileFactory::singleton().removeWriter(this);
	} else {
		size_t read(fread(m_cache.get(), 1, BLOCKSIZE, m_file));
		if (read == 0) {
			CFileFactory::singleton().removeWriter(this);
		} else {
			if (bufferevent_write(m_bev, m_cache.get(), read) != 0) {
				wle(_T("[%s] send data to buffer failed."), m_strMoniker.c_str());
				CFileFactory::singleton().removeWriter(this);
			}
		}
	}
}