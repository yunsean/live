#pragma once
#include "os.h"

class CBitStream {
public:
	CBitStream();
	CBitStream(const uint8_t* b, int nb_b);
	virtual ~CBitStream();

public:
	void initialize(const uint8_t* b, int nb_b);
	uint32_t read(int bits);	//nBitsNum >= 0 && <= 32
	void skip(int bits);		//nBitsNuM >= 0
	int offset();
	void alignSkip();

private:
	const uint8_t* m_buffer;
	int m_size;
	uint8_t* m_swap;
	uint32_t* m_result;
	uint8_t* m_pSrcByte;
	int64_t* m_move;
	int m_skiped;
	int m_nSrcByteLeftBits;
	int m_offset;
};

