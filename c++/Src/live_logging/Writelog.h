#pragma once
#include <wchar.h>
#include <stdarg.h>

#ifdef _WIN32
	#ifdef LIVE_LOGGING_EXPORTS
		#define LOGGING_API __declspec(dllexport)
	#else
		#define LOGGING_API __declspec(dllimport)
	#endif
#else 
	#define LOGGING_API
#endif

#if defined(UNICODE) || defined(_UNICODE)
typedef wchar_t* LPXSTR;
typedef const wchar_t* LPCXSTR;
#else 
typedef char* LPXSTR;
typedef const char* LPCXSTR;
#endif


//////////////////////////////////////////////////////////////////////////
//日志序列化
class LOGGING_API CLogRoller
{
public:
	CLogRoller(const int nLevel, LPCXSTR lpszModule, const unsigned int dwErrCode = 0, const unsigned int dwThreadId = 0);
	virtual ~CLogRoller();

public:
	CLogRoller&		operator<<(const int value);
	CLogRoller&		operator<<(const char value);
	CLogRoller&		operator<<(const wchar_t value);
	CLogRoller&		operator<<(const unsigned char value);
	CLogRoller&		operator<<(const short value);
	CLogRoller&		operator<<(const bool value);
	CLogRoller&		operator<<(const unsigned int value);
	CLogRoller&		operator<<(const long value);
	CLogRoller&		operator<<(const unsigned long value);
	CLogRoller&		operator<<(const long long value);
	CLogRoller&		operator<<(const unsigned long long value);
	CLogRoller&		operator<<(const float value);
	CLogRoller&		operator<<(const double value);
	CLogRoller&		operator<<(const char* value);
	CLogRoller&		operator<<(const wchar_t* value);
	CLogRoller&		operator<<(void (*value)(CLogRoller*));

protected:
	void			AddPrefix();
	void			AppendText(LPCXSTR fmt, ...);

private:
	LPXSTR			m_content;
	size_t			m_length;
	int				m_discard;
	int				m_linehome;
	unsigned int	m_level;
	unsigned int	m_errcode;
	unsigned int	m_threadid;
	LPXSTR			m_module;
};
void LOGGING_API	endl(CLogRoller*);
void LOGGING_API	flush(CLogRoller*);


//////////////////////////////////////////////////////////////////////////
//Export Method
extern const int LOGGING_API	LevelAll;
extern const int LOGGING_API	LevelFatal;
extern const int LOGGING_API	LevelError;
extern const int LOGGING_API	LevelWarning;
extern const int LOGGING_API	LevelInfo;
extern const int LOGGING_API	LevelDebug;
extern const int LOGGING_API	LevelNone;

void LOGGING_API	SetLogName(const char* lpszName);
void LOGGING_API	SetLogName(const wchar_t* lpszName);
void LOGGING_API	SetLogLevel(const int nLevel);
int  LOGGING_API	GetLevel(const char* lpszLevelName);
int  LOGGING_API	GetLevel(const wchar_t* lpszLevelName);

void LOGGING_API	WriteLog(const int nLevel, LPCXSTR lpszModule, const char* lpszFmt, ...);
void LOGGING_API	WriteLog(const int nLevel, LPCXSTR lpszModule, const wchar_t* lpszFmt, ...);
void LOGGING_API	WriteLog(const int nLevel, const unsigned int dwThreadId, const unsigned int dwErrCode, LPCXSTR lpszModule, const char* lpszFmt, ...);
void LOGGING_API	WriteLog(const int nLevel, const unsigned int dwThreadId, const unsigned int dwErrCode, LPCXSTR lpszModule, const wchar_t* lpszFmt, ...);
void LOGGING_API	WriteLog(const int nLevel, const unsigned int dwThreadId, const unsigned int dwErrCode, LPCXSTR lpszModule, const char* lpszFmt, va_list vl);
void LOGGING_API	WriteLog(const int nLevel, const unsigned int dwThreadId, const unsigned int dwErrCode, LPCXSTR lpszModule, const wchar_t* lpszFmt, va_list vl);

void LOGGING_API	WriteDbg(LPCXSTR lpszModule, const char* lpszFmt, ...);
void LOGGING_API	WriteDbg(LPCXSTR lpszModule, const wchar_t* lpszFmt, ...);
void LOGGING_API	WriteDbg(const unsigned int dwThreadId, const unsigned int dwErrCode, LPCXSTR lpszModule, const char* lpszFmt, ...);
void LOGGING_API	WriteDbg(const unsigned int dwThreadId, const unsigned int dwErrCode, LPCXSTR lpszModule, const wchar_t* lpszFmt, ...);

//////////////////////////////////////////////////////////////////////////
//Template Method
void LOGGING_API	WriteLogT(const int nLevel, LPCXSTR lpszModule, const char* lpszFmt, va_list vl);
void LOGGING_API	WriteLogT(const int nLevel, LPCXSTR lpszModule, const wchar_t* lpszFmt, va_list vl);
template <typename T, typename C>
const T& WriteLogT(const T& retValue, const int nLevel, LPCXSTR lpszModule, C lpszFmt, ...)
{
	try{
		va_list		args;
		va_start(args, lpszFmt);
		WriteLogT(nLevel, lpszModule, lpszFmt, args);
		va_end(args);
	}catch (...){
	}
	return retValue;
}
void LOGGING_API	WriteDbgT(LPCXSTR lpszModule, const char* lpszFmt, va_list vl);
void LOGGING_API	WriteDbgT(LPCXSTR lpszModule, const wchar_t* lpszFmt, va_list vl);
template <typename T, typename C>
const T& WriteDbg(const T& retValue, LPCXSTR lpszModule, C lpszFmt, ...)
{
	try{
		va_list		args;
		va_start(args, lpszFmt);
		WriteDbgT(lpszModule, lpszFmt, args);
		va_end(args);
	}catch (...){
	}
	return retValue;
}

//////////////////////////////////////////////////////////////////////////
//Macro Method
#ifdef UNICODE
#ifndef __FUNCTIONW__
#define __STR2WSTR(str)		L##str
#define _STR2WSTR(str)		__STR2WSTR(str)
#define __FUNCTIONW__		_STR2WSTR(__FUNCTION__)
#endif
#define __FUNC__			__FUNCTIONW__
#else 
#define __FUNC__			__FUNCTION__
#endif

#ifdef _WIN32
#	define wle(lpszFmt, ...)							::WriteLog(LevelError, __FUNC__, lpszFmt, __VA_ARGS__) 
#	define wlf(lpszFmt, ...)							::WriteLog(LevelFatal, __FUNC__, lpszFmt, __VA_ARGS__) 
#	define wlw(lpszFmt, ...)							::WriteLog(LevelWarning, __FUNC__, lpszFmt, __VA_ARGS__) 
#	define wli(lpszFmt, ...)							::WriteLog(LevelInfo, __FUNC__, lpszFmt, __VA_ARGS__) 
#	define wld(lpszFmt, ...)							::WriteLog(LevelDebug, __FUNC__, lpszFmt, __VA_ARGS__) 
#	define dbg(lpszFmt, ...)							::WriteDbg(__FUNC__, lpszFmt, __VA_ARGS__) 

#	define wlet(retT, lpszFmt, ...)					::WriteLogT(retT, LevelError, __FUNC__, lpszFmt, __VA_ARGS__) 
#	define wlft(retT, lpszFmt, ...)					::WriteLogT(retT, LevelFatal, __FUNC__, lpszFmt, __VA_ARGS__) 
#	define wlwt(retT, lpszFmt, ...)					::WriteLogT(retT, LevelWarning, __FUNC__, lpszFmt, __VA_ARGS__) 
#	define wlit(retT, lpszFmt, ...)					::WriteLogT(retT, LevelInfo, __FUNC__, lpszFmt, __VA_ARGS__) 
#	define wldt(retT, lpszFmt, ...)					::WriteLogT(retT, LevelDebug, __FUNC__, lpszFmt, __VA_ARGS__) 
#	define dbgt(lpszFmt, ...)							::WriteDbgT(retT, __FUNC__, lpszFmt, __VA_ARGS__) 
#else 
#	define wle(lpszFmt, args...)						::WriteLog(LevelError, __FUNC__, lpszFmt, ##args) 
#	define wlf(lpszFmt, args...)						::WriteLog(LevelFatal, __FUNC__, lpszFmt, ##args) 
#	define wlw(lpszFmt, args...)						::WriteLog(LevelWarning, __FUNC__, lpszFmt, ##args) 
#	define wli(lpszFmt, args...)						::WriteLog(LevelInfo, __FUNC__, lpszFmt, ##args) 
#	define wld(lpszFmt, args...)						::WriteLog(LevelDebug, __FUNC__, lpszFmt, ##args) 
#	define dbg(lpszFmt, args...)						::WriteDbg(__FUNC__, lpszFmt, ##args) 

#	define wlet(retT, lpszFmt, args...)				::WriteLogT(retT, LevelError, __FUNC__, lpszFmt, ##args) 
#	define wlft(retT, lpszFmt, args...)				::WriteLogT(retT, LevelFatal, __FUNC__, lpszFmt, ##args) 
#	define wlwt(retT, lpszFmt, args...)				::WriteLogT(retT, LevelWarning, __FUNC__, lpszFmt, ##args) 
#	define wlit(retT, lpszFmt, args...)				::WriteLogT(retT, LevelInfo, __FUNC__, lpszFmt, ##args) 
#	define wldt(retT, lpszFmt, args...)				::WriteLogT(retT, LevelDebug, __FUNC__, lpszFmt, ##args) 
#	define dbgt(lpszFmt, args...)						::WriteDbgT(retT, __FUNC__, lpszFmt, ##args) 
#endif

#define lre() CLogRoller(LevelError, __FUNC__)
#define lrf() CLogRoller(LevelFatal, __FUNC__)
#define lrw() CLogRoller(LevelWarning, __FUNC__)
#define lri() CLogRoller(LevelInfo, __FUNC__)
#define lrd() CLogRoller(LevelDebug, __FUNC__)

/************************************************************************
* 使用实例
* 在程序开始处首先设置一次日志文件的名称（前缀）
*		SetLogName(L"test");
*		SetLogLevel(LevelInfo);
* 有多种调用方式：
* 函数调用方式：
*		WriteLog(LevelError, L"Test", 1234, L"fdkjfakjdfkadjfadsk哇啦哇啦");
* 函数格式化方式：
*		WriteLog(LevelError, L"Format", L"a=%d, b=%I64d, e=%.2f", 34, 12345678901, 3456.7890);
* 流序列化方式：
*		CLogRoller(LevelWarning, L"Roller") << L"Hello";
* 流序列化方式：
*		CLogRoller(LevelWarning, L"num") << " int:" << true << " int:" << 1234 \
*			<< " byte:" << unsigned char(34) << " char:" << 'd' << " int64:" << 1234567890I64 
*			<< " float:" << 1234.5678f << L" double:" << 3333.333333 << endl;

* C#使用参考WriteLog.cs
************************************************************************/
