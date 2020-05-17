/* $Id$ */

#include <windows.h>

// use color constants (and combinations):
//    FOREGROUND_BLUE
//    FOREGROUND_GREEN
//    FOREGROUND_RED
//    FOREGROUND_INTENSITY
//
//    BACKGROUND_BLUE
//    BACKGROUND_GREEN
//    BACKGROUND_RED
//    BACKGROUND_INTENSITY
//
#define FOREGROUND_LIGHT_YELLOW (FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY)
#define FOREGROUND_LIGHT_GREEN (FOREGROUND_GREEN | FOREGROUND_INTENSITY)
#define FOREGROUND_LIGHT_BLUE (FOREGROUND_BLUE | FOREGROUND_INTENSITY)
#define FOREGROUND_LIGHT_CYAN (FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY)
#define FOREGROUND_LIGHT_RED (FOREGROUND_RED | FOREGROUND_INTENSITY)
#define FOREGROUND_GRAY (FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED)

class ConColor
{
public:
	WORD m_wOldColor;
	
	ConColor(DWORD newColor = -1)
	{
		m_wOldColor = SetColor(newColor);
	}
	~ConColor()
	{
		SetColor();
	}
	WORD SetColor(DWORD newColor = -1)
	{
		if(newColor == -1)
			newColor = m_wOldColor;
		HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
		CONSOLE_SCREEN_BUFFER_INFO csbiInfo;
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbiInfo);
		WORD retOldColor = csbiInfo.wAttributes;
		SetConsoleTextAttribute(hOut, (WORD)newColor);
		return retOldColor;
	}

	WORD operator () (DWORD newColor = -1) {return SetColor(newColor); };
};
