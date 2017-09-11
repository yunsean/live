#include "RtmpPlayer.h"
#include "RtmpChunk.h"
#include "RtmpMessage.h"
#include "ConsoleUI.h"
#include "Writelog.h"

CRtmpPlayer::CRtmpPlayer(uint32_t outputChunkSize)
	: m_outputChunkSize(outputChunkSize)
	, m_streamName()
	, m_streamId()  {
}
CRtmpPlayer::~CRtmpPlayer() {
}

bool CRtmpPlayer::isStream(const std::string& streamName) const {
	xtstring name(streamName);
	return m_streamName.Compare(name) == 0;
}
bool CRtmpPlayer::onMessage(evbuffer* output, CRtmpChunk* chunk) {
	switch (chunk->type()) {
	case RTMP_MSG_AudioMessage:
	case RTMP_MSG_VideoMessage:
		break;
	}
	CSmartPtr<CRtmpMessage> message(nullptr);
	if (!chunk->buildMessage(message.GetPtr())) {
		return wlet(false, _T("decode packet failed: messageType=%d, length=%d"), (int)chunk->type(), chunk->length());
	} else {
		return onMessage(output, message);
	}
}
bool CRtmpPlayer::onMessage(evbuffer* output, CRtmpMessage* message) {
	switch (message->type()) {
	case PlayStreamRequest:
		return onPlayStream(output, dynamic_cast<CPlayStreamRequest*>(message));
	default:
		return true;
	}
}

bool CRtmpPlayer::onPlayStream(evbuffer* output, CPlayStreamRequest* message) {
	return true;
}