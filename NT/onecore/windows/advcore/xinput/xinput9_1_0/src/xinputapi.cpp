#include "xinputi.h"

// XInput structures and constants
typedef struct _XINPUT_GAMEPAD {
    WORD wButtons;
    BYTE bLeftTrigger;
    BYTE bRightTrigger;
    SHORT sThumbLX;
    SHORT sThumbLY;
    SHORT sThumbRX;
    SHORT sThumbRY;
} XINPUT_GAMEPAD, *PXINPUT_GAMEPAD;

typedef struct _XINPUT_STATE {
    DWORD dwPacketNumber;
    XINPUT_GAMEPAD Gamepad;
} XINPUT_STATE, *PXINPUT_STATE;

typedef struct _XINPUT_VIBRATION {
    WORD wLeftMotorSpeed;
    WORD wRightMotorSpeed;
} XINPUT_VIBRATION, *PXINPUT_VIBRATION;

typedef struct _XINPUT_CAPABILITIES {
    BYTE Type;
    BYTE SubType;
    WORD Flags;
    XINPUT_GAMEPAD Gamepad;
    XINPUT_VIBRATION Vibration;
} XINPUT_CAPABILITIES, *PXINPUT_CAPABILITIES;

// Global variables and function pointers
namespace {
    // Function pointers for XInput API
    typedef DWORD (__cdecl* XInputGetCapabilities_t)(DWORD dwUserIndex, DWORD dwFlags, XINPUT_CAPABILITIES* pCapabilities);
    typedef DWORD (__cdecl* XInputGetState_t)(DWORD dwUserIndex, XINPUT_STATE* pState);
    typedef DWORD (__cdecl* XInputSetState_t)(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration);
    
    XInputGetCapabilities_t fnGetCaps = nullptr;
    XInputGetState_t fnGetState = nullptr;
    XInputSetState_t fnSetState = nullptr;
    
    // GUID structure
    typedef struct _GUID {
        DWORD Data1;
        WORD  Data2;
        WORD  Data3;
        BYTE  Data4[8];
    } GUID;
    
    // GUID_NULL constant
    const GUID guidNull = {0x00000000, 0x0000, 0x0000, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
    
    // Default capabilities for XInput 9.1.0
    const XINPUT_CAPABILITIES CAPS_9_1_0 = {
        0,                      // Type
        1,                      // SubType (XINPUT_DEVSUBTYPE_GAMEPAD)
        0,                      // Flags
        {                       // Gamepad
            0xFFFF,             // wButtons
            0,                  // bLeftTrigger
            0,                  // bRightTrigger
            0,                  // sThumbLX
            0,                  // sThumbLY
            0,                  // sThumbRX
            0                   // sThumbRY
        },
        {                       // Vibration
            0,                  // wLeftMotorSpeed
            0                   // wRightMotorSpeed
        }
    };
    
    // Forward declaration
    DWORD TryLoadCurrentDll();
}

DWORD __cdecl XInputGetCapabilities(DWORD dwUserIndex, DWORD dwFlags, XINPUT_CAPABILITIES* pCapabilities)
{
    DWORD result = TryLoadCurrentDll();
    
    if (result == 0 && fnGetCaps != nullptr)
    {
        result = fnGetCaps(dwUserIndex, dwFlags, pCapabilities);
        
        // If successful and we need to fill with default capabilities
        if (result == 0)
        {
            // Copy the default capabilities structure
            *pCapabilities = CAPS_9_1_0;
        }
    }
    
    return result;
}

DWORD __cdecl XInputGetDSoundAudioDeviceGuids(DWORD dwUserIndex, void* pRenderGuid, void* pCaptureGuid)
{
    if (pRenderGuid == nullptr || pCaptureGuid == nullptr)
    {
        return ERROR_BAD_ARGUMENTS;  // ERROR_BAD_ARGUMENTS
    }
    
    // Cast to our GUID type
    typedef struct { DWORD Data1; WORD Data2; WORD Data3; BYTE Data4[8]; } MY_GUID;
    MY_GUID* pRender = (MY_GUID*)pRenderGuid;
    MY_GUID* pCapture = (MY_GUID*)pCaptureGuid;
    
    DWORD result = TryLoadCurrentDll();
    
    if (result == 0 && fnGetState != nullptr)
    {
        // Try to get state to see if controller is connected
        XINPUT_STATE state;
        ZeroMemory(&state, sizeof(XINPUT_STATE));
        
        result = fnGetState(dwUserIndex, &state);
    }
    
    return result;
}

DWORD __cdecl XInputGetState(DWORD dwUserIndex, XINPUT_STATE* pState)
{
    DWORD result = TryLoadCurrentDll();
    
    if (result == 0 && fnGetState != nullptr)
    {
        result = fnGetState(dwUserIndex, pState);
    }
    
    return result;
}

DWORD __cdecl XInputSetState(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
{
    DWORD result = TryLoadCurrentDll();
    
    if (result == 0 && fnSetState != nullptr)
    {
        result = fnSetState(dwUserIndex, pVibration);
    }
    
    return result;
}

// Helper function to load XInput DLL
namespace {
    DWORD TryLoadCurrentDll()
    {
        static bool initialized = false;
        
        if (!initialized)
        {
            // Load XInput1_4.dll (or appropriate version)
            HMODULE hXInput = LoadLibraryW(L"XInput1_4.dll");
            
            if (hXInput == nullptr)
            {
                hXInput = LoadLibraryW(L"XInput9_1_0.dll");
            }
            
            if (hXInput != nullptr)
            {
                // Get function pointers
                fnGetCaps = (XInputGetCapabilities_t)GetProcAddress(hXInput, "XInputGetCapabilities");
                fnGetState = (XInputGetState_t)GetProcAddress(hXInput, "XInputGetState");
                fnSetState = (XInputSetState_t)GetProcAddress(hXInput, "XInputSetState");
            }
            
            initialized = true;
        }
        
        return (fnGetState != nullptr) ? 0 : ERROR_DLL_NOT_FOUND;  // ERROR_DLL_NOT_FOUND
    }
}

