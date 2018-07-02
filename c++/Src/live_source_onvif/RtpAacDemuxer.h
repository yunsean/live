#pragma once
#include <memory>
#include "RtpBaseDemuxer.h"
#include "SourceInterface.h"
#include "SmartArr.h"

class CRtpAacDemuxer : public CRtpBaseDemuxer {
public:
	CRtpAacDemuxer();
	~CRtpAacDemuxer();

public:
	static bool support(const char* codec) { return strncmp(codec, "mpeg4-generic", 13) == 0; }

public:
	bool init(const fsdp_media_description_t* media);
	const char* control() const { return m_control.c_str(); }
	const uint8_t* extradata(int& size) const { size = m_extradataSize; return m_extradata.get(); }
	bool handle_packet(const uint8_t *buf, int len, uint32_t timestamp, uint16_t seq, int flags);
	uint32_t fourCC() const { return MKFCC(' ', 'C', 'A', 'A'); }

private:
	bool sdp_parse_fmtp_config_aac(const char *attr, const char *value);
	bool parse_fmtp_config(const char *value);
	bool rtp_parse_mp4_au(const uint8_t* buf, int len);
	int rtp_parse_packet(const uint8_t *buf, int len, uint32_t timestamp, uint16_t seq, int flags);

private:
	struct AUHeaders {
		int size;
		int index;
	};
	
private:
	int m_clockRate;
	std::string m_control;
	uint8_t m_profileIdc;
	int m_indexDeltaLength;

	int m_sizeLength;
	int m_indexLength;
	std::string m_mode;
	std::unique_ptr<uint8_t[]> m_extradata;
	int m_extradataSize;

	int m_au_headers_length_bytes;
	int m_nb_au_headers;
	int m_au_headers_allocated;
	int m_cur_au_index;
	CSmartArr<AUHeaders> m_au_headers;
	int m_buf_pos;
	int m_buf_size;
	uint32_t m_timestamp;
	uint8_t m_buf[8191];
};

