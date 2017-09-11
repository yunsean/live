// live_sink_raw.cpp : 定义 DLL 应用程序的导出函数。
//
#include "RawFactory.h"

ISinkFactory* GetSinkFactory() {
	return &CRawFactory::singleton();
}

