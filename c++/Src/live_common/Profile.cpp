#include <iosfwd>
#include <iostream>
#include <fstream>
#include "Profile.h"
#include "Byte.h"
#include "xsystem.h"

#define INVALID_VALUE			_T("(-~!@#$%^&*+)")

//////////////////////////////////////////////////////////////////////////
//CProfile
CProfile::CProfile(LPCTSTR lpszIniFile)
	: m_strIniFile(lpszIniFile)
	, m_strAppname()
	, m_strKeyName()
	, m_strValue() {
}
CProfile::CProfile(const CProfile& cParent, LPCTSTR lpszAppOrKey)
	: m_strIniFile(cParent.m_strIniFile)
	, m_strAppname(cParent.m_strAppname.IsEmpty() ? lpszAppOrKey : cParent.m_strAppname.c_str())
	, m_strKeyName(cParent.m_strAppname.IsEmpty() ? _T("") : lpszAppOrKey)
	, m_strValue() {
}
CProfile::CProfile(const CProfile& cParent, LPCTSTR lpszAppname, LPCTSTR lpszKeyname)
	: m_strIniFile(cParent.m_strIniFile)
	, m_strAppname(lpszAppname)
	, m_strKeyName(lpszKeyname)
	, m_strValue() {
}
CProfile::CProfile(const CProfile& cParent, LPCTSTR lpszAppname, LPCTSTR lpszKeyname, LPCTSTR lpszDefault)
	: m_strIniFile(cParent.m_strIniFile)
	, m_strAppname(_tcslen(lpszAppname) ? lpszAppname : _T("."))
	, m_strKeyName(lpszKeyname)
	, m_strValue() {
	xtstring			strValue;
	getPrivateProfileString(m_strAppname, m_strKeyName, INVALID_VALUE, strValue.GetBuffer(1024), 1024, m_strIniFile);
	strValue.ReleaseBuffer();
	if (strValue == INVALID_VALUE)writePrivateProfileString(m_strAppname, m_strKeyName, lpszDefault, m_strIniFile);
}

CProfile::CProfile(const CProfile& cParent, LPCTSTR lpszAppname, LPCTSTR lpszKeyname, LPCTSTR lpszDefault, LPCTSTR lpszRemark)
	: m_strIniFile(cParent.m_strIniFile)
	, m_strAppname(_tcslen(lpszAppname) ? lpszAppname : _T("."))
	, m_strKeyName(lpszKeyname)
	, m_strValue() {
	xtstring			strValue;
	getPrivateProfileString(m_strAppname, m_strKeyName, INVALID_VALUE, strValue.GetBuffer(1024), 1024, m_strIniFile);
	strValue.ReleaseBuffer();
	if (strValue == INVALID_VALUE)writePrivateProfileString(m_strAppname, m_strKeyName, lpszDefault, m_strIniFile, lpszRemark);
}
CProfile CProfile::Default() {
	TCHAR					exeFile[1024];
	std::exefile(exeFile);
	xtstring			iniFile(exeFile);
	iniFile					+= _T(".ini");
	return CProfile(iniFile);
}

CProfile CProfile::Profile(LPCTSTR lpszIniFile) {
	return CProfile(lpszIniFile);
}
CProfile CProfile::operator [](LPCTSTR lpszKey) {
	return CProfile(*this, lpszKey);
}
CProfile CProfile::operator ()(LPCTSTR lpszKeyname) {
	return CProfile(*this, lpszKeyname);
}
CProfile CProfile::operator ()(LPCTSTR lpszAppname, LPCTSTR lpszKeyname) {
	return CProfile(*this, lpszAppname, lpszKeyname);
}
CProfile CProfile::operator ()(LPCTSTR lpszAppname, LPCTSTR lpszKeyname, LPCTSTR lpszDefault) {
	return CProfile(*this, lpszAppname, lpszKeyname, lpszDefault);
}
CProfile CProfile::operator ()(LPCTSTR lpszAppname, LPCTSTR lpszKeyname, LPCTSTR lpszDefault, LPCTSTR lpszRemark) {
	return CProfile(*this, lpszAppname, lpszKeyname, lpszDefault, lpszRemark);
}

int CProfile::Read(void* lpValue, const int nSize) {
	if (m_strKeyName.IsEmpty()) {
		m_strKeyName = m_strAppname;
		m_strAppname = _T(".");
	}
	if (m_strAppname.IsEmpty())m_strAppname = _T(".");
	if (m_strKeyName.IsEmpty())return 0;
	return getPrivateProfileStruct(m_strAppname, m_strKeyName, lpValue, nSize, m_strIniFile);
}
void CProfile::Write(void* lpValue, const int nSize) {
	if (m_strKeyName.IsEmpty()) {
		m_strKeyName = m_strAppname;
		m_strAppname = _T(".");
	}
	if (m_strAppname.IsEmpty())m_strAppname = _T(".");
	if (m_strKeyName.IsEmpty())return;
	writePrivateProfileStruct(m_strAppname, m_strKeyName, lpValue, nSize, m_strIniFile);
}

void CProfile::operator =(const int nValue) {
	if (m_strKeyName.IsEmpty())	{
		m_strKeyName = m_strAppname;
		m_strAppname = _T(".");
	}
	if (m_strAppname.IsEmpty())m_strAppname = _T(".");
	if (m_strKeyName.IsEmpty())return;
	xtstring strValue;
	strValue.Format(_T("%d"), nValue);
	writePrivateProfileString(m_strAppname, m_strKeyName, strValue, m_strIniFile);
}
void CProfile::operator =(const long long llValue) {
	if (m_strKeyName.IsEmpty()) {
		m_strKeyName = m_strAppname;
		m_strAppname = _T(".");
	}
	if (m_strAppname.IsEmpty())m_strAppname = _T(".");
	if (m_strKeyName.IsEmpty())return;
	xtstring strValue;
	strValue.Format(_T("%lld"), llValue);
	writePrivateProfileString(m_strAppname, m_strKeyName, strValue, m_strIniFile);
}
void CProfile::operator =(const double dValue) {
	if (m_strKeyName.IsEmpty())	{
		m_strKeyName = m_strAppname;
		m_strAppname = _T(".");
	}
	if (m_strAppname.IsEmpty())m_strAppname = _T(".");
	if (m_strKeyName.IsEmpty())return;
	xtstring strValue;
	strValue.Format(_T("%f"), dValue);
	writePrivateProfileString(m_strAppname, m_strKeyName, strValue, m_strIniFile);
}
void CProfile::operator=(const xtstring& strValue) {
	if (m_strKeyName.IsEmpty()) {
		m_strKeyName = m_strAppname;
		m_strAppname = _T(".");
	}
	if (m_strAppname.IsEmpty())m_strAppname = _T(".");
	if (m_strKeyName.IsEmpty())return;
	writePrivateProfileString(m_strAppname, m_strKeyName, strValue, m_strIniFile);
}
void CProfile::operator=(LPCTSTR lpszValue) {
	if (m_strKeyName.IsEmpty())	{
		m_strKeyName = m_strAppname;
		m_strAppname = _T(".");
	}
	if (m_strAppname.IsEmpty())m_strAppname = _T(".");
	if (m_strKeyName.IsEmpty())return;
	writePrivateProfileString(m_strAppname, m_strKeyName, lpszValue, m_strIniFile);
}
CProfile::operator short() {
	if (m_strKeyName.IsEmpty()) {
		m_strKeyName = m_strAppname;
		m_strAppname = _T(".");
	}
	if (m_strAppname.IsEmpty())m_strAppname = _T(".");
	if (m_strKeyName.IsEmpty())return 0;
	return (short)getPrivateProfileInt(m_strAppname, m_strKeyName, 0, m_strIniFile);
}
CProfile::operator int() {
	if (m_strKeyName.IsEmpty())	{
		m_strKeyName = m_strAppname;
		m_strAppname = _T(".");
	}
	if (m_strAppname.IsEmpty())m_strAppname = _T(".");
	if (m_strKeyName.IsEmpty())return 0;
	return getPrivateProfileInt(m_strAppname, m_strKeyName, 0, m_strIniFile);
}
CProfile::operator bool() {
	if (m_strKeyName.IsEmpty()) {
		m_strKeyName = m_strAppname;
		m_strAppname = _T(".");
	}
	if (m_strAppname.IsEmpty())m_strAppname = _T(".");
	if (m_strKeyName.IsEmpty())return false;
	return getPrivateProfileInt(m_strAppname, m_strKeyName, 0, m_strIniFile) != 0;
}
CProfile::operator unsigned short() {
	if (m_strKeyName.IsEmpty())	{
		m_strKeyName = m_strAppname;
		m_strAppname = _T(".");
	}
	if (m_strAppname.IsEmpty())m_strAppname = _T(".");
	if (m_strKeyName.IsEmpty())return 0;
	return (unsigned short)getPrivateProfileInt(m_strAppname, m_strKeyName, 0, m_strIniFile);
}
CProfile::operator unsigned int() {
	if (m_strKeyName.IsEmpty())	{
		m_strKeyName = m_strAppname;
		m_strAppname = _T(".");
	}
	if (m_strAppname.IsEmpty())m_strAppname = _T(".");
	if (m_strKeyName.IsEmpty())return 0;
	return getPrivateProfileInt(m_strAppname, m_strKeyName, 0, m_strIniFile);
}
CProfile::operator long long() {
	if (m_strKeyName.IsEmpty()) {
		m_strKeyName = m_strAppname;
		m_strAppname = _T(".");
	}
	if (m_strAppname.IsEmpty())m_strAppname = _T(".");
	if (m_strKeyName.IsEmpty())return 0LL;
	xtstring strValue;
	getPrivateProfileString(m_strAppname, m_strKeyName, _T(""), strValue.GetBuffer(32), 32, m_strIniFile);
	strValue.ReleaseBuffer();
	try{
		return _ttoi64(strValue);
	} catch (...) {
	}
	return 0;
}
CProfile::operator double() {
	if (m_strKeyName.IsEmpty())	{
		m_strKeyName = m_strAppname;
		m_strAppname = _T(".");
	}
	if (m_strAppname.IsEmpty())m_strAppname = _T(".");
	if (m_strKeyName.IsEmpty())return 0;
	xtstring		strValue;
	getPrivateProfileString(m_strAppname, m_strKeyName, _T(""), strValue.GetBuffer(32), 32, m_strIniFile);
	strValue.ReleaseBuffer();
	try	{
		return _tstof(strValue);
	} catch (...) {
	}
	return 0.0;
}
CProfile::operator LPCTSTR() {
	if (m_strKeyName.IsEmpty()) {
		m_strKeyName = m_strAppname;
		m_strAppname = _T(".");
	}
	if (m_strAppname.IsEmpty())m_strAppname = _T(".");
	if (m_strKeyName.IsEmpty())return _T("");
	getPrivateProfileString(m_strAppname, m_strKeyName, _T(""), m_strValue.GetBuffer(1024), 1024, m_strIniFile);
	m_strValue.ReleaseBuffer();
	return m_strValue;
}
bool CProfile::writePrivateProfileString(LPCTSTR lpAppName, LPCTSTR lpKeyName, LPCTSTR lpString, LPCTSTR lpFileName, LPCTSTR lpszRemark) {
#ifdef WritePrivateProfileString1
	return ::WritePrivateProfileString(lpAppName, lpKeyName, lpString, lpFileName) ? true : false;
#else 
	xstring<char> value(xstring<char>::convert(lpString));
	return WriteIniFile(lpAppName, lpKeyName, value.c_str(), (int)value.length(), lpFileName, lpszRemark);
#endif
}
unsigned int CProfile::getPrivateProfileString(LPCTSTR lpAppName, LPCTSTR lpKeyName, LPCTSTR lpDefault, LPTSTR lpReturnedString, unsigned int nSize, LPCTSTR lpFileName) {
#ifdef GetPrivateProfileString1
	return ::GetPrivateProfileString(lpAppName, lpKeyName, lpDefault, lpReturnedString, nSize, lpFileName);
#else 
	CSmartArr<char> saValue;
	int szValue(0);
	if (!ReadIniFile(lpAppName, lpKeyName, saValue, szValue, lpFileName)) {
		_tcscpy_s(lpReturnedString, nSize, lpDefault);
		return (unsigned int)_tcslen(lpDefault);
	}
	xtstring res(saValue.GetArr(), szValue);
	int count((int)res.length());
	if (count > (int)nSize - 1)
		count = nSize - 1;
	_tcsncpy_s(lpReturnedString, nSize, res.c_str(), count);
	return count;
#endif
}
unsigned int CProfile::getPrivateProfileInt(LPCTSTR lpAppName, LPCTSTR lpKeyName, int nDefault, LPCTSTR lpFileName) {
#ifdef GetPrivateProfileInt1
	return ::GetPrivateProfileInt(lpAppName, lpKeyName, nDefault, lpFileName);
#else 
	TCHAR szNumber[20];
	if (getPrivateProfileString(lpAppName, lpKeyName, _T(""), szNumber, 20, lpFileName) < 1)return 0;
	return _ttoi(szNumber);
#endif
}
bool CProfile::getPrivateProfileStruct(LPCTSTR lpszSection, LPCTSTR lpszKey, void* lpStruct, unsigned int uSizeStruct, LPCTSTR szFile)
{
#ifdef GetPrivateProfileStruct0
	return ::GetPrivateProfileStruct(lpszSection, lpszKey, lpStruct, uSizeStruct, szFile) ? true : false;
#else 
	CSmartArr<char> saValue;
	int szValue(0);
	if (!ReadIniFile(lpszSection, lpszKey, saValue, szValue, szFile))return false;
	xtstring value(saValue.GetArr(), szValue);
	CByte byValue;
	byValue.fromBase64(value);
	byValue.GetData(lpStruct, (int)uSizeStruct);
	return true;
#endif
}
bool CProfile::writePrivateProfileStruct(LPCTSTR lpszSection, LPCTSTR lpszKey, void* lpStruct, unsigned int uSizeStruct, LPCTSTR szFile)
{
#ifdef WritePrivateProfileStruct1
	return ::WritePrivateProfileStruct(lpszSection, lpszKey, lpStruct, uSizeStruct, szFile) ? true : false;
#else 
	xtstring value(CByte::toBase64((CByte::BYTE*)lpStruct, uSizeStruct));
	std::string str(value.toString());
	return WriteIniFile(lpszSection, lpszKey, str.c_str(), (int)str.length(), szFile);
#endif
}

bool CProfile::LoadIniFile(LPCSTR file, CSmartArr<char>& buf, int& len)
{
	std::ifstream is(file, std::ios::in | std::ios::binary);
	if (!is.bad()) {
		is.seekg(0, std::ios::end);
		len = (int)is.tellg();
		if (len > 0) {
			is.seekg(0, std::ios::beg);
			buf = new(std::nothrow) char[len * 2];
			is.read(buf.GetArr(), len);
			buf.GetArr()[len] = '\0';
			is.close();
			return true;
		}
	}
	is.close();
	std::ofstream os(file, std::ios::out);
	if (is.bad())return false;
	os.close();
	len = 0;
	return true;
}

bool CProfile::ParseFile(LPCSTR section, LPCSTR key, char* buf, int& sec_s, int& sec_e, int& key_s, int& key_e, int& value_s, int& value_e)
{
	if (buf == nullptr)return false;

	const char* p(buf);
	int i(0);

	sec_e = -1;
	sec_s = -1;
	key_e = -1;
	key_s = -1;
	value_s = -1;
	value_e = -1;
	while (p[i] != '\0') {
		if ((0 == i || (p[i - 1] == '\n' || p[i - 1] == 'r')) && (p[i] == '[')) {
			int section_start(i + 1);
			do {
				i++;
			} while((p[i] != ']') && (p[i] != '\0'));
			if (0 == strncmp(p + section_start, section, i-section_start)) {
				int newline_start(0);
				i++;
				while(isspace(p[i])) {
					i++;
				}
				sec_s = section_start;
				sec_e = i;
				while (!((p[i-1] == '\r' || p[i-1] == '\n') && (p[i] == '[')) && (p[i] != '\0') ) {
						int j(0);
						newline_start = i;
						while ((p[i] != '\r' && p[i] != '\n') && (p[i] != '\0')) {
							i++;
						}
						j = newline_start;
						if(';' != p[j]) {
							while(j < i && p[j] != '=') {
								j++;
								if('=' == p[j]) {
									if(strncmp(key, p + newline_start, j - newline_start) == 0) {
										key_s	= newline_start;
										key_e	= j-1;
										value_s = j+1;
										value_e = i;
										return true;
									}
								}
							}
						}
						i++;
				}
			}
		} else {
			i++;
		}
	}
	return false;
}

bool CProfile::ReadIniFile(LPCTSTR lpAppName, LPCTSTR lpKeyName, CSmartArr<char>& saData, int& szData, LPCTSTR lpFileName)
{
	CSmartArr<char> buf;
	int len(0);
	xstring<char> fileName(xstring<char>::convert(lpFileName));
	if (!LoadIniFile(fileName, buf, len))return false;

	int sec_s(-1);
	int sec_e(-1);
	int key_s(-1);
	int key_e(-1);
	int value_s(-1);
	int value_e(-1);
	xstring<char> appName(xstring<char>::convert(lpAppName));
	xstring<char> keyName(xstring<char>::convert(lpKeyName));
	if (!ParseFile(appName, keyName, buf, sec_s, sec_e, key_s, key_e, value_s, value_e))return false;

	int size(value_e - value_s);
	saData = new(std::nothrow) char[size + 1];
	if (saData.GetArr() == NULL)return false;
	memcpy(saData, buf + value_s, size);
	saData[size] = '\0';
	szData = value_e - value_s;
	return true;
}

bool CProfile::WriteIniFile(LPCTSTR lpszSection, LPCTSTR lpszKey, const void* lpData, unsigned int szData, LPCTSTR szFile, LPCTSTR szRemark)
{
	CSmartArr<char> buf;
	int filesize(0);
	xstring<char> fileName(xstring<char>::convert(szFile));
	if (!LoadIniFile(fileName, buf, filesize))return false;

	int sec_s(-1);
	int sec_e(-1);
	int key_s(-1);
	int key_e(-1);
	int value_s(-1);
	int value_e(-1);
	xstring<char> appName(xstring<char>::convert(lpszSection));
	xstring<char> keyName(xstring<char>::convert(lpszKey));
	ParseFile(appName, keyName, buf, sec_s, sec_e, key_s, key_e, value_s, value_e);

	try
	{
		std::ofstream os(fileName, std::ios::out | std::ios::binary | std::ios::trunc);
		if (sec_s == -1) {
			os.write(buf, filesize);
			if (filesize > 0)os << "\r\n";
			os << "[" << appName << "]" << "\r\n";
			if (szRemark != NULL && _tcslen(szRemark) > 0){
				xstring<char>	remark(szRemark);
				os << ";" << remark << "\r\n";
			}
			os << keyName << "=";
			os.write((char*)lpData, szData);
			os << "\r\n";
		} 
		else if(key_s == -1) {
			os.write(buf, sec_e);
			if (szRemark != NULL && _tcslen(szRemark) > 0){
				xstring<char>	remark(szRemark);
				os << ";" << remark << "\r\n";
			}
			os << keyName << "=";
			os.write((char*)lpData, szData);
			os << "\r\n";
			os.write(buf + sec_e, filesize - sec_e);
		} else {
			os.write(buf, value_s);
			os.write((char*)lpData, szData);
			os.write(buf + value_e, filesize - value_e);
		}
		os.close();
		return true;
	}
	catch (std::exception e){
		return false;
	}
}

