// traceoutput.cpp
#include "precomp.h"

namespace WiaTrace {
    DWORD g_tls_idx_ulTraceMask = TLS_OUT_OF_INDEXES;
    DWORD g_tls_idx_ulTraceLevel = TLS_OUT_OF_INDEXES;
    DWORD g_tls_idx_ulIndentLevel = TLS_OUT_OF_INDEXES;
    DWORD g_tls_idx_bActivated = TLS_OUT_OF_INDEXES;
    BOOL g_TraceOutput_bMaxFileSizeReached = FALSE;
	
	void InitThreadData();
    HRESULT GetBanner(DWORD dwBannerType, char* pszBanner, SYSTEMTIME* pst, char* pszPath, DWORD dwParam);
    HRESULT WrapFile(void* hFile);
    HRESULT WriteToFile(void* hFile, void* hMutex, DWORD dwStringLen, DWORD dwParam, char* pszString);
    BOOL IsMaxFileSizeExceeded(void* hFile, DWORD dwMaxSize);
}

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_DETACH) {
        TlsFree(WiaTrace::g_tls_idx_ulTraceMask);
        TlsFree(WiaTrace::g_tls_idx_ulTraceLevel);
        TlsFree(WiaTrace::g_tls_idx_ulIndentLevel);
        TlsFree(WiaTrace::g_tls_idx_bActivated);
        return TRUE;
    }
    
    if (dwReason == DLL_PROCESS_ATTACH) {
        WiaTrace::g_tls_idx_ulTraceMask = TlsAlloc();
        if (WiaTrace::g_tls_idx_ulTraceMask == TLS_OUT_OF_INDEXES) {
            return FALSE;
        }
        
        WiaTrace::g_tls_idx_ulTraceLevel = TlsAlloc();
        if (WiaTrace::g_tls_idx_ulTraceLevel == TLS_OUT_OF_INDEXES) {
            TlsFree(WiaTrace::g_tls_idx_ulTraceMask);
            return FALSE;
        }
        
        WiaTrace::g_tls_idx_ulIndentLevel = TlsAlloc();
        if (WiaTrace::g_tls_idx_ulIndentLevel == TLS_OUT_OF_INDEXES) {
            TlsFree(WiaTrace::g_tls_idx_ulTraceMask);
            TlsFree(WiaTrace::g_tls_idx_ulTraceLevel);
            return FALSE;
        }
        
        WiaTrace::g_tls_idx_bActivated = TlsAlloc();
        if (WiaTrace::g_tls_idx_bActivated == TLS_OUT_OF_INDEXES) {
            TlsFree(WiaTrace::g_tls_idx_ulTraceMask);
            TlsFree(WiaTrace::g_tls_idx_ulTraceLevel);
            TlsFree(WiaTrace::g_tls_idx_ulIndentLevel);
            return FALSE;
        }
    }
    else if (dwReason == DLL_THREAD_ATTACH) {
        WiaTrace::InitThreadData();
    }
    
    return TRUE;
}

DWORD WINAPI WIATRACE_DecrementIndentLevel()
{
    DWORD dwIndentLevel = (DWORD)TlsGetValue(WiaTrace::g_tls_idx_ulIndentLevel);
    if (dwIndentLevel != 0) {
        dwIndentLevel--;
        TlsSetValue(WiaTrace::g_tls_idx_ulIndentLevel, (LPVOID)dwIndentLevel);
    }
    return dwIndentLevel;
}

DWORD WINAPI WIATRACE_GetIndentLevel()
{
    return (DWORD)TlsGetValue(WiaTrace::g_tls_idx_ulIndentLevel);
}

void WINAPI WIATRACE_GetTraceSettings(DWORD* pdwTraceMask, DWORD* pdwTraceLevel, INT* pbActivated)
{
    if (pdwTraceMask != NULL) {
        *pdwTraceMask = (DWORD)TlsGetValue(WiaTrace::g_tls_idx_ulTraceMask);
    }
    if (pdwTraceLevel != NULL) {
        *pdwTraceLevel = (DWORD)TlsGetValue(WiaTrace::g_tls_idx_ulTraceLevel);
    }
    if (pbActivated != NULL) {
        *pbActivated = (INT)TlsGetValue(WiaTrace::g_tls_idx_bActivated);
    }
}

DWORD WINAPI WIATRACE_IncrementIndentLevel()
{
    DWORD dwIndentLevel = (DWORD)TlsGetValue(WiaTrace::g_tls_idx_ulIndentLevel);
    dwIndentLevel++;
    TlsSetValue(WiaTrace::g_tls_idx_ulIndentLevel, (LPVOID)dwIndentLevel);
    return dwIndentLevel;
}

void WINAPI WIATRACE_Init(HINSTANCE hInstance, char* pszModuleName)
{
    return;
}

void WINAPI WIATRACE_Term(HINSTANCE hInstance, char* pszModuleName)
{
    return;
}

void WINAPI WIATRACE_OutputString(INT bWriteToFile, void* hFile, void* hMutex, HINSTANCE hInstance, 
                                 char* pszModuleName, SYSTEMTIME* pst, DWORD dwFlags, DWORD dwMaxFileSize, 
                                 tagWiaTraceData_Type* pTraceData)
{
    char szBuffer1[2048];
    char szBuffer2[2048];
    char szBanner[256];
    DWORD dwBytesWritten;
    char* pszOutputBuffer;
    char* pszFileName;
    char* pszFormat;
    DWORD dwSeverity;
    DWORD dwHighFlags;
    char* pszCurrent;
    char cTemp;
    DWORD dwLength;
    LARGE_INTEGER liFileSize;
    LARGE_INTEGER liNewPosition;
    DWORD dwWaitResult;
    
    if (pTraceData == NULL || pTraceData->m_pszString == NULL) {
        return;
    }
    
    memset(szBuffer1, 0, sizeof(szBuffer1));
    memset(szBuffer2, 0, sizeof(szBuffer2));
    
    dwHighFlags = dwFlags & 0xFFFF0000;
    pszFileName = pTraceData->m_pszFile;
    pszOutputBuffer = szBuffer1;
    
    if (dwHighFlags == 0 || dwHighFlags == 0x01000000) {
        if (dwHighFlags == 0x01000000) {
            pszFileName = NULL;
        }
        
        dwSeverity = pTraceData->m_ulTraceSeverity;
        
        if (dwSeverity == 4 || dwSeverity == 8 || dwSeverity == 0x10) {
            if (pszFileName == NULL) {
                if (pTraceData->m_pszFunction == NULL) {
                    pszFormat = "WIA: %lu.%lu %lu %X %lu [%s] %s\r\n";
                } else {
                    pszFormat = "WIA: %lu.%lu %lu %X %lu [%s] %s, %s\r\n";
                }
            } else {
                if (pTraceData->m_pszFunction == NULL) {
                    pszFormat = "%hs(%lu) : %lu.%lu %lu %X %lu [%s] WIA: %s\r\n";
                } else {
                    pszFormat = "%hs(%lu) : %lu.%lu %lu %X %lu [%s] WIA: %s, %s\r\n";
                }
            }
        } else {
            switch (dwSeverity) {
                case 1:
                case 0x80:
                    if (pszFileName != NULL) {
                        if (pTraceData->m_pszFunction == NULL) {
                            pszFormat = "%hs(%lu) : %lu.%lu %lu %X %lu [%s] WIA: ERROR: %s\r\n";
                        } else {
                            pszFormat = "%hs(%lu) : %lu.%lu %lu %X %lu [%s] WIA: ERROR: %s, %s\r\n";
                        }
                    } else {
                        if (pTraceData->m_pszFunction != NULL) {
                            pszFormat = "WIA: %lu.%lu %lu %X %lu [%s] ERROR: %s, %s\r\n";
                        } else {
                            pszFormat = "WIA: %lu.%lu %lu %X %lu [%s] ERROR: %s\r\n";
                        }
                    }
                    break;
                    
                case 2:
                    if (pszFileName != NULL) {
                        if (pTraceData->m_pszFunction != NULL) {
                            pszFormat = "%hs(%lu) : %lu.%lu %lu %X %lu [%s] WIA: WARNING: %s, %s\r\n";
                        } else {
                            pszFormat = "%hs(%lu) : %lu.%lu %lu %X %lu [%s] WIA: WARNING: %s\r\n";
                        }
                    } else {
                        if (pTraceData->m_pszFunction == NULL) {
                            pszFormat = "WIA: %lu.%lu %lu %X %lu [%s] WARNING: %s\r\n";
                        } else {
                            pszFormat = "WIA: %lu.%lu %lu %X %lu [%s] WARNING: %s, %s\r\n";
                        }
                    }
                    break;
                    
                default:
                    return;
            }
        }
        
        StringCchPrintfA(szBuffer1, sizeof(szBuffer1), pszFormat,
                        pszFileName ? pszFileName : "",
                        pTraceData->m_ulLine,
                        pst->wHour, pst->wMinute,
                        GetCurrentThreadId(),
                        GetCurrentProcessId(),
                        WIATRACE_GetIndentLevel(),
                        pszModuleName,
                        pTraceData->m_pszString,
                        pTraceData->m_pszFunction ? pTraceData->m_pszFunction : "");
    } else {
        pszOutputBuffer = szBuffer2;
        pszFileName = pTraceData->m_pszFile;
        
        if ((dwFlags & 0x2000) == 0) {
            pszOutputBuffer = szBuffer1;
            
            switch (pTraceData->m_ulTraceSeverity) {
                case 1:
                case 0x80:
                    if (pszFileName == NULL) {
                        if (pTraceData->m_pszFunction != NULL) {
                            pszFormat = "WIA: %lu.%lu %lu %X %lu [%s] ERROR: %s, %s\r\n";
                        } else {
                            pszFormat = "WIA: %lu.%lu %lu %X %lu [%s] ERROR: %s\r\n";
                        }
                    } else {
                        if (pTraceData->m_pszFunction != NULL) {
                            pszFormat = "%hs(%lu) : %lu.%lu %lu %X %lu [%s] WIA: ERROR: %s, %s\r\n";
                        } else {
                            pszFormat = "%hs(%lu) : %lu.%lu %lu %X %lu [%s] WIA: ERROR: %s\r\n";
                        }
                    }
                    break;
                    
                case 2:
                    if (pszFileName != NULL) {
                        if (pTraceData->m_pszFunction == NULL) {
                            pszFormat = "%hs(%lu) : %lu.%lu %lu %X %lu [%s] WIA: WARNING: %s\r\n";
                        } else {
                            pszFormat = "%hs(%lu) : %lu.%lu %lu %X %lu [%s] WIA: WARNING: %s, %s\r\n";
                        }
                    } else {
                        if (pTraceData->m_pszFunction == NULL) {
                            pszFormat = "WIA: %lu.%lu %lu %X %lu [%s] WARNING: %s\r\n";
                        } else {
                            pszFormat = "WIA: %lu.%lu %lu %X %lu [%s] WARNING: %s, %s\r\n";
                        }
                    }
                    break;
                    
                case 4:
                case 8:
                case 0x10:
                    if (pszFileName == NULL) {
                        if (pTraceData->m_pszFunction == NULL) {
                            pszFormat = "WIA: %lu.%lu %lu %X %lu [%s] %s\r\n";
                        } else {
                            pszFormat = "WIA: %lu.%lu %lu %X %lu [%s] %s, %s\r\n";
                        }
                    } else {
                        if (pTraceData->m_pszFunction == NULL) {
                            pszFormat = "%hs(%lu) : %lu.%lu %lu %X %lu [%s] WIA: %s\r\n";
                        } else {
                            pszFormat = "%hs(%lu) : %lu.%lu %lu %X %lu [%s] WIA: %s, %s\r\n";
                        }
                    }
                    break;
                    
                default:
                    return;
            }
            
            StringCchPrintfA(szBuffer1, sizeof(szBuffer1), pszFormat,
                            pszFileName ? pszFileName : "",
                            pTraceData->m_ulLine,
                            pst->wHour, pst->wMinute,
                            GetCurrentThreadId(),
                            GetCurrentProcessId(),
                            WIATRACE_GetIndentLevel(),
                            pszModuleName,
                            pTraceData->m_pszString,
                            pTraceData->m_pszFunction ? pTraceData->m_pszFunction : "");
        }
        
        if (((dwFlags & 0x01000000) == 0) && pszFileName != NULL && pTraceData->m_ulLine != 0) {
            pszCurrent = szBuffer2;
            do {
                cTemp = *pszCurrent;
                pszCurrent++;
            } while (cTemp != '\0');
            
            StringCchPrintfA(szBuffer2 + (pszCurrent - szBuffer2 - 1),
                            sizeof(szBuffer2) - (pszCurrent - szBuffer2 - 1),
                            "%hs(%lu) : ", pszFileName, pTraceData->m_ulLine);
        }
        
        if ((dwFlags & 0x04000000) == 0) {
            pszCurrent = szBuffer2;
            do {
                cTemp = *pszCurrent;
                pszCurrent++;
            } while (cTemp != '\0');
            
            StringCchPrintfA(szBuffer2 + (pszCurrent - szBuffer2 - 1),
                            sizeof(szBuffer2) - (pszCurrent - szBuffer2 - 1),
                            "%lu.%lu ", pst->wHour, pst->wMinute);
        }
        
        if ((dwFlags & 0x02000000) == 0) {
            pszCurrent = szBuffer2;
            do {
                cTemp = *pszCurrent;
                pszCurrent++;
            } while (cTemp != '\0');
            
            StringCchPrintfA(szBuffer2 + (pszCurrent - szBuffer2 - 1),
                            sizeof(szBuffer2) - (pszCurrent - szBuffer2 - 1),
                            "%lu ", GetCurrentThreadId());
        }
        
        if ((dwFlags & 0x10000000) == 0) {
            pszCurrent = szBuffer2;
            do {
                cTemp = *pszCurrent;
                pszCurrent++;
            } while (cTemp != '\0');
            
            StringCchPrintfA(szBuffer2 + (pszCurrent - szBuffer2 - 1),
                            sizeof(szBuffer2) - (pszCurrent - szBuffer2 - 1),
                            "%X ", GetCurrentProcessId());
        }
        
        if ((dwFlags & 0x20000000) == 0) {
            pszCurrent = szBuffer2;
            do {
                cTemp = *pszCurrent;
                pszCurrent++;
            } while (cTemp != '\0');
            
            StringCchPrintfA(szBuffer2 + (pszCurrent - szBuffer2 - 1),
                            sizeof(szBuffer2) - (pszCurrent - szBuffer2 - 1),
                            "%lu ", WIATRACE_GetIndentLevel());
        }
        
        if ((dwFlags & 0x08000000) == 0) {
            pszCurrent = szBuffer2;
            do {
                cTemp = *pszCurrent;
                pszCurrent++;
            } while (cTemp != '\0');
            
            StringCchPrintfA(szBuffer2 + (pszCurrent - szBuffer2 - 1),
                            sizeof(szBuffer2) - (pszCurrent - szBuffer2 - 1),
                            "[%s] ", pszModuleName);
        }
        
        if ((dwFlags & 0x40000000) == 0) {
            StringCchCatA(szBuffer2, sizeof(szBuffer2), "WIA: ");
        }
        
        if (!(dwFlags & 0x80000000) && 
            (pTraceData->m_ulTraceSeverity == 1 || pTraceData->m_ulTraceSeverity == 0x80 || pTraceData->m_ulTraceSeverity == 2)) {
            switch (pTraceData->m_ulTraceSeverity) {
                case 1:
                case 0x80:
                    StringCchCatA(szBuffer2, sizeof(szBuffer2), "ERROR: ");
                    break;
                case 2:
                    StringCchCatA(szBuffer2, sizeof(szBuffer2), "WARNING: ");
                    break;
            }
        }
        
        if (((dwFlags & 0x00800000) == 0) && pTraceData->m_pszFunction != NULL) {
            pszCurrent = szBuffer2;
            do {
                cTemp = *pszCurrent;
                pszCurrent++;
            } while (cTemp != '\0');
            
            StringCchPrintfA(szBuffer2 + (pszCurrent - szBuffer2 - 1),
                            sizeof(szBuffer2) - (pszCurrent - szBuffer2 - 1),
                            "%s, ", pTraceData->m_pszFunction);
        }
        
        StringCchCatA(szBuffer2, sizeof(szBuffer2), pTraceData->m_pszString);
        StringCchCatA(szBuffer2, sizeof(szBuffer2), "\r\n");
    }
    
    if (bWriteToFile != 0) {
        WiaTrace::GetBanner(0, szBanner, pst, (char*)hFile, 0);
        WiaTrace::WriteToFile(hFile, hMutex, 0, 0, szBanner);
    }
    
    if (WiaTrace::g_TraceOutput_bMaxFileSizeReached == FALSE) {
        dwWaitResult = WaitForSingleObject(hMutex, 10000);
        if (dwWaitResult == WAIT_OBJECT_0 || dwWaitResult == WAIT_ABANDONED) {
            liNewPosition.QuadPart = 0;
            SetFilePointerEx(hFile, liNewPosition, &liFileSize, FILE_END);
            
            pszCurrent = pszOutputBuffer;
            do {
                cTemp = *pszCurrent;
                pszCurrent++;
            } while (cTemp != '\0');
            dwLength = (DWORD)(pszCurrent - pszOutputBuffer - 1);
            
            if (liFileSize.QuadPart + dwLength > dwMaxFileSize) {
                if ((dwFlags & 0x200) == 0) {
                    WiaTrace::WrapFile(hFile);
                    WiaTrace::GetBanner(0, szBanner, pst, (char*)hFile, 0);
                    
                    pszCurrent = szBanner;
                    do {
                        cTemp = *pszCurrent;
                        pszCurrent++;
                    } while (cTemp != '\0');
                    WriteFile(hFile, szBanner, (DWORD)(pszCurrent - szBanner - 1), &dwBytesWritten, NULL);
                } else {
                    WiaTrace::g_TraceOutput_bMaxFileSizeReached = TRUE;
                    WriteFile(hFile, pszOutputBuffer, dwLength, &dwBytesWritten, NULL);
                    
                    WiaTrace::GetBanner(0, szBanner, pst, (char*)hFile, 0);
                    pszCurrent = szBanner;
                    do {
                        cTemp = *pszCurrent;
                        pszCurrent++;
                    } while (cTemp != '\0');
                    pszOutputBuffer = szBanner;
                    dwLength = (DWORD)(pszCurrent - szBanner - 1);
                }
            }
            
            WriteFile(hFile, pszOutputBuffer, dwLength, &dwBytesWritten, NULL);
            ReleaseMutex(hMutex);
        } else {
            StringCchPrintfA(szBanner, sizeof(szBanner),
                            "WIA: ERROR!!! Failed to write to trace file, couldn't gain access!!!, WaitResult = %lu",
                            dwWaitResult);
        }
    }
}

void WINAPI WIATRACE_SetTraceSettings(DWORD dwTraceMask, DWORD dwTraceLevel, INT bActivated)
{
    TlsSetValue(WiaTrace::g_tls_idx_ulTraceMask, (LPVOID)dwTraceMask);
    TlsSetValue(WiaTrace::g_tls_idx_ulTraceLevel, (LPVOID)dwTraceLevel);
    TlsSetValue(WiaTrace::g_tls_idx_bActivated, (LPVOID)bActivated);
}

void WiaTrace::InitThreadData()
{
    TlsSetValue(g_tls_idx_ulTraceMask, (LPVOID)0);
    TlsSetValue(g_tls_idx_ulTraceLevel, (LPVOID)0);
    TlsSetValue(g_tls_idx_ulIndentLevel, (LPVOID)0);
    TlsSetValue(g_tls_idx_bActivated, (LPVOID)0);
}

HRESULT WiaTrace::GetBanner(DWORD dwBannerType, char* pszBanner, SYSTEMTIME* pst, char* pszPath, DWORD dwParam)
{
    char szModulePath[MAX_PATH];
    char szDrive[MAX_PATH];
    char szDir[MAX_PATH];
    char szFilename[MAX_PATH];
    char szExt[MAX_PATH];
    char szBuffer[512];
    SYSTEMTIME st;
    
    if (pszBanner == NULL) {
        return E_POINTER;
    }
    
    if (dwBannerType == 2) {
        GetLocalTime(&st);
        ExpandEnvironmentStringsA("%systemroot%\\Debug\\WIA\\wiatrace.bak.log", szBuffer, sizeof(szBuffer));
        StringCchPrintfA(pszBanner, 256,
                        "\r\n**************** File Rollover at %04lu/%02lu/%02lu %02lu:%02lu:%02lu:%03lu, previous file stored at '%s' ****************\r\n",
                        st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
                        szBuffer);
    }
    else if (dwBannerType == 1) {
        GetModuleFileNameA(NULL, szModulePath, sizeof(szModulePath));
        _splitpath_s(szModulePath, szDrive, sizeof(szDrive), szDir, sizeof(szDir), szFilename, sizeof(szFilename), szExt, sizeof(szExt));
        StringCchCatA(szFilename, sizeof(szFilename), szExt);
        
        GetLocalTime(&st);
        StringCchPrintfA(pszBanner, 256,
                        "\r\n**************** Started trace for Module: [%s] in Executable [%s] ProcessID: [%lu] at %04lu/%02lu/%02lu %02lu:%02lu:%02lu:%03lu ****************\r\n ",
                        szFilename, szModulePath, GetCurrentProcessId(),
                        st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    }
    else if (dwBannerType == 3) {
        GetLocalTime(&st);
        StringCchPrintfA(pszBanner, 256,
                        "\r\n**************** Maximum file size exceeded at %04lu/%02lu/%02lu %02lu:%02lu:%02lu:%03lu - file wrapping is turned off. ****************\r\n",
                        st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    }
    
    return S_OK;
}

HRESULT WiaTrace::WrapFile(void* hFile)
{
    char szCurrentLog[MAX_PATH];
    char szBackupLog[MAX_PATH];
    
    ExpandEnvironmentStringsA("%systemroot%\\Debug\\WIA\\wiatrace.log", szCurrentLog, sizeof(szCurrentLog));
    ExpandEnvironmentStringsA("%systemroot%\\Debug\\WIA\\wiatrace.bak.log", szBackupLog, sizeof(szBackupLog));
    
    CopyFileA(szCurrentLog, szBackupLog, FALSE);
    
    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
    SetEndOfFile(hFile);
    
    return S_OK;
}

BOOL WiaTrace::IsMaxFileSizeExceeded(void* hFile, DWORD dwMaxSize)
{
    LARGE_INTEGER liFileSize;
    liFileSize.QuadPart = 0;
    SetFilePointerEx(hFile, liFileSize, &liFileSize, FILE_END);
    return (liFileSize.QuadPart > dwMaxSize);
}

HRESULT WiaTrace::WriteToFile(void* hFile, void* hMutex, DWORD dwStringLen, DWORD dwParam, char* pszString)
{
    DWORD dwBytesWritten;
    DWORD dwWaitResult;
    LARGE_INTEGER liFileSize;
    char szBanner[256];
    char* pszCurrent;
    char cTemp;
    DWORD dwLength;
    
    if (g_TraceOutput_bMaxFileSizeReached == FALSE) {
        dwWaitResult = WaitForSingleObject(hMutex, 10000);
        if (dwWaitResult == WAIT_OBJECT_0 || dwWaitResult == WAIT_ABANDONED) {
            liFileSize.QuadPart = 0;
            SetFilePointerEx(hFile, liFileSize, &liFileSize, FILE_END);
            
            pszCurrent = pszString;
            do {
                cTemp = *pszCurrent;
                pszCurrent++;
            } while (cTemp != '\0');
            dwLength = (DWORD)(pszCurrent - pszString - 1);
            
            if (IsMaxFileSizeExceeded(hFile, dwParam)) {
                if ((dwParam & 0x200) == 0) {
                    WrapFile(hFile);
                    GetBanner(0, szBanner, NULL, (char*)hFile, 0);
                    
                    pszCurrent = szBanner;
                    do {
                        cTemp = *pszCurrent;
                        pszCurrent++;
                    } while (cTemp != '\0');
                    WriteFile(hFile, szBanner, (DWORD)(pszCurrent - szBanner - 1), &dwBytesWritten, NULL);
                } else {
                    g_TraceOutput_bMaxFileSizeReached = TRUE;
                    WriteFile(hFile, pszString, dwLength, &dwBytesWritten, NULL);
                    
                    GetBanner(0, szBanner, NULL, (char*)hFile, 0);
                    pszCurrent = szBanner;
                    do {
                        cTemp = *pszCurrent;
                        pszCurrent++;
                    } while (cTemp != '\0');
                    pszString = szBanner;
                    dwLength = (DWORD)(pszCurrent - szBanner - 1);
                }
            }
            
            WriteFile(hFile, pszString, dwLength, &dwBytesWritten, NULL);
            ReleaseMutex(hMutex);
        } else {
            StringCchPrintfA(szBanner, sizeof(szBanner),
                            "WIA: ERROR!!! Failed to write to trace file, couldn't gain access!!!, WaitResult = %lu",
                            dwWaitResult);
        }
    }
    
    return S_OK;
}

