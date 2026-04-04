#include "stdafx.h"

// External functions not defined in this file
extern HRESULT WindowsCreateStringReference(PCWSTR source, UINT32 length, HSTRING_HEADER* header, HSTRING* hstring);
extern uintptr_t __security_cookie;

// GUID for the activation factory (from original decompilation)
static const GUID __GUID_0aacf7a4_5e1d_49df_8034_fb6a68bc5ed1 =
    { 0x0aacf7a4, 0x5e1d, 0x49df, { 0x80, 0x34, 0xfb, 0x6a, 0x68, 0xbc, 0x5e, 0xd1 } };

int __cdecl main()
{
    HRESULT hr;
    HSTRING classId = nullptr;
    HSTRING_HEADER header;
    void* factory = nullptr;
    uintptr_t cookie;

    // Security cookie setup
    cookie = __security_cookie ^ (uintptr_t)&cookie;

    hr = RoInitialize(RO_INIT_MULTITHREADED);
    if (hr >= 0)
    {
        hr = WindowsCreateStringReference(
            L"Windows.ApplicationModel.Core.CoreApplication",
            0x2d,
            &header,
            &classId
        );

        if (hr < 0)
        {
            RaiseException(0xc000000d, 1, 0, nullptr);
        }

        hr = RoGetActivationFactory(classId, __GUID_0aacf7a4_5e1d_49df_8034_fb6a68bc5ed1, (void**)factory);

        if (hr >= 0)
        {
            // Call method at offset 0x34 (vtable index 0xD)
            // Original: (*pcVar1)(piVar4, uVar5) where uVar5 = 0
            typedef HRESULT (STDMETHODCALLTYPE* RunMethod)(void* pThis, void* param);
            RunMethod runMethod = *(RunMethod*)(*(void***)factory + 0x34 / sizeof(void*));
            hr = runMethod(factory, nullptr);
        }

        if (factory)
        {
            // Release via vtable offset 8 (IUnknown::Release)
            typedef ULONG (STDMETHODCALLTYPE* ReleaseMethod)(void* pThis);
            ReleaseMethod releaseMethod = *(ReleaseMethod*)(*(void***)factory + 8 / sizeof(void*));
            releaseMethod(factory);
            factory = nullptr;
        }

        if (classId)
        {
            WindowsDeleteString(classId);
        }

        RoUninitialize();
    }

    __security_check_cookie(cookie);
    return (hr < 0) ? 1 : 0;
}

