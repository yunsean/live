#pragma once
#include <functional>
#include <assert.h>
#include <string.h>
#include "os.h"

template <typename T>
class CSmartNal {
private:
	template <typename T1> 
	class CRefCount {
	public:
		CRefCount(int sz, const std::function<void(T ptr[])>& d = nullptr) 
			: m_array(nullptr)
			, m_count(0)
			, m_space(0)
			, m_counter(0)
			, m_fnDelete(d){
			if(sz){
				m_array				= new(std::nothrow) T1[sz * 2];
				if (m_array){
					m_space			= sz * 2;
					m_count			= sz;
				}
			}
			else{
				m_array				= nullptr;
			}
			m_counter++;
		}
		CRefCount(T1 arr[], int sz, const std::function<void(T ptr[])>& d = nullptr) 
			: m_array(arr)
			, m_count(sz)
			, m_space(0)
			, m_counter(0)
			, m_fnDelete(d){
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
				if(m_fnDelete && m_space > 0)
					m_fnDelete(m_array);
				else if (m_space > 0)
					delete[] m_array;
				m_array		= nullptr;
				m_count		= 0;
				m_space		= 0;
				delete this;
			}
		}
		T1* GetArr(){
			return m_array;
		}
		int GetSize(){
			return m_count;
		}
		int GetSpace(){
			return m_space;
		}
		void SetSize(const int size){
			m_count			= size;
		}

	private:
		friend class CSmartNal<T1>;
		T1*								m_array;
		int								m_count;
		int								m_space;
		int								m_counter;	
		std::function<void(T ptr[])>	m_fnDelete;
	};
	CRefCount<T>*			m_refcount;

public:
	void Assign(CRefCount<T>* refcount){
		if(refcount != nullptr)refcount->IncRefCount();
		CRefCount<T>*		oldref(m_refcount);
		m_refcount			= refcount;
		if(oldref != nullptr)oldref->DecRefCount();
	}

public:
	CSmartNal(T* anullptr = nullptr) 
		: m_refcount(nullptr){
		assert(anullptr == nullptr);
	}
	CSmartNal(const int size, const std::function<void(T ptr[])>& fnd = nullptr) 
		: m_refcount(nullptr){
		if (size > 0)m_refcount	= new(std::nothrow) CRefCount<T>(size, fnd);
	}
	CSmartNal(T arr[], int sz, const std::function<void(T ptr[])>& fnd = nullptr) 
		: m_refcount(nullptr){
		if (arr)m_refcount		= new(std::nothrow) CRefCount<T>(arr, sz, fnd);
	}
	CSmartNal(const CSmartNal<T>& sp) 
		: m_refcount(nullptr){
		Assign(sp.m_refcount);
	}
	virtual ~CSmartNal(){
		Assign(nullptr);
	}

	T* Allocate(const int size){
		if (m_refcount && m_refcount->m_space >= size)return m_refcount->m_array;
		CRefCount<T>*	ref(new(std::nothrow) CRefCount<T>(size, m_refcount ? m_refcount->m_fnDelete : nullptr));
		if (ref == nullptr)return nullptr;
		Assign(ref);
		ref->DecRefCount();
		return GetArr();
	}
	T* GetArr() const{
		if(m_refcount == nullptr)return nullptr;
		else return m_refcount->GetArr();
	}
	int GetSize() const{
		if (m_refcount == nullptr)return 0;
		else return m_refcount->GetSize();
	}
	void SetSize(const int size){
		if (m_refcount)m_refcount->SetSize(size);
	}
	int GetSpace() const{
		if (m_refcount == nullptr)return 0;
		else return m_refcount->GetSpace();
	}
	bool Copy(const T arr[], int sz){
		CRefCount<T>*	ref(new(std::nothrow) CRefCount<T>(sz, m_refcount ? m_refcount->m_fnDelete : nullptr));
		if (ref == nullptr)return false;
		memcpy(ref->GetArr(), arr, sz * sizeof(T));
		Assign(ref);
		ref->DecRefCount();
		return true;
	}
	CSmartNal& operator=(const CSmartNal<T>& sp) {
		Assign(sp.m_refcount); 
		return *this;
	}
	T& operator[](const int pos){
		assert(GetArr() != nullptr && pos < GetSize());
		return GetArr()[pos];
	}
	operator T*() const{
		return GetArr();
	}
	bool operator!(){
		return GetArr()==nullptr;
	}
	bool Attach(T arr[], int sz, const std::function<void(T ptr[])>& fnd = nullptr){
		CRefCount<T>*	ref(new(std::nothrow) CRefCount<T>(arr, sz, fnd));
		if (ref == nullptr)return false;
		Assign(ref);
		ref->DecRefCount();
		return true;
	}
	T* Detach(void){
		if(m_refcount == nullptr)return nullptr;
		T*		ptr(m_refcount->GetArr());
		delete m_refcount;
		m_refcount		= nullptr;
		return ptr;
	}
	bool operator==(const CSmartNal& sp){
		return m_refcount == sp.m_refcount;
	}
	bool operator==(const T* anullptr){
		assert(anullptr == nullptr);
		return GetArr() == nullptr;
	}
	bool operator!=(const CSmartNal& sp){
		return m_refcount != sp.m_refcount;
	}
	bool operator!=(const T* anullptr){
		assert(anullptr == nullptr);
		return GetArr() != nullptr;
	}
	template <class U>U* Cast(U*){ 
		return dynamic_cast<U*>(GetArr()); 
	} 
};