#include "EventBase.h"
#include "Channel.h"
#include "ChannelFactory.h"
#include "Writelog.h"
#include "ConsoleUI.h"
#include "json/reader.h"
#include "xutils.h"

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
	, m_lpProxy(proxy)
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
	if (m_lpProxy->Discard()) {
		m_bDiscarded = true;
	}
}

//////////////////////////////////////////////////////////////////////////
//CChannel
CChannel::CChannel(const xtstring& moniker)
	: m_startupTime(time(NULL))
	, m_strMoniker(moniker)
	, m_stage(SS_Inited)
	, m_lkProxy()
	, m_lpProxy(nullptr)

	, m_lkSink()
	, m_spSinks()

	, m_base(nullptr)
	, m_timer(nullptr)
	, m_idleBeginAt(0)
	, m_lockCache()
	, m_rawHeader()
	, m_rawCache(8 * 1024)
	, m_latestSendData(std::tickCount())
	, m_rawDataSize(0)
	, m_error() {
	cpj(_T("[%s] 创建通道"), m_strMoniker.c_str());
	wli(_T("[%s] Create channel at: %p"), m_strMoniker.c_str(), this);
}
CChannel::~CChannel() {
	if (m_timer != nullptr) {
		event_free(m_timer);
		m_timer = nullptr;
	}
	cpj(_T("[%s] 释放通道"), m_strMoniker.c_str());
	wli(_T("[%s] Release channel at: %p"), m_strMoniker.c_str(), this);
}

bool CChannel::Startup(ISourceProxy* proxy) {
	if (!m_rawCache.EnsureSize(1024 * 1024)) {
		cpe(_T("[%s] 启动输入源失败"), m_strMoniker.c_str());
		return wlet(false, _T("Alloc body cache failed."));
	}
	event_base* base(CEventBase::singleton().nextBase());
	m_lpProxy = proxy;
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
		if (sp->m_eProxyStatus != PS_Closed && sp->m_lpProxy && sp->m_lpProxy->GetStatus(&lpsz, &freeXml) && lpsz != nullptr) {
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
	case CChannel::SS_Closing:
		return "关闭输出中";
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
	if (m_lpProxy != nullptr) m_lpProxy->WantKeyFrame();
}
void CChannel::OnMediaData(ISinkProxy::PackType type, const uint8_t* const data, const int size, const bool key /* = true */) {
	if (type == ISinkProxy::Header) {
		std::lock_guard<std::mutex> lock2(m_lockCache);
		m_rawHeader.AppendData(data, size);
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
	case CChannel::SS_Closing:
		continous = onClosing();
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

bool CChannel::onInited() {
	std::unique_lock<std::recursive_mutex> lock(m_lkProxy);
	if (!m_lpProxy->StartFetch(m_base)) {
		wle(_T("StartFetch() failed."));
		cpe(_T("[%s] 启动输入源失败"), m_lpProxy->SourceName());
		m_lpProxy = NULL;
		SetStage(SS_Closing);
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
				sink->m_lpProxy->OnRawPack((ISinkProxy::PackType)type, pos, size, key != 0);
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
		if (!sink->m_lpProxy->StartWrite(m_base)) {
			sink->Discard();
		} else {
			ISinkProxy::CodecType type(sink->m_lpProxy->PreferCodec());
			switch (type) {
			case ISinkProxy::CodecType::Raw:
				if (m_rawHeader.GetSize() > 0) {
					sink->m_lpProxy->OnRawPack(ISinkProxy::Header, m_rawHeader, m_rawHeader, true);
				}
				sink->m_eProxyStatus = PS_Raw;
				break;
			case ISinkProxy::CodecType::Compressed:
				sink->m_eProxyStatus = PS_Compressed;
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
	if (m_lpProxy != NULL) {
		if (m_lpProxy->Discard() && m_stage == SS_Discard) {
			wli(_T("[%s] Convert stage to Closing."), m_strMoniker.c_str());
			SetStage(SS_Waiting);
		}
	} else {
		SetStage(SS_Closing);
	}
	return true;
}
bool CChannel::onWaiting() {
	return true;
}
bool CChannel::onClosing() {
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
		SetStage(SS_Closed);
	}
	return true;
}
bool CChannel::onClosed() {
	CChannelFactory::singleton().unbindChannel(this);
	return false;
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
void CChannel::OnDiscarded() {
	wle(_T("[%s] OnDiscarded()."), m_strMoniker.c_str());
	std::unique_lock<std::recursive_mutex> lock(m_lkProxy);
	m_lpProxy = NULL;
	if (m_stage == SS_Fetching) {
		OnError(_T("Source Closed."));
	}
	SetStage(SS_Closing);
}