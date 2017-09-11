#include "PipeWriter.h"
#include "LogWriter.h"
#include "LocalLogWriter.h"

CPipeWriter::CPipeWriter(void)
#ifdef _WIN32
	: m_shNamedPipe(::CloseHandle, INVALID_HANDLE_VALUE)
#endif
{
}

CPipeWriter::~CPipeWriter(void)
{
	ClosePipe();
}

bool CPipeWriter::IsOpend()
{
#ifdef _WIN32
	return (m_shNamedPipe != INVALID_HANDLE_VALUE && m_shNamedPipe != NULL);
#else 
	return false;
#endif
}

bool CPipeWriter::ConnectPipe()
{
#ifdef _WIN32
	const xtstring	strName(_T("\\\\.\\pipe\\seamedia_logpipe"));
	m_shNamedPipe		= ::CreateFile(strName, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (m_shNamedPipe == INVALID_HANDLE_VALUE)
	{
		xtstring			strMsg;
		strMsg.Format(L"LogClient: Connect to pipe (%s) failed. GetLastError() = %d\n", strName, GetLastError());
		OutputDebugString(strMsg);
		return false;
	}
	else
	{
		xtstring			strMsg;
		strMsg.Format(L"LogClient: Connect to pipe (%s) succeed.\n", strName);
		OutputDebugString(strMsg);
		return true;
	}
#else 
	return false;
#endif
}

bool CPipeWriter::WriteString(const xtstring& strText)
{
#ifdef _WIN32
	if (m_shNamedPipe == INVALID_HANDLE_VALUE)return false;
	DWORD				dwWritten(0);
	if(!::WriteFile(m_shNamedPipe, (LPCTSTR)strText, strText.GetLength() * sizeof(TCHAR), &dwWritten, NULL))
	{
		ClosePipe();
		return false;
	}
	return true;
#else 
	return false;
#endif
}

void CPipeWriter::ClosePipe()
{
#ifdef _WIN32
	m_shNamedPipe		= INVALID_HANDLE_VALUE;
#endif
}
