#pragma once
#include "IOAccess.h"
#include "SmartErr.h"
#include "xstring.h"
#include "Byte.h"

class CVpfWriter : public IVpfWriter, public CSmartErr {
public:
	CVpfWriter(void);
	~CVpfWriter(void);

public:
	bool OpenFile(LPCTSTR file, const uint64_t& utcBegin = ~0);
	bool WriteHead(const uint8_t* const data, const int size);
	bool WriteData(const uint8_t type, const uint8_t* const data, const int size, const uint32_t timecode = ~0);
	uint32_t GetLength() const;
	void CloseFile(bool* isValid = NULL);
	LPCTSTR GetInfo(uint64_t& utcBegin, uint64_t& utcEnd);

protected:
	bool WriteHeader(const uint8_t* const data, const int size);
	bool Write10sIndex(const uint32_t nextTC);
	bool Write1sIndex(const uint32_t nextTC);
	bool WriteMediaData(const uint8_t type, const uint32_t timecode, const uint8_t* const data, const int size);
	bool WriteFooter();

protected:
	typedef int TCSecond;
protected:
	bool AppendIndex(const TCSecond nextTC);

protected:
	virtual void OnSetError();
	virtual LPCTSTR	GetLastErrorDesc() const { return m_strDesc; }
	virtual unsigned int GetLastErrorCode() const { return m_dwCode; }

private:
	FILE* m_vpfFile;
	xtstring m_strName;
	xtstring m_strPath;
	bool m_bHasError;
	bool m_bWorking;
	uint64_t m_utcBegin;

	CByte m_byCache;
	bool m_bInnerIndex;
	TCSecond m_uLast10s;
	TCSecond m_uLast1s;
	long m_ll10sOffset;
	long m_ll1sOffset;

	long m_llLastOffset;
	long m_llBaseOffset;
	bool m_bHasHeader;
	uint32_t m_uFirstTC;
	uint32_t m_uLastTC;
	CByte m_byIndexes;
};


