#include "RtpAacDemuxer.h"
#include "BitStream.h"
#include "Writelog.h"
#include "Byte.h"
#include "xutils.h"
#include "xsystem.h"

#define SPACE_CHARS				" \t\r\n"
#define RTP_MAX_PACKET_LENGTH	8192
#define MAX_AAC_HBR_FRAME_SIZE	8192
#define RTP_FLAG_MARKER			0x2 
CRtpAacDemuxer::CRtpAacDemuxer() {
}
CRtpAacDemuxer::~CRtpAacDemuxer() {
}

bool CRtpAacDemuxer::init(const fsdp_media_description_t* media) {
	int count = fsdp_get_media_fmtp_count(media);
	if (count < 1) return wlet(false, _T("not found ftmp line."));
	m_control = fsdp_get_media_control(media);
	if (m_control.empty()) return wlet(false, _T("h264 track has not control line."));
	const char* ftmp = fsdp_get_media_fmtp(media, 0);
	if (ftmp == nullptr) return wlet(false, _T("not found ftmp line."));
	while (*ftmp && *ftmp == ' ')ftmp++;                     // strip spaces
	while (*ftmp && *ftmp != ' ')ftmp++;                     // eat protocol identifier
	while (*ftmp && *ftmp == ' ')ftmp++;
	if (!*ftmp) return wlet(false, _T("not found ftmp line."));
	char attr[256];
	int value_size = static_cast<int>(strlen(ftmp) + 1);
	std::unique_ptr<char[]> value(new char[value_size]);
	while (rtsp_next_attr_and_value(&ftmp, attr, sizeof(attr), value.get(), value_size)) {
		if (!sdp_parse_fmtp_config_aac(attr, value.get())) return false;
	}
	return true;
}

bool CRtpAacDemuxer::rtp_parse_mp4_au(const uint8_t* buf, int len) {
	if (len < 2) return false;
	int au_headers_length = utils::uintFrom2BytesBE(buf);
	if (au_headers_length > RTP_MAX_PACKET_LENGTH) return false;
	m_au_headers_length_bytes = (au_headers_length + 7) / 8;
	buf += 2;
	len -= 2;
	if (len < m_au_headers_length_bytes) return false;
	CBitStream bs(buf, m_au_headers_length_bytes);
	int au_header_size = m_sizeLength + m_indexLength;
	if (au_header_size <= 0 || (au_headers_length % au_header_size != 0)) return false;
	m_nb_au_headers = au_headers_length / au_header_size;
	if (!m_au_headers ||m_au_headers_allocated < m_nb_au_headers) {
		m_au_headers = new AUHeaders[m_nb_au_headers];
		m_au_headers_allocated = m_nb_au_headers;
	}
	for (int i = 0; i < m_nb_au_headers; ++i) {
		m_au_headers[i].size = bs.read(m_sizeLength);
		m_au_headers[i].index = bs.read(m_indexLength);
	}
	return true;
}
int CRtpAacDemuxer::rtp_parse_packet(const uint8_t *buf, int len, uint32_t timestamp, uint16_t seq, int flags) {
	if (!buf) {
		if (m_cur_au_index > m_nb_au_headers) return wlet(-1, _T("Invalid parser state"));
		if (m_buf_size - m_buf_pos < m_au_headers[m_cur_au_index].size) return wlet(-1, _T("Invalid AU size"));
		if (m_callback) m_callback(m_buf + m_buf_pos, m_au_headers[m_cur_au_index].size, m_timestamp);
		m_buf_pos += m_au_headers[m_cur_au_index].size;
		m_cur_au_index++;
		if (m_cur_au_index == m_nb_au_headers) {
			m_buf_pos = 0;
			return 0;
		}
		return 1;
	}
	if (!rtp_parse_mp4_au(buf, len)) return wlet(-1, _T("Parse mp4 au failed."));
	buf += m_au_headers_length_bytes + 2;
	len -= m_au_headers_length_bytes + 2;
	if (m_nb_au_headers == 1 && len < m_au_headers[0].size) {
		if (!m_buf_pos) {
			if (m_au_headers[0].size > MAX_AAC_HBR_FRAME_SIZE) return wlet(-1, _T("Invalid AU size\n"));
			m_buf_size = m_au_headers[0].size;
			m_timestamp = timestamp;
		}
		if (m_timestamp != timestamp || m_au_headers[0].size != m_buf_size || m_buf_pos + len > MAX_AAC_HBR_FRAME_SIZE) {
			m_buf_pos = 0;
			m_buf_size = 0;
			return wlet(-1, _T("Invalid packet received\n"));
		}
		memcpy(&m_buf[m_buf_pos], buf, len);
		m_buf_pos += len;
		if (!(flags & RTP_FLAG_MARKER)) return false;
		if (m_buf_pos != m_buf_size) {
			m_buf_pos = 0;
			return wlet(false, _T("Missed some packets, discarding frame"));
		}
		if (m_callback) m_callback(m_buf, m_buf_size, m_timestamp);
		m_buf_pos = 0;
		return 0;
	}
	if (len < m_au_headers[0].size) return wlet(-1, _T("First AU larger than packet size"));
	int result = m_au_headers[0].size;
	if (m_callback) m_callback(buf, m_au_headers[0].size, m_timestamp);
	len -= m_au_headers[0].size;
	buf += m_au_headers[0].size;
	if (len > 0 && m_nb_au_headers > 1) {
		m_buf_size = std::xmin(len, static_cast<int>(sizeof(m_buf)));
		memcpy(m_buf, buf, m_buf_size);
		m_cur_au_index = 1;
		m_buf_pos = 0;
		return 1;
	}
	return 0;
}
bool CRtpAacDemuxer::handle_packet(const uint8_t *buf, int len, uint32_t timestamp, uint16_t seq, int flags) {
	int result = rtp_parse_packet(buf, len, timestamp, seq, flags);
	do {
		if (result < 0) return wlet(false, _T("handle aac packet failed."));
		else if (result == 0) return true;
		else result = rtp_parse_packet(nullptr, 0, 0, seq, flags);
	} while (true);
}

bool CRtpAacDemuxer::sdp_parse_fmtp_config_aac(const char *attr, const char *value) {
	if (!strcmp(attr, "profile-level-id")) {
		m_profileIdc = atoi(value);
	} else if (!strcmp(attr, "sizelength")) {
		m_sizeLength = atoi(value);
	} else if (!strcmp(attr, "indexlength")) {
		m_indexLength = atoi(value);
	} else if (!strcmp(attr, "indexdeltalength")) {
		m_indexDeltaLength = atoi(value);
	} else if (!strcmp(attr, "mode")) {
		m_mode = value;
	}  else if (!strcmp(attr, "config")) {
		if (!parse_fmtp_config(value)) return wlet(false, _T("parse aac config failed."));
	}
	return true;
}

static int hex_to_data(uint8_t *data, const char *p) {
	int c, len, v;
	len = 0;
	v = 1;
	for (;;) {
		p += strspn(p, SPACE_CHARS);
		if (*p == '\0') break;
		c = toupper((unsigned char)*p++);
		if (c >= '0' && c <= '9') c = c - '0';
		else if (c >= 'A' && c <= 'F') c = c - 'A' + 10;
		else break;
		v = (v << 4) | c;
		if (v & 0x100) {
			if (data) data[len] = v;
			len++;
			v = 1;
		}
	}
	return len;
}

bool CRtpAacDemuxer::parse_fmtp_config(const char *value) {
	m_extradataSize = hex_to_data(NULL, value);
	m_extradata.reset(new uint8_t[m_extradataSize]);
	hex_to_data(m_extradata.get(), value);
	return true;
}
