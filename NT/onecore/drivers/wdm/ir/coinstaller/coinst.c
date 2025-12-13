#include <windows.h>
#include <setupapi.h>
#include <stdlib.h>
#include <string.h>

// GUID for System class
static const GUID SYSTEM_CLASS_GUID = { 0x4D36E97D, 0xE325, 0x11CE, {0xBF, 0xC1, 0x08, 0x00, 0x2B, 0xE1, 0x03, 0x18} };

// Custom structure to match original code layout (20 bytes total)
typedef struct _CUSTOM_CLASSINSTALL_DATA {
    SP_CLASSINSTALL_HEADER ClassInstallHeader;  // 8 bytes
    DWORD Flags;                                // 4 bytes
    DWORD DynamicFlags;                         // 4 bytes
    DWORD NumDynamicPages;                      // 4 bytes
} CUSTOM_CLASSINSTALL_DATA, *PCUSTOM_CLASSINSTALL_DATA;

// Function declarations
unsigned char FindClassDevice(void);
void InstallClassDriver(HDEVINFO hDevInfo, SP_DEVINFO_DATA* pDeviceInfoData, COINSTALLER_CONTEXT_DATA* pContext);

unsigned char FindClassDevice(void)
{
    unsigned char found = 0;
    HDEVINFO hDevInfo;
    SP_DEVINFO_DATA deviceInfoData;
    DWORD deviceIndex = 0;
    char deviceName[16];
    
    // Get device information set for System class devices under Root enumerator
    hDevInfo = SetupDiGetClassDevsA(&SYSTEM_CLASS_GUID, "Root", NULL, DIGCF_PRESENT);
    
    if (hDevInfo == INVALID_HANDLE_VALUE)
    {
        return 0;
    }
    
    deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    
    // Enumerate through all devices
    while (SetupDiEnumDeviceInfo(hDevInfo, deviceIndex, &deviceInfoData))
    {
        deviceIndex++;
        
        // Get device hardware ID
        if (SetupDiGetDeviceRegistryPropertyA(hDevInfo, &deviceInfoData, SPDRP_HARDWAREID, NULL, 
                                             (PBYTE)deviceName, sizeof(deviceName), NULL))
        {
            // Check if this is the CIRClass device
            if (_stricmp(deviceName, "ROOT\\CIRClass") == 0)
            {
                found = 1;
                break;
            }
        }
        else
        {
            // If we get ERROR_INSUFFICIENT_BUFFER, continue to next device
            if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            {
                break;
            }
        }
    }
    
    SetupDiDestroyDeviceInfoList(hDevInfo);
    return found;
}

void InstallClassDriver(HDEVINFO hDevInfo, SP_DEVINFO_DATA* pDeviceInfoData, COINSTALLER_CONTEXT_DATA* pContext)
{
    SP_DEVINFO_DATA newDeviceInfoData;
    CUSTOM_CLASSINSTALL_DATA customInstallData;
    
    newDeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    
    // Create device information for System device
    if (SetupDiCreateDeviceInfoA(hDevInfo, "System", &SYSTEM_CLASS_GUID, NULL, NULL, 
                                DICD_GENERATE_ID, &newDeviceInfoData))
    {
        // Set device hardware ID
        if (SetupDiSetDeviceRegistryPropertyA(hDevInfo, &newDeviceInfoData, SPDRP_HARDWAREID, 
                                             "ROOT\\CIRClass", (DWORD)strlen("ROOT\\CIRClass") + 1))
        {
            // Register the device
            if (SetupDiCallClassInstaller(DIF_REGISTERDEVICE, hDevInfo, &newDeviceInfoData))
            {
                // Set up custom install parameters to match original code
                // Original layout: 8 bytes header, then 4 DWORDs (0x12, 3, 2, 0)
                customInstallData.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
                customInstallData.ClassInstallHeader.InstallFunction = DIF_NEWDEVICEWIZARD_FINISHINSTALL;
                customInstallData.Flags = 0x12;        // Matches original 0x12
                customInstallData.DynamicFlags = 3;    // Matches original 3
                customInstallData.NumDynamicPages = 2; // Matches original 2
                
                // Note: There was also a 4th DWORD set to 0 in the original (total 20 bytes)
                
                // Set class install parameters with the custom structure
                if (SetupDiSetClassInstallParamsA(hDevInfo, &newDeviceInfoData, 
                                                 (SP_CLASSINSTALL_HEADER*)&customInstallData, 
                                                 sizeof(customInstallData)))
                {
                    // Complete the new device wizard installation
                    SetupDiCallClassInstaller(DIF_NEWDEVICEWIZARD_FINISHINSTALL, hDevInfo, &newDeviceInfoData);
                    return;  // Success - don't destroy the list
                }
            }
        }
    }
    
    // Cleanup on failure
    SetupDiDestroyDeviceInfoList(hDevInfo);
}

DWORD IRCoInstaller(DWORD InstallFunction, HDEVINFO DeviceInfoSet, SP_DEVINFO_DATA* DeviceInfoData, COINSTALLER_CONTEXT_DATA* Context)
{
    if (InstallFunction == DIF_FINISHINSTALL_ACTION)
    {
        DWORD installResult = Context->InstallResult;
        
        if (installResult != NO_ERROR)
        {
            return installResult;
        }
        
        // Check if CIRClass device already exists
        if (!FindClassDevice())
        {
            // Create device information list
            HDEVINFO hDevInfo = SetupDiCreateDeviceInfoList(&SYSTEM_CLASS_GUID, NULL);
            
            if (hDevInfo != INVALID_HANDLE_VALUE)
            {
                // Install the class driver
                InstallClassDriver(hDevInfo, DeviceInfoData, Context);
            }
        }
    }
    
    return NO_ERROR;
}

