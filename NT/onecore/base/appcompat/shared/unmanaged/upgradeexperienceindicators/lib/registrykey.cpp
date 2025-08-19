#include <windows.h>
#include <string>
#include <vector>
#include <rtlarray.h>

// Add typedefs for custom types
typedef unsigned int uint;
typedef unsigned char uchar;
typedef unsigned long ulong;

namespace Windows {
namespace Compat {
namespace Shared {

class RegistryKey {
public:
    RegistryKey() : Key(nullptr) {}
    ~RegistryKey() { UnInitialize(); }

    uint ClearKey();
    uint GetKeyStringValues(RtlStringArray* values);
    uint Initialize(const wchar_t* subKey, bool readOnly = false);
    uint Initialize(const wchar_t* subKey, void* transaction);
    uint IsKeyValuePresent(bool* present, const wchar_t* valueName);
    bool IsKeyValueStringDataExpected(const wchar_t* valueName, const wchar_t* expectedData);
    uint ReadKeyValueBinaryAlloc(uchar** data, ulong* size, const wchar_t* valueName);
    uint ReadKeyValueString(wchar_t* buffer, const wchar_t* valueName, ulong bufferSize);
    uint ReadKeyValueStringAlloc(wchar_t** stringValue, const wchar_t* valueName);
    void UnInitialize();
    uint WriteKeyValueBinary(const wchar_t* valueName, const uchar* data, ulong size);
    uint WriteKeyValueString(const wchar_t* valueName, const wchar_t* value);
    uint WriteKeyValueTimestamp(const wchar_t* valueName);

private:
    HKEY Key;
};

uint RegistryKey::ClearKey()
{
    uint result = RegDeleteTreeW(this->Key, nullptr);
    if (result > 0) {
        result = (result & 0xFFFF) | 0x80070000;
    }
    return result;
}

uint RegistryKey::GetKeyStringValues(RtlStringArray* values)
{
    uint result = 0;
    DWORD maxValueNameLen = 0;

    result = RegQueryInfoKeyW(this->Key, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                            nullptr, &maxValueNameLen, nullptr, nullptr, nullptr);
    if (result != 0) {
        if (result > 0) {
            result = (result & 0xFFFF) | 0x80070000;
        }
        return result;
    }

    maxValueNameLen++;
    wchar_t* valueName = (wchar_t*)HeapAlloc(GetProcessHeap(), 0, maxValueNameLen * sizeof(wchar_t));
    if (valueName == nullptr) {
        return 0x8007000E; // E_OUTOFMEMORY
    }

    for (DWORD index = 0; ; index++) {
        DWORD nameLength = maxValueNameLen;
        DWORD type = 0;
        
        result = RegEnumValueW(this->Key, index, valueName, &nameLength, nullptr, &type, nullptr, nullptr);
        if (result != 0) {
            if (result == ERROR_NO_MORE_ITEMS) {
                result = 0;
            }
            else if (result > 0) {
                result = (result & 0xFFFF) | 0x80070000;
            }
            break;
        }

        if (type == REG_SZ) {
            // Changed to use the correct number of arguments for Append
            result = values->Append(valueName, nameLength);
            if (result != 0) {
                break;
            }
        }
    }

    HeapFree(GetProcessHeap(), 0, valueName);
    return result;
}

uint RegistryKey::Initialize(const wchar_t* subKey, bool readOnly)
{
    if (this->Key != nullptr) {
        return 0x800704DF; // ERROR_USER_EXISTS
    }

    REGSAM access = readOnly ? KEY_READ : KEY_ALL_ACCESS;
    uint result = RegOpenKeyExW(HKEY_LOCAL_MACHINE, subKey, 0, access, &this->Key);
    
    if (result != 0) {
        this->Key = nullptr;
        if (result > 0) {
            result = (result & 0xFFFF) | 0x80070000;
        }
    }

    return result;
}

uint RegistryKey::Initialize(const wchar_t* subKey, void* transaction)
{
    if (this->Key != nullptr) {
        return 0x800704DF; // ERROR_USER_EXISTS
    }

    if (transaction == (void*)-1) {
        return 0x80070057; // E_INVALIDARG
    }

    uint result = RegCreateKeyTransactedW(HKEY_LOCAL_MACHINE, subKey, 0, nullptr, 0, 
                                        KEY_ALL_ACCESS, nullptr, &this->Key, nullptr, 
                                        (HANDLE)transaction, nullptr);
    if (result != 0) {
        this->Key = nullptr;
        if (result > 0) {
            result = (result & 0xFFFF) | 0x80070000;
        }
    }

    return result;
}

uint RegistryKey::IsKeyValuePresent(bool* present, const wchar_t* valueName)
{
    uint result = 0;
    *present = false;

    if (this->Key != nullptr) {
        result = RegGetValueW(this->Key, nullptr, valueName, RRF_RT_ANY, nullptr, nullptr, nullptr);
        if (result == 0) {
            *present = true;
        }
        else if (result > 0) {
            result = (result & 0xFFFF) | 0x80070000;
        }
        
        // Filter out ERROR_FILE_NOT_FOUND
        if (result == ERROR_FILE_NOT_FOUND) {
            result = 0;
        }
    }

    return result;
}

bool RegistryKey::IsKeyValueStringDataExpected(const wchar_t* valueName, const wchar_t* expectedData)
{
    bool result = false;
    wchar_t* storedValue = nullptr;

    // Changed the comparison since uint is unsigned
    if (this->ReadKeyValueStringAlloc(&storedValue, valueName) == 0) {
        result = (wcscmp(storedValue, expectedData) == 0);
    }

    if (storedValue != nullptr) {
        HeapFree(GetProcessHeap(), 0, storedValue);
    }

    return result;
}

uint RegistryKey::ReadKeyValueBinaryAlloc(uchar** data, ulong* size, const wchar_t* valueName)
{
    uint result = 0;
    DWORD dataSize = 0;

    *data = nullptr;
    *size = 0;

    result = RegGetValueW(this->Key, nullptr, valueName, RRF_RT_REG_BINARY, nullptr, nullptr, &dataSize);
    if (result != 0) {
        if (result > 0) {
            result = (result & 0xFFFF) | 0x80070000;
        }
        return result;
    }

    uchar* buffer = (uchar*)HeapAlloc(GetProcessHeap(), 0, dataSize);
    if (buffer == nullptr) {
        return 0x8007000E; // E_OUTOFMEMORY
    }

    result = RegGetValueW(this->Key, nullptr, valueName, RRF_RT_REG_BINARY, nullptr, buffer, &dataSize);
    if (result == 0) {
        *data = buffer;
        *size = dataSize;
    }
    else {
        if (result > 0) {
            result = (result & 0xFFFF) | 0x80070000;
        }
        HeapFree(GetProcessHeap(), 0, buffer);
    }

    return result;
}

uint RegistryKey::ReadKeyValueString(wchar_t* buffer, const wchar_t* valueName, ulong bufferSize)
{
    DWORD size = bufferSize * sizeof(wchar_t);
    uint result = RegGetValueW(this->Key, nullptr, valueName, RRF_RT_REG_SZ, nullptr, buffer, &size);
    
    if (result > 0) {
        result = (result & 0xFFFF) | 0x80070000;
    }

    return result;
}

uint RegistryKey::ReadKeyValueStringAlloc(wchar_t** stringValue, const wchar_t* valueName)
{
    uint result = 0;
    DWORD size = 0;

    *stringValue = nullptr;

    result = RegGetValueW(this->Key, nullptr, valueName, RRF_RT_REG_SZ, nullptr, nullptr, &size);
    if (result != 0) {
        if (result > 0) {
            result = (result & 0xFFFF) | 0x80070000;
        }
        return result;
    }

    wchar_t* buffer = (wchar_t*)HeapAlloc(GetProcessHeap(), 0, size);
    if (buffer == nullptr) {
        return 0x8007000E; // E_OUTOFMEMORY
    }

    result = this->ReadKeyValueString(buffer, valueName, size / sizeof(wchar_t));
    if (result == 0) {
        *stringValue = buffer;
    }
    else {
        HeapFree(GetProcessHeap(), 0, buffer);
    }

    return result;
}

void RegistryKey::UnInitialize()
{
    if (this->Key != nullptr) {
        RegCloseKey(this->Key);
        this->Key = nullptr;
    }
}

uint RegistryKey::WriteKeyValueBinary(const wchar_t* valueName, const uchar* data, ulong size)
{
    uint result = RegSetValueExW(this->Key, valueName, 0, REG_BINARY, data, size);
    
    if (result > 0) {
        result = (result & 0xFFFF) | 0x80070000;
    }

    return result;
}

uint RegistryKey::WriteKeyValueString(const wchar_t* valueName, const wchar_t* value)
{
    size_t size = (wcslen(value) + 1) * sizeof(wchar_t);
    uint result = RegSetValueExW(this->Key, valueName, 0, REG_SZ, (const BYTE*)value, size);
    
    if (result > 0) {
        result = (result & 0xFFFF) | 0x80070000;
    }

    return result;
}

uint RegistryKey::WriteKeyValueTimestamp(const wchar_t* valueName)
{
    FILETIME fileTime;
    GetSystemTimeAsFileTime(&fileTime);

    uint result = RegSetValueExW(this->Key, valueName, 0, REG_QWORD, (const BYTE*)&fileTime, sizeof(fileTime));
    
    if (result > 0) {
        result = (result & 0xFFFF) | 0x80070000;
    }

    return result;
}

} // namespace Shared
} // namespace Compat
} // namespace Windows

