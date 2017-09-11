#include "ControlMessage.h"

//////////////////////////////////////////////////////////////////////////
//CSetChunkSizeMessage (0x01)
CSetChunkSizeMessage* CSetChunkSizeMessage::build(uint32_t chunkSize) {
	CSetChunkSizeMessage* packet(new CSetChunkSizeMessage);
	packet->chunkSize = chunkSize;
	return packet;
}
int CSetChunkSizeMessage::totalSize() {
	return 4;
}
bool CSetChunkSizeMessage::encode(CByteStream* stream) {
	if (!stream->require(4)) return false;
	stream->write_4bytes(chunkSize);
	return true;
}
bool CSetChunkSizeMessage::decode(CByteStream* stream) {
	if (!stream->require(4)) return false;
	chunkSize = stream->read_4bytes();
	return true;
}

//////////////////////////////////////////////////////////////////////////
//CAbortMessage (0x02)
CAbortMessage* CAbortMessage::build(uint32_t streamId) {
	CAbortMessage* packet(new CAbortMessage);
	packet->streamId = streamId;
	return packet;
}
int CAbortMessage::totalSize() {
	return 4;
}
bool CAbortMessage::encode(CByteStream* stream) {
	if (!stream->require(4)) return false;
	stream->write_4bytes(streamId);
	return true;
}
bool CAbortMessage::decode(CByteStream* stream) {
	if (!stream->require(4)) return false;
	streamId = stream->read_4bytes();
	return true;
}

//////////////////////////////////////////////////////////////////////////
//CSetWindowAckSizeMessage (0x03)
CAcknowledgementMessage* CAcknowledgementMessage::build(uint32_t sequenceNumber) {
	CAcknowledgementMessage* packet(new CAcknowledgementMessage);
	packet->sequenceNumber = sequenceNumber;
	return packet;
}
int CAcknowledgementMessage::totalSize() {
	return 4;
}
bool CAcknowledgementMessage::encode(CByteStream* stream) {
	if (!stream->require(4)) return false;
	stream->write_4bytes(sequenceNumber);
	return true;
}
bool CAcknowledgementMessage::decode(CByteStream* stream) {
	if (!stream->require(4)) return false;
	sequenceNumber = stream->read_4bytes();
	return true;
}

//////////////////////////////////////////////////////////////////////////
//CSetWindowAckSizeMessage (0x05)
CSetWindowAckSizeMessage* CSetWindowAckSizeMessage::build(uint32_t size) {
	CSetWindowAckSizeMessage* packet(new CSetWindowAckSizeMessage);
	packet->ackowledgementWindowSize = size;
	return packet;
}
int CSetWindowAckSizeMessage::totalSize() {
	return 4;
}
bool CSetWindowAckSizeMessage::encode(CByteStream* stream) {
	if (!stream->require(4)) return false;
	stream->write_4bytes(ackowledgementWindowSize);
	return true;
}
bool CSetWindowAckSizeMessage::decode(CByteStream* stream) {
	if (!stream->require(4)) return false;
	ackowledgementWindowSize = stream->read_4bytes();
	return true;
}

//////////////////////////////////////////////////////////////////////////
//CSetPeerBandwidthMessage (0x06)
CSetPeerBandwidthMessage* CSetPeerBandwidthMessage::build(uint32_t bandwidth, LimitType type) {
	CSetPeerBandwidthMessage* packet(new CSetPeerBandwidthMessage);
	packet->bandwidth = bandwidth;
	packet->type = type;
	return packet;
}
int CSetPeerBandwidthMessage::totalSize() {
	return 5;
}
bool CSetPeerBandwidthMessage::encode(CByteStream* stream) {
	if (!stream->require(5)) return false;
	stream->write_4bytes(bandwidth);
	stream->write_1bytes(type);
	return true;
}
bool CSetPeerBandwidthMessage::decode(CByteStream* stream) {
	if (!stream->require(5)) return false;
	bandwidth = stream->read_4bytes();
	type = stream->read_1bytes();
	return true;
}