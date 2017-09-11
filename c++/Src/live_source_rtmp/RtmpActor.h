#pragma once
#include <string>
#include "os.h"

struct evbuffer;
class CRtmpChunk;
class CRtmpMessage;
interface CRtmpActor {
	enum Type{Publisher, Player};
	virtual ~CRtmpActor() {}
	virtual Type getType() const = 0;
	virtual bool isStream(const std::string& streamName) const = 0;
	virtual bool onMessage(evbuffer* output, CRtmpChunk* chunk) = 0;
	virtual bool onMessage(evbuffer* output, CRtmpMessage* message) = 0;
};