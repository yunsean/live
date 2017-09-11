#pragma once
#include <string>
#include <vector>
#include <assert.h>

#if defined(UNICODE) || defined(_UNICODE)
#define w	L
#else 
#define w
#endif

#ifdef _WIN32

#include <intrin.h>
#include <wtypes.h>
#include <winnt.h>
#include <fileapi.h>
#include <atlbase.h>
#include <stdint.h>
#include <corecrt_io.h>
#include <direct.h>

#undef min
#undef max

#ifndef ASSERT
#include <assert.h>
#define ASSERT(f)				assert((f))
#endif

#define inet_aton(ip, addr)		inet_pton(AF_INET, ip, addr)
#define PATH_SLASH				'\\'

#elif defined(__linux__)

#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define interface struct
typedef void* HMODULE;
#define PATH_SLASH				'/'

#endif

#ifndef CONTAINING_RECORD
#define CONTAINING_RECORD(addr, type, field) ((type*)((unsigned char*)addr - (unsigned long)&((type*)0)->field))  
#endif

