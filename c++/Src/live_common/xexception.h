#pragma once 
#include <exception>
#include <string.h>
#include <stdlib.h>
#include "xsystem.h"

#ifdef _WIN32
#define _GLIBCXX_USE_NOEXCEPT
#elif !defined(_GLIBCXX_USE_NOEXCEPT)
#define _GLIBCXX_USE_NOEXCEPT throw()
#endif
class xexception : public std::exception {
public:
	xexception(const char* const& msg, int err = -1) _GLIBCXX_USE_NOEXCEPT
#ifdef _WIN32
		: m_msg(_strdup(msg))
#else
		: m_msg(strdup(msg))
#endif 
		, m_err(err) {
	}
	xexception(int err) _GLIBCXX_USE_NOEXCEPT
#ifdef _WIN32
		: m_msg(_strdup(std::errorText(err).c_str()))
#else
		: m_msg(strdup(std::errorText(err).c_str()))
#endif 
		, m_err(err) {
	}
	virtual ~xexception() _GLIBCXX_USE_NOEXCEPT {
		if (m_msg) {
			free(m_msg);
			m_msg = nullptr;
		}
	}
public:
	virtual const char* what() const _GLIBCXX_USE_NOEXCEPT {
		return m_msg;
	}
	virtual int error() const {
		return m_err;
	}
private:
	char*           m_msg;
	int             m_err;
};