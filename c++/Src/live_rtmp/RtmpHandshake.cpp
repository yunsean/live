#include "HandshakeBytes.h"
#include "RtmpHandshake.h"
#include "ComplexHandshake.h"
#include "Writelog.h"
#include "xstring.h"
using namespace rtmp_handshake;

CSimpleHandshake::CSimpleHandshake()
	: m_workStage(Inited) {
}
CSimpleHandshake::~CSimpleHandshake() {
}
HandshakeResult CSimpleHandshake::handshakeWithClient(CHandshakeBytes* bytes, evbuffer* input, evbuffer* output) {
	if (m_workStage == Inited) {
		m_workStage = ReadC0C1;
	}
	if (m_workStage == ReadC0C1) {
		int result(bytes->read_c0c1(input));
		if (result == CHandshakeBytes::NeedMore) return HSRNeedMore;
		else if (result != CHandshakeBytes::Succeed) return HSRFailed;
		if (bytes->c0c1()[0] != 0x03) {
			return wlet(HSRFailed, _T("only support rtmp plain text. but received: %d"), (int)(bytes->c0c1()[0]));
		}
		wld("check c0 success, required plain text.");
		bytes->create_s0s1s2(bytes->c0c1() + 1);
		result = evbuffer_add(output, bytes->s0s1s2(), 3073);
		if (result != 0) {
			return wlet(HSRFailed, _T("simple handshake send s0s1s2 failed. result=%d"), result);
		}
		wld(_T("simple handshake send s0s1s2 success."));
		m_workStage = ReadC2;
	} 
	if (m_workStage == ReadC2) {
		int result(bytes->read_c2(input));
		if (result == CHandshakeBytes::NeedMore) return HSRNeedMore;
		else if (result != CHandshakeBytes::Succeed) return HSRFailed;
		wld(_T("simple handshake success."));
		m_workStage = Succeed;
	}
	return HSRSucceed;
}
HandshakeResult CSimpleHandshake::handshakeWithServer(CHandshakeBytes* bytes, evbuffer* input, evbuffer* output) {
	if (m_workStage == Inited) {
		bytes->create_c0c1();
		int result(evbuffer_add(output, bytes->c0c1(), 1537));
		if (result != 0) {
			return wlet(HSRFailed, _T("simple handshake send c0c1 failed. result=%d"), result);
		}
		wld(_T("write c0c1 success."));
		m_workStage = ReadS0S1S2;
	}
	if (m_workStage == ReadS0S1S2) {
		int result(bytes->read_s0s1s2(input));
		if (result == CHandshakeBytes::NeedMore) return HSRNeedMore;
		else if (result != CHandshakeBytes::Succeed) return HSRFailed;
		if (bytes->s0s1s2()[0] != 0x03) {
			return wlet(HSRFailed, _T("handshake failed, plain text required. but recieved: %d"), (int)bytes->s0s1s2()[0]);
		}
		bytes->create_c2();
		// for simple handshake, copy s1 to c2.
		// @see https://github.com/ossrs/srs/issues/418
		memcpy(bytes->c2(), bytes->s0s1s2() + 1, 1536);
		result = evbuffer_add(output, bytes->c2(), 1536);
		if (result != 0) {
			return wlet(HSRFailed, _T("simple handshake write c2 failed. result=%d"), result);
		}
		wld("simple handshake write c2 success.");
		wld("simple handshake success.");
		m_workStage = Succeed;
	}
	return HSRSucceed;
}

CComplexHandshake::CComplexHandshake()
	: m_c1(nullptr)
	, m_workStage(Inited) {
}
CComplexHandshake::~CComplexHandshake() {
	delete m_c1;
}
HandshakeResult CComplexHandshake::handshakeWithClient(CHandshakeBytes* bytes, evbuffer* input, evbuffer* output) {
	if (m_workStage == Inited) {
		m_workStage = ReadC0C1;
	}
	if (m_workStage == ReadC0C1) {
		int result(bytes->read_c0c1(input));
		if (result == CHandshakeBytes::NeedMore) return HSRNeedMore;
		else if (result != CHandshakeBytes::Succeed) return HSRFailed;
		// decode c1
		c1s1 c1;
		// try schema0.
		// @remark, use schema0 to make flash player happy.
		if (!c1.parse(bytes->c0c1() + 1, 1536, srs_schema0)) {
			return wlet(HSRFailed, _T("parse c1 schema%d error."), srs_schema0);
		}
		// try schema1
		bool is_valid = false;
		if (!c1.c1_validate_digest(is_valid) || !is_valid) {
			wld("schema0 failed, try schema1.");
			if (!c1.parse(bytes->c0c1() + 1, 1536, srs_schema1)) {
				return wlet(HSRFailed, _T("parse c1 schema%d error."), srs_schema1);
			}
			if (!c1.c1_validate_digest(is_valid) || !is_valid) {
				return wlwt(HSRTrySimple, _T("all schema valid failed, try simple handshake."));
			}
		} else {
			wld("schema0 is ok.");
		}
		wld("decode c1 success.");
		// encode s1
		c1s1 s1;
		if (!s1.s1_create(&c1)) {
			return wlet(HSRFailed, _T("create s1 from c1 failed."));
		}
		wld("create s1 from c1 success.");
		// verify s1
		if (!s1.s1_validate_digest(is_valid) || !is_valid) {
			return wlwt(HSRTrySimple, _T("verify s1 failed, try simple handshake."));
		}
		wld("verify s1 success.");
		c2s2 s2;
		if (!s2.s2_create(&c1)) {
			return wlet(HSRFailed, _T("create s2 from c1 failed."));
		}
		wld(_T("create s2 from c1 success."));
		// verify s2
		if (!s2.s2_validate(&c1, is_valid) || !is_valid) {
			return wlwt(HSRTrySimple, _T("verify s2 failed, try simple handshake."));
		}
		wld(_T("verify s2 success."));
		bytes->create_s0s1s2();
		if (!s1.dump(bytes->s0s1s2() + 1, 1536)) {
			return wlet(HSRFailed, _T("create s1 failed."));
		}
		if (!s2.dump(bytes->s0s1s2() + 1537, 1536)) {
			return wlet(HSRFailed, _T("create s2 failed."));
		}
		result = evbuffer_add(output, bytes->s0s1s2(), 3073);
		if (result != 0) {
			return wlet(HSRFailed, _T("complex handshake send s0s1s2 failed. result=%d"), result);
		}
		wld(_T("complex handshake send s0s1s2 success."));
		m_workStage = ReadC2;
	}
	if (m_workStage == ReadC2) {
		int result(bytes->read_c2(input));
		if (result == CHandshakeBytes::NeedMore) return HSRNeedMore;
		else if (result != CHandshakeBytes::Succeed) return HSRFailed;
		c2s2 c2;
		if (!c2.parse(bytes->c2(), 1536)) {
			return wlet(HSRFailed, _T("parse c2 error."));
		}
		wld(_T("complex handshake read c2 success."));
		wld(_T("complex handshake success"));
		m_workStage = Succeed;
	}
	return HSRSucceed;
}
HandshakeResult CComplexHandshake::handshakeWithServer(CHandshakeBytes* bytes, evbuffer* input, evbuffer* output) {
	if (m_workStage == Inited) {
		bytes->create_c0c1();
		m_c1 = new rtmp_handshake::c1s1;
		// sign c1
		// @remark, FMS requires the schema1(digest-key), or connect failed.
		if (!m_c1->c1_create(srs_schema1)) {
			return wlet(HSRFailed, _T("create c1 failed."));
		}
		if (!m_c1->dump(bytes->c0c1() + 1, 1536)) {
			return wlet(HSRFailed, _T("dump c1 failed."));
		}
		// verify c1
		bool is_valid;
		if (!m_c1->c1_validate_digest(is_valid) || !is_valid) {
			return wlet(HSRTrySimple, _T("validate c1 digest failed, try simple hand shake."));
		}
		int result(evbuffer_add(output, bytes->c0c1(), 1537));
		if (result != 0) {
			return wlet(HSRFailed, _T("simple handshake send c0c1 failed. result=%d"), result);
		}
		wld(_T("write c0c1 success."));
		m_workStage = ReadS0S1S2;
	}
	if (m_workStage == ReadS0S1S2) {
		int result(bytes->read_s0s1s2(input));
		if (result == CHandshakeBytes::NeedMore) return HSRNeedMore;
		else if (result != CHandshakeBytes::Succeed) return HSRFailed;
		if (bytes->s0s1s2()[0] != 0x03) {
			return wlet(HSRFailed, _T("handshake failed, plain text required. but recieved: %d"), (int)bytes->s0s1s2()[0]);
		}
		// verify s1s2
		c1s1 s1;
		if (!s1.parse(bytes->s0s1s2() + 1, 1536, m_c1->schema())) {
			return wlet(HSRFailed, _T("parse s0s1s2 failed."));
		}
		// never verify the s1,
		// for if forward to nginx-rtmp, verify s1 will failed,
		// TODO: FIXME: find the handshake schema of nginx-rtmp.
		// c2
		bytes->create_c2();
		c2s2 c2;
		if (!c2.c2_create(&s1)) {
			return wlet(HSRFailed, _T("create c2 failed."));
		}
		if (!c2.dump(bytes->c2(), 1536)) {
			return wlet(HSRFailed, _T("dump c2 failed."));
		}
		result = evbuffer_add(output, bytes->c2(), 1536);
		if (result != 0) {
			return wlet(HSRFailed, _T("complex handshake write c2 failed. result=%d"), result);
		}
		wld(_T("complex handshake write c2 success."));
		wld(_T("complex handshake success."));
		m_workStage = Succeed;
	}
	return HSRSucceed;
}