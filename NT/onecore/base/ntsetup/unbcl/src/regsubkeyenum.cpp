// regsubkeyenum.cpp

#include <windows.h>

namespace UnBCL {

class RegSubkeyEnumerator {
private:
    HKEY m_hKey;
    DWORD m_Index;

public:
    RegSubkeyEnumerator(HKEY hKey)
        : m_hKey(hKey), m_Index(0) {}

    const wchar_t* Next() {
        static wchar_t szName[256];
        DWORD dwNameLen = sizeof(szName) / sizeof(wchar_t);

        LONG result = RegEnumKeyExW(
            m_hKey,
            m_Index,
            szName,
            &dwNameLen,
            NULL,
            NULL,
            NULL,
            NULL
        );

        if (result == ERROR_SUCCESS) {
            ++m_Index;
            return szName;
        }

        return nullptr;
    }

    void Reset() {
        m_Index = 0;
    }
};

} // namespace UnBCL

