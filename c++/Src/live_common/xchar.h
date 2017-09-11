#pragma once
#include "os.h"

#ifndef _INC_TCHAR

    #ifdef _WIN32
        #include <TCHAR.H>
        #include <WTYPES.H>
        #ifndef STRICT
            #define STRICT
        #endif
    #else

        #include <string.h>

        typedef const char*		PCSTR;
        typedef const char*		LPCSTR;
		typedef char*			PSTR;
		typedef char*			LPSTR;
		typedef const wchar_t*	PCWSTR;
		typedef const wchar_t*	LPCWSTR;
		typedef wchar_t*		PWSTR;
		typedef wchar_t*		LPWSTR;
        typedef wchar_t			OLECHAR;

        #if defined(UNICODE) || defined(_UNICODE)
            #include <wchar.h>
            #ifndef _TCHAR_DEFINED
                #ifndef RC_INVOKED
                    typedef wchar_t    TCHAR;
                    typedef wchar_t		_TCHAR;
                #endif
                #define _TCHAR_DEFINED
            #endif

            #define _TEOF WEOF
            #define __TEXT(q)			L##q

            #define _tprintf			wprintf
            #define _ftprintf			fwprintf
            #define _stprintf			swprintf
            #define _sntprintf			_snwprintf
            #define _vtprintf			vwprintf
            #define _vftprintf			vfwprintf
            #define _vstprintf			vswprintf
            #define _vsntprintf			vsnwprintf
            #define _vsctprintf			_vscwprintf
            #define _tscanf				wscanf
            #define _ftscanf			fwscanf
            #define _stscanf			swscanf
            #define _fgettc				fgetwc
            #define _fgettchar			_fgetwchar
            #define _fgetts				fgetws
            #define _fputtc				fputwc
            #define _fputtchar			_fputwchar
            #define _fputts				fputws
            #define _gettc				getwc
            #define _getts				_getws
            #define _puttc				putwc
            #define _puttchar			putwchar
            #define _putts				_putws
            #define _ungettc			ungetwc
            #define _tcstod				wcstod
            #define _tcstol				wcstol
            #define _tcstoul			wcstoul
            #define _itot				_itow
            #define _ltot				_ltow
            #define _ultot				_ultow
            #define _ttoi				_wtoi
            #define _ttol				_wtol
            #define _tstof(x)			wcstof(x, x + wcslen(x))				
            #define _tcscat				wcscat
            #define _tcschr				wcschr
            #define _tcscmp				wcscmp
            #define _tcscpy				wcscpy
            #define _tcscspn			wcscspn
            #define _tcslen				wcslen
            #define _tcsncat			wcsncat
            #define _tcsncmp			wcsncmp
            #define _tcsncpy			wcsncpy
            #define _tcspbrk			wcspbrk
            #define _tcsrchr			wcsrchr
            #define _tcsspn				wcsspn
            #define _tcsstr				wcsstr
            #define _tcstok				wcstok
            #define _tcsdup				_wcsdup
            #define _tcsicmp			wcscasecmp
            #define _tcsnicmp			wcsncasecmp
            #define _tcsnset			_wcsnset
            #define _tcsrev				_wcsrev
            #define _tcsset				_wcsset
            #define _tcslwr				_wcslwr
            #define _tcsupr				_wcsupr
            #define _tcsxfrm			wcsxfrm
            #define _tcscoll			wcscoll
            #define _tcsicoll			_wcsicoll
            #define _istalpha			iswalpha
            #define _istupper			iswupper
            #define _istlower			iswlower
            #define _istdigit			iswdigit
            #define _istxdigit			iswxdigit
            #define _istspace			iswspace
            #define _istpunct			iswpunct
            #define _istalnum			iswalnum
            #define _istprint			iswprint
            #define _istgraph			iswgraph
            #define _istcntrl			iswcntrl
            #define _istascii			iswascii
            #define _totupper			towupper
            #define _totlower			towlower
            #define _tcsftime			wcsftime

            #define _tcsdec				_wcsdec
            #define _tcsinc				_wcsinc
            #define _tcsnbcnt			_wcsncnt
            #define _tcsnccnt			_wcsncnt
            #define _tcsnextc			_wcsnextc
            #define _tcsninc			_wcsninc
            #define _tcsspnp			_wcsspnp
            #define _wcsdec(_wcs1, _wcs2)	((_wcs1)>=(_wcs2) ? NULL : (_wcs2)-1)
            #define _wcsinc(_wcs)			((_wcs)+1)
            #define _wcsnextc(_wcs)			((unsigned int) *(_wcs))
            #define _wcsninc(_wcs, _inc)	(((_wcs)+(_inc)))
            #define _wcsncnt(_wcs, _cnt)	((wcslen(_wcs)>_cnt) ? _count : wcslen(_wcs))
            #define _wcsspnp(_wcs1, _wcs2)	((*((_wcs1)+wcsspn(_wcs1,_wcs2))) ? ((_wcs1)+wcsspn(_wcs1,_wcs2)) : NULL)

            #define _ttoi64(x)			wcstoll(x, x + wcslen(x), 10)
            #define _i64tot				_i64tow
            #define _ui64tot			_ui64tow
            #define _tasctime			_wasctime
            #define _tctime				_wctime
            #define _tctime32			_wctime32
            #define _tctime64			_wctime64
            #define _tstrdate			_wstrdate
            #define _tstrtime			_wstrtime
            #define _tutime				_wutime
            #define _tutime64			_wutime64
            #define _tutime32			_wutime32
            #define _tcsnccoll			_wcsncoll
            #define _tcsncoll			_wcsncoll
            #define _tcsncicoll			_wcsnicoll
            #define _tcsnicoll			_wcsnicoll
            #define _taccess			_waccess
            #define _tchmod				_wchmod
            #define _tcreat				_wcreat
            #define _tfindfirst			_wfindfirst
            #define _tfindnext			_wfindnext
            #define _tfindfirst64		_wfindfirst64
            #define _tfindfirst32		_wfindfirst32
            #define _tfindnext64		_wfindnext64
            #define _tfindnext32		_wfindnext32
            #define _tfdopen			_wfdopen
            #define _tfopen				_wfopen
            #define _tfreopen			_wfreopen
            #define _tfsopen			_wfsopen
            #define _tgetenv			_wgetenv
            #define _tputenv			_wputenv
            #define _tsearchenv			_wsearchenv
            #define _tsystem			_wsystem
            #define _tmakepath			_wmakepath
            #define _tsplitpath			_wsplitpath
            #define _tfullpath			_wfullpath
            #define _tmktemp			_wmktemp
            #define _topen				_wopen
            #define _tremove			_wremove
            #define _trename			_wrename
            #define _tsopen				_wsopen
            #define _tsetlocale			_wsetlocale
            #define _tunlink			_wunlink
            #define _tfinddata_t		_wfinddata_t
            #define _tfindfirsti64		_wfindfirsti64
            #define _tfindnexti64		_wfindnexti64
            #define _tfinddatai64_t		_wfinddatai64_t
            #define _tfinddata64_t		_wfinddata64_t
            #define _tfinddata32_t		_wfinddata32_t
            #define _tfinddata32i64_t	_wfinddata32i64_t
            #define _tfinddata64i32_t	_wfinddata64i32_t
            #define _tfindfirst32i64	_wfindfirst32i64
            #define _tfindfirst64i32	_wfindfirst64i32
            #define _tfindnext32i64		_wfindnext32i64
            #define _tfindnext64i32		_wfindnext64i32
            #define _tchdir				_wchdir
            #define _tgetcwd			_wgetcwd
            #define _tgetdcwd			_wgetdcwd
            #define _tmkdir				_wmkdir
            #define _trmdir				_wrmdir
            #define _tstat				_wstat
            #define _tstati64			_wstati64
            #define _tstat64			_wstat64
            #define _tstat32			_wstat32
            #define _tstat32i64			_wstat32i64
            #define _tstat64i32			_wstat64i32

            #define _tdirent			_wdirent
            #define _TDIR				_WDIR
            #define _topendir			_wopendir
            #define _tclosedir			_wclosedir
            #define _treaddir			_wreaddir
            #define _trewinddir			_wrewinddir
            #define _ttelldir			_wtelldir
            #define _tseekdir			_wseekdir

        #else 

            #ifndef _TCHAR_DEFINED
			    #ifndef RC_INVOKED
			        typedef char		TCHAR;
			        typedef char		_TCHAR;
                #endif
                #define _TCHAR_DEFINED
            #endif

            #define _TEOF				EOF
            #define __TEXT(q)			q

            #define _tmain				main
            #define _tWinMain			WinMain
            #define _tenviron			_environ
            #define __targv				__argv

            #define _tprintf			printf
            #define _ftprintf			fprintf
            #define _stprintf			sprintf
            #define _sntprintf			_snprintf
            #define _vtprintf			vprintf
            #define _vftprintf			vfprintf
            #define _vstprintf			vsprintf
            #define _vsntprintf			vsnprintf
            #define _vsctprintf			_vscprintf
            #define _tscanf				scanf
            #define _ftscanf			fscanf
            #define _stscanf			sscanf
            #define _fgettc				fgetc
            #define _fgettchar			_fgetchar
            #define _fgetts				fgets
            #define _fputtc				fputc
            #define _fputtchar			_fputchar
            #define _fputts				fputs
            #define _tfdopen			_fdopen
            #define _tfopen				fopen
            #define _tfreopen			freopen
            #define _tfsopen			_fsopen
            #define _tgetenv			getenv
            #define _tputenv			_putenv
            #define _tsearchenv			_searchenv
            #define _tsystem			system
            #define _tmakepath			_makepath
            #define _tsplitpath			_splitpath
            #define _tfullpath			_fullpath
            #define _gettc				getc
            #define _getts				gets
            #define _puttc				putc
            #define _puttchar			putchar
            #define _putts				puts
            #define _ungettc			ungetc
            #define _tcstod				strtod
            #define _tcstol				strtol
            #define _tcstoul			strtoul
            #define _itot				_itoa
            #define _ltot				_ltoa
            #define _ultot				_ultoa
            #define _ttoi				atoi
            #define _ttol				atol
            #define _tstof				atof
            #define _tcscat				strcat
            #define _tcschr				strchr
            #define _tcscmp				strcmp
            #define _tcscpy				strcpy
            #define _tcscspn			strcspn
            #define _tcslen				strlen
            #define _tcsncat			strncat
            #define _tcsncmp			strncmp
            #define _tcsncpy			strncpy
            #define _tcspbrk			strpbrk
            #define _tcsrchr			strrchr
            #define _tcsspn				strspn
            #define _tcsstr				strstr
            #define _tcstok				strtok
            #define _tcsdup				_strdup
            #define _tcsicmp			strcasecmp
            #define _tcsnicmp			strncasecmp
            #define _tcsnset			_strnset
            #define _tcsrev				_strrev
            #define _tcsset				_strset
            #define _tcslwr				_strlwr
            #define _tcsupr				_strupr
            #define _tcsxfrm			strxfrm
            #define _tcscoll			strcoll
            #define _tcsicoll			_stricoll
            #define _istalpha			isalpha
            #define _istupper			isupper
            #define _istlower			islower
            #define _istdigit			isdigit
            #define _istxdigit			isxdigit
            #define _istspace			isspace
            #define _istpunct			ispunct
            #define _istalnum			isalnum
            #define _istprint			isprint
            #define _istgraph			isgraph
            #define _istcntrl			iscntrl
            #define _istascii			isascii
            #define _totupper			toupper
            #define _totlower			tolower
            #define _tasctime			asctime
            #define _tctime				ctime
            #define _tctime32			_ctime32
            #define _tctime64			_ctime64
            #define _tstrdate			_strdate
            #define _tstrtime			_strtime
            #define _tutime				_utime
            #define _tutime64			_utime64
            #define _tutime32			_utime32
            #define _tcsftime			strftime

            #define _tcsdec				_strdec
            #define _tcsinc				_strinc
            #define _tcsnbcnt			_strncnt
            #define _tcsnccnt			_strncnt
            #define _tcsnextc			_strnextc
            #define _tcsninc			_strninc
            #define _tcsspnp			_strspnp
            #define _strdec(_str1, _str2)	((_str1)>=(_str2) ? NULL : (_str2)-1)
            #define _strinc(_str)			((_str)+1)
            #define _strnextc(_str)			((unsigned int) *(_str))
            #define _strninc(_str, _inc)	(((_str)+(_inc)))
            #define _strncnt(_str, _cnt)	((strlen(_str)>_cnt) ? _count : strlen(_str))
            #define _strspnp(_str1, _str2)	((*((_str1)+strspn(_str1,_str2))) ? ((_str1)+strspn(_str1,_str2)) : NULL)

            #define _tchmod				_chmod
            #define _tcreat				_creat
            #define _tfindfirst			_findfirst
            #define _tfindnext			_findnext
            #define _tfindfirst64		_findfirst64
            #define _tfindfirst32		_findfirst32
            #define _tfindnext64		_findnext64
            #define _tfindnext32		_findnext32
            #define _tmktemp			_mktemp
            #define _topen				_open
            #define _taccess			_access
            #define _tremove			remove
            #define _trename			rename
            #define _tsopen				_sopen
            #define _tsetlocale			setlocale
            #define _tunlink			_unlink
            #define _tfinddata_t		_finddata_t
            #define _tchdir				_chdir
            #define _tgetcwd			_getcwd
            #define _tgetdcwd			_getdcwd
            #define _tmkdir				_mkdir
            #define _trmdir				_rmdir
            #define _tstat				_stat

            #define _ttoi64				atoll
            #define _i64tot				_i64toa
            #define _ui64tot			_ui64toa
            #define _tcsnccoll			_strncoll
            #define _tcsncoll			_strncoll
            #define _tcsncicoll			_strnicoll
            #define _tcsnicoll			_strnicoll
            #define _tfindfirsti64		_findfirsti64
            #define _tfindnexti64		_findnexti64
            #define _tfinddatai64_t		_finddatai64_t
            #define _tfinddata64_t		_finddata64_t
            #define _tfinddata32_t		_finddata32_t
            #define _tfinddata32i64_t	_finddata32i64_t
            #define _tfinddata64i32_t	_finddata64i32_t
            #define _tfindfirst32i64	_findfirst32i64
            #define _tfindfirst64i32	_findfirst64i32
            #define _tfindnext32i64		_findnext32i64
            #define _tfindnext64i32		_findnext64i32
            #define _tstati64			_stati64
            #define _tstat64			_stat64
            #define _tstat32			_stat32
            #define _tstat32i64			_stat32i64
            #define _tstat64i32			_stat64i32

            #define _tdirent			dirent
            #define _TDIR				DIR
            #define _topendir			opendir
            #define _tclosedir			closedir
            #define _treaddir			readdir
            #define _trewinddir			rewinddir
            #define _ttelldir			telldir
            #define _tseekdir			seekdir

        #endif 

        #define _TEXT(x)				__TEXT(x)
        #define TEXT(x)					__TEXT(x)
        #define _T(x)					__TEXT(x)

        #define _vstprintf_s(str, len, fmt, ap)			_vsntprintf(str, len, fmt, ap)
        #define _vsntprintf_s(str, len, _truc, fmt, ap)	_vsntprintf(str, len, fmt, ap)
        #define _getts_s(str, len)						_getts(str)
        #define _tcscpy_s(dst, len, src)				_tcscpy(dst, src)
        #define strcpy_s(dst, len, src)                 strcpy(dst, src)
        #define wcscpy_s(dst, len, src)                 wcscpy(dst, src)
        #define strncpy_s(dst, len, src, _truc)         strncpy(dst, src, len)
        #define _tcsncpy_s(dst, len, src, _truc)		_tcsncpy(dst, src, len)
        #define _stprintf_s(str, len, fmt, ...)			_stprintf(str, fmt, ##__VA_ARGS__)
        #define _stscanf_s(str, fmt, ...)				_stscanf(str, fmt, ##__VA_ARGS__)
        #define vsnprintf_s(str, len, _truc, fmt, ap)	vsnprintf(str, len, fmt, ap)
        #define _vsnwprintf_s(str, len, _truc, fmt, ap)	vswprintf(str, len, fmt, ap)
        #define _tcscat_s(dst, size, src)				_tcscat(dst, src)
        #define _stricmp(left, right)                   strcasecmp(left, right)
        #define _wcsicmp(left, right)                   wcscasecmp(left, right)
        #define _strnicmp(left, right, len)             strncasecmp(left, right, len)
        #define _wcsnicmp(left, right, len)             wcsncasecmp(left, right, len)
        #define _mkdir(path)                            mkdir(path, 0777)
        #define _access(path, mode)                     access(path, mode)

    #endif	
		
    #ifndef ASSERT
        #include <assert.h>
        #define ASSERT(f)		assert((f))
    #endif
    #ifndef VERIFY
        #ifdef _DEBUG
            #define VERIFY(x)	ASSERT((x))
        #else
            #define VERIFY(x)	x
        #endif
    #endif

    #ifndef UNUSED
        #define UNUSED(x)		x
    #endif

#endif 

#ifndef __wtypes_h__
    #ifdef _WIN32
        #include <wtypes.h>
    #else 
        typedef char			CHAR;
		typedef wchar_t			WCHAR;
		typedef TCHAR*			LPTSTR;
		typedef const TCHAR*	LPCTSTR;
    #endif
#endif
