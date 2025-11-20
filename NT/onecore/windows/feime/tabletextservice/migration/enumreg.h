#include "cregkey.h"

class CEnumRegBase;

class CEnumRegKey {
public:
    CEnumRegKey(HKEY__ *hKey, wchar_t *lpSubKey);
    long Next(wchar_t *lpName, ULONG cchName);

private:
    HKEY__ *m_hKey;
    DWORD m_dwIndex;
    DWORD m_dwSubKeyIndex;
	int _padding_;
};

CEnumRegKey::CEnumRegKey(HKEY__ *hKey, wchar_t *lpSubKey)
{
    this->_padding_ = (int)CMyRegKey::vftable;
    m_dwIndex = 0;
    m_dwSubKeyIndex = 0;
    ((CMyRegKey*)this)->Open(hKey, L"System\\CurrentControlSet\\Control\\Keyboard Layouts", 0x20019);
}

long CEnumRegKey::Next(wchar_t *lpName, ULONG cchName)
{
    long lResult;
    DWORD dwNameSize = 0x104;
    
    if (m_hKey == NULL) {
        lResult = -0x7fffbffb;
    }
    else {
        lResult = RegEnumKeyExW(m_hKey, m_dwSubKeyIndex, lpName, &dwNameSize, 0, 0, 0, 0);
        m_dwSubKeyIndex++;
        
        if (cchName > 0) {
            lpName[cchName - 1] = L'\0';
        }
    }
    return lResult;
}

