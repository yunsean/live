#pragma once
#include <memory>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/http.h>
#include <event2/http_struct.h>
#include "SmartArr.h"

class CHandshakeBytes {
public:
	CHandshakeBytes();
	~CHandshakeBytes();

public:
	enum {Failed = -1, Succeed = 0, NeedMore = 1};

public:
	int read_c0c1(evbuffer* input);
	int read_s0s1s2(evbuffer* input);
	int read_c2(evbuffer* input);
	void create_c0c1();
	void create_s0s1s2(const char* c1 = nullptr);
	void create_c2();
	char* c0c1() { return m_c0c1; }
	char* s0s1s2() { return m_s0s1s2; }
	char* c2() { return m_c2; }

private:
	void write_4bytes(char* bytes, int32_t value);

private:
	CSmartArr<char> m_c0c1;		//[1 + 1536];
	CSmartArr<char> m_s0s1s2;	//[1 + 1536 + 1536];
	CSmartArr<char> m_c2;		//[1536];
};

