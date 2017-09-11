#pragma once 
#include <mutex>
#ifdef _WIN32
#include <synchapi.h>
#include <processthreadsapi.h>
#endif
#include "os.h"

namespace std {

	class xcritic {

	public:
		void lock() {
#ifdef _WIN32 
			if (::GetCurrentThreadId() == holderTid){
				holderCount++;
				return;
			}
			::EnterCriticalSection(&critic);
			holderTid		= ::GetCurrentThreadId();
			holderCount++;
#else 
			mutex.lock();
#endif
		}
		void unlock() {
#ifdef _WIN32
			if (::GetCurrentThreadId() != holderTid){
				return;
			}
			holderCount--;
			if (holderCount == 0) {
				holderTid	= 0;
				::LeaveCriticalSection(&critic);
			}
#else 
			mutex.unlock();
#endif
		}
		bool trylock() {
#ifdef _WIN32
			if (::GetCurrentThreadId() == holderTid){
				holderCount++;
				return true;
			}
			bool			res(::TryEnterCriticalSection(&critic) ? true : false);
			if (!res)return false;
			::EnterCriticalSection(&critic);
			holderTid		= ::GetCurrentThreadId();
			holderCount++;
			return true;
#else 
			return mutex.try_lock();
#endif
		}

	public:
		xcritic() 
#ifdef _WIN32
			: holderTid(0)
			, holderCount(0) {
				::InitializeCriticalSection(&critic);
#else 
			: mutex(){ 
#endif
		}
		~xcritic() {
#ifdef _WIN32
			::DeleteCriticalSection(&critic);
#endif
		}

    private:
#ifdef _WIN32
		CRITICAL_SECTION		critic;
		unsigned int			holderTid;
		unsigned int			holderCount;
#else 
		std::recursive_mutex	mutex;
#endif
	};
};