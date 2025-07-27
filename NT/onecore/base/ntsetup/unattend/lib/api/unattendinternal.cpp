#include <windows.h>
#include <wchar.h>
#include <vector>
#include <memory>
#include <string>

typedef long NTSTATUS;

using namespace std;

struct Unattend;

struct UnattendNode {
    struct {
        void* Reserved;
        void* Padding[2];
    } inner;
};

class IPathIterator {
public:
    virtual ~IPathIterator() {}
    virtual int OnNode(Unattend* unattend, UnattendNode node) = 0;  // Changed to pass by value
    virtual int OnAttribute(Unattend* unattend, UnattendNode node,  // Changed to pass by value
                          const wchar_t* name, const wchar_t* value) = 0;
};

struct Unattend {
    void* pMicrodom;

    static int GetRootNode(Unattend* unattend, UnattendNode* node) {
        if (!unattend || !node) return E_INVALIDARG;
        node->inner.Reserved = unattend->pMicrodom;
        return S_OK;
    }

    static int GetName(Unattend* unattend, UnattendNode* node, wchar_t** name) {
        if (!unattend || !node || !name) return E_INVALIDARG;
        
        // Simplified implementation - actual code would query the microdom
        *name = _wcsdup(L"RootNode");
        return (*name) ? S_OK : E_OUTOFMEMORY;
    }

    static int CountChildren(Unattend* unattend, UnattendNode* node, size_t* count) {
        if (!unattend || !node || !count) return E_INVALIDARG;
        
        // Simplified implementation
        *count = 0;
        return S_OK;
    }

    static int GetChild(Unattend* unattend, UnattendNode* parent, size_t index, UnattendNode* child) {
        if (!unattend || !parent || !child) return E_INVALIDARG;
        return E_NOTIMPL; // Simplified
    }

    static int GetAttributeValueByName(Unattend* unattend, UnattendNode* node, 
                                     const wchar_t* name, wchar_t** value, UnattendNode* attrNode) {
        if (!unattend || !node || !name || !value) return E_INVALIDARG;
        
        // Simplified implementation
        if (wcscmp(name, L"SampleAttr") == 0) {
            *value = _wcsdup(L"SampleValue");
            return (*value) ? S_OK : E_OUTOFMEMORY;
        }
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    }
};

template<typename T>
class AutoHeapPtr {
    T* ptr;
public:
    AutoHeapPtr(T* p = nullptr) : ptr(p) {}
    ~AutoHeapPtr() { if (ptr) HeapFree(GetProcessHeap(), 0, ptr); }
    operator T*() { return ptr; }
    T** operator&() { return &ptr; }
};

int NextPathElement(wchar_t** path, wchar_t** element) {
    if (!path || !*path || !element) {
        return E_INVALIDARG;
    }

    wchar_t* current = *path;
    while (*current == L'\\') current++;

    if (*current == L'\0') {
        return 1; // No more elements
    }

    const wchar_t* end = wcschr(current, L'\\');
    if (!end) {
        end = current + wcslen(current);
    }

    size_t len = end - current;
    wchar_t* newElement = (wchar_t*)HeapAlloc(GetProcessHeap(), 0, (len + 1) * sizeof(wchar_t)); 
    if (!newElement) {
        return E_OUTOFMEMORY;
    }

    wcsncpy_s(newElement, len + 1, current, len);
    newElement[len] = L'\0';
    *element = newElement;
    *path = const_cast<wchar_t*>((*end == L'\0') ? end : end + 1);

    return S_OK;
}

int CheckRootNodeAgainstPath(Unattend* unattend, UnattendNode* rootNode, wchar_t** path) {
    if (!unattend || !rootNode || !path) {
        return E_INVALIDARG;
    }

    AutoHeapPtr<wchar_t> nodeName;
    int result = Unattend::GetRootNode(unattend, rootNode);
    if (FAILED(result)) return result;

    result = Unattend::GetName(unattend, rootNode, &nodeName);
    if (FAILED(result)) return result;

    wchar_t* pathElement = nullptr;
    result = NextPathElement(path, &pathElement);
    
    if (result == 1) {
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    }
    if (FAILED(result)) return result;

    result = CompareStringW(LOCALE_INVARIANT, NORM_IGNORECASE, 
                          nodeName, -1, pathElement, -1);
    HeapFree(GetProcessHeap(), 0, pathElement);

    return (result == CSTR_EQUAL) ? S_OK : HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
}

extern "C" {
    DWORD WINAPI RtlNtStatusToDosError(NTSTATUS Status);
}

HRESULT ConvertNtStatusToHresult(NTSTATUS status) {
    DWORD error = RtlNtStatusToDosError(status);
    if (error != ERROR_MR_MID_NOT_FOUND) {
        return HRESULT_FROM_WIN32(error);
    }
    return status;
}

HRESULT FreeStringArray(wchar_t** stringArray, size_t count) {
    if (!stringArray || count == 0) {
        return S_OK;
    }

    HANDLE heap = GetProcessHeap();
    for (size_t i = 0; i < count; i++) {
        if (stringArray[i]) {
            HeapFree(heap, 0, stringArray[i]);
        }
    }
    HeapFree(heap, 0, stringArray);
    return S_OK;
}

HRESULT FreeUtf8(void* utf8String) {
    if (!utf8String) return E_INVALIDARG;
    HeapFree(GetProcessHeap(), 0, utf8String);
    return S_OK;
}


int IteratePath(wchar_t* path, Unattend* unattend, UnattendNode node, IPathIterator* iterator) {
    if (!path || !unattend || !iterator) {
        return E_INVALIDARG;
    }

    wchar_t* pathElement = nullptr;
    int result = NextPathElement(&path, &pathElement);
    
    if (result == 1) { // No more elements
        return iterator->OnNode(unattend, node);  // Matches interface definition
    }
    if (FAILED(result)) {
        return result;
    }

    // Handle path element and continue iteration
    AutoHeapPtr<wchar_t> elementCleanup(pathElement);
    return iterator->OnNode(unattend, node);  // Matches interface definition
}

int IteratePathFirstSubNodeWithAttrValue(
    wchar_t* nodeName,
    wchar_t** attrNames,
    wchar_t** attrValues,
    size_t attrCount,
    wchar_t* path,
    Unattend* unattend,
    UnattendNode parentNode,
    IPathIterator* iterator) 
{
    if (!nodeName || !unattend || !path || !parentNode.inner.Reserved || !iterator) {
        return E_INVALIDARG;
    }

    size_t childCount = 0;
    int result = Unattend::CountChildren(unattend, &parentNode, &childCount);
    if (FAILED(result)) return result;

    for (size_t i = 0; i < childCount; i++) {
        UnattendNode childNode;
        result = Unattend::GetChild(unattend, &parentNode, i, &childNode);
        if (FAILED(result)) continue;

        AutoHeapPtr<wchar_t> childName;
        result = Unattend::GetName(unattend, &childNode, &childName);
        if (FAILED(result) || !childName) continue;

        if (CompareStringW(LOCALE_INVARIANT, NORM_IGNORECASE, 
                         nodeName, -1, childName, -1) == CSTR_EQUAL) {
            bool matchesAll = true;
            
            for (size_t a = 0; a < attrCount && matchesAll; a++) {
                wchar_t* attrValue = nullptr;
                result = Unattend::GetAttributeValueByName(
                    unattend, &childNode, attrNames[a], &attrValue, nullptr);
                
                if (SUCCEEDED(result)) {
                    if (attrValues[a] && 
                        CompareStringW(LOCALE_INVARIANT, NORM_IGNORECASE,
                                       attrValues[a], -1, attrValue, -1) != CSTR_EQUAL) {
                        matchesAll = false;
                    }
                }
                
                if (attrValue) HeapFree(GetProcessHeap(), 0, attrValue);
            }

            if (matchesAll && SUCCEEDED(result)) {
                result = IteratePath(path, unattend, childNode, iterator);
                if (result != S_OK) {
                    break;
                }
            }
        }
    }

    return (result == S_OK && childCount > 0) ? S_OK : HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
}

HRESULT UnicodeToUtf8(const wchar_t* unicodeStr, char** utf8Str) {
    if (!unicodeStr || !utf8Str) return E_INVALIDARG;
    
    int utf8Len = WideCharToMultiByte(CP_UTF8, 0, unicodeStr, -1, nullptr, 0, nullptr, nullptr);
    if (utf8Len == 0) return HRESULT_FROM_WIN32(GetLastError());
    
    *utf8Str = (char*)HeapAlloc(GetProcessHeap(), 0, utf8Len);
    if (!*utf8Str) return E_OUTOFMEMORY;
    
    if (WideCharToMultiByte(CP_UTF8, 0, unicodeStr, -1, *utf8Str, utf8Len, nullptr, nullptr) == 0) {
        HeapFree(GetProcessHeap(), 0, *utf8Str);
        return HRESULT_FROM_WIN32(GetLastError());
    }
    
    return S_OK;
}

HRESULT Utf8ToUnicode(const char* utf8Str, wchar_t** unicodeStr) {
    if (!utf8Str || !unicodeStr) return E_INVALIDARG;
    
    int unicodeLen = MultiByteToWideChar(CP_UTF8, 0, utf8Str, -1, nullptr, 0);
    if (unicodeLen == 0) return HRESULT_FROM_WIN32(GetLastError());
    
    *unicodeStr = (wchar_t*)HeapAlloc(GetProcessHeap(), 0, unicodeLen * sizeof(wchar_t));
    if (!*unicodeStr) return E_OUTOFMEMORY;
    
    if (MultiByteToWideChar(CP_UTF8, 0, utf8Str, -1, *unicodeStr, unicodeLen) == 0) {
        HeapFree(GetProcessHeap(), 0, *unicodeStr);
        return HRESULT_FROM_WIN32(GetLastError());
    }
    
    return S_OK;
}

HRESULT ReadFileFully(const wchar_t* filePath, BYTE** buffer, DWORD* fileSize) {
    if (!filePath || !buffer || !fileSize) return E_INVALIDARG;
    
    HANDLE hFile = CreateFileW(filePath, GENERIC_READ, FILE_SHARE_READ, nullptr, 
                             OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    
    LARGE_INTEGER size;
    if (!GetFileSizeEx(hFile, &size)) {
        CloseHandle(hFile);
        return HRESULT_FROM_WIN32(GetLastError());
    }
    
    if (size.HighPart != 0 || size.LowPart == 0) {
        CloseHandle(hFile);
        return E_FAIL;
    }
    
    *buffer = (BYTE*)HeapAlloc(GetProcessHeap(), 0, size.LowPart);
    if (!*buffer) {
        CloseHandle(hFile);
        return E_OUTOFMEMORY;
    }
    
    DWORD bytesRead;
    if (!ReadFile(hFile, *buffer, size.LowPart, &bytesRead, nullptr) || bytesRead != size.LowPart) {
        HeapFree(GetProcessHeap(), 0, *buffer);
        CloseHandle(hFile);
        return HRESULT_FROM_WIN32(GetLastError());
    }
    
    *fileSize = size.LowPart;
    CloseHandle(hFile);
    return S_OK;
}

HRESULT TrimWhitespace(const wchar_t* input, wchar_t** output) {
    if (!input || !output) return E_INVALIDARG;
    
    // Find first non-whitespace
    const wchar_t* start = input;
    while (*start && iswspace(*start)) start++;
    
    if (*start == L'\0') {
        *output = _wcsdup(L"");
        return (*output) ? S_OK : E_OUTOFMEMORY;
    }
    
    // Find last non-whitespace
    const wchar_t* end = start + wcslen(start) - 1;
    while (end > start && iswspace(*end)) end--;
    
    size_t len = end - start + 1;
    *output = (wchar_t*)HeapAlloc(GetProcessHeap(), 0, (len + 1) * sizeof(wchar_t));
    if (!*output) return E_OUTOFMEMORY;
    
    wcsncpy_s(*output, len + 1, start, len);
    (*output)[len] = L'\0';
    return S_OK;
}

