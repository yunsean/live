#pragma once

#ifdef _WIN32
#ifdef LIVE_MEDIA_EXPORTS
#define MEDIA_API __declspec(dllexport)
#else
#define MEDIA_API __declspec(dllimport)
#endif
#else 
#define MEDIA_API
#endif

class MEDIA_API CBitWriter {
public:
	CBitWriter(void);
	virtual ~CBitWriter(void);

public:
	bool				Initialize(const int nSize);
	bool				Initialize(unsigned char* const lpCache, const int szCache);
	int					SetPos(int nPos);
	void				Clear();
	const unsigned char*GetData() const{return m_lpBuffer;}
	int					GetSize() const{return m_nDataSize;}

	void				WriteBYTE(const unsigned char value);
	void				WriteWORD(const unsigned short value, const bool big_endian = false);
	void				WriteDWORD(const unsigned int value, const bool big_endian = false);
	void				WriteInt(const int value, const bool big_endian = false);
	void				WriteInt64(const long long value, const bool big_endian = false);
	void				WriteDouble(const double value);
	void				WriteData(const unsigned char* pData, const int nDataLen);
	void				WriteDupBYTE(const unsigned char value, const int len);

private:
	bool				m_bExtraBuffer;
	unsigned char*		m_lpBuffer;
	int					m_szBuffer;
	int					m_nDataSize;
	int					m_nPos;
};
