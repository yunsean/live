#ifdef _WIN32
#include <atltime.h>
#endif
#include "LocalLogWriter.h"
#include <string.h>
#include "SmartHdr.h"

#ifndef _WIN32
#define GetCurrentThreadId()	(unsigned int)pthread_self()
#endif

//////////////////////////////////////////////////////////////////////////
//CLocalLogWriter
CLocalLogWriter::CLocalLogWriter(const xtstring& strName)
	: m_lock()
#ifdef _WIN32
	, m_file(INVALID_HANDLE_VALUE)
#else 
	, m_file(NULL)
#endif
	, m_name(strName) {
}
CLocalLogWriter::~CLocalLogWriter() {
}

CLocalLogWriter& CLocalLogWriter::Singleton() {
	static CLocalLogWriter	instance;
	return instance;
}
CLocalLogWriter* CLocalLogWriter::Instance(const xtstring& strName) {
	return new CLocalLogWriter(strName);
}
void CLocalLogWriter::WriteLog(const int nLevel, LPCTSTR lpszFmt, ...) {
	try {
		xtstring strLog;
		va_list args;
		va_start(args, lpszFmt);
		strLog.FormatV(lpszFmt, args);
		va_end(args);
		WriteLog(nLevel, GetCurrentThreadId(), 0, strLog);
	} catch (...) {
	}
}

void CLocalLogWriter::WriteLog(const int nLevel, const unsigned int dwErrCode, LPCTSTR lpszFmt, ...) {
	try	{
		xtstring strLog;
		va_list args;
		va_start(args, lpszFmt);
		strLog.FormatV(lpszFmt, args);
		va_end(args);
		WriteLog(nLevel, GetCurrentThreadId(), dwErrCode, strLog);
	} catch (...) {
	}
}

#ifdef _AFXDLL
void CLocalLogWriter::WriteLog(const int nLevel, const CException* lpException) {
	try	{
		TCHAR			szMsg[1024];
		lpException->GetErrorMessage(szMsg, 1024);
		WriteLog(nLevel, GetCurrentThreadId(), 0, szMsg);
	} catch (...) {
	}
}
#endif

bool CLocalLogWriter::GetFileSize(long long& llSize) {
	if (m_file == NULL)return false;
#ifdef _WIN32
	LARGE_INTEGER		li;
	if (!GetFileSizeEx(m_file, &li))return false;
	llSize				= li.QuadPart;
	return true;
#else 
	long				curpos(0); 
	long				length(0);
	curpos				= ftell(m_file);
	if(fseek(m_file, 0, SEEK_END) == -1)return false;
	length				= ftell(m_file);
	if(fseek(m_file, curpos, SEEK_SET) == -1)return false;
	llSize				= length;
	return true;
#endif
}
void CLocalLogWriter::WriteData(const void* const lpData, const int nSize) {
	if (m_file == NULL)return;
#ifdef _WIN32
	DWORD				dwWritten(0);
	::WriteFile(m_file, lpData, nSize, &dwWritten, NULL);
	::FlushFileBuffers(m_file);
#else 
	fwrite(lpData, 1, nSize, m_file);
	fflush(m_file);
#endif
}
bool CLocalLogWriter::CreateLogPath(xtstring& strName) {
#ifdef _WIN32
	TCHAR				szModule[_MAX_PATH];
	xtstring				strModule;
	::GetModuleFileName(NULL, szModule, _MAX_PATH);
	strModule			= szModule;
	strModule			= strModule.Left(strModule.ReverseFind(TEXT('\\')));
	strModule			+= L"\\log\\";
	_tmkdir(strModule);
	strName				= strModule;
	return true;
#else 
	char				tmpPath[1024];
	getcwd(tmpPath, sizeof(tmpPath));
	strcat(tmpPath, "/log/");
	mkdir(tmpPath, 0777);
	strName				= tmpPath;
	return true;
#endif
}
bool CLocalLogWriter::GetModuleName(xtstring& strName) {
#ifdef _WIN32
	TCHAR				szModule[_MAX_PATH];
	::GetModuleFileName(NULL, szModule, _MAX_PATH);
	strName				= szModule;
	strName				= strName.Mid(strName.ReverseFind(TEXT('\\')) + 1);
	if (strName.ReverseFind(TEXT('.')) > 0)
		strName			= strName.Left(strName.ReverseFind(TEXT('.')));
	return true;
#else
	size_t				linksize = 1024;
	char				exeName[1024] = {0};
	if(readlink("/proc/self/exe", exeName, linksize) ==-1)return false;
	strName				= exeName;
	strName				= strName.Mid(strName.ReverseFind(L'/') + 1);
	if (strName.ReverseFind(L'.') > 0)
		strName			= strName.Left(strName.ReverseFind(L'.'));
	return true;
#endif
}
#ifdef _WIN32
xtstring CLocalLogWriter::GetLocalTime(const bool bCompact /* = false */) {
	SYSTEMTIME			st;
	::GetLocalTime(&st);
	xtstring				strTime;
	if (bCompact)strTime.Format(_T("%04d%02d%02d%02d%02d%02d"), st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	else strTime.Format(_T("%04d-%02d-%02d %02d:%02d:%02d.%03d"), st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	return strTime;
}
#else
#include <time.h>
xtstring CLocalLogWriter::GetLocalTime(const bool bCompact /* = false */) {
	struct tm*			pst(NULL);
	time_t				t(time(NULL));
	pst					= localtime(&t);
	if (pst == NULL)return L"";
	xtstring				strTime;
	if (bCompact)strTime.Format(_T("%04d%02d%02d%02d%02d%02d"), pst->tm_year + 1900, pst->tm_mon, pst->tm_mday, pst->tm_hour, pst->tm_min, pst->tm_sec);
	else strTime.Format(_T("%04d-%02d-%02d %02d:%02d:%02d"), pst->tm_year + 1900, pst->tm_mon, pst->tm_mday, pst->tm_hour, pst->tm_min, pst->tm_sec);
	return strTime;
}
#endif

void CLocalLogWriter::WriteLog(const int nLevel, const unsigned int dwThreadId, const unsigned int dwErrCode, LPCTSTR lpszFmt, ...) {
	m_lock.lock();
	try	{
		long long			size;
		if (!GetFileSize(size)) {
			if (!OpenFile()) {
				m_lock.unlock();
				return;
			}
		} else if (size > 10 * 1024 * 1024) {
			if (!AgeFile()) {
				m_lock.unlock();
				return;	
			}
		}

		xtstring msg;
		xtstring strLog;
		va_list args;
		xtstring time(GetLocalTime());
		va_start(args, lpszFmt);
		strLog.FormatV(lpszFmt, args);
		va_end(args);
		msg.Format(_T("##%s [LogService~%d] [%d %d] %s\r\n"), time.GetString(), dwThreadId, nLevel, dwErrCode, strLog.GetString());
		WriteData(msg.GetString(), msg.GetLength() * sizeof(TCHAR));
	} catch (...) {
	}
	m_lock.unlock();
}
void CLocalLogWriter::WriteLine(LPCTSTR strMsg) {
	m_lock.lock();
	try	{
		long long			size;
		if (!GetFileSize(size)) {
			if (!OpenFile()) {
				m_lock.unlock();
				return;
			}
		} else if (size > 10 * 1024 * 1024) {
			if (!AgeFile()) {
				m_lock.unlock();
				return;
			};
		}
		WriteData(strMsg, (unsigned int)_tcslen(strMsg) * (unsigned int)sizeof(TCHAR));
	} catch (...) {
	}
	m_lock.unlock();
}
bool CLocalLogWriter::AgeFile() {
#ifdef _WIN32
	if (m_file != INVALID_HANDLE_VALUE)::CloseHandle(m_file);
	m_file				= INVALID_HANDLE_VALUE;
#else
	if (m_file)::fclose(m_file);
	m_file				= NULL;
#endif
	return OpenFile(true);
}

xtstring CLocalLogWriter::GetLatestFile(xtstring strPath, xtstring strPrefix) {
#ifdef _WIN32
	if (strPath.Right(1) != _T("\\"))
		strPath += _T("\\");
	xtstring strFind(strPath + strPrefix + L"*.log");
	WIN32_FIND_DATA wfd;
	memset(&wfd, 0, sizeof(wfd));
	xtstring strFile;
	CSmartHdr<HANDLE> shFind(::FindFirstFile(strFind, &wfd), ::FindClose, INVALID_HANDLE_VALUE);
	if (shFind != INVALID_HANDLE_VALUE)	{
		CTime tmWrite(0);
		do  {
			if (wfd.nFileSizeLow > 10 * 1000 * 1000)continue;
			CTime			tm(wfd.ftLastWriteTime);
			if (tm < tmWrite)continue;
			strFile			= strPath + wfd.cFileName;
			tmWrite			= tm;
		} while (::FindNextFile(shFind, &wfd));
	}
	return strFile;
#else
	if (strPath.Right(1) != _T("/"))
		strPath += _T("/");
	CSmartHdr<DIR*, int> dir(opendir(strPath.GetString()), closedir);
	if (dir.GetHdr() == NULL)return _T("");
	dirent* ptr(NULL);
	int len(_tcslen(strPrefix.GetString()));
	time_t tmWrite(0);
	xtstring strFile;
	while((ptr = readdir(dir)) != NULL)	{
		if (_tcscmp(ptr->d_name, ".") == 0 || _tcscmp(ptr->d_name,"..") == 0)continue;
		if (_tcsncmp(ptr->d_name, strPrefix.GetString(), len) != 0)continue;
		xtstring path(strPath + ptr->d_name);
		struct stat info;
		if (stat(path.GetString(), &info) == -1)continue;
		if (info.st_mtime < tmWrite)continue;
		strFile = path;
	}
	return strFile;
#endif
}

bool CLocalLogWriter::OpenFile(const bool bForceCreate /* = false */) {
	xtstring strModule;
	if (!CreateLogPath(strModule))return false;
	if (m_name.IsEmpty() && !GetModuleName(m_name))return false;
	if (!bForceCreate) {
		xtstring strFile(GetLatestFile(strModule, m_name));
		if (!strFile.IsEmpty()) {
#ifdef _WIN32
			m_file = CreateFile(strFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (m_file != INVALID_HANDLE_VALUE) {
				SetFilePointer(m_file, 0, NULL, SEEK_END);
				return true;
			}
#else 
			m_file = fopen(strFile.GetString(), "ab+");
			if (m_file != NULL) {
				return true;
			}
#endif
		}
	}

	xtstring strTime(GetLocalTime(true));
	strModule.AppendFormat(_T("%s_%s.log"), m_name.GetString(), strTime.GetString());
#ifdef _WIN32
	m_file = CreateFile(strModule, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (m_file == INVALID_HANDLE_VALUE)return false;
	static const WCHAR	cUnicode(0xfeff);
	WriteData(&cUnicode, sizeof(WCHAR));
#else
	m_file = fopen(strModule.GetString(), "wb+");
	if (m_file == NULL)return false;
#endif
	return true;
}
