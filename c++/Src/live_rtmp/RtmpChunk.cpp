#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/http.h>
#include <event2/http_struct.h>
#include "ProtocolAmf0.h"
#include "RtmpChunk.h"
#include "RtmpConstants.h"
#include "RtmpMessage.h"
#include "CommandMessage.h"
#include "ControlMessage.h"
#include "DataMessage.h"
#include "UserControlMessage.h"
#include "ByteStream.h"
#include "Writelog.h"
#include "xutils.h"
using namespace utils;

#define RTMP_EXTENDED_TIMESTAMP	0xFFFFFF

CRtmpChunk::CRtmpChunk()
	: m_fmt(0)
	, m_csId(0)
	, m_chunkSize(128)
	, m_header()
	, m_timestamp(~0LL)
	, m_payload(4096, 11)
	, m_stage(ReadMessageHeader)
	, m_readSize(0) {
}

void CRtmpChunk::reset(uint32_t chunkSize, uint8_t fmt, uint32_t csid) {
	if (m_stage == ReadFinish) {
		m_payload.Clear();
	}
	m_fmt = fmt;
	m_csId = csid;
	m_chunkSize = chunkSize;
	m_stage = ReadMessageHeader;
	m_readSize = chunkHeaderSize(csid);
}
CRtmpChunk::Result CRtmpChunk::reset(uint32_t chunkSize, uint8_t fmt, uint32_t csid, const MessageHeader& header) {
	if (m_stage == ReadFinish) {
		m_payload.Clear();
	}
	m_fmt = fmt;
	m_csId = csid;
	m_header = header;
	m_chunkSize = chunkSize;
	m_readSize = chunkHeaderSize(csid) + messageHeaderSize(fmt);
	m_stage = ReadPayload;
	if (m_fmt <= RTMP_FMT_TYPE2 && m_header.timestampDelta == RTMP_EXTENDED_TIMESTAMP) {
		m_stage = ReadExtendedTimeStamp;
	} else if (m_timestamp == ~0ULL) {
		m_timestamp = m_header.timestampDelta;
	} else if (m_fmt == RTMP_FMT_TYPE0) {
		m_timestamp = m_header.timestampDelta;
	} else {
		m_timestamp += m_header.timestampDelta;
	}
	if (!m_payload.EnsureSize(m_header.payloadLength, true)) return wlet(RCRFailed, _T("Allow payload cache failed."));
	return RCRSucceed;
}
CRtmpChunk::Result CRtmpChunk::append(evbuffer* input) {
	do {
		size_t size(evbuffer_get_length(input));
		if (m_stage == ReadMessageHeader) {
			uint8_t need(messageHeaderSize(m_fmt));
			if (size < need) return RCRNeedMore;
			if (!readMessageHeader(input)) return RCRFailed;
			if (!m_payload.EnsureSize(m_header.payloadLength, true)) return wlet(RCRFailed, _T("Allow payload cache failed."));
			m_readSize += need;
		} else if (m_stage == ReadExtendedTimeStamp) {
			if (size < 4) return RCRNeedMore;
			if (!readExtendedTimeStamp(input)) return RCRFailed;
			m_readSize += 4;
		} else if (m_stage == ReadPayload) {
			uint32_t remain(m_header.payloadLength - m_payload.GetSize());
			uint32_t need(std::min(m_chunkSize, remain));
			if (size < need) return RCRNeedMore;
			if (!readPayload(input, need)) return RCRFailed;
			m_readSize += need;
			if (m_stage != ReadFinish) return RCRSucceed;
		} else {
			return RCRWhole;
		}
	} while (true);
}
bool CRtmpChunk::buildMessage(CRtmpMessage*& msg) {
	CByteStream stream(payload(), length());
	CRtmpMessage* message(nullptr);
	switch (m_header.messageType) {
	case RTMP_MSG_AMF3CommandMessage:
	case RTMP_MSG_AMF0CommandMessage:
		message = newCommandMessage(&stream);
		break;
	case RTMP_MSG_AMF0DataMessage:
	case RTMP_MSG_AMF3DataMessage:
		message = newDataMessage(&stream);
		break;
	case RTMP_MSG_SetChunkSize:
	case RTMP_MSG_AbortMessage:
	case RTMP_MSG_Acknowledgement:
	case RTMP_MSG_UserControlMessage:
	case RTMP_MSG_SetWindowAckSize:
	case RTMP_MSG_SetPeerBandwidth:
		message = newControlMessage(&stream);
		break;
	default:
		break;
	}
	if (message == nullptr) {
		return wlet(false, "unknown command: %d", m_header.messageType);
	}
	if (!message->decode(&stream)) {
		delete message;
		return false;
	}
	message->m_streamId = m_header.streamId;
	msg = message;
	return true;
}
bool CRtmpChunk::sendMessage(evbuffer* output, uint32_t chunkSize, uint32_t timestamp, uint32_t streamId, CRtmpMessage* packet) {
	uint32_t size(0);
	CSmartArr<uint8_t> payload;
	if (!packet->encode(size, payload.GetArr())) return false;
	uint8_t* pos(payload);
	uint8_t* end(payload + size);
	uint8_t headerCache[18];
	while (pos < end) {
		uint32_t headerSize(0);
		if (pos == payload) headerSize = makeChunkHeader(headerCache, RTMP_FMT_TYPE0, packet->csId(), timestamp, size, packet->messageType(), streamId);
		else headerSize = makeChunkHeader(headerCache, RTMP_FMT_TYPE3, packet->csId(), timestamp);
		int result(evbuffer_add(output, headerCache, headerSize));
		if (result != 0) return wlet(false, _T("send output data failed."));
		int payloadSize(std::min(static_cast<uint32_t>(end - pos), chunkSize));
		result = evbuffer_add(output, pos, payloadSize);
		if (result != 0) return wlet(false, _T("send output data failed."));
		pos += payloadSize;
	}
	return true;
}

uint32_t CRtmpChunk::makeChunkHeader(uint8_t cache[18], uint8_t fmt, uint32_t csId, uint32_t timestamp, uint32_t payloadLength /* = 0 */, uint8_t messageType /* = 0 */, uint32_t streamId /* = 0 */) {
	uint8_t* pos(cache);
	uint8_t first((fmt << 6) & 0xC0);
	if (csId > 319) {
		first |= 1;
		*pos++ = first;
		*pos++ = (csId - 64) % 256;
		*pos++ = (csId - 64) / 256;
	} else if (csId > 63) {
		first |= 0;
		*pos++ = first;
		*pos++ = csId - 64;
	} else {
		first |= csId;
		*pos++ = first;
	}

	if (fmt <= RTMP_FMT_TYPE2) {
		if (timestamp < RTMP_EXTENDED_TIMESTAMP) {
			pos= uintTo3BytesBE(timestamp, pos);
		} else {
			pos = uintTo3BytesBE(RTMP_EXTENDED_TIMESTAMP, pos);
		}
	}
	if (fmt <= RTMP_FMT_TYPE1) {
		pos = uintTo3BytesBE(payloadLength, pos);
		*pos++ = messageType;
	}
	if (fmt <= RTMP_FMT_TYPE0) {
		pos = uintTo4BytesLE(streamId, pos);
	}
	if (fmt <= RTMP_FMT_TYPE2 && timestamp >= RTMP_EXTENDED_TIMESTAMP) {
		pos = uintTo4BytesBE(timestamp, pos);
	}
	return static_cast<int>(pos - cache);
}

CRtmpMessage* CRtmpChunk::newCommandMessage(CByteStream* stream) {
	if ((m_header.messageType == RTMP_MSG_AMF3CommandMessage) && stream->require(1)) {
		stream->skip(1);
	}
	std::string command;
	if (!amf0::amf0_read_string(stream, command)) {
		return wlet(nullptr, _T("decode AMF0/AMF3 command name failed."));
	}
	stream->skip(-1 * stream->pos());
	if (m_header.messageType == RTMP_MSG_AMF3CommandMessage) {
		stream->skip(1);
	}
	if (command == RTMP_AMF0_COMMAND_CONNECT) {
		return new CConnectAppRequest;
	} else if (command == RTMP_AMF0_COMMAND_RELEASE_STREAM) {
		return new CReleaseStreamRequest;
	} else if (command == RTMP_AMF0_COMMAND_CREATE_STREAM) {
		return new CCreateStreamRequest;
	} else if (command == RTMP_AMF0_COMMAND_FC_PUBLISH) {
		return new CFCPublishStreamRequest;
	} else if (command == RTMP_AMF0_COMMAND_FC_UNPUBLISH) {
		return new CFCUnpublishStreamRequest;
	} else if (command == RTMP_AMF0_COMMAND_DELETE_STREAM) {
		return new CDeleteStreamRequest;
	} else if (command == RTMP_AMF0_COMMAND_CLOSE_STREAM) {
		return new CCloseStreamRequest;
	} else if (command == RTMP_AMF0_COMMAND_PUBLISH) {
		return new CPublishStreamRequest;
	} else if (command == RTMP_AMF0_COMMAND_PLAY) {
		return new CPlayStreamRequest;
	} else if (command == RTMP_AMF0_COMMAND_GET_STREAMLENGTH) {
		return new CGetStreamLengthRequest;
	} else {
		return wlet(nullptr, "unknown command: %s", command.c_str());
	}
}
CRtmpMessage* CRtmpChunk::newDataMessage(CByteStream* stream) {
	if ((m_header.messageType == RTMP_MSG_AMF3CommandMessage) && stream->require(1)) {
		stream->skip(1);
	}
	std::string command;
	if (!amf0::amf0_read_string(stream, command)) {
		return wlet(nullptr, _T("decode AMF0/AMF3 command name failed."));
	}
	stream->skip(-1 * stream->pos());
	if (m_header.messageType == RTMP_MSG_AMF3CommandMessage) {
		stream->skip(1);
	}
	if (command == RTMP_AMF0_DATA_ON_METADATA ||
		command == RTMP_AMF0_DATA_SET_DATAFRAME) {
		return new COnMetadataRequest;
	} else {
		return wlet(nullptr, "unknown command: %s", command.c_str());
	}
}
CRtmpMessage* CRtmpChunk::newControlMessage(CByteStream* stream) {
	switch (m_header.messageType) {
	case RTMP_MSG_SetChunkSize:
		return CSetChunkSizeMessage::build(0);
	case RTMP_MSG_AbortMessage:
		return CAbortMessage::build(0);
	case RTMP_MSG_Acknowledgement:
		return CAcknowledgementMessage::build(0);
	case RTMP_MSG_UserControlMessage:
		return CUCFakeMessage::build(0);
	case RTMP_MSG_SetWindowAckSize:
		return CSetWindowAckSizeMessage::build(0);
	case RTMP_MSG_SetPeerBandwidth:
		return CSetPeerBandwidthMessage::build(0, CSetPeerBandwidthMessage::dynamic);
	default:
		return nullptr;
	}
}

bool CRtmpChunk::readPayload(evbuffer* input, uint32_t size) {
	int read(size > 0 ? evbuffer_remove(input, m_payload.GetPos(), size) : 0);
	if (read < 0) {
		return wlet(false, _T("read input data failed."));
	}
	m_payload.GrowSize(read);
	if (m_payload.GetSize() >= static_cast<int>(m_header.payloadLength)) {
		m_stage = ReadFinish;
	} 
	return true;
}
bool CRtmpChunk::readExtendedTimeStamp(evbuffer* input) {
	uint8_t data[4];
	if (evbuffer_remove(input, data, 4) != 4) {
		return wlet(false, _T("read extent time stamp failed."));
	}
	m_timestamp = uintFrom4BytesBE(data);
	m_stage = ReadPayload;
	return true;
}
bool CRtmpChunk::readMessageHeader(evbuffer* input) {
	uint8_t data[11];
	uint8_t need(messageHeaderSize(m_fmt));
	if (need > 0 && evbuffer_remove(input, data, need) != need) {
		return wlet(false, _T("read input data failed."));
	}
	if (m_fmt <= RTMP_FMT_TYPE2) {
		m_header.timestampDelta = uintFrom3BytesBE(data);
	} 
	if (m_fmt <= RTMP_FMT_TYPE1) {
		m_header.payloadLength = uintFrom3BytesBE(data + 3);
		m_header.messageType = data[6];
	} 
	if (m_fmt <= RTMP_FMT_TYPE0) {
		m_header.streamId = uintFrom4BytesLE(data + 7);
	}
	if (m_fmt <= RTMP_FMT_TYPE2) {
		if (m_header.timestampDelta == RTMP_EXTENDED_TIMESTAMP) {
			m_stage = ReadExtendedTimeStamp;
		} else {
			m_stage = ReadPayload;
			if (m_timestamp == ~0ULL) {
				m_timestamp = m_header.timestampDelta;
			} else if (m_fmt == RTMP_FMT_TYPE0) {
				m_timestamp = m_header.timestampDelta;
			} else {
				m_timestamp += m_header.timestampDelta;
			}
		}
	} else {
		m_stage = ReadPayload;
	}
	return true;
}
CRtmpChunk::Result CRtmpChunk::readBasicHeader(evbuffer* input, uint8_t& fmt, uint32_t& csid) {
	int size(static_cast<int>(evbuffer_get_length(input)));
	if (size < 1) return RCRNeedMore;
	uint8_t header[3];
	if (evbuffer_copyout(input, header, 1) != 1) {
		return wlet(RCRFailed, _T("read input data failed."));
	}
	uint8_t csId(header[0] & 0x3f);
	int need(0);
	if (csId > 1) {
		need = 1;
	} else if (csId == 0) {
		need = 2;
	} else if (csId == 1) {
		need = 3;
	}
	if (size < need) return RCRNeedMore;
	if (evbuffer_remove(input, header, need) != need) {
		return wlet(RCRFailed, _T("read input data failed."));
	}
	fmt = (header[0] >> 6) & 0x03;
	if (csId > 1) {
		csid = csId;
	} else if (csId == 0) {
		csid = 64 + header[1];
	} else if (csId == 1) {
		csid = 64 + header[1] + (int)header[2] * 256;
	}
	return RCRSucceed;
}
CRtmpChunk::Result CRtmpChunk::readMessageHeader(evbuffer* input, MessageHeader& header, uint8_t fmt, uint32_t csid) {
	uint8_t need(messageHeaderSize(fmt));
	size_t size(evbuffer_get_length(input));
	if (size < need) return RCRNeedMore;
	uint8_t data[11];
	if (need > 0 && evbuffer_remove(input, data, need) != need) {
		return wlet(RCRFailed, _T("read input data failed."));
	}
	if (fmt <= RTMP_FMT_TYPE2) {
		header.timestampDelta = uintFrom3BytesBE(data);
	}
	if (fmt <= RTMP_FMT_TYPE1) {
		header.payloadLength = uintFrom3BytesBE(data + 3);
		header.messageType = data[6];
	}
	if (fmt <= RTMP_FMT_TYPE0) {
		header.streamId = uintFrom4BytesLE(data + 7);
	}
	return RCRSucceed;
}

uint8_t CRtmpChunk::chunkHeaderSize(uint32_t csId) {
	if (csId > 319) {
		return 3;
	} else if (csId > 63) {
		return 2;
	} else {
		return 1;
	}
}
uint8_t CRtmpChunk::messageHeaderSize(uint8_t fmt) {
	if (fmt == RTMP_FMT_TYPE0) return 11;
	else if (fmt == RTMP_FMT_TYPE1) return 7;
	else if (fmt == RTMP_FMT_TYPE2) return 3;
	else return 0;
}