#include "HandshakeBytes.h"
#include "Writelog.h"
#include "Byte.h"
#include "xstring.h"
#include "xutils.h"

#define C0C1_Size	(1 + 1536)
#define S0S1S2_Size	(1 + 1536 + 1536)
#define C2_Size		(1536)

CHandshakeBytes::CHandshakeBytes()
	: m_c0c1(nullptr)
	, m_s0s1s2(nullptr)
	, m_c2(nullptr) {
}
CHandshakeBytes::~CHandshakeBytes() {
}

int CHandshakeBytes::read_c0c1(evbuffer* input) {
	if (!m_c0c1.isNull()) return Succeed;
	size_t length(evbuffer_get_length(input));
	if (length < C0C1_Size) return NeedMore;
	m_c0c1 = new char[C0C1_Size];
	int read(evbuffer_remove(input, m_c0c1, C0C1_Size));
	if (read != C0C1_Size) {
		m_c0c1 = nullptr;
		return wlet(Failed, _T("read c0c1 failed. ret=%d"), read);
	}
	return Succeed;
}
int CHandshakeBytes::read_s0s1s2(evbuffer* input) {
	if (!m_s0s1s2.isNull()) return Succeed;
	size_t length(evbuffer_get_length(input));
	if (length < S0S1S2_Size) return NeedMore;
	m_s0s1s2 = new char[S0S1S2_Size];
	int read(evbuffer_remove(input, m_s0s1s2, S0S1S2_Size));
	if (read != S0S1S2_Size) {
		m_s0s1s2 = nullptr;
		return wlet(Failed, _T("read s0s1s2 failed. ret=%d"), read);
	}
	return Succeed;
}
int CHandshakeBytes::read_c2(evbuffer* input) {
	if (!m_c2.isNull()) return Succeed;
	size_t length(evbuffer_get_length(input));
	if (length < C2_Size) return NeedMore;
	m_c2 = new char[C2_Size];
	int read(evbuffer_remove(input, m_c2, C2_Size));
	if (read != C2_Size) {
		m_c2 = nullptr;
		return wlet(Failed, _T("read c2 failed. ret=%d"), read);
	}
	return Succeed;
}
void CHandshakeBytes::create_c0c1() {
	m_c0c1 = new char[C0C1_Size];
	utils::randomGenerate(m_c0c1, C0C1_Size);
	m_c0c1[0] = 0x03;
	write_4bytes(m_c0c1 + 1, (int32_t)::time(NULL));
	write_4bytes(m_c0c1 + 5, 0x00000000);
}
void CHandshakeBytes::create_s0s1s2(const char* c1 /*= nullptr*/) {
	m_s0s1s2 = new char[S0S1S2_Size];
	utils::randomGenerate(m_s0s1s2, S0S1S2_Size);
	m_s0s1s2[0] = 0x03;
	write_4bytes(m_s0s1s2 + 1, (int32_t)::time(NULL));	
	memcpy(m_s0s1s2 + 5, m_c0c1 + 1, 4);	
	if (c1) {	// @see: https://github.com/ossrs/srs/issues/46
		memcpy(m_s0s1s2 + 1537, c1, 1536);
	}
}
void CHandshakeBytes::create_c2() {
	m_c2 = new char[C2_Size];
	utils::randomGenerate(m_c2, C2_Size);
	write_4bytes(m_c2, (int32_t)::time(NULL));
	memcpy(m_c2 + 4, m_s0s1s2 + 1, 4);
}

void CHandshakeBytes::write_4bytes(char* bytes, int32_t value) {
	char* pp((char*)&value);
	*bytes++ = pp[3];
	*bytes++ = pp[2];
	*bytes++ = pp[1];
	*bytes++ = pp[0];
}
