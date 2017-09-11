#include "AACUtility.h"
#include "BitReader.h"
#include "BitWriter.h"

CAACUtility::CAACUtility(void)
	: m_snAdtsHeader()
	, m_saAdtsData(nullptr)
	, m_szAdtsData(0)
{
}

CAACUtility::~CAACUtility(void)
{
}

bool CAACUtility::AesConfigToAdts(const unsigned char* const lpConfig, const int nSize, CSmartNal<unsigned char>& snAdtsHeader)
{
	/************************************************************************
		5 bits: object type
		if (object type == 31)
			6 bits + 32: object type
		4 bits: frequency index
		if (frequency index == 15)
			24 bits: frequency
		4 bits: channel configuration
		var bits: AOT Specific Config
	************************************************************************/
	if (nSize < 2)return false;

	enum{AACMain = 1, AACLC, AACSSR, AAC, SBR, Scalable};

	int					nObjectType(0);
	int					nFrequency(0);
	int					nChannel(0);
	nObjectType			= (lpConfig[0] >> 3) & 0x1f;
	//11111000 00000000
	if (nObjectType == 0x1f)
	{
		return false;
		//00000111 11100000
		nObjectType		= ((lpConfig[0] & 0x7) << 3) | ((lpConfig[1] >> 5) & 0x7);
		//00000000 00011110
		nFrequency		= ((lpConfig[1] >> 1) & 0x0f);
		if (nFrequency == 0xf)
		{
			if (nSize < 6)return false;
			//00000000 00000001 11111111 11111111 11111110 00000000
			nFrequency	= *(int*)(lpConfig + 2);
			nFrequency	= nFrequency >> 1;
			nFrequency	= nFrequency | (((int)lpConfig[1] & 0x01) << 31);
			//00000000 00000000 00000000 00000000 00000001 11100000
			nChannel	= ((lpConfig[4] & 0x01) << 3) | ((lpConfig[5] >> 5) & 0x7);
		}
		else
		{
			if (nSize < 3)return false;
			//00000000 00000001 11100000
			nChannel	= ((lpConfig[1] & 0x01) << 3) | ((lpConfig[2] >> 5) & 0x7);
		}
	}
	else
	{
		//00000111 10000000
		nFrequency	= (((lpConfig[0] & 0x7) << 1) | ((lpConfig[1] >> 7) & 0x01));
		if (nFrequency == 0xf)
		{
			return false;
			if (nSize < 5)return false;
			//00000000 01111111 11111111 11111111 11111111 10000000
			nFrequency	= *(int*)(lpConfig + 1);
			nFrequency	= nFrequency << 1;
			nFrequency	= nFrequency | ((lpConfig[5] >> 7) & 0x01);
			//00000000 00000000 00000000 00000000 00000000 01111000
			nChannel	= ((lpConfig[5] >> 3) & 0xf);
		}
		else
		{
			//00000000 01111000
			nChannel	= ((lpConfig[1] >> 3) & 0xf);
		}
	}
	if (nObjectType == AACLC)
		nObjectType		= aaclc;
	else if (nObjectType == AACSSR)
		nObjectType		= aacssr;
	else
		nObjectType		= aacmain;

	/************************************************************************
	struct CADTSHeader
	{
		int		syncword:12;		[1]11111111 [2]11110000
		int 	id:1;				[2]00001000
		int 	layer:2;			[2]00000110
		int 	absent:1;			[2]00000001
		int 	profile:2;			[3]11000000
		int 	frequency:4;		[3]00111100
		int		privatebit:1;		[3]00000010
		int		channel:3;			[3]00000001 [4]11000000
		int		original:1;			[4]00100000
		int		home:1;				[4]00010000
		int		copyrightflag:1;	[4]00001000
		int		copyrightstart:1;	[4]00000100
		int		framelength:13;		[4]00000011 [5]11111111 [6]11100000
		int		adtsbuffer:11;		[6]00011111 [7]11111100
		int		rawblock:2;			[7]00000011
	};
	************************************************************************/
	int					nLength(0);
	int					nDataBlock(nLength / 1024);
	unsigned char*		lpHeader(snAdtsHeader.Allocate(7));
	lpHeader[0]			= 0xFF;							//syncword:8
	lpHeader[1]			= 0xF1;							//syncword:4	id:1	layer:2		absent:1
	lpHeader[2]			= nObjectType << 6;				//profile:2
	lpHeader[2]			|= ((nFrequency & 0xf) << 2);	//frequency:4
	lpHeader[2]			|= (nChannel & 0x4) >> 2;		//channel:3	00000001
	lpHeader[3]			= (nChannel & 0x3) << 6;		//channel:3 11000000
	lpHeader[3]			|= (nLength & 0x1800) >> 11;	//framelength:13 00000011
	lpHeader[4]			= (nLength & 0x1FF8) >> 3;		//framelength:13 11111111
	lpHeader[5]			= (nLength & 0x7) << 5;			//framelength:13 11100000
	lpHeader[5]			|= 0x1F;						//adtsbuffer:11 00011111
	lpHeader[6]			= 0xFC;							//adtsbuffer:11 11111100
	lpHeader[6]			|= nDataBlock & 0x03;			//rawblock:2

	return true;
}

bool CAACUtility::ReadAdtsConfig(const unsigned char* const lpAdts, const int nSize, AACInfo& info)
{
	if (nSize < 7)return false;

	CBitReader						br;
	br.Initialize(lpAdts, nSize);
	info.syncword					= (unsigned short)br.GetBits(12);
	info.ID							= (unsigned char)br.GetBits(1);
	info.layer						= (unsigned char)br.GetBits(2);
	info.protection_absent			= (unsigned char)br.GetBits(1);
	info.profile					= (unsigned char)br.GetBits(2);
	info.sampling_frequency_index	= (unsigned char)br.GetBits(4);
	info.private_bit				= (unsigned char)br.GetBits(1);
	info.channel_configuration		= (unsigned char)br.GetBits(3);
	info.original_copy				= (unsigned char)br.GetBits(1);
	info.home						= (unsigned char)br.GetBits(1);
	info.copyrightflag				= (unsigned char)br.GetBits(1);
	info.copyrightstart				= (unsigned char)br.GetBits(1);
	info.framelength				= (unsigned short)br.GetBits(13);
	info.adtsbuffer					= (unsigned short)br.GetBits(11);
	info.rawblock					= (unsigned char)br.GetBits(2);

	return true;
}

bool CAACUtility::SaveAdtsToAes(const unsigned char* const lpAdts, const int nSize, unsigned char aAes[2])
{
	AACInfo							info;
	if (!ReadAdtsConfig(lpAdts, nSize, info))return false;
	aAes[0]							= 0;
	aAes[1]							= 0;
	aAes[0]							|= (info.profile << 3) & 0xf8;					//1111 1000 0000 0000
	aAes[0]							|= (info.sampling_frequency_index >> 1) & 0x07;	//0000 0111 1000 0000
	aAes[1]							|= (info.sampling_frequency_index << 7) & 0x80; //0000 0111 1000 0000
	aAes[1]							|= (info.channel_configuration << 3) & 0x78;	//0000 0000 0111 1000
	return true;
}

bool CAACUtility::IsAdtsFrame(const unsigned char* const lpData, const int nSize)
{
	if (nSize < 7)return false;
	if (((*lpData) & 0xf0ff) == 0xf0ff)return true;
	else return false;
}

bool CAACUtility::SetConfigure(const unsigned char* const lpConfig, const int nSize)
{
	m_snAdtsHeader		= nullptr;
	if (!AesConfigToAdts(lpConfig, nSize, m_snAdtsHeader))return false;
	return true;
}

bool CAACUtility::GetConfigure(int& nSamplerate, int& nChannel)
{
	if (m_snAdtsHeader == nullptr)return false;
	const static int	aSamplerate[] ={96000, 88200, 64000, 48000, 44100, 32000,
		24000, 22050, 16000, 12000, 11025, 8000, 7350};
	int					nIndex((m_snAdtsHeader[2] >> 2) & 0xf);
	if (nIndex < 0 || nIndex >= (int)(sizeof(aSamplerate) / sizeof(aSamplerate[0])))return false;
	nSamplerate			= aSamplerate[nIndex];
	nChannel			= (m_snAdtsHeader[2] & 0x1) << 2;
	nChannel			|= ((m_snAdtsHeader[3] >> 6) & 0x3);
	return true;
}

bool CAACUtility::AesFrameToAdtsFrame(const unsigned char* const lpAes, const int szAes, unsigned char*& lpAdts, int& szAdts)
{
	if (IsAdtsFrame(lpAes, szAes))return true;
	if (m_snAdtsHeader == nullptr)return false;

	int					nLength(szAes + m_snAdtsHeader.GetSize());
	int					nDataBlock(nLength / 1024);
	if (m_szAdtsData < nLength || m_saAdtsData.GetArr() == nullptr)
	{
		m_szAdtsData	= nLength * 2;
		m_saAdtsData	= new unsigned char[m_szAdtsData];
	}

	memcpy(m_saAdtsData, m_snAdtsHeader, m_snAdtsHeader.GetSize());
	m_saAdtsData[3]		|= (nLength & 0x1800) >> 11;	//framelength:13 00000011
	m_saAdtsData[4]		= (nLength & 0x1FF8) >> 3;		//framelength:13 11111111
	m_saAdtsData[5]		= (nLength & 0x7) << 5;			//framelength:13 11100000
	m_saAdtsData[6]		|= nDataBlock & 0x03;			//rawblock:2
	memcpy(m_saAdtsData + m_snAdtsHeader.GetSize(), lpAes, szAes);
	lpAdts				= m_saAdtsData;
	szAdts				= nLength;
	return true;
}
