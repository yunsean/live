#pragma once
#include <mutex>
#include <atomic>
#include <string>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <chrono>
#ifdef _WIN32
#else 
#include <unistd.h> 
#endif
#include "json/writer.h"
#include "json/value.h"
#include "xstring.h"
#include "SmartHdr.h"

class CSystemInfo {
public:
	CSystemInfo();
	~CSystemInfo();

public:
	static CSystemInfo& singleton();

public:
	bool initialize();
	std::string status(bool detail);

public:
	static void SetThreadCount(const unsigned int count);
	static void WorkThreadExit();

#ifdef _WIN32
protected:
	uint64_t fileTime2Utc(const FILETIME* ftime);
#else 
#define VMRSS_LINE		15
#define PROCESS_ITEM	14
	int get_phy_mem(const pid_t p);
	int get_total_mem();
	unsigned int get_cpu_total_occupy();
	unsigned int get_cpu_process_occupy(const pid_t p);
	const char* get_items(const char* buffer, int ie);
#endif

protected:
	std::string formatSize(uint64_t size);
	int cpuUsage();
	int64_t memoryUsage();
	int handleCount();

protected:
	static void timer_callback(evutil_socket_t fd, short what, void* context);
	void timer_callback(evutil_socket_t fd, short what);

private:
	static std::atomic<unsigned int> m_lThreadCount;

private:
	time_t m_startupTime;
	std::string m_startupAt;
	std::mutex m_mutex;
	unsigned int m_detailInfoAt;
	Json::Value m_sourceInfo;
	Json::Value m_sinkInfo;
	Json::Value m_channelInfo;
	event* m_timer;

	int m_cpuUsage;

#ifdef _WIN32
	CSmartHdr<HANDLE, BOOL> m_shProcess;
	int64_t m_latestTime;
	int64_t m_latestSystemTime;
#else 
	pid_t m_pid;
	unsigned int m_latestTotalTime;
	unsigned int m_latestProcesstime;
#endif
	int m_nCpuCount;
};

