#pragma once
#include <mutex>
#include <map>
#include <list>
#include <vector>
#include "SourceInterface.h"
#include "SmartPtr.h"
#include "json/value.h"

class CHttpClient;
struct event_base;
class CSourceFactory : public ISourceFactoryCallback {
public:
	CSourceFactory(void);
	virtual ~CSourceFactory(void);

public:
	static CSourceFactory& singleton();

public:
	bool initialize();
	ISourceProxy* createLiveSource(ISourceProxyCallback* callback, LPCTSTR device, LPCTSTR moniker, LPCTSTR param);
	ISourceProxy* createPastSource(ISourceProxyCallback* callback, LPCTSTR device, LPCTSTR moniker, LPCTSTR param, uint64_t beginTime, uint64_t endTime = 0);
	bool streamList(xtstring& xml, const xtstring& device, bool onlineOnly);
	void status(Json::Value& status);
	void uninitialize();

public:
	virtual event_base* GetPreferBase();
	virtual event_base** GetAllBase(int& count);
	virtual void OnNewSource(LPCTSTR lpszDevice, LPCTSTR lpszMoniker, LPCTSTR lpszName, LPCTSTR lpszMeta = NULL);
	virtual void OnSourceOver(LPCTSTR lpszDevice, LPCTSTR lpszMoniker);
};

