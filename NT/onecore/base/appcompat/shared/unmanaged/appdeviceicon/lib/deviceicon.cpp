#include "precomp.h"
#include <setupapi.h>
#include <initguid.h>
#include <guiddef.h>

// Forward declarations for missing functions (implement as needed)
int GUIDFromString(const wchar_t* str, GUID* guid); // Suggest using CLSIDFromString
UINT CreatePngGraphic(HICON hIcon, const wchar_t* pngPath); // You must implement this

// Function to get the device class icon and convert it to PNG
UINT __cdecl GetDeviceClassIcon(const wchar_t* classGuidStr, wchar_t* outPngPath)
{
    UINT result = 0x80070057; // Default: E_INVALIDARG
    GUID classGuid;
    HICON hIcon = nullptr;
    size_t stringLength = 0;

    // Argument validation
    if (classGuidStr == nullptr || outPngPath == nullptr) {
        return result;
    }

    // Validate string length (GUID is 38 chars + null)
    if (FAILED(StringCchLengthW(classGuidStr, 39, &stringLength))) {
        return 0x80004005; // E_FAIL
    }

    // Parse GUID from string
    if (GUIDFromString(classGuidStr, &classGuid) != 0) {
        // Load class icon
        if (SetupDiLoadClassIcon(&classGuid, &hIcon, nullptr)) {
            // Convert icon to PNG
            result = CreatePngGraphic(hIcon, outPngPath);
        } else {
            result = GetLastError();
            if ((int)result > 0) {
                result = (result & 0xffff) | 0x80070000;
            }
        }
    } else {
        result = 0x80004005; // E_FAIL
    }

    if (hIcon) {
        DestroyIcon(hIcon);
    }

    return result;
}

