#pragma once
#include <map>
#include <vector>
#include <mutex>
#include <functional>
#include "SmartPtr.h"
#include "SmartHdr.h"
#include "xstring.h"
#include "CodecInterface.h"

namespace Json {
	class Value;
}
class CDecodeFactory {
public:
	CDecodeFactory();
	~CDecodeFactory();

public:
	static CDecodeFactory& singleton();

public:
	bool initialize(bool usePlugin = true);
	IDecodeProxy* createDecoder(IDecodeProxyCallback* callback, const uint8_t* const header, const int length);
	void statusInfo(Json::Value& info);
	void uninitialize();

protected:
	bool initPlugin();

protected:
	void loadPath(LPCTSTR path);
	void loadFile(LPCTSTR file);

protected:
	typedef CSmartHdr<HMODULE>			CModulePtr;
	typedef CSmartPtr<IDecodeFactory>	IFactoryPtr;

private:
	std::vector<HMODULE>		m_vModules;
	std::recursive_mutex        m_lkFactories;
	std::vector<IFactoryPtr>	m_vFactories;
};

