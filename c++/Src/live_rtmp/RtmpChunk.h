#pragma once
#include "Byte.h"
#include "os.h"

#define RTMP_FMT_TYPE0			0	//timestamp(3B) + messageLength(3B) + messageType(1B) + streamId(4B)
#define RTMP_FMT_TYPE1			1	//timestamp(3B) + messageLength(3B) + messageType(1B)
#define RTMP_FMT_TYPE2			2	//timestamp(3B)
#define RTMP_FMT_TYPE3			3	//none
struct MessageHeader {
	uint32_t timestampDelta;
	uint32_t payloadLength;
	uint8_t messageType;
	uint32_t streamId;
};

struct evbuffer;
class CRtmpMessage;
class CByteStream;
class CRtmpChunk {
public:
	CRtmpChunk();
public:
	enum Result {
		RCRFailed = -1, RCRSucceed = 0, RCRNeedMore = 1, RCRWhole = 2
	};

public:
	static Result readBasicHeader(evbuffer* input, uint8_t& fmt, uint32_t& csid);
	static Result readMessageHeader(evbuffer* input, MessageHeader& header, uint8_t fmt, uint32_t csid);
	static bool sendMessage(evbuffer* output, uint32_t chunkSize, uint32_t timestamp, uint32_t streamId, CRtmpMessage* packet);
public:
	void reset(uint32_t chunkSize, uint8_t fmt, uint32_t csid);
	Result reset(uint32_t chunkSize, uint8_t fmt, uint32_t csid, const MessageHeader& header);
	Result append(evbuffer* input);
	uint8_t fmt() { return m_fmt; }
	uint32_t csid() { return m_csId; }
	uint64_t timestamp() { return m_timestamp; }
	uint32_t length() { return  m_header.payloadLength; }
	uint8_t type() { return m_header.messageType; }
	uint32_t streamId() { return m_header.streamId; }
	uint8_t* payload() { return m_payload.GetData(); }
	const MessageHeader& header() { return m_header; }
	uint32_t readSize() { return m_readSize; }
	bool buildMessage(CRtmpMessage*& msg);

protected:
	bool readMessageHeader(evbuffer* input);
	bool readExtendedTimeStamp(evbuffer* input);
	bool readPayload(evbuffer* input, uint32_t size);

protected:
	static uint8_t chunkHeaderSize(uint32_t csId);
	static uint8_t messageHeaderSize(uint8_t fmt);
	static uint32_t makeChunkHeader(uint8_t cache[18], uint8_t fmt, uint32_t csId, uint32_t timestamp, uint32_t payloadLength = 0, uint8_t messageType = 0, uint32_t streamId = 0);
	
protected:
	CRtmpMessage* newCommandMessage(CByteStream* stream);
	CRtmpMessage* newDataMessage(CByteStream* stream);
	CRtmpMessage* newControlMessage(CByteStream* stream);

private:
	enum WorkStage{ReadMessageHeader, ReadExtendedTimeStamp, ReadPayload, ReadFinish};

private:
	uint8_t m_fmt;
	uint32_t m_csId;
	uint32_t m_chunkSize;
	MessageHeader m_header;
	uint64_t m_timestamp;
	CByte m_payload;
	WorkStage m_stage;
	uint32_t m_readSize;
};

