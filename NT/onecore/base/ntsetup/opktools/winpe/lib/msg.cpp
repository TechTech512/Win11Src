#include <windows.h>

// Declare HINSTANCE__ if windows.h did not provide it (depends on platform SDK)
struct HINSTANCE__;

DWORD HrMsgModV(wchar_t** output, HINSTANCE__* /*hinst*/, DWORD msgId, DWORD* args, char* /*fmt*/) {
    if (!output || !msgId || !args)
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);

    DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER |
                  FORMAT_MESSAGE_FROM_SYSTEM |
                  FORMAT_MESSAGE_ARGUMENT_ARRAY;

    int len = FormatMessageW(
        flags,
        nullptr,
        msgId,
        0,
        (LPWSTR)output,
        0,
        (va_list*)args
    );

    if (len == 0)
        return HRESULT_FROM_WIN32(GetLastError());

    return S_OK;
}

DWORD HrMsg(wchar_t** output, DWORD msgId, DWORD* args) {
    return HrMsgModV(output, nullptr, msgId, args, nullptr);
}

