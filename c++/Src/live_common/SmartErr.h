#pragma once
#include <exception>
#include <mutex>
#include "xstring.h"
#include "xsystem.h"
#include "ierror.h"

class CSmartErr : public IError
{
public:
	CSmartErr()
		: m_dwCode(0)
		, m_strDesc() {
	}
	virtual ~CSmartErr() {
	}

public:
	virtual unsigned int GetLastErrorCode() const { return m_dwCode; }
	virtual LPCTSTR GetLastErrorDesc() const { return m_strDesc; }

public:
#ifdef MFC
	static xtstring	From(CException* ex, const bool bDeleteException = true);
#endif 
#ifdef WIN32
	static xtstring	From(const DWORD dwErrCode);
#endif
	static xtstring	From(const std::exception& ex);
	static xtstring	From(char* lpszFmt, ...);
	static xtstring	From(wchar_t* lpszFmt, ...);

protected:
	template <typename T> const T& SetErrorT(const T& retValue, const char* const lpszFmt, ...);
	template <typename T> const T& SetErrorT(const T& retValue, const wchar_t* const lpszFmt, ...);
	template <typename T> const T& SetErrorT(const T& retValue, const unsigned int dwCode, const char* const lpszFmt, ...);
	template <typename T> const T& SetErrorT(const T& retValue, const unsigned int dwCode, const wchar_t* const lpszFmt, ...);
#ifdef MFC
	template <typename T> const T& SetErrorT(const T& retValue, CException* ex, const bool bDeleteException = true);
	template <typename T> const T& SetErrorT(const T& retValue, const CStringA& desc, const DWORD code = 0);
	template <typename T> const T& SetErrorT(const T& retValue, const CStringW& desc, const DWORD code = 0);
#endif
	template <typename T> const T& SetErrorT(const T& retValue, const std::exception& ex);
	template <typename T> const T& SetErrorT(const T& retValue, const std::string& desc, const unsigned int code = 0);
	template <typename T> const T& SetErrorT(const T& retValue, const std::wstring& desc, const unsigned int code = 0);
#ifdef WIN32
	template <typename T> const T& SetErrorT(const T& retValue, const DWORD dwErrCode);
#endif
	template <typename T> const T& SetErrorT(const T& retValue, const IError* const lpError);
	template <typename T> const T& CleanErrorT(const T& retValue);

protected:
	void SetError(const char* const lpszFmt, ...);
	void SetError(const unsigned int dwCode, const char* const lpszFmt, ...);
	void SetError(const wchar_t* const lpszFmt, ...);
	void SetError(const unsigned int dwCode, const wchar_t* const lpszFmt, ...);
	void SetError(const std::exception& ex);
	void SetError(const std::string& desc, const unsigned int code = 0);
	void SetError(const std::wstring& desc, const unsigned int code = 0);
#ifdef MFC
	void SetError(CException* ex, const bool bDeleteException = true);
	void SetError(const CStringA& desc, const DWORD code = 0);
	void SetError(const CStringW& desc, const DWORD code = 0);
#endif 
#ifdef WIN32
	void SetError(const DWORD dwErrCode);
#endif
	void SetError(const IError* const lpError);
	void CleanError();

protected:
	virtual void OnSetError() {}

protected:
	std::recursive_mutex m_lock;
	unsigned int m_dwCode;
	xtstring m_strDesc;
};

template <typename T>
const T& CSmartErr::SetErrorT(const T& retValue, const char* const lpszFmt, ...) {
	m_lock.lock();
	m_dwCode = std::lastError();
	va_list args;
#ifdef UNICODE	
	xstring<char> str;
	va_start(args, lpszFmt);
	str.FormatV(lpszFmt, args);
	m_strDesc = str;
	va_end(args);
#else 
	va_start(args, lpszFmt);
	m_strDesc.FormatV(lpszFmt, args);
	va_end(args);
#endif
	OnSetError();
	m_lock.unlock();
	return retValue;
}

template <typename T>
const T& CSmartErr::SetErrorT(const T& retValue, const unsigned int dwCode, const char* const lpszFmt, ...) {
	m_lock.lock();
	m_dwCode = dwCode;
	va_list args;
#ifdef UNICODE
	xstring<char> str;
	va_start(args, lpszFmt);
	str.FormatV(lpszFmt, args);
	m_strDesc = str;
	va_end(args);
#else 
	va_start(args, lpszFmt);
	m_strDesc.FormatV(lpszFmt, args);
	va_end(args);
#endif
	OnSetError();
	m_lock.unlock();
	return retValue;
}

template <typename T>
const T& CSmartErr::SetErrorT(const T& retValue, const wchar_t* const lpszFmt, ...) {
	m_lock.lock();
	m_dwCode = std::lastError();
	va_list args;
#ifdef UNICODE
	va_start(args, lpszFmt);
	m_strDesc.FormatV(lpszFmt, args);
	va_end(args);
#else 
	xstring<wchar_t> str;
	va_start(args, lpszFmt);
	str.FormatV(lpszFmt, args);
	m_strDesc = str;
	va_end(args);
#endif
	OnSetError();
	m_lock.unlock();
	return retValue;
}

template <typename T>
const T& CSmartErr::SetErrorT(const T& retValue, const unsigned int dwCode, const wchar_t* const lpszFmt, ...) {
	m_lock.lock();
	m_dwCode = dwCode;
	va_list args;
#ifdef UNICODE
	va_start(args, lpszFmt);
	m_strDesc.FormatV(lpszFmt, args);
	va_end(args);
#else 
	xstring<wchar_t> str;
	va_start(args, lpszFmt);
	str.FormatV(lpszFmt, args);
	m_strDesc = str;
	va_end(args);
#endif
	OnSetError();
	m_lock.unlock();
	return retValue;
}

#ifdef MFC
template <typename T>
const T& CSmartErr::SetErrorT(const T& retValue, CException* ex, const bool bDeleteException /* = true */) {
	SetError(ex, bDeleteException);
	return retValue;
}
template <typename T>
const T& CSmartErr::SetErrorT(const T& retValue, const CStringA& desc, const DWORD code /* = 0 */) {
	SetError(desc, code);
	return retValue;
}
template <typename T>
const T& CSmartErr::SetErrorT(const T& retValue, const CStringW& desc, const DWORD code /* = 0 */) {
	SetError(desc, code);
	return retValue;
}
#endif
template <typename T>
const T& CSmartErr::SetErrorT(const T& retValue, const std::exception& ex) {
	SetError(ex);
	return retValue;
}
template <typename T>
const T& CSmartErr::SetErrorT(const T& retValue, const std::string& desc, const unsigned int code /* = 0 */) {
	SetError(desc, code);
	return retValue;
}
template <typename T>
const T& CSmartErr::SetErrorT(const T& retValue, const std::wstring& desc, const unsigned int code /* = 0 */) {
	SetError(desc, code);
	return retValue;
}

#ifdef WIN32
template <typename T>
const T& CSmartErr::SetErrorT(const T& retValue, const DWORD dwErrCode) {
	SetError(dwErrCode);
	return retValue;
}
#endif

template <typename T>
const T& CSmartErr::SetErrorT(const T& retValue, const IError* const lpError) {
	SetError(lpError);
	return retValue;
}
template <typename T>
const T& CSmartErr::CleanErrorT(const T& retValue) {
	CleanError();
	return retValue;
}