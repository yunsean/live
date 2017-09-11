#include <algorithm>
#include <string.h>
#include "SmartBlk.h"

CTypicalBlock::CTypicalBlock()
	: saCache(0)
	, szCache(0)
	, szData(0) {
}

char* CTypicalBlock::EnsureBlock(const int nSize, const bool bRemain /* = false*/ ) throw() {
	if (nSize > szCache) {
		szCache		= nSize;
		char*		lpNew(new char[szCache + 16]);
		if (bRemain && szData)
			memcpy(lpNew, saCache, szData);
		else
			szData	= 0;
		saCache.Attach(lpNew);
	}
	return saCache.GetArr();
}
void CTypicalBlock::SetSize(const int nSize) throw() {
	EnsureBlock(nSize);
	szData			= nSize;
}
void CTypicalBlock::FillData(const void* const lpData, const int nSize) throw() {
	EnsureBlock(nSize);
	memcpy(saCache, lpData, nSize);
	szData			= nSize;
}
void CTypicalBlock::AppendData(const void* const lpData, const int nSize) throw() {
	if (lpData == NULL || nSize < 1)return;
	EnsureBlock(szData + nSize, true);
	memcpy(saCache + szData, lpData, nSize);
	szData			+= nSize;
}
int CTypicalBlock::FillDatas(const void* const* const lpDataPointers, const int* const nDataSizes, const int nDataCount) {
	int				nTotalSize(0);
	for (int iData = 0; iData < nDataCount; iData++)
		nTotalSize	+= nDataSizes[iData];
	EnsureBlock(nTotalSize);
	char*			lpDataOffset(saCache.GetArr());
	for (int iData = 0; iData < nDataCount; iData++) {
		memcpy(lpDataOffset, lpDataPointers[iData], nDataSizes[iData]);
		lpDataOffset+= nDataSizes[iData];
	}
	szData			= nTotalSize;
	return nTotalSize;
}
int CTypicalBlock::GrowSize(int szGrow) {
	if (szData + szGrow < 0) {
		szGrow		= -szData;
	}
	EnsureBlock(szData + szGrow);
	szData			+= szGrow;
	return szData;
}
int CTypicalBlock::GetData(void* const lpCache, const int nMaxSize) {
	int				nCopy(std::min(nMaxSize, szData));
	memcpy(lpCache, saCache, nCopy);
	return nCopy;
}
void CTypicalBlock::Clear() {
	szData			= 0;
}
