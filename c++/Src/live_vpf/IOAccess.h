#pragma once
#include "xchar.h"
#include "SmartErr.h"
#include "os.h"
#include "ierror.h"

#ifdef _WIN32
#ifdef LIVE_VPF_EXPORTS
#define VPF_API __declspec(dllexport)
#else
#define VPF_API __declspec(dllimport)
#endif
#else 
#define VPF_API
#endif

interface VPF_API IIOReader : public IError {
	virtual ~IIOReader() {}
	virtual bool Open(LPCTSTR file) = 0;
	virtual bool CanSeek() = 0;
	virtual bool Read(void* cache, const int limit, int& read) = 0;
	virtual bool Seek(const int64_t offset) = 0;
	virtual int64_t Offset() = 0;
	virtual bool Eof() = 0;
	virtual void Close() = 0;
};

struct event_base;
interface VPF_API IVpfReader : public IError {
	virtual ~IVpfReader() {}
	virtual bool Open(LPCTSTR url, event_base* base) = 0;
	virtual bool Open(LPCTSTR file) = 0;
	virtual bool IsValid() = 0;
	virtual bool IsEof() = 0;
	virtual bool GetInfo(uint32_t& uBeginTC, uint32_t& uEndTC) = 0;
	virtual bool GetHeader(uint8_t*& lpHeader, int& szHeader) = 0;
	virtual bool ReadBody(uint8_t& byType, uint8_t*& lpData, int& szData, uint32_t& uTimecode) = 0;
	virtual bool SeekTo(uint32_t uTimecode) = 0;
	virtual bool Tell(uint32_t& uTimecode) = 0;
	virtual void Close() = 0;
};

interface VPF_API IVpfWriter : public IError {
	virtual ~IVpfWriter() {}
	virtual bool OpenFile(LPCTSTR file, const uint64_t& utcBegin = ~0) = 0;
	virtual bool WriteHead(const uint8_t* const data, const int size) = 0;
	virtual bool WriteData(const uint8_t type, const uint8_t* const data, const int size, const uint32_t timecode = ~0) = 0;
	virtual uint32_t GetLength() const = 0;
	virtual void CloseFile(bool* isValid = NULL) = 0;
	virtual LPCTSTR GetInfo(uint64_t& utcBegin, uint64_t& utcEnd) = 0;
};

VPF_API IIOReader* CreateFileReader();
VPF_API IVpfReader* CreateVpfReader(bool adjustTC = true);
VPF_API IVpfWriter* CreateVpfWriter();
