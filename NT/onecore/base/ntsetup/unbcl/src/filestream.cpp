// filestream.cpp

#include <windows.h>
#include <Shlwapi.h>
#include <stdio.h>

namespace UnBCL {

// === Base Types ===

class Object {
public:
    virtual ~Object() {}
};

class IDisposable {
public:
    virtual void Dispose() = 0;
};

enum FileAccess {
    FileAccess_Read = 1,
    FileAccess_Write = 2,
    FileAccess_ReadWrite = 3
};

enum FileMode {
    FileMode_CreateNew = 1,
    FileMode_Create = 2,
    FileMode_Open = 3,
    FileMode_OpenOrCreate = 4,
    FileMode_Truncate = 5,
    FileMode_Append = 6
};

enum FileShare {
    FileShare_None = 0,
    FileShare_Read = 1,
    FileShare_Write = 2,
    FileShare_Delete = 4,
    FileShare_Inheritable = 8
};

enum SeekOrigin {
    SeekOrigin_Begin = 0,
    SeekOrigin_Current = 1,
    SeekOrigin_End = 2
};

// === Exception Types ===

class Exception : public Object {
public:
    Exception(const wchar_t* msg) {
        OutputDebugStringW(msg);
        OutputDebugStringW(L"\n");
    }
};

class ArgumentException : public Exception {
public:
    ArgumentException(const wchar_t* msg) : Exception(msg) {}
};

class ArgumentOutOfRangeException : public Exception {
public:
    ArgumentOutOfRangeException(const wchar_t* msg) : Exception(msg) {}
};

class NotSupportedException : public Exception {
public:
    NotSupportedException(const wchar_t* msg) : Exception(msg) {}
};

class Win32Exception : public Exception {
public:
    Win32Exception(DWORD err, const wchar_t* path)
        : Exception(L"Win32Exception: path failed") {
        WCHAR buf[32];
        _itow_s(err, buf, 10);
        OutputDebugStringW(buf);
        OutputDebugStringW(L" (");
        OutputDebugStringW(path);
        OutputDebugStringW(L")\n");
    }
};

// === FileStream Class ===

class FileStream : public Object, public IDisposable {
private:
    HANDLE m_Handle;
    bool m_CanWrite;

public:
    FileStream(HANDLE handle, FileAccess access) {
        if (handle == INVALID_HANDLE_VALUE) {
            throw ArgumentException(L"invalid handle to FileStream constructor");
        }

        if (access != FileAccess_Read && access != FileAccess_Write && access != FileAccess_ReadWrite) {
            throw ArgumentOutOfRangeException(L"invalid access to FileStream constructor");
        }

        m_Handle = handle;
        m_CanWrite = (access & FileAccess_Write) != 0;
    }

    FileStream(const wchar_t* path, FileMode mode, FileAccess access, FileShare share, DWORD flags) {
        if (!path) {
            throw ArgumentException(L"null path to FileStream constructor");
        }

        if (wcslen(path) == 0) {
            throw ArgumentException(L"empty path to FileStream constructor");
        }

        WCHAR fullPath[MAX_PATH];
        DWORD len = GetFullPathNameW(path, MAX_PATH, fullPath, NULL);
        if (len == 0 || len >= MAX_PATH) {
            throw Win32Exception(GetLastError(), path);
        }

        DWORD desiredAccess = 0;
        if (access & FileAccess_Read) desiredAccess |= GENERIC_READ;
        if (access & FileAccess_Write) desiredAccess |= GENERIC_WRITE;

        DWORD shareMode = 0;
        if (share & FileShare_Read) shareMode |= FILE_SHARE_READ;
        if (share & FileShare_Write) shareMode |= FILE_SHARE_WRITE;
        if (share & FileShare_Delete) shareMode |= FILE_SHARE_DELETE;

        DWORD creationDisposition = 0;
        switch (mode) {
            case FileMode_CreateNew: creationDisposition = CREATE_NEW; break;
            case FileMode_Create: creationDisposition = CREATE_ALWAYS; break;
            case FileMode_Open: creationDisposition = OPEN_EXISTING; break;
            case FileMode_OpenOrCreate: creationDisposition = OPEN_ALWAYS; break;
            case FileMode_Truncate: creationDisposition = TRUNCATE_EXISTING; break;
            case FileMode_Append:
                if (access != FileAccess_Write) {
                    throw ArgumentException(L"append mode with access not write-only");
                }
                creationDisposition = OPEN_ALWAYS;
                break;
            default:
                throw ArgumentOutOfRangeException(L"invalid mode to FileStream constructor");
        }

        m_Handle = CreateFileW(
            fullPath,
            desiredAccess,
            shareMode,
            NULL,
            creationDisposition,
            flags,
            NULL
        );

        if (m_Handle == INVALID_HANDLE_VALUE) {
            throw Win32Exception(GetLastError(), fullPath);
        }

        m_CanWrite = (access & FileAccess_Write) != 0;

        if (mode == FileMode_Append) {
            SetFilePointer(m_Handle, 0, NULL, FILE_END);
        }

        if (GetFileType(m_Handle) != FILE_TYPE_DISK) {
            CloseHandle(m_Handle);
            throw NotSupportedException(L"attempt to create a FileStream on a non-disk file");
        }
    }

    ~FileStream() {
        Close();
    }

    void Close() {
        if (m_Handle != INVALID_HANDLE_VALUE) {
            CloseHandle(m_Handle);
            m_Handle = INVALID_HANDLE_VALUE;
        }
    }

    void Dispose() override {
        Close();
    }

    DWORD Seek(LONG offset, SeekOrigin origin) {
        return SetFilePointer(m_Handle, offset, NULL, origin);
    }

    // Add Read, Write, etc. here as needed
};

} // namespace UnBCL
