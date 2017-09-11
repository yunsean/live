#include "UserControlMessage.h"

//////////////////////////////////////////////////////////////////////////
//CUCFakeMessage (0x00)
CUCFakeMessage* CUCFakeMessage::build(uint32_t data) {
	CUCFakeMessage* message(new CUCFakeMessage);
	message->m_data = data;
	return message;
}

//////////////////////////////////////////////////////////////////////////
//CUCStreamBeginMessage (0x00)
CUCStreamBeginMessage* CUCStreamBeginMessage::build(uint32_t streamId) {
	CUCStreamBeginMessage* message(new CUCStreamBeginMessage);
	message->m_data = streamId;
	return message;
}

//////////////////////////////////////////////////////////////////////////
//CUCStreamEOFMessage (0x01)
CUCStreamEOFMessage* CUCStreamEOFMessage::build(uint32_t streamId) {
	CUCStreamEOFMessage* message(new CUCStreamEOFMessage);
	message->m_data = streamId;
	return message;
}

//////////////////////////////////////////////////////////////////////////
//CUCStreamDryMessage (0x02)
CUCStreamDryMessage* CUCStreamDryMessage::build(uint32_t streamId) {
	CUCStreamDryMessage* message(new CUCStreamDryMessage);
	message->m_data = streamId;
	return message;
}

//////////////////////////////////////////////////////////////////////////
//CUCSetBufferLengthMessage (0x03)
CUCSetBufferLengthMessage* CUCSetBufferLengthMessage::build(uint32_t streamId) {
	CUCSetBufferLengthMessage* message(new CUCSetBufferLengthMessage);
	message->m_data = streamId;
	return message;
}

//////////////////////////////////////////////////////////////////////////
//CUCStreamIsRecorded (0x04)
CUCStreamIsRecorded* CUCStreamIsRecorded::build(uint32_t streamId) {
	CUCStreamIsRecorded* message(new CUCStreamIsRecorded);
	message->m_data = streamId;
	return message;
}

//////////////////////////////////////////////////////////////////////////
//CUCStreamBeginMessage (0x06)
CUCPingRequestMessage* CUCPingRequestMessage::build(uint32_t timestamp) {
	CUCPingRequestMessage* message(new CUCPingRequestMessage);
	message->m_data = timestamp;
	return message;
}

//////////////////////////////////////////////////////////////////////////
//CUCStreamBeginMessage (0x07)
CUCPingResponseMessage* CUCPingResponseMessage::build(uint32_t timestamp) {
	CUCPingResponseMessage* message(new CUCPingResponseMessage);
	message->m_data = timestamp;
	return message;
}