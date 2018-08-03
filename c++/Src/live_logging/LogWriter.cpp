#include <time.h>
#include "LogWriter.h"
#include "Writelog.h"
#ifdef _WIN32
#include <atlbase.h>
#include <Tlhelp32.h>
#else 
#include <string.h>
#include <unistd.h>
#endif
#include "ModuleVersion.h"
#include "xcritic.h"

#ifndef _WIN32
#define GetCurrentThreadId()	(unsigned int)pthread_self()
#endif

//////////////////////////////////////////////////////////////////////////
//CLogWriter
CLogWriter::CLogWriter()
	: m_lock()
	, m_strName()
	, m_nLevel(LogLevel_Info)
	, m_cPipe()
	, m_spLocal(NULL)
	, m_bLocal(false)
{
#ifdef _WIN32
	try{
		CRegKey			key;
		if (key.Create(HKEY_CURRENT_USER, _T("Software\\Dyn\\Log")) == ERROR_SUCCESS)
		{
			DWORD		dwValue(0);
			if (key.QueryDWORDValue(_T("LogLevel"), dwValue) != ERROR_SUCCESS)
				key.SetDWORDValue(_T("LogLevel"), LogLevel_Info);
			else
				m_nLevel	= dwValue;
			if (key.QueryDWORDValue(_T("WriteLocal"), dwValue) != ERROR_SUCCESS)
				key.SetDWORDValue(_T("WriteLocal"), 0);
			else
				m_bLocal	= dwValue ? true : false;
			key.Close();
		}
		xtstring		strMsg;
		strMsg.Format(_T("LogClient: Create log instance, loglevel = %d, writelocal = %d\n"), m_nLevel, m_bLocal);
		OutputDebugString(strMsg.GetString());
	}
	catch(...){

	}
#endif
}

CLogWriter::~CLogWriter()
{
	m_cPipe.ClosePipe();
}

CLogWriter& CLogWriter::Singleton()
{
	static std::xcritic		lock;
	static CLogWriter*		instance(NULL);
	if (instance == NULL)
	{
		lock.lock();
		if (instance == NULL)
		{
			CLogWriter*		writer(new CLogWriter());
			writer->WriteEnv();
			instance		= writer;
		}
		lock.unlock();
	}
	return *instance;
}

void CLogWriter::WriteEnv()
{
	WriteLine(_T("\r\n\r\n--------------------------------------------------------\r\n"));
	
	xtstring					strModule;
	GetModuleName(strModule);
	xtstring					strLog;
	strLog.Format(_T("[%s] Startup."), strModule.GetString());
	WriteLog(LevelInfo, _T("Logger"), GetCurrentThreadId(), 0, strLog);
#ifdef _WIN32
	xtstring					strList(L"Loaded modules:\r\n");
	GetModuleList(strList);
	WriteLine(strList);
#endif
}

void CLogWriter::GetModuleList(xtstring& strList)
{
#ifdef _WIN32
	CSmartHdr<HANDLE>	shToolHelp32Snap(::CloseHandle, INVALID_HANDLE_VALUE);
	unsigned int				dwPID(GetCurrentProcessId());
	MODULEENTRY32		me32;
	me32.dwSize			= sizeof(MODULEENTRY32); 
	shToolHelp32Snap	= CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwPID);
	if (shToolHelp32Snap == INVALID_HANDLE_VALUE)return;
	if(!Module32First(shToolHelp32Snap, &me32))
	{
		WriteLog(LevelError, L"Logger",  GetCurrentThreadId(), GetLastError(), L"Module32First() failed.");
		return;
	}

	do 
	{ 
		CModuleVersion	version(me32.szExePath);
		strList.AppendFormat(L"[0x%08x - 0x%08x]%s (%s, %s)\r\n", me32.modBaseAddr, me32.modBaseAddr + me32.modBaseSize, me32.szExePath, version.GetProductVersion().c_str(), version.GetFileVersion().c_str());
		OutputDebugString(strList);
	}while(Module32Next(shToolHelp32Snap, &me32));
#endif
}

#ifdef _WIN32
xtstring CLogWriter::GetLocalTime(const bool bCompact /* = false */)
{
	SYSTEMTIME			st;
	::GetLocalTime(&st);
	xtstring				strTime;
	if (bCompact)strTime.Format(_T("%04d%02d%02d%02d%02d%02d"), st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	else strTime.Format(_T("%04d-%02d-%02d %02d:%02d:%02d.%03d"), st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	return strTime;
}
#else
#include <time.h>
xtstring CLogWriter::GetLocalTime(const bool bCompact /* = false */)
{
	struct tm*			pst(NULL);
	time_t				t(time(NULL));
	pst					= localtime(&t);
	if (pst == NULL)return L"";
	xtstring				strTime;
	if (bCompact)strTime.Format(_T("%04d%02d%02d%02d%02d%02d"), pst->tm_yday + 1900, pst->tm_mon, pst->tm_mday, pst->tm_hour, pst->tm_min, pst->tm_sec);
	else strTime.Format(_T("%04d-%02d-%02d %02d:%02d:%02d"), pst->tm_yday + 1900, pst->tm_mon, pst->tm_mday, pst->tm_hour, pst->tm_min, pst->tm_sec);
	return strTime;
}
#endif

void CLogWriter::WriteLog(const int nLevel, const xtstring& strModule, const unsigned int dwThreadId, const unsigned int dwErrCode, const xtstring& strLog)
{
	m_lock.lock();
	try{
		xtstring				time(GetLocalTime());
		xtstring				msg;
		if (dwErrCode != 0) {
			xtstring			error(::strerror(dwErrCode));
			msg.Format(_T("##%s [%s~%d] [%d %d(%s)] %s\r\n"), time.GetString(), strModule.GetString(), dwThreadId, nLevel, dwErrCode, error.c_str(), strLog.GetString());
		}
		else {
			msg.Format(_T("##%s [%s~%d] [%d %d] %s\r\n"), time.GetString(), strModule.GetString(), dwThreadId, nLevel, dwErrCode, strLog.GetString());
		}
		WriteLine(msg);
	}
	catch (...){
	}
	m_lock.unlock();
}

void CLogWriter::WriteDbg(const xtstring& strModule, const unsigned int dwThreadId, const unsigned int dwErrCode, const xtstring& strLog)
{
	try{
		xtstring				msg;
		msg.Format(_T("##dbg: [%s~%d :%d] %s\n"), strModule.GetString(), dwThreadId, dwErrCode, strLog.GetString());
#ifdef _WIN32
		OutputDebugString(msg);
#else 
		_tprintf("%s", msg.c_str());
#endif
	}
	catch (...){
	}
}

bool CLogWriter::GetModuleName(xtstring& strName)
{
#ifdef _WIN32
	TCHAR				szModule[_MAX_PATH];
	::GetModuleFileName(NULL, szModule, _MAX_PATH);
	strName				= szModule;
	strName				= strName.Mid(strName.ReverseFind(TEXT('\\')) + 1);
	strName				= strName.Left(strName.ReverseFind(TEXT('.')));
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

bool CLogWriter::WritePipe(LPCTSTR lpszMsg)
{
	enum BlockType{Create = 1, Content, Connected, Disconnected};
	if (m_cPipe.IsOpend())return m_cPipe.WriteString(lpszMsg);
	if (m_strName.IsEmpty() && !GetModuleName(m_strName))
		m_strName.Format(_T("#%d"), time(NULL));
	if(!m_cPipe.ConnectPipe())return false;
	xtstring				strName;
	strName.Format(_T("%c%s"), (char)Create, m_strName.GetString());
	if (!m_cPipe.WriteString(strName))
	{
		m_cPipe.ClosePipe();
		return false;
	}
	return m_cPipe.WriteString(lpszMsg);
}

void CLogWriter::WriteLocal(LPCTSTR lpszMsg)
{
	if (m_spLocal)return m_spLocal->WriteLine(lpszMsg);
	if (m_strName.IsEmpty() && !GetModuleName(m_strName))
	{
		m_strName.Format(_T("#%ld"), time(NULL));
	}
	m_spLocal			= CLocalLogWriter::Instance(m_strName + L"(Local)");
	m_spLocal->WriteLine(lpszMsg);
}

void CLogWriter::WriteLine(LPCTSTR strMsg)
{
	m_lock.lock();
	try{
		if(!WritePipe(strMsg) || m_bLocal)WriteLocal(strMsg);
	}
	catch (...){
	}
	m_lock.unlock();
}

void CLogWriter::SetLogName(const xtstring& strName)
{
	m_strName				= strName;
}

void CLogWriter::SetLogLevel(const int nLevel)
{
	m_nLevel				= nLevel;
}	

