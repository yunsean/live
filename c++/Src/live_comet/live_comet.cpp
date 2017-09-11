// live_comet.cpp : 定义控制台应用程序的入口点。
//
#include <fstream>
#include "net.h"
#include "Engine.h"
#include "ConsoleUI.h"
#include "Writelog.h"
#include "wingetopt.h"

void WillExit(void* lpUserdata) {
	*(bool*)lpUserdata = true;
	CEngine::singleton().shutdown();
	CConsoleUI::Singleton().PermitExit();
	CConsoleUI::Singleton().ExitProcess();
}

const static char* usage = "\nUseage:\n" \
"\t[comet -h] to check useage.\n" \
"\t[comet] to execute in terminal.\n" \
"\t[comet -d] to execute as a deamon.\n" \
"\t[comet -p num] to execute with the [num] port.\n" \
"\t[comet -s stop] to stop the deamon process.\n";

#ifndef _WIN32
void daemon();
void stop();
#endif

int main(int argc, char *argv[]) {
#ifdef _WIN32
	::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	init_sockets();
#endif

	int port = 88;
	char ch;
	while ((ch = getopt(argc, argv, "ds:p:h")) != EOF) {
		switch (ch) {
		case 'p':
			port = atoi(optarg);
			break;
		case 'h':
			printf("%s", usage);
			return 0;
#ifndef _WIN32
		case 'd':
			daemon();
			break;
		case 's':
			stop();
			return 0;
#endif
		default:
			break;
		}
	}

	bool bToQuit(false);
	CConsoleUI::Singleton().Initialize(true, true, w"%H:%M:%S ");
	CConsoleUI::Singleton().SetWillExit(WillExit, &bToQuit);
	CConsoleUI::Singleton().SetTitle(w"Message Gateway");

	CConsoleUI::Singleton().Print(CConsoleUI::White, true, w"Startup network engine at %d...", port);
	if (!CEngine::singleton().startup(port)) {
		cpe(_T("Startup network engine failed."));
		return 0;
	}

    return 0;
}

#ifndef _WIN32
void daemon() {
	switch (fork()) {
	case -1:
		return wle(_T("fork() failed."));
	case 0:
		break;
	default:
		exit(0);
	}
	
	std::ofstream pidf("/tmp/comet.pid");
	if (pidf.is_open()) {
		pidf << (long)getpid();
		pidf.close();
	}

	if (setsid() == -1) wlw(_T("setsid() failed"));
	umask(0);
	int fd = open("/dev/null", O_RDWR);
	if (fd == -1) wlw(_T("open(\"/dev/null\") failed"));
	else if (dup2(fd, STDIN_FILENO) == -1) wlw(_T("dup2(STDIN) failed"));
	else if (dup2(fd, STDOUT_FILENO) == -1) wlw(_T("dup2(STDOUT) failed"));
	else if (dup2(fd, STDERR_FILENO) == -1) wlw(_T("dup2(STDERR) failed"));
	if (fd > STDERR_FILENO && close(fd) == -1) wlw(_T("close() failed"));
}
void stop() {
	if (optarg != NULL && strcmp(optarg, "stop") == 0) {
		std::ifstream pidf("/tmp/comet.pid");
		long pid = 0;
		if (pidf.is_open()) {
			pidf >> pid;
			pidf.close();
		}
		if (pid != 0) {
			if (kill(pid, SIGTERM) == 0) cpw(w"Kill process %d succeed.", pid);
			else cpe(w"Kill process %d failed.", pid);
		}
	}
}
#endif

