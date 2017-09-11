#include "FileSource.h"
#include "Writelog.h"
#include "ConsoleUI.h"
#include "xmember.h"
#include "xsystem.h"
#include "xaid.h"

#define MAX_UINT32	(uint32_t)(~0)

CFileSource::CFileSource(const xtstring& strMoniker, const xtstring& strParam)
	: m_moniker(strMoniker)
	, m_spReader(NULL)
	, m_snHeader()
	, m_timer(event_free)

	, m_lpCallback(NULL)
	, m_uBeginTC(0)
	, m_uRefTime(MAX_UINT32)
	, m_bDiscard(false) {
	cpm(_T("[%s] 创建File Source"), m_moniker.c_str());
}
CFileSource::~CFileSource(void) {
	cpm(_T("[%s] 释放File Source"), m_moniker.c_str());
	if (m_lpCallback)m_lpCallback->OnDiscarded();
	m_lpCallback = NULL;
}

bool CFileSource::Open(ISourceProxyCallback* callback, const xtstring& strFile) {
	m_spReader = CreateVpfReader();
	if (m_spReader == NULL)return wlet(false, _T("new CFileReader() == NULL"));
	if (!m_spReader->Open(strFile))return wlet(false, _T("CFileReader.Open(%s) failed."), strFile.c_str());
	uint8_t* lpHeader(NULL);
	int szHeader(0);
	if (!m_spReader->GetHeader(lpHeader, szHeader))return wlet(false, _T("Read header failed."));
	m_snHeader.Copy(lpHeader, szHeader);
	m_lpCallback = callback;
	return true;
}
void CFileSource::ev_callback(evutil_socket_t fd, short flags, void* context) {
	CFileSource* thiz(reinterpret_cast<CFileSource*>(context));
	if (thiz != nullptr) thiz->ev_callback(fd, flags);
}
void CFileSource::ev_callback(evutil_socket_t fd, short flags) {
	if (m_bDiscard) {
		delete this;
		return;
	}
	int wait(0);
	while (wait < 1) {
		uint8_t* lpData(NULL);
		int szData(0);
		uint8_t btType(0);
		uint32_t uTimecode(0);
		if (!m_spReader->ReadBody(btType, lpData, szData, uTimecode)) {
			if (m_spReader->IsEof()) {
				if (m_lpCallback)m_lpCallback->OnError(_T("File eof."));
			}
			else {
				if (m_lpCallback)m_lpCallback->OnError(_T("Read file failed."));
			}
			m_bDiscard = true;
			wait = 1;
		} else {
			uint32_t dwCur(std::tickCount());
			if (m_uRefTime == MAX_UINT32) {
				m_uRefTime = dwCur;
				m_uBeginTC = uTimecode;
			}
			if (m_lpCallback == NULL)m_lpCallback = NULL;
			else if (btType == Header)m_lpCallback->OnHeaderData(lpData, szData);
			else if (btType == Video)m_lpCallback->OnVideoData(lpData, szData);
			else if (btType == Audio)m_lpCallback->OnAudioData(lpData, szData);
			else if (btType == Custom)m_lpCallback->OnCustomData(lpData, szData);
			wait = (int)(uTimecode - m_uBeginTC) - (int)(dwCur - m_uRefTime);
		}
	}
	if (wait <= 0) wait = 1;
	struct timeval tv = { wait / 1000, (wait % 1000) * 1000 };
	int result(evtimer_add(m_timer, &tv));
	if (result != 0) {
		if (m_lpCallback)m_lpCallback->OnError(_T("Internal error."));
		m_bDiscard = true;
		event_base_once(event_get_base(m_timer), -1, EV_TIMEOUT, ev_callback, this, NULL);
		return wle(_T("Add event to loop failed: %d."), result);
	}
}

//////////////////////////////////////////////////////////////////////////
bool CFileSource::StartFetch(event_base* base) {
	if (m_snHeader == NULL)return false;
	struct event* ev(evtimer_new(base, ev_callback, this));
	if (ev == NULL) {
		delete this;
		return wlet(false, _T("Assign event failed."));
	}
	m_timer = ev;
	static struct timeval tv = { 0, 1 * 1000 };
	int result(evtimer_add(m_timer, &tv));
	if (result != 0) {
		delete this;
		return wlet(false, _T("Startup event failed, result=%d, base=%p, ev=%p"), result, base, ev);
	}
	m_lpCallback->OnHeaderData(m_snHeader, m_snHeader.GetSize());
	return true;
}
void CFileSource::WantKeyFrame() {
}
bool CFileSource::PTZControl(const PTZAction eAction, const int nValue) {
	return false;
}
bool CFileSource::VideoEffect(const int nBright, const int nContrast, const int nSaturation, const int nHue) {
	return false;
}
bool CFileSource::Discard() {
	m_bDiscard = true;
	return true;
}
