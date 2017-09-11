#pragma once
#include "os.h"
#include "xchar.h"
#include "xstring.h"
#include "SmartArr.h"

class CProfile {
private:
	CProfile(LPCTSTR lpszIniFile);
	CProfile(const CProfile& cParent, LPCTSTR lpszAppOrKey);
	CProfile(const CProfile& cParent, LPCTSTR lpszAppname, LPCTSTR lpszKeyname);
	CProfile(const CProfile& cParent, LPCTSTR lpszAppname, LPCTSTR lpszKeyname, LPCTSTR lpszDefault);
	CProfile(const CProfile& cParent, LPCTSTR lpszAppname, LPCTSTR lpszKeyname, LPCTSTR lpszDefault, LPCTSTR lpszRemark);

public:
	static CProfile Default();
	static CProfile Profile(LPCTSTR lpszIniFile);

public:
	CProfile operator[](LPCTSTR lpszAppOrName);
	CProfile operator()(LPCTSTR lpszKeyname);
	CProfile operator()(LPCTSTR lpszAppname, LPCTSTR lpszKeyname);
	CProfile operator()(LPCTSTR lpszAppname, LPCTSTR lpszKeyname, LPCTSTR lpszDefault);
	CProfile operator()(LPCTSTR lpszAppname, LPCTSTR lpszKeyname, LPCTSTR lpszDefault, LPCTSTR lpszRemark);

	void operator=(const int nValue);
	void operator=(const long long llValue);
	void operator=(const double dValue);
	void operator=(const xtstring& strValue);
	void operator=(LPCTSTR lpszValue);
	template <typename T>	void operator=(const T* cValue);

	operator short();
	operator int();
	operator bool();
	operator unsigned short();
	operator unsigned int();
	operator long long();
	operator double();
	operator LPCTSTR();
	template <typename T>	operator T*();	//should delete return pointer

	LPCTSTR IniFile() const{return m_strIniFile.GetString();}
	int Read(void* lpValue, const int nSize);
	void Write(void* lpValue, const int nSize);

protected:
	bool writePrivateProfileString(LPCTSTR lpAppName, LPCTSTR lpKeyName, LPCTSTR lpString, LPCTSTR lpFileName, LPCTSTR lpszRemark = NULL);
	unsigned int getPrivateProfileString(LPCTSTR lpAppName, LPCTSTR lpKeyName, LPCTSTR lpDefault, LPTSTR lpReturnedString, unsigned int nSize, LPCTSTR lpFileName);
	unsigned int getPrivateProfileInt(LPCTSTR lpAppName, LPCTSTR lpKeyName, int nDefault, LPCTSTR lpFileName);
	bool getPrivateProfileStruct(LPCTSTR lpszSection, LPCTSTR lpszKey, void* lpStruct, unsigned int uSizeStruct, LPCTSTR szFile);
	bool writePrivateProfileStruct(LPCTSTR lpszSection, LPCTSTR lpszKey, void* lpStruct, unsigned int uSizeStruct, LPCTSTR szFile);

protected:
	bool LoadIniFile(LPCSTR file, CSmartArr<char>& buf, int& len);
	bool ParseFile(LPCSTR section, LPCSTR key, char* buf, int& sec_s, int& sec_e, int& key_s, int& key_e, int& value_s, int& value_e);
	bool ReadIniFile(LPCTSTR lpAppName, LPCTSTR lpKeyName, CSmartArr<char>& saData, int& szData, LPCTSTR lpFileName);
	bool WriteIniFile(LPCTSTR lpszSection, LPCTSTR lpszKey, const void* lpData, unsigned int szData, LPCTSTR szFile, LPCTSTR szRemark = NULL);

private:
#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	xtstring m_strIniFile;
	xtstring m_strAppname;
	xtstring m_strKeyName;
	xtstring m_strValue;
#ifdef _WIN32
#pragma warning(pop)
#endif
};

template <typename T> void CProfile::operator =(const T* cValue) {
	if (m_strKeyName.IsEmpty())	{
		m_strKeyName = m_strAppname;
		m_strAppname = _T(".");
	}
	if (m_strAppname.IsEmpty())m_strAppname	= _T(".");
	if (m_strKeyName.IsEmpty())return;
	void* lpData((void*)cValue);
	int nSize(sizeof(T));
	writePrivateProfileStruct(m_strAppname, m_strKeyName, lpData, nSize, m_strIniFile);
}
template <typename T> CProfile::operator T*() {
	if (m_strKeyName.IsEmpty()) {
		m_strKeyName = m_strAppname;
		m_strAppname = _T(".");
	}
	if (m_strAppname.IsEmpty())m_strAppname	= _T(".");
	if (m_strKeyName.IsEmpty())return NULL;
	T* lpData(new T());
	getPrivateProfileStruct(m_strAppname, m_strKeyName, lpData, sizeof(T), m_strIniFile);
	return lpData;
}

