#pragma once
#include <mutex>
#include <event2/bufferevent.h>
#include "HttpClient.h"
#include "SinkInterface.h"
#include "json/value.h"
#include "xstring.h"
#include "Byte.h"
#include "xsystem.h"

struct event;
struct event_base;
class CHttpClient;
class CFlvWriter : public ISinkProxy {
public:
	CFlvWriter(const xtstring& moniker, const bool reusable);
	~CFlvWriter();

public:
	bool				Startup(ISinkProxyCallback* callback);
	bool				AddClient(CHttpClient* client);
	const xtstring&		GetMoniker() const { return m_strMoniker; }
	Json::Value			GetStatus();

protected:
	virtual CodecType	PreferCodec() { return CodecType::Raw; }
	virtual bool		StartWrite(event_base* base);
	virtual void		OnVideoFormat(const CodecID codec, const int width, const int height, const double fps) {}
	virtual void		OnVideoConfig(const CodecID codec, const uint8_t* header, const int size) {}
	virtual void		OnVideoFrame(const uint8_t* const data, const int size, const uint32_t pts) {}
	virtual void		OnAudioFormat(const CodecID codec, const int channel, const int bitwidth, const int samplerate) {}
	virtual void		OnAudioConfig(const CodecID codec, const uint8_t* header, const int size) {}
	virtual void		OnAudioFrame(const uint8_t* const data, const int size, const uint32_t pts) {}
	virtual void		OnRawPack(PackType type, const uint8_t* const data, const int size, const bool key = true);
	virtual void		OnError(LPCTSTR reason);
	virtual bool		GetStatus(LPSTR* json, void(**free)(LPSTR));
	virtual bool		Discard();

protected:
	void				PostMediaData(const PackType eType, const uint8_t* const lpData, const size_t nSize, const bool bKey);
	void				DoDiscard();

protected:
	static void			on_write(struct bufferevent* event, void* context);
	static void			on_error(struct bufferevent* event, short what, void* context);
	
protected:
	typedef CSmartHdr<bufferevent*, void> bevptr;
	struct ClientData {
		ClientData(bufferevent* _bev)
			: bev(_bev, bufferevent_free)
			, invalidAt(0)
			, over(false) {
		}
		~ClientData() {
			bev = nullptr;
		}
		bevptr bev;
		uint32_t invalidAt;
		bool over;
	};
	typedef std::shared_ptr<ClientData> cliptr;

private:
	const time_t m_startupTime;
	const xtstring m_strMoniker;
	event_base* m_base;
	ISinkProxyCallback*	m_lpCallback;

	unsigned int m_tickCount;
	unsigned int m_latestCheck;
	size_t m_maxBuffer;
	unsigned int m_idleBeginAt;
	CByte m_headCache;
	uint64_t m_receivedDataSize;
	uint64_t m_sendDataSize;
	uint64_t m_dropDataSize;
	unsigned int m_latestSendData;

	std::mutex m_lockClient;
	std::map<evutil_socket_t, std::string> m_pendingClient;
	std::vector<cliptr> m_activedClient;
};

