#include <windows.h>

class WpeLogFile {
public:
    WpeLogFile();
    ~WpeLogFile();
    UINT Initialize(wchar_t* logPath);
    void Cleanup();
    UINT Initialize(void* handle, int dummy, UINT isNew);

private:
    HANDLE hLogFile;
};

WpeLogFile::WpeLogFile() : hLogFile(INVALID_HANDLE_VALUE) {}

WpeLogFile::~WpeLogFile() {
    Cleanup();
}

void WpeLogFile::Cleanup() {
    if (hLogFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hLogFile);
        hLogFile = INVALID_HANDLE_VALUE;
    }
}

int FileExists(const wchar_t* path) {
    DWORD attrib = GetFileAttributesW(path);
    return (attrib != INVALID_FILE_ATTRIBUTES && !(attrib & FILE_ATTRIBUTE_DIRECTORY));
}

UINT WpeLogFile::Initialize(void* handle, int, UINT) {
    if (handle == INVALID_HANDLE_VALUE) {
        return 0x80004005; // E_FAIL
    }
    hLogFile = (HANDLE)handle;
    return 0; // S_OK
}

UINT WpeLogFile::Initialize(wchar_t* path) {
    HANDLE fileHandle;
    UINT hr;
    int fileExisted = FileExists(path);

    fileHandle = CreateFileW(L"winpeshl.log", GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ,
                             NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (fileHandle == INVALID_HANDLE_VALUE) {
        hr = GetLastError();
        if ((int)hr > 0) {
            hr = (hr & 0xFFFF) | 0x80070000;
        }
        if ((int)hr < 0) {
            hr = GetLastError();
            if ((int)hr > 0) {
                hr = (hr & 0xFFFF) | 0x80070000;
            }
        } else {
            hr = 0x80004005; // E_FAIL
        }
        if ((int)hr < 0) {
            Cleanup();
            return hr;
        }
    }

    if (fileExisted) {
        SetFilePointer(fileHandle, 0, NULL, FILE_END);
    }

    hr = Initialize(fileHandle, (int)this, (UINT)(fileExisted == 0));
    if ((int)hr < 0) {
        Cleanup();
    }
    return hr;
}

