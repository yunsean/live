#pragma once
#include "SmartHdr.h"
#include "xstring.h"

class CSockWriter
{
public:
	CSockWriter(void);
	~CSockWriter(void);

public:
	bool				IsOpend();
	bool				ConnectPipe();
	bool				WriteString(const xtstring& strText);
	void				ClosePipe();

private:
	bool				m_inited;
#ifdef _WIN32
	CSmartHdr<SOCKET>	m_fd;
#else 
	CSmartHdr<int>		m_fd;
#endif
};

