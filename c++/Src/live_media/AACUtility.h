#pragma once
#include "SmartNal.h"
#include "SmartArr.h"

#ifdef _WIN32
#ifdef LIVE_MEDIA_EXPORTS
#define MEDIA_API __declspec(dllexport)
#else
#define MEDIA_API __declspec(dllimport)
#endif
#else 
#define MEDIA_API
#endif

class MEDIA_API CAACUtility {
public:
	CAACUtility(void);
	virtual ~CAACUtility(void);

public:
	enum AACType{aacmain = 0, aaclc, aacssr};
	struct AACInfo
	{
		unsigned short	syncword;
		unsigned char	ID;
		unsigned char	layer;
		unsigned char	protection_absent;
		unsigned char	profile;
		unsigned char	sampling_frequency_index;
		unsigned char	private_bit;
		unsigned char	channel_configuration;
		unsigned char	original_copy;
		unsigned char	home;
		unsigned char	copyrightflag;
		unsigned char	copyrightstart;
		unsigned short	framelength;
		unsigned short	adtsbuffer;	
		unsigned char	rawblock;
	};

public:
	static bool		AesConfigToAdts(const unsigned char* const lpConfig, const int nSize, CSmartNal<unsigned char>& snAdtsHeader);
	static bool		IsAdtsFrame(const unsigned char* const lpData, const int nSize);
	static bool		ReadAdtsConfig(const unsigned char* const lpAdts, const int nSize, AACInfo& info);
	static bool		SaveAdtsToAes(const unsigned char* const lpAdts, const int nSize, unsigned char aAes[2]);

public:
	bool			SetConfigure(const unsigned char* const lpConfig, const int nSize);
	bool			GetConfigure(int& nSamplerate, int& nChannel);
	bool			AesFrameToAdtsFrame(const unsigned char* const lpAes, const int szAes, unsigned char*& lpAdts, int& szAdts);

private:
#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	CSmartNal<unsigned char>	m_snAdtsHeader;
	CSmartArr<unsigned char>	m_saAdtsData;
	int							m_szAdtsData;
#ifdef _WIN32
#pragma warning(pop)
#endif
};
