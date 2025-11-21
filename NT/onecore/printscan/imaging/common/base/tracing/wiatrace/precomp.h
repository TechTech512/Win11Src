// precomp.h

#ifndef __TRACE_PCH__
#define __TRACE_PCH__

#pragma once

#include <windows.h>
#include <strsafe.h>
#include <stdlib.h>
#include <tchar.h>

// Forward declarations
struct tagWiaTraceData_Type;

// Trace data structure
struct tagWiaTraceData_Type {
    DWORD m_ulTraceSeverity;
    char* m_pszString;
    char* m_pszFile;
    char* m_pszFunction;
    DWORD m_ulLine;
};

// Function declarations
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved);
DWORD WINAPI WIATRACE_DecrementIndentLevel();
DWORD WINAPI WIATRACE_GetIndentLevel();
void WINAPI WIATRACE_GetTraceSettings(DWORD* pdwTraceMask, DWORD* pdwTraceLevel, INT* pbActivated);
DWORD WINAPI WIATRACE_IncrementIndentLevel();
void WINAPI WIATRACE_Init(HINSTANCE hInstance, char* pszModuleName);
void WINAPI WIATRACE_SetTraceSettings(DWORD dwTraceMask, DWORD dwTraceLevel, INT bActivated);

#endif

