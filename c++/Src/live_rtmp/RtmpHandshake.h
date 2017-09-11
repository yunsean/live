#pragma once

enum HandshakeResult {
	HSRFailed = -1, HSRSucceed = 0, HSRNeedMore = 1, HSRTrySimple = 2
};

struct evbuffer;
interface CRtmpHandshake {
	virtual ~CRtmpHandshake() {}
	virtual HandshakeResult handshakeWithClient(CHandshakeBytes* bytes, evbuffer* input, evbuffer* output) = 0;
	virtual HandshakeResult handshakeWithServer(CHandshakeBytes* bytes, evbuffer* input, evbuffer* output) = 0;
};

class CHandshakeBytes;
class CSimpleHandshake : public CRtmpHandshake {
public:
	CSimpleHandshake();
	virtual ~CSimpleHandshake();
public:
	virtual HandshakeResult handshakeWithClient(CHandshakeBytes* bytes, evbuffer* input, evbuffer* output);
	virtual HandshakeResult handshakeWithServer(CHandshakeBytes* bytes, evbuffer* input, evbuffer* output);
private:
	enum WorkStage { Inited, ReadC0C1, ReadC2, ReadS0S1S2, Succeed };
	WorkStage m_workStage;
};

// http://blog.csdn.net/win_lin/article/details/13006803
namespace rtmp_handshake {
	class c1s1;
}
class CComplexHandshake : public CRtmpHandshake {
public:
	CComplexHandshake();
	virtual ~CComplexHandshake();
public:
	virtual HandshakeResult handshakeWithClient(CHandshakeBytes* bytes, evbuffer* input, evbuffer* output);
	virtual HandshakeResult handshakeWithServer(CHandshakeBytes* bytes, evbuffer* input, evbuffer* output);
private:
	enum WorkStage { Inited, ReadC0C1, ReadC2, ReadS0S1S2, Succeed };
	rtmp_handshake::c1s1* m_c1;
	WorkStage m_workStage;
};
