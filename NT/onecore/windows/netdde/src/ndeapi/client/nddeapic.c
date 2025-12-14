#include <windows.h>

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    return TRUE;
}

UINT WINAPI NDdeShareAddA(LPSTR serverName, LPSTR shareName, DWORD *shareInfo, DWORD *nShareInfo, DWORD *result)
{
    return ERROR_BAD_FORMAT;
}

UINT WINAPI NDdeShareDelA(DWORD options, LPSTR serverName, DWORD reserved)
{
    return ERROR_BAD_FORMAT;
}

UINT WINAPI NDdeShareEnumA(LPSTR serverName, LPSTR shareInfo, DWORD level, void *buf, DWORD bufSize, DWORD *entriesRead)
{
    return ERROR_BAD_FORMAT;
}

UINT WINAPI NDdeShareGetInfoA(LPSTR serverName, LPSTR shareName, UINT level, BYTE *buf, DWORD bufSize, DWORD *bytesNeeded, USHORT *shareType)
{
    return ERROR_BAD_FORMAT;
}

UINT WINAPI NDdeShareSetInfoA(LPSTR serverName, LPSTR shareName, DWORD level, void *buf, DWORD bufSize, DWORD *param)
{
    return ERROR_BAD_FORMAT;
}

UINT WINAPI NDdeGetErrorStringA(UINT errorCode, LPSTR errorString, DWORD errorStringSize)
{
    return ERROR_BAD_FORMAT;
}

BOOL WINAPI NDdeIsValidShareNameA(LPSTR shareName)
{
    return FALSE;
}

BOOL WINAPI NDdeIsValidAppTopicListA(LPSTR appTopicList)
{
    return FALSE;
}

UINT WINAPI NDdeSpecialCommandA(LPSTR serverName, LPSTR shareName, DWORD command, void *data, DWORD dataSize, DWORD *result)
{
    return ERROR_BAD_FORMAT;
}

UINT WINAPI NDdeGetShareSecurityA(LPSTR serverName, LPSTR shareName, DWORD securityInfo, void *securityDescriptor, DWORD nSizeSecurityDescriptor, DWORD *lpnSizeNeeded)
{
    return ERROR_BAD_FORMAT;
}

UINT WINAPI NDdeSetShareSecurityA(LPSTR serverName, LPSTR shareName, DWORD securityInfo, void *securityDescriptor)
{
    return ERROR_BAD_FORMAT;
}

UINT WINAPI NDdeGetTrustedShareA(LPSTR serverName, LPSTR shareName, DWORD *trustInfo, DWORD *nTrustInfo, DWORD *result)
{
    return ERROR_BAD_FORMAT;
}

UINT WINAPI NDdeSetTrustedShareA(DWORD options, LPSTR serverName, DWORD reserved)
{
    return ERROR_BAD_FORMAT;
}

UINT WINAPI NDdeTrustedShareEnumA(LPSTR serverName, LPSTR trustInfo, DWORD level, void *buf, DWORD bufSize, DWORD *entriesRead)
{
    return ERROR_BAD_FORMAT;
}

UINT WINAPI NDdeShareAddW(LPWSTR serverName, LPWSTR shareName, DWORD *shareInfo, DWORD *nShareInfo, DWORD *result)
{
    return ERROR_BAD_FORMAT;
}

UINT WINAPI NDdeShareDelW(DWORD options, LPWSTR serverName, DWORD reserved)
{
    return ERROR_BAD_FORMAT;
}

UINT WINAPI NDdeShareEnumW(LPWSTR serverName, LPWSTR shareInfo, DWORD level, void *buf, DWORD bufSize, DWORD *entriesRead)
{
    return ERROR_BAD_FORMAT;
}

UINT WINAPI NDdeShareGetInfoW(LPWSTR serverName, LPWSTR shareName, UINT level, BYTE *buf, DWORD bufSize, DWORD *bytesNeeded, USHORT *shareType)
{
    return ERROR_BAD_FORMAT;
}

UINT WINAPI NDdeShareSetInfoW(LPWSTR serverName, LPWSTR shareName, DWORD level, void *buf, DWORD bufSize, DWORD *param)
{
    return ERROR_BAD_FORMAT;
}

UINT WINAPI NDdeGetErrorStringW(UINT errorCode, LPWSTR errorString, DWORD errorStringSize)
{
    return ERROR_BAD_FORMAT;
}

BOOL WINAPI NDdeIsValidShareNameW(LPWSTR shareName)
{
    return FALSE;
}

BOOL WINAPI NDdeIsValidAppTopicListW(LPWSTR appTopicList)
{
    return FALSE;
}

UINT WINAPI NDdeSpecialCommandW(LPWSTR serverName, LPWSTR shareName, DWORD command, void *data, DWORD dataSize, DWORD *result)
{
    return ERROR_BAD_FORMAT;
}

UINT WINAPI NDdeGetShareSecurityW(LPWSTR serverName, LPWSTR shareName, DWORD securityInfo, void *securityDescriptor, DWORD nSizeSecurityDescriptor, DWORD *lpnSizeNeeded)
{
    return ERROR_BAD_FORMAT;
}

UINT WINAPI NDdeSetShareSecurityW(LPWSTR serverName, LPWSTR shareName, DWORD securityInfo, void *securityDescriptor)
{
    return ERROR_BAD_FORMAT;
}

UINT WINAPI NDdeGetTrustedShareW(LPWSTR serverName, LPWSTR shareName, DWORD *trustInfo, DWORD *nTrustInfo, DWORD *result)
{
    return ERROR_BAD_FORMAT;
}

UINT WINAPI NDdeSetTrustedShareW(DWORD options, LPWSTR serverName, DWORD reserved)
{
    return ERROR_BAD_FORMAT;
}

UINT WINAPI NDdeTrustedShareEnumW(LPWSTR serverName, LPWSTR trustInfo, DWORD level, void *buf, DWORD bufSize, DWORD *entriesRead)
{
    return ERROR_BAD_FORMAT;
}

