#include <stdlib.h>
#include <string.h>
#include "SHA1CodeArith.h"
#include "xstring.h"

#define SHA_CBLOCK	64
#define SHA_BLOCK	16
#define SHA_LAST_BLOCK  56
#define SHA_LENGTH_BLOCK 8
#define SHA_DIGEST_LENGTH 20

#undef c2nl
#define c2nl(c,l)	(l =(((unsigned long)(*((c)++)))<<24), \
	l|=(((unsigned long)(*((c)++)))<<16), \
	l|=(((unsigned long)(*((c)++)))<< 8), \
	l|=(((unsigned long)(*((c)++)))    ))

#undef p_c2nl
#define p_c2nl(c,l,n)	{ \
	switch (n) { \
			case 0: l =((unsigned long)(*((c)++)))<<24; \
			case 1: l|=((unsigned long)(*((c)++)))<<16; \
			case 2: l|=((unsigned long)(*((c)++)))<< 8; \
			case 3: l|=((unsigned long)(*((c)++))); \
	} \
}

#undef c2nl_p
/* NOTE the pointer is not incremented at the end of this */
#define c2nl_p(c,l,n)	{ \
	l=0; \
	(c)+=n; \
	switch (n) { \
			case 3: l =((unsigned long)(*(--(c))))<< 8; \
			case 2: l|=((unsigned long)(*(--(c))))<<16; \
			case 1: l|=((unsigned long)(*(--(c))))<<24; \
	} \
}

#undef p_c2nl_p
#define p_c2nl_p(c,l,sc,len) { \
	switch (sc) \
				{ \
	case 0: l =((unsigned long)(*((c)++)))<<24; \
	if (--len == 0) break; \
			case 1: l|=((unsigned long)(*((c)++)))<<16; \
			if (--len == 0) break; \
			case 2: l|=((unsigned long)(*((c)++)))<< 8; \
				} \
}

#undef nl2c
#define nl2c(l,c)	(*((c)++)=(unsigned char)(((l)>>24)&0xff), \
	*((c)++)=(unsigned char)(((l)>>16)&0xff), \
	*((c)++)=(unsigned char)(((l)>> 8)&0xff), \
	*((c)++)=(unsigned char)(((l)    )&0xff))

#undef c2l
#define c2l(c,l)	(l =(((unsigned long)(*((c)++)))    ), \
	l|=(((unsigned long)(*((c)++)))<< 8), \
	l|=(((unsigned long)(*((c)++)))<<16), \
	l|=(((unsigned long)(*((c)++)))<<24))

#undef p_c2l
#define p_c2l(c,l,n)	{ \
	switch (n) { \
			case 0: l =((unsigned long)(*((c)++))); \
			case 1: l|=((unsigned long)(*((c)++)))<< 8; \
			case 2: l|=((unsigned long)(*((c)++)))<<16; \
			case 3: l|=((unsigned long)(*((c)++)))<<24; \
	} \
}

#undef c2l_p
/* NOTE the pointer is not incremented at the end of this */
#define c2l_p(c,l,n)	{ \
	l=0; \
	(c)+=n; \
	switch (n) { \
			case 3: l =((unsigned long)(*(--(c))))<<16; \
			case 2: l|=((unsigned long)(*(--(c))))<< 8; \
			case 1: l|=((unsigned long)(*(--(c)))); \
	} \
}

#undef p_c2l_p
#define p_c2l_p(c,l,sc,len) { \
	switch (sc) \
				{ \
	case 0: l =((unsigned long)(*((c)++))); \
	if (--len == 0) break; \
			case 1: l|=((unsigned long)(*((c)++)))<< 8; \
			if (--len == 0) break; \
			case 2: l|=((unsigned long)(*((c)++)))<<16; \
				} \
}

#undef l2c
#define l2c(l,c)	(*((c)++)=(unsigned char)(((l)    )&0xff), \
	*((c)++)=(unsigned char)(((l)>> 8)&0xff), \
	*((c)++)=(unsigned char)(((l)>>16)&0xff), \
	*((c)++)=(unsigned char)(((l)>>24)&0xff))

#undef ROTATE
#if defined(WIN32)
#	define ROTATE(a,n)     _lrotl(a,n)
#else
#	define ROTATE(a,n)     (((a)<<(n))|(((a)&0xffffffff)>>(32-(n))))
#endif

/* A nice byte order reversal from Wei Dai <weidai@eskimo.com> */
#if defined(WIN32)
/* 5 instructions with rotate instruction, else 9 */
#	define Endian_Reverse32(a) \
	{ \
	unsigned long l=(a); \
	(a)=((ROTATE(l,8)&0x00FF00FF)|(ROTATE(l,24)&0xFF00FF00)); \
	}
#else
/* 6 instructions with rotate instruction, else 8 */
#	define Endian_Reverse32(a) \
	{ \
	unsigned long l=(a); \
	l=(((l&0xFF00FF00)>>8L)|((l&0x00FF00FF)<<8L)); \
	(a)=ROTATE(l,16L); \
	}
#endif

#define	F_00_19(b,c,d)	((((c) ^ (d)) & (b)) ^ (d)) 
#define	F_20_39(b,c,d)	((b) ^ (c) ^ (d))
#define F_40_59(b,c,d)	(((b) & (c)) | (((b)|(c)) & (d))) 
#define	F_60_79(b,c,d)	F_20_39(b,c,d)

#undef Xupdate
#define Xupdate(a,i,ia,ib,ic,id) (a)=\
	(ia[(i)&0x0f]^ib[((i)+2)&0x0f]^ic[((i)+8)&0x0f]^id[((i)+13)&0x0f]);\
	X[(i)&0x0f]=(a)=ROTATE((a),1);

#define BODY_00_15(i,a,b,c,d,e,f,xa) \
	(f)=xa[i]+(e)+K_00_19+ROTATE((a),5)+F_00_19((b),(c),(d)); \
	(b)=ROTATE((b),30);

#define BODY_16_19(i,a,b,c,d,e,f,xa,xb,xc,xd) \
	Xupdate(f,i,xa,xb,xc,xd); \
	(f)+=(e)+K_00_19+ROTATE((a),5)+F_00_19((b),(c),(d)); \
	(b)=ROTATE((b),30);

#define BODY_20_31(i,a,b,c,d,e,f,xa,xb,xc,xd) \
	Xupdate(f,i,xa,xb,xc,xd); \
	(f)+=(e)+K_20_39+ROTATE((a),5)+F_20_39((b),(c),(d)); \
	(b)=ROTATE((b),30);

#define BODY_32_39(i,a,b,c,d,e,f,xa) \
	Xupdate(f,i,xa,xa,xa,xa); \
	(f)+=(e)+K_20_39+ROTATE((a),5)+F_20_39((b),(c),(d)); \
	(b)=ROTATE((b),30);

#define BODY_40_59(i,a,b,c,d,e,f,xa) \
	Xupdate(f,i,xa,xa,xa,xa); \
	(f)+=(e)+K_40_59+ROTATE((a),5)+F_40_59((b),(c),(d)); \
	(b)=ROTATE((b),30);

#define BODY_60_79(i,a,b,c,d,e,f,xa) \
	Xupdate(f,i,xa,xa,xa,xa); \
	(f)=X[(i)&0x0f]+(e)+K_60_79+ROTATE((a),5)+F_60_79((b),(c),(d)); \
	(b)=ROTATE((b),30);


#define K_00_19	0x5a827999L
#define K_20_39 0x6ed9eba1L
#define K_40_59 0x8f1bbcdcL
#define K_60_79 0xca62c1d6L

#define	M_c2nl 		c2nl
#define	M_p_c2nl	p_c2nl
#define	M_c2nl_p	c2nl_p
#define	M_p_c2nl_p	p_c2nl_p
#define	M_nl2c		nl2c

CSHA1CodeArith::CSHA1CodeArith(void) {
}
CSHA1CodeArith::~CSHA1CodeArith(void) {
}
void CSHA1CodeArith::Initialize() {
	m_stSHA1Ctx.h0	= 0x67452301L;
	m_stSHA1Ctx.h1	= 0xefcdab89L;
	m_stSHA1Ctx.h2	= 0x98badcfeL;
	m_stSHA1Ctx.h3	= 0x10325476L;
	m_stSHA1Ctx.h4	= 0xc3d2e1f0L;
	m_stSHA1Ctx.Nl	= 0;
	m_stSHA1Ctx.Nh	= 0;
	m_stSHA1Ctx.num	= 0;
}
void CSHA1CodeArith::UpdateData(const unsigned char* pBuffer, unsigned int uBufferSize) {
	register unsigned int* p;
	int ew;
	int ec;
	int sw;
	int sc;
	if(uBufferSize == 0) return;
	unsigned int l = (m_stSHA1Ctx.Nl + (uBufferSize << 3)) & 0xffffffffL;
	if(l < m_stSHA1Ctx.Nl) m_stSHA1Ctx.Nh++;
	m_stSHA1Ctx.Nh	+= (uBufferSize>>29);
	m_stSHA1Ctx.Nl	= l;
	if(m_stSHA1Ctx.num != 0) {
		p	= m_stSHA1Ctx.data;
		sw	= m_stSHA1Ctx.num>>2;
		sc	= m_stSHA1Ctx.num&0x03;
		if((m_stSHA1Ctx.num+uBufferSize) >= SHA_CBLOCK) {
			l = p[sw];
			M_p_c2nl(pBuffer,l,sc);
			p[sw++]	= l;
			for(; sw < SHA_LBLOCK; sw++) {
				M_c2nl(pBuffer, l);
				p[sw]	= l;
			}
			uBufferSize -= (SHA_CBLOCK - m_stSHA1Ctx.num);
			SHA1Block(p, 64);
			m_stSHA1Ctx.num	= 0;
			/* drop through and do the rest */
		} else {
			m_stSHA1Ctx.num	+= (int)uBufferSize;
			if((sc+uBufferSize) < 4) { /* ugly, add char's to a word */
				l = p[sw];
				M_p_c2nl_p(pBuffer,l,sc,uBufferSize);
				p[sw] = l;
			} else {
				ew = (m_stSHA1Ctx.num>>2);
				ec = (m_stSHA1Ctx.num&0x03);
				l = p[sw];
				M_p_c2nl(pBuffer,l,sc);
				p[sw++]		= l;
				for(; sw < ew; sw++) { 
					M_c2nl(pBuffer,l); 
					p[sw]	= l; 
				}
				if(ec) {
					M_c2nl_p(pBuffer,l,ec);
					p[sw]	= l;
				}
			}
			return;
		}
	}
	/* we now can process the input pBuffer in blocks of SHA_CBLOCK
	* chars and save the leftovers to m_stSHA1Ctx.pBuffer. */
	p = m_stSHA1Ctx.data;
	while(uBufferSize >= SHA_CBLOCK) {
		for(sw = (SHA_BLOCK / 4); sw; sw--) {
			M_c2nl(pBuffer,l); 
			*(p++)		= l;
			M_c2nl(pBuffer,l); 
			*(p++)		= l;
			M_c2nl(pBuffer,l); 
			*(p++)		= l;
			M_c2nl(pBuffer,l); 
			*(p++)		= l;
		}
		p = m_stSHA1Ctx.data;
		SHA1Block(p, 64);
		uBufferSize -= SHA_CBLOCK;
	}
	ec = (int)uBufferSize;
	m_stSHA1Ctx.num = ec;
	ew = (ec>>2);
	ec &= 0x03;
	for(sw = 0; sw < ew; sw++) { 
		M_c2nl(pBuffer,l); 
		p[sw] = l; 
	}
	M_c2nl_p(pBuffer, l, ec);
	p[sw] = l;
}

void CSHA1CodeArith::SHA1Transform(unsigned char* b) {
	unsigned int p[16];
	unsigned int* q;
	int i;

	q = p;
	for(i = (SHA_LBLOCK / 4); i; i--) {
		unsigned int l;
		c2nl(b,l); *(q++)=l;
		c2nl(b,l); *(q++)=l;
		c2nl(b,l); *(q++)=l;
		c2nl(b,l); *(q++)=l; 
	} 
	SHA1Block(p, 64);
}

void CSHA1CodeArith::SHA1Block(unsigned int* W, int num) {
	register unsigned int A,B,C,D,E,T;
	unsigned int X[16];
	A=m_stSHA1Ctx.h0;
	B=m_stSHA1Ctx.h1;
	C=m_stSHA1Ctx.h2;
	D=m_stSHA1Ctx.h3;
	E=m_stSHA1Ctx.h4;
	for (;;) {
		BODY_00_15( 0,A,B,C,D,E,T,W);
		BODY_00_15( 1,T,A,B,C,D,E,W);
		BODY_00_15( 2,E,T,A,B,C,D,W);
		BODY_00_15( 3,D,E,T,A,B,C,W);
		BODY_00_15( 4,C,D,E,T,A,B,W);
		BODY_00_15( 5,B,C,D,E,T,A,W);
		BODY_00_15( 6,A,B,C,D,E,T,W);
		BODY_00_15( 7,T,A,B,C,D,E,W);
		BODY_00_15( 8,E,T,A,B,C,D,W);
		BODY_00_15( 9,D,E,T,A,B,C,W);
		BODY_00_15(10,C,D,E,T,A,B,W);
		BODY_00_15(11,B,C,D,E,T,A,W);
		BODY_00_15(12,A,B,C,D,E,T,W);
		BODY_00_15(13,T,A,B,C,D,E,W);
		BODY_00_15(14,E,T,A,B,C,D,W);
		BODY_00_15(15,D,E,T,A,B,C,W);
		BODY_16_19(16,C,D,E,T,A,B,W,W,W,W);
		BODY_16_19(17,B,C,D,E,T,A,W,W,W,W);
		BODY_16_19(18,A,B,C,D,E,T,W,W,W,W);
		BODY_16_19(19,T,A,B,C,D,E,W,W,W,X);

		BODY_20_31(20,E,T,A,B,C,D,W,W,W,X);
		BODY_20_31(21,D,E,T,A,B,C,W,W,W,X);
		BODY_20_31(22,C,D,E,T,A,B,W,W,W,X);
		BODY_20_31(23,B,C,D,E,T,A,W,W,W,X);
		BODY_20_31(24,A,B,C,D,E,T,W,W,X,X);
		BODY_20_31(25,T,A,B,C,D,E,W,W,X,X);
		BODY_20_31(26,E,T,A,B,C,D,W,W,X,X);
		BODY_20_31(27,D,E,T,A,B,C,W,W,X,X);
		BODY_20_31(28,C,D,E,T,A,B,W,W,X,X);
		BODY_20_31(29,B,C,D,E,T,A,W,W,X,X);
		BODY_20_31(30,A,B,C,D,E,T,W,X,X,X);
		BODY_20_31(31,T,A,B,C,D,E,W,X,X,X);
		BODY_32_39(32,E,T,A,B,C,D,X);
		BODY_32_39(33,D,E,T,A,B,C,X);
		BODY_32_39(34,C,D,E,T,A,B,X);
		BODY_32_39(35,B,C,D,E,T,A,X);
		BODY_32_39(36,A,B,C,D,E,T,X);
		BODY_32_39(37,T,A,B,C,D,E,X);
		BODY_32_39(38,E,T,A,B,C,D,X);
		BODY_32_39(39,D,E,T,A,B,C,X);

		BODY_40_59(40,C,D,E,T,A,B,X);
		BODY_40_59(41,B,C,D,E,T,A,X);
		BODY_40_59(42,A,B,C,D,E,T,X);
		BODY_40_59(43,T,A,B,C,D,E,X);
		BODY_40_59(44,E,T,A,B,C,D,X);
		BODY_40_59(45,D,E,T,A,B,C,X);
		BODY_40_59(46,C,D,E,T,A,B,X);
		BODY_40_59(47,B,C,D,E,T,A,X);
		BODY_40_59(48,A,B,C,D,E,T,X);
		BODY_40_59(49,T,A,B,C,D,E,X);
		BODY_40_59(50,E,T,A,B,C,D,X);
		BODY_40_59(51,D,E,T,A,B,C,X);
		BODY_40_59(52,C,D,E,T,A,B,X);
		BODY_40_59(53,B,C,D,E,T,A,X);
		BODY_40_59(54,A,B,C,D,E,T,X);
		BODY_40_59(55,T,A,B,C,D,E,X);
		BODY_40_59(56,E,T,A,B,C,D,X);
		BODY_40_59(57,D,E,T,A,B,C,X);
		BODY_40_59(58,C,D,E,T,A,B,X);
		BODY_40_59(59,B,C,D,E,T,A,X);

		BODY_60_79(60,A,B,C,D,E,T,X);
		BODY_60_79(61,T,A,B,C,D,E,X);
		BODY_60_79(62,E,T,A,B,C,D,X);
		BODY_60_79(63,D,E,T,A,B,C,X);
		BODY_60_79(64,C,D,E,T,A,B,X);
		BODY_60_79(65,B,C,D,E,T,A,X);
		BODY_60_79(66,A,B,C,D,E,T,X);
		BODY_60_79(67,T,A,B,C,D,E,X);
		BODY_60_79(68,E,T,A,B,C,D,X);
		BODY_60_79(69,D,E,T,A,B,C,X);
		BODY_60_79(70,C,D,E,T,A,B,X);
		BODY_60_79(71,B,C,D,E,T,A,X);
		BODY_60_79(72,A,B,C,D,E,T,X);
		BODY_60_79(73,T,A,B,C,D,E,X);
		BODY_60_79(74,E,T,A,B,C,D,X);
		BODY_60_79(75,D,E,T,A,B,C,X);
		BODY_60_79(76,C,D,E,T,A,B,X);
		BODY_60_79(77,B,C,D,E,T,A,X);
		BODY_60_79(78,A,B,C,D,E,T,X);
		BODY_60_79(79,T,A,B,C,D,E,X);

		m_stSHA1Ctx.h0=(m_stSHA1Ctx.h0+E)&0xffffffffL; 
		m_stSHA1Ctx.h1=(m_stSHA1Ctx.h1+T)&0xffffffffL;
		m_stSHA1Ctx.h2=(m_stSHA1Ctx.h2+A)&0xffffffffL;
		m_stSHA1Ctx.h3=(m_stSHA1Ctx.h3+B)&0xffffffffL;
		m_stSHA1Ctx.h4=(m_stSHA1Ctx.h4+C)&0xffffffffL;

		num-=64;
		if (num <= 0) break;

		A	= m_stSHA1Ctx.h0;
		B	= m_stSHA1Ctx.h1;
		C	= m_stSHA1Ctx.h2;
		D	= m_stSHA1Ctx.h3;
		E	= m_stSHA1Ctx.h4;

		W	+= 16;
	}
}

void CSHA1CodeArith::Finalize(unsigned char sha1code[20]) {
	register int i;
	register int j;
	register unsigned int l;
	register unsigned int* p;
	unsigned char end[4]	= {0x80, 0x00, 0x00, 0x00};
	unsigned char* cp		= end;

	/* m_stSHA1Ctx.num should definitly have room for at least one more byte. */
	p = m_stSHA1Ctx.data;
	j = m_stSHA1Ctx.num;
	i = j >> 2;
	l = p[i];
	M_p_c2nl(cp, l, j & 0x03);
	p[i] = l;
	i++;
	/* i is the next 'undefined word' */
	if(m_stSHA1Ctx.num >= SHA_LAST_BLOCK) {
		for(; i<SHA_LBLOCK; i++) p[i]	= 0;
		SHA1Block(p, 64);
		i = 0;
	}
	for(; i < (SHA_LBLOCK - 2); i++) p[i] = 0;
	p[SHA_LBLOCK-2]	= m_stSHA1Ctx.Nh;
	p[SHA_LBLOCK-1] = m_stSHA1Ctx.Nl;
	SHA1Block(p, 64);
	cp = sha1code;
	l = m_stSHA1Ctx.h0; nl2c(l, cp);
	l = m_stSHA1Ctx.h1; nl2c(l, cp);
	l = m_stSHA1Ctx.h2; nl2c(l, cp);
	l = m_stSHA1Ctx.h3; nl2c(l, cp);
	l = m_stSHA1Ctx.h4; nl2c(l, cp);

	/* clear stuff, sha1_block may be leaving some stuff on the stack
	* but I'm not worried :-) */
	m_stSHA1Ctx.num	=0;
	memset(&m_stSHA1Ctx, 0, sizeof(m_stSHA1Ctx));
}

std::string CSHA1CodeArith::sha1(const unsigned char* data, int len) {
	CSHA1CodeArith arith;
	unsigned char code[16];
	arith.Initialize();
	arith.UpdateData(data, len);
	arith.Finalize(code);
	xstring<char> result;
	for (int iPos = 0; iPos < 16; iPos++) {
		xstring<char> tmp;
		tmp.AppendFormat("%02x", code[iPos]);
	}
	return result;
}


