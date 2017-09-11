#pragma once
#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <mutex>
#include "SourceInterface.h"
#include "xstring.h"
#include "IOAccess.h"
#include "SmartPtr.h"
#include "SmartNal.h"
#include "SmartHdr.h"
#include "xthread.h"

class CFileSource : public ISourceProxy {
public:
	CFileSource(const xtstring& strMoniker, const xtstring& strParam);
	~CFileSource(void);

public:
	bool Open(ISourceProxyCallback* callback, const xtstring& strFile);

protected:
	//ISourceProxy
	virtual LPCTSTR SourceName() const { return m_moniker; }
	virtual bool StartFetch(event_base* base);
	virtual void WantKeyFrame();
	virtual bool PTZControl(const PTZAction eAction, const int nValue);
	virtual bool VideoEffect(const int nBright, const int nContrast, const int nSaturation, const int nHue);
	virtual bool Discard();

protected:
	static void ev_callback(evutil_socket_t fd, short flags, void* context);
	void ev_callback(evutil_socket_t fd, short flags);

protected:
	enum PackType { Header = 0, Video, Audio, Error, Custom };

private:
	xtstring m_moniker;
	CSmartPtr<IVpfReader> m_spReader;
	CSmartNal<uint8_t> m_snHeader;
	CSmartHdr<event*, void> m_timer;

	ISourceProxyCallback* m_lpCallback;	
	uint32_t m_uBeginTC;
	uint32_t m_uRefTime;
	bool m_bDiscard;
};

