#pragma once
#include <memory>
#include <functional>
#include "xstring.h"
#include "freesdp/freesdp/freesdp.h"

class CRtpBaseDemuxer {
public:
	CRtpBaseDemuxer();
	~CRtpBaseDemuxer();

public:
	virtual bool init(const fsdp_media_description_t* media) = 0;
	virtual void setCallback(const std::function<void(const uint8_t* data, int size, uint32_t timecode)>& callback) { m_callback = callback; }
	virtual const char* control() const = 0;
	virtual void setInterleaved(int interleaved) { m_interleaved = interleaved; }
	virtual void setClientPort(int port) { m_clientPort = port; }
	virtual std::string transport() const;
	virtual int interleaved() const { return m_interleaved; }
	virtual int clientPort() const { return m_clientPort; }
	virtual const uint8_t* extradata(int& size) const { size = 0; return nullptr; }
	virtual bool handle_packet(const uint8_t *buf, int len, uint32_t timestamp, uint16_t seq, int flags) = 0;
	virtual uint32_t fourCC() const = 0;

protected:
	void get_word_until_chars(char *buf, int buf_size, const char *sep, const char **pp);
	void get_word_sep(char *buf, int buf_size, const char *sep, const char **pp);
	int rtsp_next_attr_and_value(const char **p, char *attr, int attr_size, char *value, int value_size);

protected:
	int m_interleaved;
	int m_clientPort;
	std::function<void (const uint8_t* data, int size, uint32_t timecode)> m_callback;
};

