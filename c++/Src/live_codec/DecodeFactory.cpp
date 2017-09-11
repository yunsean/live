#include "DecodeFactory.h"
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
#define PLUGIN_PATTERN	_T("Plugin\\sink\\*.*")
#else 
#define PLUGIN_PATTERN	_T("Plugin/sink/*.*")
#endif

CDecodeFactory::CDecodeFactory()
	: m_vModules()
	, m_lkFactories()
	, m_vFactories() {
}
CDecodeFactory::~CDecodeFactory() {
}

CDecodeFactory& CDecodeFactory::singleton() {
	static CDecodeFactory    instance;
	return instance;
}
bool CDecodeFactory::initialize(bool usePlugin /* = true */) {
	if (usePlugin && !initPlugin())return false;
	return true;
}
IDecodeProxy* CDecodeFactory::createDecoder(IDecodeProxyCallback* callback, const uint8_t* const header, const int length) {
	IDecodeProxy* lpProxy(NULL);
	std::lock_guard<std::recursive_mutex> lock(m_lkFactories);
	auto found(std::find_if(m_vFactories.begin(), m_vFactories.end(),
		[&](IFactoryPtr sp) ->bool {
		return sp->DidSupport(header, length);
	}));
	if (found != m_vFactories.end()) {
		lpProxy = (*found)->CreateDecoder(callback, header, length);
	}
	return lpProxy;
}
void CDecodeFactory::statusInfo(Json::Value& info) {
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
bool CDecodeFactory::initPlugin() {
	xtstring exePath(file::exeuteFilePath());
	xtstring pluginPath(exePath + PLUGIN_PATTERN);
	if (!file::findFiles(pluginPath, [this](LPCTSTR file, bool isDirectory) {
		if (isDirectory) {
			loadPath(file);
		}
		else {
			loadFile(file);
		}
	}, false)) {
		return wlet(false, _T("find plugin failed!"));
	}
	return true;
}

void CDecodeFactory::loadPath(LPCTSTR path) {
	xtstring pluginPath(path);
	pluginPath.Append(_T("*.vplugin.dll"));
	file::findFiles(pluginPath, [this](LPCTSTR file, bool isDirectory) {
		if (!isDirectory) {
			loadFile(file);
		}
	}, false);
}

#ifdef _WIN32
void CDecodeFactory::loadFile(LPCTSTR file) {
	typedef IDecodeFactory* (*GetDecodeFactory)();
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
	GetDecodeFactory fnGetPlugInFun((GetDecodeFactory)::GetProcAddress(hInstance, "GetDecodeFactory"));
	if (fnGetPlugInFun == NULL)return;
	IFactoryPtr spPlugin(fnGetPlugInFun(), &IDecodeFactory::Destroy);
	if (spPlugin == NULL) {
		return wle(_T("GetSignalFactory(%s) failed."), path.c_str());
	}
	LPCTSTR lpszName(spPlugin->FactoryName());
	if (!spPlugin->Initialize()) {
		cpe(_T("Load codec plugin[%s] failed"), lpszName);
		return wle(_T("Initialize() from [%s] failed."), path.c_str());
	}
	cpj(_T("Load codec plugin[%s] ok"), lpszName);
	m_vModules.push_back(hInstance);
	m_vFactories.push_back(spPlugin);
}
#else 
void CDecodeFactory::loadFile(LPCTSTR file) {
	typedef IDecodeFactory* (*GetDecodeFactory)();
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
	}
	catch (...) {
		return wle(_T("LoadLibrary(%s) failed. error=%s"), file, dlerror());
	}
	if (module == NULL) {
		return wle(_T("LoadLibrary(%s) failed, error=%s"), file, dlerror());
	}
	GetDecodeFactory getPlugInFun((GetDecodeFactory)::dlsym(module, "_Z14GetDecodeFactoryv"));
	if (getPlugInFun == NULL)return;
	IFactoryPtr plugin(getPlugInFun(), &IDecodeFactory::Destroy);
	if (plugin == NULL) {
		return wle(_T("GetSignalFactory(%s) failed."), file);
	}
	LPCTSTR name(plugin->FactoryName());
	if (!plugin->Initialize()) {
		cpe(_T("加载解码插件[%s]失败"), name);
		return wle(_T("Initialize() from [%s] failed."), file);
	}
	cpj(_T("加载解码插件[%s]成功"), name);
	m_vModules.push_back(module);
	m_vFactories.push_back(plugin);
}
#endif

void CDecodeFactory::uninitialize() {
	std::lock_guard<std::recursive_mutex> lock(m_lkFactories);
	std::for_each(m_vFactories.begin(), m_vFactories.end(),
		[](IDecodeFactory* sp) {
		sp->Uninitialize();
	});
	m_vFactories.clear();
}