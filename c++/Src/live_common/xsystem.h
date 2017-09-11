#pragma once
#include <algorithm>
#include <string>
#include <iostream> 
#include <iomanip>
#include <iosfwd>
#include <sstream>
#ifdef _WIN32
#include <wtypes.h>
#include <WinBase.h>
#include <iomanip>
#include <synchapi.h>
#include <processthreadsapi.h>
#include <libloaderapi.h>
#include <errhandlingapi.h>
#include <sysinfoapi.h>
#include <sys/timeb.h>
#else 
#include <unistd.h>
#include <sys/syscall.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#endif
#include "xchar.h"

namespace std {

	inline bool isLittleEndian() {
		static int little_endian_check = -1;
		if (little_endian_check == -1) {
			union {
				int32_t i;
				int8_t c;
			} little_check_union;
			little_check_union.i = 0x01;
			little_endian_check = little_check_union.c;
		}
		return (little_endian_check == 1);
	}

	inline static bool sleep(int millisecond) {
#ifdef _WIN32
		::WaitForSingleObject(::GetCurrentThread(), millisecond);
		return true;
#else 
		int res(usleep(millisecond * 1000));
		if (res == 0)return true;
		else return false;
#endif
	}

	inline static unsigned int tid() {
#ifdef _WIN32
		return ::GetCurrentThreadId();
#else 
		return syscall(__NR_gettid);
#endif
	}

	inline static unsigned int pid() {
#ifdef _WIN32
		return ::GetCurrentProcessId();
#else 
		return getpid();
#endif
	}

	inline static bool exefile(TCHAR name[1024]){
#ifdef _WIN32
		::GetModuleFileName(NULL, name, 1024);
		return true;
#else 
		size_t				linksize = 1024;
		memset(name, 0, 1024);
		if(readlink("/proc/self/exe", name, linksize) ==-1)return false;
		return true;
#endif
	}

	inline static unsigned int lastError() {
#ifdef _WIN32
		return ::GetLastError();
#else 
		return (unsigned int)errno;
#endif
	}

    inline static std::string errorText(int err) {
#ifdef _WIN32
        LPVOID			lpMsgBuf(NULL);
        FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (char*) &lpMsgBuf, 0, NULL );
        std::string     desc((char*)lpMsgBuf);
        LocalFree(lpMsgBuf);
        return desc;
#else 
        char            desc[1024] = {0};
        if (strerror_r(err, desc, 1024) == 0)return "";
        return desc;
#endif
    }    

	inline static unsigned int tickCount() {
#ifdef _WIN32
		return ::GetTickCount();
#else 
		struct timespec		ts;
		clock_gettime(CLOCK_MONOTONIC, &ts); 
		return (unsigned int)(ts.tv_sec * 1000 + ts.tv_nsec / (1000 * 1000));
#endif
	}

	inline static unsigned long long utc() {
#ifdef _WIN32
		SYSTEMTIME st = {};
		FILETIME ft = {};
		GetSystemTime(&st);
		SystemTimeToFileTime(&st, &ft);
		return ((LARGE_INTEGER*)&ft)->QuadPart;
#else 
		struct timespec ts = {0, 0};
		clock_gettime(CLOCK_REALTIME, &ts);
		return (unsigned long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000;
#endif
	}

    inline static unsigned int cpuCount() {
#ifdef _WIN32 
        SYSTEM_INFO         info;  
        GetSystemInfo(&info);  
        return info.dwNumberOfProcessors;
#else 
        return sysconf(_SC_NPROCESSORS_ONLN);
#endif
    }

#ifdef _WIN32
    static LONG                 initializeLock_gettimeofday(0);
    inline int gettimeofday(struct timeval* tp, int* tz) {
        static LARGE_INTEGER    tickFrequency;
        static LARGE_INTEGER    epochOffset;
        static bool             isInitialized(false);
        LARGE_INTEGER           tickNow;
        QueryPerformanceCounter(&tickNow);
        if (!isInitialized) {
            if(1 == InterlockedIncrement(&initializeLock_gettimeofday)) {
                struct timeb            tb;
                ftime(&tb);
                tp->tv_sec              = (long)tb.time;
                tp->tv_usec             = 1000 * tb.millitm;
                QueryPerformanceFrequency(&tickFrequency); 
                epochOffset.QuadPart    = tp->tv_sec * tickFrequency.QuadPart + (tp->tv_usec * tickFrequency.QuadPart) / 1000000L - tickNow.QuadPart;
                isInitialized           = true; 
                return 0;
            } else {
                InterlockedDecrement(&initializeLock_gettimeofday);
                while(!isInitialized){
                    Sleep(1);
                }
            }
        }
        tickNow.QuadPart        += epochOffset.QuadPart;
        tp->tv_sec              = (long)(tickNow.QuadPart / tickFrequency.QuadPart);
        tp->tv_usec             = (long)(((tickNow.QuadPart % tickFrequency.QuadPart) * 1000000L) / tickFrequency.QuadPart);
        return 0;
    }
#else
    inline int gettimeofday(struct timeval* tv, struct timezone* tz) {
        return ::gettimeofday(tv, tz);
    }
#endif

    template <typename _Ty>
    inline static std::string ntos(_Ty num, int base = 10, int width = 0, int precision = 0) {
        std::ostringstream  oss;
        oss << setw(width) << setbase(base) << setprecision(precision) << num;
        return oss.str();
    }

    template <typename _Ty>
    inline static std::wstring ntows(_Ty num, int base = 10, int width = 0, int precision = 0) {
        std::wostringstream  oss;
        oss << setw(width) << setbase(base) << setprecision(precision) << num;
        return oss.str();
    }

    template<class _Ty, typename _Ch> 
    _Ty tston(const std::basic_string<_Ch>& str, int base = 10) {
        _Ty value;
        std::basic_istringstream<_Ch> iss(str);
        iss >> setbase(base) >> value;
        return value;
    }
    template<bool, typename _Ch>
    bool tston(const std::basic_string<_Ch>& str) {
        bool value;
        std::basic_istringstream<_Ch> iss(str);
        iss >> boolalpha >> value;
        return value;
    }

	template <class _Ty>
	inline static const _Ty& xmin(const _Ty& x, const _Ty& y) {
#ifdef _WIN32
		return min(x, y);
#else 
		return std::min(x, y);
#endif
	}

	template <class _Ty>
	inline static const _Ty& xmax(const _Ty& x, const _Ty& y) {
#ifdef _WIN32
		return max(x, y);
#else 
		return std::max(x, y);
#endif
	}
};

#if defined(UNICODE) || defined(_UNICODE)
#define ntots   ntows 
#else 
#define ntots   ntos
#endif