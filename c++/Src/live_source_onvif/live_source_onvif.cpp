// live_source_hxht.cpp : 定义 DLL 应用程序的导出函数。
//
#include "OnvifFactory.h"

ISourceFactory* GetSourceFactory() {
	return &COnvifFactory::Singleton();
}


