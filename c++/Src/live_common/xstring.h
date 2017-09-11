#pragma once
#include <string>
#include <locale>
#include <stdarg.h>
#include <algorithm>
#include "xchar.h"
#if defined(_WIN32)
#include <comutil.h>
#elif !defined(NO_ICONV)
#include <iconv.h>
#endif

#define STD_BUF_SIZE		1024

template<typename CT>
class xstring : public std::basic_string<CT> {

	typedef typename std::basic_string<CT>		MYBASE;
	typedef typename MYBASE::const_pointer		const_pointer;
	typedef typename MYBASE::pointer			pointer;
	typedef typename MYBASE::iterator			iterator;
	typedef typename MYBASE::const_iterator		const_iterator;
	typedef typename MYBASE::reverse_iterator	reverse_iterator;
	typedef typename MYBASE::size_type			size_type;
	typedef typename MYBASE::value_type			value_type;
	typedef typename MYBASE::allocator_type		allocator_type;

	typedef const TCHAR*						PCTSTR;
	typedef const OLECHAR*						PCOLESTR;
	typedef OLECHAR*							POLESTR;

public:
#ifdef _WIN32
#define CodePage		int
#define CodePage_UTF8	CP_UTF8
#define CodePage_GB2312	936
#define CodePage_BIG5	950
#else 
#define CodePage		const char*
#define CodePage_UTF8	"utf-8"
#define CodePage_GB2312	"gb2312"
#define CodePage_BIG5	"big5"
#endif

public:
	template <typename CP>
	static std::basic_string<CT> format(const CP* szFmt, ...) {
		if (szFmt == NULL)return NULL;
		xstring<CT>		str;
		va_list			argList;
		va_start(argList, szFmt);
		str.FormatV(szFmt, argList);
		va_end(argList);
		return str;
	}
	template <typename CP>
	static std::basic_string<CT> format(const CP* szFmt, va_list argList) {
		if (szFmt == NULL)return NULL;
		xstring<CT>		str;
		str.FormatV(szFmt, argList);
		return str;
	}
	template <typename P>
	static std::basic_string<CT> convert(const P* src, size_t len = 0, CodePage cp = CodePage_UTF8) {
		std::basic_string<CT>	dst;
		return dup(dst, src, len, cp);
	}
	template <typename P>
	static std::basic_string<CT> convert(const std::basic_string<P>& src, CodePage cp = CodePage_UTF8) {
		std::basic_string<CT>	dst;
		return dup(dst, src.c_str(), src.length(), cp);
	}
	template <typename P, typename R>
	static std::basic_string<R> convert(const P* src, size_t len = 0, CodePage cp = CodePage_UTF8) {
		std::basic_string<R>	dst;
		return dup(dst, src, len, cp);
	}
	template <typename P, typename R>
	static std::basic_string<R> convert(const std::basic_string<P>& src, CodePage cp = CodePage_UTF8) {
		std::basic_string<R>	dst;
		return dup(dst, src.c_str(), src.length(), cp);
	}

public:
	xstring() {
	}
	template <typename CP>
	xstring(const xstring<CP>& str)
		: std::basic_string<CT>(convert<CP, CT>(str)) {
	}
	xstring(const xstring<CT>& str)
		: std::basic_string<CT>(str) {
	}
	template <typename CP>
	xstring(const std::basic_string<CP>& str)
		: std::basic_string<CT>(convert<CP, CT>(str)) {
	}
	xstring(const std::basic_string<CT>& str)
		: std::basic_string<CT>(str) {
	}
	template <typename CP>
	xstring(const CP* src, const size_t len = 0)
		: std::basic_string<CT>(convert<CP, CT>(src, len)) {
	}
	xstring(std::string::const_iterator first, std::string::const_iterator last)
		: std::basic_string<CT>(convert<char, CT>(&*first, (int)(last - first))) {
	}
	xstring(std::wstring::const_iterator first, std::wstring::const_iterator last)
		: std::basic_string<CT>(convert<wchar_t, CT>(&*first, (int)(last - first))) {
	}
	xstring(size_type size, value_type val, const allocator_type& al = allocator_type())
		: std::basic_string<CT>(size, val, al) {
	}

public:
	xstring<CT>& operator=(const xstring<CT>& src) {
		std::basic_string<CT>::operator =(src);
		return *this;
	}
	template <typename CP>
	xstring<CT>& operator=(const std::basic_string<CP>& src) {
		if ((void*)src.c_str() == (void*)this->c_str())
			return *this;
		dup(*this, src.c_str(), src.length());
		return *this;
	}
	template <typename CP>
	xstring<CT>& operator=(const CP* src) {
		dup(*this, src);
		return *this;
	}
	xstring<CT>& operator=(CT t) {
		this->assign(1, t);
		return *this;
	}

public:
	template <typename CP>
	xstring<CT>& operator+=(const xstring<CP>& str) {
		const xstring<CT>	src(str);
		std::basic_string<CT>::operator +=(src);
		return *this;
	}
	xstring<CT>& operator+=(const xstring<CT>& str) {
		std::basic_string<CT>::operator +=(str);
		return *this;
	}
	template <typename CP>
	xstring<CT>& operator+=(const std::basic_string<CP>& str) {
		const xstring<CT>	src(str);
		std::basic_string<CT>::operator +=(src);
		return *this;
	}
	xstring<CT>& operator+=(const std::basic_string<CT>& str) {
		std::basic_string<CT>::operator +=(str);
		return *this;
	}
	template <typename CP>
	xstring<CT>& operator+=(const CP* str) {
		const xstring<CT>	src(str);
		std::basic_string<CT>::operator +=(src);
		return *this;
	}
	xstring<CT>& operator+=(const CT* str) {
		std::basic_string<CT>::operator +=(str);
		return *this;
	}
	xstring<CT>& operator+=(CT ch) {
		this->append(1, ch);
		return *this;
	}

public:
	template <typename CP>
	xstring<CT> operator+(const xstring<CP>& str) {
		xstring<CT> dst(*this);
		dst += str;
		return dst;
	}
	template <typename CP>
	xstring<CT> operator+(const std::basic_string<CP>& str) {
		xstring<CT> dst(*this);
		dst += str;
		return dst;
	}
	template <typename CP>
	xstring<CT> operator+(const CP* str) {
		xstring<CT> dst(*this);
		dst += str;
		return dst;
	}

public:
	std::string toString(CodePage cp = CodePage_UTF8) const {
		std::string res;
		dup(res, cp, this->c_str(), this->length());
		return res;
	}
	std::wstring towString() const {
		xstring<wchar_t>	str(*this);
		return str;
	}

public:
	template <typename CP>
	void Format(const CP* szFmt, ...) {
		if (szFmt == NULL)return;
		va_list argList;
		va_start(argList, szFmt);
		FormatV(szFmt, argList);
		va_end(argList);
	}
	void FormatV(const char* szFormat, va_list argList) {
		if (szFormat == NULL)return;
		xstring<char> str;
		int nLen(len(szFormat) + STD_BUF_SIZE);
		vsnprintf_s(str.GetBuffer(nLen), nLen - 1, nLen - 1, szFormat, argList);
		str.ReleaseBuffer();
		*this = str;
	}
	void FormatV(const wchar_t* szFormat, va_list argList) {
		if (szFormat == NULL)return;
		xstring<wchar_t>str;
		int nLen(len(szFormat) + STD_BUF_SIZE);
		_vsnwprintf_s(str.GetBuffer(nLen), nLen - 1, nLen - 1, szFormat, argList);
		str.ReleaseBuffer();
		*this = str;
	}

public:
	void Append(const CT* szThat) {
		if (szThat == NULL)return;
		this->append(szThat);
	}
	template <typename CP>
	void AppendFormat(const CP* szFmt, ...) {
		if (szFmt == NULL)return;
		va_list argList;
		va_start(argList, szFmt);
		AppendFormatV(szFmt, argList);
		va_end(argList);
	}
	void AppendFormatV(const char* szFmt, va_list argList) {
		if (szFmt == NULL)return;
		CT szBuf[STD_BUF_SIZE];
		int nLen(vsnprintf_s(szBuf, STD_BUF_SIZE - 1, STD_BUF_SIZE - 1, szFmt, argList));
#if defined(UNICODE) || defined(_UNICODE)
		if (0 < nLen)this->append(convert(szBuf, nLen));
#else 
		if (0 < nLen)this->append(szBuf, nLen);
#endif
	}
	void AppendFormatV(const wchar_t* szFmt, va_list argList) {
		if (szFmt == NULL)return;
		wchar_t szBuf[STD_BUF_SIZE];
		int nLen(_vsnwprintf_s(szBuf, STD_BUF_SIZE - 1, STD_BUF_SIZE - 1, szFmt, argList));
#if defined(UNICODE) || defined(_UNICODE)
		if (0 < nLen)this->append(szBuf, nLen);
#else 
		if (0 < nLen)this->append(convert(szBuf, nLen));
#endif
	}
	int Compare(const_pointer szThat) const {
		return _tcscmp(this->c_str(), szThat);
	}
	int CompareNoCase(const_pointer szThat) const {
		return _tcsicmp(this->c_str(), szThat);
	}
	int Delete(int nIdx, int nCount = 1) {
		if (nIdx < 0) nIdx = 0;
		if (nIdx < this->GetLength())
			this->erase(static_cast<size_type>(nIdx), static_cast<size_type>(nCount));
		return GetLength();
	}
	void Empty() {
		this->erase();
	}
	int Find(CT ch) const {
		size_type nIdx(this->find_first_of(ch));
		return static_cast<int>(std::basic_string<CT>::npos == nIdx ? -1 : nIdx);
	}
	int Find(const_pointer szSub) const {
		size_type nIdx(this->find(szSub));
		return static_cast<int>(std::basic_string<CT>::npos == nIdx ? -1 : nIdx);
	}
	int Find(CT ch, int nStart) const {
		size_type nIdx(this->find_first_of(ch, static_cast<size_type>(nStart)));
		return static_cast<int>(std::basic_string<CT>::npos == nIdx ? -1 : nIdx);
	}
	int Find(const_pointer szSub, int nStart) const {
		size_type nIdx(this->find(szSub, static_cast<size_type>(nStart)));
		return static_cast<int>(std::basic_string<CT>::npos == nIdx ? -1 : nIdx);
	}
	int FindOneOf(const_pointer szCharSet) const {
		size_type nIdx(this->find_first_of(szCharSet));
		return static_cast<int>(std::basic_string<CT>::npos == nIdx ? -1 : nIdx);
	}
	int GetAllocLength() {
		return static_cast<int>(this->capacity());
	}
	CT GetAt(int nIdx) const {
		return this->at(static_cast<size_type>(nIdx));
	}
	CT* GetBuffer(int nMinLen = -1) {
		if (static_cast<int>(this->size()) < nMinLen)
			this->resize(static_cast<size_type>(nMinLen));
		return this->empty() ? const_cast<CT*>(this->data()) : &(this->at(0));
	}
	CT* GetBufferSetLength(int nLen) {
		nLen = (nLen > 0 ? nLen : 0);
		if (this->capacity() < 1 && nLen == 0)
			this->resize(1);
		this->resize(static_cast<size_type>(nLen));
		return const_cast<CT*>(this->data());
	}
	int GetLength() const {
		return static_cast<int>(this->length());
	}
	const_pointer GetString() const {
		return this->c_str();
	}
	int Insert(int nIdx, CT ch) {
		if (static_cast<size_type>(nIdx) > this->size() - 1)
			this->append(1, ch);
		else
			this->insert(static_cast<size_type>(nIdx), 1, ch);
		return GetLength();
	}
	int Insert(int nIdx, const_pointer sz) {
		if (static_cast<size_type>(nIdx) >= this->size())
			this->append(sz, static_cast<size_type>(len(sz)));
		else
			this->insert(static_cast<size_type>(nIdx), sz);
		return GetLength();
	}
	bool IsEmpty() const {
		return this->empty();
	}
	xstring<CT> Left(int nCount) const {
		nCount = std::max(0, std::min(nCount, static_cast<int>(this->size())));
		return this->substr(0, static_cast<size_type>(nCount));
	}
	xstring<CT>& MakeLower() {
		for (auto it = this->begin(); it != this->end(); it++) {
			*it = std::tolower(*it, std::locale());
		}
		return *this;
	}
	void MakeReverse() {
		std::reverse(this->begin(), this->end());
	}
	xstring<CT>& MakeUpper() {
		for (auto it = this->begin(); it != this->end(); it++) {
			*it = std::toupper(*it, std::locale());
		}
		return *this;
	}
	xstring<CT> Mid(int nFirst) const {
		return Mid(nFirst, this->GetLength() - nFirst);
	}
	xstring<CT> Mid(int nFirst, int nCount) const {
		if (nFirst < 0) nFirst = 0;
		if (nCount < 0) nCount = 0;
		int nSize(static_cast<int>(this->size()));
		if (nFirst + nCount > nSize)
			nCount = nSize - nFirst;
		if (nFirst > nSize)return xstring<CT>();

		ASSERT(nFirst >= 0);
		ASSERT(nFirst + nCount <= nSize);
		return this->substr(static_cast<size_type>(nFirst), static_cast<size_type>(nCount));
	}
	void ReleaseBuffer(int nNewLen = -1) {
		this->resize(static_cast<size_type>(nNewLen > -1 ? nNewLen : len(this->c_str())));
	}
	int Remove(CT ch) {
		size_type nIdx(0);
		int nRemoved(0);
		while ((nIdx = this->find_first_of(ch)) != std::basic_string<CT>::npos) {
			this->erase(nIdx, 1);
			nRemoved++;
		}
		return nRemoved;
	}
	int Replace(CT chOld, CT chNew) {
		int nReplaced(0);
		for (iterator iter = this->begin(); iter != this->end(); iter++) {
			if (*iter == chOld) {
				*iter = chNew;
				nReplaced++;
			}
		}
		return nReplaced;
	}
	int Replace(const_pointer szOld, const_pointer szNew) {
		int nReplaced(0);
		size_type nIdx(0);
		size_type nOldLen(len(szOld));
		if (0 != nOldLen) {
			size_type nNewLen(len(szNew));
			if (nNewLen > nOldLen) {
				int nFound(0);
				while (nIdx < this->length() && (nIdx = this->find(szOld, nIdx)) != std::basic_string<CT>::npos) {
					nFound++;
					nIdx += nOldLen;
				}
				this->reserve(this->size() + nFound * (nNewLen - nOldLen));
			}
			static const CT ch(CT(0));
			const_pointer szRealNew(szNew == 0 ? &ch : szNew);
			nIdx = 0;
			while (nIdx < this->length() && (nIdx = this->find(szOld, nIdx)) != std::basic_string<CT>::npos) {
				this->replace(this->begin() + nIdx, this->begin() + nIdx + nOldLen, szRealNew);
				nReplaced++;
				nIdx += nNewLen;
			}
		}
		return nReplaced;
	}
	int ReverseFind(CT ch) const {
		size_type nIdx(this->find_last_of(ch));
		return static_cast<int>(std::basic_string<CT>::npos == nIdx ? -1 : nIdx);
	}
	int ReverseFind(const_pointer szFind, size_type pos = std::basic_string<CT>::npos) const {
		size_type nIdx(this->rfind(0 == szFind ? xstring<CT>() : szFind, pos));
		return static_cast<int>(std::basic_string<CT>::npos == nIdx ? -1 : nIdx);
	}
	xstring<CT> Right(int nCount) const {
		nCount = std::max(0, std::min(nCount, static_cast<int>(this->size())));
		return this->substr(this->size() - static_cast<size_type>(nCount));
	}
	void SetAt(int nIndex, CT ch) {
		ASSERT(this->size() > static_cast<size_type>(nIndex));
		this->at(static_cast<size_type>(nIndex)) = ch;
	}
	xstring<CT> SpanExcluding(const_pointer szCharSet) const {
		size_type pos(this->find_first_of(szCharSet));
		return pos == std::basic_string<CT>::npos ? *this : Left(static_cast<int>(pos));
	}
	xstring<CT> SpanIncluding(const_pointer szCharSet) const {
		size_type pos(this->find_first_not_of(szCharSet));
		return pos == std::basic_string<CT>::npos ? *this : Left(static_cast<int>(pos));
	}
	xstring<CT>& Trim() {
		return TrimLeft().TrimRight();
	}
	xstring<CT>& Trim(const_pointer szTrimChars) {
		return TrimLeft(szTrimChars).TrimRight(szTrimChars);
	}
	xstring<CT>& TrimLeft() {
		this->erase(this->begin(), std::find_if(this->begin(), this->end(),
			[](CT t) {
			return !std::isspace(t, std::locale());
		}));
		return *this;
	}
	xstring<CT>& Normalize() {
		return Trim().ToLower();
	}
	xstring<CT>& TrimLeft(CT tTrim) {
		this->erase(0, this->find_first_not_of(tTrim));
		return *this;
	}
	xstring<CT>& TrimLeft(const_pointer szTrimChars) {
		this->erase(0, this->find_first_not_of(szTrimChars));
		return *this;
	}
	xstring<CT>& TrimRight() {
		reverse_iterator it(std::find_if(this->rbegin(), this->rend(),
			[](CT t) {
			return !std::isspace(t, std::locale());
		}));
		if (!(this->rend() == it)) this->erase(this->rend() - it);
		this->erase(!(it == this->rend()) ? this->find_last_of(*it) + 1 : 0);
		return *this;
	}
	xstring<CT>& TrimRight(CT tTrim) {
		size_type nIdx(this->find_last_not_of(tTrim));
		this->erase(std::basic_string<CT>::npos == nIdx ? 0 : ++nIdx);
		return *this;
	}
	xstring<CT>& TrimRight(const_pointer szTrimChars) {
		size_type nIdx(this->find_last_not_of(szTrimChars));
		this->erase(std::basic_string<CT>::npos == nIdx ? 0 : ++nIdx);
		return *this;
	}
	void FreeExtra() {
		xstring<CT> mt;
		this->swap(mt);
		if (!mt.empty())this->assign(mt.c_str(), mt.size());
	}
	CT& operator[](int nIdx) {
		return static_cast<std::basic_string<CT>*>(this)->operator[](static_cast<size_type>(nIdx));
	}
	const CT& operator[](int nIdx) const {
		return static_cast<const std::basic_string<CT>*>(this)->operator[](static_cast<size_type>(nIdx));
	}
	CT& operator[](unsigned int nIdx) {
		return static_cast<std::basic_string<CT>*>(this)->operator[](static_cast<size_type>(nIdx));
	}
	const CT& operator[](unsigned int nIdx) const {
		return static_cast<const std::basic_string<CT>*>(this)->operator[](static_cast<size_type>(nIdx));
	}
	operator const CT*() const {
		return this->c_str();
	}

public:

#ifdef _WIN32
	BSTR AllocSysString() const
	{
		xstring<OLECHAR>	os(*this);
		return ::SysAllocString(os.c_str());
	}
	BSTR SetSysString(BSTR* pbstr) const {
		xstring<OLECHAR>	os(*this);
		if (!::SysReAllocStringLen(pbstr, os.c_str(), os.length()))
			throw std::runtime_error("out of memory");
		ASSERT(*pbstr != 0);
		return *pbstr;
	}
#endif

	//////////////////////////////////////////////////////////////////////////
protected:
	template <typename CP>
	static int len(const CP* pT) {
		return (pT == NULL) ? 0 : (int)std::basic_string<CP>::traits_type::length(pT);
	}
	template <typename CP>
	static int len(const std::basic_string<CP>& s) {
		return static_cast<int>(s.length());
	}

protected:
#if defined(_WIN32)
	static std::string& dup(std::string& dst, const wchar_t* src, size_t len = 0, CodePage dstCp = CodePage_UTF8) {
		if (len == 0 && src != nullptr)
			len = wcslen(src);
		if (src == nullptr || len == 0) {
			return dst.erase();
		} else {
			int olen(WideCharToMultiByte(dstCp, 0, src, (int)len, NULL, 0, NULL, NULL));
			if (olen <= 0)return dst.erase();
			dst.resize(olen);
			char* res(const_cast<std::string::pointer>(dst.data()));
			WideCharToMultiByte(dstCp, 0, src, (int)len, res, olen, NULL, NULL);
			return dst;
		}
	}
	static std::string& dup(std::string& dst, CodePage dstCp, const wchar_t* src, size_t len = 0) {
		return dup(dst, src, len, dstCp);
	}
	static std::wstring& dup(std::wstring& dst, const char* src, size_t len = 0, int srcCp = CP_UTF8) {
		if (len == 0 && src != nullptr)
			len = strlen(src);
		if (src == NULL || len == 0) {
			return dst.erase();
		} else {
			int olen(MultiByteToWideChar(srcCp, 0, src, (int)len, NULL, 0));
			if (olen < 1)return dst.erase();
			dst.resize(olen);
			wchar_t*		res(const_cast<std::wstring::pointer>(dst.data()));
			MultiByteToWideChar(srcCp, 0, src, (int)len, res, olen);
			return dst;
		}
	}
	static std::string& dup(std::string& dst, CodePage dstCp, const char* src, size_t len = 0, CodePage srcCp = CodePage_UTF8) {
		if (len == 0 && src != nullptr)
			len = strlen(src);
		if (src == NULL || len == 0) {
			return dst.erase();
		}
		if (dstCp == srcCp) {
			dst = std::string(src, len);
		} else {
			std::wstring wstr;
			dup(wstr, src, len, srcCp);
			dup(dst, dstCp, wstr.c_str(), wstr.length());
		}
		return dst;
	}
	static std::string& dup(std::string& dst, const char* src, size_t len = 0, CodePage srcCp = CodePage_UTF8) {
		return dup(dst, CP_UTF8, src, len, srcCp);
	}
#elif !defined(NO_ICONV)
	static std::string& dup(std::string& dst, const wchar_t* src, size_t len = 0, CodePage dstCp = CodePage_UTF8) {
		if (len == 0 && src != nullptr)
			len = wcslen(src);
		if (src == nullptr || len == 0) {
			return dst.erase();
		} else {
			dst.resize((len + 1) * 4);
			char* in((char*)src);
			size_t ilen(len * sizeof(wchar_t));
			char* out((char*)dst.data());
			char* end(out);
			size_t olen((len + 1) * 4);
			iconv_t conv(iconv_open(dstCp, "WCHAR_T"));
			if (conv == (iconv_t)-1)return dst.erase();
			if (iconv(conv, &in, &ilen, &end, &olen) == (size_t)-1)return dst.erase();
			dst.resize(end - out);
			iconv_close(conv);
			return dst;
		}
	}
	static std::string& dup(std::string& dst, CodePage dstCp, const wchar_t* src, size_t len = 0) {
		return dup(dst, src, len, dstCp);
	}
	static std::wstring& dup(std::wstring& dst, const char* src, size_t len = 0, CodePage srcCp = CodePage_UTF8) {
		if (len < 0 && src != nullptr)
			len = strlen(src);
		if (src == nullptr || len < 0) {
			return dst.erase();
		} else {
			dst.resize(len + 1);
			char* in((char*)src);
			size_t ilen(len * sizeof(char));
			char* out((char*)dst.data());
			char* end(out);
			size_t olen((len + 1) * sizeof(wchar_t));
			iconv_t conv(iconv_open("WCHAR_T", srcCp));
			if (conv == (iconv_t)-1)return dst.erase();
			if (iconv(conv, &in, &ilen, &end, &olen) == (size_t)-1)return dst;
			iconv_close(conv);
			dst.resize((end - out) / sizeof(wchar_t));
			return dst;
		}
	}
	static std::string& dup(std::string& dst, CodePage dstCp, const char* src, size_t len = 0, CodePage srcCp = CodePage_UTF8) {
		if (len == 0 && src != nullptr)
			len = strlen(src);
		if (src == nullptr || len == 0) {
			return dst.erase();
		} else {
			if (strcmp(dstCp, srcCp) == 0) {
				dst = std::string(src, len);
			} else {
				dst.resize((len + 1) * 4);
				char* in((char*)src);
				size_t ilen(len * sizeof(char));
				char* out((char*)dst.data());
				char* end(out);
				size_t olen((len + 1) * 4);
				iconv_t conv(iconv_open(dstCp, srcCp));
				if (conv == (iconv_t)-1)return dst.erase();
				if (iconv(conv, &in, &ilen, &end, &olen) == (size_t)-1) {
					return dst.erase();
				}
				dst.resize(end - out);
				iconv_close(conv);
			}
			return dst;
		}
	}
	static std::string& dup(std::string& dst, const char* src, size_t len = 0, CodePage srcCp = CodePage_UTF8) {
		return dup(dst, CodePage_UTF8, src, len, srcCp);
	}
#endif
	static std::wstring& dup(std::wstring& dst, const wchar_t* src, size_t len = 0, CodePage cp = CodePage_UTF8) {
		if (len == 0 && src != nullptr)
			len = wcslen(src);
		if (src == nullptr || len == 0)
			return dst.erase();
		else
			return dst.assign(src, len);
	}
};

template <typename CT, typename CP>
xstring<CT> operator+(const CP* left, const xstring<CT>& right) {
	xstring<CT> dst(left);
	dst += right;
	return dst;
}
template <typename CT, typename CP>
xstring<CT> operator+(const std::basic_string<CP> left, const xstring<CT>& right) {
	xstring<CT> dst(left);
	dst += right;
	return dst;
}

typedef xstring<TCHAR> xtstring;
#ifdef UNICODE
typedef std::wstring tstring;
#else 
typedef std::string tstring;
#endif

