#include "RtpBaseDemuxer.h"
#include "xstring.h"

#define SPACE_CHARS " \t\r\n"
CRtpBaseDemuxer::CRtpBaseDemuxer()
	: m_interleaved(-1)
	, m_clientPort(-1) {
}

CRtpBaseDemuxer::~CRtpBaseDemuxer() {
}

std::string CRtpBaseDemuxer::transport() const {
	if (m_interleaved >= 0) {
		return xstring<char>::format("RTP/AVP/TCP;unicast;interleaved=%d-%d", m_interleaved, m_interleaved + 1);
	} else {
		return xstring<char>::format("RTP/AVP/UDP;unicast;client_port=%d-%d", m_clientPort, m_clientPort + 1);
	}
}

void CRtpBaseDemuxer::get_word_until_chars(char *buf, int buf_size, const char *sep, const char **pp) {
	const char *p;
	char *q;
	p = *pp;
	p += strspn(p, SPACE_CHARS);
	q = buf;
	while (!strchr(sep, *p) && *p != '\0') {
		if ((q - buf) < buf_size - 1) *q++ = *p;
		p++;
	}
	if (buf_size > 0) *q = '\0';
	*pp = p;
}
void CRtpBaseDemuxer::get_word_sep(char *buf, int buf_size, const char *sep, const char **pp) {
	if (**pp == '/') (*pp)++;
	get_word_until_chars(buf, buf_size, sep, pp);
}
int CRtpBaseDemuxer::rtsp_next_attr_and_value(const char **p, char *attr, int attr_size, char *value, int value_size) {
	*p += strspn(*p, SPACE_CHARS);
	if (**p) {
		get_word_sep(attr, attr_size, "=", p);
		if (**p == '=') (*p)++;
		get_word_sep(value, value_size, ";", p);
		if (**p == ';') (*p)++;
		return 1;
	}
	return 0;
}
