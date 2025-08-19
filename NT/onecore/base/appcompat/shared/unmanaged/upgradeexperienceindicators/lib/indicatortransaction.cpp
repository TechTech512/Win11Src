#include <windows.h>
#include <wchar.h>
#include <memory.h>
#include <malloc.h>
#include <stdlib.h>      // For malloc, free
#include <string.h>
#include <strsafe.h>
#include <wincrypt.h>

// External function declarations (assumed to exist in other source files)
extern "C" {
    HANDLE __stdcall CreateTransaction(LPSECURITY_ATTRIBUTES lpTransactionAttributes, LPGUID UOW, DWORD CreateOptions, DWORD IsolationLevel, DWORD IsolationFlags, DWORD Timeout, LPWSTR Description);
    BOOL __stdcall CommitTransaction(HANDLE TransactionHandle);
}

// Namespace declarations (assumed based on usage)
namespace Windows {
namespace Compat {
namespace Shared {

    class RegistryKey {
    public:
        HKEY Key;
        // ... other members
        
        int Initialize(const wchar_t* keyPath, HANDLE transaction);
        int UnInitialize();
        int ClearKey();
        int WriteKeyValueString(const wchar_t* valueName, const wchar_t* data);
        int WriteKeyValueBinary(const wchar_t* valueName, const unsigned char* data, DWORD dataSize);
        int WriteKeyValueTimestamp(const wchar_t* valueName);
    };

    class IndicatorTelemetry {
    public:
        int Initialize(bool param1, bool param2, char* param3, wchar_t* param4, RegistryKey* param5, wchar_t* param6);
        int UnInitialize();
        int SendIndicator(const wchar_t* indicatorName, const wchar_t* indicatorValue, bool isReduced, RegistryKey* key);
        int SendIndicatorsRemoved(RegistryKey* currentKey, RegistryKey* newKey, bool isReduced);
    };

    class HashProvider {
    public:
        HCRYPTPROV CryptProv;
        HCRYPTHASH CryptHash;
        
        ~HashProvider();
        int InitializeCryptProvider();
        int GetChecksumValueAlloc(unsigned char** checksum, unsigned long* checksumSize);
    };

    class RegistryChecksum {
    public:
        int AddKey(HKEY key);
        int AddUnique();
    };

} // namespace Shared
} // namespace Compat
} // namespace Windows

// Class definition for IndicatorTransaction
class IndicatorTransaction {
public:
    ~IndicatorTransaction();
    int AddChecksum();
    int AddIndicator(wchar_t* indicatorName, wchar_t* indicatorValue, bool isReduced);
    static unsigned int AreIndicatorsNotTampered(bool* result, wchar_t* subKeyName);
    static int CalculateCheckSum(unsigned char** registryKey, unsigned long* transactionHandle, HKEY* keyHandle, void* unused);
    unsigned int Finalize();
    unsigned int Initialize(wchar_t** indicatorNames, unsigned long nameCount, wchar_t* subKeyName, bool telemetryParam, char* param5, bool param6);

private:
    HANDLE Transaction;
    Windows::Compat::Shared::RegistryKey Key;
    Windows::Compat::Shared::RegistryKey ReducedKey;
    Windows::Compat::Shared::RegistryKey CurrentKey;
    Windows::Compat::Shared::RegistryKey CurrentReducedKey;
    Windows::Compat::Shared::RegistryKey SharedKey;
    Windows::Compat::Shared::IndicatorTelemetry Telemetry;
    wchar_t** IndicatorNameList;
    unsigned long IndicatorNameListLength;
    wchar_t* SubKeyName;
    unsigned __int64 SetIndicatorBitmap;
    bool SomethingFailed;
};

// Destructor
IndicatorTransaction::~IndicatorTransaction() {
    if (this->Transaction != (HANDLE)-1) {
        CloseHandle(this->Transaction);
    }
}

// AddChecksum method
int IndicatorTransaction::AddChecksum() {
    int result = (this->Key.Key != NULL) ? 0x80070057 : 0x7FF8FFA9;
    
    if (result >= 0) {
        result = CalculateCheckSum((unsigned char**)this->Key.Key, (unsigned long*)this->Transaction, NULL, NULL);
        if (result >= 0) {
            result = this->SharedKey.WriteKeyValueBinary(this->SubKeyName, (const unsigned char*)&this->SetIndicatorBitmap, sizeof(this->SetIndicatorBitmap));
        }
    }
    
    return result;
}

// AddIndicator method
int IndicatorTransaction::AddIndicator(wchar_t* indicatorName, wchar_t* indicatorValue, bool isReduced) {
    if (this->SomethingFailed) {
        return -0x7FFFBFFB; // 0x80004005 with different sign
    }

    Windows::Compat::Shared::RegistryKey* targetKey = isReduced ? &this->ReducedKey : &this->Key;
    int result = targetKey->WriteKeyValueString(indicatorName, indicatorValue);
    
    if (result < 0) {
        this->SomethingFailed = true;
        return result;
    }

    // Check if indicator name is in the predefined list
    bool found = false;
    if (this->IndicatorNameListLength > 0) {
        for (unsigned long i = 0; i < this->IndicatorNameListLength; i++) {
            if (wcscmp(indicatorName, this->IndicatorNameList[i]) == 0) {
                found = true;
                break;
            }
        }
    }

    if (!found) {
        // Update bitmap to track which indicators were set
        unsigned int bitPosition = 0; // This would need proper calculation
        this->SetIndicatorBitmap |= (1ULL << bitPosition);
        
        Windows::Compat::Shared::RegistryKey* telemetryKey = isReduced ? &this->CurrentReducedKey : &this->CurrentKey;
        result = this->Telemetry.SendIndicator(indicatorName, indicatorValue, isReduced, telemetryKey);
            
        if (result < 0) {
            this->SomethingFailed = true;
        }
    }
    
    return result;
}

// Static method to check if indicators were tampered with
unsigned int IndicatorTransaction::AreIndicatorsNotTampered(bool* result, wchar_t* subKeyName) {
    HKEY hKey = NULL;
    void* valueData = NULL;
    int valueSize = 0;
    unsigned int status = 0x20019; // Default error code
    
    // Open the shared registry key
    LONG regResult = RegOpenKeyExW(
        HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Shared",
        0,
        KEY_READ,
        &hKey);
    
    if (regResult == ERROR_SUCCESS) {
        // Query the checksum value size
        regResult = RegQueryValueExW(hKey, subKeyName, NULL, NULL, NULL, (LPDWORD)&valueSize);
        
        if (regResult == ERROR_FILE_NOT_FOUND) {
            *result = false;
            status = ERROR_SUCCESS;
            RegCloseKey(hKey);
            return status;
        }
        
        // Allocate memory for the value
        valueData = malloc(valueSize);
        if (!valueData) {
            status = ERROR_OUTOFMEMORY;
            RegCloseKey(hKey);
            return status;
        }
        
        // Read the value
        regResult = RegQueryValueExW(hKey, subKeyName, NULL, NULL, (LPBYTE)valueData, (LPDWORD)&valueSize);
        
        if (regResult == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            hKey = NULL;
            
            // Build the full key path
            wchar_t fullKeyPath[0x104];
            HRESULT strResult = StringCchPrintfW(fullKeyPath, 0x104, L"%ls\\%ls", 
                L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Shared",
                subKeyName);
                
            if (strResult != S_OK) {
                status = strResult;
                free(valueData);
                return status;
            }
            
            // Open the specific subkey
            regResult = RegOpenKeyExW(
                HKEY_LOCAL_MACHINE,
                fullKeyPath,
                0,
                KEY_READ,
                &hKey);
                
            if (regResult == ERROR_SUCCESS) {
                // Calculate current checksum
                unsigned char* currentChecksum = NULL;
                unsigned long currentChecksumSize = 0;
                
                int calcResult = CalculateCheckSum((unsigned char**)hKey, NULL, (HKEY*)&currentChecksum, NULL);
                
                if (calcResult >= 0) {
                    // Compare with stored checksum
                    if (valueSize == currentChecksumSize && 
                        memcmp(valueData, currentChecksum, valueSize) == 0) {
                        *result = true;
                    } else {
                        *result = false;
                    }
                    status = ERROR_SUCCESS;
                    
                    // Free the calculated checksum (assumed to be allocated by CalculateCheckSum)
                    free(currentChecksum);
                } else {
                    status = calcResult;
                }
            }
        }
    }
    
    // Cleanup
    if (hKey) {
        RegCloseKey(hKey);
    }
    if (valueData) {
        free(valueData);
    }
    
    return status;
}

// Static method to calculate checksum
int IndicatorTransaction::CalculateCheckSum(unsigned char** registryKey, unsigned long* transactionHandle, HKEY* keyHandle, void* unused) {
    Windows::Compat::Shared::HashProvider hashProvider;
    Windows::Compat::Shared::RegistryChecksum registryChecksum;
    HKEY hReducedKey = NULL;
    int result = -1;
    
    // Initialize the hash provider
    result = hashProvider.InitializeCryptProvider();
    if (result < 0) {
        return result;
    }
    
    // Add the main key to the checksum
    result = registryChecksum.AddKey((HKEY)registryKey);
    if (result < 0) {
        goto cleanup;
    }
    
    // Open and add the Reduced subkey
    if (transactionHandle) {
        result = RegOpenKeyTransactedW((HKEY)registryKey, L"Reduced", 0, KEY_READ, &hReducedKey, (HANDLE)transactionHandle, NULL);
    } else {
        result = RegOpenKeyExW((HKEY)registryKey, L"Reduced", 0, KEY_READ, &hReducedKey);
    }
    
    if (result == ERROR_SUCCESS) {
        result = registryChecksum.AddKey(hReducedKey);
        if (result < 0) {
            goto cleanup;
        }
    }
    
    // Add unique identifier
    result = registryChecksum.AddUnique();
    if (result < 0) {
        goto cleanup;
    }
    
    // Get the checksum value
    unsigned char* checksum = NULL;
    unsigned long checksumSize = 0;
    result = hashProvider.GetChecksumValueAlloc(&checksum, &checksumSize);
    if (result >= 0) {
        // The caller is responsible for freeing the checksum memory
        *((unsigned char**)keyHandle) = checksum;
        *((unsigned long*)unused) = checksumSize;
        result = 0;
    }
    
cleanup:
    if (hReducedKey) {
        RegCloseKey(hReducedKey);
    }
    hashProvider.~HashProvider();
    
    return result;
}

// Finalize method
unsigned int IndicatorTransaction::Finalize() {
    int result;
    
    // Send telemetry for removed indicators
    result = this->Telemetry.SendIndicatorsRemoved(&this->CurrentKey, &this->Key, false);
    
    if (result >= 0) {
        result = this->Telemetry.SendIndicatorsRemoved(&this->CurrentKey, &this->Key, false);
    }
    
    if (result >= 0) {
        result = this->Telemetry.UnInitialize();
    }
    
    if (result >= 0 && !this->SomethingFailed) {
        // Check if all expected indicators were set
        if (this->SetIndicatorBitmap == ~0ULL) { // All bits set
            // Write timestamp and checksum
            result = this->Key.WriteKeyValueTimestamp(this->SubKeyName);
            
            if (result >= 0) {
                result = AddChecksum();
                
                if (result >= 0) {
                    // Commit the transaction
                    if (CommitTransaction(this->Transaction)) {
                        result = ERROR_SUCCESS;
                    } else {
                        result = GetLastError();
                        if (result > 0) {
                            result |= 0x80070000;
                        }
                    }
                }
            }
        } else {
            result = 0x80004005; // E_FAIL
        }
    }
    
    // Cleanup all registry keys
    this->Key.UnInitialize();
    this->ReducedKey.UnInitialize();
    this->CurrentKey.UnInitialize();
    this->CurrentReducedKey.UnInitialize();
    this->SharedKey.UnInitialize();
    
    return result;
}

// Initialize method
unsigned int IndicatorTransaction::Initialize(wchar_t** indicatorNames, unsigned long nameCount, wchar_t* subKeyName, bool telemetryParam, char* param5, bool param6) {
    Windows::Compat::Shared::RegistryKey utcSyncKey;
    wchar_t fullKeyPath[0x104];
    wchar_t fullReducedKeyPath[0x104];
    wchar_t telemetryKeyName[0x104];
    unsigned int result = 0;
    bool indicatorsValid = false;
    
    this->IndicatorNameList = indicatorNames;
    this->IndicatorNameListLength = nameCount;
    this->SubKeyName = subKeyName;
    
    // Check if indicators were tampered with
    int tamperResult = AreIndicatorsNotTampered(&indicatorsValid, subKeyName);
    
    // Determine if we should proceed based on tamper check and parameter
    bool shouldProceed = param6 || (tamperResult < 0) || !indicatorsValid;
    
    // Create transaction
    this->Transaction = CreateTransaction(NULL, NULL, 0, 0, 0, 0, NULL);
    if (this->Transaction == (HANDLE)-1) {
        result = GetLastError();
        if (result > 0) {
            result |= 0x80070000;
        }
        this->SomethingFailed = true;
        return result;
    }
    
    // Initialize main registry key
    StringCchPrintfW(fullKeyPath, 0x104, L"%ls\\%ls", 
        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Shared",
        subKeyName);
        
    result = this->Key.Initialize(fullKeyPath, this->Transaction);
    if (result != 0) {
        this->SomethingFailed = true;
        return result;
    }
    
    // Clear the key
    result = this->Key.ClearKey();
    if (result != 0) {
        this->SomethingFailed = true;
        return result;
    }
    
    // Initialize reduced registry key
    StringCchPrintfW(fullReducedKeyPath, 0x104, L"%ls\\%ls\\Reduced", 
        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Shared",
        subKeyName);
        
    result = this->ReducedKey.Initialize(fullReducedKeyPath, this->Transaction);
    if (result != 0) {
        this->SomethingFailed = true;
        return result;
    }
    
    // Initialize shared registry key
    result = this->SharedKey.Initialize(
        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Shared",
        this->Transaction);
        
    if (result != 0) {
        this->SomethingFailed = true;
        return result;
    }
    
    // Determine telemetry key name based on subkey name
    if (wcscmp(subKeyName, L"UpgradeExperienceIndicators") == 0) {
        StringCchCopyW(telemetryKeyName, 0x104, L"InventoryMiscellaneousUexIndicator");
    } else {
        StringCchPrintfW(telemetryKeyName, 0x104, L"%ls%ls", subKeyName, L"UexIndicator");
    }
    
    // Initialize UTC sync key for telemetry
    utcSyncKey.Key = NULL;
    result = utcSyncKey.Initialize(
        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Shared\\UtcSyncId",
        this->Transaction);
        
    if (result != 0) {
        this->SomethingFailed = true;
        return result;
    }
    
    // Initialize telemetry
    result = this->Telemetry.Initialize(
        telemetryParam, shouldProceed, param5, telemetryKeyName, &utcSyncKey, subKeyName);
        
    utcSyncKey.UnInitialize();
    
    if (result != 0) {
        this->SomethingFailed = true;
        return result;
    }
    
    // If indicators were valid, initialize current keys for comparison
    if (!shouldProceed) {
        this->CurrentKey.Initialize(fullKeyPath, false);
        this->CurrentReducedKey.Initialize(fullReducedKeyPath, false);
    }
    
    this->SomethingFailed = false;
    return ERROR_SUCCESS;
}

