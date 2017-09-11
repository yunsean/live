#pragma once
#include "ProtocolAmf0.h"
#include "RtmpConstants.h"
#include "os.h"
#include "SmartPtr.h"

enum MessageType {
	ConnectAppRequest, ConnectAppResponse, 
	ReleaseStreamRequest, FCPublishStreamRequest, 
	FCUnpublishStreamRequest, FCUnpublishStreamResponse,
	CreateStreamRequest, CreateStreamResponse,
	DeleteStreamRequest, CloseStreamRequest,
	PublishStreamRequest, PublishStreamResponse,
	PlayStreamRequest, PlayStreamResponse,
	GetStreamLengthRequest,
	OnMetadataRequest,
	//Protocol Control Message
	SetChunkSize = 0x101, AbortMessage = 0x102, Acknowledgement = 0x103,
	UserControlMessage = 0x104, SetWindowAckSize = 0x105, SetPeerBandwidth = 0x106,
	//User Control Message
	UserControlStreamBegin = 0x200, UserControlStreamEOF = 0x201, UserControlStreamDry = 0x202,
	UserControlSetBufferLength = 0x203, UserControlStreamIsRecorded	= 0x204, 
	UserControlPingRequest = 0x206, UserControlPingResponse = 0x207,
};

class CRtmpChunk;
class CByteStream;
class CRtmpMessage {
public:
	virtual ~CRtmpMessage() {}
public:
	virtual int type() = 0;
	virtual bool encode(uint32_t& size, uint8_t*& payload);
	virtual bool decode(CByteStream* stream) = 0;
	virtual int totalSize() = 0;
	virtual uint32_t csId() = 0;
	virtual uint32_t messageType() = 0;
	uint32_t streamId() const { return m_streamId; }
protected:
	virtual bool encode(CByteStream* stream) = 0;
private:
	friend class CRtmpChunk;
	uint32_t m_streamId;
};

template<uint32_t CSID, uint32_t MESSAGETYPE, int TYPE>
class CBaseRtmpMessage : public CRtmpMessage {
public:
	virtual ~CBaseRtmpMessage() {}
public:
	virtual int type() { return TYPE; }
	virtual uint32_t csId() { return CSID; }
	virtual uint32_t messageType() { return MESSAGETYPE; }
};

template<uint32_t CSID, uint32_t MESSAGETYPE, int TYPE>
class CAmfxMessage : public CBaseRtmpMessage<CSID, MESSAGETYPE, TYPE> {
public:
	virtual ~CAmfxMessage() {}
protected:
	template <typename T> int sizeOf(T value) {
		return amf0::Amf0Size::size(value);
	}
	template<typename T, typename ... Types> int sizeOf(T first, Types ... rest) {
		return amf0::Amf0Size::size(first) + sizeOf(rest...);
	}

	template <typename T> bool write(CByteStream* stream, T value) {
		return amf0::amf0_write(stream, value);
	}
	template<typename T, typename ... Types> bool write(CByteStream* stream, T first, Types ... rest) {
		return amf0::amf0_write(stream, first) && write(stream, rest...);
	}

	template <typename T> bool read(CByteStream* stream, T& value) {
		if (stream->empty()) return true;
		return amf0::amf0_read(stream, value);
	}
	template<typename T, typename ... Types> bool read(CByteStream* stream, T& first, Types& ... rest) {
		if (stream->empty()) return true;
		return amf0::amf0_read(stream, first) && read(stream, rest...);
	}
};

#define MESSAGEPACKET_METHOD(...) \
protected: \
virtual bool encode(CByteStream* stream) { \
	return write(stream, __VA_ARGS__); \
} \
virtual bool decode(CByteStream* stream) { \
	return read(stream, __VA_ARGS__); \
} \
virtual int totalSize() { \
	return sizeOf(__VA_ARGS__); \
}
