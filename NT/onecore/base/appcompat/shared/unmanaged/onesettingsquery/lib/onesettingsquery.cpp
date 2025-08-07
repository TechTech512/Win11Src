#include <windows.h>
#include <winhttp.h>
#include <strsafe.h>
#include <shlwapi.h>
#include <wchar.h>
#include <stdlib.h>
#include <atlbase.h>
#include <rtlarray.h>

class OneSettingsQuery
{
public:
    RtlNameValueArray m_ValueMap;
    HINTERNET m_Internet;
    HINTERNET m_Connection;
    HINTERNET m_Request;
    wchar_t* m_ETag;
    wchar_t* m_Partner;
    wchar_t* m_Feature;
    ULONG m_RefreshIntervalMinutes;

    OneSettingsQuery();
    ~OneSettingsQuery();
    
    int CreateHeaders(wchar_t* param1, wchar_t** param2);
    int CreateQueryString(wchar_t* buffer, ULONG size);
    ULONG GetETagHeaderFromRegistry(wchar_t** param1);
    int GetSetting(wchar_t* name, wchar_t* value, ULONG size);
    int GetSettingDword(wchar_t* name, ULONG* value);
    RtlNameValueArray* GetValueMap();
    ULONG Initialize(wchar_t* partner, wchar_t* feature, wchar_t* host, ULONG flags);
    int InitializeInternal(wchar_t* partner, wchar_t* feature, wchar_t* host, bool allowTelemetry, wchar_t* etag);
    ULONG OpenWebRequest(wchar_t* host);
    int ParseJsonResult(char* json);
    ULONG ReadSavedSettingsFromRegistry();
    ULONG RetrieveOneSettingsFromWeb(wchar_t* host);
    ULONG SaveSettingsToRegistry();
    ULONG ShouldReuseOneSettings(bool* shouldReuse);
    ULONG Uninitialize();
    ULONG UpdateRefreshDeadline();
    ULONG WriteSettingsValues(HKEY settingsKey);
};

OneSettingsQuery::OneSettingsQuery() :
    m_Internet(nullptr),
    m_Connection(nullptr),
    m_Request(nullptr),
    m_ETag(nullptr),
    m_Partner(nullptr),
    m_Feature(nullptr),
    m_RefreshIntervalMinutes(0)
{
    m_ValueMap.Initialize(nullptr, 16);
}

OneSettingsQuery::~OneSettingsQuery()
{
    if (m_ETag)
    {
        HeapFree(GetProcessHeap(), 0, m_ETag);
        m_ETag = nullptr;
    }
    
    if (m_Partner)
    {
        HeapFree(GetProcessHeap(), 0, m_Partner);
        m_Partner = nullptr;
    }
    
    if (m_Feature)
    {
        HeapFree(GetProcessHeap(), 0, m_Feature);
        m_Feature = nullptr;
    }

    m_ValueMap.Free();
    Uninitialize();
}

int OneSettingsQuery::CreateHeaders(wchar_t* param1, wchar_t** param2)
{
    wchar_t* etagHeader = nullptr;
    *param2 = nullptr;

    ULONG result = GetETagHeaderFromRegistry(&etagHeader);
    if (result != ERROR_SUCCESS || !etagHeader)
        return result;

    size_t etagLength;
    if (FAILED(StringCchLengthW(etagHeader, STRSAFE_MAX_CCH, &etagLength)))
    {
        HeapFree(GetProcessHeap(), 0, etagHeader);
        return E_INVALIDARG;
    }

    size_t headerLength = etagLength + 32; // Space for "If-None-Match: " and CRLF
    *param2 = (wchar_t*)HeapAlloc(GetProcessHeap(), 0, headerLength * sizeof(wchar_t));
    if (!*param2)
    {
        HeapFree(GetProcessHeap(), 0, etagHeader);
        return E_OUTOFMEMORY;
    }

    if (FAILED(StringCchPrintfW(*param2, headerLength, L"If-None-Match: %s\r\n", etagHeader)))
    {
        HeapFree(GetProcessHeap(), 0, *param2);
        *param2 = nullptr;
        HeapFree(GetProcessHeap(), 0, etagHeader);
        return E_FAIL;
    }

    HeapFree(GetProcessHeap(), 0, etagHeader);
    return S_OK;
}

int OneSettingsQuery::CreateQueryString(wchar_t* buffer, ULONG size)
{
    if (!buffer || size < 1)
        return E_INVALIDARG;

    wchar_t buildLab[512] = {0};
    DWORD buildLabSize = sizeof(buildLab);
    
    if (RegGetValueW(HKEY_LOCAL_MACHINE, 
        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 
        L"BuildLabEx", 
        RRF_RT_REG_SZ, 
        nullptr, 
        buildLab, 
        &buildLabSize) == ERROR_SUCCESS)
    {
        return StringCchPrintfW(buffer, size, 
            L"settings/v2.0/%s/%s?os=Windows&osVer=%s", 
            m_Partner, m_Feature, buildLab);
    }
    
    return StringCchPrintfW(buffer, size, L"settings/v2.0/%s/%s", m_Partner, m_Feature);
}

ULONG OneSettingsQuery::GetETagHeaderFromRegistry(wchar_t** param1)
{
    wchar_t keyPath[MAX_PATH];
    *param1 = nullptr;

    if (!m_Partner || !m_Feature)
        return E_INVALIDARG;

    if (FAILED(StringCchPrintfW(keyPath, _countof(keyPath),
        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\OneSettings\\%s\\%s",
        m_Partner, m_Feature)))
    {
        return E_INVALIDARG;
    }

    DWORD etagSize = 0;
    ULONG result = RegGetValueW(HKEY_LOCAL_MACHINE, keyPath, L"ETag",
        RRF_RT_REG_SZ, nullptr, nullptr, &etagSize);

    if (result != ERROR_SUCCESS)
        return result;

    *param1 = (wchar_t*)HeapAlloc(GetProcessHeap(), 0, etagSize);
    if (!*param1)
        return E_OUTOFMEMORY;

    result = RegGetValueW(HKEY_LOCAL_MACHINE, keyPath, L"ETag",
        RRF_RT_REG_SZ, nullptr, *param1, &etagSize);

    if (result != ERROR_SUCCESS)
    {
        HeapFree(GetProcessHeap(), 0, *param1);
        *param1 = nullptr;
    }

    return result;
}

int OneSettingsQuery::GetSetting(wchar_t* name, wchar_t* value, ULONG size) {
    if (!name || !value || size == 0)
        return E_INVALIDARG;

    *value = L'\0';

    for (ULONG i = 0; i < m_ValueMap.Array.GetCount(); i++) {
        RtlNameValueArray::NameValuePair* pair = m_ValueMap.Array.GetPtr(i);
        if (pair && _wcsicmp(pair->Name, name) == 0) {
            return StringCchCopyW(value, size, pair->Value);
        }
    }
    return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
}

RtlNameValueArray* OneSettingsQuery::GetValueMap() {
    RtlNameValueArray* newMap = new RtlNameValueArray();
    if (!newMap)
        return nullptr;

    newMap->Initialize(nullptr, 16); // Use correct Initialize parameters

    for (ULONG i = 0; i < m_ValueMap.Array.GetCount(); i++) {
        RtlNameValueArray::NameValuePair* srcPair = m_ValueMap.Array.GetPtr(i);
        if (srcPair) {
            int result = newMap->Insert(nullptr, srcPair->Name, srcPair->Value, i);
            if (FAILED(result)) {
                delete newMap;
                return nullptr;
            }
        }
    }
    return newMap;
}


ULONG OneSettingsQuery::Initialize(wchar_t* partner, wchar_t* feature, wchar_t* host, ULONG flags)
{
    if (!partner || !feature || !host)
        return E_INVALIDARG;

    m_Partner = _wcsdup(partner);
    m_Feature = _wcsdup(feature);

    if (!m_Partner || !m_Feature)
    {
        if (m_Partner) { free(m_Partner); m_Partner = nullptr; }
        if (m_Feature) { free(m_Feature); m_Feature = nullptr; }
        return E_OUTOFMEMORY;
    }

    bool allowTelemetry = (flags & 1) != 0; // Simplified check
    return InitializeInternal(partner, feature, host, allowTelemetry, nullptr);
}

int OneSettingsQuery::InitializeInternal(wchar_t* partner, wchar_t* feature, wchar_t* host, 
                                       bool allowTelemetry, wchar_t* etag)
{
    bool shouldReuse = false;
    int result = 0;

    if (allowTelemetry)
    {
        result = ReadSavedSettingsFromRegistry();
    }
    else
    {
        result = ShouldReuseOneSettings(&shouldReuse);
        if (SUCCEEDED(result)) 
        {
            if (!shouldReuse)
            {
                result = OpenWebRequest(host);
                if (SUCCEEDED(result))
                {
                    result = RetrieveOneSettingsFromWeb(host);
                    if (result == S_OK)
                    {
                        SaveSettingsToRegistry();
                    }
                }
            }
            else
            {
                result = ReadSavedSettingsFromRegistry();
            }
        }
    }

    if (SUCCEEDED(result))
    {
        UpdateRefreshDeadline();
    }

    return result;
}

ULONG OneSettingsQuery::OpenWebRequest(wchar_t* host)
{
    m_Internet = WinHttpOpen(
        L"OneSettingsQuery/1.0", 
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME, 
        WINHTTP_NO_PROXY_BYPASS, 
        WINHTTP_FLAG_ASYNC);
    
    if (!m_Internet)
        return GetLastError();

    m_Connection = WinHttpConnect(
        m_Internet, 
        host, 
        INTERNET_DEFAULT_HTTPS_PORT, 
        0);
    
    if (!m_Connection)
    {
        DWORD error = GetLastError();
        WinHttpCloseHandle(m_Internet);
        m_Internet = nullptr;
        return error;
    }

    wchar_t queryPath[512];
    if (FAILED(CreateQueryString(queryPath, _countof(queryPath))))
    {
        WinHttpCloseHandle(m_Connection);
        WinHttpCloseHandle(m_Internet);
        m_Connection = nullptr;
        m_Internet = nullptr;
        return E_FAIL;
    }

    m_Request = WinHttpOpenRequest(
        m_Connection,
        L"GET",
        queryPath,
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE);
    
    if (!m_Request)
    {
        DWORD error = GetLastError();
        WinHttpCloseHandle(m_Connection);
        WinHttpCloseHandle(m_Internet);
        m_Connection = nullptr;
        m_Internet = nullptr;
        return error;
    }

    wchar_t* headers = nullptr;
    if (SUCCEEDED(CreateHeaders(nullptr, &headers)))
    {
        if (headers)
        {
            WinHttpAddRequestHeaders(m_Request, headers, -1, WINHTTP_ADDREQ_FLAG_ADD);
            HeapFree(GetProcessHeap(), 0, headers);
        }
    }

    return ERROR_SUCCESS;
}

int OneSettingsQuery::ParseJsonResult(char* json)
{
    // Simplified JSON parsing implementation
    // In a real implementation, you would use a proper JSON parser
    if (!json)
        return E_INVALIDARG;

    m_ValueMap.Free();
    m_ValueMap.Initialize(nullptr, 16);

    // This is a placeholder for actual JSON parsing logic
    // The real implementation would parse the JSON and populate m_ValueMap
    // with the settings values

    return S_OK;
}

ULONG OneSettingsQuery::ReadSavedSettingsFromRegistry()
{
    wchar_t keyPath[MAX_PATH];
    HKEY hKey = nullptr;
    ULONG result = StringCchPrintfW(
        keyPath, 
        _countof(keyPath), 
        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\OneSettings\\%s\\%s", 
        m_Partner, 
        m_Feature);

    if (FAILED(result))
        return result;

    DWORD refreshInterval = 0;
    DWORD refreshSize = sizeof(refreshInterval);
    result = RegGetValueW(
        HKEY_LOCAL_MACHINE,
        keyPath,
        L"RefreshInterval",
        RRF_RT_REG_DWORD,
        nullptr,
        &refreshInterval,
        &refreshSize);

    if (result == ERROR_SUCCESS)
    {
        m_RefreshIntervalMinutes = refreshInterval;
    }

    result = RegOpenKeyExW(
        HKEY_LOCAL_MACHINE,
        keyPath,
        0,
        KEY_READ,
        &hKey);

    if (result == ERROR_SUCCESS)
    {
        m_ValueMap.Free();
        m_ValueMap.Initialize(nullptr, 32);

        wchar_t valueName[MAX_PATH];
        wchar_t valueData[MAX_PATH];
        DWORD valueNameSize, valueDataSize, type;
        ULONG index = 0;

        while (true)
        {
            valueNameSize = _countof(valueName);
            valueDataSize = sizeof(valueData);
            result = RegEnumValueW(
                hKey,
                index,
                valueName,
                &valueNameSize,
                nullptr,
                &type,
                (LPBYTE)valueData,
                &valueDataSize);

            if (result != ERROR_SUCCESS)
                break;

            if (type == REG_SZ)
            {
                m_ValueMap.Insert(nullptr, valueName, valueData, m_ValueMap.Array.GetCount());
            }

            index++;
        }
        RegCloseKey(hKey);
    }

    return result;
}

ULONG OneSettingsQuery::RetrieveOneSettingsFromWeb(wchar_t* host)
{
    if (!m_Request)
        return E_HANDLE;

    if (!WinHttpSendRequest(m_Request, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                           WINHTTP_NO_REQUEST_DATA, 0, 0, 0))
    {
        return GetLastError();
    }

    if (!WinHttpReceiveResponse(m_Request, nullptr))
    {
        return GetLastError();
    }

    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);
    if (!WinHttpQueryHeaders(m_Request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                            WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusCodeSize, 0))
    {
        return GetLastError();
    }

    if (statusCode == HTTP_STATUS_NOT_MODIFIED)
    {
        return S_FALSE;
    }

    if (statusCode != HTTP_STATUS_OK)
    {
        return HRESULT_FROM_WIN32(statusCode);
    }

    DWORD etagSize = 0;
    WinHttpQueryHeaders(m_Request, WINHTTP_QUERY_ETAG, WINHTTP_HEADER_NAME_BY_INDEX,
                       nullptr, &etagSize, 0);

    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER && etagSize > 0)
    {
        if (m_ETag)
        {
            HeapFree(GetProcessHeap(), 0, m_ETag);
            m_ETag = nullptr;
        }

        m_ETag = (wchar_t*)HeapAlloc(GetProcessHeap(), 0, etagSize);
        if (m_ETag)
        {
            if (!WinHttpQueryHeaders(m_Request, WINHTTP_QUERY_ETAG, WINHTTP_HEADER_NAME_BY_INDEX,
                                   m_ETag, &etagSize, 0))
            {
                HeapFree(GetProcessHeap(), 0, m_ETag);
                m_ETag = nullptr;
            }
        }
    }

    DWORD contentSize = 0;
    if (!WinHttpQueryDataAvailable(m_Request, &contentSize))
    {
        return GetLastError();
    }

    char* content = (char*)HeapAlloc(GetProcessHeap(), 0, contentSize + 1);
    if (!content)
    {
        return E_OUTOFMEMORY;
    }

    DWORD bytesRead = 0;
    if (!WinHttpReadData(m_Request, content, contentSize, &bytesRead))
    {
        HeapFree(GetProcessHeap(), 0, content);
        return GetLastError();
    }

    content[bytesRead] = '\0';
    int parseResult = ParseJsonResult(content);
    HeapFree(GetProcessHeap(), 0, content);

    return parseResult;
}

ULONG OneSettingsQuery::SaveSettingsToRegistry()
{
    HKEY hRoot = nullptr;
    HKEY hPartner = nullptr;
    HKEY hFeature = nullptr;
    HKEY hSettings = nullptr;

    ULONG result = RegCreateKeyExW(
        HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\OneSettings",
        0,
        nullptr,
        REG_OPTION_NON_VOLATILE,
        KEY_WRITE,
        nullptr,
        &hRoot,
        nullptr);

    if (result != ERROR_SUCCESS)
        return result;

    result = RegCreateKeyExW(
        hRoot,
        m_Partner,
        0,
        nullptr,
        REG_OPTION_NON_VOLATILE,
        KEY_WRITE,
        nullptr,
        &hPartner,
        nullptr);

    if (result != ERROR_SUCCESS)
        goto cleanup;

    result = RegCreateKeyExW(
        hPartner,
        m_Feature,
        0,
        nullptr,
        REG_OPTION_NON_VOLATILE,
        KEY_WRITE,
        nullptr,
        &hFeature,
        nullptr);

    if (result != ERROR_SUCCESS)
        goto cleanup;

    if (m_ETag)
    {
        result = RegSetValueExW(
            hFeature,
            L"ETag",
            0,
            REG_SZ,
            (const BYTE*)m_ETag,
            (wcslen(m_ETag) + 1) * sizeof(wchar_t));
    }

    if (result == ERROR_SUCCESS)
    {
        result = RegSetValueExW(
            hFeature,
            L"RefreshInterval",
            0,
            REG_DWORD,
            (const BYTE*)&m_RefreshIntervalMinutes,
            sizeof(m_RefreshIntervalMinutes));
    }

    if (result == ERROR_SUCCESS)
    {
        SHDeleteKeyW(hFeature, L"Settings");
        result = RegCreateKeyExW(
            hFeature,
            L"Settings",
            0,
            nullptr,
            REG_OPTION_NON_VOLATILE,
            KEY_WRITE,
            nullptr,
            &hSettings,
            nullptr);

        if (result == ERROR_SUCCESS)
        {
            result = WriteSettingsValues(hSettings);
        }
    }

cleanup:
    if (hSettings) RegCloseKey(hSettings);
    if (hFeature) RegCloseKey(hFeature);
    if (hPartner) RegCloseKey(hPartner);
    if (hRoot) RegCloseKey(hRoot);

    return result;
}

ULONG OneSettingsQuery::ShouldReuseOneSettings(bool* shouldReuse)
{
    if (!shouldReuse)
        return E_INVALIDARG;

    *shouldReuse = false;

    wchar_t keyPath[MAX_PATH];
    if (FAILED(StringCchPrintfW(keyPath, _countof(keyPath),
        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\OneSettings\\%s\\%s",
        m_Partner, m_Feature)))
    {
        return E_INVALIDARG;
    }

    FILETIME refreshAfter = {0};
    DWORD refreshSize = sizeof(refreshAfter);
    ULONG result = RegGetValueW(
        HKEY_LOCAL_MACHINE,
        keyPath,
        L"RefreshAfter",
        RRF_RT_REG_BINARY,
        nullptr,
        &refreshAfter,
        &refreshSize);

    if (result == ERROR_SUCCESS)
    {
        FILETIME currentTime;
        GetSystemTimeAsFileTime(&currentTime);

        if (CompareFileTime(&currentTime, &refreshAfter) < 0)
        {
            *shouldReuse = true;
        }
    }

    return result;
}

ULONG OneSettingsQuery::Uninitialize()
{
    if (m_Request)
    {
        WinHttpCloseHandle(m_Request);
        m_Request = nullptr;
    }
    if (m_Connection)
    {
        WinHttpCloseHandle(m_Connection);
        m_Connection = nullptr;
    }
    if (m_Internet)
    {
        WinHttpCloseHandle(m_Internet);
        m_Internet = nullptr;
    }
    return ERROR_SUCCESS;
}

ULONG OneSettingsQuery::UpdateRefreshDeadline()
{
    if (m_RefreshIntervalMinutes == 0)
        return S_FALSE;

    FILETIME currentTime;
    GetSystemTimeAsFileTime(&currentTime);

    ULARGE_INTEGER interval;
    interval.QuadPart = (ULONGLONG)m_RefreshIntervalMinutes * 60 * 10000000;

    ULARGE_INTEGER current;
    current.LowPart = currentTime.dwLowDateTime;
    current.HighPart = currentTime.dwHighDateTime;

    ULARGE_INTEGER refreshAfter;
    refreshAfter.QuadPart = current.QuadPart + interval.QuadPart;

    FILETIME newRefresh;
    newRefresh.dwLowDateTime = refreshAfter.LowPart;
    newRefresh.dwHighDateTime = refreshAfter.HighPart;

    wchar_t keyPath[MAX_PATH];
    if (FAILED(StringCchPrintfW(keyPath, _countof(keyPath),
        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\OneSettings\\%s\\%s",
        m_Partner, m_Feature)))
    {
        return E_INVALIDARG;
    }

    HKEY hKey = nullptr;
    ULONG result = RegOpenKeyExW(
        HKEY_LOCAL_MACHINE,
        keyPath,
        0,
        KEY_WRITE,
        &hKey);

    if (result == ERROR_SUCCESS)
    {
        result = RegSetValueExW(
            hKey,
            L"RefreshAfter",
            0,
            REG_BINARY,
            (const BYTE*)&newRefresh,
            sizeof(newRefresh));

        RegCloseKey(hKey);
    }

    return result;
}

ULONG OneSettingsQuery::WriteSettingsValues(HKEY settingsKey) {
    if (!settingsKey)
        return E_INVALIDARG;

    for (ULONG i = 0; i < m_ValueMap.Array.GetCount(); i++) {
        RtlNameValueArray::NameValuePair* pair = m_ValueMap.Array.GetPtr(i);
        if (pair) {
            ULONG result = RegSetValueExW(
                settingsKey,
                pair->Name,
                0,
                REG_SZ,
                (const BYTE*)pair->Value,
                (wcslen(pair->Value) + 1) * sizeof(wchar_t));
            if (result != ERROR_SUCCESS)
                return result;
        }
    }
    return ERROR_SUCCESS;
}

// Helper function implementations
void CleanupJsonObject(RtlArray<IProperty*>** object)  // Add the template parameter
{
    if (*object)
    {
        (*object)->Free();
        delete *object;
        *object = nullptr;
    }
}

int TelIsTelemetryTypeAllowed(ULONG type)
{
    // Default implementation - in real code this would check telemetry permissions
    return 1;
}

