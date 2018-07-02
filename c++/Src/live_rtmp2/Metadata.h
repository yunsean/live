#pragma once   
#include "Byte.h"

typedef struct RTMPMetadata {
	bool			hasVideo;
	unsigned int    width;
	unsigned int    height;
	unsigned int    frameRate;  
	unsigned int    videoBitRate;
	unsigned int    spsLen;
	unsigned char   sps[1024];
	unsigned int    ppsLen;
	unsigned char   pps[1024];

	bool            hasAudio;
	unsigned int    sampleRate;
	unsigned int    audioBitRate;
	unsigned int    channels;
	unsigned int    esdsLen;
	unsigned char   esds[10];

} *LPRTMPMetadata;

class CMetadata {
public:
	CMetadata(void);
	~CMetadata(void);

public:
	enum {
		FLV_CODECID_AVC = 7,
		FLV_CODECID_AAC = 10,
		FLV_CODECID_MP3 = 2,
	};

public:
	static void onMetadata(LPRTMPMetadata lpMetaData, CByte& body);

protected:
	static char* put_byte(char* output, uint8_t nVal);
	static char* put_be16(char* output, uint16_t nVal);
	static char* put_be24(char* output, uint32_t nVal);
	static char* put_be32(char* output, uint32_t nVal);
	static char* put_be64(char* output, uint64_t nVal);
	static char* put_amf_string(char* c, const char* str);
	static char* put_amf_double(char* c, double d);
};