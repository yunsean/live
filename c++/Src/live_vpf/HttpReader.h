#pragma once
#include "event2/event.h"
#include "event2/http.h"
#include "event2/http_struct.h"
#include "IOAccess.h"
#include "SmartBlk.h"
#include "xstring.h"
#include "SmartErr.h"

class CHttpReader : public IIOReader, public CSmartErr {
public:
	CHttpReader(event_base* base);
	~CHttpReader(void);

protected:
	//IIOReader
	bool		Open(LPCTSTR strFile);
	bool		CanSeek() { return true; }
	bool		Read(void* cache, const int limit, int& read);
	bool		Seek(const int64_t offset);
	int64_t		Offset();
	bool		Eof();
	void		Close();

protected:
	virtual void OnSetError();
	virtual LPCTSTR	GetLastErrorDesc() const { return m_strDesc; }
	virtual unsigned int GetLastErrorCode() const { return m_dwCode; }

protected:
	bool openRequest(const int64_t offset);
	static void request_callback(struct evhttp_request* request, void* context);
	void request_callback(struct evhttp_request* request);
	void closeRequest();

protected:
	struct CHttpBlock : public CTypicalBlock {
		int ReadData(void* const lpCache, const int szWant);
	};

private:
	xtstring m_url;
	event_base* m_eventBase;
	evhttp_request* m_request;
	CSmartBlk<CHttpBlock> m_blocks;
	CHttpBlock* m_writing;
	CHttpBlock* m_reading;
	bool m_hasError;
	int64_t m_offset;
};

