#pragma once
#include <vector>
#include <algorithm>
#include <mutex>
#include <chrono>
#include <condition_variable>

namespace std{

	class xevent {
	public:
		xevent(bool manualReset = false, bool initialState = false) 
			: m_mutex()
			, m_cond()
			, m_manualReset(manualReset)
			, m_state(initialState) {
		}
		virtual ~xevent() {
		}

	public:
		void set() {
			std::unique_lock<std::mutex> lock(m_mutex);
			m_state = true;
			m_cond.notify_all();
		}
		void reset() {
			std::unique_lock<std::mutex> lock(m_mutex);
			m_state = false;
		}
		bool is() {
			return m_state;
		}
		bool wait(int timeout = -1) {
			std::unique_lock<std::mutex> lock(m_mutex);
			bool res(true);
			if (!m_state) {
				if (timeout == -1) {
					m_cond.wait(lock);
				} else {
					std::cv_status status(std::cv_status::timeout);
					status = m_cond.wait_for(lock, std::chrono::milliseconds(timeout));
					res = (status == std::cv_status::no_timeout);
				}
			}
			if (!m_manualReset)m_state = false;
			return res;
		}

	protected:
		std::mutex				m_mutex;
		std::condition_variable	m_cond;
		bool					m_manualReset;
		bool					m_state;
	};
};