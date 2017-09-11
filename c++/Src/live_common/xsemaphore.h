#pragma once 
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <chrono>

namespace std {
	class xsemaphore {
	public:
		xsemaphore()
			: m_count()
			, m_mutex()
			, m_cond(){
				m_count.store(0);
		}
		virtual ~xsemaphore() {
		}
	public:
		void notify() {
			std::lock_guard<std::mutex>		lock(m_mutex);
			m_count++;
			m_cond.notify_one();
		}
		bool wait(int timeout = -1) {
			std::unique_lock<std::mutex>	lock(m_mutex);
			if (timeout == -1){
				while (!m_count) {
					m_cond.wait(lock);
				}
			} else {
				std::chrono::system_clock::duration duration = std::chrono::milliseconds(timeout);
				std::chrono::time_point<std::chrono::system_clock>	until(std::chrono::system_clock::now());
				until += duration;
#ifdef _WIN32
				std::cv_status::cv_status status(std::cv_status::no_timeout);
#else 
				std::cv_status status(std::cv_status::no_timeout);
#endif
				while (!m_count && status == std::cv_status::no_timeout) {
					status = m_cond.wait_until(lock, until);
				}
				if (status != std::cv_status::no_timeout)return false;
			}
			m_count--;
			return true;
		}
	private:
		std::atomic_int m_count;
		std::mutex m_mutex;
		std::condition_variable m_cond;
	};
};
