#pragma once
#include <functional>
#include "os.h"
#include "xchar.h"

#ifdef _WIN32
#ifdef LIVE_LOGGING_EXPORTS
#define LOGGING_API __declspec(dllexport)
#else
#define LOGGING_API __declspec(dllimport)
#endif
#else 
#define LOGGING_API
#endif

class LOGGING_API CConsoleUI
{
public:
	enum{Error = -1, Warning = -2, Journal = -3, Message = -4};
	enum ConColor{Black = 0, Blue, Green, SkyBlue, Red, Purple,
		Yellow, White, Gray, LightBlue, LightGreen, LightLGreen, 
		LightRed, LightPurple, LightYellow, BrightWhite};

public:
	static CConsoleUI&			Singleton();

public:
	typedef std::function<void (void* userData)> WillExit;

public:
	virtual bool				Initialize(bool enableClose = true, const bool addContext = true, const TCHAR* timeFmt = _T("%H:%M:%S"), const bool autoResetColor = false) = 0;
	virtual void				EnableClose(const bool enable = true) = 0;
	virtual void				BlockExit(bool msgLoop = false) = 0;
	virtual void				PermitExit() = 0;
	virtual void				SetWillExit(const WillExit& willExit, void* userData) = 0;
	virtual void				ReadLine(TCHAR* buffer, const int maxLen, const bool echo = true) = 0;
	virtual void				PromptExit(const TCHAR* message = _T("")) = 0;
	virtual void				ExitProcess() = 0;

	virtual void				SetIcon(const unsigned int iconID) = 0;
	virtual void				SetTitle(LPCTSTR format, ...) = 0;
	virtual void				PrintMessage(LPCTSTR format, ...) = 0;
	virtual void				PrintJournal(LPCTSTR format, ...) = 0;
	virtual void				PrintWarning(LPCTSTR format, ...) = 0;
	virtual void				PrintError(LPCTSTR format, ...) = 0;

	virtual void				SetColor(const ConColor foreground, const ConColor background = Black) = 0;
	virtual void				ResetColor() = 0;
	virtual void				PrintPrefix() = 0;
	virtual void				PrintParts(LPCTSTR format, ...) = 0;
	virtual void				Print(const int typeOrColor, const bool prefix, LPCTSTR format, ...) = 0;
	virtual void				PrintBlankLine() = 0;
	virtual void				PrintSplitLine() = 0;

protected:
	CConsoleUI(){}
	virtual ~CConsoleUI(){}
};

#ifdef _WIN32
#define cpe(lpszFmt, ...)		CConsoleUI::Singleton().PrintError(lpszFmt, __VA_ARGS__)
#define cpw(lpszFmt, ...)		CConsoleUI::Singleton().PrintWarning(lpszFmt, __VA_ARGS__)
#define cpj(lpszFmt, ...)		CConsoleUI::Singleton().PrintJournal(lpszFmt, __VA_ARGS__)
#define cpm(lpszFmt, ...)		CConsoleUI::Singleton().PrintMessage(lpszFmt, __VA_ARGS__)
#define cpp(lpszFmt, ...)       CConsoleUI::Singleton().PrintParts(lpszFmt, __VA_ARGS__);
#define cp(type, lpszFmt, ...)	CConsoleUI::Singleton().Print(type, true, lpszFmt, __VA_ARGS__)
#define cpbl()					CConsoleUI::Singleton().PrintBlankLine()
#else 
#define cpe(lpszFmt, ...)		CConsoleUI::Singleton().PrintError(lpszFmt, ##__VA_ARGS__)
#define cpw(lpszFmt, ...)		CConsoleUI::Singleton().PrintWarning(lpszFmt, ##__VA_ARGS__)
#define cpj(lpszFmt, ...)		CConsoleUI::Singleton().PrintJournal(lpszFmt, ##__VA_ARGS__)
#define cpm(lpszFmt, ...)		CConsoleUI::Singleton().PrintMessage(lpszFmt, ##__VA_ARGS__)
#define cpp(lpszFmt, ...)       CConsoleUI::Singleton().PrintParts(lpszFmt, ##__VA_ARGS__);
#define cp(type, lpszFmt, ...)	CConsoleUI::Singleton().Print(type, true, lpszFmt, ##__VA_ARGS__)
#define cpbl()					CConsoleUI::Singleton().PrintBlankLine()
#endif
