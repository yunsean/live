#include <iomanip>
#include "EventBase.h"
#ifdef _WIN32
#include <wtypes.h>
#include <psapi.h>
#pragma comment(lib,"psapi.lib")
#else 
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#endif
#include "SystemInfo.h"
#include "ConsoleUI.h"
#include "Writelog.h"
#include "SourceFactory.h"
#include "SinkFactory.h"
#include "ChannelFactory.h"
#include "xsystem.h"

#define MIN_INTERVAL_UPDATE		(5 * 1000)

std::atomic<unsigned int> CSystemInfo::m_lThreadCount(0);
CSystemInfo::CSystemInfo()
	: m_startupTime(time(NULL))
	, m_startupAt()
	, m_mutex()
	, m_detailInfoAt(0)
	, m_sourceInfo()
	, m_sinkInfo()
	, m_channelInfo()
	, m_timer(nullptr)

#ifdef _WIN32
	, m_shProcess(::CloseHandle, INVALID_HANDLE_VALUE)
	, m_latestSystemTime(0)
	, m_latestTime(0)
#endif
	, m_nCpuCount(0){
	char tmp[64];
	strftime(tmp, sizeof(tmp), "%Y-%m-%d %X", localtime(&m_startupTime));
	m_startupAt = tmp;
}
CSystemInfo::~CSystemInfo() {
	if (m_timer != nullptr) {
		event_free(m_timer);
		m_timer = nullptr;
	}
}

CSystemInfo& CSystemInfo::singleton() {
	static CSystemInfo instance;
	return instance;
}

bool CSystemInfo::initialize() {
#ifdef _WIN32
	SYSTEM_INFO info;
	GetSystemInfo(&info);
	m_nCpuCount = (int)info.dwNumberOfProcessors;
	DuplicateHandle(GetCurrentProcess(), GetCurrentProcess(), GetCurrentProcess(), &m_shProcess, 0, FALSE, DUPLICATE_SAME_ACCESS);
	if (m_shProcess.isValid()) {
		FILETIME now, creation, exit, kernel, user;
		GetSystemTimeAsFileTime(&now);
		if (GetProcessTimes(m_shProcess, &creation, &exit, &kernel, &user)) {
			m_latestSystemTime = (fileTime2Utc(&kernel) + fileTime2Utc(&user)) / m_nCpuCount;
			m_latestTime = fileTime2Utc(&now);
		}
	}
#else 
	m_pid = getpid();
	m_latestTotalTime = get_cpu_total_occupy();
	m_latestProcesstime = get_cpu_process_occupy(m_pid);
#endif
	m_timer = evtimer_new(CEventBase::singleton().preferBase(), timer_callback, this);
	struct timeval tv = { 10, 0 };
	evtimer_add(m_timer, &tv);
	return true;
}
void CSystemInfo::timer_callback(evutil_socket_t fd, short what, void* context) {
	CSystemInfo* thiz(reinterpret_cast<CSystemInfo*>(context));
	thiz->timer_callback(fd, what);
}
void CSystemInfo::timer_callback(evutil_socket_t fd, short what) {
	m_cpuUsage = cpuUsage();
	static struct timeval tv = { 10, 0 };
	evtimer_add(m_timer, &tv);
}
std::string CSystemInfo::status(bool detail) {
	time_t now(time(NULL));
	unsigned int seconds(static_cast<int>(now - m_startupTime));
	int day = seconds / (24 * 3600);
	seconds %= (24 * 3600);
	int hour = seconds / 3600;
	seconds %= 3600;
	int minute = seconds / 60;

	if (detail && std::tickCount() - m_detailInfoAt > MIN_INTERVAL_UPDATE) {
		std::unique_lock<std::mutex> lock(m_mutex, std::try_to_lock);
		if (lock.owns_lock()) {
			m_sourceInfo.clear();
			m_sinkInfo.clear();
			m_channelInfo.clear();
			CSourceFactory::singleton().status(m_sourceInfo);
			CSinkFactory::singleton().status(m_sinkInfo);
			CChannelFactory::singleton().status(m_channelInfo);
			m_detailInfoAt = std::tickCount();
		}
	}

	Json::Value value;
	value["启动时间"] = m_startupAt;
	value["运行时间"] = xstring<char>::format("%d天%d小时%d分", day, hour, minute);
	std::lock_guard<std::mutex> lock(m_mutex);
	if (detail) {
		value["输入详情"] = m_sourceInfo;
		value["输出详情"] = m_sinkInfo;
		value["通道详情"] = m_channelInfo;
	}
	value["活跃数量"] = m_sourceInfo.size();
	value["线程数量"] = (unsigned int)m_lThreadCount;
	value["CPU消耗"] = xstring<char>::format("%d%s", m_cpuUsage, m_cpuUsage > 0 ? "%" : "");
	value["内存消耗"] = formatSize(memoryUsage());
	value["占用句柄"] = handleCount();
	return value.toStyledString();
}
std::string CSystemInfo::formatSize(uint64_t size) {
	if (size > 1024LL * 1024 * 1024 * 512) {
		return xstring<char>::format("%.2f TB", size / (1024 * 1024 * 1024 * 1024.f));
	} if (size > 1024 * 1024 * 512) {
		return xstring<char>::format("%.2f GB", size / (1024 * 1024 * 1024.f));
	}
	else if (size > 1024 * 512) {
		return xstring<char>::format("%.2f MB", size / (1024 * 1024.f));
	}
	else {
		return xstring<char>::format("%.2f KB", size / (1024.f));
	}
}
#ifdef _WIN32
uint64_t CSystemInfo::fileTime2Utc(const FILETIME* ftime) {
	LARGE_INTEGER li;
	li.LowPart = ftime->dwLowDateTime;
	li.HighPart = ftime->dwHighDateTime;
	return li.QuadPart;
}
int CSystemInfo::cpuUsage() {
	if (!m_shProcess.isValid()) return -1;
	FILETIME now, creation, exit, kernel, user; 
	GetSystemTimeAsFileTime(&now);
	if (!GetProcessTimes(m_shProcess, &creation, &exit, &kernel, &user)) {
		return -1;
	}
	int64_t systemTime((fileTime2Utc(&kernel) + fileTime2Utc(&user)) / m_nCpuCount);
	int64_t time(fileTime2Utc(&now));
	int64_t systemTimeDelta(systemTime - m_latestSystemTime);
	int64_t timeDelta(time - m_latestTime);
	if (timeDelta == 0) {
		return cpuUsage();
	}
	m_latestSystemTime = systemTime;
	m_latestTime = time;
	return (int)((systemTimeDelta * 100 + timeDelta / 2) / timeDelta);
}
int64_t CSystemInfo::memoryUsage() {
	PROCESS_MEMORY_COUNTERS pmc;
	GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
	return pmc.WorkingSetSize + pmc.PagefileUsage;
}
int CSystemInfo::handleCount() {
	DWORD count(0);
	GetProcessHandleCount(GetCurrentProcess(), &count);
	return count;
}
#else 
int CSystemInfo::get_phy_mem(const pid_t p) {
	char file[64] = { 0 };
	FILE* fd;
	char line_buff[1024] = { 0 };
	sprintf(file, "/proc/%d/status", p);
	fd = fopen(file, "r"); 
	if (fd == NULL) return 0;
	int i;
	char name[32];
	int vmrss;
	for (i = 0; i < VMRSS_LINE - 1; i++) {
		fgets(line_buff, sizeof(line_buff), fd);
	}
	fgets(line_buff, sizeof(line_buff), fd);
	sscanf(line_buff, "%s %d", name, &vmrss);
	fclose(fd);
	return vmrss;
}
int CSystemInfo::get_total_mem() {
	const char* file = "/proc/meminfo";
	FILE* fd;
	char line_buff[1024] = { 0 };                                                                                      
	fd = fopen(file, "r");
	if (fd == NULL) return 0;
	char name[32];
	int memtotal;
	fgets(line_buff, sizeof(line_buff), fd);
	sscanf(line_buff, "%s %d", name, &memtotal);
	fclose(fd); 
	return memtotal;
}
unsigned int CSystemInfo::get_cpu_process_occupy(const pid_t p) {
	char file[64] = { 0 };
	FILE* fd;
	char line_buff[1024] = { 0 };
	sprintf(file, "/proc/%d/stat", p);
	fd = fopen(file, "r");
	if (fd == NULL) return 0;
	fgets(line_buff, sizeof(line_buff), fd);
	pid_t pid;
	sscanf(line_buff, "%u", &pid);
	const char* q = get_items(line_buff, PROCESS_ITEM);
	unsigned int utime;
	unsigned int stime;
	unsigned int cutime;
	unsigned int cstime;
	sscanf(q, "%u %u %u %u", &utime, &stime, &cutime, &cstime);
	fclose(fd); 
	return (utime + stime + cutime + cstime);
}
unsigned int CSystemInfo::get_cpu_total_occupy() {
	FILE* fd;
	char buff[1024] = { 0 }; 
	fd = fopen("/proc/stat", "r");
	if (fd == NULL) return 0;
	fgets(buff, sizeof(buff), fd); 
	char name[16];
	unsigned int user;
	unsigned int nice;
	unsigned int system;
	unsigned int idle;
	sscanf(buff, "%s %u %u %u %u", name, &user, &nice, &system, &idle);
	fclose(fd);
	return (user + nice + system + idle);
}
const char* CSystemInfo::get_items(const char* buffer, int ie) {
	assert(buffer);
	const char* p = buffer;
	int len = strlen(buffer);
	int count = 0;
	if (1 == ie || ie < 1) {
		return p;
	}
	for (int i = 0; i < len; i++) {
		if (' ' == *p) {
			count++;
			if (count == ie - 1) {
				p++;
				break;
			}
		}
		p++;
	}
	return p;
}
int CSystemInfo::cpuUsage() {
	try {
		unsigned int totalcputime2 = get_cpu_total_occupy();
		unsigned int procputime2 = get_cpu_process_occupy(m_pid);
		int usage = 100 * (procputime2 - m_latestProcesstime) / (totalcputime2 - m_latestTotalTime);
		m_latestProcesstime = procputime2;
		m_latestTotalTime = totalcputime2;
		return usage;
	} catch (...) {
		return -1;
	}
}
int64_t CSystemInfo::memoryUsage() {
	try {
		return get_phy_mem(m_pid);
	} catch (...) {
		return 0;
	}
}
int CSystemInfo::handleCount() {
}
#endif

void CSystemInfo::SetThreadCount(const unsigned int count) {
	m_lThreadCount = count;
}
void CSystemInfo::WorkThreadExit() {
	--m_lThreadCount;
}