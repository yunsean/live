#include "Writelog.h"
#include "LogWriter.h"
#include <errno.h>
#include <string.h>
#include "xsystem.h"

CLogRoller::CLogRoller(const int nLevel, LPCXSTR strModule, const unsigned int dwErrCode /* = 0 */, const unsigned int dwThreadId /* = 0 */)
	: m_content(NULL)
	, m_length(0)
	, m_discard(nLevel < CLogWriter::Singleton().GetLogLevel())
	, m_linehome(true)
	, m_level(nLevel)
	, m_errcode(dwErrCode)
	, m_threadid(dwThreadId > 0 ? dwThreadId : std::tid())
	, m_module(NULL)
{	
	size_t				len(_tcslen(strModule) + 1);
	m_module			= new TCHAR[len];
	_tcscpy_s(m_module, len, strModule);
}

CLogRoller::~CLogRoller()
{
	if (m_content && !m_discard && _tcslen(m_content) > 0)
	{
		size_t			len(_tcslen(m_content));
		if (len < 2 || m_content[len - 2] != '\r' || m_content[len - 1] != '\n')
			_tcscat_s(m_content, m_length - len, _T("\r\n"));
		CLogWriter::Singleton().WriteLine(m_content);
	}
	delete m_content;
	delete m_module;
}

#ifdef _WIN32
xtstring GetLocalTime(const bool bCompact = false)
{
	SYSTEMTIME			st;
	GetLocalTime(&st);
	xtstring				strTime;
	if (bCompact)strTime.Format(_T("%04d%02d%02d%02d%02d%02d"), st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	else strTime.Format(_T("%04d-%02d-%02d %02d:%02d:%02d.%03d"), st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	return strTime;
}
#else
#include <time.h>
xtstring GetLocalTime(const bool bCompact = false)
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

void CLogRoller::AppendText(LPCXSTR fmt, ...)
{
	va_list				args;
	xtstring				temp;
	va_start(args, fmt);
	temp.AppendFormatV(fmt, args);
	va_end(args);
	size_t				len(m_content ? _tcslen(m_content) : 0);
	if (m_content == NULL || len + temp.GetLength() > m_length)
	{
		size_t			nl(len + temp.GetLength());
		nl				+= 1024 - (nl % 1024);
		LPXSTR			ns(new(std::nothrow) TCHAR[nl]);
		if (ns == NULL)return;
		memset(ns, 0, nl);
		if (m_content)_tcscpy_s(ns, nl, m_content);
		m_content		= ns;
		m_length		= nl;
	}
	_tcscat_s(m_content, m_length - len, temp.GetString());
}

void CLogRoller::AddPrefix()
{
	xtstring				time(GetLocalTime());
	AppendText(_T("##%s [%s~%d] [%d %d] "), time.GetString(), m_module, m_threadid, m_level, m_errcode);
	m_linehome			= false;
}

CLogRoller& CLogRoller::operator <<(const bool value)
{
	if (!m_discard && m_linehome)AddPrefix();
	if (!m_discard)AppendText(_T("%s"), value ? _T("true") : _T("false"));
	return *this;
}

CLogRoller& CLogRoller::operator <<(const char value)
{
	if (!m_discard && m_linehome)AddPrefix();
	if (!m_discard)AppendText(_T("%C"), value);
	return *this;
}

CLogRoller& CLogRoller::operator <<(const wchar_t value)
{
	if (!m_discard && m_linehome)AddPrefix();
	if (!m_discard)AppendText(_T("%c"), value);
	return *this;
}

CLogRoller& CLogRoller::operator <<(const unsigned char value)
{
	if (!m_discard && m_linehome)AddPrefix();
	if (!m_discard)AppendText(_T("0x%02x"), value);
	return *this;
}

CLogRoller& CLogRoller::operator <<(const short value)
{
	if (!m_discard && m_linehome)AddPrefix();
	if (!m_discard)AppendText(_T("%d"), (int)value);
	return *this;
}

CLogRoller& CLogRoller::operator <<(const int value)
{
	if (!m_discard && m_linehome)AddPrefix();
	if (!m_discard)AppendText(_T("%d"), value);
	return *this;
}

CLogRoller& CLogRoller::operator <<(const unsigned int value)
{
	if (!m_discard && m_linehome)AddPrefix();
	if (!m_discard)AppendText(_T("%u"), value);
	return *this;
}

CLogRoller& CLogRoller::operator <<(const long value)
{
	if (!m_discard && m_linehome)AddPrefix();
	if (!m_discard)AppendText(_T("%d"), value);
	return *this;
}

CLogRoller& CLogRoller::operator <<(const unsigned long value)
{
	if (!m_discard && m_linehome)AddPrefix();
	if (!m_discard)AppendText(_T("%u"), value);
	return *this;
}

CLogRoller& CLogRoller::operator <<(const long long value)
{
	if (!m_discard && m_linehome)AddPrefix();
	if (!m_discard)AppendText(_T("%I64d"), value);
	return *this;
}

CLogRoller& CLogRoller::operator <<(const unsigned long long value)
{
	if (!m_discard && m_linehome)AddPrefix();
	if (!m_discard)AppendText(_T("%I64u"), value);
	return *this;
}

CLogRoller& CLogRoller::operator <<(const float value)
{
	if (!m_discard && m_linehome)AddPrefix();
	if (!m_discard)AppendText(_T("%f"), value);
	return *this;
}

CLogRoller& CLogRoller::operator <<(const double value)
{
	if (!m_discard && m_linehome)AddPrefix();
	if (!m_discard)AppendText(_T("%f"), value);
	return *this;
}

CLogRoller& CLogRoller::operator <<(const char* value)
{
	if (!m_discard && m_linehome)AddPrefix();
#ifdef UNICODE
	if (!m_discard)AppendText(_T("%S"), value);
#else 
	if (!m_discard)AppendText(_T("%s"), value);
#endif
	return *this;
}

CLogRoller& CLogRoller::operator <<(const wchar_t* value)
{
	if (!m_discard && m_linehome)AddPrefix();
#ifdef UNICODE
	if (!m_discard)AppendText(_T("%s"), value);
#else 
	if (!m_discard)AppendText(_T("%S"), value);
#endif
	return *this;
}

CLogRoller& CLogRoller::operator <<(void (*value)(CLogRoller *))
{
	if (m_discard)return *this;
	if (value == endl)
	{
		AppendText(_T("\r\n"));
		m_linehome	= true;
	}
	else if (value == flush)
	{
		AppendText(_T("\r\n"));
		m_linehome	= true;
		CLogWriter::Singleton().WriteLine(m_content);
		if (m_content)m_content[0]	= '\0';
	}
	return *this;
}

void endl(CLogRoller*){}
void flush(CLogRoller*){}


//////////////////////////////////////////////////////////////////////////
//For C#
void SetLogName(const char* lpszName)
{
	CLogWriter::Singleton().SetLogName(lpszName);
}

void SetLogName(const wchar_t* lpszName)
{
	CLogWriter::Singleton().SetLogName(lpszName);
}

void SetLogLevel(const int nLevel)
{
	CLogWriter::Singleton().SetLogLevel(nLevel);
}

void WriteLog(const int nLevel, LPCXSTR lpszModule, const char* lpszFmt, ...)
{
	try
	{
		xstring<char>		strLog;
		va_list					args;

		va_start(args, lpszFmt);
		strLog.FormatV(lpszFmt, args);
		va_end(args);
#ifdef UNICODE
		CLogWriter::Singleton().WriteLog(nLevel, lpszModule, std::tid(), std::lastError(), strLog.GetString());
#else 
		CLogWriter::Singleton().WriteLog(nLevel, lpszModule, std::tid(), std::lastError(), strLog);
#endif
	}
	catch (...)
	{
	}
}
void WriteLog(const int nLevel, LPCXSTR lpszModule, const wchar_t* lpszFmt, ...)
{
	try
	{
		xstring<wchar_t>	strLog;
		va_list					args;

		va_start(args, lpszFmt);
		strLog.FormatV(lpszFmt, args);
		va_end(args);
#ifdef UNICODE
		CLogWriter::Singleton().WriteLog(nLevel, lpszModule, std::tid(), std::lastError(), strLog);
#else 
		CLogWriter::Singleton().WriteLog(nLevel, lpszModule, std::tid(), std::lastError(), strLog.GetString());
#endif
	}
	catch (...)
	{
	}
}

void WriteLog(const int nLevel, const unsigned int dwThreadId, const unsigned int dwErrCode, LPCXSTR lpszModule, const char* lpszFmt, ...)
{
	try
	{
		xstring<char>		strLog;
		va_list					args;

		va_start(args, lpszFmt);
		strLog.FormatV(lpszFmt, args);
		va_end(args);
#ifdef UNICODE
		CLogWriter::Singleton().WriteLog(nLevel, lpszModule, dwThreadId, dwErrCode, strLog.GetString());
#else 
		CLogWriter::Singleton().WriteLog(nLevel, lpszModule, dwThreadId, dwErrCode, strLog);
#endif
	}
	catch (...)
	{
	}
}

void WriteLog(const int nLevel, const unsigned int dwThreadId, const unsigned int dwErrCode, LPCXSTR lpszModule, const wchar_t* lpszFmt, ...)
{
	try
	{
		xstring<wchar_t>	strLog;
		va_list					args;

		va_start(args, lpszFmt);
		strLog.FormatV(lpszFmt, args);
		va_end(args);
#ifdef UNICODE
		CLogWriter::Singleton().WriteLog(nLevel, lpszModule, dwThreadId, dwErrCode, strLog);
#else 
		CLogWriter::Singleton().WriteLog(nLevel, lpszModule, dwThreadId, dwErrCode, strLog.GetString());
#endif
	}
	catch (...)
	{
	}
}

void WriteLog(const int nLevel, const unsigned int dwThreadId, const unsigned int dwErrCode, LPCXSTR lpszModule, const char* lpszFmt, va_list vl)
{
	try
	{
		xstring<char>		strLog;
		strLog.FormatV(lpszFmt, vl);
#ifdef UNICODE
		CLogWriter::Singleton().WriteLog(nLevel, lpszModule, dwThreadId, dwErrCode, strLog.GetString());
#else 
		CLogWriter::Singleton().WriteLog(nLevel, lpszModule, dwThreadId, dwErrCode, strLog);
#endif
	}
	catch (...)
	{
	}
}

void WriteLog(const int nLevel, const unsigned int dwThreadId, const unsigned int dwErrCode, LPCXSTR lpszModule, const wchar_t* lpszFmt, va_list vl)
{
	try
	{
		xstring<wchar_t>	strLog;
		strLog.FormatV(lpszFmt, vl);
#ifdef UNICODE
		CLogWriter::Singleton().WriteLog(nLevel, lpszModule, dwThreadId, dwErrCode, strLog);
#else 
		CLogWriter::Singleton().WriteLog(nLevel, lpszModule, dwThreadId, dwErrCode, strLog.GetString());
#endif
	}
	catch (...)
	{
	}
}

void WriteLogT(const int nLevel, LPCXSTR lpszModule, const char* lpszFmt, va_list vl)
{
	WriteLog(nLevel, std::tid(), std::lastError(), lpszModule, lpszFmt, vl);
}

void WriteLogT(const int nLevel, LPCXSTR lpszModule, const wchar_t* lpszFmt, va_list vl)
{
	WriteLog(nLevel, std::tid(), std::lastError(), lpszModule, lpszFmt, vl);
}

void WriteDbg(const LPCXSTR lpszModule, const char* lpszFmt, ...)
{
	try
	{
		xstring<char>		strLog;
		va_list					args;

		va_start(args, lpszFmt);
		strLog.FormatV(lpszFmt, args);
		va_end(args);
#ifdef UNICODE
		CLogWriter::Singleton().WriteDbg(lpszModule, std::tid(), std::lastError(), strLog.GetString());
#else 
		CLogWriter::Singleton().WriteDbg(lpszModule, std::tid(), std::lastError(), strLog);
#endif
	}
	catch (...)
	{
	}
}

void WriteDbg(const LPCXSTR lpszModule, const wchar_t* lpszFmt, ...)
{
	try
	{
		xstring<wchar_t>	strLog;
		va_list					args;

		va_start(args, lpszFmt);
		strLog.FormatV(lpszFmt, args);
		va_end(args);
#ifdef UNICODE
		CLogWriter::Singleton().WriteDbg(lpszModule, std::tid(), std::lastError(), strLog);
#else 
		CLogWriter::Singleton().WriteDbg(lpszModule, std::tid(), std::lastError(), strLog.GetString());
#endif
	}
	catch (...)
	{
	}
}

void WriteDbg(const unsigned int dwThreadId, const unsigned int dwErrCode, const LPCXSTR lpszModule, const char* lpszFmt, ...)
{
	try
	{
		xstring<char>		strLog;
		va_list					args;

		va_start(args, lpszFmt);
		strLog.FormatV(lpszFmt, args);
		va_end(args);
#ifdef UNICODE
		CLogWriter::Singleton().WriteDbg(lpszModule, dwThreadId, dwErrCode, strLog.GetString());
#else 
		CLogWriter::Singleton().WriteDbg(lpszModule, dwThreadId, dwErrCode, strLog);
#endif
	}
	catch (...)
	{
	}
}

void WriteDbg(const unsigned int dwThreadId, const unsigned int dwErrCode, const LPCXSTR lpszModule, const wchar_t* lpszFmt, ...)
{
	try
	{
		xstring<wchar_t>	strLog;
		va_list					args;

		va_start(args, lpszFmt);
		strLog.FormatV(lpszFmt, args);
		va_end(args);
#ifdef UNICODE
		CLogWriter::Singleton().WriteDbg(lpszModule, dwThreadId, dwErrCode, strLog);
#else 
		CLogWriter::Singleton().WriteDbg(lpszModule, dwThreadId, dwErrCode, strLog.GetString());
#endif
	}
	catch (...)
	{
	}
}

void WriteDbgT(LPCXSTR lpszModule, const char* lpszFmt, va_list vl)
{
	try
	{
		xstring<char>		strLog;
		strLog.FormatV(lpszFmt, vl);
#ifdef UNICODE
		CLogWriter::Singleton().WriteDbg(lpszModule, std::tid(), std::lastError(), strLog.GetString());
#else 
		CLogWriter::Singleton().WriteDbg(lpszModule, std::tid(), std::lastError(), strLog);
#endif
	}
	catch (...)
	{
	}
}

void WriteDbgT(LPCXSTR lpszModule, const wchar_t* lpszFmt, va_list vl)
{
	try
	{
		xstring<wchar_t>	strLog;
		strLog.FormatV(lpszFmt, vl);
#ifdef UNICODE
		CLogWriter::Singleton().WriteDbg(lpszModule, std::tid(), std::lastError(), strLog);
#else 
		CLogWriter::Singleton().WriteDbg(lpszModule, std::tid(), std::lastError(), strLog.GetString());
#endif
	}
	catch (...)
	{
	}
}

int GetLevel(const char* lpszLevelName)
{
	if (strcmp(lpszLevelName, "LevelAll") == 0)
		return LevelAll;
	else if (strcmp(lpszLevelName, "LevelFatal") == 0)
		return LevelFatal;
	else if (strcmp(lpszLevelName, "LevelError") == 0)
		return LevelError;
	else if (strcmp(lpszLevelName, "LevelWarning") == 0)
		return LevelWarning;
	else if (strcmp(lpszLevelName, "LevelInfo") == 0)
		return LevelInfo;
	else if (strcmp(lpszLevelName, "LevelDebug") == 0)
		return LevelDebug;
	else if (strcmp(lpszLevelName, "LevelNone") == 0)
		return LevelNone;
	else 
		return -1;
}

int GetLevel(const wchar_t* lpszLevelName)
{
	if (wcscmp(lpszLevelName, L"LevelAll") == 0)
		return LevelAll;
	else if (wcscmp(lpszLevelName, L"LevelFatal") == 0)
		return LevelFatal;
	else if (wcscmp(lpszLevelName, L"LevelError") == 0)
		return LevelError;
	else if (wcscmp(lpszLevelName, L"LevelWarning") == 0)
		return LevelWarning;
	else if (wcscmp(lpszLevelName, L"LevelInfo") == 0)
		return LevelInfo;
	else if (wcscmp(lpszLevelName, L"LevelDebug") == 0)
		return LevelDebug;
	else if (wcscmp(lpszLevelName, L"LevelNone") == 0)
		return LevelNone;
	else 
		return -1;
}

//////////////////////////////////////////////////////////////////////////
//Variable
const int LevelAll		= LogLevel_All;
const int LevelFatal	= LogLevel_Fatal;
const int LevelError	= LogLevel_Error;
const int LevelWarning	= LogLevel_Warning;
const int LevelInfo		= LogLevel_Info;
const int LevelDebug	= LogLevel_Debug;
const int LevelNone		= LogLevel_None;

