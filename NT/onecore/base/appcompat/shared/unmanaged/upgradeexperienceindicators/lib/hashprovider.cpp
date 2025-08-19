#include <windows.h>
#include <wincrypt.h>
#include <stdlib.h>

// Add these typedefs if you want to keep using uint/uchar/ulong
typedef unsigned int uint;
typedef unsigned char uchar;
typedef unsigned long ulong;

namespace Windows {
namespace Compat {
namespace Shared {

class HashProvider {
public:
    ~HashProvider();
    uint AddToChecksum(wchar_t* data);
    uint GetChecksumValueAlloc(uchar** checksum, ulong* size);
    uint InitializeCryptProvider();

private:
    HCRYPTPROV CryptProv = 0;
    HCRYPTHASH CryptHash = 0;
};

HashProvider::~HashProvider()
{
    if (this->CryptHash != 0) {
        CryptDestroyHash(this->CryptHash);
    }
    if (this->CryptProv != 0) {
        CryptReleaseContext(this->CryptProv, 0);
    }
}

uint HashProvider::AddToChecksum(wchar_t* data)
{
    uint result = 0;
    wchar_t* ptr = data;
    
    // Calculate string length
    while (*ptr != L'\0') {
        ptr++;
    }
    
    int dataSize = ((int)(ptr - data)) * sizeof(wchar_t);
    int success = CryptHashData(this->CryptHash, (BYTE*)data, dataSize, 0);
    
    if (!success) {
        result = GetLastError();
        if (result > 0) {
            result = (result & 0xFFFF) | 0x80070000;
        }
    }
    
    return result;
}

uint HashProvider::GetChecksumValueAlloc(uchar** checksum, ulong* size)
{
    uint result = 0;
    DWORD hashSize = 4;
    
    if (this->CryptHash == 0) {
        return 0x80004005; // E_POINTER
    }
    
    // First get the hash size
    if (!CryptGetHashParam(this->CryptHash, HP_HASHSIZE, (BYTE*)&hashSize, &hashSize, 0)) {
        result = GetLastError();
        if (result > 0) {
            result = (result & 0xFFFF) | 0x80070000;
        }
        return result;
    }
    
    // Allocate memory for the hash
    uchar* hashData = (uchar*)malloc(hashSize);
    if (hashData == nullptr) {
        return 0x8007000E; // E_OUTOFMEMORY
    }
    
    // Get the hash value
    if (!CryptGetHashParam(this->CryptHash, HP_HASHVAL, hashData, &hashSize, 0)) {
        result = GetLastError();
        if (result > 0) {
            result = (result & 0xFFFF) | 0x80070000;
        }
        free(hashData);
        return result;
    }
    
    *checksum = hashData;
    *size = hashSize;
    return 0;
}

uint HashProvider::InitializeCryptProvider()
{
    uint result = 0;
    HCRYPTPROV prov = 0;
    HCRYPTHASH hash = 0;
    
    if (!CryptAcquireContextW(&prov, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_SILENT)) {
        goto error;
    }
    
    if (!CryptCreateHash(prov, CALG_SHA1, 0, 0, &hash)) {
        goto error;
    }
    
    this->CryptProv = prov;
    this->CryptHash = hash;
    prov = 0;
    hash = 0;
    
error:
    if (hash != 0) {
        CryptDestroyHash(hash);
    }
    if (prov != 0) {
        CryptReleaseContext(prov, 0);
    }
    
    if (result == 0 && this->CryptHash == 0) {
        result = GetLastError();
        if (result > 0) {
            result = (result & 0xFFFF) | 0x80070000;
        }
    }
    
    return result;
}

} // namespace Shared
} // namespace Compat
} // namespace Windows

