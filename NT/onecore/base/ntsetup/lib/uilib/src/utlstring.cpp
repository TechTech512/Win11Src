/*

Copyright (c) Microsoft Corporation
Reverse-engineered by github/masonleeback

*/

#include "pch.h"
#include <utlstring.h>

void UTLString::v_Free()
{
    if (m_lpsz != NULL) {
        LocalFree(m_lpsz);
    }
    m_lpsz = NULL;
    m_cch = 0;
}

wchar_t* UTLString::v_CopyString(const wchar_t* src, ULONG* size)
{
    wchar_t* pszDest = NULL;
    ULONG length;
    const wchar_t* pszSrc;

    pszSrc = src;
    if (src != NULL) {
        length = wcslen(src);
        if (length != 0) {
            pszDest = (wchar_t*)LocalAlloc(LMEM_ZEROINIT, sizeof(wchar_t*) * length + 2);
            if (pszDest != NULL) {
                if (StringCchCopyW(pszDest, length + 1, pszSrc) < 0) {
                    LocalFree(pszDest);
                    pszDest = NULL;
                    *size = 0;
                }
                else {
                    *size = length;
                }
            }
        }
    }

    return pszDest;
}

UTLString::UTLString(const wchar_t* lpsz) : m_cch(NULL), m_lpsz(NULL)
{
    if (lpsz != NULL) {
        m_lpsz = v_CopyString(lpsz, &m_cch);
    }
}

UTLString::~UTLString()
{
    v_Free();
}

DWORD UTLString::LoadStringW(HINSTANCE hInstance, UINT uID)
{
    wchar_t* lpsz;
    int cch = 0;

    if (hInstance != NULL) {
        lpsz = (wchar_t*)LocalAlloc(LMEM_ZEROINIT, sizeof(wchar_t) * 1024);
        if (lpsz != NULL) {
            cch = ::LoadStringW(hInstance, uID, lpsz, 1024);
            if (cch == 0) {
                LocalFree(lpsz);
                lpsz = NULL;
            }
        }
        // Free up any existing buffers already allocated
        v_Free();
        m_lpsz = lpsz;
        m_cch = cch;
    }

    // There was no supplied handle, return 0
    return cch;
}

bool UTLString::Append(const wchar_t* pszText)
{
    UINT uLength;
    UINT cchDest;
    wchar_t* lpszDest;

    if (pszText != NULL) {
        uLength = wcslen(pszText);
        cchDest = uLength + 1 + m_cch;
        if (m_lpsz == NULL) {
            m_lpsz = v_CopyString(pszText, &m_cch);
            return true;
        }
        lpszDest = (wchar_t*)LocalAlloc(LMEM_ZEROINIT, sizeof(wchar_t) * (cchDest));
        if (lpszDest != NULL) {
            if (StringCchCopyW(lpszDest, cchDest, m_lpsz) >= 0 &&
                StringCchCopyW(&lpszDest[m_cch], uLength + 1, pszText) >= 0)
            {
                v_Free();
                m_lpsz = lpszDest;
                m_cch = cchDest - 1;
                return true;
            }
            LocalFree(lpszDest);
        }
    }

    return false;
}

DWORD UTLString::FormatMessageW(HINSTANCE hInstance, DWORD uID, ...)
{
    DWORD strSize;
    wchar_t* strUse;
    UTLString* ustFormat;
    char* args;
    va_list va;

    va_start(va, uID);
    va_copy(args, va);

    ustFormat = new UTLString(NULL);
    strSize = ustFormat->LoadStringW(hInstance, uID);
    if (strSize != 0) {
        strUse = ustFormat->m_lpsz;
        if (ustFormat->m_lpsz == NULL) {
            // strUse = &DllPath; why is this using a dependency from autorun???
        }
        strSize = ::FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER, strUse, 0, 0, (LPWSTR)&m_lpsz, 0, &args);
        m_cch = strSize;
    }
    args = NULL;

    return 0;
}
#if defined(_MSC_VER) && _MSC_VER < 1900
void operator delete(void* p, unsigned int) {
    ::operator delete(p);
}
#endif
