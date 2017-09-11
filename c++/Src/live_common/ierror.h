#pragma once
#include "xchar.h"

struct IError {
public:
	virtual unsigned int GetLastErrorCode() const = 0;
	virtual LPCTSTR GetLastErrorDesc() const = 0;
};