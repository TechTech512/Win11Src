/*

Copyright (c) Microsoft Corporation
Reverse-engineered by github/masonleeback

*/

#ifndef _UTLSTRING_H_
#define _UTLSTRING_H_

class UTLString {
public:
    void v_Free();
    wchar_t* v_CopyString(const wchar_t* src, ULONG* size);

    UTLString() : m_cch(NULL), m_lpsz(NULL) { };
    UTLString(const wchar_t* lpsz);
    ~UTLString();

    DWORD LoadStringW(HINSTANCE hInstance, UINT uID);
    bool Append(const wchar_t* pszText);
    DWORD FormatMessageW(HINSTANCE hInstance, DWORD uID, ...);

    ULONG m_cch;
    PWCHAR m_lpsz;
};

#endif // _UTLSTRING_H_
