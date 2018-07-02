#include "rtmp.h"  
#include "rtmp_sys.h" 
#include "amf.h"
#include "Metadata.h"

char* CMetadata::put_byte(char* output, uint8_t nVal) {
	output[0] = nVal;
	return output + 1;
}
char* CMetadata::put_be16(char* output, uint16_t nVal) {
	output[1] = nVal & 0xff;
	output[0] = nVal >> 8;
	return output + 2;
}
char* CMetadata::put_be24(char* output, uint32_t nVal) {
	output[2] = nVal & 0xff;
	output[1] = nVal >> 8;
	output[0] = nVal >> 16;
	return output + 3;
}
char* CMetadata::put_be32(char* output, uint32_t nVal) {
	output[3] = nVal & 0xff;
	output[2] = nVal >> 8;
	output[1] = nVal >> 16;
	output[0] = nVal >> 24;
	return output + 4;
}
char* CMetadata::put_be64(char* output, uint64_t nVal) {
	output = put_be32(output, static_cast<uint32_t>(nVal >> 32));
	output = put_be32(output, static_cast<uint32_t>(nVal));
	return output;
}
char* CMetadata::put_amf_string(char* output, const char* str) {
	uint16_t len = static_cast<uint16_t>(strlen(str));
	output = put_be16(output, len);
	memcpy(output, str, len);
	return output + len;
}
char* CMetadata::put_amf_double(char* output, double d) {
	*output++ = AMF_NUMBER;
	unsigned char* ci;
	unsigned char* co;
	ci = (unsigned char*)&d;
	co = (unsigned char*)output;
	co[0] = ci[7];
	co[1] = ci[6];
	co[2] = ci[5];
	co[3] = ci[4];
	co[4] = ci[3];
	co[5] = ci[2];
	co[6] = ci[1];
	co[7] = ci[0];
	return output + 8;
}

void CMetadata::onMetadata(LPRTMPMetadata lpMetaData, CByte& body) {
	body.EnsureSize(1024 * 4);
	char* p = (char*)body.GetData();
	p = put_byte(p, AMF_STRING);
	p = put_amf_string(p, "@setDataFrame");

	p = put_byte(p, AMF_STRING);
	p = put_amf_string(p, "onMetaData");

	p = put_byte(p, AMF_OBJECT);
	p = put_amf_string(p, "copyright");
	p = put_byte(p, AMF_STRING);
	p = put_amf_string(p, "yunsean.com");

	if (lpMetaData->hasVideo) {
		p = put_amf_string(p, "videocodecid");
		p = put_amf_double(p, FLV_CODECID_AVC);
		p = put_amf_string(p, "width");
		p = put_amf_double(p, lpMetaData->width);
		p = put_amf_string(p, "height");
		p = put_amf_double(p, lpMetaData->height);
		p = put_amf_string(p, "framerate");
		p = put_amf_double(p, lpMetaData->frameRate);
		p = put_amf_string(p, "videodatarate");
		p = put_amf_double(p, lpMetaData->videoBitRate);
	}
	if (lpMetaData->hasAudio) {
		p = put_amf_string(p, "audiocodecid");
		p = put_amf_double(p, FLV_CODECID_AAC);
		p = put_amf_string(p, "audiosamplerate");
		p = put_amf_double(p, lpMetaData->sampleRate);
		p = put_amf_string(p, "audiodatarate");
		p = put_amf_double(p, lpMetaData->audioBitRate);
		p = put_amf_string(p, "audiochannels");
		p = put_amf_double(p, lpMetaData->channels);
	}
	p = put_amf_string(p, "");
	p = put_byte(p, AMF_OBJECT_END);

	body.SetSize(p - (char*)body.GetData());
}