// wdslog.h - Windows Deployment Services Logging Interface
#pragma once

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

// Log message structure
typedef struct tagLOG_PARTIAL_MSG {
    DWORD   dwFlags;        // Log message flags
    LPCSTR  pszFormat;      // Format string
    va_list argList;        // Variable arguments
    // ... additional internal members ...
} LOG_PARTIAL_MSG, *PLOG_PARTIAL_MSG;

/// <summary>
/// Constructs a log message with variable arguments (wide char version)
/// </summary>
/// <param name="dwFlags">Flags controlling log message behavior</param>
/// <param name="pszFormat">Format string for the message</param>
/// <param name="...">Variable arguments matching the format string</param>
/// <returns>Pointer to constructed log message structure</returns>
PLOG_PARTIAL_MSG WINAPI ConstructPartialMsgW(
    DWORD dwFlags,
    LPCSTR pszFormat,
    ...
);

/// <summary>
/// Constructs a log message with va_list (wide char version)
/// </summary>
/// <param name="dwFlags">Flags controlling log message behavior</param>
/// <param name="pszFormat">Format string for the message</param>
/// <param name="argList">Variable arguments list</param>
/// <returns>Pointer to constructed log message structure</returns>
PLOG_PARTIAL_MSG WINAPI ConstructPartialMsgVW(
    DWORD dwFlags,
    LPCSTR pszFormat,
    va_list argList
);

#ifdef __cplusplus
} // extern "C"
#endif

