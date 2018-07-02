#pragma once
#include <string>

#ifdef _WIN32
#ifdef LIVE_LITERAL_EXPORTS
#define LITERAL_API __declspec(dllexport)
#else
#define LITERAL_API __declspec(dllimport)
#endif
#else 
#define LITERAL_API
#endif

#define SHA_LBLOCK	16
class LITERAL_API CSHA1CodeArith {
public:
	CSHA1CodeArith(void);
	~CSHA1CodeArith(void);

private:
	static std::string sha1(const unsigned char* sha1, int len);

public:
	void	Initialize(void);
	void	UpdateData(const unsigned char* pBuffer, unsigned int uBufferSize);
	void	Finalize(unsigned char sha1code[20]);

private:
	void	SHA1Transform(unsigned char* b);
	void	SHA1Block(unsigned int* W, int num);

private:
	struct SHA1_CTX {
		unsigned int h0, h1, h2, h3, h4;
		unsigned int Nl, Nh;
		unsigned int data[SHA_LBLOCK];
		int num;
	} m_stSHA1Ctx;
};
