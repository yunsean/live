#pragma once
#include "RtpBaseDemuxer.h"
#include "SourceInterface.h"
#include "Byte.h"

class CRtpH264Demuxer : public CRtpBaseDemuxer {
public:
	CRtpH264Demuxer();
	~CRtpH264Demuxer();

public:
	static bool support(const char* codec) { return strncmp(codec, "H264", 4) == 0; }

public:
	bool init(const fsdp_media_description_t* media);
	const char* control() const { return m_control.c_str(); }
	const uint8_t* extradata(int& size) const { size = m_extradataSize; return m_extradata.get(); }
	bool handle_packet(const uint8_t *buf, int len, uint32_t timestamp, uint16_t seq, int flags);
	uint32_t fourCC() const { return MKFCC(' ', 'C', 'V', 'A'); }

private:
	bool sdp_parse_fmtp_config_h264(const char *attr, const char *value);
	void parse_profile_level_id(const char *value);
	int base64_decode(const char* input, uint8_t* output);
	bool h264_parse_sprop_parameter_sets(const char *value);
	
private:
	bool h264_handle_aggregated_packet(const uint8_t *buf, int len, int skip_between);
	bool h264_handle_frag_packet(const uint8_t *buf, int len, int start_bit, int end_bit, const uint8_t *nal_header, int nal_header_len);
	bool h264_handle_packet_fu_a(const uint8_t *buf, int len);
	bool h264_handle_packet_normal(const uint8_t *buf, int len);

private:
	int m_cachePadding;
	float m_videoFrameRate;
	int m_clockRate;
	int m_packetizationMode;
	std::string m_control;
	uint8_t m_profileIdc;
	uint8_t m_profileIop;
	uint8_t m_levelIdc;
	std::unique_ptr<uint8_t[]> m_extradata;
	int m_extradataSize;
	CByte m_cache;
	uint64_t m_timestamp;
	uint32_t m_prevTimestamp;
};

