#include <memory>
#include "event2/event.h"
#include "event2/bufferevent.h"
#include "event2/buffer.h"
#include "event2/http.h" 
#include "event2/http_struct.h"
#include "FlvWriter.h"
#include "FlvFactory.h"
#include "ByteStream.h"
#include "AVCConfig.h"
#include "H264Utility.h"
#include "ConsoleUI.h"
#include "Writelog.h"
#include "xutils.h"

#define CLOSE_AFTER_INVALID_MS	(10 * 1000)
#define CHECK_INTERVAL			(1 * 1000)
#define BUFFER_THRESHOLD		2
#define RELEASE_AFTER_IDEL_MS	(5 * 1000)

CFlvWriter::CFlvWriter(const xtstring& moniker, const bool reusable)
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
	, m_activedClient()

	, m_h264Utility() {
	cpm(_T("[%s] 创建Flv Writer"), m_strMoniker.c_str());
	wli(_T("[%s] Build Flv Writer."), m_strMoniker.c_str());

	static const uint8_t header[] = { 'F', 'L', 'V', 0x01, 0x01, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x00 };
	m_headCache.AppendData(header, sizeof(header));
}
CFlvWriter::~CFlvWriter() {
	wli(_T("[%s] Remove Flv Writer."), m_strMoniker.c_str());
	for (cliptr cli : m_activedClient) {
		cli->bev = nullptr;
		CFlvFactory::DelPlayingCount();
	}
	for (auto cli : m_pendingClient) {
		evutil_closesocket(cli.first);
		CFlvFactory::DelPlayingCount();
	}
	m_activedClient.clear();
	m_pendingClient.clear();
	CFlvFactory::AddTotalDataSize(m_receivedDataSize, m_sendDataSize);
	cpm(_T("[%s] 释放Flv Writer"), m_strMoniker.c_str());
}

bool CFlvWriter::Startup(ISinkProxyCallback* callback) {
	if (callback == nullptr) return false;
	m_lpCallback = callback;
	return true;
}
bool CFlvWriter::AddClient(ISinkHttpRequest* client) {
	std::string header(GetHttpSucceedResponse(client, _T("video/x-flv")));
	std::lock_guard<std::mutex> lock(m_lockClient);
	m_pendingClient.insert(std::make_pair(client->GetSocket(true), header));
	CFlvFactory::AddPlayingCount();
	return true;
}

//////////////////////////////////////////////////////////////////////////
void CFlvWriter::write_avc_config(const uint8_t* header, int size) {
	CAVCConfig config;
	int len(0);
	const unsigned char* sps(m_h264Utility.GetSps(len));
	if (sps != nullptr && len > 0) config.AddSps(sps, len);
	const unsigned char* pps(m_h264Utility.GetPps(len));
	if (pps != nullptr && len > 0) config.AddPps(pps, len);
	CSmartNal<unsigned char> avCc;
	config.Serialize(avCc);
	if (avCc.GetSize() < 1) return;
	int bodySize = 1 + 1 + 3 + avCc.GetSize();
	int tagSize = 1 + 3 + 3 + 1 + 3 + bodySize + 4;
	std::unique_ptr<uint8_t[]> cache(new uint8_t[tagSize]);
	CByteStream bs(cache.get(), tagSize);

	bs.write_1bytes(0x09);			//TagType
	bs.write_3bytes(bodySize);		//DataSize
	bs.write_3bytes(0);				//Timestamp
	bs.write_1bytes(0x00);			//TimestampExtended
	bs.write_3bytes(0);				//StreamID

	bs.write_1bytes(0x17);			//FrameType & Codec
	bs.write_1bytes(0);				//AVCPacketType
	bs.write_3bytes(0);				//CompositionTime
	bs.write_bytes(avCc.GetArr(), avCc.GetSize());

	bs.write_4bytes(tagSize - 4);
	m_headCache.AppendData(cache.get(), tagSize);
}
void CFlvWriter::write_aac_config(const uint8_t* header, int size) {
	int bodySize = 1 + 1 + size;
	int tagSize = 1 + 3 + 3 + 1 + 3 + bodySize + 4;
	std::unique_ptr<uint8_t[]> cache(new uint8_t[tagSize]);
	CByteStream bs(cache.get(), tagSize);

	bs.write_1bytes(0x08);          //TagType
	bs.write_3bytes(bodySize);		//DataSize
	bs.write_3bytes(0);             //Timestamp
	bs.write_1bytes(0x00);          //TimestampExtended
	bs.write_3bytes(0);             //StreamID

	bs.write_1bytes((uint8_t)0xaf);  //SoundFormat[4] & SoundRate[2] & SoundSize[1] & SoundType[1]
	bs.write_1bytes(0);              //AACPacketType
	bs.write_bytes(header, size);

	bs.write_4bytes(tagSize - 4);
	m_headCache.AppendData(cache.get(), tagSize);
}
void CFlvWriter::write_avc_frame(const uint8_t* data, int size, uint32_t pts, bool key) {
	data += 4;
	size -= 4;		//Skip 00000001
	int bodySize = 1 + 1 + 3 + 4 + size;
	int tagSize = 1 + 3 + 3 + 1 + 3 + bodySize + 4;
	m_frameCache.EnsureSize(tagSize);
	CByteStream bs(m_frameCache.GetData(), tagSize);

	bs.write_1bytes(0x09);							//TagType
	bs.write_3bytes(bodySize);						//DataSize
	bs.write_3bytes((int)(pts & 0xffffff));			//Timestamp
	bs.write_1bytes((uint8_t)((pts >> 24) & 0xff));	//TimestampExtended
	bs.write_3bytes(0);								//StreamID

	bs.write_1bytes(key ? 0x17 : 0x27);				//FrameType & Codec
	bs.write_1bytes(0x01);							//AVCPacketType
	bs.write_3bytes(0x00);							//CompositionTime
	bs.write_4bytes(size);							//Nal Length
	bs.write_bytes(data, size);

	bs.write_4bytes(tagSize - 4);
	PostMediaData(m_frameCache.GetData(), tagSize, key);
}
void CFlvWriter::write_aac_frame(const uint8_t* data, int size, uint32_t pts) {
	int bodySize = 1 + 1 + size;
	int tagSize = 1 + 3 + 3 + 1 + 3 + bodySize + 4;
	m_frameCache.EnsureSize(tagSize);
	CByteStream bs(m_frameCache.GetData(), tagSize);

	bs.write_1bytes(0x08);							//TagType
	bs.write_3bytes(bodySize);						//DataSize
	bs.write_3bytes((int)(pts & 0xffffff));			//Timestamp
	bs.write_1bytes((uint8_t)((pts >> 24) & 0xff));	//TimestampExtended
	bs.write_3bytes(0);	

	bs.write_1bytes((uint8_t)0xaf);                 //SoundFormat[4] & SoundRate[2] & SoundSize[1] & SoundType[1]
	bs.write_1bytes(0x01);                          //AACPacketType
	bs.write_bytes(data, size);

	bs.write_4bytes(tagSize - 4);
	PostMediaData(m_frameCache.GetData(), tagSize, false);
}

//////////////////////////////////////////////////////////////////////////
bool CFlvWriter::StartWrite(event_base* base) {
	m_base = base;
	return true;
}
void CFlvWriter::OnVideoConfig(const CodecID codec, const uint8_t* header, const int size) {
	if (m_h264Utility.PickExtraData(header, size)) {
		write_avc_config(header, size);
	}
}
void CFlvWriter::OnVideoFrame(const uint8_t* const data, const int size, const uint32_t pts) {
	bool isKey(m_h264Utility.GetSliceInfo(data, size) == CH264Utility::islice);
	write_avc_frame(data, size, pts, isKey);
}
void CFlvWriter::OnAudioConfig(const CodecID codec, const uint8_t* header, const int size) {
	write_aac_config(header, size);
}
void CFlvWriter::OnAudioFrame(const uint8_t* const data, const int size, const uint32_t pts) {
	write_aac_frame(data, size, pts);
}
void CFlvWriter::OnError(LPCTSTR lpszReason) {
}
bool CFlvWriter::GetStatus(LPSTR* json, void(**free)(LPSTR)) {
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

std::string CFlvWriter::GetHttpSucceedResponse(ISinkHttpRequest* request, const xtstring& contentType /* = _T("text/html") */) {
	xtstring strHeader;
	strHeader.Format(_T("%s 200 OK\r\n"), request->GetVersion());
	strHeader += _T("Server: simplehttp\r\n");
	strHeader += _T("Connection: close\r\n");
	strHeader += _T("Cache-Control: no-cache\r\n");
	if (!contentType.IsEmpty()) strHeader += "Content-Type: " + contentType + "\r\n";
	strHeader += _T("\r\n");
	return strHeader.toString().c_str();
}
void CFlvWriter::PostMediaData(const uint8_t* const lpData, const size_t nSize, const bool bKey) {
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

	m_latestSendData = m_tickCount;
	std::lock_guard<std::mutex> lock(m_lockClient);
	m_activedClient.erase(std::remove_if(m_activedClient.begin(), m_activedClient.end(),
		[this, needCheck, maxBuffer, lpData, nSize](cliptr cli) -> bool {
		if (cli->over) {
			CFlvFactory::DelPlayingCount();
			return true;
		} else if (cli->invalidAt != 0) {
			if (m_tickCount - cli->invalidAt > CLOSE_AFTER_INVALID_MS) {
				CFlvFactory::DelPlayingCount();
				return true;
			}
		} else {
			if (needCheck) {
				evbuffer* evbuf(bufferevent_get_output(cli->bev));
				size_t len(evbuffer_get_length(evbuf));
				if (len > maxBuffer) cli->invalidAt = m_tickCount;
			} if (cli->invalidAt == 0) {
				if (bufferevent_write(cli->bev, lpData, nSize) != 0) {
					CFlvFactory::DelPlayingCount();
					return true;
				}
				m_sendDataSize += nSize;
			} else {
				m_dropDataSize += nSize;
			}
		}
		return false;
	}), m_activedClient.end());
	m_maxBuffer += nSize;
	if (bKey && m_pendingClient.size() > 0) {
		for (auto client : m_pendingClient) {
			bufferevent* bev = bufferevent_socket_new(m_base, client.first, BEV_OPT_CLOSE_ON_FREE);
			if (bev == NULL) {
				evutil_closesocket(client.first);
				wle(_T("Create buffer event for client failed."));
				CFlvFactory::DelPlayingCount();
				continue;
			}
			cliptr cli(new ClientData(bev));
			bufferevent_setcb(bev, NULL, on_write, on_error, cli.get());
			if (bufferevent_enable(bev, EV_READ | EV_WRITE) != 0 ||
				bufferevent_write(bev, client.second.c_str(), client.second.length()) != 0 ||
				bufferevent_write(bev, m_headCache, m_headCache) != 0) {
				CFlvFactory::DelPlayingCount();
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
void CFlvWriter::DoDiscard() {
	if (m_lpCallback != nullptr) {
		m_lpCallback->OnDiscarded();
	}
	CFlvFactory::singleton().unbindWriter(this);
}

bool CFlvWriter::Discard() {
	DoDiscard();
	return true;
}

void CFlvWriter::on_error(struct bufferevent* bev, short what, void* context) {
	ClientData* cli(reinterpret_cast<ClientData*>(context));
	if (what & (BEV_EVENT_ERROR | BEV_EVENT_EOF | BEV_EVENT_TIMEOUT)) {
		cli->over = true;
	}
}
void CFlvWriter::on_write(struct bufferevent* event, void* context) {
	ClientData* cli(reinterpret_cast<ClientData*>(context));
	cli->invalidAt = 0;
}

Json::Value CFlvWriter::GetStatus() {
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

	//这里的数据受FlvFactory里面的锁保护，保证在调用这部分代码的时候对象不可能被Discard函数所释放
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
