// errors.cpp

#include "pch.h"

typedef struct _SETUP_ERROR_RESULT {
    int state;
    wchar_t* message;
} SETUP_ERROR_RESULT;

void FreeSetupErrorResult(SETUP_ERROR_RESULT* result) {
    if (!result) return;

    if (result->message) {
        HeapFree(GetProcessHeap(), 0, result->message);
        result->message = NULL;
    }

    ZeroMemory(result, sizeof(SETUP_ERROR_RESULT));
}

int StrAllocatingPrintf(wchar_t** out, const wchar_t* format, ...) {
    if (!out || !format) return E_INVALIDARG;

    va_list args;
    va_start(args, format);

    int length = _vscwprintf(format, args);  // Get required buffer size
    if (length < 0) {
        va_end(args);
        return E_FAIL;
    }

    *out = (wchar_t*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (length + 1) * sizeof(wchar_t));
    if (!*out) {
        va_end(args);
        return E_OUTOFMEMORY;
    }

    // Reinitialize args before second use
    va_end(args);
    va_start(args, format);

    int written = _vsnwprintf(*out, length + 1, format, args);
    va_end(args);

    if (written < 0) {
        HeapFree(GetProcessHeap(), 0, *out);
        *out = NULL;
        return E_FAIL;
    }

    return S_OK;
}

int SetSetupErrorResult(SETUP_ERROR_RESULT* result, const wchar_t* message, int errorCode) {
    if (!result) return E_INVALIDARG;
    if (!message) return E_POINTER;

    if (result->state == 0 || result == (SETUP_ERROR_RESULT*)1) {
        FreeSetupErrorResult(result);
        result->state = 0;

        int hr = StrAllocatingPrintf(&result->message, L"%s", message);
        if (SUCCEEDED(hr)) {
            result->state = 1;
        }

        return hr;
    }

    return 0;
}

