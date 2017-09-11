#pragma once
#include "xstring.h"
#include "SmartArr.h"

class CByte {
public:
	typedef unsigned char		BYTE;
	typedef BYTE*				LPBYTE;

public:
	CByte(int increment = 1024);
	CByte(int increment, int reseved);
	CByte(std::nothrow_t nothrow, int increment = 1024);
	CByte(const char* const arr, std::nothrow_t nothrow = std::nothrow, int increment = 1024) throw();

public:
	CByte&			operator=(char arr[]) throw();
	CByte&			operator=(const CByte& src) throw();
	BYTE&			operator[](const int pos) throw();
	operator		BYTE*() const {return saCache;}
	operator		int() const {return szData;}
	bool			operator<(const BYTE* const right);
	bool			operator>(const BYTE* const right);
	bool			operator==(const BYTE* const right);
	bool			operator!=(const BYTE* const right);
	bool			operator==(const int nNull);
	bool			operator!=(const int nNull);
	void			Attach(BYTE* arr, const int dataSize, const int cacheSize = -1) throw();
	BYTE*			Detach();

public:
	BYTE*			EnsureSize(const int nSize, const bool bRemain = false) throw();
	int				SetSize(const int nSize) throw();
    int             FillData(const CByte& src) throw();
	int				FillData(const void* const lpData, const int nSize) throw();
    int				AppendData(const CByte& src) throw();
    int				AppendData(const void* const lpData, const int nSize) throw();
	int				FillDatas(const void* const* const lpDataPointers, const int* const nDataSizes, const int nDataCount) throw();
	int				AppendDatas(const void* const* const lpDataPointers, const int* const nDataSizes, const int nDataCount) throw();
	int				GrowSize(const int nSize) throw();
	int				LeftShift(const int nCount);
	int				GetSize() const { return szData; }
	const BYTE*     GetData() const { return saCache.GetArr() + nReserved; }
	BYTE*			GetData() { return saCache.GetArr() + nReserved; }
	BYTE*			GetPos() { return saCache.GetArr() + szData + nReserved; }
	int				GetData(void* const lpCache, const int nMaxSize);
	int				GetLimit() const { return szCache; }
	void			Clear();

public:
	static xtstring	toHex(const BYTE* const lpData, const int nSize, const int insertBlankPerNumber);
	static int		fromHex(BYTE* const lpData, const xtstring& strHex);
	static xtstring	toBase64(const BYTE* const lpData, const int nSize);
	static int		fromBase64(BYTE* const lpData, const xtstring& strBase64);
	static xtstring	toPrintable(const BYTE* const lpData, const int nSize);

public:
	xtstring		toHex(const int onlyTopNumber = -1, const int insertBlankPerNumber = 0);
	bool			fromHex(const xtstring& strHex);
	xtstring		toBase64(const int onlyTopNumber = -1);
	bool			fromBase64(const xtstring& strBase64);
	xtstring		toPrintable(const int onlyTopNumber = -1);

protected:
#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	bool			bNoThrow;
	int				nInc;
	int				nReserved;	//×ó²à±£Áô³¤¶È
	CSmartArr<BYTE>	saCache;
	int				szCache;
	int				szData;
#ifdef _WIN32
#pragma warning(pop)
#endif
};