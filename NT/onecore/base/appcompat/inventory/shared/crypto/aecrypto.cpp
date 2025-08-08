#include "stdafx.h"

// Helper macros for error handling
#define HRESULT_FROM_WIN32(x) ((x) <= 0 ? ((DWORD)(x)) : (((x) & 0x0000FFFF) | (FACILITY_WIN32 << 16) | 0x80000000))

// Main functions implementation

uint AeComputePeHeaderHash(IMAGE_NT_HEADERS* ntHeaders, ULONGLONG param2, AEID001_INFO* hashInfo) {
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    DWORD dwHashSize = 0;
    DWORD dwResult = 0;
    uint uReturn = 0x80004005; // Default error

    // Validate parameters
    if (!ntHeaders || !hashInfo) {
        return 0x80070057; // E_INVALIDARG
    }

    // Check PE header magic number
    if (*(short*)((BYTE*)ntHeaders + 0x18) == 0x10b) { // PE32
        BYTE* start = (BYTE*)ntHeaders + 0x34;
        BYTE* end = (BYTE*)ntHeaders + 0x38;

        // Validate range
        if (start >= (BYTE*)ntHeaders && end > start) {
            // Acquire crypto context
            if (!CryptAcquireContextW(&hProv, NULL, L"Microsoft Base Cryptographic Provider v1.0", PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
                dwResult = GetLastError();
                uReturn = HRESULT_FROM_WIN32(dwResult);
                goto cleanup;
            }

            // Create SHA-1 hash
            if (!CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash)) {
                dwResult = GetLastError();
                uReturn = HRESULT_FROM_WIN32(dwResult);
                goto cleanup;
            }

            // Hash the PE header data
            if (!CryptHashData(hHash, start, (DWORD)(end - start), 0)) {
                dwResult = GetLastError();
                uReturn = HRESULT_FROM_WIN32(dwResult);
                goto cleanup;
            }

            // Hash additional parameters if needed
            if (!CryptHashData(hHash, (BYTE*)&ntHeaders, sizeof(ntHeaders), 0)) {
                dwResult = GetLastError();
                uReturn = HRESULT_FROM_WIN32(dwResult);
                goto cleanup;
            }

            // Get the hash size
            dwHashSize = sizeof(hashInfo->data);
            if (!CryptGetHashParam(hHash, HP_HASHVAL, hashInfo->data, &dwHashSize, 0)) {
                dwResult = GetLastError();
                uReturn = HRESULT_FROM_WIN32(dwResult);
                goto cleanup;
            }

            if (dwHashSize == 0x14) { // SHA-1 hash size
                hashInfo->data[0] = 1; // Mark as valid
                uReturn = 0; // Success
            } else {
                uReturn = 0x8000FFFF; // E_UNEXPECTED
            }
        } else {
            uReturn = 0xD000007B; // Invalid range
        }
    } else if (*(short*)((BYTE*)ntHeaders + 0x18) == 0x20b) { // PE32+
        // Similar handling for PE32+ format
        // ... (implementation would be similar to PE32 case)
        uReturn = 0x80070057; // E_INVALIDARG for now
    } else {
        uReturn = 0x80070057; // E_INVALIDARG
    }

cleanup:
    if (hHash) {
        CryptDestroyHash(hHash);
    }
    if (hProv) {
        CryptReleaseContext(hProv, 0);
    }
    return uReturn;
}

uint AeComputeProgramIdentityHash(PROGRAM_IDENTITY_INFO* identityInfo, wchar_t* output) {
    wchar_t buffer[0x310] = {0};
    wchar_t versionBuffer[8] = {0};
    AEID001_INFO hashInfo = {0};
    uint result = 0;

    // Validate parameters
    if (!identityInfo || !output) {
        return 0x80070057; // E_INVALIDARG
    }

    // Validate and trim each string field
    if (identityInfo->name) {
        identityInfo->name = AeTrimString(identityInfo->name);
    }
    if (identityInfo->publisher) {
        identityInfo->publisher = AeTrimString(identityInfo->publisher);
    }
    if (identityInfo->version) {
        identityInfo->version = AeTrimString(identityInfo->version);
    }
    if (identityInfo->language) {
        identityInfo->language = AeTrimString(identityInfo->language);
    }

    // Format the identity string
    result = FormatNPVString(identityInfo->publisher, buffer, identityInfo->name, versionBuffer, 0, NULL);
    if (result != 0) {
        return result;
    }

    // Compute MD5 hash of the formatted string
    result = ComputeMd5Hash((uchar*)buffer, wcslen(buffer) * sizeof(wchar_t), (uchar*)&hashInfo);
    if (result != 0) {
        return result;
    }

    // Convert version string to DWORD if present
    if (versionBuffer[0] != L'\0') {
        ULONG version = 0;
        result = StringToDword(versionBuffer, 0, &version);
        if (result != 0) {
            return result;
        }
    }

    // Format the final hash string
    result = FormatAeHashString(&hashInfo, output);
    return result;
}

uint AeComputeStringHash(wchar_t* input, wchar_t* output) {
    AEID001_INFO hashInfo = {0};
    uint result = 0;

    // Validate parameters
    if (!output) {
        return 0x80070057; // E_INVALIDARG
    }

    // Compute SHA-1 hash of the input string
    result = ComputeSha1Hash((uchar*)input, (input ? wcslen(input) * sizeof(wchar_t) : 0), (uchar*)&hashInfo);
    if (result != 0) {
        return result;
    }

    // Format the hash string
    result = FormatAeHashString(&hashInfo, output);
    return result;
}

wchar_t* AeTrimString(wchar_t* str) {
    if (!str) {
        return NULL;
    }

    // Trim trailing whitespace
    wchar_t* end = str + wcslen(str) - 1;
    while (end >= str && (iswspace(*end) || *end == L'\t')) {  // Fixed missing parenthesis
        *end = L'\0';
        end--;
    }

    // Trim leading whitespace
    while (*str && (iswspace(*str) || *str == L'\t')) {
        str++;
    }

    return str;
}

uint FormatNPVString(wchar_t* p1, wchar_t* p2, wchar_t* p3, wchar_t* p4, ULONG param5, ULONG* param6) {
    // Validate parameters
    if (!p2 || !p4) {
        return 0x80070057; // E_INVALIDARG
    }

    // Format the string: "p<publisher>_<name>_<version>"
    HRESULT hr = StringCchPrintfW(p2, 0x310, L"p%s_%s_%s", p1 ? p1 : L"", p3 ? p3 : L"", p4);
    if (FAILED(hr)) {
        return hr;
    }

    // Convert to lowercase
    size_t len = 0;
    hr = StringCchLengthW(p2, 0x310, &len);
    if (FAILED(hr)) {
        return hr;
    }

    for (size_t i = 0; i < len; i++) {
        p2[i] = towlower(p2[i]);
    }

    // Store the length if requested
    if (param6) {
        *param6 = (ULONG)len;
    }

    return 0;
}

uint ComputeMd5Hash(uchar* input, ulong inputSize, uchar* output) {
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    DWORD dwHashSize = 0;
    DWORD dwResult = 0;

    if (!CryptAcquireContextW(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        dwResult = GetLastError();
        return HRESULT_FROM_WIN32(dwResult);
    }

    if (!CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash)) {
        dwResult = GetLastError();
        CryptReleaseContext(hProv, 0);
        return HRESULT_FROM_WIN32(dwResult);
    }

    if (!CryptHashData(hHash, input, inputSize, 0)) {
        dwResult = GetLastError();
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return HRESULT_FROM_WIN32(dwResult);
    }

    dwHashSize = 16; // MD5 hash size
    if (!CryptGetHashParam(hHash, HP_HASHVAL, output, &dwHashSize, 0)) {
        dwResult = GetLastError();
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return HRESULT_FROM_WIN32(dwResult);
    }

    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
    return 0;
}

uint ComputeSha1Hash(uchar* input, ulong inputSize, uchar* output) {
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    DWORD dwHashSize = 0;
    DWORD dwResult = 0;

    if (!CryptAcquireContextW(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        dwResult = GetLastError();
        return HRESULT_FROM_WIN32(dwResult);
    }

    if (!CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash)) {
        dwResult = GetLastError();
        CryptReleaseContext(hProv, 0);
        return HRESULT_FROM_WIN32(dwResult);
    }

    if (!CryptHashData(hHash, input, inputSize, 0)) {
        dwResult = GetLastError();
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return HRESULT_FROM_WIN32(dwResult);
    }

    dwHashSize = 20; // SHA-1 hash size
    if (!CryptGetHashParam(hHash, HP_HASHVAL, output, &dwHashSize, 0)) {
        dwResult = GetLastError();
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return HRESULT_FROM_WIN32(dwResult);
    }

    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
    return 0;
}

uint FormatAeHashString(AEID001_INFO* hashInfo, wchar_t* output) {
    // Format the hash as a hexadecimal string
    if (!hashInfo || !output) {
        return 0x80070057; // E_INVALIDARG
    }

    const wchar_t* hexDigits = L"0123456789abcdef";
    for (int i = 0; i < sizeof(hashInfo->data); i++) {
        output[i * 2] = hexDigits[(hashInfo->data[i] >> 4) & 0xF];
        output[i * 2 + 1] = hexDigits[hashInfo->data[i] & 0xF];
    }
    output[sizeof(hashInfo->data) * 2] = L'\0';

    return 0;
}

DWORD StringToDword(wchar_t* str, int param2, ULONG* output) {
    wchar_t* endPtr = NULL;
    
    if (!str || !output) {
        return 0x80070057; // E_INVALIDARG
    }

    *output = wcstoul(str, &endPtr, 10);
    if (endPtr == str || *endPtr != L'\0') {
        return 0x80070057; // E_INVALIDARG
    }

    return 0;
}

ULONG PageErrorExceptionHandler(_EXCEPTION_POINTERS* exceptionInfo, char* param2, ULONG param3) {
    // Basic exception handler
    return EXCEPTION_EXECUTE_HANDLER;
}

