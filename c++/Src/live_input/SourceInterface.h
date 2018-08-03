#pragma once
#include <map>
#include "os.h"
#include "xchar.h"

#ifndef MKFCC
#define MKFCC(a, b, c, d) (((uint32_t)a << 0) + ((uint32_t)b << 8) + ((uint32_t)c << 16) + ((uint32_t)d << 24))
#endif

interface ISourceProxyCallback {
	virtual ~ISourceProxyCallback() {}
	virtual void			OnHeaderData(const uint8_t* const data, const int size) = 0;
	virtual void			OnVideoData(const uint8_t* const data, const int size, const bool key = true) = 0;
	virtual void			OnAudioData(const uint8_t* const data, const int size) = 0;
	virtual void			OnCustomData(const uint8_t* const data, const int size) = 0;
	virtual void			OnError(LPCTSTR reason) = 0;
	virtual void			OnStatistics(LPCTSTR statistics) = 0;
	virtual void			OnControlResult(const unsigned int token, bool result, LPCTSTR reason) = 0;
	virtual void			OnDiscarded() = 0;
};

/*输入源代理类
* 工作流程：
*	1、	网关会通过Factory工厂方式创建SourceProxy的对象；	
*	2、	SourceProxy对象内部可以有自己的工作线程（event）；
*	3、	在网关调用ISourceProxy::Discard前，SourceProxy对象绝对不能自动释放自己；
*	4、	当SourceProxy对象内部出错时，首先调用OnError通知网关，并且可以终止内部线程，但是不能释放自己；
*	5、	当网关通过OnError检测到对象错误，或者网关需要主动释放SourceProxy对象时，会调用Discard释放SourceProxy对象；
*	6、	在Discard中，SourceProxy可以同步释放自己，也可以在自己的线程中异步释放自己，
		但在释放自己完成后，必须通过OnDestroied告知网关自己已经释放完成。
*	7、	网关在收到OnDestroied后，才会安全的释放内部的Source对象。

*PTZ和VideoEffect控制流程：
*	1、网关会通过PTZControl或者VideoEffect发送命令；
*	2、如果SourceProxy内部立即执行命令，则直接返回Failed或者Finished代表执行完成；
*	3、如果SourceProxy需要一段时间（比如异步网络请求）执行命令，则应该先返回Pending，
	   并且在执行完成（或者异步响应）后，通过ISourceProxyCallback.OnControlResult回调执行结果
*	4、回调模式下，网关最多等待5s，否则视为执行失败
*/
struct event_base;
interface ISourceProxy {
	enum PTZAction { Stop = 0x0000, ZoomIn = 0x0001, ZoomOut = 0x0002, FocusNear = 0x0004, FocusFar = 0x0008,
		IrisOpen = 0x0010, IrisClose = 0x0020, TiltUp = 0x0040, TiltDown = 0x0080, PanLeft = 0x0100, PanRight = 0x0200 };
	enum ControlResult { Failed = -1, Finished = 0, Pending = 1};
	virtual ~ISourceProxy() {}
	virtual LPCTSTR			SourceName() const = 0;
	virtual unsigned int    MaxStartDelay(const unsigned int def) { return def; }
	virtual unsigned int    MaxFrameDelay(const unsigned int def) { return def; }
	virtual bool			StartFetch(event_base* base) = 0;
	virtual void			WantKeyFrame() = 0;
	virtual ControlResult	PTZControl(const unsigned int token, const unsigned int action, const int speed) = 0;
	virtual ControlResult	VideoEffect(const unsigned int token, const int bright, const int contrast, const int saturation, const int hue) = 0;
	virtual bool			Discard() = 0;
};

interface ISourceFactoryCallback {
	virtual ~ISourceFactoryCallback() {}
	virtual event_base*		GetPreferBase() = 0;
	virtual event_base**	GetAllBase(int& count) = 0;
	virtual void			OnNewSource(LPCTSTR lpszDevice, LPCTSTR lpszMoniker, LPCTSTR lpszName, LPCTSTR lpszMeta = NULL) = 0;
	virtual void			OnSourceOver(LPCTSTR lpszDevice, LPCTSTR lpszMoniker) = 0;
};

struct event_base;
interface ISourceFactory {
	enum SupportState { unsupport, supported, maybesupport };
	virtual ~ISourceFactory() {}
	virtual LPCTSTR			FactoryName() const = 0;
	virtual bool			Initialize(ISourceFactoryCallback* callback) = 0;
	virtual SupportState	DidSupport(LPCTSTR device, LPCTSTR moniker, LPCTSTR param) = 0;
	virtual ISourceProxy*	CreateLiveProxy(ISourceProxyCallback* callback, LPCTSTR device, LPCTSTR moniker, LPCTSTR param = NULL) = 0;
	virtual ISourceProxy*	CreatePastProxy(ISourceProxyCallback* callback, LPCTSTR device, LPCTSTR moniker, LPCTSTR param, uint64_t beginTime, uint64_t endTime = 0) = 0;
	virtual bool			GetSourceList(LPTSTR* xml, void(**free)(LPTSTR), bool onlineOnly, LPCTSTR device = NULL) = 0;
	virtual bool			GetStatusInfo(LPSTR* json, void(**free)(LPSTR)) = 0;
	virtual void			Uninitialize() = 0;
	virtual void			Destroy() = 0;
};

ISourceFactory*				GetSourceFactory();
