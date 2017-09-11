#pragma once
#include <functional>
#include "xstring.h"

#ifdef _WIN32
#else 
void _split_whole_name(const char *whole_name, char *fname, char *ext);
void _splitpath(const char* path, char* drive, char* dir, char* fname, char* ext);
void _splitpath_s(const char* path, char* drive, size_t, char* dir, size_t, char* fname, size_t, char* ext);

struct _finddata_t {
	char *name;
	int attrib;
	unsigned long size;
};
#define FILE_ATTRIBUTE_READONLY             0x00000001  
#define FILE_ATTRIBUTE_HIDDEN               0x00000002  
#define FILE_ATTRIBUTE_SYSTEM               0x00000004  
#define FILE_ATTRIBUTE_DIRECTORY            0x00000010  
#define FILE_ATTRIBUTE_ARCHIVE              0x00000020  
#define FILE_ATTRIBUTE_NORMAL               0x00000080 

intptr_t _findfirst(const char *pattern, struct _finddata_t *data);
int _findnext(intptr_t id, struct _finddata_t *data);
int _findclose(intptr_t id);
#endif
namespace file {
	bool findFiles(LPCTSTR path, std::function<void(LPCTSTR file, bool isDirectory)> onFile, const bool recursion = false);
	xtstring parentDirectory(LPCTSTR file);
	xtstring filename(LPCTSTR path);
	xtstring exeuteFilePath();
};
