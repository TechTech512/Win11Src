// file.cpp

#include <windows.h>
#include <Shlwapi.h>
#include <winioctl.h>

namespace UnBCL {

// === Minimal UnBCL types ===

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
    bool IsNullOrEmpty() const { return !m_Data || m_Data[0] == 0; }
};

template <typename T = Object>
class SmartPtr {
public:
    T* m_pObj;

    SmartPtr() : m_pObj(nullptr) {}
    SmartPtr(T* obj) : m_pObj(obj) {}
    ~SmartPtr() { delete m_pObj; }

    void Assign(T* obj) {
        delete m_pObj;
        m_pObj = obj;
    }

    void DeAssign() {
        delete m_pObj;
        m_pObj = nullptr;
    }

    T* operator->() const { return m_pObj; }
    T* Get() const { return m_pObj; }
};

class Path {
public:
    static void GetFullPath(String* s) {
        // Stub — real impl would canonicalize
    }

    static void CheckPathTooLong(String* s) {
        if (wcslen(s->CStr()) >= MAX_PATH) {
            OutputDebugStringW(L"[Path] Path too long\n");
        }
    }

    static String* WithLongPrefix(String* input, bool add) {
        if (!add || !input) return input;
        const wchar_t* raw = input->CStr();
        const wchar_t* prefix = L"\\\\?\\";
        size_t len = wcslen(prefix) + wcslen(raw);
        wchar_t* out = new wchar_t[len + 1];
        wcscpy(out, prefix);
        wcscat(out, raw);
        return new String(out);
    }
};

// === Actual method ===

int File_SetCompressionState(String* path, USHORT compression)
{
    if (!path || path->IsNullOrEmpty())
        return 0;

    Path::GetFullPath(path);
    SmartPtr<String> resolved(Path::WithLongPrefix(path, true));
    Path::CheckPathTooLong(resolved.Get());

    const wchar_t* filePath = resolved->CStr();
    DWORD attrs = GetFileAttributesW(filePath);
    if (attrs == INVALID_FILE_ATTRIBUTES)
        return 0;

    bool wasReadOnly = (attrs & FILE_ATTRIBUTE_READONLY);
    if (wasReadOnly) {
        SetFileAttributesW(filePath, attrs & ~FILE_ATTRIBUTE_READONLY);
    }

    HANDLE hFile = CreateFileW(
        filePath,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS,
        NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        if (wasReadOnly)
            SetFileAttributesW(filePath, attrs);
        return 0;
    }

    USHORT state = compression;
    DWORD bytesReturned = 0;
    BOOL result = DeviceIoControl(
        hFile,
        FSCTL_SET_COMPRESSION,
        &state,
        sizeof(state),
        NULL,
        0,
        &bytesReturned,
        NULL);

    CloseHandle(hFile);

    if (wasReadOnly) {
        SetFileAttributesW(filePath, attrs);
    }

    return result ? 1 : 0;
}

} // namespace UnBCL

