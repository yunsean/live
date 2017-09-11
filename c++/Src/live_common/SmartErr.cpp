#include "SmartErr.h"

#ifdef MFC
xtstring CSmartErr::From(CException* ex, const bool bDeleteException /* = true */) {
	xtstring desc;
	LPTSTR lpsz(desc.GetBuffer(1024));
	ex->GetErrorMessage(lpsz, 1024);
	desc.ReleaseBuffer();
	if (bDeleteException)ex->Delete();
	return desc;
}
#endif
#ifdef WIN32
xtstring CSmartErr::From(const DWORD dwErrCode) {
	LPVOID lpMsgBuf(NULL);
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, dwErrCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf,	0, NULL );
	xtstring desc((LPTSTR)lpMsgBuf);
	LocalFree(lpMsgBuf);
	return desc;
}
#endif

xtstring CSmartErr::From(const std::exception& ex) {
	xtstring desc(ex.what());
	return desc;
}
xtstring CSmartErr::From(char* lpszFmt, ...) {
	xstring<char> desc;
	va_list					args;
	va_start(args, lpszFmt);
	desc.FormatV(lpszFmt, args);
	va_end(args);
#ifdef UNICODE
	xtstring result(desc);
	return result;
#else 
	return desc;
#endif
}
xtstring CSmartErr::From(wchar_t* lpszFmt, ...) {
	xstring<wchar_t> desc;
	va_list args;
	va_start(args, lpszFmt);
	desc.FormatV(lpszFmt, args);
	va_end(args);
#ifdef UNICODE
	return desc;
#else 
	xtstring result(desc);
	return result;
#endif
}
void CSmartErr::SetError(const char* const lpszFmt, ...) {
	m_lock.lock();
	m_dwCode = std::lastError();
	va_list args;
#ifdef UNICODE
	xstring<char> str;
	va_start(args, lpszFmt);
	str.FormatV(lpszFmt, args);
	va_end(args);
	m_strDesc = str;
#else 
	va_start(args, lpszFmt);
	m_strDesc.FormatV(lpszFmt, args);
	va_end(args);
#endif
	OnSetError();
	m_lock.unlock();
}
void CSmartErr::SetError(const unsigned int dwCode, const char* const lpszFmt, ...) {
	m_lock.lock();
	m_dwCode = dwCode;
	va_list args;
#ifdef UNICODE
	xstring<char> str;
	va_start(args, lpszFmt);
	str.FormatV(lpszFmt, args);
	va_end(args);
	m_strDesc = str;
#else 
	va_start(args, lpszFmt);
	m_strDesc.FormatV(lpszFmt, args);
	va_end(args);
#endif
	OnSetError();
	m_lock.unlock();
}
void CSmartErr::SetError(const wchar_t* const lpszFmt, ...) {
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
	va_end(args);
	m_strDesc = str;
#endif
	OnSetError();
	m_lock.unlock();
}
void CSmartErr::SetError(const unsigned int dwCode, const wchar_t* const lpszFmt, ...) {
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
	va_end(args);
	m_strDesc = str;
#endif
	OnSetError();
	m_lock.unlock();
}
void CSmartErr::SetError(const std::exception& ex) {
	m_lock.lock();
	m_dwCode = std::lastError();
	m_strDesc = ex.what();
	OnSetError();
	m_lock.unlock();
}
void CSmartErr::SetError(const std::string& desc, const unsigned int code /* = -1 */) {
	m_lock.lock();
	m_dwCode = code == 0 ? std::lastError() : code;
	m_strDesc = desc.c_str();
	OnSetError();
	m_lock.unlock();
}
void CSmartErr::SetError(const std::wstring& desc, const unsigned int code /* = -1 */) {
	m_lock.lock();
	m_dwCode = code == 0 ? std::lastError() : code;
	m_strDesc = desc.c_str();
	OnSetError();
	m_lock.unlock();
}

#ifdef MFC
void CSmartErr::SetError(CException* ex, const bool bDeleteException /* = true */) {
	m_lock.lock();
	m_dwCode = std::lastError();
	LPTSTR lpsz(m_strDesc.GetBuffer(1024));
	ex->GetErrorMessage(lpsz, 1024);
	m_strDesc.ReleaseBuffer();
	if (bDeleteException)ex->Delete();
	OnSetError();
	m_lock.unlock();
}
void CSmartErr::SetError(const CStringA& desc, const DWORD code /* = 0 */) {
	m_lock.lock();
	m_dwCode = code == 0 ? std::lastError() : code;
	m_strDesc = desc.GetString();
	OnSetError();
	m_lock.unlock();
}
void CSmartErr::SetError(const CStringW& desc, const DWORD code /* = 0 */) {
	m_lock.lock();
	m_dwCode = code == 0 ? std::lastError() : code;
	m_strDesc = desc.GetString();
	OnSetError();
	m_lock.unlock();
}
#endif
#ifdef WIN32
void CSmartErr::SetError(const DWORD dwErrCode) {
	m_lock.lock();
	m_dwCode = dwErrCode;
	LPVOID lpMsgBuf(NULL);
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, dwErrCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf,	0, NULL );
	m_strDesc = (LPTSTR)lpMsgBuf;
	LocalFree(lpMsgBuf);
	OnSetError();
	m_lock.unlock();
}
#endif

void CSmartErr::SetError(const IError* const lpError) {
	if (lpError == NULL)return;
	m_lock.lock();
	m_dwCode = lpError->GetLastErrorCode();
	m_strDesc = lpError->GetLastErrorDesc();
	m_lock.unlock();
}
void CSmartErr::CleanError() {
	m_lock.lock();
	m_dwCode = 0;
	m_strDesc.Empty();
	m_lock.unlock();
}