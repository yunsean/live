#pragma once
#include "RtmpMessage.h"

//////////////////////////////////////////////////////////////////////////
//Protocol Control Message
#define RTMP_MSG_SetChunkSize                   0x01
#define RTMP_MSG_AbortMessage                   0x02
#define RTMP_MSG_Acknowledgement                0x03
#define RTMP_MSG_UserControlMessage             0x04
#define RTMP_MSG_SetWindowAckSize				0x05
#define RTMP_MSG_SetPeerBandwidth               0x06
#define RTMP_MSG_EdgeAndOriginServerCommand     0x07

template<uint32_t MESSAGETYPE, int TYPE>
class CControlMessage : public CBaseRtmpMessage<RTMP_CID_ProtocolControl, MESSAGETYPE, TYPE> {
public:
	virtual ~CControlMessage() {}
};

//////////////////////////////////////////////////////////////////////////
//CSetChunkSizeMessage (0x01)
class CSetChunkSizeMessage : public CControlMessage<RTMP_MSG_SetChunkSize, SetChunkSize> {
public:
	uint32_t chunkSize;
public:
	static CSetChunkSizeMessage* build(uint32_t chunkSize);
protected:
	CSetChunkSizeMessage() : chunkSize(128) {}
	virtual int totalSize();
	virtual bool encode(CByteStream* stream);
	virtual bool decode(CByteStream* stream);
};

//////////////////////////////////////////////////////////////////////////
//Abort Message (0x02)
class CAbortMessage : public CControlMessage<RTMP_MSG_AbortMessage, AbortMessage> {
public:
	uint32_t streamId;
public:
	static CAbortMessage* build(uint32_t streamId);
protected:
	CAbortMessage() : streamId(0) {}
	virtual int totalSize();
	virtual bool encode(CByteStream* stream);
	virtual bool decode(CByteStream* stream);
};

//////////////////////////////////////////////////////////////////////////
//CAcknowledgementMessage (0x03)
class CAcknowledgementMessage : public CControlMessage<RTMP_MSG_Acknowledgement, SetWindowAckSize> {
public:
	uint32_t sequenceNumber;
public:
	static CAcknowledgementMessage* build(uint32_t sequenceNumber);
protected:
	CAcknowledgementMessage() : sequenceNumber(0) {}
	virtual int totalSize();
	virtual bool encode(CByteStream* stream);
	virtual bool decode(CByteStream* stream);
};

//////////////////////////////////////////////////////////////////////////
//CUserControlMessage (0x04)
template<uint32_t EVENT>
class CUserControlMessage;

//////////////////////////////////////////////////////////////////////////
//CSetWindowAckSizeMessage (0x05)
class CSetWindowAckSizeMessage : public CControlMessage<RTMP_MSG_SetWindowAckSize, SetWindowAckSize> {
public:
	uint32_t ackowledgementWindowSize;
public:
	static CSetWindowAckSizeMessage* build(uint32_t size);
protected:
	CSetWindowAckSizeMessage() : ackowledgementWindowSize(0) {}
	virtual int totalSize();
	virtual bool encode(CByteStream* stream);
	virtual bool decode(CByteStream* stream);
};

//////////////////////////////////////////////////////////////////////////
//CSetPeerBandwidthMessage (0x06)
class CSetPeerBandwidthMessage : public CControlMessage<RTMP_MSG_SetPeerBandwidth, SetPeerBandwidth> {
public:
	enum LimitType {
		hard = 0, soft = 1, dynamic = 2
	};
public:
	uint32_t bandwidth;
	int8_t type;
public:
	static CSetPeerBandwidthMessage* build(uint32_t bandwidth, LimitType type);
protected:
	CSetPeerBandwidthMessage() : bandwidth(0), type(0) {}
	virtual int totalSize();
	virtual bool encode(CByteStream* stream);
	virtual bool decode(CByteStream* stream);
};
