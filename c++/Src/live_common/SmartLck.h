#pragma once
#include "os.h"
#include "xcritic.h"

template <class T>
class CSmartLock {
public:
	CSmartLock(T& lock)
		: m_pMutex(&lock) {
		m_pMutex->lock();
	}
	~CSmartLock(void) {
		if(m_pMutex) {
			m_pMutex->unlock();
		}
	}

	void Unlock(void) {
		if(m_pMutex) {
			m_pMutex->unlock();
			m_pMutex		= NULL;
		}
	}
private:
	T*			m_pMutex;
};

typedef CSmartLock<std::xcritic>	CSmartLck;


