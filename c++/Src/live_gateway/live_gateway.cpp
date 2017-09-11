// live_gateway.cpp : 定义控制台应用程序的入口点。
//
#include "EventBase.h"
#include "SystemInfo.h"
#include "Profile.h"
#include "ConsoleUI.h"
#include "SinkFactory.h"
#include "SourceFactory.h"
#include "CodecFactory.h"
#include "ChannelFactory.h"
#include "GatewayServer.h"
#include "net.h"

void WillExit(void* lpUserdata) {
	*(bool*)lpUserdata = true;
	CConsoleUI::Singleton().PermitExit();
	CConsoleUI::Singleton().ExitProcess();
	CSourceFactory::singleton().uninitialize();	
	CEventBase::singleton().shutdown();
	CEventBase::singleton().shutdown();
	CGatewayServer::singleton().shutdown();
}

int main() {
#ifdef _WIN32
	::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	init_sockets();
#endif
	CProfile profile(CProfile::Default());
	bool bToQuit(false);
	CConsoleUI::Singleton().Initialize(true, true, _T("%H:%M:%S "));
	CConsoleUI::Singleton().SetWillExit(WillExit, &bToQuit);
	CConsoleUI::Singleton().SetTitle(_T("数据转发网关 - 总连接:0, 正在播放:0"));

	if (!CEventBase::singleton().initialize()) {
		cpe(_T("初始化网络核心引擎失败!"));
		return 0;
	}

	if (!CSystemInfo::singleton().initialize()) {
		cpe(_T("初始化状态监视器失败！!"));
		return 0;
	}
	int httpPort(profile(_T("Http"), _T("Port"), _T("89")));
	CConsoleUI::Singleton().Print(CConsoleUI::LightPurple, true, _T("初始化HTTP服务框架..."));
	if (!CGatewayServer::singleton().startup(httpPort)) {
		cpe(_T("初始化HTTP服务框架失败，按任意键退出服务！"));
		return 0;
	}
	CConsoleUI::Singleton().Print(CConsoleUI::White, false, _T("\t\t在端口[%d]创建侦听成功！"), httpPort);

	CConsoleUI::Singleton().Print(CConsoleUI::LightPurple, true, _T("初始化设备..."));
	if (!CSourceFactory::singleton().initialize()) {
		cpe(_T("初始化设备库失败，按任意键退出服务!"));
		return 0;
	}
	if (!CSinkFactory::singleton().initialize()) {
		cpe(_T("初始化输出库失败，按任意键退出服务!"));
		return 0;
	}
	if (!CCodecFactory::singleton().initialize()) {
		cpe(_T("初始化解码库失败，按任意键退出服务!"));
		return 0;
	}
	if (!CChannelFactory::singleton().initialize()) {
		cpe(_T("初始化转发引擎失败，按任意键退出服务!"));
		return 0;
	}

	if (!CEventBase::singleton().startup()) {
		cpe(_T("启动网络核心引擎失败！"));
		return 0;
	}
	CConsoleUI::Singleton().Print(CConsoleUI::Green, true, _T("启动网络核心引擎..."));
	//CConsoleUI::Singleton().EnableClose(false);
	CConsoleUI::Singleton().Print(CConsoleUI::BrightWhite, true, _T("启动完成"));
	CConsoleUI::Singleton().BlockExit(true);
	WillExit(&bToQuit);

    return 0;
}

