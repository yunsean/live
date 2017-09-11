#include "ModuleVersion.h"
#ifdef _WIN32
#pragma comment(lib, "Version.lib")
#endif

#ifdef _WIN32
CModuleVersion::CModuleVersion(LPCTSTR lpszFileName /* = NULL */)
	: m_saVersionInfo(NULL)
{
	memset((VS_FIXEDFILEINFO*)this, 0, sizeof(VS_FIXEDFILEINFO));
	if (lpszFileName)InitVersionInfo(lpszFileName);
}

CModuleVersion::~CModuleVersion(void)
{
}

bool CModuleVersion::InitVersionInfo(LPCTSTR lpszFileName)
{
	bool			bResult(false);
	DWORD			len(0);
	DWORD			dwDummyHandle(0);
	LPVOID			lpvi(NULL);
	unsigned int			iLen(0);
	bool			bRet(0);
	
	len					= GetFileVersionInfoSize(lpszFileName, &dwDummyHandle);
	if (len <= 0)return false;
	m_saVersionInfo		= new unsigned char[len];
	bRet				= ::GetFileVersionInfo(lpszFileName, 0, len, m_saVersionInfo) ? true : false;
	if (!bRet)return false;
	bRet				= VerQueryValue(m_saVersionInfo, _T("\\"), &lpvi, &iLen) ? true : false;
	if (!bRet)return false;
	*(VS_FIXEDFILEINFO*)this = *(VS_FIXEDFILEINFO*)lpvi;
	if (VerQueryValue(m_saVersionInfo, _T("\\VarFileInfo\\Translation"), &lpvi, &iLen) && iLen >= 4)
		m_translation	= *(TRANSLATION*)lpvi;
	bResult				= (dwSignature == VS_FFI_SIGNATURE);
	return bResult;
}

xtstring CModuleVersion::GetVersionValue(LPCTSTR lpszVersionType)
{
	xtstring			strVal;
	xtstring			strQuery;
	LPCTSTR			ptszVal(NULL);
	unsigned int			iLenVal(0);

	if (m_saVersionInfo) 
	{
		strQuery.Format(L"\\StringFileInfo\\%04x%04x\\%s", m_translation.langID, m_translation.charset, lpszVersionType);
		if (VerQueryValue(m_saVersionInfo, (LPTSTR)(LPCTSTR)strQuery, (LPVOID*)&ptszVal, &iLenVal))
			strVal = ptszVal;
	}

	return strVal;
}

xtstring CModuleVersion::GetProductVersion()
{
	xtstring			strVal;
	strVal.Format(L"%d.%d.%d.%d", HIWORD(dwProductVersionMS), LOWORD(dwProductVersionMS), HIWORD(dwProductVersionLS), LOWORD(dwProductVersionLS));
	return strVal;
}

xtstring CModuleVersion::GetFileVersion()
{
	xtstring			strVal;
	strVal.Format(L"%d.%d.%d.%d", HIWORD(dwFileVersionMS), LOWORD(dwFileVersionMS), HIWORD(dwFileVersionLS), LOWORD(dwFileVersionLS));
	return strVal;
}
#endif