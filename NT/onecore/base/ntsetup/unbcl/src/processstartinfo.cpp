// processstartinfo.cpp

#include <windows.h>

namespace UnBCL {

class ProcessStartInfo {
public:
    wchar_t* m_FileName;
    wchar_t* m_Arguments;
    wchar_t* m_WorkingDirectory;
    wchar_t* m_UserName;
    wchar_t* m_Domain;
    wchar_t* m_Password;

    bool m_CreateNoWindow;

public:
    ProcessStartInfo()
        : m_FileName(nullptr),
          m_Arguments(nullptr),
          m_WorkingDirectory(nullptr),
          m_UserName(nullptr),
          m_Domain(nullptr),
          m_Password(nullptr),
          m_CreateNoWindow(false)
    {
    }

    ~ProcessStartInfo() {
        Free(m_FileName);
        Free(m_Arguments);
        Free(m_WorkingDirectory);
        Free(m_UserName);
        Free(m_Domain);
        Free(m_Password);
    }

private:
    void Free(wchar_t* ptr) {
        if (ptr) {
            delete[] ptr;
        }
    }

    // You could optionally define SetX methods that deep-copy input strings
};

} // namespace UnBCL

