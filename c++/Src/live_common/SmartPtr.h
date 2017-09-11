#pragma once
#include <functional>
#include "os.h"

#ifndef ASSERT
#define ASSERT(x)	
#endif

template <typename T>
class CSmartPtr {
private:
	template <typename T1> 
	class CRefCount {
	public:
		CRefCount(T1* p = 0, const std::function<void (T* ptr)>& d = 0) 
			: m_ptr(0)
			, m_counter(0)
			, m_deleter1(d)
			, m_deleter2(nullptr)
			, m_usememfun(false){
			if(p != nullptr)
				m_ptr		= p;
			m_counter++;
		}
		CRefCount(T1* p, void (T1::*fnd)()) 
			: m_ptr(nullptr)
			, m_counter(0)
			, m_deleter1(nullptr)
			, m_deleter2(fnd)
			, m_usememfun(fnd != nullptr){
			if(p != nullptr)
				m_ptr		= p;
			m_counter++;
		}
		~CRefCount() { 
		}

		void IncRefCount(){
			m_counter++;
		}
		void DecRefCount(){
			m_counter--;
			if(m_counter <= 0){
				if (m_deleter1 && m_ptr)
					m_deleter1(m_ptr);
				else if (m_usememfun && m_ptr)
					m_deleter2(*m_ptr);
				else if (m_ptr)
					delete m_ptr;
				m_ptr		= 0;
				delete this;
			}
		}
		T1*& GetPtr(){
			return m_ptr;
		}

	private:
		T1*							m_ptr;
		int							m_counter;	
		std::function<void(T1* ptr)>m_deleter1;
		std::mem_fun_ref_t<void, T> m_deleter2;
		bool						m_usememfun;
	};
	CRefCount<T>*					m_refcount;

public:
	void Assign(CRefCount<T>* refcount){
		if(refcount != 0)refcount->IncRefCount();
		CRefCount<T>*		oldref(m_refcount);
		m_refcount			= refcount;
		if(oldref != 0)oldref->DecRefCount();
	}

public:
	CSmartPtr() 
		: m_refcount(0){
	}
	CSmartPtr(T* ptr) 
		: m_refcount(0){
		if (ptr)m_refcount = new(std::nothrow) CRefCount<T>(ptr, 0);
	}
	CSmartPtr(T* ptr, const std::function<void (T* ptr)>& fnd) 
		: m_refcount(0){
		if (ptr || fnd)m_refcount = new(std::nothrow) CRefCount<T>(ptr, fnd);
	}
	CSmartPtr(T* ptr, void (T::*fnd)())
		: m_refcount(0) {
		if (ptr || fnd)m_refcount = new(std::nothrow) CRefCount<T>(ptr, fnd);
	}
	CSmartPtr(const CSmartPtr<T>& sp) 
		: m_refcount(0){
		Assign(sp.m_refcount);
	}
	virtual~CSmartPtr(){
		Assign(0);
	}

	T* GetPtr() const {
		if (m_refcount == 0)
			return 0;
		else
			return m_refcount->GetPtr();
	}
	T*& GetPtr() {
		if (m_refcount == 0) {
			m_refcount = new CRefCount<T>;
		}
		return m_refcount->GetPtr();
	}
	CSmartPtr& operator=(const CSmartPtr<T>& sp) {
		Assign(sp.m_refcount); 
		return *this;
	}
	T* operator->(){
		ASSERT(GetPtr() != 0);
		return GetPtr();
	}
	const T* operator->() const{
		ASSERT(GetPtr() != 0);
		return (const T*)GetPtr();
	}
	operator T*() const{
		return GetPtr();
	}
	bool operator!(){
		return GetPtr()==0;
	}
	void Attach(T* ptr, const std::function<void (T* ptr)>& fnd = 0){
		CRefCount<T>*	ref(new(std::nothrow) CRefCount<T>(ptr, fnd));
		Assign(ref);
		ref->DecRefCount();
	}
	T* Detach(void){
		if(m_refcount == 0)return 0;
		T*		ptr(m_refcount->GetPtr());
		delete m_refcount;
		m_refcount		= 0;
		return ptr;
	}
	bool operator==(const CSmartPtr& sp){
		return m_refcount == sp.m_refcount;
	}
	bool operator==(const T* p){
		return GetPtr() == p;
	}
	bool operator!=(const CSmartPtr& sp){
		return m_refcount != sp.m_refcount;
	}
	bool operator!=(const T* p){
		return GetPtr() != p;
	}

	bool operator>(CSmartPtr &Other) { 
		return this.operator>(*Other->GetPtr()); 
	}
	bool operator<(CSmartPtr &Other) { 
		return this.operator<(*Other->GetPtr()); 
	}
	template <class U>U* Cast(U*) { 
		return dynamic_cast<U*>(GetPtr()); 
	} 
};