#include "BitWriter.h"
#include <string.h>

CBitWriter::CBitWriter(void)
	: m_bExtraBuffer(false)
	, m_lpBuffer(nullptr)
	, m_szBuffer(0)
	, m_nDataSize(0)
	, m_nPos(0) {
}
CBitWriter::~CBitWriter(void) {
	if (!m_bExtraBuffer)delete[] m_lpBuffer;
}

bool CBitWriter::Initialize(const int nSize) {
	m_bExtraBuffer		= false;
	if (m_lpBuffer)delete[] m_lpBuffer;
	m_lpBuffer			= new unsigned char[nSize];
	m_szBuffer			= nSize;
	memset(m_lpBuffer, 0, m_szBuffer);
	m_nPos				= 0;
	m_nDataSize			= 0;
	return true;
}
bool CBitWriter::Initialize(unsigned char* const lpCache, const int szCache) {
	m_bExtraBuffer		= true;
	m_lpBuffer			= lpCache;
	m_szBuffer			= szCache;
	m_nPos				= 0;
	m_nDataSize			= 0;
	return true;
}

void CBitWriter::Clear() {
	if (m_lpBuffer) {
		memset(m_lpBuffer, 0, m_szBuffer);
		m_nPos			= 0;
		m_nDataSize		= 0;
	}	
}
int CBitWriter::SetPos(int nPos) {
	if (nPos < 0)return -1;	
	if (nPos < m_nDataSize) {
		m_nPos			= nPos;
		return nPos;
	} else if (nPos < m_szBuffer) {
		m_nDataSize		= nPos;
		m_nPos			= nPos;
		return nPos;
	}
	return -1;
}

void CBitWriter::WriteBYTE(const unsigned char value) {
	m_lpBuffer[m_nPos]	= value;
	SetPos(m_nPos + 1);
}
void CBitWriter::WriteWORD(const unsigned short value, const bool big_endian /* = false */) {
	if (big_endian) {
		unsigned char*			p((unsigned char*)&value);
		for (int i = 1; i >= 0; i--) {
			WriteBYTE(p[i]);
		}
	} else {
		memcpy(m_lpBuffer + m_nPos, &value, 2);
		SetPos(m_nPos + 2);
	}
}
void CBitWriter::WriteDWORD(const unsigned int value, const bool big_endian /* = false */) {
	if (big_endian) {
		unsigned char*			p((unsigned char*)&value);
		for (int i = 3; i >= 0; i--) {
			WriteBYTE(p[i]);
		}
	} else {
		memcpy(m_lpBuffer + m_nPos, &value, 4);
		SetPos(m_nPos + 4);
	}
}
void CBitWriter::WriteInt(const int value, const bool big_endian /* = false */) {
	if (big_endian) {
		unsigned char*			p((unsigned char*)&value);
		for (int i = 3; i >= 0; i--) {
			WriteBYTE(p[i]);
		}
	} else {
		memcpy(m_lpBuffer + m_nPos, &value, 4);
		SetPos(m_nPos + 4);
	}
}
void CBitWriter::WriteInt64(const long long value, const bool big_endian /* = false */) {
	if (big_endian) {
		unsigned char*			p((unsigned char*)&value);
		for (int i = 7; i >= 0; i--) {
			WriteBYTE(p[i]);
		}
	} else {
		memcpy(m_lpBuffer + m_nPos, &value, 8);
		SetPos(m_nPos + 8);
	}
}
void CBitWriter::WriteDouble(const double value) {
	memcpy(m_lpBuffer + m_nPos, &value, 8);
	SetPos(m_nPos + 8);
}
void CBitWriter::WriteData(const unsigned char* pData, const int nDataLen) {
	memcpy(m_lpBuffer + m_nPos, pData, nDataLen);
	SetPos(m_nPos + nDataLen);
}
void CBitWriter::WriteDupBYTE(const unsigned char value, const int len) {
	memset(m_lpBuffer + m_nPos, value, len);
	SetPos(m_nPos + len);
}
