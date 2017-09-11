#include <locale.h>
#include <iostream>
#include <stdarg.h>
#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif
#include "ConsoleUIImpl.h"
#include "xsystem.h"

#define TEXTBUF_LEN		(1024 * 100)

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CConsoleUIImpl::CConsoleUIImpl(void)
	: m_saTimeFmt()
#ifdef _WIN32
	, m_hOutput(GetStdHandle(STD_OUTPUT_HANDLE))
	, m_wOriginAttrib(0)
	, m_wCurAttrib(-1)
#else 
	, m_wCurColor(0)
#endif
	, m_evQuit()
	, m_bAddContext(true)
	, m_lock()
	, m_file(NULL)
	, m_fnWillExit(nullptr)
	, m_lpUserdata(NULL)
	, m_saTextBuf(TEXTBUF_LEN)
{
}

CConsoleUIImpl::~CConsoleUIImpl(void)
{  
#ifdef _WIN32
	if (m_hOutput)SetConsoleTextAttribute(m_hOutput, m_wOriginAttrib);
	if(m_file)fclose(m_file);
	::PostMessage(NULL, WM_QUIT, 0, 0);
	::FreeConsole(); 
#else 
	_tprintf(_T("\033[0m"));
#endif
}

CConsoleUIImpl& CConsoleUIImpl::Singleton()
{
	static CConsoleUIImpl	instance;
	return instance;
}

#ifdef _WIN32
void CConsoleUIImpl::RemoveCloseMenu()
{
	HWND					hWnd(GetConsoleWindow());
	if (hWnd == NULL)return;
	HMENU					hMenu(::GetSystemMenu(hWnd, false));
	if(hMenu == NULL)return;
	::RemoveMenu(hMenu,   SC_CLOSE,MF_BYCOMMAND);
}

BOOL CConsoleUIImpl::HandlerRoutine(DWORD dwCtrlType)
{
	switch(dwCtrlType) {
	case CTRL_C_EVENT:
		break;
	case CTRL_CLOSE_EVENT:
		if (Singleton().m_fnWillExit) {
			RemoveCloseMenu();
			Singleton().m_fnWillExit(Singleton().m_lpUserdata);
			::TerminateThread(::GetCurrentThread(), 0);
		}
		else {
			Singleton().PermitExit();
		}
		return true;
	}

	return true;
}
#endif

bool CConsoleUIImpl::Initialize(bool enableClose /* = true */, const bool addContext /* = true */, const TCHAR* timeFmt /* = _T */, const bool autoReset /*= false*/ )
{
	if (timeFmt != NULL) {
		size_t					len(_tcslen(timeFmt));
		m_saTimeFmt				= new(std::nothrow) TCHAR[len + 1];
		if (m_saTimeFmt.GetArr() == NULL)return false;
		memset(m_saTimeFmt, 0, (len + 1) * sizeof(TCHAR));
		memcpy(m_saTimeFmt, timeFmt, sizeof(TCHAR) * len);
	}
	m_bAddContext				= addContext;
	m_bAutoReset				= autoReset;

#ifdef _WIN32
	if (m_hOutput == NULL) {
		AllocConsole();
		int						hCrun(_open_osfhandle((intptr_t)GetStdHandle(STD_OUTPUT_HANDLE), _O_TEXT));
		m_file					= _fdopen(hCrun, "w");   
		setvbuf(m_file, NULL, _IONBF, 0);   
		*stdout					= *m_file;
		m_hOutput				= GetStdHandle(STD_OUTPUT_HANDLE);
		Print(LightPurple, false, _T("Console Window Ready!"));
		PrintSplitLine();
		PrintBlankLine();
	}
	CONSOLE_SCREEN_BUFFER_INFO	info;
	::GetConsoleScreenBufferInfo(m_hOutput, &info);
	m_wOriginAttrib				= info.wAttributes;
	m_evQuit.reset();
	SetConsoleCtrlHandler(HandlerRoutine, true);
	if (!enableClose)EnableClose(false);
#else 
	setvbuf(stdout, NULL, _IOFBF, 0);
#endif

	return true;
}

void CConsoleUIImpl::EnableClose(const bool bEnable /* = false */)
{
#ifdef _WIN32
	HWND					hWnd(GetConsoleWindow());
	if (hWnd == NULL)return;
	HMENU					hMenu(::GetSystemMenu(hWnd, false));
	if(hMenu == NULL)return;
	::EnableMenuItem(hMenu, SC_CLOSE, (bEnable ? MF_ENABLED : MF_DISABLED) | MF_BYCOMMAND);
#endif
}

void CConsoleUIImpl::BlockExit(bool msgLoop /* = false */)
{
#ifdef _WIN32
	if (msgLoop){
		MSG					msg;
		while (!m_evQuit.is()) {
			if (!PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)){
				Sleep(15);
				continue;
			}
			if (msg.message == WM_QUIT)break;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	else {
		m_evQuit.wait();
	}
#else 
	m_evQuit.wait();
#endif
}

void CConsoleUIImpl::PermitExit()
{
	m_evQuit.set();
}

void CConsoleUIImpl::SetWillExit(const WillExit& fnWillExit, void* lpUserdata)
{
	m_fnWillExit			= fnWillExit;
	m_lpUserdata			= lpUserdata;
}

void CConsoleUIImpl::ReadLine(TCHAR* buffer, const int maxLen, const bool echo /* = true */)
{
#if defined(_WIN32) && !defined(USE_STD)
	DWORD					dwRead(0);
	DWORD					dwFlag(ENABLE_LINE_INPUT);
	if (echo)dwFlag			|= ENABLE_ECHO_INPUT;
	SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), dwFlag);
	ReadConsole(GetStdHandle(STD_INPUT_HANDLE), buffer, maxLen, &dwRead, NULL);
	if (dwRead != 0)dwRead--;
	buffer[dwRead]			= 0;
	PrintBlankLine();
#else 
	fflush(stdout);
	_fgetts(buffer, maxLen, stdin); 
#endif 
}

void CConsoleUIImpl::PromptExit(const TCHAR* message /* = _T */)
{
	_PrintText(message, LightRed, Black, false);
#ifdef _WIN32
	SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), ENABLE_LINE_INPUT);
	DWORD					dwRead(0);
	ReadConsole(GetStdHandle(STD_INPUT_HANDLE), NULL, 0, &dwRead, NULL);
	::TerminateProcess(GetCurrentProcess(), -1);
#endif
}

void CConsoleUIImpl::ExitProcess()
{
#if defined(_WIN32) && !defined(USE_STD)
	::ExitProcess(0);
#else 
	::exit(0);
#endif
}

void CConsoleUIImpl::SetIcon(const unsigned int iconID)
{
#ifdef _WIN32
	HWND					hwnd(GetConsoleWindow());   
	SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(iconID)));
	SendMessage(hwnd, WM_SETICON, ICON_SMALL2, (LPARAM)LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(iconID)));
	SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(iconID)));
#endif
}

void CConsoleUIImpl::SetTitle(LPCTSTR format, ...)
{
#ifdef _WIN32
	std::unique_lock<std::mutex>	lock(m_lock);
	va_list							argument_ptr; 
	memset(m_saTextBuf, 0, TEXTBUF_LEN);
	va_start(argument_ptr, format);   
	_vstprintf_s(m_saTextBuf, TEXTBUF_LEN, format, argument_ptr);
	va_end(argument_ptr);
	SetConsoleTitle(m_saTextBuf);
#endif
}

void CConsoleUIImpl::PrintMessage(LPCTSTR format, ...)
{
	std::unique_lock<std::mutex>	lock(m_lock);
	va_list							argument_ptr;
	memset(m_saTextBuf, 0, TEXTBUF_LEN);
	va_start(argument_ptr, format);   
	_vstprintf_s(m_saTextBuf, TEXTBUF_LEN, format, argument_ptr);
	va_end(argument_ptr);
	_PrintText(m_saTextBuf, Gray);
}

void CConsoleUIImpl::PrintJournal(LPCTSTR format, ...)
{
	std::unique_lock<std::mutex>	lock(m_lock);
	va_list							argument_ptr;
	memset(m_saTextBuf, 0, TEXTBUF_LEN);
	va_start(argument_ptr, format);   
	_vstprintf_s(m_saTextBuf, TEXTBUF_LEN, format, argument_ptr);
	va_end(argument_ptr);
	_PrintText(m_saTextBuf, BrightWhite);
}

void CConsoleUIImpl::PrintWarning(LPCTSTR format, ...)
{
	std::unique_lock<std::mutex>	lock(m_lock);
	va_list							argument_ptr;
	memset(m_saTextBuf, 0, TEXTBUF_LEN);
	va_start(argument_ptr, format);   
	_vstprintf_s(m_saTextBuf, TEXTBUF_LEN, format, argument_ptr);
	va_end(argument_ptr);
	_PrintText(m_saTextBuf, LightYellow);
}

void CConsoleUIImpl::PrintError(LPCTSTR format, ...)
{
	std::unique_lock<std::mutex>	lock(m_lock);
	va_list							argument_ptr;
	memset(m_saTextBuf, 0, TEXTBUF_LEN);
	va_start(argument_ptr, format);   
	_vstprintf_s(m_saTextBuf, TEXTBUF_LEN, format, argument_ptr);
	va_end(argument_ptr);
	_PrintText(m_saTextBuf, LightRed); 
}

void CConsoleUIImpl::Print(const int typeOrColor, const bool prefix, LPCTSTR format, ...)
{
	std::unique_lock<std::mutex>	lock(m_lock);
	va_list							argument_ptr;
	ConColor						color;
	switch(typeOrColor) {
	case Error:
		color						= LightRed;
		break;
	case Warning:
		color						= LightYellow;
		break;
	case Journal:
		color						= BrightWhite;
		break;
	case Message:
		color						= Gray;
		break;
	default:
		if (typeOrColor < 0)color						= White;
		else if (typeOrColor > (int)BrightWhite)color	= White;
		else color										= (ConColor)typeOrColor;
	}
	memset(m_saTextBuf, 0, TEXTBUF_LEN); 
	va_start(argument_ptr, format);   
	_vstprintf_s(m_saTextBuf, TEXTBUF_LEN, format, argument_ptr);
	va_end(argument_ptr);
	_PrintText(m_saTextBuf, color, Black, prefix);
}

void CConsoleUIImpl::PrintBlankLine()
{
	std::unique_lock<std::mutex>	lock(m_lock);
	_PrintText(NULL, White);
}

void CConsoleUIImpl::PrintSplitLine()
{
	Print(BrightWhite, false, _T("-------------------------------------------"), White);
}

void CConsoleUIImpl::ResetColor()
{
	std::unique_lock<std::mutex>	lock(m_lock);
	_ResetColor();
}

void CConsoleUIImpl::SetColor(const ConColor foreground, const ConColor background /* = Black */)
{
	std::unique_lock<std::mutex>	lock(m_lock);
	_SetColor(foreground, background);
}

void CConsoleUIImpl::PrintPrefix()
{
	std::unique_lock<std::mutex>	lock(m_lock);
	_PrintPrefix();
}

void CConsoleUIImpl::PrintParts(LPCTSTR format, ...)
{
	std::unique_lock<std::mutex>	lock(m_lock);
	va_list							argument_ptr;
	memset(m_saTextBuf, 0, TEXTBUF_LEN);
	va_start(argument_ptr, format);   
	_vstprintf_s(m_saTextBuf, TEXTBUF_LEN, format, argument_ptr);
	va_end(argument_ptr);
#if defined(_WIN32) && !defined(USE_STD)
	WriteConsole(m_hOutput, m_saTextBuf, (DWORD)_tcslen(m_saTextBuf), NULL, NULL);
#else 
	_tprintf(_T("%s"), m_saTextBuf.GetArr());
#endif
}

void CConsoleUIImpl::_ResetColor()
{
#ifdef _WIN32
	if (m_hOutput)SetConsoleTextAttribute(m_hOutput, m_wOriginAttrib);
	m_wCurAttrib			= m_wOriginAttrib;
#else 
	_tprintf(_T("\033[0m"));
	m_wCurColor				= 0;
#endif
}

void CConsoleUIImpl::_SetColor(const ConColor foreground, const ConColor background /* = Black */)
{
#ifdef _WIN32
	unsigned short			attr((foreground & 0xf) | ((background << 4) & 0xf0));
	if (m_wCurAttrib == attr)return;
	m_wCurAttrib			= attr;
	SetConsoleTextAttribute(m_hOutput, attr);
#else 
	unsigned short			color((foreground & 0xf) | ((background << 4) & 0xf0));
	if (color == m_wCurColor)return;
	enum ConColor{Black = 0, Blue, Green, SkyBlue, Red, Purple,
		Yellow, White, Gray, LightBlue, LightGreen, LightLGreen, 
		LightRed, LightPurple, LightYellow, BrightWhite};
	const int				colors[] = {0x30, 0x34, 0x32, 0x36, 0x31, 0x35, 0x33, 0x37};
	_tprintf(_T("\033[%02x;%02x;%02xm"), foreground > 0x07 ? 0x01: 0x22, colors[foreground % 8], colors[background % 8] + 0x10);
#endif
}

void CConsoleUIImpl::_PrintTime()
{
	if (m_saTimeFmt.GetArr() == NULL)return;
	TCHAR					buf[256];
	time_t					tmt(time(NULL));
#ifdef _WIN32
	struct tm				tm;
	localtime_s(&tm, &tmt);
	_tcsftime(buf, 256, m_saTimeFmt, &tm);
	_tcscat_s(buf, _T(" "));
#else 
	struct tm*				tm(localtime(&tmt));
	_tcsftime(buf, 256, m_saTimeFmt, tm);
#endif
#if defined(_WIN32) && !defined(USE_STD)
	WriteConsole(m_hOutput, buf, (DWORD)_tcslen(buf), NULL, NULL);
#else 
	_tprintf(_T("%s "), buf);
	fflush(stdout);
#endif
}

void CConsoleUIImpl::_PrintPrefix()
{
	_PrintTime();
#if defined(_WIN32) && !defined(USE_STD)
	if (m_bAddContext) {
		TCHAR				buf[20];
		_stprintf_s(buf, _T("%-4x> "), std::tid());
		WriteConsole(m_hOutput, buf, (DWORD)_tcslen(buf), NULL, NULL);
	} else {
		WriteConsole(m_hOutput, _T("> "), 2, NULL, NULL);
	}
#else 
	if (m_bAddContext)
		_tprintf(_T("%-4x> "), std::tid());
	else
		_tprintf(_T("> "));
	fflush(stdout);
#endif
}

void CConsoleUIImpl::_PrintText(LPCTSTR text, ConColor foreground, ConColor background /* = Black */, bool prefix /* = true */)
{
	if (text) {
		_SetColor(foreground, background);
		if (prefix)_PrintPrefix();		
#if defined(_WIN32) && !defined(USE_STD)
		WriteConsole(m_hOutput, text, (DWORD)_tcslen(text), NULL, NULL);
		int					len((int)_tcslen(text));
		if (len < 2 || (text[len - 1] != TEXT('\n') && text[len - 2] != TEXT('\n')))
			WriteConsole(m_hOutput, _T("\n"), 1, NULL, NULL);
#else 
		_tprintf(_T("%s"), text);
		int					len(_tcslen(text));
		if (len < 2 || (text[len - 1] != TEXT('\n') && text[len - 2] != TEXT('\n')))
			_tprintf(_T("\n"));
		fflush(stdout);
#endif
		if (m_bAutoReset)_ResetColor();
	}
	else
	{
#if defined(_WIN32) && !defined(USE_STD)
		WriteConsole(m_hOutput, _T("\r\n"), 2, NULL, NULL);
#else 
		_tprintf(_T("\r\n"));
		fflush(stdout);
#endif
	}
}
