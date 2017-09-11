#include "InputFactory.h"
#include "file.h"
#include "Writelog.h"
#include "ConsoleUI.h"
#include "Profile.h"
#include "json/value.h"
#include "json/reader.h"
#ifdef _WIN32
#else
#include <dlfcn.h>
#endif

#ifdef _WIN32
#define PLUGIN_PATTERN	_T("Plugin\\source\\*.*")
#else 
#define PLUGIN_PATTERN	_T("Plugin/source/*.*")
#endif

CInputFactory::CInputFactory()
	: m_vModules()
	, m_lkFactories()
	, m_vFactories()

	, m_vForwardHost()
	, m_nLatestHost(0) {
}
CInputFactory::~CInputFactory() {
}


CInputFactory& CInputFactory::singleton() {
	static CInputFactory    instance;
	return instance;
}
bool CInputFactory::initialize(ISourceFactoryCallback* callback, bool usePlugin /* = true */) {
	if (usePlugin && !initPlugin(callback))return false;
	return true;
}
bool CInputFactory::initPlugin(ISourceFactoryCallback* callback) {
	xtstring exePath(file::exeuteFilePath());
	xtstring pluginPath(exePath + PLUGIN_PATTERN);
	if (!file::findFiles(pluginPath, [this, callback](LPCTSTR file, bool isDirectory) {
		if (isDirectory) {
			loadPath(file, callback);
		}
		else {
			loadFile(file, callback);
		}
	}, false)) {
		return wlet(false, _T("find plugin failed!"));
	}
	if (m_vFactories.size() < 1)return false;
	return true;
}

void CInputFactory::loadPath(LPCTSTR path, ISourceFactoryCallback* callback) {
	xtstring pluginPath(path);
	pluginPath.Append(_T("*.vplugin.dll"));
	file::findFiles(pluginPath, [this, callback](LPCTSTR file, bool isDirectory) {
		if (!isDirectory) {
			loadFile(file, callback);
		}
	}, false);
}

#ifdef _WIN32
void CInputFactory::loadFile(LPCTSTR file, ISourceFactoryCallback* callback) {
	typedef ISourceFactory* (*GetSourceFactory)();
	xtstring path(file);
	int pos(path.Find(_T(".")));
	if (pos < 0) return;
	xtstring extName(path.Mid(pos + 1));
	extName.MakeLower();
	if (extName != _T("dll") && extName != _T("ocx"))return;
	wli(_T("Find plugin: %s"), path.c_str());

	xtstring parent(file::parentDirectory(path));
	::SetCurrentDirectory(parent);
	HMODULE hInstance(LoadLibrary(path));
	if (hInstance == NULL) {
		return wle(_T("LoadLibrary(%s) failed."), path.c_str());
	}
	GetSourceFactory fnGetPlugInFun((GetSourceFactory)::GetProcAddress(hInstance, "GetSourceFactory"));
	if (fnGetPlugInFun == NULL)return;
	IFactoryPtr spPlugin(fnGetPlugInFun(), &ISourceFactory::Destroy);
	if (spPlugin == NULL) {
		return wle(_T("GetSignalFactory(%s) failed."), path.c_str());
	}
	LPCTSTR lpszName(spPlugin->FactoryName());
	if (!spPlugin->Initialize(callback)) {
		cpe(_T("Load source plugin[%s] failed"), lpszName);
		return wle(_T("Initialize() from [%s] failed."), path.c_str());
	}
	cpj(_T("Load source plugin[%s] ok"), lpszName);
	m_vModules.push_back(hInstance);
	m_vFactories.push_back(spPlugin);
}
#else 
void CInputFactory::loadFile(LPCTSTR file, ISourceFactoryCallback* callback) {
	typedef ISourceFactory* (*GetSourceFactory)();
	xtstring path(file);
	int pos(path.Find(_T(".")));
	if (pos < 0) return;
	xtstring extName(path.Mid(pos + 1));
	extName.MakeLower();
	if (extName != _T("so"))return;
	wli(_T("Find plugin: %s"), path.c_str());

	HMODULE module(NULL);
	try {
		module = dlopen(file, RTLD_NOW);
	} catch (...) {
		return wle(_T("LoadLibrary(%s) failed. error=%s"), file, dlerror());
	}
	if (module == NULL) {
		return wle(_T("LoadLibrary(%s) failed, error=%s"), file, dlerror());
	}
	GetSourceFactory getPlugInFun((GetSourceFactory)::dlsym(module, "_Z16GetSourceFactoryv"));
	if (getPlugInFun == NULL)return;
	IFactoryPtr plugin(getPlugInFun(), &ISourceFactory::Destroy);
	if (plugin == NULL) {
		return wle(_T("GetSignalFactory(%s) failed."), file);
	}
	LPCTSTR name(plugin->FactoryName());
	if (!plugin->Initialize(callback)) {
		cpe(_T("加载输入插件[%s]失败"), lpszName);
		return wle(_T("Initialize() from [%s] failed."), file);
	}
	cpj(_T("加载输入插件[%s]成功"), name);
	m_vModules.push_back(module);
	m_vFactories.push_back(plugin);
}
#endif

//////////////////////////////////////////////////////////////////////////
ISourceProxy* CInputFactory::createLiveSource(ISourceProxyCallback* callback, LPCTSTR device, LPCTSTR moniker, LPCTSTR param) {
	ISourceProxy* lpProxy(NULL);
	std::lock_guard<std::recursive_mutex> lock(m_lkFactories);
	auto found(std::find_if(m_vFactories.begin(), m_vFactories.end(),
		[&](IFactoryPtr sp) ->bool {
		ISourceFactory::SupportState state(sp->DidSupport(device, moniker, param));
		if (state == ISourceFactory::supported)return true;
		return false;
	}));
	if (found != m_vFactories.end()) {
		lpProxy = (*found)->CreateLiveProxy(callback, device, moniker, param);
	}
	if (lpProxy == NULL) {
		std::find_if(m_vFactories.begin(), m_vFactories.end(),
			[&](ISourceFactory* sp) -> bool {
			ISourceFactory::SupportState	state(sp->DidSupport(device, moniker, param));
			if (state == ISourceFactory::unsupport)return false;
			lpProxy = sp->CreateLiveProxy(callback, device, moniker, param);
			return (lpProxy != NULL);
		});
	}
	return lpProxy;
}
ISourceProxy* CInputFactory::createPastSource(ISourceProxyCallback* callback, LPCTSTR device, LPCTSTR moniker, LPCTSTR param, uint64_t beginTime, uint64_t endTime /* = 0 */) {
	return nullptr;
}
bool CInputFactory::streamList(xtstring& xml, const xtstring& device, bool onlineOnly) {
	typedef void(*FnFreeXml)(LPTSTR);
	xml = _T("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n<signals>\r\n");
	std::for_each(m_vFactories.begin(), m_vFactories.end(),
		[&xml, onlineOnly, &device](ISourceFactory* sp) {
		LPTSTR lpsz(nullptr);
		FnFreeXml freeXml(nullptr);
		if (sp->GetSourceList(&lpsz, &freeXml, onlineOnly, device) && lpsz != nullptr) {
			xml += lpsz;
			if (freeXml != nullptr) freeXml(lpsz);
		}
	});
	xml += L"</signals>";
	return true;
}
void CInputFactory::statusInfo(Json::Value& info) {
	typedef void(*FnFreeJson)(LPSTR);
	for (auto sp : m_vFactories) {
		LPSTR lpsz(nullptr);
		FnFreeJson freeXml(nullptr);
		if (sp->GetStatusInfo(&lpsz, &freeXml) && lpsz != nullptr) {
			Json::Reader reader;
			Json::Value value;
			std::string json(lpsz);
			if (freeXml != nullptr) freeXml(lpsz);
			if (reader.parse(json, value)) {
				info.append(value);
			}
		}
	}
}

void CInputFactory::uninitialize() {
	std::lock_guard<std::recursive_mutex> lock(m_lkFactories);
	std::for_each(m_vFactories.begin(), m_vFactories.end(),
		[](ISourceFactory* sp) {
		sp->Uninitialize();
	});
	m_vFactories.clear();
}

