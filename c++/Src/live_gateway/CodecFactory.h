#pragma once
#include <mutex>
#include <map>
#include <list>
#include <vector>
#include "CodecInterface.h"
#include "SmartPtr.h"
#include "json/value.h"

class CCodecFactory {
public:
	CCodecFactory();
	~CCodecFactory();

public:
	static CCodecFactory& singleton();

public:
	bool initialize();
	void status(Json::Value& status);
	void uninitialize();
};

