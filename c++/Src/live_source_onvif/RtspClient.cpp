#include <memory>
#include <sstream>
#include "net.h"
#include "os.h"
#include "RtspClient.h"
#include "Writelog.h"
#include "ConsoleUI.h"
#include "xstring.h"
#include "xsystem.h"
#include "xutils.h"
#include "MD5CodeArith.h"
#include "freesdp/freesdp/freesdp.h"

CRtspClient::CRtspClient(event_base* base, ICallback* callback, const char* username, const char* password)
	: m_callback(callback)
	, m_evbase(base)
	, m_username(username)
	, m_password(password)
	, m_session(xstring<char>::format("%d", std::tickCount()))
	, m_seqId(0)
	, m_bev(bufferevent_free, nullptr)
	, m_url()
	, m_rtspStage(Connecting)
	, m_readRtspWhileStreaming(false)
	, m_contentLength(0)
	, m_cacheSize(512)
	, m_cache(new(std::nothrow) char[m_cacheSize])

	, m_videoTrack(nullptr)
	, m_audioTrack(nullptr) 

	, m_rtpChannel(0)
	, m_rtpLength(0)
	, m_latestHeartbeat(0) {
}
CRtspClient::~CRtspClient() {
}

bool CRtspClient::feed(const char* url) {
	//url = "rtsp://184.72.239.149/vod/mp4://BigBuckBunny_175k.mov";
	CSmartHdr<evhttp_uri*, void> uri(evhttp_uri_parse_with_flags(url, EVHTTP_URI_NONCONFORMANT), evhttp_uri_free);
	if (uri == nullptr) return wlet(false, _T("Parse url [%s] failed."), xtstring(url).c_str());
	const char* host = evhttp_uri_get_host(uri);
	int port = evhttp_uri_get_port(uri);
	if (port == -1) port = 554;

	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	if (inet_aton(host, &server_addr.sin_addr) == 0) return wlet(false, _T("Invalid url."));
	struct bufferevent* bev = bufferevent_socket_new(m_evbase, -1, BEV_OPT_CLOSE_ON_FREE);
	bufferevent_setcb(bev, on_read, nullptr, on_event, this);
	int result = bufferevent_socket_connect(bev, (struct sockaddr *)&server_addr, sizeof(server_addr));
	if (result != 0) return wlet(false, _T("connect to server failed."));
	bufferevent_enable(bev, EV_READ | EV_WRITE);
	m_url = url;
	m_bev = bev;
	return true;
}

void CRtspClient::on_read(struct bufferevent* bev, void* context) {
	CRtspClient* thiz = reinterpret_cast<CRtspClient*>(context);
	thiz->on_read(bev);
}
void CRtspClient::on_read(struct bufferevent* bev) {
	evbuffer* input = bufferevent_get_input(bev);
	if (m_rtspStage == Streaming && !m_readRtspWhileStreaming) {
		while (true) {
			size_t size = evbuffer_get_length(input);
			if (m_rtpLength > 0) {
				if (static_cast<int>(size) < m_rtpLength) return;
				onRtp();
				m_rtpLength = 0;
			} else {
				if (size < 4) return;
				uint8_t header[4];
				const static evbuffer_ptr range = { 4 };
				const static char RtspHeader[] = { 'R', 'T', 'S', 'P' };
				evbuffer_ptr pos = evbuffer_search_range(input, RtspHeader, 4, nullptr, &range);
				if (pos.pos == 0) {
					m_readRtspWhileStreaming = true;
					break;
				} else {
					bufferevent_read(m_bev, header, 4);
					m_rtpChannel = header[1];
					m_rtpLength = utils::uintFrom2BytesBE(header + 2);
				}
			}
		}
	} 
	while (true) {
		if (m_contentLength > 0) {
			size_t size = evbuffer_get_length(input);
			if (static_cast<int>(size) < m_contentLength) return;
			else break;
		} else {
			const static char eof_flag[] = "\r\n\r\n";
			evbuffer_ptr eof = evbuffer_search(input, eof_flag, 4, NULL);
			if (eof.pos < 0) return;
			if (!parse_header(eof.pos + 4)) return onError(_T("RSTPÓ¦´ð´íÎó"));
			std::string contentLength = m_headers["Content-Length"];
			if (!contentLength.empty()) {
				m_contentLength = atoi(contentLength.c_str());
			} else {
				m_contentLength = 0;
				break;
			}
		}
	}

	int statusCode = atoi(m_headers["StatusCode"].c_str());
	if (statusCode == 401 && !onUnauthorized()) return onError(xtstring(_T("RTSPÇëÇó´íÎó: ")) + m_headers["Status"]), wle(_T("rtsp request failed£¬ status=%d, url=%s"), m_rtspStage, xtstring(m_url).c_str());
	else if (statusCode == 401) m_rtspStage = CRtspClient::Options;
	else if (statusCode != 200) return onError(xtstring(_T("RTSPÇëÇó´íÎó: ")) + m_headers["Status"]), wle(_T("rtsp request failed£¬ status=%d, url=%s"), m_rtspStage, xtstring(m_url).c_str());

	switch (m_rtspStage) {
	case CRtspClient::Options: onOptions(); break;
	case CRtspClient::Describe: onDescribe(); break;
	case CRtspClient::SetupVideo: onSetupVideo(); break;
	case CRtspClient::SetupAudio: onSetupAudio(); break;
	case CRtspClient::Play: onPlay(); break;
	case CRtspClient::Streaming: m_readRtspWhileStreaming = false;  break;
	}
	m_contentLength = 0;
}
void CRtspClient::on_event(struct bufferevent* bev, short what, void* context) {
	CRtspClient* thiz = reinterpret_cast<CRtspClient*>(context);
	thiz->on_event(bev, what);
}
void CRtspClient::on_event(struct bufferevent* bev, short what) {
	if (what == BEV_EVENT_CONNECTED) {
		onConnect();
	} else if (what == BEV_EVENT_EOF) {
		onError(_T("¶ÁÈ¡RTSPÁ÷´íÎó"));
	}
}

void CRtspClient::onConnect() {
	std::string body = format("OPTIONS %s RTSP/1.0\r\n"
		"CSeq: %u\r\n"
		"User-Agent: Lavf56.7.104\r\n"
		"\r\n", m_url.c_str(), m_seqId++);
	bufferevent_write(m_bev, body.c_str(), body.length());
	m_rtspStage = Options;
}
void CRtspClient::onOptions() {
	std::string body = format("DESCRIBE %s RTSP/1.0\r\n"
		"Accept: application/sdp\r\n"
		"%s"
		"CSeq: %u\r\n"
		"User-Agent: Lavf56.7.104\r\n"
		"\r\n", m_url.c_str(), authorize("DESCRIBE", m_url).c_str(), m_seqId++);
	bufferevent_write(m_bev, body.c_str(), body.length());
	m_rtspStage = Describe;
}
void CRtspClient::onDescribe() {
	if (m_cacheSize < m_contentLength) {
		m_cacheSize = (m_contentLength / 512) * 512 + 512;
		m_cache.reset(new char[m_cacheSize]);
	}
	size_t read = bufferevent_read(m_bev, m_cache.get(), m_contentLength);
	m_cache.get()[read] = '\0';
	if (!parse_sdp(m_cache.get())) return onError(_T("½âÎöSDP´íÎó")), wle(_T("Parse sdp failed, url=%s"), xtstring(m_url).c_str());
	if (m_videoTrack.get() != nullptr) {
		std::string body = format("SETUP %s/%s RTSP/1.0\r\n"
			"%s"
			"Transport: %s\r\n"
			"x-Dynamic-Rate: 0\r\n"
			"CSeq: %u\r\n"
			"User-Agent: Lavf56.7.104\r\n"
			"Session: %s\r\n"
			"\r\n", m_url.c_str(), m_videoTrack.get()->control(), 
			authorize("SETUP", m_url + "/" + m_videoTrack.get()->control()).c_str(),
			m_videoTrack.get()->transport().c_str(), m_seqId++, m_session.c_str());
		bufferevent_write(m_bev, body.c_str(), body.length());
		m_rtspStage = SetupVideo;
	} else {
		onSetupVideo();
	}
}
void CRtspClient::onSetupVideo() {
	if (m_audioTrack.get() != nullptr) {
		std::string body = format("SETUP %s/%s RTSP/1.0\r\n"
			"%s"
			"Transport: %s\r\n"
			"x-Dynamic-Rate: 0\r\n"
			"CSeq: %u\r\n"
			"User-Agent: Lavf56.7.104\r\n"
			"Session: %s\r\n"
			"\r\n", m_url.c_str(), m_audioTrack.get()->control(),
			authorize("SETUP", m_url + "/" + m_audioTrack.get()->control()).c_str(),
			m_audioTrack.get()->transport().c_str(), m_seqId++, m_session.c_str());
		bufferevent_write(m_bev, body.c_str(), body.length());
		m_rtspStage = SetupAudio;
	} else {
		onSetupAudio();
	}
}
void CRtspClient::onSetupAudio() {
	std::string body = format("PLAY %s RTSP/1.0\r\n"
		"Range: npt=0.000-\r\n"
		"%s"
		"CSeq: %u\r\n"
		"User-Agent: Lavf56.7.104\r\n"
		"Session: %s\r\n"
		"\r\n", m_url.c_str(), authorize("PLAY", m_url).c_str(), m_seqId++, m_session.c_str());
	bufferevent_write(m_bev, body.c_str(), body.length());
	m_rtspStage = Play;
}
void CRtspClient::onPlay() {
	m_rtspStage = Streaming;
	m_latestHeartbeat = std::tickCount();
	if (m_callback != NULL && m_videoTrack.get() != nullptr) {
		int size = 0;
		const uint8_t* data = m_videoTrack.get()->extradata(size);
		if (size > 0 && data != nullptr) m_callback->onVideoConfig(m_videoTrack.get()->fourCC(), data, size);
	}
	if (m_callback != NULL && m_audioTrack.get() != nullptr) {
		int size = 0;
		const uint8_t* data = m_audioTrack.get()->extradata(size);
		if (size > 0 && data != nullptr) m_callback->onAudioConfig(m_audioTrack.get()->fourCC(),  data, size);
	}
}
void CRtspClient::onRtp() {
	if (m_cacheSize < m_rtpLength) {
		m_cacheSize = (m_rtpLength / 512) * 512 + 512;
		m_cache.reset(new char[m_cacheSize]);
	}
	size_t read = bufferevent_read(m_bev, m_cache.get(), m_rtpLength);
	m_cache.get()[read] = '\0';
	if (!parse_packet(reinterpret_cast<uint8_t*>(m_cache.get()), static_cast<int>(read))) return onError(_T("½âÎöRTPÊý¾Ý´íÎó."));
	if (std::tickCount() - m_latestHeartbeat > 60 * 1000) {
		std::string body = format("GET_PARAMETER %s RTSP/1.0\r\n"
			"CSeq: %u\r\n"
			"User-Agent: Lavf56.7.104\r\n"
			"Session: %s\r\n"
			"\r\n", m_url.c_str(), m_seqId++, m_session.c_str());
		bufferevent_write(m_bev, body.c_str(), body.length());
		m_latestHeartbeat = std::tickCount();
	}
}

void CRtspClient::onError(LPCTSTR reason) {
	if (m_callback != nullptr) m_callback->onError(reason);
}
bool CRtspClient::onUnauthorized() {
	if (m_username.empty() && m_password.empty()) return wlet(false, _T("Rtsp server need authorized, but there has not username or password."));
	xstring<char> authorization(m_headers["WWW-Authenticate"]);
	authorization.Trim();
	int pos(authorization.Find(" "));
	if (pos < 0) return wlet(false, "Need authorized, but server did not return an Authorization: %s", authorization.c_str());
	xstring<char> mode(authorization.Left(pos));
	if (mode.CompareNoCase("Basic") == 0) {
		std::string str(m_username + ":" + m_password);
		m_basic = "Authorization: Basic " + CByte::toBase64(reinterpret_cast<const unsigned char*>(str.c_str()), static_cast<int>(str.length())).toString() + "\r\n";
		m_nonce = "";
	} else if (mode.CompareNoCase("Digest") == 0) {
		authorization += " ";
		int start(authorization.Find("realm="));
		int end(authorization.Find(" ", start + 5));
		if (start < 0 || end < 0) return wlet(false, "Need authorized, but server did not return an valid Authorization: %s", authorization.c_str());
		m_realm = authorization.Mid(start + 6, end - start - 6);
		start = authorization.Find("nonce=");
		end = authorization.Find(" ", start + 1);
		if (start < 0 || end < 0) return wlet(false, "Need authorized, but server did not return an valid Authorization: %s", authorization.c_str());
		m_nonce = authorization.Mid(start + 6, end - start - 6);
		m_basic = "";
	}
	return true;
}

#define RTP_FLAG_KEY    0x1 ///< RTP packet contains a keyframe
#define RTP_FLAG_MARKER 0x2 ///< RTP marker bit was set for this packet
bool CRtspClient::parse_packet(const uint8_t* buf, int len) {
	int flags = 0;
	int csrc = buf[0] & 0x0f;
	int ext = buf[0] & 0x10;
	uint8_t payload_type = buf[1] & 0x7f;
	if (buf[1] & 0x80) flags |= RTP_FLAG_MARKER;
	int seq = utils::uintFrom2BytesLE(buf + 2);
	uint32_t timestamp = utils::uintFrom4BytesBE(buf + 4);
	unsigned int ssrc = utils::uintFrom4BytesBE(buf + 8);
	if (buf[0] & 0x20) {
		int padding = buf[len - 1];
		if (len >= 12 + padding) len -= padding;
	}
	len -= 12 + 4 * csrc;
	buf += 12 + 4 * csrc;
	if (ext) {	//RFC 3550 Section 5.3.1 RTP Header Extension handling
		if (len < 4) 
			return wlet(true, _T("illegal packet."));
		ext = (utils::uintFrom2BytesLE(buf + 2) + 1) << 2; // calculate the header extension length (stored as number of 32-bit words) 
		if (len < ext) 
			return wlet(true, _T("illegal packet: len[%d] < ext[%d], %02d%02d%02d%02d"), len, ext, (int)buf[0], (int)buf[1], (int)buf[2], (int)buf[3]);
		len -= ext;	//skip past RTP header extension
		buf += ext;
	}
	CRtpBaseDemuxer* demuxer = nullptr;
	bool isAudio = false;
	if (m_videoTrack.get() && m_rtpChannel == m_videoTrack.get()->interleaved()) {
		demuxer = m_videoTrack.get();
		isAudio = false;
	} else if (m_audioTrack.get() && m_rtpChannel == m_audioTrack.get()->interleaved()) {
		demuxer = m_audioTrack.get();
		isAudio = true;
	} 
	if (demuxer != nullptr && !demuxer->handle_packet(buf, len, timestamp, seq, flags)) {
		return wlet(true, _T("parse rtp packet failed."));
	}
	return true;
}
bool CRtspClient::parse_sdp(const char* sdp) {
	auto dsc = fsdp_description_new();
	auto result = fsdp_parse(sdp, dsc);
	if (result != FSDPE_OK) return wlet(false, _T("parse sdp failed."));
	auto count = static_cast<int>(fsdp_get_media_count(dsc));
	for (int i = 0; i < count; i++) {
		const fsdp_media_description_t* media = fsdp_get_media(dsc, i);
		if (media == nullptr) continue;
		const char* codec = fsdp_get_media_rtpmap_encoding_name(media, 0);
		if (codec == nullptr) continue;
		if (CRtpH264Demuxer::support(codec)) {
			std::unique_ptr<CRtpH264Demuxer> track(new CRtpH264Demuxer());
			if (!track.get()->init(media)) return false;
			track.get()->setCallback([this](const uint8_t* data, int size, uint32_t timecode) {
				if (m_callback != nullptr) m_callback->onVideoFrame(timecode, data, size);
			});
			track->setInterleaved(0);
			m_videoTrack = std::move(track);
		} else if (CRtpAacDemuxer::support(codec)) {
			std::unique_ptr<CRtpAacDemuxer> track(new CRtpAacDemuxer());
			if (!track.get()->init(media)) return false;
			track.get()->setCallback([this](const uint8_t* data, int size, uint32_t timecode) {
				if (m_callback != nullptr) m_callback->onAudioFrame(timecode, data, size);
			});
			track->setInterleaved(2);
			m_audioTrack = std::move(track);
		}
	}
	if (m_videoTrack.get() == nullptr) return wlet(false, _T("Can not found the h264 video track."));

	return true;
}
std::string CRtspClient::format(const char* fmt, ...) {
	va_list argList;
	va_start(argList, fmt);
	int len = vsnprintf(nullptr, 0, fmt, argList) + 1;
	if (len > 0) {
		std::unique_ptr<char[]> buf(new char[len]);
		vsnprintf_s(buf.get(), len, len, fmt, argList);
		va_end(argList);
		return std::string(buf.get(), buf.get() + len - 1);
	}
	va_end(argList);
	return "";
}
std::string CRtspClient::authorize(const char* method, const std::string& url) {
	if (!m_basic.empty()) return m_basic;
	if (m_nonce.empty()) return "";
	CMD5CodeArith md5;
	unsigned char dh1[16];
	unsigned char dh2[16];
	unsigned char dh[16];
	md5.Initialize();
	md5.UpdateData(reinterpret_cast<const unsigned char*>(m_username.c_str()), static_cast<int>(m_username.length()));
	md5.UpdateData(reinterpret_cast<const unsigned char*>(":"), 1);
	md5.UpdateData(reinterpret_cast<const unsigned char*>(m_realm.c_str()), static_cast<int>(m_realm.length()));
	md5.UpdateData(reinterpret_cast<const unsigned char*>(":"), 1);
	md5.UpdateData(reinterpret_cast<const unsigned char*>(m_password.c_str()), static_cast<int>(m_password.length()));
	md5.Finalize(dh1);

	md5.Initialize();
	md5.UpdateData(reinterpret_cast<const unsigned char*>(method), static_cast<int>(strlen(method)));
	md5.UpdateData(reinterpret_cast<const unsigned char*>(":"), 1);
	md5.UpdateData(reinterpret_cast<const unsigned char*>(url.c_str()), static_cast<int>(url.length()));
	md5.Finalize(dh2);

	md5.Initialize();
	md5.UpdateData(dh1, 16);
	md5.UpdateData(reinterpret_cast<const unsigned char*>(":"), 1);
	md5.UpdateData(reinterpret_cast<const unsigned char*>(m_nonce.c_str()), static_cast<int>(m_nonce.length()));
	md5.UpdateData(reinterpret_cast<const unsigned char*>(":"), 1);
	md5.UpdateData(dh2, 16);
	md5.Finalize(dh);

	std::string response = CByte::toHex(dh, 16, 0).toString();
	xstring<char> result;
	result.Format("Authorization: Digest username=\"%s\",realm=\"%s\", nonce=\"%s\",uri=\"%s\"", m_username.c_str(), m_realm.c_str(), m_nonce.c_str(), url.c_str());
	return result;
}
bool CRtspClient::parse_header(size_t length) {
	if (length > m_cacheSize) {
		m_cacheSize = (length / 512) * 512 + 512;
		m_cache.reset(new char[m_cacheSize]);
	}
	size_t read = bufferevent_read(m_bev, m_cache.get(), length);
	if (read == 0) return false;
	m_cache.get()[read] = '\0';
	const char* head = m_cache.get();
	const char* end = m_cache.get() + read;
	const char* tail = head;

	m_headers.clear();
	while (tail != end && *tail != ' ') ++tail;
	m_headers["RtspVersion"] = std::string(head, tail);
	while (tail != end && *tail == ' ') ++tail;
	head = tail;
	while (tail != end && *tail != ' ') ++tail;
	m_headers["StatusCode"] = std::string(head, tail);
	while (tail != end && *tail == ' ') ++tail;
	head = tail;
	while (tail != end && *tail != '\r') ++tail;
	m_headers["Status"] = std::string(head, tail);
	head = tail;
	while (tail != end && (*tail == '\r' || *tail == '\n')) ++tail;
	head = tail;
	while (head != end && *head != '\r') {
		while (tail != end && *tail != '\r' && *tail != '\n') ++tail;
		const char* colon = reinterpret_cast<const char*>(::memchr(head, ':', tail - head));
		if (colon == NULL) break;
		const char *value = colon + 1;
		while (value != tail && *value == ' ') ++value;
		m_headers[std::string(head, colon)] = std::string(value, tail);
		while (tail != end && (*tail == '\r' || *tail == '\n')) ++tail;
		head = tail;
	}
	xstring<char> session(m_headers["Session"]);
	if (!session.empty()) {
		session.Replace(";", " ");
		int pos(session.Find(" "));
		if (pos > 0) m_session = session.Left(pos);
		else m_session = session;
	}
	return true;
}


