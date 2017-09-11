#include "RtmpConnection.h"
#include "RtmpFactory.h"
#include "RtmpConstants.h"
#include "RtmpPublisher.h"
#include "RtmpPlayer.h"
#include "net.h"
#include "xutils.h"
#include "Writelog.h"
#include "ConsoleUI.h"
using namespace amf0;

CRtmpConnection::CRtmpConnection()
	: m_fd()
	, m_clientIp()
	, m_clientPort()
	, m_bev(bufferevent_free)
	, m_nextStreamId(0)

	, m_workStage(Inited)
	, m_handshakeBytes(nullptr)
	, m_handshake(nullptr)

	, m_inputChunkSize(128)
	, m_outputChunkSize(128)
	, m_chunkStreams()
	, m_pendingChunk(nullptr)
	, m_chunkStage(ReadChunkHeader)
	, m_chunkFmt(0)
	, m_csId(0)

	, m_ackWindowSize(0)
	, m_ackedSize(0)
	, m_receivedSize(0)

	, m_actors() {
	cpm(_T("[%p] RTMP连接创建"), this);
}
CRtmpConnection::~CRtmpConnection() {
	cpm(_T("[%p] RTMP连接释放"), this);
}

bool CRtmpConnection::Startup(event_base* base, evutil_socket_t fd, struct sockaddr* addr) {
	char ip[1024];
	m_clientIp = inet_ntop(AF_INET, &(((sockaddr_in*)addr)->sin_addr), ip, sizeof(ip));
	m_clientPort = ntohs(((sockaddr_in*)addr)->sin_port);
	m_fd = fd;
	m_bev = bufferevent_socket_new(base, m_fd, BEV_OPT_CLOSE_ON_FREE);
	if (m_bev == NULL) {
		return wlet(false, _T("Create buffer event failed."));
	}
	bufferevent_setcb(m_bev, &CRtmpConnection::on_read, NULL, &CRtmpConnection::on_error, this);
	bufferevent_enable(m_bev, EV_READ | EV_WRITE);
	m_workStage = Handshake;
	return true;
}
void CRtmpConnection::Release() {
	delete this;
}

void CRtmpConnection::on_read(struct bufferevent* bev, void* context) {
	CRtmpConnection* thiz = reinterpret_cast<CRtmpConnection*>(context);
	thiz->on_read(bev);
}
void CRtmpConnection::on_read(struct bufferevent* bev) {
	bool result(true);
	do {
		switch (m_workStage) {
		case Handshake:
			result = onHandshake(bev);
			break;
		default:
			result = onReadChunk(bev);
			break;
		}
	} while (result && m_workStage != Corrupted);
	if (m_workStage == Corrupted) {
		CRtmpFactory::Singleton().DeleteConnection(this);
	}
}
bool CRtmpConnection::onHandshake(bufferevent* bev) {
	evbuffer* input(bufferevent_get_input(bev));
	evbuffer* output(bufferevent_get_output(bev));
	if (m_handshake == nullptr) {
		m_handshakeBytes = new CHandshakeBytes;
		m_handshake = new CComplexHandshake;
	}
	HandshakeResult result(m_handshake->handshakeWithClient(m_handshakeBytes, input, output));
	if (result == HSRFailed) {
		return wlet(onError(_T("与客户端握手失败")), _T("Hand shake with client failed."));
	} else if (result == HSRTrySimple) {
		m_handshake = new CSimpleHandshake;
		return onHandshake(bev);
	} else if (result == HSRSucceed) {
		m_workStage = WaitConnect;
	} else {
		return false;
	}
	return true;
}
bool CRtmpConnection::onReadChunk(bufferevent* bev) {
	evbuffer* input(bufferevent_get_input(bev));
	if (m_chunkStage == ReadChunkHeader) {
		auto result(CRtmpChunk::readBasicHeader(input, m_chunkFmt, m_csId));
		if (result == CRtmpChunk::RCRNeedMore) return false;
		if (result == CRtmpChunk::RCRFailed) return wlet(onError(_T("解析RTMP协议失败")), _T("Parse chunk basic header failed."));
		if (m_chunkFmt == RTMP_FMT_TYPE0) {
			m_chunkStage = ReadMessageHeader;
			return true;
		}
		auto csit(m_chunkStreams.find(m_csId));
		ChunkStream* cs(nullptr);
		if (csit == m_chunkStreams.end()) {
			cs = new ChunkStream;
			m_chunkStreams.insert(std::make_pair(m_csId, cs));
		} else {
			cs = csit->second;
		}
		if (cs->active == nullptr) {
			return wlet(onError(_T("解析RTMP协议失败")), _T("The active chunk is null while the chunk fmt is not type0."));
		} else {
			m_chunkStage = ReadChunkBody;
			m_pendingChunk = cs->active;
			m_pendingChunk->reset(m_inputChunkSize, m_chunkFmt, m_csId);
			return true;
		}
	} else if (m_chunkStage == ReadMessageHeader) {
		MessageHeader header;
		auto result(CRtmpChunk::readMessageHeader(input, header, m_chunkFmt, m_csId));
		if (result == CRtmpChunk::RCRNeedMore) return false;
		if (result == CRtmpChunk::RCRFailed) return wlet(onError(_T("解析RTMP协议失败")), _T("Parse chunk basic header failed."));
		auto csit(m_chunkStreams.find(m_csId));
		ChunkStream* cs(nullptr);
		if (csit == m_chunkStreams.end()) {
			cs = new ChunkStream;
			m_chunkStreams.insert(std::make_pair(m_csId, cs));
		} else {
			cs = csit->second;
		}
		auto found(cs->chunks.find(header.streamId));
		if (found == cs->chunks.end()) {
			cs->active = new CRtmpChunk;
			cs->chunks.insert(std::make_pair(header.streamId, cs->active));
		} else {
			cs->active = found->second;
		}
		m_pendingChunk = cs->active;
		result = m_pendingChunk->reset(m_inputChunkSize, m_chunkFmt, m_csId, header);
		if (result == CRtmpChunk::RCRFailed) return wlet(onError(_T("解析RTMP协议失败")), _T("Parse chunk basic header failed."));
		m_chunkStage = ReadChunkBody;
		return true;
	} else {
		auto result(m_pendingChunk->append(input));
		if (result == CRtmpChunk::RCRNeedMore) return false;
		if (result == CRtmpChunk::RCRFailed) return wlet(onError(_T("解析RTMP数据包失败")), _T("Parse chunk failed."));
		m_receivedSize += m_pendingChunk->readSize();
		if (!autoSendAcknowledgement(bev)) return wlet(onError(_T("RTMP数据流错误")), _T("Send acknowledgement message failed."));
		bool continous(true);
		if (result == CRtmpChunk::RCRWhole) continous = onMessage(bev, m_pendingChunk);
		m_chunkStage = ReadChunkHeader;
		return continous;
	}
}

bool CRtmpConnection::onMessage(bufferevent* bev, CRtmpChunk* chunk) {
	auto found(m_actors.find(chunk->streamId()));
	if (found != m_actors.end()) {
		evbuffer* output(bufferevent_get_output(bev));
		if (found->second->onMessage(output, chunk)) return true;
		if (found->second->getType() == CRtmpActor::Publisher) {
			CRtmpFactory::Singleton().DeletePublisher(dynamic_cast<CRtmpPublisher*>(found->second.GetPtr()));
		}
		m_actors.erase(found);
		return false;	//TODO:是否还可以继续，数据流可能已经出错了
	} 
	CSmartPtr<CRtmpMessage> packet(nullptr);
	if (!chunk->buildMessage(packet.GetPtr())) {
		return wlet(onError(_T("不支持的客户端版本")), _T("decode packet failed: messageType=%d, length=%d"), (int)chunk->type(), chunk->length());
	} 
	switch (packet->type()) {
	case ConnectAppRequest:
		return onConnectApp(bev, dynamic_cast<CConnectAppRequest*>(packet.GetPtr()));
	case SetChunkSize:
		return onSetChunkSize(dynamic_cast<CSetChunkSizeMessage*>(packet.GetPtr()));
	case SetWindowAckSize:
		return onSetWindowAckSize(dynamic_cast<CSetWindowAckSizeMessage*>(packet.GetPtr()));
	case CreateStreamRequest:
		return onCreateStream(bev, dynamic_cast<CCreateStreamRequest*>(packet.GetPtr()));
	case PublishStreamRequest:
		return onPublishStream(bev, dynamic_cast<CPublishStreamRequest*>(packet.GetPtr()));
	case PlayStreamRequest:
		return onPlayStream(bev, dynamic_cast<CPlayStreamRequest*>(packet.GetPtr()));
	case FCUnpublishStreamRequest:
		return onUnpublishStream(bev, dynamic_cast<CFCUnpublishStreamRequest*>(packet.GetPtr()));
	case DeleteStreamRequest:
		return onDeleteStream(bev, dynamic_cast<CDeleteStreamRequest*>(packet.GetPtr()));
	default:
		return true;
	}
}

bool CRtmpConnection::onCreateStream(bufferevent* bev, CCreateStreamRequest* request) {
	evbuffer* output(bufferevent_get_output(bev));
	CSmartPtr<CCreateStreamResponse> pkt(new CCreateStreamResponse(request->transactionId));
	pkt->transactionId = request->transactionId;
	pkt->props = Amf0Any::null();
	pkt->streamId = ++m_nextStreamId;
	return CRtmpChunk::sendMessage(output, m_outputChunkSize, 0, request->streamId(), pkt);
}
bool CRtmpConnection::onPublishStream(bufferevent* bev, CPublishStreamRequest* request) {
	evbuffer* output(bufferevent_get_output(bev));
	CSmartPtr<CRtmpActor> actor(new CRtmpPublisher(m_outputChunkSize));
	if (!actor->onMessage(output, request)) return false;
	m_actors.insert(std::make_pair(request->streamId(), actor));
	CRtmpFactory::Singleton().RegistPublisher(dynamic_cast<CRtmpPublisher*>(actor.GetPtr()));
	return true;
}
bool CRtmpConnection::onPlayStream(bufferevent* bev, CPlayStreamRequest* request) {
	evbuffer* output(bufferevent_get_output(bev));
	CSmartPtr<CRtmpActor> actor(new CRtmpPlayer(m_outputChunkSize));
	if (!actor->onMessage(output, request)) return false;
	m_actors.insert(std::make_pair(request->streamId(), actor));
	return true;
}
bool CRtmpConnection::onUnpublishStream(bufferevent* bev, CFCUnpublishStreamRequest* request) {
	evbuffer* output(bufferevent_get_output(bev));
	auto streamName(request->streamName);
	auto found(std::find_if(m_actors.begin(), m_actors.end(), 
		[&streamName](std::pair<uint32_t, CSmartPtr<CRtmpActor>> kv) -> bool {
		return kv.second->isStream(streamName);
	}));
	if (found == m_actors.end()) {
		cpe(_T("[%s] 未找到的流被停止"), streamName.c_str());
		return wlet(true, _T("Not found the stream [%s] to unpublish."), streamName.c_str());
	}
	return found->second->onMessage(output, request);
}
bool CRtmpConnection::onDeleteStream(bufferevent* bev, CDeleteStreamRequest* request) {
	auto found(m_actors.find(static_cast<int>(request->streamId)));
	if (found != m_actors.end()) {
		if (found->second->getType() == CRtmpActor::Publisher) {
			CRtmpFactory::Singleton().DeletePublisher(dynamic_cast<CRtmpPublisher*>(found->second.GetPtr()));
		}
		m_actors.erase(found);
	}
	return true;
}
bool CRtmpConnection::onSetChunkSize(CSetChunkSizeMessage* packet) {
	m_inputChunkSize = packet->chunkSize;
	return true;
}
bool CRtmpConnection::onSetWindowAckSize(CSetWindowAckSizeMessage* packet) {
	m_ackWindowSize = packet->ackowledgementWindowSize;
	return true;
}
bool CRtmpConnection::onConnectApp(bufferevent* bev, CConnectAppRequest* request) {
	CSmartPtr<CRtmpMessage> packet(nullptr);
	evbuffer* output(bufferevent_get_output(bev));
	packet = CSetWindowAckSizeMessage::build(5 * 1000 * 1000);
	if (!CRtmpChunk::sendMessage(output, m_outputChunkSize, 0, request->streamId(), packet)) {
		return false;
	}
	packet = CSetPeerBandwidthMessage::build(5 * 1000 * 1000, CSetPeerBandwidthMessage::dynamic);
	if (!CRtmpChunk::sendMessage(output, m_outputChunkSize, 0, request->streamId(), packet)) {
		return false;
	}
	packet = CSetChunkSizeMessage::build(4000);
	if (!CRtmpChunk::sendMessage(output, m_outputChunkSize, 0, request->streamId(), packet)) {
		return false;
	}
	m_outputChunkSize = 4000;
	packet = CUCStreamBeginMessage::build(0);
	if (!CRtmpChunk::sendMessage(output, m_outputChunkSize, 0, request->streamId(), packet)) {
		return false;
	}
	
	CSmartPtr<CConnectAppResponse> pkt(new CConnectAppResponse(request->transactionId));
	pkt->props = Amf0Any::object();
	pkt->info = Amf0Any::object();
	pkt->props->set("fmsVer", Amf0Any::string("FMS/3,5,3,888"));
	pkt->props->set("capabilities", Amf0Any::number(127));
	pkt->props->set("mode", Amf0Any::number(1));

	pkt->info->set(StatusLevel, Amf0Any::string(StatusLevelStatus));
	pkt->info->set(StatusCode, Amf0Any::string(StatusCodeConnectSuccess));
	pkt->info->set(StatusDescription, Amf0Any::string("Connection succeeded"));
	Amf0Any* prop(request->command->ensurePropertyNumber("objectEncoding"));
	if (prop != NULL) {
		pkt->info->set("objectEncoding", Amf0Any::number(prop->toNumber()));
	} else {
		pkt->info->set("objectEncoding", Amf0Any::number(0));
	}
	return CRtmpChunk::sendMessage(output, m_outputChunkSize, 0, request->streamId(), pkt);
}

bool CRtmpConnection::autoSendAcknowledgement(bufferevent* bev) {
	if (m_ackWindowSize == 0) return true;
	if (m_receivedSize - m_ackedSize < m_ackWindowSize) return true;
	evbuffer* output(bufferevent_get_output(bev));
	CSmartPtr<CAcknowledgementMessage> pkt(CAcknowledgementMessage::build((uint32_t)m_receivedSize));
	if (!CRtmpChunk::sendMessage(output, m_outputChunkSize, 0, 0, pkt)) return false;
	m_ackedSize = m_receivedSize;
	return true;
}

void CRtmpConnection::on_error(struct bufferevent* bev, short what, void* context) {
	CRtmpConnection* thiz = reinterpret_cast<CRtmpConnection*>(context);
	thiz->on_error(bev, what);
}
void CRtmpConnection::on_error(struct bufferevent* bev, short what) {
	if (what & (BEV_EVENT_ERROR | BEV_EVENT_EOF | BEV_EVENT_TIMEOUT)) {
		bufferevent_disable(bev, EV_READ | EV_WRITE);
		m_bev = nullptr;
		m_workStage = Corrupted;
		CRtmpFactory::Singleton().DeleteConnection(this);
	}
}

bool CRtmpConnection::onError(LPCTSTR reason) {
	cpe(_T("%s"), reason);
	m_workStage = Corrupted;
	m_bev = nullptr;
	return false;
}
