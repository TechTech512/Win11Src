#include <windows.h>
#include <string.h>

// Forward declarations for external functions that exist in other source files
extern "C" {
    ULONG AmipRegGetFileKey(HKEY__**, wchar_t*, void*, wchar_t*, int, int);
    ULONG AmiUtilityRegGetValue(void*, ULONG*, HKEY__*, wchar_t*, int);
    void AslLogCallPrintf(int, const char*, int, const char*, ...);
}

/**
 * Retrieves program ID and file ID metadata for a given file
 * 
 * @param filePath      Path to the file or registry key
 * @param fileHandle    File handle or additional path information
 * @param unused1       [Unused parameter]
 * @param unused2       [Unused parameter]
 * @param unused3       [Unused parameter]
 * @return ULONG        Error code (0 for success)
 */
ULONG AmiFileGetProgramIdFileId(
    wchar_t* filePath,
    wchar_t* fileHandle,
    void* unused1,
    wchar_t* unused2,
    void* unused3)
{
    ULONG result;
    wchar_t* storeContext = nullptr;  // Originally in_ECX
    unsigned short* outputBuffer = nullptr;  // Originally in_EDX
    int esiValue = 0;  // Originally unaff_ESI
    int ediValue = 0;  // Originally unaff_EDI

    if (filePath == nullptr) {
        result = 0x57;  // ERROR_INVALID_PARAMETER
        AslLogCallPrintf(1, "AmiFileGetProgramIdFileId", 0xd5, "Invalid parameters passed");
        return result;
    }

    if (fileHandle == nullptr) {
        result = 0x57;  // ERROR_INVALID_PARAMETER
        AslLogCallPrintf(1, "AmiFileGetProgramIdFileId", 0xed,
                        "Either AmiFileHandle or FilePath should be specified [%d]");
        return result;
    }

    // Get file key from registry
    result = AmipRegGetFileKey(reinterpret_cast<HKEY__**>(filePath), 
                             fileHandle,
                             storeContext,
                             storeContext,  // Originally in_ECX reused
                             ediValue,
                             esiValue);

    if (result == 0) {
        if (outputBuffer != nullptr) {
            // Clear output buffer
            memset(outputBuffer, 0, sizeof(*outputBuffer));

            // Get registry value
            result = AmiUtilityRegGetValue(
                nullptr,
                reinterpret_cast<ULONG*>(L"101"),
                nullptr,
                filePath,
                static_cast<int>(reinterpret_cast<uintptr_t>(fileHandle)));

            if (result != 0) {
                if (result != 2 && result != 5 && result != 0x20) {
                    AslLogCallPrintf(1, "AmiFileGetProgramIdFileId", 0x10c,
                                    "Could not get file property for %S [%d]", filePath, result);
                }
                *outputBuffer = 0;
            }
        }
        result = 0;  // Success
    }
    else if (result != 2 && result != 5 && result != 0x7b && result != 0x20) {
        AslLogCallPrintf(1, "AmiFileGetProgramIdFileId", 0xf6,
                        "Could not get file metadata for %S [%d]", filePath, result);
    }

    return result;
}

