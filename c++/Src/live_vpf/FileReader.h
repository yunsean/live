#pragma once
#include "IOAccess.h"
#include "SmartErr.h"
#include "SmartHdr.h"

class CFileReader : public IIOReader, public CSmartErr {
public:
	CFileReader(void);
	virtual ~CFileReader(void);

protected:
	//IIOReader
	bool		Open(LPCTSTR strFile);
	bool		CanSeek() { return true; }
	bool		Read(void* cache, const int limit, int& read);
	bool		Seek(const int64_t offset);
	int64_t		Offset();
	bool		Eof();
	void		Close();

protected:
	virtual void OnSetError();
	virtual LPCTSTR	GetLastErrorDesc() const { return m_strDesc; }
	virtual unsigned int GetLastErrorCode() const { return m_dwCode; }

private:
	CSmartHdr<FILE*> m_file;
};

