#include <windows.h>
#include <wincrypt.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <algorithm>

// Add typedef for uint if not already defined
typedef unsigned int uint;

namespace Windows {
namespace Compat {
namespace Shared {

class HashProvider {
public:
    HCRYPTPROV CryptProv = 0;
    HCRYPTHASH CryptHash = 0;

    ~HashProvider() {
        if (CryptHash) CryptDestroyHash(CryptHash);
        if (CryptProv) CryptReleaseContext(CryptProv, 0);
    }

    uint InitializeCryptProvider() {
        if (!CryptAcquireContextW(&CryptProv, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
            return GetLastError() | 0x80070000;
        }

        if (!CryptCreateHash(CryptProv, CALG_SHA1, 0, 0, &CryptHash)) {
            DWORD err = GetLastError();
            CryptReleaseContext(CryptProv, 0);
            CryptProv = 0;
            return err | 0x80070000;
        }

        return 0;
    }
};

class RegistryKey {
public:
    HKEY Key = nullptr;
};

struct RegistryValue {
    std::wstring name;
    DWORD type;
    std::vector<BYTE> data;
};

class RegistryChecksum {
public:
    HashProvider Hash;

    uint AddKey(HKEY hKey) {
        DWORD maxValueNameLen = 0;
        DWORD maxValueDataLen = 0;
        DWORD valueCount = 0;
        
        // Get registry key information
        if (RegQueryInfoKeyW(hKey, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                            &valueCount, &maxValueNameLen, &maxValueDataLen, nullptr, nullptr) != ERROR_SUCCESS) {
            return GetLastError() | 0x80070000;
        }

        maxValueNameLen++;
        std::vector<wchar_t> valueName(maxValueNameLen);
        std::vector<BYTE> valueData(maxValueDataLen);

        // Initialize hash provider if needed
        if (!Hash.CryptHash && Hash.InitializeCryptProvider() != 0) {
            return GetLastError() | 0x80070000;
        }

        // Process all values in the key
        for (DWORD i = 0; i < valueCount; i++) {
            DWORD nameLen = maxValueNameLen;
            DWORD dataLen = maxValueDataLen;
            DWORD type = 0;

            if (RegEnumValueW(hKey, i, &valueName[0], &nameLen, 
                            nullptr, &type, &valueData[0], &dataLen) != ERROR_SUCCESS) {
                continue;
            }

            // Add value name to hash
            if (!CryptHashData(Hash.CryptHash, 
                             reinterpret_cast<const BYTE*>(&valueName[0]), 
                             nameLen * sizeof(wchar_t), 0)) {
                return GetLastError() | 0x80070000;
            }

            // Add value data to hash based on type
            switch (type) {
                case REG_SZ:
                case REG_EXPAND_SZ:
                case REG_MULTI_SZ:
                    if (!CryptHashData(Hash.CryptHash, &valueData[0], dataLen, 0)) {
                        return GetLastError() | 0x80070000;
                    }
                    break;

                case REG_DWORD:
                case REG_QWORD:
                case REG_BINARY:
                    if (!CryptHashData(Hash.CryptHash, &valueData[0], dataLen, 0)) {
                        return GetLastError() | 0x80070000;
                    }
                    break;

                default:
                    // Skip other types
                    break;
            }
        }

        return 0;
    }

    uint AddUnique() {
        FILETIME ft;
        GetSystemTimeAsFileTime(&ft);

        if (!Hash.CryptHash && Hash.InitializeCryptProvider() != 0) {
            return GetLastError() | 0x80070000;
        }

        if (!CryptHashData(Hash.CryptHash, reinterpret_cast<BYTE*>(&ft), sizeof(ft), 0)) {
            return GetLastError() | 0x80070000;
        }

        return 0;
    }

    uint GetChecksumValueAlloc(BYTE** checksum, DWORD* size) {
        DWORD hashSize = 0;
        DWORD dwSize = sizeof(DWORD);
        
        if (!CryptGetHashParam(Hash.CryptHash, HP_HASHSIZE, reinterpret_cast<BYTE*>(&hashSize), &dwSize, 0)) {
            return GetLastError() | 0x80070000;
        }

        *checksum = static_cast<BYTE*>(malloc(hashSize));
        if (!*checksum) {
            return ERROR_OUTOFMEMORY | 0x80070000;
        }

        if (!CryptGetHashParam(Hash.CryptHash, HP_HASHVAL, *checksum, &hashSize, 0)) {
            DWORD err = GetLastError();
            free(*checksum);
            *checksum = nullptr;
            return err | 0x80070000;
        }

        *size = hashSize;
        return 0;
    }

    static uint CalculateRegistryChecksumAlloc(BYTE** checksum, DWORD* size, HKEY hKey) {
        RegistryChecksum rc;
        uint result = rc.Hash.InitializeCryptProvider();
        if (result != 0) return result;

        result = rc.AddKey(hKey);
        if (result != 0) return result;

        result = rc.AddUnique();
        if (result != 0) return result;

        return rc.GetChecksumValueAlloc(checksum, size);
    }

    static uint Create(HKEY targetKey, HKEY sourceKey, const wchar_t* valueName) {
        BYTE* checksum = nullptr;
        DWORD size = 0;
        
        uint result = CalculateRegistryChecksumAlloc(&checksum, &size, sourceKey);
        if (result != 0) return result;

        result = RegSetValueExW(targetKey, valueName, 0, REG_BINARY, checksum, size);
        free(checksum);
        
        return (result == ERROR_SUCCESS) ? 0 : (result | 0x80070000);
    }

    static uint IsIntact(bool* isIntact, HKEY storedKey, HKEY currentKey, const wchar_t* valueName) {
        *isIntact = false;
        
        // Read stored checksum
        DWORD storedSize = 0;
        if (RegQueryValueExW(storedKey, valueName, nullptr, nullptr, nullptr, &storedSize) != ERROR_SUCCESS) {
            return GetLastError() | 0x80070000;
        }

        std::vector<BYTE> storedChecksum(storedSize);
        if (RegQueryValueExW(storedKey, valueName, nullptr, nullptr, &storedChecksum[0], &storedSize) != ERROR_SUCCESS) {
            return GetLastError() | 0x80070000;
        }

        // Calculate current checksum
        BYTE* currentChecksum = nullptr;
        DWORD currentSize = 0;
        uint result = CalculateRegistryChecksumAlloc(&currentChecksum, &currentSize, currentKey);
        if (result != 0) return result;

        // Compare checksums
        *isIntact = (storedSize == currentSize) && 
                   (memcmp(&storedChecksum[0], currentChecksum, storedSize) == 0);

        free(currentChecksum);
        return 0;
    }
};

} // namespace Shared
} // namespace Compat
} // namespace Windows

