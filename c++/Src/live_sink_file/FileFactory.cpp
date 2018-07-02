#include <iostream>  
#include <fstream> 
#include <event2/event.h>
#include <event2/thread.h>
#include <event2/event_struct.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/http.h>
#include <event2/listener.h>
#include "net.h"
#include "FileFactory.h"
#include "Profile.h"
#include "Writelog.h"
#include "ConsoleUI.h"
#include "json/value.h"
#include "xaid.h"
#include "file.h"

unsigned int CFileFactory::m_lLicenseCount(0);
std::atomic<unsigned int> CFileFactory::m_lInvokeCount(0);
std::atomic<unsigned int> CFileFactory::m_lPlayingCount(0);
std::atomic<uint64_t> CFileFactory::m_lTotalReceived(0);
std::atomic<uint64_t> CFileFactory::m_lTotalSend(0);
CFileFactory::CFileFactory()
	: m_lpszFactoryName(_T("Flv Sender"))
	, m_mappers()
	, m_writers() {
}
CFileFactory::~CFileFactory() {
}

CFileFactory& CFileFactory::singleton() {
	static CFileFactory instance;
	return instance;
}

LPCTSTR CFileFactory::FactoryName() const {
	return m_lpszFactoryName;
}
bool CFileFactory::Initialize(ISinkFactoryCallback* callback) {
#ifdef _WIN32
	if (evthread_use_windows_threads() != 0) {
#else 
	if (evthread_use_pthreads() != 0) {
#endif
		return wlet(false, _T("init multiple thread failed."));
	}
	CProfile profile(CProfile::Default());
	int nCount(profile(_T("File"), _T("MapperCount"), _T("-1")));
	if (nCount < 0) {
		profile(_T("File"), _T("MapperCount")) = _T("0");
		profile(_T("File"), _T("Mapper0")) = _T("/vod:/tmp/vod");
		nCount = 0;
	}
	if (nCount < 1) {
		cpw(_T("没有配置任何路径映射..."));
	} else {
		for (int iServer = 0; iServer < nCount; iServer++) {
			xtstring strTmp;
			strTmp.Format(_T("Mapper%d"), iServer);
			xtstring strValue((LPCTSTR)profile(_T("File"), strTmp));
			int pos(strValue.Find(_T(":")));
			if (pos < 0) continue;
			xtstring remote(strValue.Left(pos).Trim());
			xtstring local(strValue.Mid(pos + 1).Trim());
			if (remote.IsEmpty() || local.IsEmpty()) continue;
			m_mappers.insert(std::make_pair(remote, local));
		}
		cpj(_T("加载路劲映射配置完成"));
	}
	xtstring files = (LPCTSTR)profile(_T("File"), _T("Default Files"));
	while (!files.IsEmpty()) {
		int pos(files.Find(_T(" ")));
		if (pos < 0) {
			m_defaultFile.push_back(files);
			break;
		} else {
			m_defaultFile.push_back(files.Left(pos));
			files = files.Mid(pos + 1);
		}
	} 
	if (m_defaultFile.empty()) {
		m_defaultFile.push_back(_T("index.html"));
	}
	loadMimes();
	return true;
}
bool CFileFactory::HandleRequest(ISinkHttpRequest* request) {
	xtstring path(request->GetPath());
	for (auto it = m_mappers.begin(); it != m_mappers.end(); it++) {
		const xtstring& remote(it->first);
		if (_tcsncmp(path.c_str(), remote.c_str(), remote.GetLength()) == 0) {
			xtstring file = it->second;
			file += path.Mid(remote.GetLength());
			if (path.CompareNoCase(remote) == 0) {
				auto found(std::find_if(m_defaultFile.begin(), m_defaultFile.end(), [&file](const xtstring& d) {
					xtstring path(file + d);
					if (_taccess(path.c_str(), 0) == 0) return true;
					else return false;
				}));
				if (found == m_defaultFile.end()) continue;
				else file = file + *found;
			}
			CSmartPtr<CFileWriter> sp(new CFileWriter(file));
			if (!sp->Open()) continue;
			if (sp->Startup(request)) {
				std::lock_guard<std::mutex> lock(m_mutex);
				m_writers.push_back(sp);
			}
			return true;
		}
	}
	return false;
}
bool CFileFactory::GetStatusInfo(LPSTR* json, void(**free)(LPSTR)) {
	*json = nullptr;
	return true;
}
void CFileFactory::Uninitialize() {
}
void CFileFactory::Destroy() {
}

void CFileFactory::loadMimes() {
	xstring<char> file;
	file.Format(_T("%smimes.txt"), file::parentDirectory(file::exeuteFilePath()).c_str());
	std::ifstream ifs(file.c_str(), std::ios_base::in);
	if (!ifs) return wle(_T("load mimes file failed."));
	char buf[1024];
	while (!ifs.eof()) {
		ifs.getline(buf, sizeof(buf));
		xtstring line(buf);
		int pos(line.Find(_T(":")));
		if (pos < 0) continue;
		xtstring mime(line.Left(pos).Trim());
		xtstring types(line.Mid(pos + 1).Trim());
		if (mime.IsEmpty() || types.IsEmpty()) continue;
		while (!types.IsEmpty()) {
			pos = types.Find(_T(" "));
			if (pos < 0) {
				m_mimes.insert(std::make_pair(types, mime));
				break;
			} else {
				m_mimes.insert(std::make_pair(types.Left(pos), mime));
				types = types.Mid(pos + 1);
			}
		}
	}
}
xtstring CFileFactory::getMime(const xtstring& file) {
	int pos(file.ReverseFind(_T('.')));
	if (pos < 0) return _T("application/octet-stream");
	xtstring type(file.Mid(pos + 1));
	auto found(m_mimes.find(type));
	if (found == m_mimes.end()) return _T("application/octet-stream");
	else return found->second;
}

void CFileFactory::removeWriter(CFileWriter* writer) {
	std::lock_guard<std::mutex> lock(m_mutex);
	auto found(std::find_if(m_writers.begin(), m_writers.end(), [writer](CWriterPtr sp) {
		return sp.GetPtr() == writer;
	}));
	if (found != m_writers.end()) {
		m_writers.erase(found);
	}
}
