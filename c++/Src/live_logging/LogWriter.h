#pragma once
#if defined(USE_SOCKET) || !defined(_WIN32)
#include "SockWriter.h"
#else 
#include "PipeWriter.h"
#endif
#include "LocalLogWriter.h"
#include "SmartPtr.h"
#include "xcritic.h"

#define LogLevel_All		0
#define LogLevel_Debug		1
#define LogLevel_Info		2
#define LogLevel_Warning	3
#define LogLevel_Error		4
#define LogLevel_Fatal		5
#define LogLevel_LoggerInfo	8
#define LogLevel_LoggerErr	9
#define LogLevel_None		9

//////////////////////////////////////////////////////////////////////////
//»’÷æ–¥»Î
class CLogWriter
{
protected:
	CLogWriter();
	virtual ~CLogWriter(void);

public:
	static CLogWriter&	Singleton();

public:
	void				SetLogName(const xtstring& strName);
	void				SetLogLevel(const int nLevel);

public:
	void				WriteEnv();
	void				WriteLog(const int nLevel, const xtstring& strModule, const unsigned int dwThreadId, const unsigned int dwErrCode, const xtstring& strLog);
	void				WriteDbg(const xtstring& strModule, const unsigned int dwThreadId, const unsigned int dwErrCode, const xtstring& strLog);

protected:
	friend class CLogRoller;
	xtstring		GetLogName() const{return m_strName;}
	int					GetLogLevel() const{return m_nLevel;}
	void				WriteLine(LPCTSTR lpszMsg);
	bool				WritePipe(LPCTSTR lpszMsg);
	void				WriteLocal(LPCTSTR lpszMsg);

protected:
	void				GetModuleList(xtstring& strList);
	xtstring		GetLocalTime(const bool bCompact = false);
	bool				GetModuleName(xtstring& strName);

protected:
	typedef CSmartPtr<CLocalLogWriter>	CLocalLogWriterPtr;

private:
	std::xcritic		m_lock;
	xtstring		m_strName;
	int					m_nLevel;
#if defined(USE_SOCKET) || !defined(_WIN32)
	CSockWriter			m_cPipe;
#else 
	CPipeWriter			m_cPipe;
#endif
	CLocalLogWriterPtr	m_spLocal;
	bool				m_bLocal;
};
