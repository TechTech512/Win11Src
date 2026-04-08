#include <windows.h>

// DXCore API stub implementation
// This is a placeholder for systems where DXCore is not available

extern "C" {

HRESULT DXCoreCreateAdapterFactory(
    REFIID riid,           // Interface ID to create (typically IID_IDXCoreAdapterFactory)
    void** ppvFactory      // Receives the adapter factory interface pointer
)
{
    /* 
    0x1750  1  DXCoreCreateAdapterFactory
    */
    return E_NOTIMPL;  // 0x80004001 - Not implemented
}

} // extern "C"

