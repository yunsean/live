#include "BitStream.h"

//////////////////////////////////////////////////////////////////////
CBitStream::CBitStream()
	: m_buffer(nullptr)
	, m_size(0)
	, m_swap(new uint8_t[8])
	, m_result((uint32_t*)&m_swap[4])
	, m_pSrcByte(&m_swap[3])
	, m_move((int64_t*)m_swap)
	, m_skiped(0)
	, m_nSrcByteLeftBits(0)
	, m_offset(0) {
}
CBitStream::CBitStream(const uint8_t* b, int nb_b)
	: m_buffer(b)
	, m_size(nb_b)
	, m_swap(new uint8_t[8])
	, m_result((uint32_t*)&m_swap[4])
	, m_pSrcByte(&m_swap[3])
	, m_move((int64_t*)m_swap)
	, m_skiped(0)
	, m_nSrcByteLeftBits(0)
	, m_offset(0) {
}
CBitStream::~CBitStream() {
	delete[] m_swap;
	m_swap = nullptr;
}

void CBitStream::initialize(const uint8_t* b, int nb_b) {
	m_buffer = b;
	m_size = nb_b;
	m_skiped = 0;
	m_nSrcByteLeftBits = 0;
	m_offset = 0;
}
uint32_t CBitStream::read(int bits) {
	m_offset += bits;
	if(m_skiped) {//有需要跳过的BIT位，先跳过
		if(m_nSrcByteLeftBits >= m_skiped) {
			*m_move = *m_move << m_skiped;
			m_nSrcByteLeftBits -= m_skiped;	
		} else {
			m_skiped -= m_nSrcByteLeftBits;
			m_nSrcByteLeftBits = 0;
			m_buffer += (m_skiped/8);
			m_size -= (m_skiped/8);
			int n(m_skiped % 8);
			if (n) {
				if(m_size <= 0)return 0;//BUFFER用完，退出
				*m_pSrcByte = *m_buffer++;
				m_size--;
				*m_move = *m_move << n;
				m_nSrcByteLeftBits	= 8 - n;
			}
		}
		m_skiped = 0;
	}
	*m_result = 0;
	if(m_nSrcByteLeftBits) {//上次有移剩余的BIT位，先移完
		if(bits >= m_nSrcByteLeftBits) {
			*m_move = *m_move << m_nSrcByteLeftBits;
			bits -= m_nSrcByteLeftBits;
			m_nSrcByteLeftBits = 0;
		} else {
			*m_move = *m_move << bits;
			m_nSrcByteLeftBits = m_nSrcByteLeftBits - bits;
			bits = 0;
		}
	}
	while(bits) {//开始左移
		if(m_size <= 0)return 0;//BUFFER用完，退出
		*m_pSrcByte = *m_buffer++;
		m_size--;
		if(bits >= 8) {
			*m_move = *m_move << 8;
			bits -= 8;
		} else {
			*m_move = *m_move << bits;
			m_nSrcByteLeftBits = 8 - bits;
			bits = 0;
		}
	}
	return *m_result;
}
void CBitStream::skip(int bits) {
	m_skiped += bits;
	m_offset += bits;
}
int CBitStream::offset() {
	return m_offset;
}
void CBitStream::alignSkip() {
	int					n(-m_offset & 7);
    if (n)skip(n);
}