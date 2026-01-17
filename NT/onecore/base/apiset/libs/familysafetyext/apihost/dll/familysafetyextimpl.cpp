// familysafetyextimpl.cpp

#include <Windows.h>
#include <wrl.h>
#include <wrl/client.h>
#include <wrl/implements.h>
#include <wil/resource.h>
#include <wil/result.h>
#include <windowsstringp.h>

// Add at the top of the file after includes
void* __cdecl operator new(size_t size)
{
    return HeapAlloc(GetProcessHeap(), 0, size);
}

void __cdecl operator delete(void* ptr)
{
    if (ptr)
        HeapFree(GetProcessHeap(), 0, ptr);
}

void __cdecl operator delete(void* ptr, unsigned int)
{
    if (ptr)
        HeapFree(GetProcessHeap(), 0, ptr);
}

void __cdecl operator delete[](void* ptr)
{
    if (ptr)
        HeapFree(GetProcessHeap(), 0, ptr);
}

void __cdecl operator delete[](void* ptr, unsigned int)
{
    if (ptr)
        HeapFree(GetProcessHeap(), 0, ptr);
}

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::Foundation;

struct __declspec(uuid("0f2495e9-edd6-46ef-a1f3-36713f4b5114")) IFamilySettingsStatics : public IUnknown
{
    // Vtable will be set up by COM
};

struct ComPtrHelper : public Microsoft::WRL::ComPtr<IFamilySettingsStatics>
{
    static void InternalRelease(Microsoft::WRL::ComPtr<IFamilySettingsStatics>* ptr)
    {
        // Cast to access protected member
        static_cast<ComPtrHelper*>(ptr)->Microsoft::WRL::ComPtr<IFamilySettingsStatics>::InternalRelease();
    }
};

extern "C" char* __cdecl IsChildAccount(HSTRING__* param_1, int* param_2)
{
    // Local variables matching the original structure
    IFamilySettingsStatics* familySettingsStatics = nullptr;
    boolean isChildAccount = FALSE;
    HRESULT hr = S_OK;
    wchar_t className[45] = L"Windows.FamilySafety.Internal.FamilySettings";
    
    // Create StringReference for the class name
	Windows::Internal::StringReference classId(className);
    
    // Get the activation factory
    hr = GetActivationFactory(classId._hstring, &familySettingsStatics);
    
    if (FAILED(hr))
    {
        // Log the failure with original file and line information
        wil::details::in1diag3::_Log_Hr(
            reinterpret_cast<void*>(_ReturnAddress()),
            0x1a,
            "onecore\\base\\apiset\\libs\\familysafetyext\\apihost\\dll\\familysafetyextimpl.cpp",
            hr
        );
    }
    else
    {
        // Call the IsChildAccount method through the vtable
        // The original shows offset 0x20 in the vtable
        hr = (*(HRESULT(__stdcall**)(void*, boolean*))(*((uintptr_t*)familySettingsStatics) + 0x20))(familySettingsStatics, &isChildAccount);
        
        if (SUCCEEDED(hr))
        {
            *param_2 = (isChildAccount != FALSE) ? 1 : 0;
            goto cleanup;
        }
        else
        {
            // Log the failure
            wil::details::in1diag3::_Log_Hr(
                reinterpret_cast<void*>(_ReturnAddress()),
                0x2d,
                reinterpret_cast<const char*>(param_1),
                hr
            );
        }
    }

cleanup:
    // Release the ComPtr (automatically handled by destructor)
    // The original calls InternalRelease
    ComPtrHelper::InternalRelease((Microsoft::WRL::ComPtr<IFamilySettingsStatics>*)&familySettingsStatics);
    
    return reinterpret_cast<char*>(hr);
}

