#include "file.h"
#ifdef _WIN32
#else 
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <fnmatch.h>
#endif
#include "SmartHdr.h"
#include "xsystem.h"

#ifdef _WIN32
#else 
void _split_whole_name(const char *whole_name, char *fname, char *ext) {
	const char* p_ext(rindex(whole_name, '.'));
	if (NULL != p_ext) {
		if (NULL != ext) strcpy(ext, p_ext);
		if (NULL != fname) snprintf(fname, p_ext - whole_name + 1, "%s", whole_name);
	}
	else {
		if (NULL != ext) ext[0] = '\0';
		if (NULL != fname) strcpy(fname, whole_name);
	}
}
void _splitpath(const char* path, char* drive, char* dir, char* fname, char* ext) {
	const char* p_whole_name;
	if (NULL != drive) {
		drive[0] = '\0';
	}
	if (NULL == path) {
		if (NULL != dir) strcpy(dir, ".");
		if (NULL != fname) fname[0] = '\0';
		if (NULL != ext) ext[0] = '\0';
		return;
	}
	if ('/' == path[strlen(path) - 1]) {
		if (NULL != dir) strcpy(dir, path);
		if (NULL != fname) fname[0] = '\0';
		if (NULL != ext) ext[0] = '\0';
		return;
	}
	p_whole_name = rindex(path, '/');
	if (NULL != p_whole_name) {
		p_whole_name++;
		_split_whole_name(p_whole_name, fname, ext);
		if (NULL != dir) {
			strncpy(dir, path, p_whole_name - path);
			dir[p_whole_name - path] = '\0';
		}
	}
	else {
		_split_whole_name(path, fname, ext);
		if (NULL != dir) dir[0] = '\0';
	}
}
void _splitpath_s(const char* path, char* drive, size_t, char* dir, size_t, char* fname, size_t, char* ext) {
	_splitpath(path, drive, dir, fname, ext);
}

struct _find_search_t {
	char *pattern;
	char *curfn;
	char *directory;
	int dirlen;
	DIR *dirfd;
};
intptr_t _findfirst(const char *pattern, struct _finddata_t *data) {
	_find_search_t *fs = new _find_search_t;
	fs->curfn = NULL;
	fs->pattern = NULL;
	const char *mask = strrchr(pattern, '/');
	if (mask) {
		mask++;
		fs->dirlen = static_cast<int>(mask - pattern);
		fs->directory = (char *)malloc(fs->dirlen + 1);
		memcpy(fs->directory, pattern, fs->dirlen);
		fs->directory[fs->dirlen] = 0;
	}
	else {
		mask = pattern;
		fs->directory = strdup(".");
		fs->dirlen = 1;
	}
	fs->dirfd = opendir(fs->directory);
	if (!fs->dirfd) {
		_findclose((intptr_t)fs);
		return 0;
	}
	if (strcmp(mask, "*.*") == 0) {
		mask += 2;
	}
	fs->pattern = strdup(mask);
	if (_findnext((intptr_t)fs, data) < 0) {
		_findclose((intptr_t)fs);
		return 0;
	}
	return (intptr_t)fs;
}
int _findnext(intptr_t id, struct _finddata_t *data) {
	_find_search_t *fs = reinterpret_cast<_find_search_t*>(id);
	dirent *entry;
	for (;;) {
		if (!(entry = readdir(fs->dirfd))) {
			return -1;
		}
		if (fnmatch(fs->pattern, entry->d_name, 0) == 0) {
			break;
		}
	}
	if (fs->curfn) {
		free(fs->curfn);
	}
	data->name = fs->curfn = strdup(entry->d_name);
	size_t namelen = strlen(entry->d_name);
	char *xfn = new char[fs->dirlen + 1 + namelen + 1];
	sprintf(xfn, "%s/%s", fs->directory, entry->d_name);
	struct stat stat_buf;
	if (stat(xfn, &stat_buf)) {
		data->attrib = FILE_ATTRIBUTE_NORMAL;
		data->size = 0;
	}
	else {
		if (S_ISDIR(stat_buf.st_mode)) {
			data->attrib = FILE_ATTRIBUTE_DIRECTORY;
		}
		else {
			data->attrib = FILE_ATTRIBUTE_NORMAL;
		}
		data->size = (unsigned long)stat_buf.st_size;
	}
	delete[] xfn;
	if (data->name[0] == '.') {
		data->attrib |= FILE_ATTRIBUTE_HIDDEN;
	}
	return 0;
}
int _findclose(intptr_t id) {
	_find_search_t *fs = reinterpret_cast<_find_search_t *>(id);
	int result = fs->dirfd ? closedir(fs->dirfd) : 0;
	free(fs->pattern);
	free(fs->directory);
	if (fs->curfn) {
		free(fs->curfn);
	}
	delete fs;
	return result;
}
#endif

namespace file {
#ifdef _WIN32
	bool findFiles(LPCTSTR path, std::function<void(LPCTSTR file, bool isDirectory)> onFile, const bool recursion /* = false */) {
		WIN32_FIND_DATA findData = {};
		const static int maxFileName(1024 > _MAX_PATH ? 1024 : _MAX_PATH);
		TCHAR fileName[maxFileName];
		TCHAR searchPattern[maxFileName];
		TCHAR searchName[_MAX_FNAME];
		TCHAR searchExt[_MAX_EXT];
		TCHAR rootPath[maxFileName];
		TCHAR partyDrive[_MAX_DRIVE];
		TCHAR partyPath[maxFileName];
		size_t prefix(0);
		CSmartHdr<HANDLE> shFind(::FindClose, INVALID_HANDLE_VALUE);
		if (path == NULL || _tcslen(path) == 0) path = _T("*.*");
		if (_tfullpath(searchPattern, path, maxFileName) == NULL) return false;
		_tsplitpath_s(searchPattern, partyDrive, _MAX_DRIVE, partyPath, maxFileName, searchName, _MAX_FNAME, searchExt, _MAX_EXT);
		if (_tcslen(searchName) == 0 && _tcslen(searchExt) == 0) {
			_tcscpy_s(searchName, _T("*"));
			_tcscpy_s(searchExt, _T(".*"));
			_tcscat_s(searchPattern, maxFileName - _tcslen(searchPattern), _T("*.*"));
		}
		_tmakepath_s(rootPath, partyDrive, partyPath, NULL, NULL);
		_tcscpy_s(fileName, rootPath);
		prefix = _tcslen(rootPath);
		shFind = ::FindFirstFile(searchPattern, &findData);
		if (!shFind.isValid()) return false;
		do {
			_tcscpy_s(fileName + prefix, maxFileName - prefix, findData.cFileName);
			if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				if (_tcscmp(findData.cFileName, _T(".")) == 0) continue;
				if (_tcscmp(findData.cFileName, _T("..")) == 0) continue;
				_tcscat_s(fileName, maxFileName - _tcslen(fileName), _T("\\"));
				onFile(fileName, true);
				if (recursion) {
					_tcscat_s(fileName, maxFileName - _tcslen(fileName), searchName);
					_tcscat_s(fileName, maxFileName - _tcslen(fileName), searchExt);
					findFiles(fileName, onFile, recursion);
				}
			}
			else {
				onFile(fileName, false);
			}
		} while (::FindNextFile(shFind, &findData));
		return true;
	}
	xtstring parentDirectory(LPCTSTR file) {
		const TCHAR* fname = _tcsrchr(file, '\\');
		const TCHAR* fname1 = _tcsrchr(file, '/');
		if (fname == NULL) fname = fname1;
		else if (fname1 > fname) fname = fname1;
		if (fname == NULL) return _T("");
		fname++;
		xtstring path(file, (int)(fname - file));
		return path;
	}
	xtstring filename(LPCTSTR path) {
		const TCHAR* fname = _tcsrchr(path, '\\');
		const TCHAR* fname1 = _tcsrchr(path, '/');
		if (fname == NULL) fname = fname1;
		else if (fname1 > fname) fname = fname1;
		if (fname == NULL) return _T("");
		fname++;
		xtstring res(fname);
		return res;
	}
#else 
	bool findFiles(LPCTSTR path, std::function<void(LPCTSTR file, bool isDirectory)> onFile, const bool recursion /* = false */) {
		const static int maxFileName(1024);
		TCHAR fileName[maxFileName];
		TCHAR searchPattern[maxFileName];
		TCHAR searchName[1024];
		TCHAR searchExt[256];
		TCHAR rootPath[maxFileName];
		TCHAR partyPath[maxFileName];
		int prefix(0);
		if (path == NULL || strlen(path) == 0) path = "*.*";
		_splitpath(path, NULL, partyPath, searchName, searchExt);
		if (strlen(searchName) == 0 && strlen(searchExt) == 0) {
			strcpy(searchName, "*");
			strcpy(searchExt, ".*");
		}
		if (strlen(partyPath) == 0) strcpy(partyPath, ".");
		if (!realpath(partyPath, searchPattern)) return false;
		strcat(searchPattern, "/");
		strcpy(rootPath, searchPattern);
		strcat(searchPattern, searchName);
		strcat(searchPattern, searchExt);
		strcpy(fileName, rootPath);
		prefix = strlen(fileName);
		CSmartHdr<intptr_t> shFind(_findclose, 0);
		_finddata_t findData = {};
		shFind = _findfirst(searchPattern, &findData);
		if (!shFind.isValid()) return false;
		do {
			strcpy(fileName + prefix, findData.name);
			if (findData.attrib & FILE_ATTRIBUTE_DIRECTORY) {
				if (strcmp(findData.name, ".") == 0) continue;
				if (strcmp(findData.name, "..") == 0) continue;
				strcat(fileName, "/");
				onFile(fileName, true);
				if (recursion) {
					strcat(fileName, searchName);
					strcat(fileName, searchExt);
					findFiles(fileName, onFile, recursion);
				}
			}
			else {
				onFile(fileName, false);
			}
		} while (_findnext(shFind, &findData) == 0);
		return true;

		return true;
	}
	xtstring parentDirectory(LPCTSTR file) {
		const TCHAR* fname = _tcsrchr(file, '/');
		if (fname == NULL) return _T("");
		fname++;
		xtstring path(file, (int)(fname - file));
		return path;
	}
	xtstring filename(LPCTSTR path) {
		const TCHAR* fname = _tcsrchr(path, '/');
		if (fname == NULL) return _T("");
		fname++;
		xtstring res(fname);
		return res;
	}
#endif
	xtstring exeuteFilePath() {
		TCHAR file[1024];
		if (!std::exefile(file)) return _T("");
		return parentDirectory(file);
	}
}