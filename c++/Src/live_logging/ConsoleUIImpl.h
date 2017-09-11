#pragma once
#include "ConsoleUI.h"
#include <mutex>
#include "xevent.h"
#include "SmartArr.h"

class CConsoleUIImpl : public CConsoleUI
{
public:
	CConsoleUIImpl(void);
	~CConsoleUIImpl(void);

public:
	static CConsoleUIImpl&	Singleton();

public:
	bool			Initialize(bool enableClose = true, const bool addContext = true, const TCHAR* timeFmt = _T(" %H:%M:%S"), const bool autoReset = false);
	void			EnableClose(const bool enable = true);
	void			BlockExit(bool msgLoop = false);
	void			PermitExit();
	void			SetWillExit(const WillExit& willExit, void* userData);
	void			ReadLine(TCHAR* buffer, const int maxLen, const bool echo = true);
	void			PromptExit(const TCHAR* message = _T(""));
	void			ExitProcess();

	void			SetIcon(const unsigned int iconID);
	void			SetTitle(LPCTSTR format, ...);
	void			PrintMessage(LPCTSTR format, ...);
	void			PrintJournal(LPCTSTR format, ...);
	void			PrintWarning(LPCTSTR format, ...);
	void			PrintError(LPCTSTR format, ...);

	void            SetColor(const ConColor foreground, const ConColor background = Black);
	void			ResetColor();
	void            PrintPrefix();
	void            PrintParts(LPCTSTR format, ...);
	void			Print(const int typeOrColor, const bool prefix, LPCTSTR format, ...);
	void			PrintBlankLine();
	void			PrintSplitLine();

protected:
#ifdef _WIN32
	static void			RemoveCloseMenu();
	static BOOL WINAPI	HandlerRoutine(DWORD dwCtrlType);
#endif

protected:
	void				_ResetColor();
	void				_SetColor(const ConColor foreground, const ConColor background = Black);
	void				_PrintPrefix();
	void				_PrintTime();
	void				_PrintText(LPCTSTR text, ConColor foreground, ConColor background = Black, bool prefix = true);

private:
	CSmartArr<TCHAR>	m_saTimeFmt;
	bool				m_bAutoReset;
#ifdef _WIN32
	HANDLE				m_hOutput;
	unsigned short		m_wOriginAttrib;
	unsigned short		m_wCurAttrib;
#else 
	unsigned short		m_wCurColor; 
#endif
	std::xevent			m_evQuit;
	bool				m_bAddContext;
	std::mutex			m_lock;
	FILE*				m_file;
	WillExit			m_fnWillExit;
	void*				m_lpUserdata;
	CSmartArr<TCHAR>	m_saTextBuf;
};

