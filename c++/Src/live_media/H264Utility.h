#ifndef _DYN_H264_UTILITY_H_
#define _DYN_H264_UTILITY_H_
#include <functional>
#include "Byte.h"

#ifdef _WIN32
#ifdef LIVE_MEDIA_EXPORTS
#define MEDIA_API __declspec(dllexport)
#else
#define MEDIA_API __declspec(dllimport)
#endif
#else 
#define MEDIA_API
#endif

struct h264_sps_t;
class MEDIA_API CH264Utility {
public:
	enum H264NalType{unknal = -1, sei = 6, sps = 7, pps = 8};
	enum H264SliceType{unkslice = -1, islice = 2, pslice = 4, bslice = 8};

public:
	CH264Utility(void);
	virtual ~CH264Utility(void);

public:
	static H264NalType		GetNalType(const unsigned char* const data);
	static int				NextNalStart(const unsigned char* const data, const int size, const int from = 0);

public:
	bool					SetExtraData(const unsigned char* const data, const int size, int* width = NULL, int* height = NULL);
	bool					PickExtraData(const unsigned char* const data, const int size, int* width = NULL, int* height = NULL);
	const unsigned char*    NormalizeH264(const unsigned char* const data, const int size, int& len, bool& key);
	void					NormalizeH264(const unsigned char* const data, const int size, const std::function<void(const unsigned char* const data, const int size, bool key)>& callback, const bool autoAddConfig = true);
	H264SliceType			GetSliceInfo(const unsigned char* const data, const int size, int* framenum = nullptr);
	const unsigned char*	GetSps(int& len);
	const unsigned char*	GetPps(int& len);
	bool					GetImageSize(int* width, int* height);

private:
#pragma warning(push)
#pragma warning(disable: 4251)
	CByte					m_bySps;
	CByte					m_byPps;
	CByte					m_byCache;
	CByte					m_byNal;
	h264_sps_t*				m_spsinfo;
	int						m_nLatestFrameNum;
	bool					m_nLatestIsKey;
#pragma warning(pop)
};

#endif
