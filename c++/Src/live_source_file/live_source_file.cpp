// live_source_file.cpp : 定义 DLL 应用程序的导出函数。
//
#include "SourceInterface.h"
#include "FileFactory.h"

ISourceFactory* GetSourceFactory() {
	return new CFileFactory();
}

