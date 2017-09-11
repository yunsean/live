#include "ShareBlk.h"

/************************************************************************
Ϊ������Ч�ʣ��������Գ�Ա�����������м���
************************************************************************/
CSharedBlock::CSharedBlock()
	: m_lRefCount(0)
	, m_fnRelease(nullptr)
	, m_synchro(nullptr)
	, m_bReleased(true) {
}
long CSharedBlock::SetRef(const long count) {
	std::unique_lock<std::mutex> lock(*m_synchro);
	m_lRefCount = count;
	m_bReleased = false;
	return m_lRefCount;
}
long CSharedBlock::AddRef() {
	std::unique_lock<std::mutex> lock(*m_synchro);
	m_lRefCount++;
	return m_lRefCount;
}
long CSharedBlock::Release() {
	std::unique_lock<std::mutex> lock(*m_synchro);
	m_lRefCount--;
	if (m_lRefCount <= 0) {
		if (!m_bReleased)m_fnRelease(this);
		m_bReleased	= true;
		return 0;
	}
	return m_lRefCount;
}
void CSharedBlock::Reset() {
	std::unique_lock<std::mutex> lock(*m_synchro);
	m_lRefCount = 0;
	if (!m_bReleased)m_fnRelease(this);
	m_bReleased	= true;
}
void CSharedBlock::SetCB(const std::function<void (CSharedBlock*)>& cb, std::mutex* syn) {
	m_fnRelease			= cb;
	m_synchro			= syn;
}

