#pragma once
#include "SmartHdr.h"
#include "xstring.h"

class CPipeWriter
{
public:
	CPipeWriter(void);
	virtual ~CPipeWriter(void);

public:
	bool				IsOpend();
	bool				ConnectPipe();
	bool				WriteString(const xtstring& strText);
	void				ClosePipe();

private:
#ifdef _WIN32
	CSmartHdr<HANDLE>	m_shNamedPipe;
#endif
};

