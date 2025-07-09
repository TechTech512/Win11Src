// log.c

#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include "bfsvc_types.h"

// External/global variables (declare as needed)
extern int LogEnabled;
extern int FilterLevel;
extern _func___cdecl_void__LOG_MESSAGE_TYPE_wchar_t_ptr LogMessageCallback;

// Log message with formatting
long __cdecl BfspLogMessage(_LOG_MESSAGE_TYPE type, const wchar_t *format, ...) {
    if (LogEnabled && (int)type >= FilterLevel && (int)type < 5) {
        wchar_t buffer[512];
        va_list args;
        va_start(args, format);
        _vsnwprintf_s(buffer, sizeof(buffer) / sizeof(wchar_t), _TRUNCATE, format, args);
        va_end(args);

        if (LogMessageCallback) {
            LogMessageCallback(type, buffer);
        } else {
            FILE *out = (type < BfLogWarning) ? stdout : stderr;
            const wchar_t *prefix;

            switch (type) {
                case BfLogWarning:     prefix = L"BFSVC Warning: %s\n"; break;
                case BfLogError:       prefix = L"BFSVC Error: %s\n";   break;
                default:               prefix = L"BFSVC: %s\n";         break;
            }

            fwprintf(out, prefix, buffer);
            fflush(out);
        }
    }
    return 0;
}

