#include "BitReader.h"

//////////////////////////////////////////////////////////////////////
CBitReader::CBitReader()
	: m_lpBuffer(nullptr)
	, m_szBuffer(0)
	, m_lpSwap(new unsigned char [8])
	, m_pRetrun((unsigned int*)&m_lpSwap[4])
	, m_pSrcByte(&m_lpSwap[3])
	, m_pMove((long long*)m_lpSwap)

	, m_nSkipNum(0)
	, m_nSrcByteLeftBits(0)
	, m_nBitsOffset(0) {
}
CBitReader::~CBitReader() {
	delete[] m_lpSwap;
	m_lpSwap	= nullptr;
}

void CBitReader::Initialize(const unsigned char* const lpData, const int szData) {
	m_lpBuffer			= lpData;
	m_szBuffer			= szData;
	m_nSkipNum			= 0;
	m_nSrcByteLeftBits	= 0;
	m_nBitsOffset		= 0;
}

unsigned int CBitReader::GetBits(int nBitsNum) {
	m_nBitsOffset				+= nBitsNum;
	if (m_nSkipNum) {
		if (m_nSrcByteLeftBits >= m_nSkipNum) {
			*m_pMove			= *m_pMove << m_nSkipNum;
			m_nSrcByteLeftBits	-= m_nSkipNum;	
		} else {
			m_nSkipNum			-= m_nSrcByteLeftBits;
			m_nSrcByteLeftBits	= 0;
			m_lpBuffer			+= (m_nSkipNum/8);
			m_szBuffer		-= (m_nSkipNum/8);
			int					n(m_nSkipNum % 8);
			if (n) {
				if (m_szBuffer <= 0)return 0;
				*m_pSrcByte			= *m_lpBuffer++;
				m_szBuffer--;
				*m_pMove			= *m_pMove << n;
				m_nSrcByteLeftBits	= 8 - n;
			}
		}
		m_nSkipNum				= 0;
	}
	*m_pRetrun					= 0;
	if (m_nSrcByteLeftBits) {
		if (nBitsNum >= m_nSrcByteLeftBits) {
			*m_pMove			= *m_pMove << m_nSrcByteLeftBits;
			nBitsNum			-= m_nSrcByteLeftBits;
			m_nSrcByteLeftBits	= 0;
		} else {
			*m_pMove			= *m_pMove << nBitsNum;
			m_nSrcByteLeftBits	= m_nSrcByteLeftBits - nBitsNum;
			nBitsNum			= 0;
		}
	}
	while(nBitsNum) {
		if (m_szBuffer <= 0)return 0;
		*m_pSrcByte				= *m_lpBuffer++;
		m_szBuffer--;
		if (nBitsNum >= 8) {
			*m_pMove			= *m_pMove << 8;
			nBitsNum			-= 8;
		} else {
			*m_pMove			= *m_pMove << nBitsNum;
			m_nSrcByteLeftBits	= 8 - nBitsNum;
			nBitsNum			= 0;
		}
	}
	return *m_pRetrun;
}
void CBitReader::SkipBits(int nBitsNum) {
	m_nSkipNum			+= nBitsNum;
	m_nBitsOffset		+= nBitsNum;
}
int CBitReader::GetBitsOffset() {
	return m_nBitsOffset;
}
void CBitReader::AlignSkip() {
	int					n(-m_nBitsOffset & 7);
    if (n)SkipBits(n);
}