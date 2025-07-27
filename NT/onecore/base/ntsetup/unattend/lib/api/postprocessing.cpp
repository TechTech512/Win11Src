#include <windows.h>
#include <wchar.h>
#include <memory>

// Forward declarations
struct _UNATTEND_CONTEXT;

// Helper for auto heap string cleanup
class AutoHeapString {
    wchar_t* ptr;
public:
    AutoHeapString(wchar_t* p = nullptr) : ptr(p) {}
    ~AutoHeapString() { if (ptr) HeapFree(GetProcessHeap(), 0, ptr); }
    operator wchar_t*() { return ptr; }
    wchar_t** operator&() { return &ptr; }
    wchar_t* detach() { wchar_t* temp = ptr; ptr = nullptr; return temp; }
};

// External dependencies
extern "C" {
    int StrAllocatingPrintf(wchar_t** output, const wchar_t* format, ...);
    int UnattendCtxGetFlag(_UNATTEND_CONTEXT* context, const wchar_t* path, 
                          const wchar_t* attribute, int defaultValue, int* result);
    int UnattendCtxSetString(_UNATTEND_CONTEXT* context, const wchar_t* path, 
                            const wchar_t* attribute, const wchar_t* value);
    extern const struct {
        const wchar_t* lpszFriendlyName;
    } c_passNames[];
}

HRESULT UnattendIsPassUnusedInCtx(_UNATTEND_CONTEXT* context, const wchar_t* passName, int* isUnused) {
    if (!context || !passName || !isUnused) {
        return E_INVALIDARG;
    }

    AutoHeapString xpath;
    *isUnused = 0;
    
    // Create XPath query for the pass
    HRESULT hr = StrAllocatingPrintf(&xpath, L"unattend\\settings[pass=%s]", passName);
    if (FAILED(hr)) {
        return hr;
    }

    // Check if pass was processed
    int wasProcessed = 0;
    hr = UnattendCtxGetFlag(context, xpath, L"wasPassProcessed", 0, &wasProcessed);
    if (SUCCEEDED(hr)) {
        *isUnused = (wasProcessed == 0) ? 1 : 0;
    }

    return hr;
}

HRESULT UnattendMarkPassUsedInCtx(_UNATTEND_CONTEXT* context, const wchar_t* passName) {
    if (!context || !passName) {
        return E_INVALIDARG;
    }

    AutoHeapString xpath;
    
    // Create XPath query for the pass
    HRESULT hr = StrAllocatingPrintf(&xpath, L"unattend\\settings[pass=%s]", passName);
    if (FAILED(hr)) {
        return hr;
    }

    // Mark pass as processed
    return UnattendCtxSetString(context, xpath, L"wasPassProcessed", L"true");
}

HRESULT UnattendUsedPassesExistInCtx(_UNATTEND_CONTEXT* context, int* hasUsedPasses) {
    if (!context || !hasUsedPasses) {
        return E_INVALIDARG;
    }

    *hasUsedPasses = 0;
    const int PASS_COUNT = 56; // 0x38 in hex, based on original code's 0x37 comparison
    
    for (int i = 0; i < PASS_COUNT; i++) {
        int isUnused = 0;
        HRESULT hr = UnattendIsPassUnusedInCtx(context, c_passNames[i].lpszFriendlyName, &isUnused);
        if (FAILED(hr)) {
            return hr;
        }

        if (!isUnused) {
            *hasUsedPasses = 1;
            break; // Found at least one used pass
        }
    }

    return S_OK;
}

