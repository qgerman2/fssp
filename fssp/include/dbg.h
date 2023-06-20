// https://github.com/petercoin/OutputDebugStringWithFormatString
#ifndef _DBG_H_
#define _DBG_H_
#include <tchar.h>
#include <stdio.h>
#include <Windows.h>

#define dprintf(format, ...) ShowDebugBase(TEXT(format), ##__VA_ARGS__)

inline void ShowDebugBase(const TCHAR *format, ...)
{
    va_list args;
    SYSTEMTIME time;
    TCHAR tempBuf[1920];
    TCHAR msgBuf[2048];

    GetLocalTime(&time);

    va_start(args, format);
    _vstprintf_s(tempBuf, format, args);
    _stprintf_s(msgBuf, TEXT("%s\n"), tempBuf);

    OutputDebugString(msgBuf);
    va_end(args);
}

#endif