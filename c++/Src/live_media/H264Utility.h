#ifndef _DYN_H264_UTILITY_H_
#define _DYN_H264_UTILITY_H_
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
	const unsigned char*    NormalizeH264(const unsigned char* const data, const int size, int& len, bool& key);
	bool					PickExtraData(const unsigned char* const data, const int size);
	H264SliceType			GetSliceInfo(const unsigned char* const data, const int size);

private:
	CByte					m_bySps;
	CByte					m_byPps;
	CByte					m_byCache;
	h264_sps_t*				m_spsinfo;
};

#endif
