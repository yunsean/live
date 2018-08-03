#pragma once
#include "SinkInterface.h"
#include "json/value.h"

class CSinkFactory : public ISinkFactoryCallback {
public:
	CSinkFactory();
	~CSinkFactory();

public:
	static CSinkFactory& singleton();

public:
	bool initialize();
	bool handleRequest(ISinkHttpRequest* request);
	void status(Json::Value& status);

protected:
	virtual event_base*			GetPreferBase();
	virtual event_base**		GetAllBase(int& count);
	virtual ISinkProxyCallback*	AddSink(ISinkProxy* proxy, LPCTSTR moniker, LPCTSTR device, LPCTSTR param = NULL);
	virtual ISinkProxyCallback*	AddSink(ISinkProxy* proxy, LPCTSTR moniker, LPCTSTR device, LPCTSTR param, uint64_t beginTime, uint64_t endTime = 0);
};

