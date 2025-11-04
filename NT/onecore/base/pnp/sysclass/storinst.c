#include <windows.h>
#include <setupapi.h>
#include <cfg.h>
#include <cfgmgr32.h>
#include <guiddef.h>
#include <devguid.h>

// GUID_DEVCLASS_TAPEDRIVE definition
const GUID GUID_DEVCLASS_TAPEDRIVE = 
{ 0x6D807884, 0x7D21, 0x11CF, { 0x90, 0x1C, 0x08, 0x00, 0x3E, 0x30, 0x1F, 0x73 } };

// External function declarations
extern void *pSetupMalloc(DWORD size);
extern void pSetupFree(void *ptr);

// Function prototypes
unsigned char __cdecl CopyKey(HKEY hSourceKey, HKEY hTargetKey);
unsigned char __cdecl OverrideFriendlyNameForTape(HDEVINFO DeviceInfoSet, PSP_DEVINFO_DATA DeviceInfoData);
DWORD __cdecl StorageCoInstaller(DWORD InstallFunction, HDEVINFO DeviceInfoSet, PSP_DEVINFO_DATA DeviceInfoData, PCOINSTALLER_CONTEXT_DATA ContextData);
unsigned char __cdecl StorageCopyDeviceSettings(HDEVINFO DeviceInfoSet, PSP_DEVINFO_DATA DeviceInfoData, HKEY hTargetKey);

unsigned char __cdecl CopyKey(HKEY hSourceKey, HKEY hTargetKey)
{
    unsigned char bResult;
    DWORD dwIndex;
    DWORD dwValueNameSize;
    DWORD dwDataSize;
    DWORD dwType;
    DWORD dwDisposition;
    DWORD cValues;
    DWORD cMaxValueNameLen;
    DWORD cMaxValueLen;
    DWORD cSubKeys;
    DWORD cMaxSubKeyLen;
    WCHAR *pszValueName;
    BYTE *pbData;
    HKEY hSubKey;
    HKEY hNewSubKey;
    
    bResult = FALSE;
    
    if (RegQueryInfoKeyW(hSourceKey, NULL, NULL, NULL, &cSubKeys, &cMaxSubKeyLen, NULL, 
                        &cValues, &cMaxValueNameLen, &cMaxValueLen, NULL, NULL) != ERROR_SUCCESS) {
        return FALSE;
    }
    
    dwValueNameSize = (cMaxValueNameLen + 1) * sizeof(WCHAR);
    pszValueName = (WCHAR*)pSetupMalloc(dwValueNameSize);
    if (pszValueName == NULL) {
        return FALSE;
    }
    
    if (cMaxValueLen == 0) {
        pbData = NULL;
        dwDataSize = 0;
    } else {
        dwDataSize = cMaxValueLen;
        pbData = (BYTE*)pSetupMalloc(dwDataSize);
        if (pbData == NULL) {
            pSetupFree(pszValueName);
            return FALSE;
        }
    }
    
    // Copy values
    for (dwIndex = 0; dwIndex < cValues; dwIndex++) {
        DWORD dwTempValueNameSize = dwValueNameSize / sizeof(WCHAR);
        DWORD dwTempDataSize = dwDataSize;
        
        if (RegEnumValueW(hSourceKey, dwIndex, pszValueName, &dwTempValueNameSize, 
                         NULL, &dwType, pbData, &dwTempDataSize) == ERROR_SUCCESS) {
            RegSetValueExW(hTargetKey, pszValueName, 0, dwType, pbData, dwTempDataSize);
        }
    }
    
    if (pbData != NULL) {
        pSetupFree(pbData);
    }
    
    // Copy subkeys
    for (dwIndex = 0; dwIndex < cSubKeys; dwIndex++) {
        DWORD dwSubKeyNameSize = (cMaxSubKeyLen + 1);
        
        if (RegEnumKeyExW(hSourceKey, dwIndex, pszValueName, &dwSubKeyNameSize, 
                         NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
            if (RegOpenKeyExW(hSourceKey, pszValueName, 0, KEY_READ, &hSubKey) == ERROR_SUCCESS) {
                if (RegCreateKeyExW(hTargetKey, pszValueName, 0, NULL, 0, 
                                   KEY_WRITE, NULL, &hNewSubKey, &dwDisposition) == ERROR_SUCCESS) {
                    CopyKey(hSubKey, hNewSubKey);
                    RegCloseKey(hNewSubKey);
                }
                RegCloseKey(hSubKey);
            }
        }
    }
    
    pSetupFree(pszValueName);
    bResult = TRUE;
    
    return bResult;
}

unsigned char __cdecl OverrideFriendlyNameForTape(HDEVINFO DeviceInfoSet, PSP_DEVINFO_DATA DeviceInfoData)
{
    unsigned char bResult;
    HINF hInf;
    DWORD dwLastError;
    SP_DRVINFO_DATA_W driverInfoData;
    SP_DRVINFO_DETAIL_DATA_W driverInfoDetail;
    WCHAR szSectionName[256];
    INFCONTEXT context;
    DWORD dwUseInfDesc;
    
    bResult = FALSE;
    
    memset(&driverInfoData, 0, sizeof(driverInfoData));
    driverInfoData.cbSize = sizeof(driverInfoData);
    
    if (!SetupDiGetSelectedDriverW(DeviceInfoSet, DeviceInfoData, &driverInfoData)) {
        return FALSE;
    }
    
    memset(&driverInfoDetail, 0, sizeof(driverInfoDetail));
    driverInfoDetail.cbSize = sizeof(driverInfoDetail);
    
    if (!SetupDiGetDriverInfoDetailW(DeviceInfoSet, DeviceInfoData, &driverInfoData, 
                                    &driverInfoDetail, sizeof(driverInfoDetail), NULL)) {
        dwLastError = GetLastError();
        if (dwLastError != ERROR_INSUFFICIENT_BUFFER) {
            return FALSE;
        }
    }
    
    hInf = SetupOpenInfFileW(driverInfoDetail.InfFileName, NULL, INF_STYLE_WIN4, NULL);
    if (hInf != INVALID_HANDLE_VALUE) {
        memset(szSectionName, 0, sizeof(szSectionName));
        SetupDiGetActualSectionToInstallW(hInf, driverInfoDetail.SectionName, 
                                         szSectionName, sizeof(szSectionName)/sizeof(WCHAR), NULL, NULL);
        
        if (SetupFindFirstLineW(hInf, szSectionName, L"UseInfDeviceDesc", &context)) {
            dwUseInfDesc = 0;
            if (SetupGetIntField(&context, 1, (PINT)&dwUseInfDesc) && dwUseInfDesc != 0) {
                SetupDiSetDeviceRegistryPropertyW(DeviceInfoSet, DeviceInfoData, 
                                                 SPDRP_FRIENDLYNAME, NULL, 0);
                bResult = TRUE;
            }
        }
        SetupCloseInfFile(hInf);
    }
    
    return bResult;
}

DWORD __cdecl StorageCoInstaller(DWORD dwInstallFunction, HDEVINFO DeviceInfoSet, 
                                PSP_DEVINFO_DATA DeviceInfoData, 
                                PCOINSTALLER_CONTEXT_DATA ContextData)
{
    int *pPrivateData;
    unsigned char bTapeOverride;
    DWORD dwResult;
    HKEY hDeviceKey;
    int i;
    DWORD dwStatus;
    DWORD dwProblem;
    DWORD dwPropertySize;
    WCHAR *pszFriendlyName;
    
    dwResult = 0;
    
    if (dwInstallFunction == DIF_INSTALLDEVICE) {
        if (ContextData->PostProcessing == 0) {
            if (DeviceInfoData == NULL || 
                CM_Get_DevNode_Status(&dwStatus, &dwProblem, DeviceInfoData->DevInst, 0) != CR_SUCCESS ||
                (dwStatus & DN_HAS_PROBLEM) != 0) {
                dwResult = 0;
            } else {
                pPrivateData = (int*)pSetupMalloc(8);
                if (pPrivateData == NULL) {
                    dwResult = 0;
                } else {
                    pPrivateData[0] = 0;
                    pPrivateData[1] = (int)INVALID_HANDLE_VALUE;
                    
                    hDeviceKey = SetupDiCreateDevRegKeyW(DeviceInfoSet, DeviceInfoData, 
                                                        DICS_FLAG_GLOBAL, 0, DIREG_DEV, NULL, NULL);
                    pPrivateData[1] = (int)hDeviceKey;
                    
                    if (hDeviceKey != INVALID_HANDLE_VALUE) {
                        StorageCopyDeviceSettings(DeviceInfoSet, DeviceInfoData, hDeviceKey);
                    }
                    
                    dwPropertySize = 0;
                    if (!SetupDiGetDeviceRegistryPropertyW(DeviceInfoSet, DeviceInfoData, 
                                                          SPDRP_FRIENDLYNAME, NULL, NULL, 0, &dwPropertySize)) {
                        DWORD dwLastError = GetLastError();
                        if (dwLastError == ERROR_INSUFFICIENT_BUFFER && dwPropertySize > 0) {
                            pszFriendlyName = (WCHAR*)pSetupMalloc(dwPropertySize);
                            if (pszFriendlyName != NULL) {
                                if (SetupDiGetDeviceRegistryPropertyW(DeviceInfoSet, DeviceInfoData, 
                                                                     SPDRP_DEVICEDESC, NULL, 
                                                                     (PBYTE)pszFriendlyName, 
                                                                     dwPropertySize, NULL)) {
                                    pPrivateData[0] = (int)pszFriendlyName;
                                } else {
                                    pSetupFree(pszFriendlyName);
                                }
                            }
                        }
                    }
                    
                    ContextData->PrivateData = pPrivateData;
                    dwResult = ERROR_SUCCESS_REBOOT_REQUIRED;
                }
            }
        } else {
            pPrivateData = (int*)ContextData->PrivateData;
            
            if (ContextData->InstallResult == NO_ERROR) {
                for (i = 0; i < 4; i++) {
                    if (((DWORD*)&DeviceInfoData->ClassGuid)[i] != ((DWORD*)&GUID_DEVCLASS_TAPEDRIVE)[i]) {
                        break;
                    }
                }
                
                if (i == 4) {
                    bTapeOverride = OverrideFriendlyNameForTape(DeviceInfoSet, DeviceInfoData);
                    if (!bTapeOverride) {
                        if (pPrivateData[0] != 0) {
                            int len = lstrlenW((WCHAR*)pPrivateData[0]);
                            SetupDiSetDeviceRegistryPropertyW(DeviceInfoSet, DeviceInfoData, 
                                                             SPDRP_FRIENDLYNAME, 
                                                             (PBYTE)pPrivateData[0], 
                                                             len * 2 + 2);
                        }
                    }
                } else {
                    if (pPrivateData[0] != 0) {
                        int len = lstrlenW((WCHAR*)pPrivateData[0]);
                        SetupDiSetDeviceRegistryPropertyW(DeviceInfoSet, DeviceInfoData, 
                                                         SPDRP_FRIENDLYNAME, 
                                                         (PBYTE)pPrivateData[0], 
                                                         len * 2 + 2);
                    }
                }
            }
            
            if ((HKEY)pPrivateData[1] != INVALID_HANDLE_VALUE) {
                RegCloseKey((HKEY)pPrivateData[1]);
            }
            
            if (pPrivateData[0] != 0) {
                pSetupFree((void*)pPrivateData[0]);
            }
            
            pSetupFree(pPrivateData);
            dwResult = ContextData->InstallResult;
        }
    } else {
        dwResult = 0;
    }
    
    return dwResult;
}

unsigned char __cdecl StorageCopyDeviceSettings(HDEVINFO DeviceInfoSet, PSP_DEVINFO_DATA DeviceInfoData, HKEY hTargetKey)
{
    unsigned char bResult;
    int i;
    HKEY hStorageSettings;
    DWORD dwPropertySize;
    WCHAR *pszHardwareIds;
    WCHAR *pszCurrentId;
    HKEY hDeviceSettings;
    
    bResult = FALSE;
    hStorageSettings = NULL;
    pszHardwareIds = NULL;
    hDeviceSettings = NULL;
    
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, 
                     L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Storage\\DeviceSettings\\", 
                     0, KEY_READ, &hStorageSettings) == ERROR_SUCCESS) {
        dwPropertySize = 0;
        if (!SetupDiGetDeviceRegistryPropertyW(DeviceInfoSet, DeviceInfoData, 
                                              SPDRP_HARDWAREID, NULL, NULL, 0, &dwPropertySize) && 
            dwPropertySize > 0) {
            pszHardwareIds = (WCHAR*)pSetupMalloc(dwPropertySize);
            if (pszHardwareIds != NULL) {
                if (SetupDiGetDeviceRegistryPropertyW(DeviceInfoSet, DeviceInfoData, 
                                                     SPDRP_HARDWAREID, NULL, 
                                                     (PBYTE)pszHardwareIds, dwPropertySize, NULL)) {
                    pszCurrentId = pszHardwareIds;
                    while (*pszCurrentId != 0) {
                        for (i = 0; pszCurrentId[i] != 0; i++) {
                            if (pszCurrentId[i] == L'\\') {
                                pszCurrentId[i] = L'#';
                            }
                        }
                        
                        if (RegOpenKeyExW(hStorageSettings, pszCurrentId, 0, KEY_READ, &hDeviceSettings) == ERROR_SUCCESS) {
                            CopyKey(hDeviceSettings, hTargetKey);
                            bResult = TRUE;
                            RegCloseKey(hDeviceSettings);
                            hDeviceSettings = NULL;
                            break;
                        }
                        
                        while (*pszCurrentId != 0) {
                            pszCurrentId++;
                        }
                        pszCurrentId++;
                        
                        if (hDeviceSettings != NULL) {
                            RegCloseKey(hDeviceSettings);
                            hDeviceSettings = NULL;
                        }
                    }
                }
            }
        }
        
        if (hStorageSettings != NULL) {
            RegCloseKey(hStorageSettings);
        }
        
        if (pszHardwareIds != NULL) {
            pSetupFree(pszHardwareIds);
        }
    }
    
    return bResult;
}

