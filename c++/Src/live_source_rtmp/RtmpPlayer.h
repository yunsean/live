#pragma once
#include "RtmpActor.h"
#include "CommandMessage.h"
#include "xstring.h"

class CRtmpPlayer : public CRtmpActor {
public:
	CRtmpPlayer(uint32_t outputChunkSize);
	~CRtmpPlayer();

protected:
	virtual Type getType() const { return Player; }
	virtual bool isStream(const std::string& streamName) const;
	virtual bool onMessage(evbuffer* output, CRtmpChunk* chunk);
	virtual bool onMessage(evbuffer* output, CRtmpMessage* message);

protected:
	bool onPlayStream(evbuffer* output, CPlayStreamRequest* message);

private:
	uint32_t m_outputChunkSize;
	xtstring m_streamName;
	uint32_t m_streamId;
};

