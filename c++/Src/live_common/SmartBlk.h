#pragma once
#include <vector>
#include <queue>
#include <algorithm>
#include <condition_variable>
#include <mutex>
#include "os.h"
#include "xevent.h"
#include "SmartArr.h"
#include "SmartHdr.h"

#ifndef INFINITE
#define INFINITE	(-1)
#endif
#ifndef IGNORE
#define IGNORE		0
#endif

class CTypicalBlock {
public:
	CTypicalBlock();
	virtual ~CTypicalBlock(){}

public:
	char*	EnsureBlock(const int nSize, const bool bRemain = false) throw();
	void	SetSize(const int nSize) throw();
	void	FillData(const void* const lpData, const int nSize) throw();
	void	AppendData(const void* const lpData, const int nSize) throw();
	int		FillDatas(const void* const* const lpDataPointers, const int* const nDataSizes, const int nDataCount);
	int		GrowSize(int szGrow);
	int		GetSize(){return szData;}
	void*	GetData(){return saCache;}
	int		GetData(void* const lpCache, const int nMaxSize);
	void	Clear();

protected:
	CSmartArr<char>	saCache;
	int				szCache;
	int				szData;
};

template <typename T>
class CSmartBlk {
public:
	CSmartBlk();
	virtual ~CSmartBlk();

public:
	bool			Initialize(const int nBlockCount = 10);
	bool			GetEmptyBlock(T*& lpBlock, const int nWait = INFINITE);
	bool			GetValidBlock(T*& lpBlock, const int nWait = INFINITE);
	bool			PeekEmptyBlock(T*& lpBlock, const int nWait = INFINITE);
	bool			PeekValidBlock(T*& lpBlock, const int nWait = INFINITE);
	void			ReturnEmptyBlock(T* lpBlock);
	void			ReturnValidBlock(T* lpBlock);

	int				GetAllBlockCount();
	int				GetValidBlockCount();
	int				GetEmptyBlockCount();
	bool			IsEmpty(void);
	void			Reset(void);
	void			Finished(void);
	bool			IsFinished(void);

	void			Uninitialize(void);

protected:
	typedef std::vector<T*>		TVector;
	typedef std::queue<T*>		TQueue;

private:
	bool						m_initialized;
	TVector						m_allBlocks;
	TQueue						m_emptyBlocks;
	TQueue						m_validBlocks;

	bool						m_finished;
	std::mutex					m_mutex;
	std::condition_variable		m_condEmpty;
	std::condition_variable		m_condValid;
};

//////////////////////////////////////////////////////////////////////////
template <typename T>
CSmartBlk<T>::CSmartBlk()
	: m_initialized(false)
	, m_allBlocks()
	, m_emptyBlocks()
	, m_validBlocks()
	
	, m_finished(false)
	, m_mutex()
	, m_condEmpty()
	, m_condValid() {
}
template <typename T>
CSmartBlk<T>::~CSmartBlk() {
	Uninitialize();
}

template <typename T>
bool CSmartBlk<T>::Initialize(const int nBlockCount /* = 10 */) {
	Uninitialize();	
	std::unique_lock<std::mutex>	lock(m_mutex);
	for (int iBlock = 0; iBlock < nBlockCount; iBlock++) {
		T* lpBlock(new T);
		m_allBlocks.push_back(lpBlock);
		m_emptyBlocks.push(lpBlock);
	}
	m_finished = false;
	m_initialized = true;
	return true;
}
template <typename T>
void CSmartBlk<T>::Uninitialize() {
	std::unique_lock<std::mutex>	lock(m_mutex);
	m_initialized = false;
	m_condEmpty.notify_all();
	m_condValid.notify_all();
	while(!m_emptyBlocks.empty())m_emptyBlocks.pop();
	while(!m_validBlocks.empty())m_validBlocks.pop();
	for (T* item : m_allBlocks) {
		delete item;
	}
	m_allBlocks.clear();
}

template <typename T>
bool CSmartBlk<T>::GetEmptyBlock(T*& lpBlock, const int nWait /* = INFINITE */) {
	std::unique_lock<std::mutex> lock(m_mutex);
	if (!m_initialized)return false;
	if(!m_emptyBlocks.empty()) {
		lpBlock = m_emptyBlocks.front();
		m_emptyBlocks.pop();
		return true;
	} else if (m_finished) {
        return false;
    } else if(nWait == IGNORE) {
		return false;
	} else if(nWait == INFINITE) {
		while (m_emptyBlocks.empty() && m_initialized && !m_finished) {
			m_condEmpty.wait(lock);
		}
		if (m_emptyBlocks.empty()) {
			return false;
		} else {
			lpBlock = m_emptyBlocks.front();
			m_emptyBlocks.pop();
			return true;
		}
	} else {
		std::chrono::milliseconds wait(nWait);
		std::chrono::time_point<std::chrono::system_clock>	over(std::chrono::system_clock::now());
		over += wait;
		while (m_emptyBlocks.empty() && m_initialized && !m_finished) {
			std::chrono::time_point<std::chrono::system_clock>	now(std::chrono::system_clock::now());
			if (now >= over)break;
			if (m_condEmpty.wait_until(lock, over) == std::cv_status::timeout)break;
		}
		if (m_emptyBlocks.empty()) {
			return false;
		} else {
			lpBlock = m_emptyBlocks.front();
			m_emptyBlocks.pop();
			return true;
		}
	}
}
template <typename T>
bool CSmartBlk<T>::GetValidBlock(T*& lpBlock, const int nWait /* = INFINITE */) {
	std::unique_lock<std::mutex>	lock(m_mutex);
	if (!m_initialized)return false;
	if(!m_validBlocks.empty()) {
		lpBlock = m_validBlocks.front();
		m_validBlocks.pop();
		return true;
	} else if (m_finished) {
        return false;
    } else if(nWait == IGNORE) {
		return false;
	} else if(nWait == INFINITE) {
		while (m_validBlocks.empty() && m_initialized && !m_finished) {
			m_condValid.wait(lock);
		}
		if (m_validBlocks.empty()) {
			return false;
		} else {
			lpBlock = m_validBlocks.front();
			m_validBlocks.pop();
			return true;
		}
	} else {
		std::chrono::milliseconds wait(nWait);
		std::chrono::time_point<std::chrono::system_clock>	over(std::chrono::system_clock::now());
		over += wait;
		while (m_emptyBlocks.empty() && m_initialized && !m_finished) {
			std::chrono::time_point<std::chrono::system_clock>	now(std::chrono::system_clock::now());
			if (now >= over)break;
			if (m_condValid.wait_until(lock, over) == std::cv_status::timeout)break;
		}
		if (m_validBlocks.empty()) {
			return false;
		} else {
			lpBlock = m_validBlocks.front();
			m_validBlocks.pop();
			return true;
		}
	}
}
template <typename T>
bool CSmartBlk<T>::PeekEmptyBlock(T*& lpBlock, const int nWait /* = INFINITE */) {
	std::unique_lock<std::mutex>	lock(m_mutex);
	if (!m_initialized)return false;
	if(!m_emptyBlocks.empty()) {
		lpBlock = m_emptyBlocks.front();
		return true;
	} else if (m_finished) {
        return false;
    } else if(nWait == IGNORE) {
		return false;
	} else if(nWait == INFINITE) {
		while (m_emptyBlocks.empty() && m_initialized && !m_finished) {
			m_condEmpty.wait(lock);
		}
		if (m_emptyBlocks.empty()) {
			return false;
		} else {
			lpBlock = m_emptyBlocks.front();
			return true;
		}
	} else {
		std::chrono::milliseconds wait(nWait);
		std::chrono::time_point<std::chrono::system_clock>	over(std::chrono::system_clock::now());
		over += wait;
		while (m_emptyBlocks.empty() && m_initialized && !m_finished) {
			std::chrono::time_point<std::chrono::system_clock>	now(std::chrono::system_clock::now());
			if (now >= over)break;
			if (m_condEmpty.wait_until(lock, over) == std::cv_status::timeout)break;
		}
		if (m_emptyBlocks.empty()) {
			return false;
		} else {
			lpBlock = m_emptyBlocks.front();
			return true;
		}
	}
}
template <typename T>
bool CSmartBlk<T>::PeekValidBlock(T*& lpBlock, const int nWait /* = INFINITE */) {
	std::unique_lock<std::mutex>	lock(m_mutex);
	if (!m_initialized)return false;

	if(!m_validBlocks.empty()) {
		lpBlock = m_validBlocks.front();
		return true;
	} else if (m_finished) {
        return false;
    } else if(nWait == IGNORE) {
		return false;
	} else if (nWait == INFINITE) {
		while (m_validBlocks.empty() && m_initialized && !m_finished) {
			m_condValid.wait(lock);
		}
		if (m_validBlocks.empty()) {
			return false;
		} else {
			lpBlock  = m_validBlocks.front();
			return true;
		}
	} else {
		std::chrono::milliseconds wait(nWait);
		std::chrono::time_point<std::chrono::system_clock>	over(std::chrono::system_clock::now());
		over += wait;
		while (m_emptyBlocks.empty() && m_initialized && !m_finished) {
			std::chrono::time_point<std::chrono::system_clock>	now(std::chrono::system_clock::now());
			if (now >= over)break;
			if (m_condValid.wait_until(lock, over) == std::cv_status::timeout)break;
		}
		if (m_validBlocks.empty()) {
			return false;
		} else {
			lpBlock = m_validBlocks.front();
			return true;
		}
	}
}
template <typename T>
void CSmartBlk<T>::ReturnEmptyBlock(T* lpBlock) {
	std::unique_lock<std::mutex>	lock(m_mutex);
	if (!m_initialized)return;
	m_emptyBlocks.push(lpBlock);
	m_condEmpty.notify_one();
}
template <typename T>
void CSmartBlk<T>::ReturnValidBlock(T* lpBlock) {
	std::unique_lock<std::mutex>	lock(m_mutex);
	if (!m_initialized)return;
	m_validBlocks.push(lpBlock);
	m_condValid.notify_one();
}
template <typename T>
void CSmartBlk<T>::Reset() {
	std::unique_lock<std::mutex>	lock(m_mutex);
	if (!m_initialized)return;
	while (!m_emptyBlocks.empty())m_emptyBlocks.pop();
	while (!m_validBlocks.empty())m_validBlocks.pop();
	for (T* item : m_allBlocks) {
		m_emptyBlocks.push(item);
	}
	m_finished = false;
}
template <typename T>
void CSmartBlk<T>::Finished(void) {
	std::unique_lock<std::mutex>	lock(m_mutex);
	m_finished = true;
	m_condEmpty.notify_all();
	m_condValid.notify_all();
}
template <typename T>
bool CSmartBlk<T>::IsFinished(void) {
	std::unique_lock<std::mutex>	lock(m_mutex);
    if (!m_initialized)return true;
	if (m_validBlocks.size() > 0)return false;
	return m_finished;
}
template <typename T>
bool CSmartBlk<T>::IsEmpty(void) {
	std::unique_lock<std::mutex>	lock(m_mutex);
	return m_validBlocks.empty();
}
template <typename T>
int CSmartBlk<T>::GetAllBlockCount() {
	return m_allBlocks.size();
}
template <typename T>
int CSmartBlk<T>::GetValidBlockCount() {
	std::unique_lock<std::mutex>	lock(m_mutex);
	return m_validBlocks.size();
}
template <typename T>
int CSmartBlk<T>::GetEmptyBlockCount() {
	std::unique_lock<std::mutex>	lock(m_mutex);
	return m_emptyBlocks.size();
}
