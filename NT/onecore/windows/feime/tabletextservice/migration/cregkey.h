#include <windows.h>

class CMyRegKey {
public:
    CMyRegKey();
    ~CMyRegKey();
    
    long Close();
    long Open(HKEY__ *hKey, wchar_t *lpSubKey, ULONG ulOptions);
    long QueryValue(ULONG *lpType, wchar_t *lpValueName);
    long QueryValueCch(wchar_t *lpValue, wchar_t *lpValueName, ULONG cchData);
    long RecurseDeleteKey(wchar_t *lpSubKey);
	static bool vftable;

private:
    HKEY__ *m_hKey;
    int _padding_;
};

CMyRegKey::CMyRegKey()
{
}

CMyRegKey::~CMyRegKey()
{
    _padding_ = (int)vftable;
    Close();
}

long CMyRegKey::Close()
{
    long lResult = 0;
    if (m_hKey != NULL) {
        lResult = RegCloseKey(m_hKey);
        m_hKey = NULL;
    }
    return lResult;
}

long CMyRegKey::Open(HKEY__ *hKey, wchar_t *lpSubKey, ULONG ulOptions)
{
    HKEY__ *hOpenedKey = NULL;
    int iResult = RegOpenKeyExW(hKey, lpSubKey, 0, ulOptions, &hOpenedKey);
    if (iResult == 0) {
        iResult = Close();
        m_hKey = hOpenedKey;
    }
    return iResult;
}

long CMyRegKey::QueryValue(ULONG *lpType, wchar_t *lpValueName)
{
    long lResult;
    DWORD dwType = 0;
    DWORD dwDataSize = 4;
    
    lResult = RegQueryValueExW(m_hKey, lpValueName, 0, &dwType, (LPBYTE)lpType, &dwDataSize);
    return lResult;
}

long CMyRegKey::QueryValueCch(wchar_t *lpValue, wchar_t *lpValueName, ULONG cchData)
{
    long lResult;
    DWORD dwType = 0;
    int cbData = cchData * 2;
    
    lResult = RegQueryValueExW(m_hKey, lpValueName, 0, &dwType, (LPBYTE)lpValue, (LPDWORD)&cbData);
    if (cchData != 0) {
        lpValue[(lResult != 0 ? 0 : cchData - 1)] = L'\0';
    }
    return lResult;
}

long CMyRegKey::RecurseDeleteKey(wchar_t *lpSubKey)
{
    int iResult;
    FILETIME ftLastWriteTime;
    CMyRegKey subKey;
    wchar_t szName[255];
    DWORD dwNameSize = 255;
    
    subKey.m_hKey = NULL;
    subKey._padding_ = (int)vftable;
    
    iResult = Open(subKey.m_hKey, lpSubKey, 0x2001f);
    
    do {
        if (iResult != 0) {
            subKey._padding_ = (int)vftable;
            if (subKey.m_hKey != NULL) {
                RegCloseKey(subKey.m_hKey);
            }
            return iResult;
        }
        
        dwNameSize = 255;
        iResult = RegEnumKeyExW(subKey.m_hKey, 0, szName, &dwNameSize, 0, 0, 0, &ftLastWriteTime);
        if (iResult != 0) {
            if (subKey.m_hKey != NULL) {
                RegCloseKey(subKey.m_hKey);
                subKey.m_hKey = NULL;
            }
            iResult = RegDeleteKeyW(m_hKey, lpSubKey);
            subKey._padding_ = (int)vftable;
            if (subKey.m_hKey != NULL) {
                RegCloseKey(subKey.m_hKey);
            }
            return iResult;
        }
        iResult = subKey.RecurseDeleteKey(szName);
    } while (true);
}

