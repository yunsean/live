#pragma once
#include <map>
#include <vector>
#include <mutex>
#include <functional>
#include "SmartPtr.h"
#include "SmartHdr.h"
#include "xstring.h"
#include "SinkInterface.h"

namespace Json {
	class Value;
}
struct event_base;
class COutputFactory {
public:
	COutputFactory();
	~COutputFactory();

public:
	static COutputFactory& singleton();

public:
	bool initialize(ISinkFactoryCallback* callback, bool usePlugin = true);
	void statusInfo(Json::Value& info);
	void uninitialize();

protected:
	bool initPlugin(ISinkFactoryCallback* callback);

protected:
	void loadPath(LPCTSTR path, ISinkFactoryCallback* callback);
	void loadFile(LPCTSTR file, ISinkFactoryCallback* callback);

protected:
	typedef CSmartHdr<HMODULE>			CModulePtr;
	typedef CSmartPtr<ISinkFactory>		IFactoryPtr;

private:
	std::vector<HMODULE>		m_vModules;
	std::recursive_mutex        m_lkFactoried;
	std::vector<IFactoryPtr>	m_vFactories;
};

