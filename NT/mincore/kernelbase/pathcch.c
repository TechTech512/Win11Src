#include <windows.h>
#include <strsafe.h>
#include <wchar.h>
#include <ctype.h>
#include <malloc.h>

// Forward declarations
BOOL PathIsUNCEx(const wchar_t* pszPath, wchar_t** ppszServer);
BOOL PathIsVolumeGUIDWorker(const wchar_t* pszPath);
BOOL StringIsGUIDWorker(const wchar_t* pszString);
BOOL StrIsEqualWorker(size_t cch, const wchar_t* psz1, const wchar_t* psz2);
HRESULT PathCchCombineEx(wchar_t* pszPathOut, size_t cchPathOut, const wchar_t* pszPathIn, const wchar_t* pszMore, DWORD dwFlags);
BOOL PathCchIsRoot(const wchar_t* pszPath);
HRESULT PathCchSkipRoot(const wchar_t* pszPath, wchar_t** ppszRootEnd);
HRESULT PathCchStripToRoot(wchar_t* pszPath, size_t cchPath);

int PathCchAddBackslashEx(wchar_t* pszPath, size_t cchPath, wchar_t** ppszEnd, size_t* pcchRemaining)
{
    if (pszPath != NULL) {
        pszPath[0] = L'\0';
        pszPath[1] = L'\0';
    }
    if (cchPath != 0) {
        *(DWORD*)cchPath = 0;
    }

    size_t pathLen = wcslen(pszPath);
    if (pathLen >= cchPath) {
        return STRSAFE_E_INSUFFICIENT_BUFFER;
    }

    size_t remaining = cchPath - pathLen;
    wchar_t* end = pszPath + pathLen;

    if ((pathLen == 0) || (end[-1] == L'\\')) {
        return S_OK;
    }

    int result = StringCchCopyExW(end, remaining, L"\\", NULL, NULL, 0);
    if (SUCCEEDED(result)) {
        if (ppszEnd != NULL) {
            *ppszEnd = end;
        }
        if (pcchRemaining != NULL) {
            *pcchRemaining = remaining - 1;
        }
    }

    return result;
}

HRESULT PathCchAppend(wchar_t* pszPath, size_t cchPath, const wchar_t* pszMore)
{
    if (pszMore == NULL) {
        return E_INVALIDARG;
    }

    const wchar_t* pathToAppend = pszMore;
    if (!PathIsUNCEx(pszPath, NULL)) {
        if (!StrIsEqualWorker(4, pszPath, 0)) {
            while (*pathToAppend == L'\\') {
                pathToAppend++;
            }
        }
    }

    return PathCchCombineEx(pszPath, cchPath, pszPath, pathToAppend, 0);
}

HRESULT PathCchCanonicalizeEx(wchar_t* pszPathOut, size_t cchPathOut, const wchar_t* pszPathIn, DWORD dwFlags)
{
    if (cchPathOut > MAX_PATH) {
        return STRSAFE_E_INVALID_PARAMETER;
    }

    size_t bufSize = (cchPathOut < MAX_PATH) ? cchPathOut : MAX_PATH;
    wchar_t* buffer = pszPathOut;
    HRESULT hr = StringCchCopyW(buffer, bufSize, pszPathIn);

    if (FAILED(hr)) {
        return hr;
    }

    if (PathIsUNCEx(buffer, NULL)) {
        hr = StringCchCopyExW(buffer, bufSize, L"\\\\", NULL, NULL, 0);
        if (FAILED(hr)) {
            return hr;
        }
    }
    else {
        if (StrIsEqualWorker(4, buffer, 0)) {
            if (iswalpha(buffer[4]) && buffer[5] == L':') {
                buffer += 4;
            }
        }
    }

    wchar_t* current = buffer;
    while (*current != L'\0') {
        const wchar_t* nextSlash = wcschr(current, L'\\');
        size_t segmentLen = (nextSlash == NULL) ? wcslen(current) : (nextSlash - current);

        if (segmentLen > 260 || segmentLen > 32767) {
            return STRSAFE_E_INVALID_PARAMETER;
        }

        if (segmentLen == 1 && *current == L'.') {
            if (nextSlash == NULL) {
                current++;
                if (current > buffer && !PathCchIsRoot(buffer)) {
                    current[-1] = L'\0';
                }
            }
            else {
                current = (wchar_t*)(nextSlash + 1);
            }
        }
        else if (segmentLen == 2 && current[0] == L'.' && current[1] == L'.') {
            if (current > buffer && !PathCchIsRoot(buffer)) {
                wchar_t* prevSlash = current - 1;
                while (prevSlash >= buffer && *prevSlash != L'\\') {
                    prevSlash--;
                }

                if (prevSlash >= buffer) {
                    *prevSlash = L'\0';
                    current = prevSlash;
                }
                else {
                    *buffer = L'\0';
                    current = buffer;
                }
            }
            else if (nextSlash == NULL) {
                current += 2;
            }
            else {
                current = (wchar_t*)(nextSlash + 1);
            }
        }
        else {
            hr = StringCchCopyNExW(current, segmentLen, buffer, &bufSize, NULL, NULL, 0);
            if (hr == STRSAFE_E_INSUFFICIENT_BUFFER && segmentLen == 1 && *current == L'\\') {
                if (current[1] == L'\0' || (current[1] == L'.' && current[2] == L'\0')) {
                    hr = S_OK;
                    continue;
                }
                if (bufSize == 1 && current[1] == L'.' && current[2] == L'.') {
                    *buffer = L'\0';
                    bufSize = 0;
                    hr = S_OK;
                }
            }

            if (FAILED(hr)) {
                return hr;
            }

            current += segmentLen;
        }
    }

    if (wcslen(buffer) > 0) {
        if (buffer[0] == L'\0' && bufSize > 1) {
            buffer[0] = L'\\';
            buffer[1] = L'\0';
        }
        if (buffer[1] == L':' && buffer[2] == L'\0' && bufSize > 3) {
            buffer[2] = L'\\';
            buffer[3] = L'\0';
        }
    }

    return S_OK;
}

HRESULT PathCchCombineEx(wchar_t* pszPathOut, size_t cchPathOut, const wchar_t* pszPathIn, const wchar_t* pszMore, DWORD dwFlags)
{
    if (pszPathOut == NULL || cchPathOut == 0 || cchPathOut > 32767) {
        return E_INVALIDARG;
    }

    size_t path1Len = 0;
    size_t path2Len = 0;
    wchar_t* combinedPath = NULL;

    if (pszPathIn != NULL) {
        path1Len = wcslen(pszPathIn);
        if (path1Len > 32767) {
            return STRSAFE_E_INVALID_PARAMETER;
        }
        if (path1Len > 0) {
            path1Len++;
        }
    }

    if (pszMore != NULL) {
        path2Len = wcslen(pszMore);
        if (path2Len > 32767) {
            return STRSAFE_E_INVALID_PARAMETER;
        }
        if (path2Len > 0) {
            path2Len++;
        }
    }

    if (path1Len + path2Len < MAX_PATH) {
        combinedPath = (wchar_t*)_alloca((path1Len + path2Len + 1) * sizeof(wchar_t));
    }
    else {
        combinedPath = (wchar_t*)LocalAlloc(LMEM_FIXED, (path1Len + path2Len + 1) * sizeof(wchar_t));
        if (combinedPath == NULL) {
            return E_OUTOFMEMORY;
        }
    }

    if (path1Len == 0) {
        if (path2Len == 0) {
            *combinedPath = L'\0';
        }
        else {
            HRESULT hr = StringCchCopyW(combinedPath, path1Len + path2Len, pszMore);
            if (FAILED(hr)) {
                if (combinedPath != pszPathOut) {
                    LocalFree(combinedPath);
                }
                return hr;
            }
        }
    }
    else {
        HRESULT hr = StringCchCopyW(combinedPath, path1Len, pszPathIn);
        if (FAILED(hr)) {
            if (combinedPath != pszPathOut) {
                LocalFree(combinedPath);
            }
            return hr;
        }

        if (path2Len > 0) {
            if (*pszMore == L'\\') {
                if (pszMore[1] != L'\\') {
                    hr = PathCchStripToRoot(combinedPath, path1Len);
                    if (FAILED(hr)) {
                        if (combinedPath != pszPathOut) {
                            LocalFree(combinedPath);
                        }
                        return hr;
                    }

                    hr = PathCchAddBackslashEx(combinedPath, path1Len, NULL, NULL);
                    if (FAILED(hr)) {
                        if (combinedPath != pszPathOut) {
                            LocalFree(combinedPath);
                        }
                        return hr;
                    }
                }
            }
            else if (iswalpha(*pszMore) && pszMore[1] == L':') {
                // Absolute path - just copy it
            }
            else {
                hr = PathCchAddBackslashEx(combinedPath, path1Len, NULL, NULL);
                if (FAILED(hr)) {
                    if (combinedPath != pszPathOut) {
                        LocalFree(combinedPath);
                    }
                    return hr;
                }
            }

            hr = StringCchCatW(combinedPath, path1Len + path2Len, pszMore);
            if (FAILED(hr)) {
                if (combinedPath != pszPathOut) {
                    LocalFree(combinedPath);
                }
                return hr;
            }
        }
    }

    HRESULT hr = PathCchCanonicalizeEx(pszPathOut, cchPathOut, combinedPath, dwFlags);
    if (combinedPath != pszPathOut) {
        LocalFree(combinedPath);
    }
    return hr;
}

BOOL PathCchIsRoot(const wchar_t* pszPath)
{
    if (pszPath == NULL || *pszPath == L'\0') {
        return FALSE;
    }

    if (iswalpha(*pszPath) && pszPath[1] == L':' && pszPath[2] == L'\\' && pszPath[3] == L'\0') {
        return TRUE;
    }

    if (*pszPath == L'\\' && pszPath[1] == L'\\') {
        if (pszPath[2] == L'?') {
            if (StrIsEqualWorker(5, pszPath, 0)) {
                return FALSE;
            }
        }
        else {
            if (PathIsVolumeGUIDWorker(pszPath)) {
                return FALSE;
            }
        }

        int backslashCount = 0;
        while (*pszPath != L'\0') {
            if (*pszPath == L'\\' && ++backslashCount > 1) {
                return FALSE;
            }
            pszPath++;
        }
        return TRUE;
    }

    if (StrIsEqualWorker(4, pszPath, 0)) {
        if (iswalpha(pszPath[4]) && pszPath[5] == L':') {
            if (StrIsEqualWorker(3, pszPath + 4, 0)) {
                return TRUE;
            }
        }
    }

    if (PathIsVolumeGUIDWorker(pszPath)) {
        if (pszPath[48] == L'\\' && pszPath[49] == L'\0') {
            return TRUE;
        }
    }

    return FALSE;
}

HRESULT PathCchRemoveBackslash(wchar_t* pszPath, size_t cchPath)
{
    size_t pathLen = wcslen(pszPath);
    if (pathLen >= cchPath) {
        return STRSAFE_E_INSUFFICIENT_BUFFER;
    }

    if (pathLen == 0 || pszPath[pathLen - 1] != L'\\') {
        return S_FALSE;
    }

    if (!PathCchIsRoot(pszPath)) {
        pszPath[pathLen - 1] = L'\0';
        return S_OK;
    }

    return S_FALSE;
}

HRESULT PathCchRemoveFileSpec(wchar_t* pszPath, size_t cchPath)
{
    if (pszPath == NULL) {
        return E_INVALIDARG;
    }

    HRESULT hr = PathCchSkipRoot(pszPath, NULL);
    if (FAILED(hr)) {
        hr = S_OK;
    }

    if (cchPath > MAX_PATH) {
        return E_INVALIDARG;
    }

    wchar_t* lastSlash = NULL;
    wchar_t* current = pszPath;
    while (*current != L'\0') {
        wchar_t* slash = wcschr(current, L'\\');
        if (slash == NULL) {
            break;
        }

        lastSlash = slash;
        if (slash - pszPath >= (ptrdiff_t)cchPath) {
            return E_INVALIDARG;
        }
        current = slash + 1;
    }

    if (lastSlash == NULL) {
        return PathCchRemoveBackslash(pszPath, cchPath);
    }
    else {
        *lastSlash = L'\0';
        PathCchRemoveBackslash(pszPath, cchPath);
    }

    return S_OK;
}

HRESULT PathCchSkipRoot(const wchar_t* pszPath, wchar_t** ppszRootEnd)
{
    if (pszPath == NULL || *pszPath == L'\0' || ppszRootEnd == NULL) {
        return E_INVALIDARG;
    }

    *ppszRootEnd = NULL;

    if (PathIsUNCEx(pszPath, ppszRootEnd)) {
        return S_OK;
    }

    if (PathIsVolumeGUIDWorker(pszPath)) {
        *ppszRootEnd = (wchar_t*)(pszPath + 48);
        return S_OK;
    }

    if (StrIsEqualWorker(4, pszPath, 0)) {
        pszPath += 4;
    }

    if (!iswalpha(*pszPath) || pszPath[1] != L':') {
        return E_INVALIDARG;
    }

    pszPath += 2;
    if (*pszPath != L'\\') {
        return E_INVALIDARG;
    }

    *ppszRootEnd = (wchar_t*)(pszPath + 1);
    return S_OK;
}

HRESULT PathCchStripToRoot(wchar_t* pszPath, size_t cchPath)
{
    if (pszPath == NULL || cchPath == 0 || cchPath > 32767) {
        return E_INVALIDARG;
    }

    wchar_t* rootEnd = NULL;
    HRESULT hr = PathCchSkipRoot(pszPath, &rootEnd);
    if (FAILED(hr)) {
        return hr;
    }

    if (rootEnd - pszPath >= (ptrdiff_t)cchPath) {
        return E_INVALIDARG;
    }

    if (*rootEnd == L'\0') {
        hr = PathCchRemoveBackslash(pszPath, cchPath);
    }
    else {
        *rootEnd = L'\0';
        PathCchRemoveBackslash(pszPath, cchPath);
    }

    if (FAILED(hr)) {
        StringCchCopyW(pszPath, cchPath, L"");
    }

    return hr;
}

BOOL PathIsUNCEx(const wchar_t* pszPath, wchar_t** ppszServer)
{
    if (ppszServer != NULL) {
        *ppszServer = NULL;
    }

    if (pszPath[0] != L'\\' || pszPath[1] != L'\\') {
        return FALSE;
    }

    int prefixLen = 2;
    if (pszPath[2] == L'?') {
        if (StrIsEqualWorker(5, pszPath, 0)) {
            return FALSE;
        }
        prefixLen = 8;
    }
    else {
        if (PathIsVolumeGUIDWorker(pszPath)) {
            return FALSE;
        }
    }

    if (ppszServer != NULL) {
        *ppszServer = (wchar_t*)(pszPath + prefixLen);
    }

    return TRUE;
}

BOOL PathIsVolumeGUIDWorker(const wchar_t* pszPath)
{
    if (!StrIsEqualWorker(10, pszPath, 0)) {
        return FALSE;
    }

    return StringIsGUIDWorker(pszPath + 10);
}

BOOL StringIsGUIDWorker(const wchar_t* pszString)
{
    static const wchar_t GUID_FORMAT[] = L"{00000000-0000-0000-0000-000000000000}";

    for (size_t i = 0; i < 38; i++) {
        wchar_t c = pszString[i];
        wchar_t f = GUID_FORMAT[i];

        if (c != f && (f != L'0' || 
            ((c < L'0' || c > L'9') && 
             (c < L'A' || c > L'F') && 
             (c < L'a' || c > L'f')))) {
            return FALSE;
        }
    }

    return TRUE;
}

BOOL StrIsEqualWorker(size_t cch, const wchar_t* psz1, const wchar_t* psz2)
{
    while (cch-- > 0 && *psz1 && *psz2) {
        wchar_t c1 = towlower(*psz1++);
        wchar_t c2 = towlower(*psz2++);
        if (c1 != c2) {
            return FALSE;
        }
    }

    return (*psz1 == *psz2);
}

