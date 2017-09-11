#include "HttpReader.h"
#include "VpfReader.h"
#include "FileReader.h"
#include "Writelog.h"
#include "SmartRet.h"
#include "xsystem.h"

#define CACHE_SIZE	(1024 * 100)
#define MAX_UINT32	(uint32_t)(~0)

CVpfReader::CVpfReader(bool adjustTC /* = true */) 
	: m_spReader(NULL)
	, m_strUrl()
	, m_bIsHttp(false)
	, m_bAdjustTC(adjustTC)
	, m_bHasError(true)

	, m_lkDatas()
	, m_llIndexes(0LL)
	, m_vIndexes()
	, m_byHeader()
	, m_uBeginTC(0)
	, m_uEndTC(0)
	, m_uReadTC(0)

	, m_llBodies(0LL)
	, m_saCache()
	, m_szRemain(0)
	, m_szBlock(0)
	, m_bEof(false) {
}
CVpfReader::~CVpfReader(void) {
	Close();
}

//////////////////////////////////////////////////////////////////////////
void CVpfReader::OnSetError() {
	wle(m_strDesc);
	m_bHasError = true;
}

//////////////////////////////////////////////////////////////////////////
bool CVpfReader::Open(LPCTSTR url, event_base* base) {
	Close();
	std::lock_guard<std::mutex> lock(m_lkDatas);
	m_saCache = new(std::nothrow) uint8_t[CACHE_SIZE];
	if (m_saCache.GetArr() == NULL)return SetErrorT(false, _T("Alloc cache failed."));
	m_spReader = new(std::nothrow) CHttpReader(base);
	if (m_spReader == nullptr)return SetErrorT(false, _T("Create IOReader failed."));
	if (!m_spReader->Open(url))return SetErrorT(false, _T("IIOReader.Open(%s) failed."), m_strUrl.c_str());
	if (!ReadHeader())return false;
	if (m_spReader->CanSeek() && !ReadIndexes())return false;
	if (m_spReader->CanSeek() && !m_spReader->Seek(m_llBodies))return false;
	m_bHasError = false;
	m_strUrl = url;
	return true;
}
bool CVpfReader::Open(LPCTSTR file) {
	Close();
	std::lock_guard<std::mutex> lock(m_lkDatas);
	m_saCache = new(std::nothrow) uint8_t[CACHE_SIZE];
	if (m_saCache.GetArr() == NULL)return SetErrorT(false, _T("Alloc cache failed."));
	m_spReader = new(std::nothrow) CFileReader();
	if (m_spReader == nullptr)return SetErrorT(false, _T("Create IOReader failed."));
	if (!m_spReader->Open(file))return SetErrorT(false, _T("IIOReader.Open(%s) failed."), m_strUrl.c_str());
	if (!ReadHeader())return false;
	if (m_spReader->CanSeek() && !ReadIndexes())return false;
	if (m_spReader->CanSeek() && !m_spReader->Seek(m_llBodies))return false;
	m_bHasError = false;
	m_strUrl = file;
	return true;
}
bool CVpfReader::IsValid() {
	std::lock_guard<std::mutex> lock(m_lkDatas);
	if (m_bHasError)return false;
	if (m_bEof)return false;
	return true;
}
bool CVpfReader::IsEof() {
	std::lock_guard<std::mutex> lock(m_lkDatas);
	if (m_bHasError)return false;
	return m_bEof;
}
bool CVpfReader::GetInfo(uint32_t& uBeginTC, uint32_t& uEndTC) {
	std::lock_guard<std::mutex> lock(m_lkDatas);
	if (m_bHasError)return false;
	if (m_uBeginTC == MAX_UINT32 && m_uEndTC == MAX_UINT32) {
		uBeginTC = 0;
		uEndTC = -1;
	} else if (m_bAdjustTC)	{
		uBeginTC = 0;
		uEndTC = m_uEndTC - m_uBeginTC;
	} else {
		uBeginTC = m_uBeginTC;
		uEndTC = m_uEndTC;
	}
	return true;
}
bool CVpfReader::GetHeader(uint8_t*& lpHeader, int& szHeader) {
	std::lock_guard<std::mutex> lock(m_lkDatas);
	if (m_bHasError)return false;
	lpHeader = m_byHeader;
	szHeader = m_byHeader;
	return true;
}
bool CVpfReader::ReadBody(uint8_t& byType, uint8_t*& lpData, int& szData, uint32_t& uTimecode) {
	/************************************************************************
	*3字节			块长度（不包含这三个字节）
	*1字节			块类型
	*块数据
	*4字节		时间戳
	*N字节		媒体数据
	*4字节			本块总长度
	************************************************************************/
	std::lock_guard<std::mutex> lock(m_lkDatas);
	if (m_bHasError)return false;
	m_szRemain -= m_szBlock;
	if (m_szRemain > 0 && m_szBlock > 0) {
		memmove(m_saCache, m_saCache + m_szBlock, m_szRemain);
	}
	m_szBlock = 0;
	while (1) {
		if (!ReadBlock())return false;
		int nFirstUINT(*(int*)m_saCache.GetArr());
		const int nSize((nFirstUINT & 0xffffff) + 3);
		const uint8_t bType((nFirstUINT >> 24) & 0xff);
		if (bType >= 128) {
			m_szRemain -= nSize;
			if (m_szRemain > 0)memmove(m_saCache, m_saCache + nSize, m_szRemain);
			continue;
		}
		byType = bType;
		lpData = m_saCache + 8;
		szData = nSize - 8 - 4;
		uTimecode = *(uint32_t*)(m_saCache.GetArr() + 4);
		m_szBlock = nSize;
		if (m_bAdjustTC)uTimecode -= m_uBeginTC;
		m_uReadTC = uTimecode;
		break;
	}
	return true;
}
bool CVpfReader::SeekTo(uint32_t uTimecode) {
	std::lock_guard<std::mutex> lock(m_lkDatas);
	if (m_bHasError)return false;
	if (!m_spReader->CanSeek())return SetErrorT(false, _T("The source not allow seek."));
	if (m_bAdjustTC) uTimecode += m_uBeginTC;
	TCSecond uSecond(uTimecode / 1000 - 1);
	auto result(std::upper_bound(m_vIndexes.begin(), m_vIndexes.end(), uSecond));
	if (result == m_vIndexes.end())return SetErrorT(false, _T("Invalid timecode:%u, should[%u - %u]"), uTimecode, m_uBeginTC, m_uEndTC);
	if (!m_spReader->Seek(result->offset))return SetErrorT(false, _T("Reader seek failed."));
	m_szRemain = 0;
	m_szBlock = 0;
	if (m_bAdjustTC)m_uReadTC = uTimecode - m_uBeginTC;
	else m_uReadTC = uTimecode;
	m_bEof = false;
	m_bHasError = false;
	return true;
}
bool CVpfReader::Tell(uint32_t& uTimecode) {
	std::lock_guard<std::mutex> lock(m_lkDatas);
	if (m_bHasError)return false;
	uTimecode = m_uReadTC;
	return true;
}
void CVpfReader::Close() {
	std::lock_guard<std::mutex> lock(m_lkDatas);
	m_spReader = NULL;
	m_bAdjustTC = true;
	m_bHasError = true;

	m_llIndexes = 0LL;
	m_vIndexes.clear();
	m_byHeader.Clear();
	m_uBeginTC = 0;
	m_uEndTC = 0;
	m_uReadTC = 0;

	m_llBodies = 0LL;
	m_saCache = NULL;
	m_szRemain = 0;
	m_szBlock = 0;
	m_bEof = false;
}

bool CVpfReader::ReadHeader() {
	/************************************************************************
	*文件头
	*8字节			尾部索引偏移量
	*4字节			起始时间戳(ms)
	*4字节			最末时间戳(ms)
	*4字节			头部长度
	*N字节			头部块数据
	************************************************************************/
	if (m_spReader->Offset() != 0 && (!m_spReader->CanSeek() || !m_spReader->Seek(0)))return SetErrorT(false, m_spReader);
	int szRead(0);
	if (!m_spReader->Read(m_saCache, 20, szRead))return SetErrorT(false, _T("Reader read failed."));
	if (szRead != 20)return SetErrorT(false, std::lastError(), _T("Read header failed. szRead=%duint8_ts, excpte:20uint8_ts"), szRead);
	uint8_t* lpData(m_saCache);
	m_llIndexes = *(int64_t*)lpData;
	lpData += 8;					//尾部索引偏移量
	m_uBeginTC = *(uint32_t*)lpData;
	lpData += 4;					//起始时间戳(ms)
	m_uEndTC = *(uint32_t*)lpData;
	lpData += 4;					//最末时间戳(ms)
	int szHeader(*(int*)lpData);//头部长度
	if (!m_byHeader.EnsureSize(szHeader))return SetErrorT(false, _T("Alloc header cache failed."));
	if (!m_spReader->Read(m_byHeader.GetData(), szHeader, szRead))return SetErrorT(false, _T("Reader read failed."));
	if (szRead != szHeader)return SetErrorT(false, _T("Read header failed, szRead=%duint8_ts, expect:%d bytes"), szRead, szHeader);
	m_byHeader.GrowSize(szHeader);
	m_szRemain = 0;
	m_llBodies = szHeader + (8 + 4 + 4 + 4);
	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CVpfReader::ReadIndexes() {
	/************************************************************************
	*尾部
	*3字节			块长度（不包含这三个字节）
	*1字节			块类型(255)
	*尾块数据
	*4字节		第一个时间戳
	*4字节		最后一个时间戳
	*8字节		第一个索引项对应的绝对偏移量
	*索引项列表
	*1字节	秒值跳变值（递增差值）
	*3字节	秒值跳变帧偏移量（递增差值）
	************************************************************************/
	if (!m_spReader->CanSeek())return false;
	if (m_llIndexes == 0)return SetErrorT(false, _T("Invalid indexes offset value"));
	if (!m_spReader->Seek(m_llIndexes))return SetErrorT(false, _T("Reader seek failed."));
	int szRead(0);
	if (!m_spReader->Read(m_saCache, 20, szRead))return SetErrorT(false, m_spReader);
	if (szRead != 20)return SetErrorT(false, std::lastError(), _T("Read header failed. szRead=%duint8_ts, excpte:16u bytes"), szRead);
	uint8_t* lpData(m_saCache);
	int64_t llBaseAddr(0LL);
	int nFirstUINT(*(int*)lpData);
	const int nSize((nFirstUINT & 0xffffff) + 3);
	const uint8_t bType((nFirstUINT >> 24) & 0xff);
	if (bType != 0xff)return SetErrorT(false, _T("Does not found the footer indexes."));
	int nIndexSize(nSize - 1 - 4 - 4 - 8);
	lpData += 4;					//块长度&块类型
	m_uBeginTC = *(uint32_t*)lpData;
	lpData += 4;					//第一个时间戳
	m_uEndTC = *(uint32_t*)lpData;
	lpData += 4;					//最后一个时间戳
	llBaseAddr = *(int64_t*)lpData;	//第一个索引项对应的绝对偏移量
	if (m_uBeginTC == MAX_UINT32)return SetErrorT(false, _T("Invalid begin timecode."));

	int szRemain(0);
	TCSecond nBaseSec((int)(m_uBeginTC / 1000));
	if (m_spReader->Read(m_saCache + szRemain, 1024, szRead)) {
		szRemain += szRead;
	}
	m_vIndexes.clear();
	try {
		m_vIndexes.reserve(nIndexSize / 4 + 10);
	} catch (std::exception& ex) {
		return SetErrorT(false, ex);
	}
	m_vIndexes.push_back(INDEXENTRY(nBaseSec, llBaseAddr));
	while (szRemain >= 4 && nIndexSize >= 4) {
		uint32_t* lpBegin((uint32_t*)m_saCache.GetArr());
		uint32_t* lpEnd(lpBegin + szRemain / 4);
		std::for_each(lpBegin, lpEnd,
			[this, &llBaseAddr, &nBaseSec](uint32_t dw) {
			const uint8_t	bGrow(dw & 0xff);
			const int	nGrow((dw >> 8) & 0xffffff);
			nBaseSec += bGrow;
			llBaseAddr += nGrow;
			m_vIndexes.push_back(CVpfReader::INDEXENTRY(nBaseSec, llBaseAddr));
		});
		int szUsed((szRemain / 4) * 4);
		szRemain -= szUsed;
		nIndexSize -= szUsed;
		if (szRemain >= 0)memcpy(m_saCache, m_saCache + szUsed, szRemain);
		if (m_spReader->Read(m_saCache + szRemain, 1024, szRead)) {
			szRemain += szRead;
		}
	}
	return true;
}

bool CVpfReader::ReadBlock() {
	/************************************************************************
	*3字节			块长度（不包含这三个字节）
	*1字节			块类型
	*块数据
	*4字节		时间戳
	*N字节		媒体数据
	*4字节			本块总长度
	************************************************************************/
	while (m_szRemain < 4) {
		int szRead(0);
		int nWant(std::min(1024 * 2, CACHE_SIZE - m_szRemain));
		if (!m_spReader->Read(m_saCache + m_szRemain, nWant, szRead))return SetErrorT(false, _T("Reader read failed."));
		if (szRead <= 0)return SetErrorT(false, _T("Read data block failed: read size is zero."));
		m_szRemain += szRead;
		if (m_szRemain < 4)continue;
	}

	int nFirstUINT(*(int*)m_saCache.GetArr());
	const int nSize((nFirstUINT & 0xffffff) + 3);
	const uint8_t bType((nFirstUINT >> 24) & 0xff);
	if (bType == 0xff)return raaa(false, m_bEof = true);
	if (nSize > CACHE_SIZE - 100)return SetErrorT(false, std::lastError(), _T("The body block too large: size=%d"), nSize);
	while (m_szRemain < nSize) {
		int szRead(0);
		int nWant(std::min(1024 * 2, CACHE_SIZE - m_szRemain));
		if (!m_spReader->Read(m_saCache + m_szRemain, nWant, szRead))return SetErrorT(false, _T("Reader read failed."));
		if (szRead == 0 && m_spReader->Eof())return raaa(false, m_bEof = true);
		if (szRead <= 0)return SetErrorT(false, _T("Read data block error: read size is zero."));
		m_szRemain += szRead;
		if (m_szRemain < 4)continue;
	}
	return true;
}

IVpfReader* CreateVpfReader(bool adjustTC /*= true*/) {
	return new CVpfReader(adjustTC);
}