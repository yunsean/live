#include "RtpH264Demuxer.h"
#include "Writelog.h"
#include "Byte.h"
#include "xutils.h"

CRtpH264Demuxer::CRtpH264Demuxer()
	: m_cachePadding(10)
	, m_videoFrameRate(.0f)
	, m_clockRate(0)

	, m_extradata()
	, m_extradataSize(0)
	, m_cache()
	, m_timestamp(-1)
	, m_prevTimestamp(-1) {
}
CRtpH264Demuxer::~CRtpH264Demuxer() {
}

bool CRtpH264Demuxer::init(const fsdp_media_description_t* media) {
	m_videoFrameRate = fsdp_get_media_framerate(media);
	m_clockRate = fsdp_get_media_rtpmap_clock_rate(media, 0);
	if (m_clockRate < 1) m_clockRate = 1000;
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
		if (!sdp_parse_fmtp_config_h264(attr, value.get())) return false;
	}
	return true;
}

static const uint8_t start_sequence[] = { 0, 0, 0, 1 };
bool CRtpH264Demuxer::h264_handle_aggregated_packet(const uint8_t *buf, int len, int skip_between) {
	int total_length = 0;
	uint8_t* dst = NULL;
	// first we are going to figure out the total size
	for (int pass = 0; pass < 2; pass++) {
		const uint8_t *src = buf;
		int src_len = len;
		while (src_len > 2) {
			uint16_t nal_size = utils::uintFrom2BytesBE(src); // consume the length of the aggregate
			src += 2;
			src_len -= 2;
			if (nal_size <= src_len) {
				if (pass == 0) { // counting
					total_length += sizeof(start_sequence) + nal_size;
				} else { // copying
					memcpy(dst, start_sequence, sizeof(start_sequence));
					dst += sizeof(start_sequence);
					memcpy(dst, src, nal_size);
					dst += nal_size;
				}
			} else {
				return wlet(false, _T("nal size exceeds length: %d %d"), nal_size, src_len);
			}
			// eat what we handled
			src += nal_size + skip_between;
			src_len -= nal_size + skip_between;
		}
		if (pass == 0) { // now we know the total size of the packet (with the start sequences added) 
			m_cache.EnsureSize(total_length + m_cachePadding);
			dst = m_cache.GetData() + m_cachePadding;
		}
	}
	if (m_callback) m_callback(m_cache.GetData() + m_cachePadding, total_length, static_cast<uint32_t>(m_timestamp * 1000 / m_clockRate));
	return true;
}
bool CRtpH264Demuxer::h264_handle_frag_packet(const uint8_t *buf, int len, int start_bit, int end_bit, const uint8_t *nal_header, int nal_header_len) {
	if (start_bit) {
		m_cache.EnsureSize(m_cachePadding + sizeof(start_sequence) + nal_header_len + len);
		m_cache.SetSize(m_cachePadding);
	} else {
		m_cache.EnsureSize(m_cache.GetSize() + len, true);
	}
	uint8_t* ptr(m_cache.GetData() + m_cache.GetSize());
	if (start_bit) {
		memcpy(ptr, start_sequence, sizeof(start_sequence));
		ptr += sizeof(start_sequence);
		memcpy(ptr, nal_header, nal_header_len);
		ptr += nal_header_len;
	}
	memcpy(ptr, buf, len);
	ptr += len;
	m_cache.SetSize(static_cast<int>(ptr - m_cache.GetData()));
	if (end_bit) {
		if (m_callback) m_callback(m_cache.GetData() + m_cachePadding, m_cache.GetSize() - m_cachePadding, static_cast<uint32_t>(m_timestamp * 1000 / m_clockRate));
	}
	return true;
}
bool CRtpH264Demuxer::h264_handle_packet_fu_a(const uint8_t *buf, int len) {
	if (len < 3) return wlet(false, "Too short data for FU-A H.264 RTP packet");
	uint8_t fu_indicator = buf[0];
	uint8_t fu_header = buf[1];
	uint8_t start_bit = fu_header >> 7;
	uint8_t end_bit = (fu_header >> 6) & 0x01;
	uint8_t nal_type = fu_header & 0x1f;
	uint8_t nal = fu_indicator & 0xe0 | nal_type;
	buf += 2;
	len -= 2;
	return h264_handle_frag_packet(buf, len, start_bit, end_bit, &nal, 1);
}
bool CRtpH264Demuxer::h264_handle_packet_normal(const uint8_t *buf, int len) {
	if ((buf[0] & 0x1f) > 0x05) return true;
	int total_length = len + sizeof(start_sequence);
	m_cache.EnsureSize(total_length + m_cachePadding);
	memcpy(m_cache.GetData() + m_cachePadding, start_sequence, sizeof(start_sequence));
	memcpy(m_cache.GetData() + m_cachePadding + sizeof(start_sequence), buf, len);
	if (m_callback) m_callback(m_cache.GetData() + m_cachePadding, total_length, static_cast<uint32_t>(m_timestamp * 1000 / m_clockRate));
	return true;
}

bool CRtpH264Demuxer::handle_packet(const uint8_t *buf, int len, uint32_t timestamp, uint16_t seq, int flags) {
	uint8_t nal; 
	uint8_t type;
	int result = 0;
	if (!len) return wlet(false, _T("Empty H.264 RTP packet"));
	nal = buf[0];
	type = nal & 0x1f;
	if (type >= 1 && type <= 23) type = 1;
	if (m_prevTimestamp == -1 && m_timestamp == -1) {
		m_timestamp = timestamp;
	} else {
		m_timestamp += (timestamp - m_prevTimestamp);
	}
	m_prevTimestamp = timestamp;
	switch (type) {
	case 0:                    // undefined, but pass them through
	case 1:
		return h264_handle_packet_normal(buf, len);
	case 24:                   // STAP-A (one packet, multiple nals), consume the STAP-A NAL
		return h264_handle_aggregated_packet(buf + 1, len - 1, 0);
	case 25:                   // STAP-B
	case 26:                   // MTAP-16
	case 27:                   // MTAP-24
	case 29:                   // FU-B
		return wlet(false, "RTP H.264 NAL unit type %d", type);
	case 28:                   // FU-A (fragmented nal)
		return h264_handle_packet_fu_a(buf, len);
	case 30:                   // undefined
	case 31:                   // undefined
	default:
		return wlet(false, "Undefined type (%d)", type);
	}
}

bool CRtpH264Demuxer::sdp_parse_fmtp_config_h264(const char *attr, const char *value) {
	if (!strcmp(attr, "packetization-mode")) {
		m_packetizationMode = atoi(value);
		if (m_packetizationMode > 1) return wlet(false, _T("Interleaved RTP mode is not supported yet."));
	} else if (!strcmp(attr, "profile-level-id")) {
		if (strlen(value) == 6) parse_profile_level_id(value);
	} else if (!strcmp(attr, "sprop-parameter-sets")) {
		if (*value == 0 || value[strlen(value) - 1] == ',') return wlet(false, _T("Missing PPS in sprop-parameter-sets, ignoring"));
		if (!h264_parse_sprop_parameter_sets(value)) return wlet(false, _T("Parse extradata failed."));
	}
	return true;
}
int CRtpH264Demuxer::base64_decode(const char* input, uint8_t* output) {
	size_t length(strlen(input));
	static const char CHAR_63('+');
	static const char CHAR_64('-');
	static const char CHAR_PAD('=');
	union {
		unsigned char bytes[4];
		unsigned int block;
	} buffer;
	buffer.block = 0;
	int j(0);
	for (size_t i = 0; i < length; i++) {
		int m(i % 4);
		wchar_t x(input[i]);
		int val(0);
		if (x >= 'A' && x <= 'Z')val = x - 'A';
		else if (x >= 'a' && x <= 'z')val = x - 'a' + 'Z' - 'A' + 1;
		else if (x >= '0' && x <= '9')val = x - '0' + ('Z' - 'A' + 1) * 2;
		else if (x == CHAR_63)val = 62;
		else if (x == CHAR_64)val = 63;
		if (x != CHAR_PAD)buffer.block |= val << (3 - m) * 6;
		else m--;
		if (m == 3 || x == CHAR_PAD) {
			output[j++] = buffer.bytes[2];
			if (x != CHAR_PAD || m > 1) {
				output[j++] = buffer.bytes[1];
				if (x != CHAR_PAD || m > 2)output[j++] = buffer.bytes[0];
			}
			buffer.block = 0;
		}
		if (x == CHAR_PAD)break;
	}
	return j;
}
bool CRtpH264Demuxer::h264_parse_sprop_parameter_sets(const char *value) {
	char base64packet[1024];
	uint8_t decoded_packet[1024];
	int packet_size;
	static const uint8_t start_sequence[] = { 0, 0, 0, 1 };
	size_t dest_length = strlen(value) + sizeof(start_sequence) * 2;
	uint8_t* dest = new uint8_t[dest_length];
	int dest_offset = 0;
	while (*value) {
		char *dst = base64packet;
		while (*value && *value != ','
			&& (dst - base64packet) < sizeof(base64packet) - 1) {
			*dst++ = *value++;
		}
		*dst++ = '\0';
		if (*value == ',') value++;
		packet_size = base64_decode(base64packet, decoded_packet);
		if (packet_size > 0) {
			memcpy(dest + dest_offset, start_sequence, sizeof(start_sequence));
			dest_offset += sizeof(start_sequence);
			memcpy(dest + dest_offset, decoded_packet, packet_size);
			dest_offset += packet_size;
		}
	}
	m_extradata.reset(dest);
	m_extradataSize = dest_offset;
	return true;
}
void CRtpH264Demuxer::parse_profile_level_id(const char* value) {
	char buffer[3];
	buffer[0] = value[0];
	buffer[1] = value[1];
	buffer[2] = '\0';
	m_profileIdc = static_cast<uint8_t>((buffer, NULL, 16));
	buffer[0] = value[2];
	buffer[1] = value[3];
	m_profileIop = static_cast<uint8_t>(strtol(buffer, NULL, 16));
	buffer[0] = value[4];
	buffer[1] = value[5];
	m_levelIdc = static_cast<uint8_t>(strtol(buffer, NULL, 16));
}
