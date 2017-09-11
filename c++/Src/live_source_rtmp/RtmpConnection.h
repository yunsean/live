#pragma once
#include <vector>
#include <mutex>
#include <map>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/http.h>
#include <event2/http_struct.h>
#include "SourceInterface.h"
#include "HandshakeBytes.h"
#include "RtmpHandshake.h"
#include "RtmpChunk.h"
#include "RtmpMessage.h"
#include "RtmpActor.h"
#include "CommandMessage.h"
#include "UserControlMessage.h"
#include "DataMessage.h"
#include "SmartPtr.h"
#include "SmartHdr.h"
#include "SmartNal.h"
#include "xstring.h"

class CRtmpConnection {
public:
	CRtmpConnection();
	~CRtmpConnection();

public:
	bool Startup(event_base* base, evutil_socket_t fd, struct sockaddr* addr);
	void Release();

protected:
	static void on_read(struct bufferevent* bev, void* context);
	void on_read(struct bufferevent* bev);
	static void on_error(struct bufferevent* bev, short what, void* context);
	void on_error(struct bufferevent* bev, short what);

protected:	
	//result true indicate need continue, result false indicate need more data or have an error.
	bool onHandshake(bufferevent* bev);
	bool onReadChunk(bufferevent* bev);

	bool onMessage(bufferevent* bev, CRtmpChunk* chunk);
	bool onSetChunkSize(CSetChunkSizeMessage* packet);
	bool onSetWindowAckSize(CSetWindowAckSizeMessage* packet);
	bool onConnectApp(bufferevent* bev, CConnectAppRequest* request);
	bool onCreateStream(bufferevent* bev, CCreateStreamRequest* request);
	bool onDeleteStream(bufferevent* bev, CDeleteStreamRequest* request);
	bool onPublishStream(bufferevent* bev, CPublishStreamRequest* request);
	bool onPlayStream(bufferevent* bev, CPlayStreamRequest* request);
	bool onUnpublishStream(bufferevent* bev, CFCUnpublishStreamRequest* request);

protected:
	bool autoSendAcknowledgement(bufferevent* bev);
	bool onError(LPCTSTR reason);
	
protected:
	struct ChunkStream {
		ChunkStream() : chunks(), active(nullptr) {}
		std::map<uint32_t, CSmartPtr<CRtmpChunk>> chunks;
		CRtmpChunk* active;
	};
		
protected:
	enum WorkStage { Inited, Handshake, WaitConnect, Corrupted };
	enum ChunkStage { ReadChunkHeader, ReadMessageHeader, ReadChunkBody };

private:
	evutil_socket_t m_fd;
	xtstring m_clientIp;
	int m_clientPort;
	CSmartHdr<bufferevent*, void> m_bev;
	uint32_t m_nextStreamId;

	WorkStage m_workStage;
	CSmartPtr<CHandshakeBytes> m_handshakeBytes;
	CSmartPtr<CRtmpHandshake> m_handshake;

	uint32_t m_inputChunkSize;
	uint32_t m_outputChunkSize;
	std::map<uint32_t, CSmartPtr<ChunkStream>> m_chunkStreams;
	CRtmpChunk* m_pendingChunk;
	ChunkStage m_chunkStage;
	uint8_t m_chunkFmt;
	uint32_t m_csId;

	uint32_t m_ackWindowSize;
	int64_t m_ackedSize;
	int64_t m_receivedSize;

	std::map<uint32_t, CSmartPtr<CRtmpActor>> m_actors;
};

