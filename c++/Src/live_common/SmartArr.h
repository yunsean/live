#pragma once
#include <assert.h>
#include "os.h"

template <typename T>
class CSmartArr {
private:
	typedef void (*DELETEARR)(T ptr[]);
	template <typename T1> 
	class CRefCount	{
	public:
		CRefCount(int sz, DELETEARR d = nullptr) 
			: m_array(nullptr)
			, m_counter(0)
			, m_fnDelete(d){
			if(sz)m_array	= new T[sz];
			m_counter++;
		}
		CRefCount(T arr[], DELETEARR d = nullptr) 
			: m_array(arr)
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
				if(m_fnDelete)
					m_fnDelete(m_array);
				else
					delete[] m_array;
				m_array		= nullptr;
				delete this;
			}
		}

		T1*& GetArr(){
			return m_array;
		}
	private:
		T1*					m_array;
		int					m_counter;	
		DELETEARR			m_fnDelete;
	};
	CRefCount<T>*			m_refcount;

public:
	void Assign(CRefCount<T>* refcount){
		if(refcount != nullptr)refcount->IncRefCount();
		CRefCount<T>*		oldref(m_refcount);
		m_refcount			= refcount;
		if(oldref != nullptr) oldref->DecRefCount();
	}

public:
	CSmartArr(void) 
		: m_refcount(nullptr){
	}
	CSmartArr(const int size, DELETEARR fnd = nullptr) 
		: m_refcount(nullptr){
		if (size > 0)m_refcount	= new CRefCount<T>(size, fnd);
	}
	CSmartArr(T arr[], DELETEARR fnd = nullptr) :
		m_refcount(nullptr){
		if (arr)m_refcount	= new CRefCount<T>(arr, fnd);
	}

	CSmartArr(const CSmartArr<T>& sp) 
		: m_refcount(nullptr){
		Assign(sp.m_refcount);
	}
	virtual ~CSmartArr(){
		Assign(nullptr);
	}

	T* GetArr() const {
		if (m_refcount == nullptr) return nullptr;
		else return m_refcount->GetArr();
	}
	T*& GetArr() {
		if (m_refcount == nullptr) {
			m_refcount = new CRefCount<T>(nullptr);
		}
		return m_refcount->GetArr();
	}
	CSmartArr& operator=(const CSmartArr<T>& sp) {
		Assign(sp.m_refcount); 
		return *this;
	}
	CSmartArr& operator=(T arr[]) {
		Assign(nullptr);
		CRefCount<T>*	ref(new CRefCount<T>(arr));
		if (ref == nullptr)return *this;
		Assign(ref);
		ref->DecRefCount();
		return *this;
	}
	T& operator[](const int pos){
		assert(GetArr() != nullptr);
		return GetArr()[pos];
	}
	operator T*() const{
		return GetArr();
	}
	bool operator!(){
		return GetArr()==nullptr;
	}
	bool Attach(T arr[], DELETEARR fnd = nullptr){
		CRefCount<T>*	ref(new CRefCount<T>(arr, fnd));
		if (ref == nullptr)return false;
		Assign(ref);
		ref->DecRefCount();
		return true;
	}
	bool Copy(T arr[], const int size, DELETEARR fnd = nullptr){
		CRefCount<T>*	ref(new CRefCount<T>(size, fnd));
		if (ref == nullptr)return false;
		if (ref->GetArr() == nullptr)return false;
		memcpy(ref->GetArr(), arr, size);
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
	bool isNull() {
		return GetArr() == nullptr;
	}
	bool operator==(const CSmartArr& sp){
		return GetArr() == sp.GetArr();
	}
	bool operator==(const int nnullptr){
		return GetArr() == nullptr;
	}
	bool operator!=(const CSmartArr& sp){
		return GetArr() != sp.GetArr();
	}
	bool operator!=(const int nnullptr){
		return GetArr() != nullptr;
	}
	template <class U>U* Cast(U*) { 
		return dynamic_cast<U*>(GetArr()); 
	} 
};
