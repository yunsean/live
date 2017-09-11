#include <event2/event.h>
#include <event2/thread.h>
#include <event2/event_struct.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include "FileSource.h"
#include "FileFactory.h"
#include "ConsoleUI.h"
#include "Writelog.h"
#include "Profile.h"
#include "Markup.h"
#include "file.h"
#include "os.h"
#include "SmartPtr.h"

CFileFactory::CFileFactory()
	: m_lpszFactoryName(_T("File Input Source")){
}

CFileFactory::~CFileFactory() {
}

LPCTSTR CFileFactory::FactoryName() const {
	return m_lpszFactoryName;
}
bool CFileFactory::Initialize(ISourceFactoryCallback* callback) {
#ifdef _WIN32
	if (evthread_use_windows_threads() != 0) {
#else 
	if (evthread_use_pthreads() != 0) {
#endif
		return wlet(false, _T("init multiple thread failed."));
	}

	CProfile profile(CProfile::Default());
	m_strRootPath = (LPCTSTR)profile(_T("vpf"), _T("Root Path"), _T(""));
	m_strRootPath.Trim();
	if (m_strRootPath.IsEmpty())
		m_strRootPath = file::parentDirectory(file::exeuteFilePath());
#ifdef _WIN32
	if (m_strRootPath.Right(1) != _T("\\"))
		m_strRootPath += _T("\\");
#else 
	if (m_strRootPath.Right(1) != _T("/"))
		m_strRootPath += _T("/");
#endif
	cpw(_T("VPF根目录： %s"), m_strRootPath.c_str());

	return true;
}
ISourceFactory::SupportState CFileFactory::DidSupport(LPCTSTR device, LPCTSTR moniker, LPCTSTR param) {
	if (device != NULL && _tcslen(device) > 0 && _tcscmp(device, _T("vpf")) != 0) {
		return unsupport;
	}
	xtstring file(moniker);
	if (file.Right(4).CompareNoCase(_T(".vpf")) != 0)
		file += _T(".vpf");
	if (_taccess(file, 4) == -1) {
		return ISourceFactory::unsupport;
	}
	return maybesupport;
}
ISourceProxy* CFileFactory::CreateLiveProxy(ISourceProxyCallback* callback, LPCTSTR device, LPCTSTR moniker, LPCTSTR param) {
	xtstring file(moniker);
	xtstring vendor(device);
	if (!vendor.IsEmpty() && vendor.CompareNoCase(_T("vpf")) != 0) {
		return NULL;
	}
	if (file.Right(4).CompareNoCase(_T(".vpf")) != 0) {
		file += _T(".vpf");
	}
	if (_taccess(file, 4) == -1) {
		return wlet(nullptr, _T("File [%s] is not exist."), file.c_str());
	}
	CSmartPtr<CFileSource> spSource(new CFileSource(moniker, param));
	if (!spSource->Open(callback, file))return wlet(nullptr, _T("Open file[%s] failed."));
	return spSource.Detach();
}
ISourceProxy* CFileFactory::CreatePastProxy(ISourceProxyCallback* callback, LPCTSTR device, LPCTSTR moniker, LPCTSTR param, uint64_t beginTime, uint64_t endTime /* = 0 */) {
	return NULL;
}
bool CFileFactory::GetSourceList(LPTSTR* _xml, void(**free)(LPTSTR), bool onlineOnly, LPCTSTR device /* = NULL */) {
	if (!_xml)return false;
	if (device != NULL && _tcslen(device) > 0 && _tcsicmp(device, _T("vpf")) != 0)return false;
	xtstring strFind(m_strRootPath + _T("*.vpf"));
	CMarkup xml;
	xml.AddElem(_T("vpf"));
	xml.AddAttrib(_T("name"), _T("VPF文件"));
	xml.IntoElem();
	file::findFiles(strFind, [&xml](LPCTSTR file, bool) {
		xtstring name(file);
		int pos(name.ReverseFind(_T('.')));
		if (pos > 0) name = name.Left(pos);
		pos = name.ReverseFind(_T(PATH_SLASH));
		if (pos > 0) name = name.Mid(pos + 1);
		xml.AddElem(_T("signal"));
		xml.AddAttrib(_T("name"), name);
		xml.AddAttrib(_T("moniker"), file);
		xml.AddAttrib(_T("device"), _T("vpf"));
	});
	xtstring text(xml.GetDoc());
	const TCHAR* lpsz(text.c_str());
	const size_t len(text.length());
	TCHAR* result = new TCHAR[len + 1];
	_tcscpy(result, lpsz);
	*_xml = result;
	*free = [](LPTSTR ptr) {
		delete[] reinterpret_cast<TCHAR*>(ptr);
	};
	return true;
}
bool CFileFactory::GetStatusInfo(LPSTR* json, void(**free)(LPSTR)) {
	return false;
}
void CFileFactory::Uninitialize() {

}
void CFileFactory::Destroy() {
	delete this;
}
