#pragma once
#include "xcritic.h"
#include "xstring.h"

//////////////////////////////////////////////////////////////////////////
//»’÷æ–¥»Î
class CLogWriter;
class CLocalLogWriter
{
protected:
	CLocalLogWriter(const xtstring& strName = _T(""));
public:
	virtual ~CLocalLogWriter(void);

public:
	static CLocalLogWriter&	Singleton();
	static CLocalLogWriter*	Instance(const xtstring& strName);

public:
	void				WriteLog(const int nLevel, LPCTSTR lpszFmt, ...);
	void				WriteLog(const int nLevel, const unsigned int dwErrCode, LPCTSTR lpszFmt, ...);
#ifdef _AFXDLL
	void				WriteLog(const int nLevel, const CException* lpException);
#endif
	void				WriteLog(const int nLevel, const unsigned int dwThreadId, const unsigned int dwErrCode, LPCTSTR lpszFmt, ...);

protected:
	friend class CLogWriter;
	void				WriteLine(LPCTSTR lpszMsg);
	bool				OpenFile(const bool bForceCreate = false);
	bool				AgeFile();

protected:
	bool				GetFileSize(long long& llSize);
	void				WriteData(const void* const lpData, const int nSize);
	bool				CreateLogPath(xtstring& strName);
	bool				GetModuleName(xtstring& strName);
	xtstring		GetLocalTime(const bool bCompact = false);
	xtstring		GetLatestFile(xtstring strPath, xtstring strPrefix);

private:
	std::xcritic		m_lock;
#ifdef _WIN32
	HANDLE				m_file;
#else
	FILE*				m_file;
#endif
	xtstring		m_name;
};
