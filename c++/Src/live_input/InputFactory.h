#pragma once
#include <map>
#include <vector>
#include <mutex>
#include <functional>
#include "SmartPtr.h"
#include "SmartHdr.h"
#include "xstring.h"
#include "SourceInterface.h"

namespace Json {
	class Value;
}
struct event_base;
class CInputFactory {
public:
	CInputFactory();
	~CInputFactory();

public:
	static CInputFactory& singleton();

public:
	bool initialize(ISourceFactoryCallback* callback, bool usePlugin = true);
	ISourceProxy* createLiveSource(ISourceProxyCallback* callback, LPCTSTR device, LPCTSTR moniker, LPCTSTR param);
	ISourceProxy* createPastSource(ISourceProxyCallback* callback, LPCTSTR device, LPCTSTR moniker, LPCTSTR param, uint64_t beginTime, uint64_t endTime = 0);
	bool streamList(xtstring& xml, const xtstring& device, bool onlineOnly);
	void statusInfo(Json::Value& value);
	void uninitialize();

protected:
	bool initPlugin(ISourceFactoryCallback* callback);

protected:
	void loadPath(LPCTSTR path, ISourceFactoryCallback* callback);
	void loadFile(LPCTSTR file, ISourceFactoryCallback* callback);

protected:
	typedef CSmartHdr<HMODULE>				CModulePtr;
	typedef CSmartPtr<ISourceFactory>		IFactoryPtr;

private:
	std::vector<HMODULE>		m_vModules;
	std::recursive_mutex        m_lkFactories;
	std::vector<IFactoryPtr>	m_vFactories;

	std::vector<xtstring>		m_vForwardHost;
	int							m_nLatestHost;
};

