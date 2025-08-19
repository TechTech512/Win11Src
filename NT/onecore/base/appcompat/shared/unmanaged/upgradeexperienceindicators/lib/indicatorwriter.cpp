#include <windows.h>
#include <ktmw32.h>
#include <string>
#include <vector>
#include <stdlib.h>
#include <wincrypt.h>
#include <strsafe.h>

namespace Windows {
namespace Compat {
namespace Shared {

class RegistryKey {
public:
    HKEY Key;
    
    RegistryKey() : Key(nullptr) {}
    
    unsigned int Initialize(const wchar_t* subKey, bool readOnly = false) {
        DWORD options = readOnly ? KEY_READ : KEY_ALL_ACCESS;
        LONG result = RegCreateKeyExW(HKEY_LOCAL_MACHINE, subKey, 0, NULL, 
                                    REG_OPTION_NON_VOLATILE, options, NULL, &Key, NULL);
        return (result == ERROR_SUCCESS) ? 0 : result;
    }
    
    unsigned int Initialize(const wchar_t* subKey, void* transaction) {
        LONG result = RegCreateKeyTransactedW(HKEY_LOCAL_MACHINE, subKey, 0, NULL, 
                                           REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, 
                                           NULL, &Key, NULL, (HANDLE)transaction, NULL);
        return (result == ERROR_SUCCESS) ? 0 : result;
    }
    
    unsigned int WriteKeyValueString(const wchar_t* valueName, const wchar_t* value) {
        if (!Key) return ERROR_INVALID_HANDLE;
        LONG result = RegSetValueExW(Key, valueName, 0, REG_SZ, 
                                   (const BYTE*)value, (wcslen(value) + 1) * sizeof(wchar_t));
        return (result == ERROR_SUCCESS) ? 0 : result;
    }
    
    unsigned int WriteKeyValueTimestamp(const wchar_t* valueName) {
        if (!Key) return ERROR_INVALID_HANDLE;
        FILETIME currentTime;
        GetSystemTimeAsFileTime(&currentTime);
        LONG result = RegSetValueExW(Key, valueName, 0, REG_BINARY, 
                                   (const BYTE*)&currentTime, sizeof(currentTime));
        return (result == ERROR_SUCCESS) ? 0 : result;
    }
    
    unsigned int ClearKey() {
        if (!Key) return ERROR_INVALID_HANDLE;
        
        DWORD index = 0;
        wchar_t valueName[256];
        DWORD valueNameSize = _countof(valueName);
        
        while (RegEnumValueW(Key, index, valueName, &valueNameSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
            RegDeleteValueW(Key, valueName);
            valueNameSize = _countof(valueName);
            index++;
        }
        return 0;
    }
    
    void UnInitialize() {
        if (Key) {
            RegCloseKey(Key);
            Key = nullptr;
        }
    }
};

class RegistryChecksum {
public:
    static unsigned int Create(RegistryKey* targetKey, RegistryKey* sourceKey, const wchar_t* valueName) {
        HCRYPTPROV hProv = 0;
        HCRYPTHASH hHash = 0;
        BYTE hash[32];
        DWORD hashSize = sizeof(hash);
        
        if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
            return GetLastError();
        }
        
        if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
            CryptReleaseContext(hProv, 0);
            return GetLastError();
        }
        
        // Hash the registry key contents
        HashRegistryKey(hHash, sourceKey->Key);
        
        if (!CryptGetHashParam(hHash, HP_HASHVAL, hash, &hashSize, 0)) {
            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);
            return GetLastError();
        }
        
        LONG result = RegSetValueExW(targetKey->Key, valueName, 0, REG_BINARY, hash, hashSize);
        
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return (result == ERROR_SUCCESS) ? 0 : result;
    }
    
    static unsigned int IsIntact(bool* isIntact, RegistryKey* storedKey, RegistryKey* currentKey, const wchar_t* valueName) {
        *isIntact = false;
        
        // Read stored checksum
        DWORD storedChecksumSize = 0;
        LONG result = RegQueryValueExW(storedKey->Key, valueName, NULL, NULL, NULL, &storedChecksumSize);
        if (result != ERROR_SUCCESS) return result;
        
        std::vector<BYTE> storedChecksum(storedChecksumSize);
        DWORD type;
        result = RegQueryValueExW(storedKey->Key, valueName, NULL, &type, &storedChecksum[0], &storedChecksumSize);
        if (result != ERROR_SUCCESS) return result;
        
        // Calculate current checksum
        HCRYPTPROV hProv = 0;
        HCRYPTHASH hHash = 0;
        BYTE currentChecksum[32];
        DWORD currentChecksumSize = sizeof(currentChecksum);
        
        if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
            return GetLastError();
        }
        
        if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
            CryptReleaseContext(hProv, 0);
            return GetLastError();
        }
        
        HashRegistryKey(hHash, currentKey->Key);
        
        if (!CryptGetHashParam(hHash, HP_HASHVAL, currentChecksum, &currentChecksumSize, 0)) {
            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);
            return GetLastError();
        }
        
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        
        // Compare checksums
        *isIntact = (storedChecksumSize == currentChecksumSize) && 
                    (memcmp(&storedChecksum[0], currentChecksum, currentChecksumSize) == 0);
        
        return 0;
    }
    
private:
    static void HashRegistryKey(HCRYPTHASH hHash, HKEY hKey) {
        DWORD index = 0;
        wchar_t valueName[256];
        DWORD valueNameSize = _countof(valueName);
        DWORD valueType = 0;
        BYTE valueData[1024];
        DWORD valueDataSize = sizeof(valueData);
        
        while (RegEnumValueW(hKey, index, valueName, &valueNameSize, NULL, &valueType, valueData, &valueDataSize) == ERROR_SUCCESS) {
            // Hash value name
            CryptHashData(hHash, (const BYTE*)valueName, valueNameSize * sizeof(wchar_t), 0);
            
            // Hash value type
            CryptHashData(hHash, (const BYTE*)&valueType, sizeof(valueType), 0);
            
            // Hash value data
            if (valueDataSize > 0) {
                CryptHashData(hHash, valueData, valueDataSize, 0);
            }
            
            valueNameSize = _countof(valueName);
            valueDataSize = sizeof(valueData);
            index++;
        }
    }
};

class IndicatorTelemetry {
public:
    bool IsSendingNonReducedTelemetry;
    bool IsFullSync;
    unsigned int EventCount;
    void* CorrelationVector;

    IndicatorTelemetry() : 
        IsSendingNonReducedTelemetry(false), 
        IsFullSync(false), 
        EventCount(0), 
        CorrelationVector(nullptr) {}
    
    unsigned int SendIndicator(const wchar_t* name, const wchar_t* value, bool isReduced, RegistryKey* key) {
        // Simplified telemetry implementation
        EventCount++;
        return 0;
    }
    
    unsigned int SendIndicatorsRemoved(RegistryKey* currentKey, RegistryKey* key, bool isFullSync) {
        // Simplified telemetry implementation
        return 0;
    }
    
    unsigned int Initialize(const wchar_t* source, const wchar_t* type, bool isFullSync, 
                   const wchar_t* syncId, RegistryKey* key, const wchar_t* utcSyncId) {
        IsFullSync = isFullSync;
        return 0;
    }
    
    unsigned int UnInitialize() {
        return 0;
    }
};

class IndicatorWriter {
public:
    void* Transaction;
    RegistryKey Key;
    RegistryKey KeyCurrent;
    RegistryKey KeyShared;
    RegistryKey KeyReduced;
    RegistryKey KeyReducedCurrent;
    RegistryKey KeyReducedShared;
    IndicatorTelemetry Telemetry;
    bool ContainsReducedData;
    bool SentFinalTelemetry;

    IndicatorWriter() {
        Transaction = (void*)-1;
        Key.Key = nullptr;
        KeyCurrent.Key = nullptr;
        KeyShared.Key = nullptr;
        KeyReduced.Key = nullptr;
        KeyReducedCurrent.Key = nullptr;
        KeyReducedShared.Key = nullptr;
        ContainsReducedData = false;
        SentFinalTelemetry = false;
    }

    unsigned int Add(const wchar_t* name, const wchar_t* value, bool isReduced) {
        if (!ContainsReducedData && isReduced) {
            return 0x80070057; // E_INVALIDARG
        }

        RegistryKey* targetKey = isReduced ? &KeyReduced : &Key;
        unsigned int result = targetKey->WriteKeyValueString(name, value);
        if (result != 0) {
            return result;
        }

        return Telemetry.SendIndicator(name, value, isReduced, targetKey);
    }

    unsigned int Initialize(const wchar_t* basePath, const wchar_t* indicatorType, 
                   bool isFullSync, bool verifyChecksum, 
                   const char* syncId, bool containsReducedData) {
        if (!basePath || !indicatorType || !syncId) {
            return 0x80070057; // E_INVALIDARG
        }

        ContainsReducedData = containsReducedData;
        SentFinalTelemetry = false;

        // Create transaction
        Transaction = CreateTransaction(nullptr, nullptr, 0, 0, 0, 0, nullptr);
        if (Transaction == (void*)-1) {
            unsigned int err = GetLastError();
            return (err > 0) ? (err & 0xFFFF) | 0x80070000 : err;
        }

        // Set up registry keys
        unsigned int result = SetUpRegistryKeys(basePath, indicatorType);
        if (result != 0) {
            return result;
        }

        // Verify checksums if requested
        if (verifyChecksum) {
            bool checksumValid = true;
            if (!ContainsReducedData) {
                result = RegistryChecksum::IsIntact(&checksumValid, &KeyShared, &Key, indicatorType);
                if (result != 0 || !checksumValid) {
                    // Rollback transaction and return error
                    DoRollbackTransaction();
                    return 0x80070057; // E_INVALIDARG
                }
            } else {
                result = RegistryChecksum::IsIntact(&checksumValid, &KeyShared, &Key, indicatorType);
                if (result != 0 || !checksumValid) {
                    // Rollback transaction and return error
                    DoRollbackTransaction();
                    return 0x80070057; // E_INVALIDARG
                }

                bool reducedChecksumValid = true;
                result = RegistryChecksum::IsIntact(&reducedChecksumValid, &KeyReducedShared, 
                                                  &KeyReduced, indicatorType);
                if (result != 0 || !reducedChecksumValid) {
                    // Rollback transaction and return error
                    DoRollbackTransaction();
                    return 0x80070057; // E_INVALIDARG
                }
                checksumValid &= reducedChecksumValid;
            }
        }

        // Set up telemetry
        result = SetUpTelemetry(basePath, indicatorType, isFullSync, syncId);
        if (result != 0) {
            DoRollbackTransaction();
            return result;
        }

        // Clear existing keys
        result = Key.ClearKey();
        if (result != 0) {
            DoRollbackTransaction();
            return result;
        }

        result = KeyShared.ClearKey();
        if (result != 0) {
            DoRollbackTransaction();
            return result;
        }

        if (ContainsReducedData) {
            result = KeyReduced.ClearKey();
            if (result != 0) {
                DoRollbackTransaction();
                return result;
            }

            result = KeyReducedShared.ClearKey();
            if (result != 0) {
                DoRollbackTransaction();
                return result;
            }
        }

        return 0;
    }

    unsigned int SendFinalTelemetry() {
        if (SentFinalTelemetry) {
            return 0;
        }

        unsigned int result = Telemetry.SendIndicatorsRemoved(&KeyCurrent, &Key, true);
        if (result != 0) {
            return result;
        }

        if (ContainsReducedData) {
            result = Telemetry.SendIndicatorsRemoved(&KeyReducedCurrent, &KeyReduced, false);
            if (result != 0) {
                return result;
            }
        }

        result = Telemetry.UnInitialize();
        SentFinalTelemetry = true;
        return result;
    }

    unsigned int SetUpRegistryKeys(const wchar_t* basePath, const wchar_t* indicatorType) {
        wchar_t path[260];
        
        // Create main path
        if (StringCchPrintfW(path, _countof(path), L"%s\\%s", basePath, indicatorType) != S_OK) {
            return 0x80070057; // E_INVALIDARG
        }

        unsigned int result = Key.Initialize(path, Transaction);
        if (result != 0) return result;

        result = KeyCurrent.Initialize(path, true); // read-only
        if (result != 0) return result;

        if (ContainsReducedData) {
            if (StringCchPrintfW(path, _countof(path), L"%s\\%s_Reduced", basePath, indicatorType) != S_OK) {
                return 0x80070057; // E_INVALIDARG
            }

            result = KeyReduced.Initialize(path, Transaction);
            if (result != 0) return result;

            result = KeyReducedCurrent.Initialize(path, true); // read-only
            if (result != 0) return result;
        }

        // Create shared path
        if (StringCchPrintfW(path, _countof(path), L"%s\\%s_Shared", basePath, indicatorType) != S_OK) {
            return 0x80070057; // E_INVALIDARG
        }

        result = KeyShared.Initialize(path, Transaction);
        if (result != 0) return result;

        if (ContainsReducedData) {
            if (StringCchPrintfW(path, _countof(path), L"%s\\%s_Shared_Reduced", basePath, indicatorType) != S_OK) {
                return 0x80070057; // E_INVALIDARG
            }

            result = KeyReducedShared.Initialize(path, Transaction);
            if (result != 0) return result;
        }

        return 0;
    }

    unsigned int SetUpTelemetry(const wchar_t* source, const wchar_t* type, 
                       bool isFullSync, const char* syncId) {
        wchar_t eventName[262];
        wchar_t syncIdWide[256];
        
        // Convert syncId from char* to wchar_t*
        size_t converted = 0;
        mbstowcs_s(&converted, syncIdWide, syncId, _countof(syncIdWide));
        
        if (StringCchPrintfW(eventName, _countof(eventName), 
                           L"InventoryMiscellaneousUexIndicator_%s_%s", 
                           type, syncIdWide) != S_OK) {
            return 0x80070057; // E_INVALIDARG
        }

        return Telemetry.Initialize(source, type, isFullSync, eventName, &Key, L"UtcSyncId");
    }

    void DoRollbackTransaction() {
        if (Transaction != (void*)-1) {
            RollbackTransaction((HANDLE)Transaction);
            CloseHandle(Transaction);
            Transaction = (void*)-1;
        }
    }

    void UnInitialize() {
        SendFinalTelemetry();
        
        Key.UnInitialize();
        KeyCurrent.UnInitialize();
        KeyShared.UnInitialize();
        KeyReduced.UnInitialize();
        KeyReducedCurrent.UnInitialize();
        KeyReducedShared.UnInitialize();
        
        if (Transaction != (void*)-1) {
            CloseHandle(Transaction);
            Transaction = (void*)-1;
        }
    }

    unsigned int Write() {
        unsigned int result = SendFinalTelemetry();
        if (result != 0) {
            DoRollbackTransaction();
            return result;
        }

        result = Key.WriteKeyValueTimestamp(L"Timestamp");
        if (result != 0) {
            DoRollbackTransaction();
            return result;
        }

        if (ContainsReducedData) {
            result = KeyReduced.WriteKeyValueTimestamp(L"Timestamp");
            if (result != 0) {
                DoRollbackTransaction();
                return result;
            }
        }

        result = RegistryChecksum::Create(&KeyShared, &Key, L"Checksum");
        if (result != 0) {
            DoRollbackTransaction();
            return result;
        }

        if (ContainsReducedData) {
            result = RegistryChecksum::Create(&KeyReducedShared, &KeyReduced, L"Checksum");
            if (result != 0) {
                DoRollbackTransaction();
                return result;
            }
        }

        if (!CommitTransaction((HANDLE)Transaction)) {
            result = GetLastError();
            DoRollbackTransaction();
            return (result > 0) ? (result & 0xFFFF) | 0x80070000 : result;
        }

        CloseHandle(Transaction);
        Transaction = (void*)-1;
        return 0;
    }
};

} // namespace Shared
} // namespace Compat
} // namespace Windows
