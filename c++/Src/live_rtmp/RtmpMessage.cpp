#include "RtmpMessage.h"
#include "ByteStream.h"
#include "RtmpConstants.h"
#include "ProtocolAmf0.h"
#include "Writelog.h"
#include "RtmpChunk.h"
using namespace amf0;

bool CRtmpMessage::encode(uint32_t& size, uint8_t*& payload) {
	int length(totalSize());
	uint8_t* data = NULL;
	if (length > 0) {
		data = new uint8_t[length];
		CByteStream stream(data, length);
		if (!encode(&stream)) return false;
	}
	size = length;
	payload = data;
	return true;
}
