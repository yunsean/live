// live_sink_flv.cpp : 定义 DLL 应用程序的导出函数。
//
#include "NalFactory.h"

IDecodeFactory* GetDecodeFactory() {
	return &CNalFactory::singleton();
}

