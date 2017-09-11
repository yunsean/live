#include "event2/event.h"
#include "event2/bufferevent.h"
#include "event2/buffer.h"
#include "event2/http.h" 
#include "event2/http_struct.h"
#include "RawWriter.h"
#include "RawFactory.h"
#include "ConsoleUI.h"
#include "Writelog.h"
#include "xutils.h"

#define CLOSE_AFTER_INVALID_MS	(10 * 1000)
#define CHECK_INTERVAL			(1 * 1000)
#define BUFFER_THRESHOLD		2
#define RELEASE_AFTER_IDEL_MS	(5 * 1000)

CRawWriter::CRawWriter(const xtstring& moniker, const bool reusable)
	: m_startupTime(time(NULL))
	, m_strMoniker(moniker)
	, m_base(nullptr)
	, m_lpCallback(nullptr)

	, m_tickCount(0)
	, m_latestCheck(0)
	, m_maxBuffer(1 * 1024 * 1024)
	, m_idleBeginAt(0)
	, m_headCache()
	, m_receivedDataSize(0)
	, m_sendDataSize(0)
	, m_dropDataSize(0)
	, m_latestSendData(std::tickCount())

	, m_lockClient()
	, m_pendingClient()
	, m_activedClient() {
	cpm(_T("[%s] 创建Raw Writer"), m_strMoniker.c_str());
	wli(_T("[%s] Build Raw Writer."), m_strMoniker.c_str());
}
CRawWriter::~CRawWriter() {
	wli(_T("[%s] Remove Raw Writer."), m_strMoniker.c_str());
	for (cliptr cli : m_activedClient) {
		cli->bev = nullptr;
		CRawFactory::DelPlayingCount();
	}
	for (auto cli : m_pendingClient) {
		evutil_closesocket(cli.first);
		CRawFactory::DelPlayingCount();
	}
	m_activedClient.clear();
	m_pendingClient.clear();
	CRawFactory::AddTotalDataSize(m_receivedDataSize, m_sendDataSize);
	cpm(_T("[%s] 释放Raw Writer"), m_strMoniker.c_str());
}

bool CRawWriter::Startup(ISinkProxyCallback* callback) {
	if (callback == nullptr) return false;
	m_lpCallback = callback;
	return true;
}
bool CRawWriter::AddClient(CHttpClient* client) {
	std::string header(client->getHttpSucceedResponse(_T("video/x-flv")));
	std::lock_guard<std::mutex> lock(m_lockClient);
	m_pendingClient.insert(std::make_pair(client->fd(), header));
	CRawFactory::AddPlayingCount();
	return true;
}
//////////////////////////////////////////////////////////////////////////
bool CRawWriter::StartWrite(event_base* base) {
	m_base = base;
	return true;
}
void CRawWriter::OnRawPack(PackType type, const uint8_t* const data, const int size, const bool key /* = true */) {
	if (type == Header) {
		int szData(ntohl(size + 1));
		static const int eType(Header);
		const void* lpDatas[] = { &szData, &eType, data };
		int szDatas[] = { 4, 1, size };
		m_headCache.AppendDatas(lpDatas, szDatas, sizeof(szDatas) / sizeof(szDatas[0]));
	}
	PostMediaData(type, data, size, true);
}
void CRawWriter::OnError(LPCTSTR lpszReason) {
	PostMediaData(Error, reinterpret_cast<const uint8_t*>(lpszReason), _tcslen(lpszReason) * sizeof(TCHAR), false);
}
bool CRawWriter::GetStatus(LPSTR* json, void(**free)(LPSTR)) {
	Json::Value value(GetStatus());
	std::string text(value.toStyledString());
	const CHAR* lpsz(text.c_str());
	const size_t len(text.length() + 1);
	CHAR* result = new CHAR[len];
	strcpy_s(result, len, lpsz);
	*json = result;
	*free = [](LPSTR ptr) {
		delete[] reinterpret_cast<CHAR*>(ptr);
	};
	return true;
}
void CRawWriter::PostMediaData(const PackType eType, const uint8_t* const lpData, const size_t nSize, const bool bKey) {
	m_receivedDataSize += nSize;
	m_tickCount = std::tickCount();
	if (m_pendingClient.size() == 0 && m_activedClient.size() == 0) {
		if (m_idleBeginAt != 0) {
			if (m_tickCount - m_idleBeginAt > RELEASE_AFTER_IDEL_MS) {
				DoDiscard();
				return;
			}
		} else {
			m_idleBeginAt = m_tickCount;
		}
	} else {
		m_idleBeginAt = 0;
	}
	if (m_latestCheck == 0) m_latestCheck = m_tickCount;
	bool needCheck(m_tickCount - m_latestCheck > CHECK_INTERVAL);
	size_t maxBuffer(0);
	if (needCheck && m_maxBuffer > 0) {
		maxBuffer = std::xmax(m_maxBuffer * BUFFER_THRESHOLD, (size_t)(10 * 1024));
		m_maxBuffer = 0;
		m_latestCheck = m_tickCount;
	}

	uint8_t header[5];
	*(uint32_t*)header = ntohl(static_cast<int>(nSize) + 1);
	header[4] = eType;
	m_latestSendData = m_tickCount;
	std::lock_guard<std::mutex> lock(m_lockClient);
	m_activedClient.erase(std::remove_if(m_activedClient.begin(), m_activedClient.end(),
		[this, needCheck, maxBuffer, &header, lpData, nSize](cliptr cli) -> bool {
		if (cli->over) {
			CRawFactory::DelPlayingCount();
			return true;
		} else if (cli->invalidAt != 0) {
			if (m_tickCount - cli->invalidAt > CLOSE_AFTER_INVALID_MS) {
				CRawFactory::DelPlayingCount();
				return true;
			}
		} else {
			if (needCheck) {
				evbuffer* evbuf(bufferevent_get_output(cli->bev));
				size_t len(evbuffer_get_length(evbuf));
				if (len > maxBuffer) cli->invalidAt = m_tickCount;
			} if (cli->invalidAt == 0) {
				if (bufferevent_write(cli->bev, header, 5) != 0 ||
					bufferevent_write(cli->bev, lpData, nSize) != 0) {
					CRawFactory::DelPlayingCount();
					return true;
				}
				m_sendDataSize += nSize;
			} else {
				m_dropDataSize += nSize;
			}
		}
		return false;
	}), m_activedClient.end());
	m_maxBuffer += 5 + nSize;
	if (m_pendingClient.size() > 0) {
		for (auto client : m_pendingClient) {
			bufferevent* bev = bufferevent_socket_new(m_base, client.first, BEV_OPT_CLOSE_ON_FREE);
			if (bev == NULL) {
				evutil_closesocket(client.first);
				wle(_T("Create buffer event for client failed."));
				CRawFactory::DelPlayingCount();
				continue;
			}
			cliptr cli(new ClientData(bev));
			bufferevent_setcb(bev, NULL, on_write, on_error, cli.get());
			if (bufferevent_enable(bev, EV_READ | EV_WRITE) != 0 ||
				bufferevent_write(bev, client.second.c_str(), client.second.length()) != 0 ||
				bufferevent_write(bev, m_headCache, m_headCache) != 0) {
				CRawFactory::DelPlayingCount();
			} else {
				m_sendDataSize += client.second.length();
				m_sendDataSize += m_headCache;
				m_activedClient.push_back(cli);
			}
		}
		m_pendingClient.clear();
		m_lpCallback->WantKeyFrame();
	}
}
void CRawWriter::DoDiscard() {
	if (m_lpCallback != nullptr) {
		m_lpCallback->OnDiscarded();
	}
	CRawFactory::singleton().unbindWriter(this);
}

bool CRawWriter::Discard() {
	DoDiscard();
	return true;
}

void CRawWriter::on_error(struct bufferevent* bev, short what, void* context) {
	ClientData* cli(reinterpret_cast<ClientData*>(context));
	if (what & (BEV_EVENT_ERROR | BEV_EVENT_EOF | BEV_EVENT_TIMEOUT)) {
		cli->over = true;
	}
}
void CRawWriter::on_write(struct bufferevent* event, void* context) {
	ClientData* cli(reinterpret_cast<ClientData*>(context));
	cli->invalidAt = 0;
}

Json::Value CRawWriter::GetStatus() {
	auto number2Size([](uint64_t size) -> xstring<char> {
		if (size > 1024LL * 1024 * 1024 * 512) {
			return xstring<char>::format("%.2f TB", size / (1024 * 1024 * 1024 * 1024.f));
		} else if (size > 1024 * 1024 * 512) {
			return xstring<char>::format("%.2f GB", size / (1024 * 1024 * 1024.f));
		} else if (size > 1024 * 512) {
			return xstring<char>::format("%.2f MB", size / (1024 * 1024.f));
		} else {
			return xstring<char>::format("%.2f KB", size / (1024.f));
		}
	});

	//这里的数据受RawFactory里面的锁保护，保证在调用这部分代码的时候对象不可能被Discard函数所释放
	Json::Value status;
	status["活跃客户端"] = (int)m_activedClient.size();
	status["接收数据量"] = number2Size(m_receivedDataSize);
	status["发送数据量"] = number2Size(m_sendDataSize);
	status["丢弃数据量"] = number2Size(m_dropDataSize);
	time_t now(time(NULL));
	unsigned int seconds(static_cast<int>(now - m_startupTime));
	int day = seconds / (24 * 3600);
	seconds %= (24 * 3600);
	int hour = seconds / 3600;
	seconds %= 3600;
	int minute = seconds / 60;
	status["已运行时间"] = xstring<char>::format("%d天%d小时%d分", day, hour, minute);
	return status;
}
