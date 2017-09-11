// live_source_rtmp.cpp : 定义 DLL 应用程序的导出函数。
//
#include "VodFactory.h"

ISourceFactory* GetSourceFactory() {
	return &CVodFactory::Singleton();
}
