// fileenum.cpp

#include <windows.h>
#include <tchar.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <Shlwapi.h>

namespace UnBCL {

class String {
private:
    wchar_t* m_Data;

public:
    String(const wchar_t* str) {
        size_t len = wcslen(str);
        m_Data = new wchar_t[len + 1];
        wcscpy(m_Data, str);
    }

    ~String() {
        delete[] m_Data;
    }

    const wchar_t* CStr() const { return m_Data; }

    static String* Format(const wchar_t* fmt, const wchar_t* param = nullptr) {
        wchar_t buffer[512];
        wsprintfW(buffer, fmt, param ? param : L"(null)");
        return new String(buffer);
    }
};

template <typename T = Object>
class SmartPtr {
private:
    T* m_pObj;

public:
    SmartPtr() : m_pObj(nullptr) {}
    SmartPtr(T* obj) : m_pObj(obj) {}
    ~SmartPtr() { delete m_pObj; }

    T* Get() const { return m_pObj; }
    void Assign(T* obj) {
        delete m_pObj;
        m_pObj = obj;
    }

    void DeAssign() {
        delete m_pObj;
        m_pObj = nullptr;
    }

    T* operator->() const { return m_pObj; }
};

class ArgumentNullException {
public:
    ArgumentNullException(const wchar_t* paramName) {
        OutputDebugStringW(L"[ArgumentNullException] ");
        OutputDebugStringW(paramName);
        OutputDebugStringW(L"\n");
    }
};

class ArgumentException {
public:
    ArgumentException(const wchar_t* paramName) {
        OutputDebugStringW(L"[ArgumentException] ");
        OutputDebugStringW(paramName);
        OutputDebugStringW(L"\n");
    }
};

class Win32Exception {
public:
    static void ThrowLastError(String* msg, const char* file, int line) {
        DWORD code = GetLastError();
        OutputDebugStringW(L"[Win32Exception] ");
        OutputDebugStringW(msg ? msg->CStr() : L"(null)");
        OutputDebugStringW(L"\n");
        OutputDebugStringA(file);
        OutputDebugStringA(": ");
        char num[12];
        wsprintfA(num, "%d", line);
        OutputDebugStringA(num);
        OutputDebugStringA("\n");
        ExitProcess(code);
    }
};

class Directory {
public:
    static bool Exists(String* path) {
        DWORD attrs = GetFileAttributesW(path->CStr());
        return (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY));
    }
};

class StringPtr {
private:
    SmartPtr<String> m_Ptr;

public:
    StringPtr() {}
    StringPtr(const wchar_t* str) { m_Ptr.Assign(new String(str)); }
    StringPtr(String* s) { m_Ptr.Assign(s); }

    void operator=(const StringPtr& other) {
        m_Ptr.Assign(new String(other->CStr()));
    }

    const wchar_t* CStr() const {
        return m_Ptr.Get() ? m_Ptr.Get()->CStr() : L"";
    }

    String* operator->() const { return m_Ptr.Get(); }
};

template <typename T>
T* AddStackTraceToException(T* ex, char*) {
    return ex;
}

class FileSystemEnumerator {
private:
    HANDLE m_FindHandle;
    WIN32_FIND_DATAW m_Fd;

    StringPtr m_RootDir;
    StringPtr m_FileExt;
    StringPtr m_Current;
    StringPtr m_CurrentFileName;

public:
    FileSystemEnumerator(const wchar_t* rootDir, const wchar_t* ext);
    ~FileSystemEnumerator();

    void Reset();
};

FileSystemEnumerator::FileSystemEnumerator(const wchar_t* rootDir, const wchar_t* ext)
    : m_FindHandle(INVALID_HANDLE_VALUE)
{
    if (!rootDir) {
        ArgumentNullException* ex = new ArgumentNullException(L"RootDir");
        OutputDebugStringW(L"[Fatal] Null RootDir\n");
        ExitProcess(1);
    }

    wchar_t fullPath[MAX_PATH];
    DWORD len = GetFullPathNameW(rootDir, MAX_PATH, fullPath, nullptr);

    if (len == 0 || len >= MAX_PATH) {
        String* msg = String::Format(L"unable to get full path for %s", rootDir);
        Win32Exception::ThrowLastError(msg, "fileenum.cpp", __LINE__);
    }

    String* fullStr = new String(fullPath);
    m_RootDir = StringPtr(fullStr);

    if (!Directory::Exists(fullStr)) {
        ArgumentException* ex = new ArgumentException(L"RootDir");
        OutputDebugStringW(L"[Fatal] Invalid RootDir\n");
        ExitProcess(1);
    }

    m_FileExt = StringPtr(ext);
    m_Current = StringPtr();
    m_CurrentFileName = StringPtr();
}

FileSystemEnumerator::~FileSystemEnumerator()
{
    Reset();
}

void FileSystemEnumerator::Reset()
{
    if (m_FindHandle != INVALID_HANDLE_VALUE) {
        FindClose(m_FindHandle);
        m_FindHandle = INVALID_HANDLE_VALUE;
    }
}

} // namespace UnBCL

