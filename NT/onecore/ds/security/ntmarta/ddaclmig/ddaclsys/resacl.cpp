#pragma warning (disable:4995)

#include "resacl.h"

// Helper function
void FreeLocalMemory(void* memoryPtr)
{
    if (memoryPtr != 0) {
        LocalFree(memoryPtr);
    }
    return;
}

// Destructor
CResDDACL::~CResDDACL()
{
    this->_padding_ = (int)vftable;
    if (this->m_hLogFile != (HANDLE)0xffffffff) {
        CloseHandle(this->m_hLogFile);
        this->m_hLogFile = (HANDLE)0xffffffff;
    }
    return;
}

void __cdecl operator delete(void* ptr, unsigned int param2)
{
    if (ptr != NULL) {
        free(ptr);
    }
}

// Scalar deleting destructor
void* __thiscall CResDDACL::scalar_deleting_destructor(unsigned int param_1)
{
    this->~CResDDACL();
    if ((param_1 & 1) != 0) {
        FreeLocalMemory(this);  // Use free() instead of operator delete
    }
    return this;
}

// Create log file
unsigned long __thiscall CResDDACL::CreateLogFile(void)
{
    wchar_t currentChar;
    HANDLE fileHandle;
    int stringResult;
    unsigned long errorCode;
    wchar_t* formatString;
    LARGE_INTEGER fileSize;
    wchar_t logFilePath[260];
    unsigned int securityCookie;
    
    securityCookie = __security_cookie ^ (unsigned int)&securityCookie;
    errorCode = 0;
    formatString = this->m_szLogDir;
    
    do {
        currentChar = *formatString;
        formatString = formatString + 1;
    } while (currentChar != L'\0');
    
    int pathLength = (int)formatString - (int)(this->m_szLogDir + 1) > 1;
    
    if (pathLength != 0) {
        if (this->m_szLogDir[pathLength - 1] == L'\\') {
            formatString = L"%s%s";
        }
        else {
            formatString = L"%s\\%s";
        }
        
        stringResult = StringCchPrintfW(logFilePath, 0x104, formatString, this->m_szLogDir, L"ddacl.log");
        
        if (stringResult < 0) {
            errorCode = 0xe;
        }
        else {
            fileHandle = CreateFileW(logFilePath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            this->m_hLogFile = fileHandle;
            
            if (fileHandle != INVALID_HANDLE_VALUE) {
                stringResult = GetFileSizeEx(fileHandle, &fileSize);
                if (stringResult == 0) goto cleanup;
                
                if ((fileSize.LowPart < 1) && (fileSize.HighPart < 0x200001)) {
                    stringResult = SetFilePointer(this->m_hLogFile, 0, NULL, FILE_END);
                    if (stringResult == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR) {
                        CloseHandle(this->m_hLogFile);
                        this->m_hLogFile = INVALID_HANDLE_VALUE;
                    }
                    goto cleanup;
                }
                
                stringResult = SetEndOfFile(this->m_hLogFile);
                if (stringResult != 0) goto cleanup;
                
                CloseHandle(this->m_hLogFile);
                fileHandle = CreateFileW(logFilePath, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                this->m_hLogFile = fileHandle;
                
                if (fileHandle != INVALID_HANDLE_VALUE) goto cleanup;
            }
            errorCode = GetLastError();
        }
    }
    
cleanup:
    __security_check_cookie(securityCookie);
    return errorCode;
}

// Enumerate volume
unsigned long __thiscall CResDDACL::EnumerateVolume(int param_1)
{
    HANDLE findHandle;
    unsigned long errorCode;
    int findResult;
    unsigned int errorValue;
    wchar_t volumeName[260];
    unsigned int securityCookie;
    
    securityCookie = __security_cookie ^ (unsigned int)&securityCookie;
    findHandle = FindFirstVolumeW(volumeName, 0x104);
    
    if (findHandle == INVALID_HANDLE_VALUE) {
        errorCode = GetLastError();
    }
    else {
        do {
            this->logPrintA((unsigned long)this, (char*)0x0);
            errorCode = this->HandleEachVolume(param_1, volumeName);
            
            if (errorCode != 0) {
                this->logPrintA((unsigned long)this, (char*)0x1);
            }
            
            findResult = FindNextVolumeW(findHandle, volumeName, 0x104);
        } while (findResult != 0);
        
        errorValue = GetLastError();
        errorCode = (errorValue != 0x12) ? errorValue : 0;
        FindVolumeClose(findHandle);
    }
    
    __security_check_cookie(securityCookie);
    return errorCode;
}

// Fix ACL
unsigned long __thiscall CResDDACL::FixACL(wchar_t* param_1)
{
    unsigned long result;
    
    this->logPrintA((unsigned long)this, (char*)0x0);
    result = this->ResetDirectoryTreeSecurity(param_1, param_1);
    
    if (result == 0) {
        this->logPrintA((unsigned long)this, (char*)0x0);
        result = 0;
    }
    else {
        this->logPrintA((unsigned long)this, (char*)0x1);
    }
    
    return result;
}

// Handle each volume
unsigned long __thiscall CResDDACL::HandleEachVolume(int param_1, wchar_t* param_2)
{
    short currentChar;
    short* pathPtr;
    int volumeResult;
    unsigned long errorCode;
    short* tempPtr;
    unsigned int volumeIndex;
    wchar_t volumePathNames[260];
    unsigned int securityCookie;
    int pathLength;
    short* pathArray;
    DWORD bufferSize = 0x104;
    
    securityCookie = __security_cookie ^ (unsigned int)&securityCookie;
    pathLength = 0;
    
    this->logPrintA((unsigned long)this, (char*)0x0);
    
    volumeResult = GetVolumePathNamesForVolumeNameW(param_2, volumePathNames, 0x104, &bufferSize);
    
    if (volumeResult != 0) {
        this->logPrintA((unsigned long)this, (char*)0x0);
        pathPtr = (short*)volumePathNames;
        
        for (volumeIndex = 0; (volumePathNames[0] != 0 && (volumeIndex < 0x104)); volumeIndex = volumeIndex + volumeResult) {
            tempPtr = pathPtr + 1;
            pathArray = pathPtr;
            
            do {
                currentChar = *pathArray;
                pathArray = pathArray + 1;
            } while (currentChar != (short)pathLength);
            
            volumeResult = (int)pathArray - (int)tempPtr > 1;
            pathPtr[volumeResult] = 9;
            pathPtr = pathPtr + volumeResult + 1;
            volumePathNames[0] = *pathPtr;
        }
        
        this->logPrintA((unsigned long)this, (char*)0x0);
    }
    
    errorCode = this->IsVolumeInteresting(param_2, (int*)&tempPtr);
    
    if ((((errorCode == 0) && (tempPtr != (short*)0x0)) &&
        (errorCode = this->TestACL(param_2, &pathLength), errorCode == 0)) && (pathLength != 0)) {
        errorCode = this->FixACL(param_2);
    }
    
    __security_check_cookie(securityCookie);
    return errorCode;
}

// Has other OS version
unsigned long __thiscall CResDDACL::HasOtherOSVersion(wchar_t* param_1, int* param_2)
{
    wchar_t currentChar;
    int stringResult;
    unsigned int fileAttributes;
    wchar_t** pathList;
    int pathIndex;
    unsigned long errorCode;
    wchar_t* pathPtr;
    wchar_t* versionPaths[3];
    wchar_t fullPath[520];
    unsigned int securityCookie;
    
    securityCookie = __security_cookie ^ (unsigned int)&securityCookie;
    versionPaths[0] = L"Windows";
    versionPaths[1] = L"WinNT";
    versionPaths[2] = (wchar_t*)0x0;
    *param_2 = 0;
    
    if (param_1 != (wchar_t*)0x0) {
        pathPtr = param_1;
        do {
            currentChar = *pathPtr;
            pathPtr = pathPtr + 1;
        } while (currentChar != L'\0');
        
        pathIndex = (int)pathPtr - (int)(param_1 + 1) > 1;
        
        if (pathIndex != 0) {
            pathList = versionPaths;
            do {
                if (param_1[pathIndex - 1] == L'\\') {
                    pathPtr = L"%s%s";
                }
                else {
                    pathPtr = L"%s\\%s";
                }
                
                stringResult = StringCchPrintfW(fullPath, 0x104, pathPtr, param_1, *pathList);
                
                if (((-1 < stringResult) && (fileAttributes = GetFileAttributesW(fullPath), fileAttributes != 0xffffffff)) &&
                   ((fileAttributes >> 4 & 1) != 0)) {
                    ((CResDDACL*)param_2)->vftable = (void*)1;
                    break;
                }
                
                pathList = pathList + 1;
            } while (*pathList != (wchar_t*)0x0);
            
            errorCode = 0;
            goto cleanup;
        }
    }
    
    errorCode = 0x57;
    
cleanup:
    this->logPrintA((unsigned long)param_2, (char*)0x0);
    __security_check_cookie(securityCookie);
    return errorCode;
}

// Has XP default DACL
unsigned long __thiscall CResDDACL::HasXPDefaultDACL(wchar_t* param_1, int* param_2)
{
    int securityResult;
    unsigned long errorCode;
    int isDefault;
    wchar_t* securityString;
    PSECURITY_DESCRIPTOR securityDescriptor;
    
    errorCode = 0;
    isDefault = 1;
    *param_2 = 0;
    securityString = NULL;
    securityDescriptor = NULL;
    
    securityResult = GetNamedSecurityInfoW(param_1, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, NULL, NULL, &securityDescriptor);
    
    if (securityResult == ERROR_SUCCESS) {
        securityResult = ConvertSecurityDescriptorToStringSecurityDescriptorW(securityDescriptor, SDDL_REVISION_1, DACL_SECURITY_INFORMATION, &securityString, NULL);
        
        if (securityResult == 0) {
            errorCode = GetLastError();
        }
        else {
            this->logPrintA((unsigned long)this, (char*)0x0);
            
            securityResult = _wcsicmp(securityString,
                         L"D:(A;OICI;FA;;;BA)(A;OICI;FA;;;SY)(A;OICIIO;GA;;;CO)(A;OICI;0x1200a9;;;BU)(A;CI;LC;;;BU)(A;CIIO;DC;;;BU)(A;;0x1200a9;;;WD)");
            
            if (securityResult != 0) {
                securityResult = _wcsicmp(securityString,
                         L"D:AI(A;;FA;;;BA)(A;OICIIO;GA;;;BA)(A;;FA;;;SY)(A;OICIIO;GA;;;SY)(A;OICIIO;GA;;;CO)(A;;0x1200a9;;;BU)(A;OICIIO;GXGR;;;BU)(A;CI;LC;;;BU)(A;CIIO;DC;;;BU)(A;;0x1200a9;;;WD)");
                
                if (securityResult != 0) {
                    isDefault = 0;
                }
            }
            *param_2 = isDefault;
        }
    }
    else {
        *param_2 = 0;
        this->logPrintA((unsigned long)this, (char*)0x0);
    }
    
    if (securityString != NULL) {
        LocalFree(securityString);
    }
    
    if (securityDescriptor != NULL) {
        LocalFree(securityDescriptor);
    }
    
    return errorCode;
}

// Initialize
unsigned long __thiscall CResDDACL::Initialize(void)
{
    int regResult;
    unsigned long errorCode;
    unsigned int valueType;
    unsigned int dataSize;
    HKEY regKey;
    
    errorCode = 0;
    valueType = 0;
    dataSize = 0;
    regKey = NULL;
    
    regResult = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\Setup", 0, KEY_READ, &regKey);
    
    if (regResult == ERROR_SUCCESS) {
        valueType = REG_SZ;
        dataSize = 0x104;
        regResult = RegQueryValueExW(regKey, L"WorkingDirectory", NULL, (LPDWORD)&valueType, (LPBYTE)this->m_szLogDir, (LPDWORD)&dataSize);
        RegCloseKey(regKey);
        if (regResult != ERROR_SUCCESS) goto getWindowsDir;
    }
    else {
getWindowsDir:
        regResult = GetWindowsDirectoryW(this->m_szLogDir, 0x104);
        if (regResult == 0) goto checkOS;
    }
    
    this->CreateLogFile();
    
checkOS:
    regResult = IsOS(0x1d);
    if (regResult == 0) {
        errorCode = this->ReadSystemPartition(this->m_szSystemPartition);
        if (errorCode != 0) {
            this->logPrintA((unsigned long)this, (char*)0x1);
        }
    }
    else {
        this->logPrintA((unsigned long)this, (char*)0x0);
    }
    
    return errorCode;
}

// Is volume interesting
unsigned long __thiscall CResDDACL::IsVolumeInteresting(wchar_t* param_1, int* param_2)
{
    int driveType;
    int volumeResult;
    unsigned long errorCode;
    unsigned int flags;
    wchar_t fileSystemName[100];
    unsigned int securityCookie;
    
    securityCookie = __security_cookie ^ (unsigned int)&securityCookie;
    memset(fileSystemName, 0, sizeof(fileSystemName));
    flags = 0;
    *param_2 = 0;
    
    driveType = GetDriveTypeW(param_1);
    
    if ((driveType == DRIVE_FIXED) || (driveType == DRIVE_REMOVABLE)) {
        this->logPrintA((unsigned long)this, (char*)0x0);
        
        volumeResult = GetVolumeInformationW(param_1, NULL, 0, NULL, NULL, (LPDWORD)&flags, fileSystemName, 100);
        
        if (volumeResult != 0) {
            if ((_wcsicmp(fileSystemName, L"ntfs") == 0) &&
               (this->logPrintA((unsigned long)this, (char*)0x0), (flags & 0x80000) == 0)) {
                *param_2 = 1;
            }
            else {
                this->logPrintA((unsigned long)this, (char*)0x0);
            }
            goto cleanup;
        }
        
        volumeResult = GetLastError();
        if ((volumeResult == ERROR_NOT_READY) && (driveType == DRIVE_REMOVABLE)) goto cleanup;
    }
    
    this->logPrintA((unsigned long)this, (char*)0x0);
    
cleanup:
    errorCode = 0;
    __security_check_cookie(securityCookie);
    return errorCode;
}

// Is volume system
unsigned long __thiscall CResDDACL::IsVolumeSystem(wchar_t* param_1, int* param_2)
{
    short currentChar;
    unsigned long errorCode;
    unsigned short* buffer;
    unsigned int bytesReturned;
    short* pathPtr;
    HANDLE handle;
    int bufferSize;
    int compareResult;
    int allocatedMemory;
    unsigned short* dataPtr;
    wchar_t volumePath[260];
    unsigned int securityCookie;
    
    securityCookie = __security_cookie ^ (unsigned int)&securityCookie;
    memset(volumePath, 0, sizeof(volumePath));
    *param_2 = 0;
    bytesReturned = 0;
    bufferSize = 0x105;
    
    pathPtr = (short*)volumePath;
    do {
        if ((bufferSize == -0x7ffffef9) || (currentChar = *param_1, currentChar == 0)) break;
        *pathPtr = currentChar;
        pathPtr = pathPtr + 1;
        param_1 = param_1 + 1;
        bufferSize = bufferSize - 1;
    } while (bufferSize != 0);
    
    if (bufferSize == 0) {
        pathPtr = pathPtr - 1;
    }
    *pathPtr = 0;
    
    pathPtr = (short*)volumePath;
    do {
        currentChar = *pathPtr;
        pathPtr = pathPtr + 1;
    } while (currentChar != 0);
    
    int pathLength = (int)pathPtr - (int)volumePath > 1;
    
    if (pathLength != 0) {
        handle = CreateFileW(volumePath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
        
        if (handle == INVALID_HANDLE_VALUE) {
            errorCode = GetLastError();
            this->logPrintA((unsigned long)this, (char*)0x1);
        }
        else {
            bufferSize = 0x20e;
            buffer = NULL;
            allocatedMemory = 0;
            
            while (buffer = (unsigned short*)LocalAlloc(LMEM_FIXED, bufferSize), buffer != NULL) {
                memset(buffer, 0, bufferSize);
                
                if (DeviceIoControl(handle, 0x4d0008, NULL, 0, buffer, bufferSize, (LPDWORD)&bytesReturned, NULL)) {
                    allocatedMemory = (int)LocalAlloc(LMEM_FIXED, (unsigned int)*buffer * 2 + 2);
                    if (allocatedMemory == 0) {
                        errorCode = GetLastError();
                        goto cleanup;
                    }
                    
                    dataPtr = buffer + 1;
                    memcpy((void*)allocatedMemory, dataPtr, (unsigned int)*buffer * 2);
                    *(wchar_t*)(allocatedMemory + (unsigned int)(*buffer >> 1) * 2) = 0;
                    
                    compareResult = _wcsicmp((wchar_t*)allocatedMemory, this->m_szSystemPartition);
                    if (compareResult == 0) {
                        *param_2 = 1;
                    }
                    
                    errorCode = 0;
                    goto cleanup;
                }
                
                errorCode = GetLastError();
                if ((errorCode != ERROR_INSUFFICIENT_BUFFER) && (errorCode != ERROR_MORE_DATA)) goto cleanup;
                
                bufferSize = bytesReturned + 2;
                LocalFree(buffer);
                buffer = NULL;
            }
            
            errorCode = ERROR_OUTOFMEMORY;
            
cleanup:
            this->logPrintA((unsigned long)this, (char*)0x1);
            CloseHandle(handle);
            
            if (buffer != NULL) {
                LocalFree(buffer);
            }
            if (allocatedMemory != 0) {
                LocalFree((HLOCAL)allocatedMemory);
            }
        }
        goto done;
    }
    
    errorCode = ERROR_INVALID_PARAMETER;
    
done:
    __security_check_cookie(securityCookie);
    return errorCode;
}

// Log print A
void __thiscall CResDDACL::logPrintA(unsigned long param_1, char* param_2)
{
    char currentChar;
    int stringResult;
    char* messagePtr;
    char* writePtr;
    SYSTEMTIME systemTime;
    char timeBuffer[60];
    char logBuffer[260];
    unsigned int bytesWritten;
    unsigned int securityCookie;
    
    securityCookie = __security_cookie ^ (unsigned int)&securityCookie;
    
    if (*(HANDLE*)(param_1 + 4) != INVALID_HANDLE_VALUE) {
        GetLocalTime(&systemTime);
        stringResult = StringCchPrintfA(timeBuffer, 0x3c, "%02u/%02u %02u:%02u:%02u\t",
                                        systemTime.wMonth, systemTime.wDay,
                                        systemTime.wHour, systemTime.wMinute, systemTime.wSecond);
        
        if (-1 < stringResult) {
            messagePtr = timeBuffer;
            do {
                currentChar = *messagePtr;
                messagePtr = messagePtr + 1;
            } while (currentChar != '\0');
            
            WriteFile(*(HANDLE*)(param_1 + 4), timeBuffer, (int)messagePtr - (int)(timeBuffer + 1), (LPDWORD)&bytesWritten, NULL);
        }
        
        if (param_2 == (char*)0x1) {
            messagePtr = "Error\t";
        }
        else if (param_2 == (char*)0x2) {
            messagePtr = "Warning\t";
        }
        else {
            messagePtr = "Info\t";
        }
        
        writePtr = messagePtr;
        do {
            currentChar = *writePtr;
            writePtr = writePtr + 1;
        } while (currentChar != '\0');
        
        WriteFile(*(HANDLE*)(param_1 + 4), messagePtr, (int)writePtr - (int)(messagePtr + 1), (LPDWORD)&bytesWritten, NULL);
        
		size_t newLength = 0;
		va_list emptyArgs;
		va_start(emptyArgs, messagePtr);
        stringResult = StringVPrintfWorkerA(logBuffer, 260, &newLength, messagePtr, emptyArgs);
		va_end(emptyArgs);
        
        if (-1 < stringResult) {
            messagePtr = logBuffer;
            do {
                currentChar = *messagePtr;
                messagePtr = messagePtr + 1;
            } while (currentChar != '\0');
            
            WriteFile(*(HANDLE*)(param_1 + 4), logBuffer, (int)messagePtr - (int)(logBuffer + 1), (LPDWORD)&bytesWritten, NULL);
        }
        
        WriteFile(*(HANDLE*)(param_1 + 4), "\r\n", 2, (LPDWORD)&bytesWritten, NULL);
    }
    
    __security_check_cookie(securityCookie);
    return;
}

// Read sysprep opt out key
void __thiscall CResDDACL::ReadSysPrepOptOutKey(int* param_1)
{
    int regResult;
    unsigned int valueType;
    unsigned int dataSize;
    HKEY regKey;
    DWORD valueData;
    
    valueType = 0;
    dataSize = 0;
    regKey = NULL;
    *param_1 = 0;
    valueData = 0;
    
    regResult = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\Setup", 0, KEY_READ, &regKey);
    
    if (regResult == ERROR_SUCCESS) {
        valueType = REG_DWORD;
        dataSize = sizeof(DWORD);
        regResult = RegQueryValueExW(regKey, L"DDACLSys_Disabled", NULL, (LPDWORD)&valueType, (LPBYTE)&valueData, (LPDWORD)&dataSize);
        
        if (regResult == ERROR_SUCCESS) {
            *param_1 = (unsigned int)(valueData == 1);
            this->logPrintA((unsigned long)this, (char*)0x0);
        }
        
        RegCloseKey(regKey);
    }
    
    return;
}

// Read system partition
unsigned long __thiscall CResDDACL::ReadSystemPartition(wchar_t* param_1)
{
    unsigned long regResult;
    unsigned int valueType;
    unsigned int dataSize;
    HKEY regKey;
    
    valueType = 0;
    dataSize = 0;
    regKey = NULL;
    
    regResult = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\Setup", 0, KEY_READ, &regKey);
    
    if (regResult == ERROR_SUCCESS) {
        valueType = REG_SZ;
        dataSize = 0x104;
        regResult = RegQueryValueExW(regKey, L"SystemPartition", NULL, (LPDWORD)&valueType, (LPBYTE)param_1, (LPDWORD)&dataSize);
        RegCloseKey(regKey);
    }
    
    return regResult;
}

// Reset directory tree security
unsigned long __thiscall CResDDACL::ResetDirectoryTreeSecurity(wchar_t* param_1, wchar_t* param_2)
{
    int conversionResult;
    unsigned long errorCode;
    PSECURITY_DESCRIPTOR securityDescriptor;
    unsigned int daclPresent;
    PACL dacl;
    unsigned int daclDefaulted;
    unsigned int controlBits;
    unsigned int revision;
    
    errorCode = 0;
    securityDescriptor = NULL;
    
    conversionResult = ConvertStringSecurityDescriptorToSecurityDescriptorW(
        L"D:(A;;FA;;;BA)(A;OICIIO;GA;;;BA)(A;;FA;;;SY)(A;OICIIO;GA;;;SY)(A;;0x1301bf;;;AU)(A;OICIIO;SDGXGWGR;;;AU)(A;;0x1200a9;;;BU)(A;OICIIO;GXGR;;;BU)",
        SDDL_REVISION_1, &securityDescriptor, NULL);
    
    if (conversionResult == 0) {
        errorCode = GetLastError();
    }
    else {
        controlBits = 0;
        revision = 0;
        conversionResult = GetSecurityDescriptorControl(securityDescriptor, (PSECURITY_DESCRIPTOR_CONTROL)&controlBits, (LPDWORD)&revision);
        
        if (conversionResult == 0) {
            errorCode = GetLastError();
        }
        else {
            daclPresent = 0;
            dacl = NULL;
            daclDefaulted = 0;
            conversionResult = GetSecurityDescriptorDacl(securityDescriptor, (LPBOOL)&daclPresent, &dacl, (LPBOOL)&daclDefaulted);
            
            if (conversionResult == 0) {
                errorCode = GetLastError();
            }
            else {
                errorCode = SetNamedSecurityInfoW(param_1, SE_FILE_OBJECT, (daclPresent != 0) ? DACL_SECURITY_INFORMATION : 0, NULL, NULL, dacl, NULL);
                
                if (errorCode != 0) {
                    this->logPrintA((unsigned long)this, (char*)0x1);
                }
            }
        }
    }
    
    if (securityDescriptor != NULL) {
        LocalFree(securityDescriptor);
    }
    
    return errorCode;
}

// Test ACL
unsigned long __thiscall CResDDACL::TestACL(wchar_t* param_1, int* param_2)
{
    unsigned long errorCode;
    int isSystemVolume;
    int hasOtherOS;
    int hasDefaultDACL;
    
    errorCode = 0;
    *param_2 = 0;
    isSystemVolume = 0;
    hasOtherOS = 0;
    hasDefaultDACL = 0;
    
    this->IsVolumeSystem(param_1, &isSystemVolume);
    
    if (isSystemVolume == 0) {
        this->logPrintA((unsigned long)this, (char*)0x0);
        
        errorCode = this->HasOtherOSVersion(param_1, &hasOtherOS);
        
        if (errorCode == 0) {
            if (hasOtherOS == 0) {
                errorCode = this->HasXPDefaultDACL(param_1, &hasDefaultDACL);
                
                if (errorCode != 0) {
                    return errorCode;
                }
                
                this->logPrintA((unsigned long)this, (char*)0x0);
                *param_2 = hasDefaultDACL;
            }
            errorCode = 0;
        }
    }
    else {
        this->logPrintA((unsigned long)this, (char*)0x0);
    }
    
    return errorCode;
}

