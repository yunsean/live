#pragma once
#include "SourceInterface.h"
#include "xstring.h"

class CFileFactory : public ISourceFactory {
public:
	CFileFactory();
	~CFileFactory();

public:
	virtual LPCTSTR			FactoryName() const;
	virtual bool			Initialize(ISourceFactoryCallback* callback);
	virtual SupportState	DidSupport(LPCTSTR device, LPCTSTR moniker, LPCTSTR param);
	virtual ISourceProxy*	CreateLiveProxy(ISourceProxyCallback* callback, LPCTSTR device, LPCTSTR moniker, LPCTSTR param);
	virtual ISourceProxy*	CreatePastProxy(ISourceProxyCallback* callback, LPCTSTR device, LPCTSTR moniker, LPCTSTR param, uint64_t beginTime, uint64_t endTime = 0);
	virtual bool			GetSourceList(LPTSTR* xml, void(**free)(LPTSTR), bool onlineOnly, LPCTSTR device = NULL);
	virtual bool			GetStatusInfo(LPSTR* json, void(**free)(LPSTR));
	virtual void			Uninitialize();
	virtual void			Destroy();

private:
	const LPCTSTR m_lpszFactoryName;
	xtstring m_strRootPath;
};

