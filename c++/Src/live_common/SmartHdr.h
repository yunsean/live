#pragma once 
#include "os.h"

#ifdef _WIN32
template <typename handleType, typename retType = int, typename parType = handleType>
#else 
template <typename handleType, typename retType = int, typename parType = handleType>
#endif
class CSmartHdr {
#if defined(_WIN32) && !defined(_WIN64)
	typedef retType (_stdcall *STDCALLDELETEPTR)(parType ptr);
	typedef retType (__cdecl *CDECALLDELETEPTR)(parType ptr);
	typedef retType (__fastcall *FASTCALLDELETEPTR)(parType ptr);
#else 
	typedef retType (*CDECALLDELETEPTR)(parType ptr);
#endif

	template <typename handleType1, typename retType1>
	class CRefCount	{
	public:
		CRefCount(handleType1 h, CDECALLDELETEPTR cd, 
#if defined(_WIN32) && !defined(_WIN64)
			STDCALLDELETEPTR sd, FASTCALLDELETEPTR fd, 
#endif
			const handleType1& except) 
			: m_handler(except)
			, m_counter(0)
			, m_fnCdeCallDelete(cd)
#if defined(_WIN32) && !defined(_WIN64)
			, m_fnStdCallDelete(sd)
			, m_fnFastCallDelete(fd)
#endif
			, m_hExceptCleanup(except){
			if(h != except)
				m_handler	= h;
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
				if(m_fnCdeCallDelete && m_handler != m_hExceptCleanup){
					m_fnCdeCallDelete(m_handler);
				}
#if defined(_WIN32) && !defined(_WIN64)
				else if(m_fnStdCallDelete && m_handler != m_hExceptCleanup){
					m_fnStdCallDelete(m_handler);
				}
				else if(m_fnFastCallDelete && m_handler != m_hExceptCleanup){
					m_fnFastCallDelete(m_handler);
				}
#endif

				m_handler	= m_hExceptCleanup;
				delete this;
			}
		}

		handleType1& GetHdr(){
			return m_handler;
		}

	private:
		handleType1							m_handler;
		int									m_counter;	
		CDECALLDELETEPTR					m_fnCdeCallDelete;
#if defined(_WIN32) && !defined(_WIN64)
		STDCALLDELETEPTR					m_fnStdCallDelete;
		FASTCALLDELETEPTR					m_fnFastCallDelete;
#endif 
		handleType1							m_hExceptCleanup;
	};

	CRefCount<handleType, retType>*			m_refcount;
	CDECALLDELETEPTR						m_fnCdeCallDelete;
#if defined(_WIN32) && !defined(_WIN64)
	STDCALLDELETEPTR						m_fnStdCallDelete;
	FASTCALLDELETEPTR						m_fnFastCallDelete;
#endif
	handleType								m_hExceptCleanup;

protected:
	void Assign(CRefCount<handleType, retType>* refcount){
		if(refcount != 0)
			refcount->IncRefCount();

		CRefCount<handleType, retType>*		oldref(m_refcount);
		m_refcount							= refcount;

		if(oldref != 0) 
			oldref->DecRefCount();
	}

public:
	CSmartHdr(const CDECALLDELETEPTR fnd, const handleType& except = 0)
		: m_refcount(0)
		, m_fnCdeCallDelete(fnd)
#if defined(_WIN32) && !defined(_WIN64)
		, m_fnStdCallDelete(0)
		, m_fnFastCallDelete(0)
#endif
		, m_hExceptCleanup(except){
		m_refcount							= new CRefCount<handleType, retType>(except, fnd, 
#if defined(_WIN32) && !defined(_WIN64)
			0, 0,
#endif
			m_hExceptCleanup);
	}
#if defined(_WIN32) && !defined(_WIN64)
	CSmartHdr(const STDCALLDELETEPTR fnd, const handleType& except = 0)
		: m_refcount(0)
		, m_fnCdeCallDelete(0)
		, m_fnStdCallDelete(fnd)
		, m_fnFastCallDelete(0)
		, m_hExceptCleanup(except){
		m_refcount							= new CRefCount<handleType, retType>(except, 0, fnd, 0, m_hExceptCleanup);
	}
	CSmartHdr(const FASTCALLDELETEPTR fnd, const handleType& except = 0)
		: m_refcount(0)
		, m_fnCdeCallDelete(0)
		, m_fnStdCallDelete(0)
		, m_fnFastCallDelete(fnd)
		, m_hExceptCleanup(except){
		m_refcount							= new CRefCount<handleType, retType>(except, 0, 0, fnd, m_hExceptCleanup);
	}
#endif
	CSmartHdr(handleType h, CDECALLDELETEPTR fnd, const handleType& except = 0) 
		: m_refcount(0)
		, m_fnCdeCallDelete(fnd)
#if defined(_WIN32) && !defined(_WIN64)
		, m_fnStdCallDelete(0)
		, m_fnFastCallDelete(0)
#endif
		, m_hExceptCleanup(except){
		m_refcount							= new CRefCount<handleType, retType>(h, fnd,
#if defined(_WIN32) && !defined(_WIN64)
			0, 0,
#endif
			m_hExceptCleanup);
	}
#if defined(_WIN32) && !defined(_WIN64)
	CSmartHdr(handleType h, STDCALLDELETEPTR fnd, const handleType& except = 0) 
		: m_refcount(0)
		, m_fnCdeCallDelete(0)
		, m_fnStdCallDelete(fnd)
		, m_fnFastCallDelete(0)
		, m_hExceptCleanup(except){
		m_refcount							= new CRefCount<handleType, retType>(h, 0, fnd, 0, m_hExceptCleanup);
	}
	CSmartHdr(handleType h, FASTCALLDELETEPTR fnd, const handleType& except = 0) 
		: m_refcount(0)
		, m_fnCdeCallDelete(0)
		, m_fnStdCallDelete(0)
		, m_fnFastCallDelete(fnd)
		, m_hExceptCleanup(except){
		m_refcount							= new CRefCount<handleType, retType>(h, 0, 0, fnd, m_hExceptCleanup);
	}
#endif
	CSmartHdr(const CSmartHdr<handleType, retType>& sp) 
		: m_refcount(0)
		, m_fnCdeCallDelete(sp.m_fnCdeCallDelete)
#if defined(_WIN32) && !defined(_WIN64)
		, m_fnStdCallDelete(sp.m_fnStdCallDelete)
		, m_fnFastCallDelete(sp.m_fnFastCallDelete)
#endif
		, m_hExceptCleanup(sp.m_hExceptCleanup){
		Assign(sp.m_refcount);
	}
	virtual ~CSmartHdr(){
		Assign(0);
	}

	handleType GetHdr() const throw(){
		if(m_refcount == 0) 
			return m_hExceptCleanup;
		else
			return m_refcount->GetHdr();
	}

	CSmartHdr& operator=(const CSmartHdr<handleType, retType>& sp) {
		Assign(sp.m_refcount); 
		return *this;
	}
	CSmartHdr& operator=(handleType h) {
		Assign(new CRefCount<handleType, retType>(h, m_fnCdeCallDelete,
#if defined(_WIN32) && !defined(_WIN64)
			m_fnStdCallDelete, m_fnFastCallDelete, 
#endif
			m_hExceptCleanup));
		m_refcount->DecRefCount();
		return *this;
	}
	operator handleType() const throw(){
		return GetHdr();
	}
	handleType* operator&() throw(){
		if(m_refcount == 0)
		{
			Assign(new CRefCount<handleType, retType>(0, m_fnCdeCallDelete, 
#if defined(_WIN32) && !defined(_WIN64)
				m_fnStdCallDelete, m_fnFastCallDelete,
#endif
				m_hExceptCleanup));
			m_refcount->DecRefCount();
		}
		return &m_refcount->GetHdr();
	}
	handleType operator->() throw(){
		if(m_refcount == 0)
		{
			Assign(new CRefCount<handleType, retType>(0, m_fnCdeCallDelete,
#if defined(_WIN32) && !defined(_WIN64)
				m_fnStdCallDelete, m_fnFastCallDelete, 
#endif
				m_hExceptCleanup));
			m_refcount->DecRefCount();
		}
		return m_refcount->GetHdr();
	}
	bool operator!(){
		return GetHdr()==0;
	}

	void Attach(handleType h){
		CRefCount<handleType, retType>*		ref(new CRefCount<handleType, retType>(h, m_fnCdeCallDelete, 
#if defined(_WIN32) && !defined(_WIN64)
			m_fnStdCallDelete, m_fnFastCallDelete, 
#endif
			m_hExceptCleanup));
		Assign(ref);
	}
	void Attach(handleType h, CDECALLDELETEPTR fnd){
		m_fnCdeCallDelete					= fnd;
#if defined(_WIN32) && !defined(_WIN64)
		m_fnStdCallDelete					= 0;
		m_fnFastCallDelete					= 0;
#endif
		CRefCount<handleType, retType>*		ref(new CRefCount<handleType, retType>(h, m_fnCdeCallDelete,
#if defined(_WIN32) && !defined(_WIN64)
			m_fnStdCallDelete, m_fnFastCallDelete,
#endif
			m_hExceptCleanup));
		Assign(ref);
	}
#if defined(_WIN32) && !defined(_WIN64)
	void Attach(handleType h, STDCALLDELETEPTR fnd){
		m_fnCdeCallDelete					= 0;
		m_fnStdCallDelete					= fnd;
		m_fnFastCallDelete					= 0;
		CRefCount<handleType, retType>*		ref(new CRefCount<handleType, retType>(h, m_fnCdeCallDelete, m_fnStdCallDelete, m_fnFastCallDelete, m_hExceptCleanup));
		Assign(ref);
	}
	void Attach(handleType h, FASTCALLDELETEPTR fnd){
		m_fnCdeCallDelete					= 0;
		m_fnStdCallDelete					= 0;
		m_fnFastCallDelete					= fnd;
		CRefCount<handleType, retType>*		ref(new CRefCount<handleType, retType>(h, m_fnCdeCallDelete, m_fnStdCallDelete, m_fnFastCallDelete, m_hExceptCleanup));
		Assign(ref);
	}
#endif
	handleType Detach(void){
		if(m_refcount == 0)return 0;
		handleType							h(m_refcount->GetHdr());
		delete m_refcount;
		m_refcount							= 0;
		return h;
	}

	bool operator==(const CSmartHdr<handleType, retType>& sp){
		return m_refcount == sp.m_refcount;
	}
	bool operator==(const handleType& h) {
		return GetHdr() == h;
	}
	bool operator!=(const CSmartHdr<handleType, retType>& sp){
		return m_refcount != sp.m_refcount;
	}
	bool operator!=(const handleType& h) {
		return GetHdr() != h;
	}

	bool isValid() {
		return GetHdr() != m_hExceptCleanup;
	}

	template <class U> U Cast(U) { 
		return dynamic_cast<U>(GetHdr()); 
	} 			
};
