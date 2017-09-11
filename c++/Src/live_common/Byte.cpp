#include <algorithm>
#include "Byte.h"

CByte::CByte(int increment /* = 1024 */)
	: bNoThrow(false)
	, nInc(increment)
	, nReserved(0)
	, saCache(0)
	, szCache(0)
	, szData(0) {
}
CByte::CByte(int increment, int reseved)
	: bNoThrow(false)
	, nInc(increment)
	, nReserved(reseved)
	, saCache(0)
	, szCache(0)
	, szData(0) {
}
CByte::CByte(const char* const arr, std::nothrow_t nothrow /* = std::nothrow */, int increment /* = 1024 */) throw()
	: bNoThrow(false)
	, nInc(increment)
	, nReserved(0)
	, saCache(0)
	, szCache(0)
	, szData(0) {
	if (!EnsureSize((int)strlen(arr)))return;
	memcpy(saCache, arr, strlen(arr));
	szData = (int)strlen(arr);
}
CByte::CByte(std::nothrow_t nothrow, int increment /* = 1024 */)
	: bNoThrow(true)
	, nInc(increment)
	, saCache(0)
	, szCache(0)
	, szData(0) {
}

CByte& CByte::operator=(char arr[]) throw() {
	if (!EnsureSize((int)strlen(arr)))return *this;
	memcpy(saCache, arr, strlen(arr));
	szData = (int)strlen(arr);
	return *this;
}
CByte& CByte::operator=(const CByte& src) throw() {
	if (!EnsureSize(src.szData))return *this;
	memcpy(saCache, src.saCache, szData);
	szData = src.szData;
	return *this;
}
CByte::BYTE& CByte::operator[](const int pos) throw() {
	return saCache[pos];
}

bool CByte::operator<(const BYTE* const right) {
	int result(memcmp(saCache, right, szData));
	return result < 0;
}
bool CByte::operator>(const BYTE* const right) {
	int result(memcmp(saCache, right, szData));
	return result > 0;
}

bool CByte::operator==(const BYTE* const right) {
	int result(memcmp(saCache, right, szData));
	return result == 0;
}
bool CByte::operator!=(const BYTE* const right) {
	int result(memcmp(saCache, right, szData));
	return result != 0;
}

bool CByte::operator==(const int nNull) {
	return saCache.GetArr() == NULL;
}
bool CByte::operator!=(const int nNull) {
	return saCache.GetArr() != NULL;
}

void CByte::Attach(BYTE* arr, const int dataSize, const int cacheSize /* = -1 */) throw() {
	saCache = arr;
	szData = dataSize;
	nReserved = 0;
	szCache = cacheSize > 0 ? cacheSize : dataSize;
}
CByte::BYTE* CByte::Detach() {
	return saCache.Detach();
}

CByte::BYTE* CByte::EnsureSize(const int nSize, const bool bRemain /* = false*/) throw() {
	if (nReserved + nSize > szCache) {
		szCache = ((nReserved + nSize + 16 + nInc - 1) / nInc) * nInc;
		LPBYTE lpNew(NULL);
		if (bNoThrow)lpNew = new(std::nothrow) BYTE[szCache];
		else lpNew = new BYTE[szCache];
		if (lpNew == NULL)return NULL;
		if (bRemain && saCache && (szData + nReserved) > 0)
			memcpy(lpNew, saCache, szData + nReserved);
		else
			szData = 0;
		saCache.Attach(lpNew);
	}
	return saCache.GetArr();
}
int CByte::SetSize(const int nSize) throw() {
	if (nSize < 1)return 0;
	if (!EnsureSize(nSize))return 0;
	szData = nSize;
	return nSize;
}

int CByte::FillData(const CByte& src)  throw() {
    if (src.GetData() == NULL || src.GetSize() < 1)return 0;
    if (!EnsureSize(src.GetSize()))return 0;
    memcpy(saCache + nReserved, src.GetData(), src.GetSize());
    szData = src.GetSize();
    return szData;
}
int CByte::FillData(const void* const lpData, const int nSize) throw() {
	if (lpData == NULL || nSize < 1)return 0;
	if (!EnsureSize(nSize))return 0;
	memcpy(saCache + nReserved, lpData, nSize);
	szData = nSize;
	return szData;
}
int CByte::AppendData(const CByte& src) throw() {
    if (src.GetData() == NULL || src.GetSize() < 1)return szData;
    if (!EnsureSize(szData + src.GetSize(), true))return szData;
    memcpy(saCache + nReserved + szData, src.GetData(), src.GetSize());
    szData += src.GetSize();
    return szData;
}
int CByte::AppendData(const void* const lpData, const int nSize) throw() {
	if (lpData == NULL || nSize < 1)return szData;
	if (!EnsureSize(szData + nSize, true))return szData;
	memcpy(saCache + nReserved + szData, lpData, nSize);
	szData += nSize;
	return szData;
}
int CByte::FillDatas(const void* const* const lpDataPointers, const int* const nDataSizes, const int nDataCount) throw() {
	if (nDataCount < 1)return 0;
	int nTotalSize(0);
	for (int iData = 0; iData < nDataCount; iData++) {
		nTotalSize += nDataSizes[iData];
	}
	if (!EnsureSize(nTotalSize, false))return 0;
	LPBYTE lpDataOffset(saCache.GetArr() + nReserved);
	for (int iData = 0; iData < nDataCount; iData++) {
		memcpy(lpDataOffset, lpDataPointers[iData], nDataSizes[iData]);
		lpDataOffset+= nDataSizes[iData];
	}
	szData = nTotalSize;
	return nTotalSize;
}
int CByte::AppendDatas(const void* const* const lpDataPointers, const int* const nDataSizes, const int nDataCount) throw() {
	if (nDataCount < 1)return 0;
	int nTotalSize(0);
	for (int iData = 0; iData < nDataCount; iData++) {
		nTotalSize += nDataSizes[iData];
	}
	if (!EnsureSize(nTotalSize + szData, true))return 0;
	LPBYTE lpDataOffset(saCache.GetArr() + nReserved + szData);
	for (int iData = 0; iData < nDataCount; iData++) {
		memcpy(lpDataOffset, lpDataPointers[iData], nDataSizes[iData]);
		lpDataOffset+= nDataSizes[iData];
	}
	szData += nTotalSize;
	return szData;
}

int CByte::GrowSize(const int nSize) throw() {
	if (!EnsureSize(szData + nSize, true))return 0;
	szData += nSize;
	return szData;
}
int CByte::LeftShift(const int nCount)  {
	if (nCount >= szData) {
		szData = 0;
		return 0;
	}
	memmove(saCache + nReserved, saCache + nReserved + nCount, szData - nCount);
	szData -= nCount;
	return szData;
}
int CByte::GetData(void* const lpCache, const int nMaxSize) {
	int nCopy(nMaxSize < szData ? nMaxSize : szData);
	memcpy(lpCache, saCache + nReserved, nCopy);
	return nCopy;
}
void CByte::Clear() {
	szData = 0;
}

xtstring CByte::toHex(const BYTE* const lpData, const int nSize, const int insertBlankPerNumber) {
	xtstring strHex;
	int nLen(nSize * 2 + ((insertBlankPerNumber > 0) ? (nSize / insertBlankPerNumber) : 0) + 2);
	LPTSTR lpszHead(strHex.GetBuffer(nLen));
#ifdef _WIN32
	LPTSTR lpszEnd(lpszHead + nLen);
#endif
	LPTSTR lpszTail(lpszHead);
	for (int i = 0; i < nSize; i++)	{
		lpszTail += _stprintf_s(lpszTail, lpszEnd - lpszTail, _T("%02X"), lpData[i]);
		if (insertBlankPerNumber > 0 && ((i + 1) % insertBlankPerNumber == 0)) {
			*lpszTail++ = ' ';
		}
	}
	*lpszTail++ = 0;
	strHex.ReleaseBuffer((int)(lpszTail - lpszHead));
	return strHex;
}
int CByte::fromHex(BYTE* const lpData, const xtstring& strHex) {
	xtstring strHex1(strHex);
	strHex1.Replace(_T(" "), _T(""));
	LPCTSTR lpsz(strHex1.GetString());
	int nLen(strHex1.GetLength() / 2);
	BYTE* lpDes(lpData);
	for(int i = 0; i < nLen; i++) {
		unsigned short	tmp(0);
		_stscanf_s(lpsz, _T("%2hx"), &tmp);
		lpDes[i] = (BYTE)tmp;
		lpsz += 2;
	}
	return nLen;
}
xtstring CByte::toBase64(const BYTE* const lpData, const int nSize) {
	xtstring strBase64;
	const BYTE* inData(lpData);
	int dataLength(nSize);
	int len([](int dataLength) -> int{
		int len = dataLength + dataLength / 3 + (int)(dataLength % 3 != 0);
		if(len % 4)len	+= 4 - len % 4;
		return len;
	}(nSize));
	LPTSTR out(strBase64.GetBuffer(len));
	static const TCHAR CHAR_63('+');
	static const TCHAR CHAR_64('-');
	static const TCHAR CHAR_PAD('=');
	static const TCHAR alph[] = {
		'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
		'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z',
		'0','1','2','3','4','5','6','7','8','9', CHAR_63, CHAR_64
	};
	const int mask(0x3F);
	union {
		unsigned char bytes[4];
		unsigned int block;
	} buffer;
	for(int i = 0, j = 0, left = dataLength; i < dataLength; i += 3, j += 4, left -= 3) {
		buffer.bytes[2] = inData[i];
		if (left > 1) {
			buffer.bytes[1]	= inData[i + 1];
			if (left > 2)buffer.bytes[0]= inData[i + 2];
			else buffer.bytes[0]		= 0;
		} else {
			buffer.bytes[1] = 0;
			buffer.bytes[0] = 0;
		}
		out[j] = alph[(buffer.block >> 18) & mask];
		out[j + 1] = alph[(buffer.block >> 12) & mask];
		if(left > 1) {
			out[j + 2] = alph[(buffer.block >> 6) & mask];
			if (left > 2)out[j + 3] = alph[buffer.block & mask];
			else out[j + 3] = CHAR_PAD;
		} else {
			out[j + 2] = CHAR_PAD;
			out[j + 3] = CHAR_PAD;
		}
	}
	strBase64.ReleaseBuffer(len);
	return strBase64;
}
int CByte::fromBase64(BYTE* const lpData, const xtstring& strBase64) {
	BYTE* outData(lpData);
	LPCTSTR inCode(strBase64.GetString());
	int codeLength(strBase64.GetLength());	
	static const wchar_t CHAR_63('+');
	static const wchar_t CHAR_64('-');
	static const wchar_t CHAR_PAD('=');
	union {
		unsigned char bytes[4];
		unsigned int block;
	} buffer;
	buffer.block = 0;
	int j(0);
	for (int i = 0; i < codeLength; i++) {
		int m(i % 4);
		wchar_t x(inCode[i]);
		int val(0);
		if(x >= 'A' && x <= 'Z')val = x - 'A';
		else if(x >= 'a' && x <= 'z')val = x - 'a' + 'Z' - 'A' + 1;
		else if(x >= '0' && x <= '9')val = x - '0' + ('Z' - 'A' + 1) * 2;
		else if(x == CHAR_63)val = 62;
		else if(x == CHAR_64)val = 63;
		if(x != CHAR_PAD)buffer.block |= val << (3 - m) * 6;
		else m--;
		if(m == 3 || x == CHAR_PAD) {
			outData[j++] = buffer.bytes[2];
			if(x != CHAR_PAD || m > 1) {
				outData[j++] = buffer.bytes[1];
				if(x != CHAR_PAD || m > 2)outData[j++]	= buffer.bytes[0];
			}
			buffer.block = 0;
		}
		if(x == CHAR_PAD)break;
	}
	return j;
}
xtstring CByte::toPrintable(const BYTE* const lpData, const int nSize) {
	const BYTE* begin(lpData);
	const BYTE* end(lpData + nSize + 1);
	xtstring des;
	std::for_each(begin, end, 
		[&des](BYTE b){
			if (::isprint(b))des.AppendFormat(_T("%c"), b);
			else des.Append(_T(" "));
	});
	return des;
}
xtstring CByte::toHex(const int onlyTopNumber /* = -1 */, const int insertBlankPerNumber /* = 0 */) {
	int nLen(onlyTopNumber);
	if (nLen < 0 || nLen > szData) {
		nLen = szData;
	}
	return toHex(saCache + nReserved, nLen, insertBlankPerNumber);
}
bool CByte::fromHex(const xtstring& strHex) {
	xtstring strHex1(strHex);
	strHex1.Replace(_T(" "), _T(""));
	if (!EnsureSize(strHex1.GetLength() / 2))return false;
	szData = fromHex(saCache.GetArr() + nReserved, strHex1);
	return true;
}
xtstring CByte::toBase64(const int onlyTopNumber /* = -1 */) {
	int nLen(onlyTopNumber);
	if (nLen < 0 || nLen > szData) {
		nLen = szData;
	}
	return toBase64(saCache + nReserved, nLen);
}
bool CByte::fromBase64(const xtstring& strBase64) {
	int codeLength(strBase64.GetLength());
	int nLen(codeLength - codeLength / 4);
	if (!EnsureSize(nLen))return false;
	szData = fromBase64(saCache + nReserved, strBase64);
	return true;
}
xtstring CByte::toPrintable(const int onlyTopNumber /* = -1 */) {
	int nLen(onlyTopNumber);
	if (nLen < 0 || nLen > szData) {
		nLen = szData;
	}
	return toPrintable(saCache, nLen);
}