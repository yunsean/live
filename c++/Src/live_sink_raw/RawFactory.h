#pragma once
#include <vector>
#include <mutex>
#include <atomic>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/http.h>
#include <event2/http_struct.h>
#include "SinkInterface.h"
#include "RawWriter.h"
#include "SmartHdr.h"
#include "SmartPtr.h"

class CNalFactory : public ISinkFactory {
public:
	CNalFactory();
	~CNalFactory();

public:
	static CNalFactory& singleton();

public:
	static void AddPlayingCount();
	static void DelPlayingCount();
	static void AddTotalDataSize(uint64_t received, uint64_t send);
	static void UpdateTitle();

public:
	bool bindWriter(CHttpClient* client, const xtstring& device, const xtstring& moniker, const xtstring& params);
	void unbindWriter(CFlvWriter* writer);

protected:
	virtual LPCTSTR FactoryName() const;
	virtual bool	Initialize(ISinkFactoryCallback* callback);
	virtual int		HandleSort() { return 0; }
	virtual bool	HandleRequest(ISinkHttpRequest* request);
	virtual bool	GetStatusInfo(LPSTR* json, void(**free)(LPSTR));
	virtual void	Uninitialize();
	virtual void	Destroy();

private:
	static void listen_callback(struct evconnlistener* conn, evutil_socket_t fd, struct sockaddr* addr, int addrlen, void* context);
	void listen_callback(struct evconnlistener* conn, evutil_socket_t fd, struct sockaddr* addr, int addrlen);

private:
	static unsigned int m_lLicenseCount;
	static std::atomic<unsigned int> m_lInvokeCount;
	static std::atomic<unsigned int> m_lPlayingCount;
	static std::atomic<uint64_t> m_lTotalReceived;
	static std::atomic<uint64_t> m_lTotalSend;

protected:
	typedef CSmartPtr<CFlvWriter> CWriterPtr;

private:
	const LPCTSTR m_lpszFactoryName;
	ISinkFactoryCallback* m_lpCallback;
	std::vector<CSmartHdr<evconnlistener*, void>> m_listeners;

	std::recursive_mutex m_lkWriters;
	std::map<xtstring, CWriterPtr> m_liveWriter;
};

