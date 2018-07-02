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

class LITERAL_API CMD5CodeArith {
public:
	CMD5CodeArith(void);
	~CMD5CodeArith(void);

public:
	static std::string md5(const unsigned char* data, int len);

public:
	void Initialize(void);
	void UpdateData(const unsigned char* pInBuffer, unsigned int nBufferSize);
	void Finalize(unsigned char md5code[16]);

private:
	struct MD5_CTX {
		unsigned int state[4]; /* state (ABCD) */
		unsigned int count[2]; /* number of bits, modulo 2^64 (lsb first) */
		unsigned char buffer[64]; /* input buffer */
	} m_stMD5Ctx;

private:
	static void MD5_memcpy(unsigned char* output, unsigned char* input, unsigned int len);
	static void MD5Transform(unsigned int state[4], const unsigned char block[64]);
	static void MD5_memset(unsigned char* output, int value, unsigned int len);
	static void Decode(unsigned int *output, const unsigned char *input, unsigned int len);
	static void Encode(unsigned char *output, const unsigned int *input, unsigned int len);
};
