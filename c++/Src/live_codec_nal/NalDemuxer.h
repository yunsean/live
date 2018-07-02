#pragma once
#include <mutex>
#include "HttpClient.h"
#include "CodecInterface.h"
#include "json/value.h"
#include "xstring.h"
#include "Byte.h"
#include "xsystem.h"

/**
* 头格式：  
* 1字节视音频标识： V视频  A音频
* 4字节视音频编码： ' CVA' ' CAA' ' 3PM'
* 2字节配置块长度： 
* N字节配置块数据
* 循环

* 帧格式：
* 4字节时间戳
* N字节数据（H264带4字节前导0x00000001）
************************************************************************/

struct event;
struct event_base;
class CHttpClient;
class CNalDemuxer : public IDecodeProxy {
public:
	CNalDemuxer(IDecodeProxyCallback* callback);
	~CNalDemuxer();

protected:
	virtual bool	Start();
	virtual bool	CrossThread() { return false; }
	virtual bool	AddHeader(const uint8_t* const data, const int size);
	virtual bool	AddVideo(const uint8_t* const data, const int size);
	virtual bool	AddAudio(const uint8_t* const data, const int size);
	virtual bool	AddCustom(const uint8_t* const data, const int size);
	virtual void	Stop();
	virtual bool	Discard();

private:
	IDecodeProxyCallback* m_lpCallback;
};

