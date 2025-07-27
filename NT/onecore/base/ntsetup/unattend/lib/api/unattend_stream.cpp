#include <windows.h>
#include <wchar.h>
#include <objbase.h>
#include <memory>

// Forward declarations
struct _UNATTEND_CONTEXT;
struct _UNATTEND_NODE;

// Helper for automatic COM release with proper operator->
template<typename T>
class AutoRelease {
    T* ptr;
public:
    AutoRelease(T* p = nullptr) : ptr(p) {}
    ~AutoRelease() { if (ptr) ptr->Release(); }
    operator T*() { return ptr; }
    T** operator&() { return &ptr; }
    T* operator->() { return ptr; }
    T* detach() { T* temp = ptr; ptr = nullptr; return temp; }
};

// RAII wrapper for heap-allocated strings
template<typename T>
class AutoHeapPtr {
    T* ptr;
public:
    AutoHeapPtr(T* p = nullptr) : ptr(p) {}
    ~AutoHeapPtr() { if (ptr) HeapFree(GetProcessHeap(), 0, ptr); }
    operator T*() { return ptr; }
    T** operator&() { return &ptr; }
    T* detach() { T* temp = ptr; ptr = nullptr; return temp; }
};

// Forward declarations for functions we'll use
HRESULT UnattendCtxSerializeToBuffer(_UNATTEND_CONTEXT* context, wchar_t** buffer, ULONG* bufferSize);
HRESULT UnattendCtxSerializeToBufferFromNode(_UNATTEND_CONTEXT* context, _UNATTEND_NODE* node, 
                                            wchar_t** buffer, ULONG* bufferSize);

HRESULT ConvertUnicodeStringToStreamWithBOM(const wchar_t* unicodeStr, IStream** stream) {
    if (!unicodeStr || !stream) {
        return E_INVALIDARG;
    }

    // Create a memory stream
    AutoRelease<IStream> memStream;
    HRESULT hr = CreateStreamOnHGlobal(nullptr, TRUE, &memStream);
    if (FAILED(hr)) {
        return hr;
    }

    // Write UTF-16 LE BOM (0xFEFF)
    const WORD utf16BOM = 0xFEFF;
    ULONG bytesWritten = 0;
    hr = memStream->Write(&utf16BOM, sizeof(utf16BOM), &bytesWritten);
    if (FAILED(hr)) {
        return hr;
    }

    // Skip BOM if already present in input
    const wchar_t* strToWrite = unicodeStr;
    if (unicodeStr[0] == 0xFEFF) {
        strToWrite++;
    }

    // Write the string content
    size_t strLen = wcslen(strToWrite);
    size_t bytesToWrite = strLen * sizeof(wchar_t);
    hr = memStream->Write(strToWrite, static_cast<ULONG>(bytesToWrite), &bytesWritten);
    if (FAILED(hr)) {
        return hr;
    }

    // Rewind stream to beginning
    LARGE_INTEGER zero = {0};
    hr = memStream->Seek(zero, STREAM_SEEK_SET, nullptr);
    if (FAILED(hr)) {
        return hr;
    }

    // Transfer ownership to output parameter
    *stream = memStream.detach();
    return S_OK;
}

HRESULT UnattendCtxSerializeToStream(_UNATTEND_CONTEXT* context, IStream** stream) {
    if (!stream) {
        return E_INVALIDARG;
    }

    // Serialize to buffer first
    wchar_t* buffer = nullptr;
    ULONG bufferSize = 0;
    HRESULT hr = UnattendCtxSerializeToBuffer(context, &buffer, &bufferSize);
    if (FAILED(hr)) {
        return hr;
    }

    // Convert buffer to stream
    AutoHeapPtr<wchar_t> bufferCleanup(buffer);
    return ConvertUnicodeStringToStreamWithBOM(buffer, stream);
}

HRESULT UnattendCtxSerializeToStreamFromNode(
    _UNATTEND_CONTEXT* context,
    _UNATTEND_NODE* node,
    IStream** stream) 
{
    if (!stream) {
        return E_INVALIDARG;
    }

    // Serialize to buffer first
    wchar_t* buffer = nullptr;
    ULONG bufferSize = 0;
    HRESULT hr = UnattendCtxSerializeToBufferFromNode(context, node, &buffer, &bufferSize);
    if (FAILED(hr)) {
        return hr;
    }

    // Convert buffer to stream
    AutoHeapPtr<wchar_t> bufferCleanup(buffer);
    return ConvertUnicodeStringToStreamWithBOM(buffer, stream);
}

