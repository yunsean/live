#include "OutputFactory.h"
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

COutputFactory::COutputFactory()
	: m_vModules()
	, m_lkFactoried() 
	, m_vFactories() {
}
COutputFactory::~COutputFactory() {
}

COutputFactory& COutputFactory::singleton() {
	static COutputFactory    instance;
	return instance;
}
bool COutputFactory::initialize(ISinkFactoryCallback* callback, bool usePlugin /* = true */) {
	if (usePlugin && !initPlugin(callback))return false;
	return true;
}
void COutputFactory::statusInfo(Json::Value& info) {
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
bool COutputFactory::initPlugin(ISinkFactoryCallback* callback) {
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

void COutputFactory::loadPath(LPCTSTR path, ISinkFactoryCallback* callback) {
	xtstring pluginPath(path);
	pluginPath.Append(_T("*.vplugin.dll"));
	file::findFiles(pluginPath, [this, callback](LPCTSTR file, bool isDirectory) {
		if (!isDirectory) {
			loadFile(file, callback);
		}
	}, false);
}

#ifdef _WIN32
void COutputFactory::loadFile(LPCTSTR file, ISinkFactoryCallback* callback) {
	typedef ISinkFactory* (*GetSinkFactory)();
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
		return wle(L"LoadLibrary(%s) failed.", path.c_str());
	}
	GetSinkFactory fnGetPlugInFun((GetSinkFactory)::GetProcAddress(hInstance, "GetSinkFactory"));
	if (fnGetPlugInFun == NULL)return;
	IFactoryPtr spPlugin(fnGetPlugInFun(), &ISinkFactory::Destroy);
	if (spPlugin == NULL) {
		return wle(L"GetSignalFactory(%s) failed.", path.c_str());
	}
	LPCTSTR lpszName(spPlugin->FactoryName());
	if (!spPlugin->Initialize(callback)) {
		cpe(_T("加载输出插件[%s]失败"), lpszName);
		return wle(L"Initialize() from [%s] failed.", path.c_str());
	}
	cpj(_T("加载输出插件[%s]成功"), lpszName);
	m_vModules.push_back(hInstance);
	m_vFactories.push_back(spPlugin);
}
#else 
void COutputFactory::loadFile(LPCTSTR file, ISinkFactoryCallback* callback) {
	typedef ISinkFactory* (*GetSinkFactory)();
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
	GetSinkFactory getPlugInFun((GetSinkFactory)::dlsym(module, "_Z14GetSinkFactoryv"));
	if (getPlugInFun == NULL)return;
	IFactoryPtr plugin(getPlugInFun(), &ISinkFactory::Destroy);
	if (plugin == NULL) {
		return wle(_T("GetSignalFactory(%s) failed."), file);
	}
	LPCTSTR name(plugin->FactoryName());
	if (!plugin->Initialize(callback)) {
		cpe(_T("加载输出插件[%s]失败"), name);
		return wle(_T("Initialize() from [%s] failed."), file);
	}
	cpj(_T("加载输出插件[%s]成功"), name);
	m_vModules.push_back(module);
	m_vFactories.push_back(plugin);
}
#endif

void COutputFactory::uninitialize() {
	std::lock_guard<std::recursive_mutex> lock(m_lkFactoried);
	std::for_each(m_vFactories.begin(), m_vFactories.end(),
		[](ISinkFactory* sp) {
		sp->Uninitialize();
	});
	m_vFactories.clear();
}