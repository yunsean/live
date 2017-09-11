#include "ConsoleUI.h"
#include "ConsoleUIImpl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CConsoleUI& CConsoleUI::Singleton()
{
	return CConsoleUIImpl::Singleton();
}

