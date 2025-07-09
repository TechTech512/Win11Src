// bfsvc_types.h

#pragma once
#include <windows.h>

typedef struct _MAPPED_FILE_CONTEXT {
    HANDLE hFile;
    HANDLE hMapping;
    LPVOID lpBaseAddress;
} _MAPPED_FILE_CONTEXT;

typedef enum _LOG_MESSAGE_TYPE {
    BfLogAlways = 0,
    BfLogError,
    BfLogWarning,
    BfLogInformation,
    BfLogDebug
} _LOG_MESSAGE_TYPE;

typedef void (__cdecl *_func___cdecl_void__LOG_MESSAGE_TYPE_wchar_t_ptr)(
    _LOG_MESSAGE_TYPE type,
    const wchar_t *message
);

