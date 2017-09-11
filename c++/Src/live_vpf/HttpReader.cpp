#include "event2/buffer.h"
#include "HttpReader.h"
#include "Writelog.h"
#include "xaid.h"

#define BLOCK_SIZE	(1024 * 2)

//////////////////////////////////////////////////////////////////////////
int CHttpReader::CHttpBlock::ReadData(void* const lpCache, const int szWant) {
	if (saCache.GetArr() == NULL)return 0;
	if (szData <= 0)return 0;
	if (szWant <= 0)return 0;
	int nRead(std::min(szWant, szData));
	if (nRead > 0)memcpy(lpCache, saCache, nRead);
	szData -= nRead;
	if (nRead > 0 && szData > 0)memcpy(saCache, saCache + nRead, szData);
	return nRead;
}

//////////////////////////////////////////////////////////////////////////
CHttpReader::CHttpReader(event_base* base)
	: m_url()
	, m_eventBase()
	, m_request()
	, m_blocks()
	, m_writing(NULL)
	, m_reading(NULL)
	, m_hasError(false)
	, m_offset(0) {
}
CHttpReader::~CHttpReader() {
	closeRequest();
}

//////////////////////////////////////////////////////////////////////////
void CHttpReader::OnSetError() {
	wle(m_strDesc);
}

bool CHttpReader::Open(LPCTSTR strFile) {
	m_url = strFile;
	return openRequest(0);
}
bool CHttpReader::Read(void* cache, const int limit, int& read) {
	if (m_hasError)return false;
	read = 0;
	while (!m_hasError) {
		if (m_reading && m_reading->GetSize() <= 0) {
			m_blocks.ReturnEmptyBlock(m_reading);
			m_reading = NULL;
		}
		if (m_reading == NULL && !m_blocks.GetValidBlock(m_reading, INFINITE)) {
			if (m_blocks.IsFinished()) {
				break;
			}
			else {
				return SetErrorT(false, _T("Read data failed."));
			}
		}
		if (!m_reading) {
			return false;
		}
		read += m_reading->ReadData(reinterpret_cast<char*>(cache) + read, limit - read);
		cache = reinterpret_cast<char*>(cache) + read;
		if (read >= limit)break;
	}
	if (m_hasError)return false;
	m_offset += read;
	return true;
}
bool CHttpReader::Seek(const int64_t offset) {
	closeRequest();
	return openRequest(offset);
}
int64_t CHttpReader::Offset() {
	return m_offset;
}
bool CHttpReader::Eof() {
	return false;
}
void CHttpReader::Close() {
	closeRequest();
}

bool CHttpReader::openRequest(const int64_t offset) {
	const char* url = m_url.toString().c_str();
	CSmartHdr<evhttp_uri*, void> uri(evhttp_uri_parse_with_flags(url, EVHTTP_URI_NONCONFORMANT), evhttp_uri_free);
	if (uri == nullptr) {
		return SetErrorT(false, _T("Parse url [%s] failed."), xtstring(url).c_str());
	}
	evhttp_request* request = evhttp_request_new(&CHttpReader::request_callback, this);
	if (uri == nullptr) {
		return SetErrorT(false, _T("Create http request failed."));
	}
	auto finalize(std::destruct_invoke([request]() {
		evhttp_request_free(request);
	}));
	const char* host = evhttp_uri_get_host(uri);
	int port = evhttp_uri_get_port(uri);
	const char* path = evhttp_uri_get_path(uri);
	const char* query = evhttp_uri_get_query(uri);
	if (port == -1) port = 80;
	m_blocks.Reset();
	evhttp_connection* conn = evhttp_connection_base_new(m_eventBase, NULL, host, port);
	if (conn == nullptr) {
		return SetErrorT(false, _T("Connect to [%s:%d] failed for url."), host, port, xtstring(url).c_str());
	}
	std::string query_path = std::string(path != nullptr ? path : "");
	if (!query_path.empty()) {
		if (query != nullptr) {
			query_path += "?";
			query_path += query;
		}
	}
	else {
		query_path = "/";
	}
	int result = evhttp_make_request(conn, request, EVHTTP_REQ_GET, query_path.c_str());
	if (result != 0) {
		return SetErrorT(nullptr, _T("Request http[%s] failed."), xtstring(url).c_str());
	}
	xstring<char> host_port(xstring<char>::format("%s:%d", host, port));
	evhttp_add_header(request->output_headers, "Host", host_port);
	if (offset != 0) {
		xtstring range;
		range.Format(_T("bytes=%lld-"), offset);
		evhttp_add_header(request->output_headers, "Host", host_port);
	}
	finalize.cancel();
	m_request = request;
	m_offset = offset;
	return true;
}
void CHttpReader::closeRequest() {
	m_blocks.Finished();
	if (m_request != nullptr) {
		evhttp_cancel_request(m_request);
		m_request = nullptr;
	}
}

void CHttpReader::request_callback(struct evhttp_request* request, void* context) {
	CHttpReader* thiz(reinterpret_cast<CHttpReader*>(context));
	thiz->request_callback(request);
}
void CHttpReader::request_callback(struct evhttp_request* request) {
	while (true) {
		if (m_writing && m_writing->GetSize() >= BLOCK_SIZE) {
			m_blocks.ReturnValidBlock(m_writing);
			m_writing = nullptr;
		}
		if (m_writing == nullptr) {
			if (!m_blocks.GetEmptyBlock(m_writing, INFINITE))return;
			if (!m_writing->EnsureBlock(BLOCK_SIZE)) {
				m_hasError = true;
				return;
			}
		}
		char* data = reinterpret_cast<char*>(m_writing->GetData());
		int read = evbuffer_remove(request->input_buffer, data + m_writing->GetSize(), 1024 * 2);
		if (read > 0) {
			m_writing->GrowSize(read);
		}
	}
}