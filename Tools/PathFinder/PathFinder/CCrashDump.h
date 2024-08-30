#pragma once
#pragma comment (lib, "Dbghelp.lib")
#include <direct.h>
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <Dbghelp.h>
#include <psapi.h>
#include <crtdbg.h>

class CCrashDump
{
public:
    CCrashDump();

    ~CCrashDump()
    {

    }

    static void Crash(void);

    static LONG WINAPI MyExceptionFilter(__in PEXCEPTION_POINTERS pExceptionPointer);

    static void SetHandlerDump(void);

    static void myInvalidParameterHandler(const wchar_t* expression, const wchar_t* function, const wchar_t* file, unsigned int   line, uintptr_t   pReserved);

    static int _custom_Report_hook(int ireposttype, char* message, int* returnvalue);

    static void myPurecallHandler(void);

    static long _DumpCount;
};
