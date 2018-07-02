#pragma once
#include <vector>
#include <mutex>
#include <atomic>
#include <list>
#include <mutex>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/http.h>
#include <event2/http_struct.h>
#include "SinkInterface.h"
#include "FileWriter.h"
#include "SmartHdr.h"
#include "SmartPtr.h"

class CFileFactory : public ISinkFactory {
public:
	CFileFactory();
	~CFileFactory();

public:
	static CFileFactory& singleton();

public:
	void removeWriter(CFileWriter* writer);
	xtstring getMime(const xtstring& file);

protected:
	virtual LPCTSTR FactoryName() const;
	virtual bool	Initialize(ISinkFactoryCallback* callback);
	virtual int		HandleSort() { return 100; }
	virtual bool	HandleRequest(ISinkHttpRequest* request);
	virtual bool	GetStatusInfo(LPSTR* json, void(**free)(LPSTR));
	virtual void	Uninitialize();
	virtual void	Destroy();

private:
	void loadMimes();

private:
	static unsigned int m_lLicenseCount;
	static std::atomic<unsigned int> m_lInvokeCount;
	static std::atomic<unsigned int> m_lPlayingCount;
	static std::atomic<uint64_t> m_lTotalReceived;
	static std::atomic<uint64_t> m_lTotalSend;

protected:
	typedef CSmartPtr<CFileWriter> CWriterPtr;

private:
	const LPCTSTR m_lpszFactoryName;
	std::map<xtstring, xtstring> m_mappers;
	std::map<xtstring, xtstring> m_mimes;
	std::mutex m_mutex;
	std::list<CWriterPtr> m_writers;
	std::list<xtstring> m_defaultFile;
};

