#pragma once
#include "ControlMessage.h"

#define RTMP_UCMSG_StreamBegin		0x00
#define RTMP_UCMSG_StreamEOF		0x01
#define RTMP_UCMSG_StreamDry		0x02
#define RTMP_UCMSG_SetBufferLength	0x03
#define RTMP_UCMSG_StreamIsRecorded	0x04
#define RTMP_UCMSG_PingRequest		0x06
#define RTMP_UCMSG_PingResponse		0x07

//////////////////////////////////////////////////////////////////////////
//CUserControlMessage
template<uint32_t EVENT>
class CUserControlMessage : public CControlMessage<RTMP_MSG_UserControlMessage, 0x200 + EVENT> {
public:
	uint16_t event() const { return m_event; }
	uint32_t streamId() const { return m_data; }
protected:
	CUserControlMessage() : m_event(EVENT) {}
	virtual int totalSize() {
		int size(2);
		switch (m_event) {
		case RTMP_UCMSG_StreamBegin:
		case RTMP_UCMSG_StreamEOF:
		case RTMP_UCMSG_StreamDry:
		case RTMP_UCMSG_StreamIsRecorded:
		case RTMP_UCMSG_PingRequest:
		case RTMP_UCMSG_PingResponse:
			size += 4;
			break;
		case RTMP_UCMSG_SetBufferLength:
			size += 8;
			break;
		}
		return size;
	}
	virtual bool encode(CByteStream* stream) {
		if (!stream->require(totalSize())) return false;
		stream->write_2bytes(EVENT);
		switch (m_event) {
		case RTMP_UCMSG_StreamBegin:
		case RTMP_UCMSG_StreamEOF:
		case RTMP_UCMSG_StreamDry:
		case RTMP_UCMSG_StreamIsRecorded:
		case RTMP_UCMSG_PingRequest:
		case RTMP_UCMSG_PingResponse:
			stream->write_4bytes(m_data);
			break;
		case RTMP_UCMSG_SetBufferLength:
			stream->write_4bytes(m_data);
			stream->write_4bytes(m_extraData);
			break;
		}
		return true;
	}
	virtual bool decode(CByteStream* stream) {
		if (!stream->require(2)) return false;
		m_event = stream->read_2bytes();
		switch (m_event) {
		case RTMP_UCMSG_StreamBegin:
		case RTMP_UCMSG_StreamEOF:
		case RTMP_UCMSG_StreamDry:
		case RTMP_UCMSG_StreamIsRecorded:
		case RTMP_UCMSG_PingRequest:
		case RTMP_UCMSG_PingResponse:
			if (!stream->require(4)) return false;
			m_data = stream->read_4bytes();
			break;
		case RTMP_UCMSG_SetBufferLength:
			if (!stream->require(8)) return false;
			m_data = stream->read_4bytes();
			m_extraData = stream->read_4bytes();
			break;
		}
		return true;
	}
protected:
	uint16_t m_event;
	uint32_t m_data;
	uint32_t m_extraData;
};

//////////////////////////////////////////////////////////////////////////
//CUCFakeMessage
class CUCFakeMessage : public CUserControlMessage<0> {
public:
	static CUCFakeMessage* build(uint32_t data);
};

//////////////////////////////////////////////////////////////////////////
//CUCStreamBeginMessage (0x00)
class CUCStreamBeginMessage : public CUserControlMessage<RTMP_UCMSG_StreamBegin> {
public:
	static CUCStreamBeginMessage* build(uint32_t streamId);
};

//////////////////////////////////////////////////////////////////////////
//CUCStreamEOFMessage (0x01)
class CUCStreamEOFMessage : public CUserControlMessage<RTMP_UCMSG_StreamEOF> {
public:
	static CUCStreamEOFMessage* build(uint32_t streamId);
};

//////////////////////////////////////////////////////////////////////////
//CUCStreamDryMessage (0x02)
class CUCStreamDryMessage : public CUserControlMessage<RTMP_UCMSG_StreamDry> {
public:
	static CUCStreamDryMessage* build(uint32_t streamId);
};

//////////////////////////////////////////////////////////////////////////
//CUCSetBufferLengthMessage (0x03)
class CUCSetBufferLengthMessage : public CUserControlMessage<RTMP_UCMSG_SetBufferLength> {
public:
	static CUCSetBufferLengthMessage* build(uint32_t streamId);
public:
	uint32_t bufferLength() const { return m_extraData; }
};

//////////////////////////////////////////////////////////////////////////
//CUCStreamIsRecorded Message (0x04)
class CUCStreamIsRecorded : public CUserControlMessage<RTMP_UCMSG_StreamIsRecorded> {
public:
	static CUCStreamIsRecorded* build(uint32_t streamId);
};

//////////////////////////////////////////////////////////////////////////
//CUCPingRequestMessage (0x06)
class CUCPingRequestMessage : public CUserControlMessage<RTMP_UCMSG_PingRequest> {
public:
	static CUCPingRequestMessage* build(uint32_t timestamp);
public:
	uint32_t timestamp() const { return m_data; }
};

//////////////////////////////////////////////////////////////////////////
//CUCPingResponseMessage (0x07)
class CUCPingResponseMessage : public CUserControlMessage<RTMP_UCMSG_PingResponse> {
public:
	static CUCPingResponseMessage* build(uint32_t timestamp);
};

