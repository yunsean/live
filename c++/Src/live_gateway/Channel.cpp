#include "EventBase.h"
#include "Channel.h"
#include "ChannelFactory.h"
#include "DecodeFactory.h"
#include "Writelog.h"
#include "ConsoleUI.h"
#include "json/reader.h"
#include "xutils.h"
#include "SmartRet.h"

#define CLOSE_AFTER_INVALID_MS	(10 * 1000)
#define CHECK_INTERVAL			(1 * 1000)
#define BUFFER_THRESHOLD		2
#define RELEASE_AFTER_IDEL_MS	(5 * 1000)
#define RELEASE_NODATA_MS		(15 * 1000)

//////////////////////////////////////////////////////////////////////////
//CSinkProxy
CChannel::CSinkProxy::CSinkProxy(std::recursive_mutex& mutex, CChannel* channel, ISinkProxy* proxy)
	: m_lkProxy(mutex)
	, m_eProxyStatus(PS_Pending)
	, m_lpChannel(channel)
	, m_lpSourcer(proxy)
	, m_bDiscarded(false) {
}
void CChannel::CSinkProxy::WantKeyFrame() {
	if (m_lpChannel != nullptr) {
		m_lpChannel->WantKeyFrame();
	}
}
void CChannel::CSinkProxy::OnDiscarded() {
	std::lock_guard<std::recursive_mutex> lock(m_lkProxy);
	m_eProxyStatus = PS_Closed;
}
void CChannel::CSinkProxy::Discard() {
	if (m_bDiscarded) return;
	m_eProxyStatus = PS_Discarded;
	if (m_lpSourcer->Discard()) {
		m_bDiscarded = true;
	}
}


//////////////////////////////////////////////////////////////////////////
//CControlClient
CChannel::CControlClient::CControlClient(const int a, const int s)
	: bev(bufferevent_free)
	, invalidAt(0)
	, mode(false)
	, state(ControlState::pending)
	, action(a)
	, speed(s) {
}
CChannel::CControlClient::CControlClient(const int b, const int c, const int s, const int h)
	: bev(bufferevent_free)
	, invalidAt(0)
	, mode(true)
	, state(ControlState::pending)
	, bright(b)
	, contrast(c)
	, saturation(s)
	, hue(h) {
}


//////////////////////////////////////////////////////////////////////////
//CChannel
CChannel::CChannel(const xtstring& moniker)
	: m_startupTime(time(NULL))
	, m_strMoniker(moniker)
	, m_stage(SS_Inited)
	, m_lkProxy()
	, m_bNeedDecoder(false)
	, m_lpSourcer(nullptr)
	, m_lpDecoder(nullptr)

	, m_lkSink()
	, m_spSinks()

	, m_base(nullptr)
	, m_timer(nullptr)
	, m_idleBeginAt(0)
	, m_lockCache()
	, m_rawHeaders()
	, m_rawCache(8 * 1024)
	, m_latestSendData(std::tickCount())
	, m_rawDataSize(0)
	, m_error()

	, m_lockClient()
	, m_pendingClient()
	, m_activedClient()
	, m_token(1) {
	cpj(_T("[%s] 创建通道"), m_strMoniker.c_str());
	wli(_T("[%s] Create channel at: %p"), m_strMoniker.c_str(), this);
}
CChannel::~CChannel() {
	if (m_timer != nullptr) {
		event_free(m_timer);
		m_timer = nullptr;
	}
	for (auto cli : m_pendingClient) {
		evutil_closesocket(cli.first);
	}
	m_activedClient.clear();
	m_pendingClient.clear();
	cpj(_T("[%s] 释放通道"), m_strMoniker.c_str());
	wli(_T("[%s] Release channel at: %p"), m_strMoniker.c_str(), this);
}

bool CChannel::Startup(ISourceProxy* proxy) {
	if (!m_rawCache.EnsureSize(1024 * 1024)) {
		cpe(_T("[%s] 启动输入源失败"), m_strMoniker.c_str());
		return wlet(false, _T("Alloc body cache failed."));
	}
	event_base* base(CEventBase::singleton().preferBase());
	m_lpSourcer = proxy;
	m_base = base;
	m_timer = evtimer_new(base, timer_callback, this);
	if (m_timer == nullptr) {
		cpe(_T("[%s] 分配缓冲区错误"), m_strMoniker.c_str());
		return wlet(false, _T("Startup source failed."));
	}
	event_active(m_timer, EV_TIMEOUT, 0);
	return true;
}
ISinkProxyCallback* CChannel::AddSink(ISinkProxy* client) {
	std::unique_lock<std::recursive_mutex> lock(m_lkSink);
	if (m_stage != SS_Fetching && m_stage != SS_Inited) return wlet(nullptr, _T("[%s] The channel is closing, stage=%d"), m_strMoniker.c_str(), m_stage);
	CSmartPtr<CSinkProxy> callback(new CSinkProxy(m_lkSink, this, client));
	m_spSinks.push_back(callback);
	return callback;
}
bool CChannel::PTZControl(CHttpClient* client, const int action, const int speed) {
	if (m_lpSourcer == nullptr) return false;
	std::lock_guard<std::recursive_mutex> lock(m_lockClient);
	m_pendingClient.insert(std::make_pair(client->fd(true), std::shared_ptr<CControlClient>(new CControlClient(action, speed))));
	m_idleBeginAt = std::tickCount();
	return true;
}
bool CChannel::VideoEffect(CHttpClient* client, const int bright, const int contrast, const int saturation, const int hue) {
	if (m_lpSourcer == nullptr) return false;
	std::lock_guard<std::recursive_mutex> lock(m_lockClient);
	m_pendingClient.insert(std::make_pair(client->fd(true), std::shared_ptr<CControlClient>(new CControlClient(bright, contrast, saturation, hue))));
	m_idleBeginAt = std::tickCount();
	return true;
}
void CChannel::GetStatus(Json::Value& status) {
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

	typedef void(*FnFreeJson)(LPSTR);
	status["通道名称"] = m_strMoniker.toString().c_str();
	status["当前状态"] = getStage();
	status["最近信源"] = m_latestSendData;
	status["总数据量"] = number2Size(m_rawDataSize);
	time_t now(time(NULL));
	unsigned int seconds(static_cast<int>(now - m_startupTime));
	int day = seconds / (24 * 3600);
	seconds %= (24 * 3600);
	int hour = seconds / 3600;
	seconds %= 3600;
	int minute = seconds / 60;
	status["运行时间"] = xstring<char>::format("%d天%d小时%d分", day, hour, minute);
	std::lock_guard<std::recursive_mutex> lock(m_lkSink);
	Json::Value sinks;
	for (auto sp : m_spSinks) {
		Json::Value sink;
		LPSTR lpsz(nullptr);
		FnFreeJson freeXml(nullptr);
		if (sp->m_eProxyStatus != PS_Closed && sp->m_lpSourcer && sp->m_lpSourcer->GetStatus(&lpsz, &freeXml) && lpsz != nullptr) {
			Json::Reader reader;
			reader.parse(lpsz, sink);
			if (freeXml != nullptr) freeXml(lpsz);
		}
		sink["Sink状态"] = getStatus(sp->m_eProxyStatus);
		sinks.append(sink);
	}
	status["Sink列表"] = sinks;
}
const char* CChannel::getStage() {
	switch (m_stage) {
	case CChannel::SS_Inited:
		return "初始化";
	case CChannel::SS_Fetching:
		return "执行中";
	case CChannel::SS_Discard:
		return "等待回收";
	case CChannel::SS_Waiting:
		return "关闭信源中";
	case CChannel::SS_SinkClosing:
		return "关闭输出中";
	case CChannel::SS_DecodeClosing:
		return "关闭解码器中";
	case CChannel::SS_Closed:
		return "已关闭";
	default:
		return "未知";
	}
}
const char* CChannel::getStatus(ProxyStatus status) {
	switch (status) {
	case CChannel::PS_Pending:
		return "初始化";
	case CChannel::PS_Raw:
		return "源转发";
	case CChannel::PS_Compressed:
		return "压缩转发";
	case CChannel::PS_Baseband:
		return "基带转发";
	case CChannel::PS_Discarded:
		return "关闭中";
	case CChannel::PS_Closed:
		return "已关闭";
	default:
		return "未知";
	}
}

void CChannel::WantKeyFrame() {
	std::lock_guard<std::recursive_mutex> lock3(m_lkProxy);
	if (m_lpSourcer != nullptr) m_lpSourcer->WantKeyFrame();
}
void CChannel::OnMediaData(ISinkProxy::PackType type, const uint8_t* const data, const int size, const bool key /* = true */) {
	if (type == ISinkProxy::Header) {
		std::lock_guard<std::mutex> lock2(m_lockCache);
		CSmartNal<uint8_t> nal;
		nal.Copy(data, size);
		m_rawHeaders.push_back(nal);
	}
	if (m_stage == SS_Fetching) {
		std::lock_guard<std::mutex> lock2(m_lockCache);
		if (m_rawCache.GetSize() > 5 * 1024 * 1024) {
			m_error = _T("Cache too long.");
			cpe(_T("[%s] 数据缓冲区溢出"), m_strMoniker.c_str());
			SetStage(SS_Discard);
			return;
		}
		if (!m_rawCache.EnsureSize(m_rawCache.GetSize() + 6 + size)) {
			m_error = _T("Alloc cache failed.");
			cpe(_T("[%s] 分配缓冲区错误"), m_strMoniker.c_str());
			SetStage(SS_Discard);
			return;
		}
		uint8_t* pos(m_rawCache.GetPos());
		pos = utils::uintTo4BytesLE(size + 6, pos);
		*pos++ = type;
		*pos++ = key ? 1 : 0;
		memcpy(pos, data, size);
		m_rawCache.GrowSize(size + 6);
		m_rawDataSize += size;
	}
}
void CChannel::SetStage(SourceStage stage) {
	wli(_T("[%s] Convert stage from %d to %d."), m_strMoniker.c_str(), m_stage, stage);
	m_stage = stage;
}

void CChannel::timer_callback(evutil_socket_t fd, short events, void* context) {
	CChannel* thiz(reinterpret_cast<CChannel*>(context));
	thiz->timer_callback(fd, events);
}
void CChannel::timer_callback(evutil_socket_t fd, short events) {
	handleControlRequest();

	bool continous(true);
	switch (m_stage) {
	case CChannel::SS_Inited:
		continous = onInited();
		break;
	case CChannel::SS_Fetching: 
		continous = onFetching();
		break;
	case CChannel::SS_Discard:
		continous = onDiscard();
		break;
	case CChannel::SS_Waiting:
		continous = onWaiting();
		break;
	case CChannel::SS_SinkClosing:
		continous = onSinkClosing();
		break;
	case CChannel::SS_DecodeClosing:
		continous = onDecodeClosing();
		break;
	case CChannel::SS_Closed: 
		continous = onClosed();
		break;
	}

	if (continous) {
		static struct timeval tv = { 0, 50 * 1000 };
		event_add(m_timer, &tv);
	}
}
void CChannel::handleControlRequest() {
	std::lock_guard<std::recursive_mutex> lock(m_lockClient);
	if (m_pendingClient.size() > 0) {
		auto invalidAt(std::tickCount() + 5 * 1000);
		for (auto client : m_pendingClient) {
			cliptr cli(client.second);
			bevptr bev(bufferevent_socket_new(m_base, client.first, BEV_OPT_CLOSE_ON_FREE), bufferevent_free);
			if (bev == NULL) {
				evutil_closesocket(client.first);
				wle(_T("Create buffer event for client failed."));
				continue;
			}
			cli->bev = bev;
			bufferevent_setcb(bev, NULL, NULL, on_error, cli.get());
			if (bufferevent_enable(bev, EV_READ | EV_WRITE) != 0) {
				wle(_T("Set event for client failed."));
				continue;
			}
			cli->invalidAt = invalidAt;
			handleControl(cli);
		}
		m_pendingClient.clear();
	}
	auto tickCount(std::tickCount());
	for (auto it = m_activedClient.begin(); it != m_activedClient.end(); ) {
		cliptr cli(it->second);
		auto current(it++);
		if (cli->state == pending && cli->invalidAt < tickCount) cli->state = failed;
		if (cli->state == finished) sendResponse(cli);
		else if (cli->state == failed) sendResponse(cli, 400, "Failed");
		if (cli->state == closed) m_activedClient.erase(current);
	}
}
void CChannel::handleControl(cliptr cli) {
	int token = m_token++;
	m_activedClient.insert(std::make_pair(token, cli));
	auto result(cli->mode ? m_lpSourcer->VideoEffect(token, cli->bright, cli->contrast, cli->saturation, cli->hue) : m_lpSourcer->PTZControl(token, cli->action, cli->speed));
	if (result == ISourceProxy::Failed) sendResponse(cli, 400, "Failed");
	else if (result == ISourceProxy::Finished) sendResponse(cli);
}
void CChannel::sendResponse(cliptr cli, int code /* = 20 */, const char* status /* = "OK" */) {
	xstring<char> strHeader;
	strHeader.Format("HTTP/1.1 %d %s\r\n", code, status);
	strHeader += "Server: simplehttp\r\n";
	strHeader += "Connection: close\r\n";
	strHeader += "Cache-Control: no-cache\r\n";
	strHeader += "Content-Length: 0\r\n";
	strHeader += "\r\n";
	const char* data(strHeader.c_str());
	const size_t size(strHeader.length());
	bufferevent_write(cli->bev, data, size);
	bufferevent_flush(cli->bev, EV_WRITE, BEV_FLUSH);
	cli->state = over;
}
void CChannel::on_error(struct bufferevent* bev, short what, void* context) {
	CControlClient* cli(reinterpret_cast<CControlClient*>(context));
	if (what & (BEV_EVENT_ERROR | BEV_EVENT_EOF | BEV_EVENT_TIMEOUT)) {
		cli->state = closed;
	}
}

bool CChannel::onInited() {
	if (m_spSinks.size() < 1) {
		uint32_t tickCount(std::tickCount());
		if (m_idleBeginAt == 0) m_idleBeginAt = tickCount;
		else if (tickCount - m_idleBeginAt > RELEASE_NODATA_MS) SetStage(SS_Discard);
		return true;
	}
	std::unique_lock<std::recursive_mutex> lock(m_lkProxy);
	if (!m_lpSourcer->StartFetch(m_base)) {
		wle(_T("StartFetch() failed."));
		cpe(_T("[%s] 启动输入源失败"), m_lpSourcer->SourceName());
		m_lpSourcer = NULL;
		SetStage(SS_SinkClosing);
	} else {
		SetStage(SS_Fetching);
	}
	return true;
}
bool CChannel::onFetching() {
	uint32_t tickCount(std::tickCount());
	std::lock_guard<std::recursive_mutex> lock(m_lkSink);
	if (m_spSinks.size() == 0) {
		if (m_idleBeginAt != 0) {
			if (tickCount - m_idleBeginAt > RELEASE_AFTER_IDEL_MS) {
				SetStage(SS_Discard);
			}
		} else {
			m_idleBeginAt = tickCount;
		}
		return true;
	} else {
		m_idleBeginAt = 0;
	}

	if (tickCount - m_latestSendData > RELEASE_NODATA_MS) {
		m_error = _T("Source fetch timeout.");
		cpw(_T("[%s] 信号源超时返回数据，疑似断流."), m_strMoniker.c_str());
		wlw(_T("[%s] The latest send data is too long ago."), m_strMoniker.c_str());
		SetStage(SS_Discard);
		return true;
	}

	if (m_lpDecoder == nullptr && m_bNeedDecoder && m_rawHeaders.size() > 0) {
		if (!CreateDecoder()) {
			wle(_T("Create decoder failed, will discard all sinks which need compression data."));
			for (auto sink : m_spSinks) {
				if (sink->m_eProxyStatus != PS_Compressed && sink->m_eProxyStatus != PS_Baseband) continue;
				sink->Discard();
			}
			m_bNeedDecoder = false;
		}
	} else if (m_lpDecoder != nullptr && !m_bNeedDecoder) {
		if (m_lpDecoder->Discard()) {
			m_lpDecoder = nullptr;
		}
	}
	
	std::lock_guard<std::mutex> lock2(m_lockCache);
	if (m_rawCache.GetSize() > 0) {
		uint8_t* pos(m_rawCache.GetData());
		int length(m_rawCache.GetSize());
		uint8_t* end(pos + length);
		while (pos + 6 < end) {
			uint32_t size(utils::uintFrom3BytesLE(pos) - 6);
			pos += 4;
			uint8_t type(*pos++);
			uint8_t key(*pos++);
			if (pos + size > end) break;
			for (auto sink : m_spSinks) {
				if (sink->m_eProxyStatus != PS_Raw) continue;
				sink->m_lpSourcer->OnRawPack((ISinkProxy::PackType)type, pos, size, key != 0);
			}
			if (m_lpDecoder != nullptr) {
				switch (type) {
				case ISinkProxy::Header:
					m_lpDecoder->AddHeader(pos, size);
					break;
				case ISinkProxy::Video:
					m_lpDecoder->AddVideo(pos, size);
					break;
				case ISinkProxy::Audio:
					m_lpDecoder->AddAudio(pos, size);
					break;
				case ISinkProxy::Error:
					if (m_lpDecoder->Discard()) {
						m_lpDecoder = nullptr;
					}
					break;
				case ISinkProxy::Custom:
					m_lpDecoder->AddCustom(pos, size);
					break;
				}
			}
			pos += size;
		}
		m_rawCache.Clear();
		m_latestSendData = tickCount;
	}

	bool hasNewSink(false);
	m_spSinks.erase(std::remove_if(m_spSinks.begin(), m_spSinks.end(),
		[this, &hasNewSink](CSmartPtr<CSinkProxy>& sink) -> bool {
		if (sink->m_eProxyStatus == PS_Closed) return true;
		if (sink->m_eProxyStatus != PS_Pending) return false;
		if (!sink->m_lpSourcer->StartWrite(m_base)) {
			sink->Discard();
		} else {
			ISinkProxy::CodecType type(sink->m_lpSourcer->PreferCodec());
			switch (type) {
			case ISinkProxy::CodecType::Raw:
				for (auto nal : m_rawHeaders) {
					sink->m_lpSourcer->OnRawPack(ISinkProxy::Header, nal.GetArr(), nal.GetSize(), true);
				}
				sink->m_eProxyStatus = PS_Raw;
				break;
			case ISinkProxy::CodecType::Compressed:
				sink->m_eProxyStatus = PS_Compressed;
				m_bNeedDecoder = true;
				if (m_rawHeaders.size() > 0 && !CreateDecoder()) {
					wle(_T("Create decoder failed."));
					sink->Discard();
				}
				break;
			case ISinkProxy::CodecType::Baseband:
				sink->m_eProxyStatus = PS_Baseband;
				break;
			}
			hasNewSink = true;
		}
		return false;
	}), m_spSinks.end());
	if (hasNewSink) {
		WantKeyFrame();
	}
	return true;
}
bool CChannel::onDiscard() {
	std::unique_lock<std::recursive_mutex> lock(m_lkProxy);
	if (m_lpSourcer != NULL) {
		if (m_lpSourcer->Discard() && m_stage == SS_Discard) {
			wli(_T("[%s] Convert stage to Closing."), m_strMoniker.c_str());
			SetStage(SS_Waiting);
		}
	} else {
		SetStage(SS_SinkClosing);
	}
	return true;
}
bool CChannel::onWaiting() {
	return true;
}
bool CChannel::onSinkClosing() {
	std::lock_guard<std::recursive_mutex> lock(m_lkSink);
	m_spSinks.erase(std::remove_if(m_spSinks.begin(), m_spSinks.end(), 
		[this](CSmartPtr<CSinkProxy>& sink) -> bool {
		if (sink->m_eProxyStatus == PS_Closed) {
			return true;
		}
		sink->Discard();
		return false;
	}), m_spSinks.end());
	if (m_spSinks.size() == 0) {
		SetStage(SS_DecodeClosing);
	}
	return true;
}
bool CChannel::onDecodeClosing() {
	if (m_lpDecoder != nullptr && !m_lpDecoder->Discard()) {
		return true;
	}
	SetStage(SS_Closed);
	return true;
}
bool CChannel::onClosed() {
	CChannelFactory::singleton().unbindChannel(this);
	return false;
}

bool CChannel::CreateDecoder() {
	if (m_rawHeaders.size() < 1) return wlet(false, _T("No header found."));
	CSmartPtr<IDecodeProxy> decoder(CDecodeFactory::singleton().createDecoder(this, m_rawHeaders[0].GetArr(), m_rawHeaders[0].GetSize()), 
		[](IDecodeProxy* lp) {
		lp->Discard();
	});
	if (decoder == nullptr) return wlet(false, _T("Not found any decoder can support this source."));
	if (!decoder->Start()) return wlet(false, _T("Start decoder failed"));
	for (auto header : m_rawHeaders) {
		if (!decoder->AddHeader(header.GetArr(), header.GetSize())) {
			return wlet(false, _T("Add header to decoder failed"));
		}
	}
	m_lpDecoder = decoder.Detach();
	return true;
}

void CChannel::OnHeaderData(const uint8_t* const data, const int size) {
	OnMediaData(ISinkProxy::Header, data, size);
}
void CChannel::OnVideoData(const uint8_t* const data, const int size, const bool key /*= true*/) {
	OnMediaData(ISinkProxy::Video, data, size, key);
}
void CChannel::OnAudioData(const uint8_t* const data, const int size) {
	OnMediaData(ISinkProxy::Audio, data, size);
}
void CChannel::OnCustomData(const uint8_t* const data, const int size) {
	OnMediaData(ISinkProxy::Custom, data, size);
}
void CChannel::OnError(LPCTSTR reason) {
	cpe(_T("[%s] %s"), m_strMoniker.c_str(), reason);
	m_error = reason;
	wle(_T("[%s] %s"), m_strMoniker.c_str(), reason);
	SetStage(SS_Discard);
}
void CChannel::OnStatistics(LPCTSTR statistics) {

}
void CChannel::OnControlResult(const unsigned int token, bool result, LPCTSTR reason) {
	std::lock_guard<std::recursive_mutex> lock(m_lockClient);
	auto found(m_activedClient.find(token));
	if (found == m_activedClient.end()) return;
	auto cli(found->second);
	cli->state = result ? finished : failed;
}
void CChannel::OnDiscarded() {
	wle(_T("[%s] OnDiscarded()."), m_strMoniker.c_str());
	std::unique_lock<std::recursive_mutex> lock(m_lkProxy);
	m_lpSourcer = NULL;
	if (m_stage == SS_Fetching) {
		OnError(_T("Source Closed."));
	}
	SetStage(SS_SinkClosing);
}

void CChannel::OnVideoFormat(const CodecID codec, const int width, const int height, const double fps) {
	m_videoCodec = codec;
}
void CChannel::OnVideoConfig(const CodecID codec, const uint8_t* header, const int size) {
	//TODO 处理跨线程回调
	m_videoCodec = codec;
	for (auto sink : m_spSinks) {
		if (sink->m_eProxyStatus != PS_Compressed) continue;
		sink->m_lpSourcer->OnVideoConfig((ISinkProxy::CodecID)codec, header, size);
	}
}
void CChannel::OnVideoFrame(const uint8_t* const data, const int size, const uint32_t pts) {
	//TODO 处理跨线程回调
	if (m_videoCodec == CodecID::AVC) {
		for (auto sink : m_spSinks) {
			if (sink->m_eProxyStatus != PS_Compressed) continue;
			sink->m_lpSourcer->OnVideoFrame(data, size, pts);
		}
	}
}
void CChannel::OnAudioFormat(const CodecID codec, const int channel, const int bitwidth, const int samplerate) {
	m_audioCodec = codec;
}
void CChannel::OnAudioConfig(const CodecID codec, const uint8_t* header, const int size) {
	//TODO 处理跨线程回调
	m_audioCodec = codec;
	for (auto sink : m_spSinks) {
		if (sink->m_eProxyStatus != PS_Compressed) continue;
		sink->m_lpSourcer->OnAudioConfig((ISinkProxy::CodecID)codec, header, size);
	}
}
void CChannel::OnAudioFrame(const uint8_t* const data, const int size, const uint32_t pts) {
	//TODO 处理跨线程回调
	if (m_audioCodec == CodecID::AAC) {
		for (auto sink : m_spSinks) {
			if (sink->m_eProxyStatus != PS_Compressed) continue;
			sink->m_lpSourcer->OnAudioFrame(data, size, pts);
		}
	}
}
