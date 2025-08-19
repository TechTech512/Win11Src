#include <windows.h>
#include <winhttp.h>
#include <setupapi.h>
#include <strsafe.h>
#include <evntprov.h>
#include <shlwapi.h>
#include <traceloggingprovider.h>
#include <traceloggingcorrelationvector.hpp>
#include <guiddef.h>
#include <appmodel.h>
#include <vector>
#include <string>

// Type definitions to replace decompiler types
typedef unsigned char uchar;
typedef unsigned long ulong;

// Forward declarations
int __cdecl DownloadCabFile(wchar_t* url, wchar_t* outputPath);
int __cdecl ExtractCabFiles(wchar_t* cabPath, wchar_t* targetPath);
int __cdecl UpdateRegValue(wchar_t* value);
namespace Windows {
    namespace Compat {
        namespace Appraiser {
            namespace WicaFactory {
                extern unsigned int TestingFlags;
                extern class TraceLoggingCorrelationVector* CorrelationVector;
                void RunPrintf(int line, const char* file, const char* function, const char* format, ...);
                void CensusId();
            }

            namespace Utilities {
                bool IsMachineOptedInToCoreTelemetry();
                int ReadRegistryValue(HKEY rootKey, const wchar_t* keyPath, DWORD valueType, void* outValue, DWORD bufferSize);
                int WriteRegistryValue(HKEY rootKey, const wchar_t* keyPath, DWORD valueType, const void* data, DWORD dataSize);
                int ExpandDirectory(const wchar_t* path, wchar_t* expandedPath, DWORD bufferSize);
                int DeleteFilesInFolder(const wchar_t* folderPath);
                int EnsureDirectoryExists(const wchar_t* path);
                int VerifySignatureRecursive(const wchar_t* filePath);
                int SendRequest(const wchar_t* url, wchar_t* outResponse, DWORD bufferSize);
                int ResolveForwardLink(const wchar_t* linkUrl, wchar_t* resolvedUrl, DWORD bufferSize);
                wchar_t* GetOneSettingsServer();
            }

            namespace XmlAppraiser {
                int RunXml(wchar_t* xmlFilePath, wchar_t* parameters);
            }

            void WriteLog(bool isError, int lineNumber, const char* filePath, const char* functionName, int errorCode, const char* message);
        }
    }
}

bool IsOneSettingsOverridingWuCheck();

// Windows API imports
extern "C" {
    void WINAPI McGenEventRegister(LPCGUID ProviderId, PENABLECALLBACK EnableCallback, PVOID CallbackContext, PREGHANDLE RegHandle);
    void WINAPI McGenEventUnregister(REGHANDLE RegHandle);
}

namespace Windows {
    namespace Compat {
        namespace Appraiser {
            namespace GatedScenario {

                // Cabinet extraction callback
                UINT __stdcall CabinetCallback(PVOID Context, UINT Notification, UINT Param1, UINT Param2)
                {
                    if (Notification == 0x11) { // FILE_IN_CABINET notification
                        wchar_t* targetPath = reinterpret_cast<wchar_t*>(Context);
                        wchar_t* sourceFile = reinterpret_cast<wchar_t*>(Param1);
                        wchar_t* destFile = reinterpret_cast<wchar_t*>(Param2);

                        HRESULT hr = StringCchCopyW(targetPath, MAX_PATH, destFile);
                        if (FAILED(hr)) {
                            WriteLog(true, 0x140, 
                                "onecore\\base\\appcompat\\appraiser\\gated\\lib\\gatedscenario.cpp",
                                "Windows::Compat::Appraiser::GatedScenario::CabinetCallback",
                                hr,
                                "Failed to copy target path");
                            return 0xA1;
                        }

                        hr = StringCchCatW(targetPath, MAX_PATH, sourceFile);
                        if (FAILED(hr)) {
                            WriteLog(true, 0x14C, 
                                "onecore\\base\\appcompat\\appraiser\\gated\\lib\\gatedscenario.cpp",
                                "Windows::Compat::Appraiser::GatedScenario::CabinetCallback",
                                hr,
                                "Failed to concatenate string");
                            return 0xA1;
                        }

                        return 1; // Continue extraction
                    }
                    return 0; // Unhandled notification
                }

                // Compare two registry links
                int CompareLinks(bool* result, const wchar_t* link1, const wchar_t* link2)
                {
                    wchar_t value1[MAX_PATH] = {0};
                    wchar_t value2[MAX_PATH] = {0};
                    
                    int retVal = Utilities::ReadRegistryValue(
                        HKEY_LOCAL_MACHINE,
                        link1,
                        REG_SZ,
                        value1,
                        sizeof(value1));
                    
                    if (retVal == 1) {
                        *result = true;
                        return 0;
                    }
                    else if (retVal < 0) {
                        WriteLog(true, 0xD4,
                            "onecore\\base\\appcompat\\appraiser\\gated\\lib\\gatedscenario.cpp",
                            "Windows::Compat::Appraiser::GatedScenario::CompareLinks",
                            retVal,
                            "Failed to read registry value");
                        return retVal;
                    }

                    retVal = Utilities::ReadRegistryValue(
                        HKEY_LOCAL_MACHINE,
                        link2,
                        REG_SZ,
                        value2,
                        sizeof(value2));
                    
                    if (retVal < 0) {
                        return retVal;
                    }

                    // Compare the links lexicographically
                    *result = (wcscmp(value1, value2) != 0);
                    return 0;
                }

             // Add this function declaration before its usage (around line 430)
             int __cdecl DownloadAndExtractCab(wchar_t* url, wchar_t* targetDir, wchar_t* tempPath)
             {
                int result = 0;
                const char* functionName = "Windows::Compat::Appraiser::GatedScenario::DownloadAndExtractCab";
                const char* fileName = "onecore\\base\\appcompat\\appraiser\\gated\\lib\\gatedscenario.cpp";

                // Ensure target directory exists
                result = Utilities::EnsureDirectoryExists(targetDir);
                if (result < 0) {
                    if ((WicaFactory::TestingFlags & 1) != 0) {
                       WicaFactory::RunPrintf(0xED, fileName, functionName, "Failed to check/create directory: [0x%x].", result);
                    }
                    WriteLog(true, 0xED, fileName, functionName, result, "Failed to check/create directory");
                    return result;
                }

                // Delete existing files in the folder
                result = Utilities::DeleteFilesInFolder(targetDir);
                if (result < 0) {
                    if ((WicaFactory::TestingFlags & 1) != 0) {
                       WicaFactory::RunPrintf(0xF0, fileName, functionName, "Failed to clear cab folder: [0x%x].", result);
                    }
                    WriteLog(true, 0xF0, fileName, functionName, result, "Failed to clear cab folder");
                    return result;
                }

                 // Download the CAB file
                result = DownloadCabFile(url, tempPath);
                if (result < 0) {
                    if ((WicaFactory::TestingFlags & 1) != 0) {
                       WicaFactory::RunPrintf(0xF5, fileName, functionName, "Failed to download cab file: [0x%x].", result);
                    }
                    WriteLog(true, 0xF5, fileName, functionName, result, "Failed to download cab file");
                    return result;
                }

                 // Verify signature of the downloaded file
                result = Utilities::VerifySignatureRecursive(tempPath);
                if (result < 0) {
                    if ((WicaFactory::TestingFlags & 1) != 0) {
                        WicaFactory::RunPrintf(0xF9, fileName, functionName, "Failed verify cab signature: [0x%x].", result);
                    }
                    WriteLog(true, 0xF9, fileName, functionName, result, "Failed verify cab signature");
                    return result;
                }

                    // Extract files from CAB
                    result = ExtractCabFiles(tempPath, targetDir);
                    if (result < 0) {
                       if ((WicaFactory::TestingFlags & 1) != 0) {
                           WicaFactory::RunPrintf(0xFC, fileName, functionName, "Failed to extract cab file: [0x%x].", result);
                       }
                       WriteLog(true, 0xFC, fileName, functionName, result, "Failed to extract cab file");
                       return result;
                    }

                    return 0; // Success
                }

                // Download CAB file implementation
                int __cdecl DownloadCabFile(wchar_t* url, wchar_t* outputPath)
                {
                    HINTERNET hSession = nullptr;
                    HINTERNET hConnect = nullptr;
                    HINTERNET hRequest = nullptr;
                    HANDLE hFile = INVALID_HANDLE_VALUE;
                    int result = 0;
                    wchar_t responseBuffer[1] = {0}; // Dummy buffer since we're not using the response

                    // Try with default proxy settings first
                    result = Utilities::SendRequest(url, responseBuffer, sizeof(responseBuffer));
                    if (FAILED(result)) {
                        WriteLog(true, 0x17B,
                            "onecore\\base\\appcompat\\appraiser\\gated\\lib\\gatedscenario.cpp",
                            "Windows::Compat::Appraiser::GatedScenario::DownloadCabFile",
                            result,
                            "Failed to send request using default proxy");

                        // Fall back to auto-proxy
                        result = Utilities::SendRequest(url, responseBuffer, sizeof(responseBuffer));
                        if (FAILED(result)) {
                            WriteLog(true, 0x181,
                                "onecore\\base\\appcompat\\appraiser\\gated\\lib\\gatedscenario.cpp",
                                "Windows::Compat::Appraiser::GatedScenario::DownloadCabFile",
                                result,
                                "Failed to send request using auto proxy");
                            return result;
                        }
                    }

                    // Create output file
                    hFile = CreateFileW(outputPath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
                    if (hFile == INVALID_HANDLE_VALUE) {
                        DWORD err = GetLastError();
                        WriteLog(true, 0x195,
                            "onecore\\base\\appcompat\\appraiser\\gated\\lib\\gatedscenario.cpp",
                            "Windows::Compat::Appraiser::GatedScenario::DownloadCabFile",
                            err,
                            "Failed to create file");
                        return HRESULT_FROM_WIN32(err);
                    }

                    // Download data
                    DWORD bytesAvailable = 0;
                    if (!WinHttpQueryDataAvailable(hRequest, &bytesAvailable)) {
                        DWORD err = GetLastError();
                        WriteLog(true, 0x19E,
                            "onecore\\base\\appcompat\\appraiser\\gated\\lib\\gatedscenario.cpp",
                            "Windows::Compat::Appraiser::GatedScenario::DownloadCabFile",
                            err,
                            "Failed to verify if data is available");
                        CloseHandle(hFile);
                        return HRESULT_FROM_WIN32(err);
                    }

                    // Allocate buffer
                    const DWORD bufferSize = 8192; // 8KB
                    BYTE* buffer = (BYTE*)HeapAlloc(GetProcessHeap(), 0, bufferSize);
                    if (!buffer) {
                        CloseHandle(hFile);
                        return E_OUTOFMEMORY;
                    }

                    // Download loop
                    DWORD bytesRead = 0;
                    DWORD bytesWritten = 0;
                    do {
                        if (!WinHttpReadData(hRequest, buffer, bufferSize, &bytesRead)) {
                            DWORD err = GetLastError();
                            WriteLog(true, 0x1B1,
                                "onecore\\base\\appcompat\\appraiser\\gated\\lib\\gatedscenario.cpp",
                                "Windows::Compat::Appraiser::GatedScenario::DownloadCabFile",
                                err,
                                "Failed to read data");
                            result = HRESULT_FROM_WIN32(err);
                            break;
                        }

                        if (bytesRead > 0) {
                            if (!WriteFile(hFile, buffer, bytesRead, &bytesWritten, nullptr) || bytesWritten != bytesRead) {
                                DWORD err = GetLastError();
                                WriteLog(true, 0x1BC,
                                    "onecore\\base\\appcompat\\appraiser\\gated\\lib\\gatedscenario.cpp",
                                    "Windows::Compat::Appraiser::GatedScenario::DownloadCabFile",
                                    err,
                                    "Failed to write file");
                                result = HRESULT_FROM_WIN32(err);
                                break;
                            }
                        }
                    } while (bytesRead > 0);

                    // Cleanup
                    HeapFree(GetProcessHeap(), 0, buffer);
                    CloseHandle(hFile);
                    if (hRequest) WinHttpCloseHandle(hRequest);
                    if (hConnect) WinHttpCloseHandle(hConnect);
                    if (hSession) WinHttpCloseHandle(hSession);

                    return result;
                }

                // Extract files from CAB
                int __cdecl ExtractCabFiles(wchar_t* cabPath, wchar_t* targetPath)
                {
                    if (!PathFileExistsW(cabPath)) {
                        WriteLog(true, 0x112,
                            "onecore\\base\\appcompat\\appraiser\\gated\\lib\\gatedscenario.cpp",
                            "Windows::Compat::Appraiser::GatedScenario::ExtractCabFiles",
                            ERROR_FILE_NOT_FOUND,
                            "Failed to find file");
                        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
                    }

                    if (!SetupIterateCabinetW(cabPath, 0, CabinetCallback, targetPath)) {
                        DWORD err = GetLastError();
                        WriteLog(true, 0x11A,
                            "onecore\\base\\appcompat\\appraiser\\gated\\lib\\gatedscenario.cpp",
                            "Windows::Compat::Appraiser::GatedScenario::ExtractCabFiles",
                            err,
                            "Failed to extract cab");
                        return HRESULT_FROM_WIN32(err);
                    }

                    return S_OK;
                }

                // Check if scenario is applicable
                bool __cdecl IsApplicable()
                {
                    // Check if OneSettings is overriding WU check
                    if (IsOneSettingsOverridingWuCheck()) {
                        return true;
                    }

                    // Check Windows Update registry settings
                    DWORD auOptions = 0;
                    int hr = Utilities::ReadRegistryValue(
                        HKEY_LOCAL_MACHINE,  // Changed from 0x10 to proper HKEY
                        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate\\Auto Update",
                        REG_DWORD,  // Changed from 0x1000F750 to proper REG_DWORD
                        &auOptions,
                        sizeof(auOptions));

                    if (FAILED(hr)) {
                        WriteLog(true, 0x215,
                            "onecore\\base\\appcompat\\appraiser\\gated\\lib\\gatedscenario.cpp",
                            "Windows::Compat::Appraiser::GatedScenario::IsApplicable",
                            hr,
                            "Error reading registry value");
                        return false;
                    }

                    // Check GWX.exe existence
                    wchar_t gwxPath[MAX_PATH] = {0};
                    hr = Utilities::ExpandDirectory(L"%WINDIR%\\system32\\GWX\\GWX.exe", gwxPath, MAX_PATH);
                    if (FAILED(hr)) {
                        WriteLog(true, 0x218,
                            "onecore\\base\\appcompat\\appraiser\\gated\\lib\\gatedscenario.cpp",
                            "Windows::Compat::Appraiser::GatedScenario::IsApplicable",
                            hr,
                            "Error expanding GWX location");
                        return false;
                    }

                    return PathFileExistsW(gwxPath) == TRUE;
                }

                // OneSettings query helper class
                class OneSettingsQuery {
                public:
                    OneSettingsQuery() {}
                    ~OneSettingsQuery() {}

                    int Initialize(const wchar_t* server, const wchar_t* nameSpace, const wchar_t* scope, DWORD flags) {
                        return S_OK;
                    }

                    int GetSetting(const wchar_t* name, wchar_t* outValue, DWORD bufferSize) {
                        return S_OK;
                    }

                    void Uninitialize() {
                    }
                };

                // Check if OneSettings is overriding Windows Update check
                bool __cdecl IsOneSettingsOverridingWuCheck()
                {
                    wchar_t settingValue[32] = {0};
                    wchar_t* server = Utilities::GetOneSettingsServer();

                    // Query OneSettings
                    OneSettingsQuery query;
                    int hr = query.Initialize(server, L"", L"", 0);
                    if (SUCCEEDED(hr)) {
                        hr = query.GetSetting(L"SKIPWUCHECK", settingValue, 32);
                        if (SUCCEEDED(hr)) {
                            return _wcsicmp(settingValue, L"TRUE") == 0;
                        }
                    }

                    return false;
                }
                // Main gated scenario execution
                void __cdecl RunGatedScenario()
                {
                    // Register providers
                    McGenEventRegister(nullptr, nullptr, nullptr, nullptr);

                    if (Utilities::IsMachineOptedInToCoreTelemetry()) {
                        TraceLoggingRegister(nullptr);
                        TraceLoggingRegister(nullptr);
                        WriteLog(false, 0x2D,
                            "onecore\\base\\appcompat\\appraiser\\gated\\lib\\gatedscenario.cpp",
                            "Windows::Compat::Appraiser::GatedScenario::RunGatedScenario",
                            0,
                            "Registered Appraiser UTC providers");
                    }

                    // Check registry for override
                    DWORD overrideValue = 0;
                    int hr = Utilities::ReadRegistryValue(
                        HKEY_LOCAL_MACHINE,  // Changed from 0x2 to proper HKEY
                        L"Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Appraiser",
                        REG_DWORD,  // Changed from 0x1000F15C to proper REG_DWORD
                        &overrideValue,
                        sizeof(overrideValue));

                    bool shouldRun = IsApplicable();
                    if (!shouldRun && hr != 0) {
                        WriteLog(false, __LINE__,
                            "onecore\\base\\appcompat\\appraiser\\gated\\lib\\gatedscenario.cpp",
                            "Windows::Compat::Appraiser::GatedScenario::RunGatedScenario",
                            0,
                            "Scenario not applicable");
                        goto cleanup;
                    }

                    // Expand paths
                    wchar_t cabPath[MAX_PATH] = {0};
                    hr = Utilities::ExpandDirectory(
                        L"%WINDIR%\\appcompat\\appraiser\\Gated\\DailyGatedCheck.cab",
                        cabPath,
                        MAX_PATH);
                    if (FAILED(hr)) {
                        WriteLog(true, __LINE__,
                            "onecore\\base\\appcompat\\appraiser\\gated\\lib\\gatedscenario.cpp",
                            "Windows::Compat::Appraiser::GatedScenario::RunGatedScenario",
                            hr,
                            "Error expanding cab path");
                        goto cleanup;
                    }

                    wchar_t workingDir[MAX_PATH] = {0};
                    hr = Utilities::ExpandDirectory(
                        L"%WINDIR%\\appcompat\\appraiser\\Gated\\",
                        workingDir,
                        MAX_PATH);
                    if (FAILED(hr)) {
                        WriteLog(true, __LINE__,
                            "onecore\\base\\appcompat\\appraiser\\gated\\lib\\gatedscenario.cpp",
                            "Windows::Compat::Appraiser::GatedScenario::RunGatedScenario",
                            hr,
                            "Error expanding working directory");
                        goto cleanup;
                    }

                    // Check if we need to download new CAB
                    bool downloadNeeded = false;
                    if (hr == 0) {
                        // Check if URL is HTTP
                        if (wcsnicmp(cabPath, L"http", 4) == 0) {
                            wchar_t resolvedUrl[MAX_PATH] = {0};
                            hr = Utilities::ResolveForwardLink(
                                L"http://go.microsoft.com/fwlink/?LinkID=618338",
                                resolvedUrl,
                                MAX_PATH);
                            if (FAILED(hr)) {
                                WriteLog(true, __LINE__,
                                    "onecore\\base\\appcompat\\appraiser\\gated\\lib\\gatedscenario.cpp",
                                    "Windows::Compat::Appraiser::GatedScenario::RunGatedScenario",
                                    hr,
                                    "Failed to resolve forward link");
                                goto cleanup;
                            }

                            // Compare with existing link
                            bool linksDifferent = false;
                            hr = CompareLinks(&linksDifferent, cabPath, resolvedUrl);
                            if (FAILED(hr)) {
                                WriteLog(true, __LINE__,
                                    "onecore\\base\\appcompat\\appraiser\\gated\\lib\\gatedscenario.cpp",
                                    "Windows::Compat::Appraiser::GatedScenario::RunGatedScenario",
                                    hr,
                                    "Failed to compare the links");
                                goto cleanup;
                            }

                            downloadNeeded = linksDifferent;
                        }
                    }

                    // Download and extract if needed
                    if (downloadNeeded || hr != 0) {
                        wchar_t tempPath[MAX_PATH] = {0};
                        GetTempPathW(MAX_PATH, tempPath);
                        PathAppendW(tempPath, L"DailyGatedCheck.cab");

                        hr = DownloadAndExtractCab(cabPath, workingDir, tempPath);
                        if (FAILED(hr)) {
                            WriteLog(true, __LINE__,
                                "onecore\\base\\appcompat\\appraiser\\gated\\lib\\gatedscenario.cpp",
                                "Windows::Compat::Appraiser::GatedScenario::RunGatedScenario",
                                hr,
                                "Failed to download and extract file");
                            goto cleanup;
                        }

                        // Update registry with new URL
                        if (hr == 0) {
                            hr = UpdateRegValue(cabPath);
                            if (FAILED(hr)) {
                                WriteLog(true, __LINE__,
                                    "onecore\\base\\appcompat\\appraiser\\gated\\lib\\gatedscenario.cpp",
                                    "Windows::Compat::Appraiser::GatedScenario::RunGatedScenario",
                                    hr,
                                    "Failed to update registry value");
                                goto cleanup;
                            }
                        }
                    }

                    // Run the gated XML file
                    wchar_t xmlPath[MAX_PATH] = {0};
                    PathCombineW(xmlPath, workingDir, L"gated.xml");

                    if (!PathFileExistsW(xmlPath)) {
                        WriteLog(true, __LINE__,
                            "onecore\\base\\appcompat\\appraiser\\gated\\lib\\gatedscenario.cpp",
                            "Windows::Compat::Appraiser::GatedScenario::RunGatedScenario",
                            ERROR_INVALID_PARAMETER,
                            "XML file not found");
                        goto cleanup;
                    }

                    hr = XmlAppraiser::RunXml(xmlPath, nullptr);
                    if (FAILED(hr)) {
                        WriteLog(true, __LINE__,
                            "onecore\\base\\appcompat\\appraiser\\gated\\lib\\gatedscenario.cpp",
                            "Windows::Compat::Appraiser::GatedScenario::RunGatedScenario",
                            hr,
                            "Failed to run gated xml file");
                    }

                cleanup:
                    // Unregister providers
                    TraceLoggingUnregister(nullptr);
                    TraceLoggingUnregister(nullptr);
                    WriteLog(false, 0xB0,
                        "onecore\\base\\appcompat\\appraiser\\gated\\lib\\gatedscenario.cpp",
                        "Windows::Compat::Appraiser::GatedScenario::RunGatedScenario",
                        0,
                        "Unregistered Appraiser UTC providers");
                    REGHANDLE regHandle = NULL;
                    McGenEventUnregister(regHandle);
                }

                // Update registry value
                int __cdecl UpdateRegValue(wchar_t* value)
                {
                    int hr = Utilities::WriteRegistryValue(
                        HKEY_LOCAL_MACHINE,  // Changed from reinterpret_cast<wchar_t*>(HKEY_LOCAL_MACHINE)
                        L"Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Appraiser",
                        REG_SZ,
                        value,  // Changed from reinterpret_cast<uchar*>(value)
                        (wcslen(value) + 1) * sizeof(wchar_t));

                    if (FAILED(hr)) {
                        WriteLog(true, 0x1ED,
                            "onecore\\base\\appcompat\\appraiser\\gated\\lib\\gatedscenario.cpp",
                            "Windows::Compat::Appraiser::GatedScenario::UpdateRegValue",
                            hr,
                            "Failed to write registry value");
                    }

                    return hr;
                }
            }
        }
    }
}

