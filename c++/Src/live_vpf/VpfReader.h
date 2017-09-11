#pragma once
#include <vector>
#include <mutex>
#include "os.h"
#include "IOAccess.h"
#include "xstring.h"
#include "SmartPtr.h"
#include "SmartArr.h"
#include "Byte.h"
#include "SmartErr.h"

/************************************************************************
*文件头
*8字节			尾部索引偏移量
*4字节			起始时间戳(ms)
*4字节			最末时间戳(ms)
*4字节			头部长度（不包含这四个字节，通过此字节可以跳转到真正的数据块）
*N字节			头部块数据
*数据块
*3字节			块长度（不包含这三个字节）
*1字节			块类型
*<128		用户定义类型
*>=128		系统定义数据
*系统数据
*255		下一个秒值跳变帧偏移量（12字节）
*254		下一个10秒值跳变帧偏移量（12字节）
*4字节	上一个跳变偏移量（相对本块头部）
*4字节	下一个时间戳
*4字节	下一个跳变偏移量（相对本块头部）
*用户数据
*4字节		时间戳
*N字节		媒体数据
*4字节			本块总长度（包含这四个字节）
*	*	*	*	*	*	*	*	*	*
*尾部
*4字节			第一个时间戳
*4字节			最后一个时间戳
*8字节			第一个索引项对应的绝对偏移量
*索引项列表
*1字节		秒值跳变值（递增差值）
*3字节		秒值跳变帧偏移量（递增差值）
************************************************************************/
struct event_base;
class CVpfReader : public IVpfReader, public CSmartErr {
public:
	CVpfReader(bool adjustTC = true);
	~CVpfReader(void);

public:
	bool Open(LPCTSTR url, event_base* base);
	bool Open(LPCTSTR file);
	bool IsValid();
	bool IsEof();
	bool GetInfo(uint32_t& uBeginTC, uint32_t& uEndTC);
	bool GetHeader(uint8_t*& lpHeader, int& szHeader);
	bool ReadBody(uint8_t& byType, uint8_t*& lpData, int& szData, uint32_t& uTimecode);
	bool SeekTo(uint32_t uTimecode);
	bool Tell(uint32_t& uTimecode);
	void Close();

protected:
	virtual void OnSetError();
	virtual LPCTSTR	GetLastErrorDesc() const { return m_strDesc; }
	virtual unsigned int GetLastErrorCode() const { return m_dwCode; }

protected:
	bool OpenReader(bool adjustTC);
	bool ReadHeader();
	bool ReadIndexes();
	bool ReadBlock();

protected:
	typedef int TCSecond;
	struct INDEXENTRY {
		INDEXENTRY(TCSecond s, int64_t o) : timecode(s), offset(o) {}
		friend bool operator<(const INDEXENTRY& left, const TCSecond& right) { return left.timecode < right; }
		friend bool operator<(const TCSecond& left, const INDEXENTRY& right) { return left < right.timecode; }
		TCSecond timecode;
		int64_t offset;
	};
	typedef std::vector<INDEXENTRY> CIndexEntry;

private:
	CSmartPtr<IIOReader> m_spReader;
	xtstring m_strUrl;
	const bool m_bIsHttp;
	bool m_bAdjustTC;
	bool m_bHasError;

	std::mutex m_lkDatas;
	int64_t m_llIndexes;
	CIndexEntry m_vIndexes;
	CByte m_byHeader;
	uint32_t m_uBeginTC;
	uint32_t m_uEndTC;
	uint32_t m_uReadTC;

	uint64_t m_llBodies;
	CSmartArr<uint8_t> m_saCache;
	int m_szRemain;
	int m_szBlock;
	bool m_bEof;
};

