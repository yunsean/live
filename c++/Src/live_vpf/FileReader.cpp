#include "FileReader.h"
#include "Writelog.h"

CFileReader::CFileReader() 
	: m_file(::fclose) {
}
CFileReader::~CFileReader() {
	Close();
}

//////////////////////////////////////////////////////////////////////////
void CFileReader::OnSetError() {
	wle(m_strDesc);
}

bool CFileReader::Open(LPCTSTR strFile) {
	try {
		m_file = ::_tfopen(strFile, _T("rb+"));
		if (!m_file.isValid()) {
			return SetErrorT(false, _T("Open file failed."));
		}
	} catch (...) {
		return SetErrorT(false, _T("Open file failed."));
	}
	return true;
}
bool CFileReader::Read(void* cache, const int limit, int& read) {
	if (cache == nullptr) return false;
	if (!m_file.isValid()) return false;
	try {
		read = (int)::fread(reinterpret_cast<char*>(cache), 1, limit, m_file);
	} catch (std::exception ex) {
		return SetErrorT(false, ex);
	}
	return read > 0;
}
bool CFileReader::Seek(const int64_t offset) {
	if (!m_file.isValid()) return false;
	try {
		::fseek(m_file, (long)offset, SEEK_SET);
	}
	catch (std::exception ex) {
		return SetErrorT(false, ex);
	}
	return ::ftell(m_file) == offset;
}
int64_t CFileReader::Offset() {
	if (!m_file.isValid()) return false;
	try {
		return ::ftell(m_file);;
	}
	catch (std::exception ex) {
		return SetErrorT(0, ex);
	}
}
bool CFileReader::Eof() {
	if (!m_file.isValid()) return true;
	return ::feof(m_file) != 0;
}
void CFileReader::Close() {
	m_file = NULL;
}

IIOReader* CreateFileReader() {
	return new CFileReader;
}