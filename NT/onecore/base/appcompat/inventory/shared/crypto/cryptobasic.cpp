#include "stdafx.h"

// MD5 context structure - use consistent types with md5.cpp
typedef struct _MD5Context {
    unsigned int state[4];       // Changed from uint32_t to unsigned int
    unsigned int count[2];       // Changed from uint32_t to unsigned int
    unsigned char buffer[64];    // Changed from uint8_t to unsigned char
} MD5Context;

extern "C" {
    void MD5_Init(MD5Context* context);
    void MD5_Update(MD5Context* context, const unsigned char* input, unsigned int inputLen);
    void MD5_Final(MD5Context* context, unsigned char digest[16]);
}

uint ComputeMd5Hash(uchar* input, ulong inputSize, uchar* output) {
    if (!input || !output) {
        return 0x80070057; // E_INVALIDARG
    }

    MD5Context context;
    MD5_Init(&context);
    MD5_Update(&context, input, (unsigned int)inputSize);
    MD5_Final(&context, output);
    return 0;
}

uint ComputeSha1Hash(uchar* input, ulong inputSize, uchar* output) {
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    DWORD dwHashSize = 0;
    uint uReturn = 0x80004005;

    if (!input || !output) {
        return 0x80070057;
    }

    if (!CryptAcquireContextW(&hProv, NULL, L"Microsoft Base Cryptographic Provider v1.0", 
                            PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        uReturn = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }

    if (!CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash)) {
        uReturn = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }

    if (!CryptHashData(hHash, input, (DWORD)inputSize, 0)) {
        uReturn = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }

    dwHashSize = 20;
    if (!CryptGetHashParam(hHash, HP_HASHVAL, output, &dwHashSize, 0)) {
        uReturn = HRESULT_FROM_WIN32(GetLastError());
    }

cleanup:
    if (hHash) CryptDestroyHash(hHash);
    if (hProv) CryptReleaseContext(hProv, 0);
    return uReturn;
}

uint FormatAeHashString(AEID001_INFO* hashInfo, wchar_t* output) {
    static const wchar_t hexDigits[] = L"0123456789abcdef";

    if (!hashInfo || !output) {
        return 0x80070057;
    }

    for (int i = 0; i < sizeof(hashInfo->data); i++) {
        output[i*2] = hexDigits[(hashInfo->data[i] >> 4) & 0xF];
        output[i*2+1] = hexDigits[hashInfo->data[i] & 0xF];
    }
    output[sizeof(hashInfo->data)*2] = L'\0';
    return 0;
}

