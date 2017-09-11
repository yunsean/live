#pragma once
#include <vector>
#include <algorithm>
#include <stack>
#include <queue>
#include <functional>
#include <condition_variable>
#include <mutex>
#include "os.h"
#include "SmartArr.h"
#include "SmartHdr.h"

#ifndef INFINITE
#define INFINITE	(-1)
#endif
#ifndef IGNORE
#define IGNORE		0
#endif

template <typename T> class CShareBlkHeap;
template <typename T> class CShareBlkStack;
class CSharedBlock {
public:
	CSharedBlock();
	virtual ~CSharedBlock(){}
public:
	long			SetRef(const long count);
	long			AddRef();
	long			GetRef(){return m_lRefCount;}
	long			Release();
	void			Reset();

protected:
	void			SetCB(const std::function<void (CSharedBlock*)>& cb, std::mutex* syn);
protected:
	template <typename T> friend class CShareBlkHeap;
	volatile long						m_lRefCount;
	std::function<void (CSharedBlock*)>	m_fnRelease;
	std::mutex*							m_synchro;
	bool								m_bReleased;
};

template <typename T>
class CShareBlkAuto {
public:
	CShareBlkAuto(T* lp = NULL) : m_lpBlock(lp){}
	virtual ~CShareBlkAuto(){if (m_lpBlock)m_lpBlock->Release();}
public:
	T*& operator&() throw(){return m_lpBlock;}
	T*	operator->() throw(){return m_lpBlock;}
	T*	Detach(){T* lp(m_lpBlock); m_lpBlock = NULL; return lp;}
    T*  GetBlk(){return m_lpBlock;}
private:
	template <typename T1> friend class CShareBlkStack;
	T*				m_lpBlock;
};

template <typename T>
class CShareBlkHeap {
public:
	CShareBlkHeap();
	virtual ~CShareBlkHeap();

public:
	bool			Initialize(const long lCopyCount, const int nBlockCount = 10);
	bool			Initialize(const long lCopyCount, const size_t nLowerLimit, const size_t nUpperLimit, const size_t nShrinkThreshold = 0);
	bool			SetCopyCount(const long lCopyCount);
	bool			GetEmptyBlock(T*& lpBlock, const int nWait = INFINITE);
	size_t			GetBlockCount() const{return m_allBlocks.size();}
	size_t			GetEmptyCount() const{return m_emptyBlocks.size();}
	void			Uninitialize(void);

protected:
	friend class CSharedBlock;
	void			ReturnEmptyBlock(T* lpBlock);
	void			ShrinkEmptyBlock();
protected:
	typedef std::vector<T*>		TVector;
	typedef std::queue<T*>		TQueue;
private:
	bool						m_initialized;
	long						m_copiedCount;
	size_t						m_upperLimit;
	size_t						m_lowerLimit;
	size_t						m_shrinkThre;
	TVector						m_allBlocks;
	TQueue						m_emptyBlocks;

	std::mutex					m_mutexBlock;
	std::mutex					m_mutexRelease;
	std::condition_variable		m_condEmpty;
};

template <typename T>
class CShareBlkStack {
public:
	CShareBlkStack();
	virtual ~CShareBlkStack();

public:
	void			AddValidBlock(T* lpBlock);
	bool			GetValidBlock(T*& lpBlock, const int nWait = IGNORE);
	bool			GetValidBlock(CShareBlkAuto<T>& sbaBlock, const int nWait = IGNORE);
	int				GetValidBlockCount();

	bool			IsEmpty(void);
	void			Reset(void);
	void			Finished(void);
	bool			IsFinished(void);

protected:
	typedef std::queue<T*>		TQueue;

private:
	TQueue						m_validBlocks;

	std::mutex					m_mutex;
	std::condition_variable		m_condValid;
	bool						m_finished;
};

//////////////////////////////////////////////////////////////////////////
template <typename T>
CShareBlkHeap<T>::CShareBlkHeap()
	: m_initialized(false)
	, m_copiedCount(0)
	, m_lowerLimit(-1)
	, m_shrinkThre(0)
	, m_allBlocks()
	, m_emptyBlocks()

	, m_mutexBlock()
	, m_mutexRelease()
	, m_condEmpty() {
}
template <typename T>
CShareBlkHeap<T>::~CShareBlkHeap() {
	Uninitialize();
}

template <typename T>
bool CShareBlkHeap<T>::Initialize(const long lCopyCount, const int nBlockCount /* = 10 */) {
	return Initialize(lCopyCount, nBlockCount, nBlockCount);
}
template <typename T>
bool CShareBlkHeap<T>::Initialize(const long lCopyCount, const size_t nLowerLimit, const size_t nUpperLimit, const size_t nShrinkThreshold /* = 0 */) {
	Uninitialize();
	std::unique_lock<std::mutex> lock(m_mutexBlock);
	if (lCopyCount < 1)return false;
	if (nLowerLimit < 2)return false;
	if (nUpperLimit < nLowerLimit)return false;
	m_copiedCount = lCopyCount;
	m_lowerLimit = nLowerLimit;
	m_upperLimit = nUpperLimit;
	if (nShrinkThreshold == 0)
		m_shrinkThre = m_lowerLimit;
	else
		m_shrinkThre = nShrinkThreshold;
	std::function<void (CSharedBlock*)>	fn(
		[this](CSharedBlock* block){
			this->ReturnEmptyBlock(static_cast<T*>(block));
	});
	for (size_t iBlock = 0; iBlock < m_lowerLimit; iBlock++) {
		T* lpBlock(new(std::nothrow) T);
		if (lpBlock == NULL)return false;
		lpBlock->SetCB(fn, &m_mutexRelease);	
		m_allBlocks.push_back(lpBlock);
		m_emptyBlocks.push(lpBlock);
	}
	m_initialized		= true;
	return true;
}

template <typename T>
bool CShareBlkHeap<T>::SetCopyCount(const long lCopyCount) {
	if (lCopyCount < 1)return false;	
	std::unique_lock<std::mutex> lock(m_mutexBlock);
	m_copiedCount = lCopyCount;
	return true;
}
template <typename T>
bool CShareBlkHeap<T>::GetEmptyBlock(T*& lpBlock, const int nWait /* = INFINITE */) {
	std::unique_lock<std::mutex> lock(m_mutexBlock);
	if (!m_initialized)return false;
	if(!m_emptyBlocks.empty()) {
		lpBlock = m_emptyBlocks.front();
		m_emptyBlocks.pop();
		lock.unlock();
		lpBlock->SetRef(m_copiedCount);
		return true;
	} else if (m_allBlocks.size() < m_upperLimit) {
		lpBlock = new(std::nothrow) T;
		if (lpBlock != NULL) {
			lpBlock->SetCB(
				[this](CSharedBlock* block){
					this->ReturnEmptyBlock(static_cast<T*>(block));
			}, &m_mutexRelease);	
			m_allBlocks.push_back(lpBlock);
			lock.unlock();
			lpBlock->SetRef(m_copiedCount);
			return true;
		}
	} 
	if (nWait == INFINITE) {
		while (m_emptyBlocks.empty() && m_initialized) {
			m_condEmpty.wait(lock);
		}
		if (m_emptyBlocks.empty()) {
			return false;
		} else {
			lpBlock = m_emptyBlocks.front();
			m_emptyBlocks.pop();
			lock.unlock();
			lpBlock->SetRef(m_copiedCount);
			return true;
		}
	} else if(nWait == IGNORE) {
		return false;
	} else {
		std::chrono::system_clock::duration duration = std::chrono::milliseconds(nWait);
		std::chrono::time_point<std::chrono::system_clock>	over(std::chrono::system_clock::now());
		over += duration;
		while (m_emptyBlocks.empty() && m_initialized) {
			std::chrono::time_point<std::chrono::system_clock>	now(std::chrono::system_clock::now());
			if (now >= over)break;
			if (m_condEmpty.wait_until(lock, over) == std::cv_status::timeout)break;
		}
		if (m_emptyBlocks.empty()) {
			return false;
		} else {
			lpBlock = m_emptyBlocks.front();
			m_emptyBlocks.pop();
			lock.unlock();
			lpBlock->SetRef(m_copiedCount);
			return true;
		}
	}
}
template <typename T>
void CShareBlkHeap<T>::ReturnEmptyBlock(T* lpBlock) {
	std::unique_lock<std::mutex> lock(m_mutexBlock);
	if (!m_initialized)return;
	m_emptyBlocks.push(lpBlock);
	if (m_emptyBlocks.size() > m_shrinkThre)ShrinkEmptyBlock();
	m_condEmpty.notify_one();
}
template <typename T>
void CShareBlkHeap<T>::ShrinkEmptyBlock() {
	if (!m_initialized)return;
	while (m_emptyBlocks.size() > m_lowerLimit) {
		T* lpBlock(m_emptyBlocks.front());
		auto found(std::find(m_allBlocks.begin(), m_allBlocks.end(), lpBlock));
		if (found == m_allBlocks.end())break;
		m_allBlocks.erase(found);
		m_emptyBlocks.pop();
		delete lpBlock;
	}
}
template <typename T>
void CShareBlkHeap<T>::Uninitialize() {
	std::unique_lock<std::mutex> lock(m_mutexBlock);
	m_initialized = false;
	m_condEmpty.notify_all();
	while(!m_emptyBlocks.empty())m_emptyBlocks.pop();
	for (T* item : m_allBlocks) {
		delete item;
	}
	m_allBlocks.clear();
}

//////////////////////////////////////////////////////////////////////////
template <typename T>
CShareBlkStack<T>::CShareBlkStack()
	: m_validBlocks()
	, m_mutex()
	, m_condValid()
	, m_finished(false){
}
template <typename T>
CShareBlkStack<T>::~CShareBlkStack() {
	Reset();
}

template <typename T>
void CShareBlkStack<T>::AddValidBlock(T* lpBlock) {
	std::unique_lock<std::mutex> lock(m_mutex);
	m_validBlocks.push(lpBlock);
	m_condValid.notify_one();
}
template <typename T>
bool CShareBlkStack<T>::GetValidBlock(T*& lpBlock, const int nWait /* = IGNORE */) {
	std::unique_lock<std::mutex> lock(m_mutex);
	if(!m_validBlocks.empty()) {
		lpBlock = m_validBlocks.front();
		m_validBlocks.pop();
		return true;
	} else if (nWait == IGNORE) {
		return false;
	} else if(nWait == INFINITE) {
		while (m_validBlocks.empty() && !m_finished) {
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
		while (m_validBlocks.empty() && !m_finished) {
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
bool CShareBlkStack<T>::GetValidBlock(CShareBlkAuto<T>& sbaBlock, const int nWait /* = IGNORE */ ) {
	T* lpBlock(NULL);
	if (sbaBlock.m_lpBlock)sbaBlock.m_lpBlock->Release();
	sbaBlock.m_lpBlock	= NULL;
	if (!GetValidBlock(lpBlock, nWait))return false;
	sbaBlock.m_lpBlock	= lpBlock;
	return true;
}
template <typename T>
int CShareBlkStack<T>::GetValidBlockCount() {
	std::unique_lock<std::mutex> lock(m_mutex);
	return (int)m_validBlocks.size();
}
template <typename T>
bool CShareBlkStack<T>::IsEmpty(void) {
	std::unique_lock<std::mutex> lock(m_mutex);
	return m_validBlocks.empty();
}
template <typename T>
void CShareBlkStack<T>::Reset() {
	std::unique_lock<std::mutex> lock(m_mutex);
	while(!m_validBlocks.empty()) {
		T* lpBlock(m_validBlocks.front());
		if (lpBlock)lpBlock->Release();
		m_validBlocks.pop();
	}
}
template <typename T>
void CShareBlkStack<T>::Finished(void) {
	std::unique_lock<std::mutex> lock(m_mutex);
	m_finished = true;
	m_condValid.notify_all();
}
template <typename T>
bool CShareBlkStack<T>::IsFinished(void) {
	std::unique_lock<std::mutex> lock(m_mutex);
	if (m_validBlocks.size() > 0)return false;
	return m_finished;
}

