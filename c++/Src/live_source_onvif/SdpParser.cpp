#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SdpParser.h"

#define SPACE_CHARS	" "
#define FF_DYNARRAY_ADD(av_size_max, av_elt_size, av_array, av_size, \
                        av_success, av_failure) \
    do { \
        size_t av_size_new = (av_size); \
        if (!((av_size) & ((av_size) - 1))) { \
            av_size_new = (av_size) ? (av_size) << 1 : 1; \
            if (av_size_new > (av_size_max) / (av_elt_size)) { \
                av_size_new = 0; \
            } else { \
                void *av_array_new = \
                    realloc((av_array), av_size_new * (av_elt_size)); \
                if (!av_array_new) \
                    av_size_new = 0; \
                else \
                    (av_array) = av_array_new; \
            } \
        } \
        if (av_size_new) { \
            { av_success } \
            (av_size)++; \
        } else { \
            av_failure \
        } \
    } while (0)

CSdpParser::CSdpParser() {
}
CSdpParser::~CSdpParser() {
}

static void get_word_until_chars(char *buf, int buf_size, const char *sep, const char **pp) {
	const char* p = *pp;
	p += strspn(p, SPACE_CHARS);
	char* q = buf;
	while (!strchr(sep, *p) && *p != '\0') {
		if ((q - buf) < buf_size - 1) *q++ = *p;
		p++;
	}
	if (buf_size > 0) *q = '\0';
	*pp = p;
}
static void get_word_sep(char *buf, int buf_size, const char *sep, const char **pp) {
	if (**pp == '/') (*pp)++;
	get_word_until_chars(buf, buf_size, sep, pp);
}
static void get_word(char *buf, int buf_size, const char **pp) {
	get_word_until_chars(buf, buf_size, SPACE_CHARS, pp);
}
int av_strstart(const char *str, const char *pfx, const char **ptr) {
	while (*pfx && *pfx == *str) {
		pfx++;
		str++;
	}
	if (!*pfx && ptr) *ptr = str;
	return !*pfx;
}
size_t av_strlcpy(char *dst, const char *src, size_t size) {
	size_t len = 0;
	while (++len < size && *src) *dst++ = *src++;
	if (len <= size) *dst = 0;
	return len + strlen(src) - 1;
}
void av_dynarray_add(void *tab_ptr, int *nb_ptr, void *elem) {
	void **tab;
	memcpy(&tab, tab_ptr, sizeof(tab));
	FF_DYNARRAY_ADD(INT_MAX, sizeof(*tab), tab, *nb_ptr, {
		tab[*nb_ptr] = elem;
	memcpy(tab_ptr, &tab, sizeof(tab));
	}, {
		*nb_ptr = 0;
	av_freep(tab_ptr);
	});
}
#define dynarray_add(tab, nb_ptr, elem)\
do {\
    av_dynarray_add((tab), nb_ptr, (elem));\
} while(0)
static void copy_default_source_addrs(struct RTSPSource **addrs, int count, struct RTSPSource ***dest, int *dest_count) {
	RTSPSource *rtsp_src, *rtsp_src2;
	int i;
	for (i = 0; i < count; i++) {
		rtsp_src = addrs[i];
		rtsp_src2 = malloc(sizeof(*rtsp_src2));
		if (!rtsp_src2)
			continue;
		memcpy(rtsp_src2, rtsp_src, sizeof(*rtsp_src));
		dynarray_add(dest, dest_count, rtsp_src2);
	}
}

void CSdpParser::sdp_parse_line(SDPParseState* s1, int letter, const char* buf) {
	printf("%s\n", buf);
	char buf1[64], st_type[64];
	const char *p;
	int payload_type;
	std::string sdp_ip;
	RTSPStream* rtspStream;
	int ttl;
	p = buf;
	if (s1->skip_media && letter != 'm')return;
	switch (letter) {
	case 'c':
		get_word(buf1, sizeof(buf1), &p);
		if (strcmp(buf1, "IN") != 0)return;
		get_word(buf1, sizeof(buf1), &p);
		if (strcmp(buf1, "IP4") && strcmp(buf1, "IP6"))return;
		get_word_sep(buf1, sizeof(buf1), "/", &p);
		sdp_ip = p;
		ttl = 16;
		if (*p == '/') {
			p++;
			get_word_sep(buf1, sizeof(buf1), "/", &p);
			ttl = atoi(buf1);
		}
		if (nb_streams == 0) {
			s1->default_ip = sdp_ip;
			s1->default_ttl = ttl;
		} else {
			rtspStream = m_streams.at(nb_rtsp_streams - 1).get();
			rtspStream->sdp_ip = sdp_ip;
			rtspStream->sdp_ttl = ttl;
		}
		break;
	case 's':
		m_title = p;
		break;
	case 'i':
		m_comment = p;
		break;
	case 'm':
		s1->skip_media = 0;
		s1->seen_fmtp = 0;
		s1->seen_rtpmap = 0;
		StreamType codec_type = None;
		get_word(st_type, sizeof(st_type), &p);
		if (!strcmp(st_type, "audio")) codec_type = Audio;
		else if (!strcmp(st_type, "video")) codec_type = Video;
		else if (!strcmp(st_type, "application")) codec_type = Data;
		else if (!strcmp(st_type, "text")) codec_type = Subtitle;
		if (codec_type == None) {
			s1->skip_media = 1;
			return;
		}
		rtspStream = new RTSPStream();
		if (!rtspStream) return;
		rtspStream->stream_index = -1;
		rtspStream->sdp_ip = s1->default_ip;
		rtspStream->sdp_ttl = s1->default_ttl;
		get_word(buf1, sizeof(buf1), &p); /* port */
		rtspStream->sdp_port = atoi(buf1);
		get_word(buf1, sizeof(buf1), &p); /* protocol */
		if (strstr(buf1, "/AVPF") || strstr(buf1, "/SAVPF")) rtspStream->feedback = 1;
		get_word(buf1, sizeof(buf1), &p); /* format list */
		rtspStream->sdp_payload_type = atoi(buf1);
// 		st = avformat_new_stream(s, NULL);
// 		if (!st) return;
//		st->id = nb_rtsp_streams - 1;
		rtspStream->stream_index = st->index;
		st->codecpar->codec_type = codec_type;
		if (rtspStream->sdp_payload_type < RTP_PT_PRIVATE) {
			RTPDynamicProtocolHandler *handler;
			ff_rtp_get_codec_info(st->codecpar, rtsp_st->sdp_payload_type);
			if (st->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && st->codecpar->sample_rate > 0)
				avpriv_set_pts_info(st, 32, 1, st->codecpar->sample_rate);
			handler = ff_rtp_handler_find_by_id(rtsp_st->sdp_payload_type, st->codecpar->codec_type);
			init_rtp_handler(handler, rtsp_st, st);
			finalize_rtp_handler_init(s, rtsp_st, st);
		}
		if (rt->default_lang[0]) av_dict_set(&st->metadata, "language", rt->default_lang, 0);
		av_strlcpy(rtspStream->control_url, rt->control_uri, sizeof(rtsp_st->control_url));
		break;
	case 'a':
		if (av_strstart(p, "control:", &p)) {
			if (nb_streams == 0) {
				if (!strncmp(p, "rtsp://", 7)) av_strlcpy(rt->control_uri, p, sizeof(rt->control_uri));
			} else {
				char proto[32];
				rtspStream = m_streams.at(nb_rtsp_streams - 1).get();
				av_url_split(proto, sizeof(proto), NULL, 0, NULL, 0, NULL, NULL, 0, p);
				if (proto[0] == '\0') {
					if (rtsp_st->control_url[strlen(rtsp_st->control_url) - 1] != '/')
						av_strlcat(rtsp_st->control_url, "/", sizeof(rtsp_st->control_url));
					av_strlcat(rtsp_st->control_url, p, sizeof(rtsp_st->control_url));
				} else
					av_strlcpy(rtsp_st->control_url, p, sizeof(rtsp_st->control_url));
			}
		} else if (av_strstart(p, "rtpmap:", &p) && s->nb_streams > 0) {
			get_word(buf1, sizeof(buf1), &p);
			payload_type = atoi(buf1);
			rtsp_st = rt->rtsp_streams[rt->nb_rtsp_streams - 1];
			if (rtsp_st->stream_index >= 0) {
				st = s->streams[rtsp_st->stream_index];
				sdp_parse_rtpmap(s, st, rtsp_st, payload_type, p);
			}
			s1->seen_rtpmap = 1;
			if (s1->seen_fmtp) {
				parse_fmtp(s, rt, payload_type, s1->delayed_fmtp);
			}
		} else if (av_strstart(p, "fmtp:", &p) ||
			av_strstart(p, "framesize:", &p)) {
			get_word(buf1, sizeof(buf1), &p);
			payload_type = atoi(buf1);
			if (s1->seen_rtpmap) {
				parse_fmtp(s, rt, payload_type, buf);
			} else {
				s1->seen_fmtp = 1;
				av_strlcpy(s1->delayed_fmtp, buf, sizeof(s1->delayed_fmtp));
			}
		} else if (av_strstart(p, "ssrc:", &p) && s->nb_streams > 0) {
			rtsp_st = rt->rtsp_streams[rt->nb_rtsp_streams - 1];
			get_word(buf1, sizeof(buf1), &p);
			rtsp_st->ssrc = strtoll(buf1, NULL, 10);
		} else if (av_strstart(p, "range:", &p)) {
			int64_t start, end;
			rtsp_parse_range_npt(p, &start, &end);
			s->start_time = start;
			s->duration = (end == AV_NOPTS_VALUE) ? AV_NOPTS_VALUE : end - start;
		} else if (av_strstart(p, "lang:", &p)) {
			if (s->nb_streams > 0) {
				get_word(buf1, sizeof(buf1), &p);
				rtsp_st = rt->rtsp_streams[rt->nb_rtsp_streams - 1];
				if (rtsp_st->stream_index >= 0) {
					st = s->streams[rtsp_st->stream_index];
					av_dict_set(&st->metadata, "language", buf1, 0);
				}
			} else {
				get_word(rt->default_lang, sizeof(rt->default_lang), &p);
			}
		} else if (av_strstart(p, "IsRealDataType:integer;", &p)) {
			if (atoi(p) == 1) rt->transport = RTSP_TRANSPORT_RDT;
		} else if (av_strstart(p, "SampleRate:integer;", &p) &&
			s->nb_streams > 0) {
			st = s->streams[s->nb_streams - 1];
			st->codecpar->sample_rate = atoi(p);
		} else {
			if (s->nb_streams > 0) {
				rtsp_st = rt->rtsp_streams[rt->nb_rtsp_streams - 1];
				if (rtsp_st->dynamic_handler && rtsp_st->dynamic_handler->parse_sdp_a_line)
					rtsp_st->dynamic_handler->parse_sdp_a_line(s, rtsp_st->stream_index, rtsp_st->dynamic_protocol_context, buf);
			}
		}
		break;
	}
}

bool CSdpParser::parse(const char* sdp) {
	const char* p = sdp;
	int letter;
	char* q;
	char buf[16384];
	SDPParseState state;
	for (;;) {
		p += strspn(p, SPACE_CHARS);
		letter = *p;
		if (letter == '\0') break;
		p++;
		if (*p != '=') goto next_line;
		p++;
		q = buf;
		while (*p != '\n' && *p != '\r' && *p != '\0') {
			if ((q - buf) < sizeof(buf) - 1) *q++ = *p;
			p++;
		}
		*q = '\0';
		sdp_parse_line(&state, letter, buf);
	next_line:
		while (*p != '\n' && *p != '\0') p++;
		if (*p == '\n') p++;
	}
	return true;
}
