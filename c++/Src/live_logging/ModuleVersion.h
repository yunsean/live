#pragma once
#include "xstring.h"
#include "SmartArr.h"

#ifdef _WIN32
class CModuleVersion : public VS_FIXEDFILEINFO 
{
public:
	CModuleVersion(LPCTSTR lpszFileName = NULL);
	virtual ~CModuleVersion(void);

public:
	bool				InitVersionInfo(LPCTSTR lpszFileName);
	xtstring		GetVersionValue(LPCTSTR lpszVersionType);
	xtstring		GetProductVersion();
	xtstring		GetFileVersion();

protected:
	CSmartArr<unsigned char>		m_saVersionInfo; 
	struct TRANSLATION 
	{
		WORD langID;
		WORD charset;
	}m_translation;
};
#endif

