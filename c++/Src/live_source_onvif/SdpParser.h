#pragma once
#include <string>
#include <list>
#include <memory>
#include <vector>

class CSdpParser {
public:
	CSdpParser();
	~CSdpParser();

public:
	bool parse(const char* sdp);

protected:
	enum StreamType { None, Video, Audio, Data, Subtitle };

protected:
	struct SDPParseState {
		std::string default_ip;
		int default_ttl;
		int skip_media;
		int nb_default_include_source_addrs;
		struct RTSPSource** default_include_source_addrs;
		int nb_default_exclude_source_addrs;
		struct RTSPSource** default_exclude_source_addrs;
		int seen_rtpmap;
		int seen_fmtp;
		char delayed_fmtp[2048];
	};
	struct RTSPStream {
		int stream_index;
		int interleaved_min;
		int interleaved_max;
		char control_url[1024];
		int sdp_port;
		std::string sdp_ip;
		int nb_include_source_addrs;
		struct RTSPSource **include_source_addrs; /**< Source-specific multicast include source IP addresses (from SDP content) */
		int nb_exclude_source_addrs; /**< Number of source-specific multicast exclude source IP addresses (from SDP content) */
		struct RTSPSource **exclude_source_addrs; /**< Source-specific multicast exclude source IP addresses (from SDP content) */
		int sdp_ttl;
		int sdp_payload_type;
		RTPDynamicProtocolHandler* dynamic_handler;
		PayloadContext *dynamic_protocol_context;
		int feedback;
		uint32_t ssrc;
		char crypto_suite[40];
		char crypto_params[100];
	};

protected:
	void sdp_parse_line(SDPParseState* s1, int letter, const char* buf);

private:
	std::string m_title;
	std::string m_comment;
	int nb_streams;
	int nb_rtsp_streams;

	bool skip_media;
	bool seen_fmtp;
	bool seen_rtpmap;

	std::vector<std::shared_ptr<RTSPStream>> m_streams;
};

