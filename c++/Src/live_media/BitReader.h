#pragma once

#ifdef _WIN32
#ifdef LIVE_MEDIA_EXPORTS
#define MEDIA_API __declspec(dllexport)
#else
#define MEDIA_API __declspec(dllimport)
#endif
#else 
#define MEDIA_API
#endif

class MEDIA_API CBitReader {
public:
	CBitReader();
	virtual ~CBitReader();

public:
	void			Initialize(const unsigned char* const lpData, const int szData);
	unsigned int	GetBits(int nBitsNum); //nBitsNum >= 0 && <= 32
	void			SkipBits(int nBitsNum); //nBitsNuM >= 0
	int				GetBitsOffset();
	void			AlignSkip();

private:
	const unsigned char*	m_lpBuffer;
	int						m_szBuffer;
	unsigned char*			m_lpSwap;
	unsigned int*			m_pRetrun;
	unsigned char*			m_pSrcByte;
	long long*				m_pMove;

	int						m_nSkipNum;
	int						m_nSrcByteLeftBits;
	int						m_nBitsOffset;
};

