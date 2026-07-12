#include <windows.h>
#include <errorrep.h>
#include <werapi.h>
#include <werexceptionreporting.h>

tagEFaultRepRetVal __cdecl BasepReportFault(PEXCEPTION_POINTERS param_1,unsigned long param_2)

{
                    /* 0x1340  1  BasepReportFault */
  return frrvErrNoDW;
}

long __cdecl CheckForReadOnlyResourceFilter(PEXCEPTION_POINTERS param_1)

{
                    /* 0x1330  2  CheckForReadOnlyResourceFilter */
  return FALSE;
}

HRESULT WINAPI
WerpLaunchAeDebug(void *param_1,void *param_2,PEXCEPTION_RECORD param_3,_CONTEXT *param_4,
                 PWERP_DEBUGGER_INFO param_5)

{
                    /* 0x1360  21  WerpLaunchAeDebug */
  return E_FAIL;
}

HRESULT
WINAPI
WerReportAddDump(
    _In_ HREPORT hReportHandle,
    _In_ HANDLE  hProcess,
    _In_opt_ HANDLE hThread,
    _In_ WER_DUMP_TYPE dumpType,
    _In_opt_  PWER_EXCEPTION_INFORMATION pExceptionParam,
    _In_opt_ PWER_DUMP_CUSTOM_OPTIONS pDumpCustomOptions,
    _In_ DWORD dwFlags
    )

{
                    /* 0x1380  3  WerReportAddDump
                       0x1380  17  WerpAddFileBuffer */
  return E_FAIL;
}

HRESULT WINAPI
WerpAddFileBuffer(void *param_1,void *param_2,void *param_3,_WER_DUMP_TYPE param_4,
                _WER_EXCEPTION_INFORMATION *param_5,_WER_DUMP_CUSTOM_OPTIONS *param_6,unsigned long param_7)

{
                    /* 0x1380  3  WerReportAddDump
                       0x1380  17  WerpAddFileBuffer */
  return E_FAIL;
}

HRESULT
WINAPI
WerReportAddFile(
    _In_ HREPORT hReportHandle,
    _In_ PCWSTR pwzPath,
    _In_ WER_FILE_TYPE repFileType,
    _In_ DWORD  dwFileFlags
    )
{
                    /* 0x1370  4  WerReportAddFile
                       0x1370  6  WerReportCreate
                       0x1370  7  WerReportSetParameter
                       0x1370  8  WerReportSubmit
                       0x1370  16  WerStoreUploadReport */
  return E_FAIL;
}

HRESULT
WINAPI
WerReportCreate(
    _In_ PCWSTR pwzEventType,
    _In_ WER_REPORT_TYPE repType,
    _In_opt_ PWER_REPORT_INFORMATION pReportInformation,
    _Out_ HREPORT *phReportHandle
    )

{
                    /* 0x1370  4  WerReportAddFile
                       0x1370  6  WerReportCreate
                       0x1370  7  WerReportSetParameter
                       0x1370  8  WerReportSubmit
                       0x1370  16  WerStoreUploadReport */
  return E_FAIL;
}

HRESULT
WINAPI
WerReportSetParameter(
    _In_ HREPORT hReportHandle,
    _In_ DWORD dwparamID,
    _In_opt_ PCWSTR pwzName,
    _In_ PCWSTR pwzValue
    )

{
                    /* 0x1370  4  WerReportAddFile
                       0x1370  6  WerReportCreate
                       0x1370  7  WerReportSetParameter
                       0x1370  8  WerReportSubmit
                       0x1370  16  WerStoreUploadReport */
  return E_FAIL;
}

HRESULT
WINAPI
WerReportSubmit(
    _In_ HREPORT hReportHandle,
    _In_ WER_CONSENT consent,
    _In_ DWORD  dwFlags,
    _Out_opt_ PWER_SUBMIT_RESULT pSubmitResult
    )

{
                    /* 0x1370  4  WerReportAddFile
                       0x1370  6  WerReportCreate
                       0x1370  7  WerReportSetParameter
                       0x1370  8  WerReportSubmit
                       0x1370  16  WerStoreUploadReport */
  return E_FAIL;
}

HRESULT
WerStoreUploadReport(
    _In_ HREPORTSTORE hReportStore,
    _In_ PCWSTR pszReportKey,
    _In_ DWORD dwFlags,
    _Out_opt_ PWER_SUBMIT_RESULT pSubmitResult
    )

{
                    /* 0x1370  4  WerReportAddFile
                       0x1370  6  WerReportCreate
                       0x1370  7  WerReportSetParameter
                       0x1370  8  WerReportSubmit
                       0x1370  16  WerStoreUploadReport */
  return E_FAIL;
}

HRESULT
WINAPI
WerReportCloseHandle(
    _In_ HREPORT hReportHandle
    )

{
                    /* 0x1390  5  WerReportCloseHandle
                       0x1390  18  WerpCreateIntegratorReportId */
  return E_FAIL;
}

HRESULT WINAPI WerpCreateIntegratorReportId(void *param_1)

{
                    /* 0x1390  5  WerReportCloseHandle
                       0x1390  18  WerpCreateIntegratorReportId */
  return E_FAIL;
}

VOID
WerStoreClose(
    _In_opt_ _Post_invalid_ HREPORTSTORE hReportStore
    )

{
                    /* 0x13a0  9  WerStoreClose
                       0x13a0  19  WerpFreeString */
  return;
}

void __cdecl WerpFreeString(void *param_1)

{
                    /* 0x13a0  9  WerStoreClose
                       0x13a0  19  WerpFreeString */
  return;
}

HRESULT
WerStoreGetFirstReportKey(
    _In_ HREPORTSTORE hReportStore,
    _Outptr_result_maybenull_z_ PCWSTR* ppszReportKey
    )

{
                    /* 0x1350  10  WerStoreGetFirstReportKey
                       0x1350  11  WerStoreGetNextReportKey
                       0x1350  12  WerStoreGetReportCount
                       0x1350  13  WerStoreGetSizeOnDisk
                       0x1350  14  WerStoreOpen
                       0x1350  20  WerpGetDebugger
                       0x1350  22  WerpSetIntegratorReportId */
  return E_FAIL;
}

HRESULT
WerStoreGetNextReportKey(
    _In_ HREPORTSTORE hReportStore,
    _Outptr_result_maybenull_z_ PCWSTR* ppszReportKey
    )

{
                    /* 0x1350  10  WerStoreGetFirstReportKey
                       0x1350  11  WerStoreGetNextReportKey
                       0x1350  12  WerStoreGetReportCount
                       0x1350  13  WerStoreGetSizeOnDisk
                       0x1350  14  WerStoreOpen
                       0x1350  20  WerpGetDebugger
                       0x1350  22  WerpSetIntegratorReportId */
  return E_FAIL;
}

HRESULT
WerStoreGetReportCount(
    _In_ HREPORTSTORE hReportStore,
    _Out_ DWORD* pdwReportCount
    )

{
                    /* 0x1350  10  WerStoreGetFirstReportKey
                       0x1350  11  WerStoreGetNextReportKey
                       0x1350  12  WerStoreGetReportCount
                       0x1350  13  WerStoreGetSizeOnDisk
                       0x1350  14  WerStoreOpen
                       0x1350  20  WerpGetDebugger
                       0x1350  22  WerpSetIntegratorReportId */
  return E_FAIL;
}

HRESULT
WerStoreGetSizeOnDisk(
    _In_ HREPORTSTORE hReportStore,
    _Out_ ULONGLONG* pqwSizeInBytes
    )

{
                    /* 0x1350  10  WerStoreGetFirstReportKey
                       0x1350  11  WerStoreGetNextReportKey
                       0x1350  12  WerStoreGetReportCount
                       0x1350  13  WerStoreGetSizeOnDisk
                       0x1350  14  WerStoreOpen
                       0x1350  20  WerpGetDebugger
                       0x1350  22  WerpSetIntegratorReportId */
  return E_FAIL;
}

HRESULT
WerStoreOpen(
    _In_ REPORT_STORE_TYPES repStoreType,
    _Out_ PHREPORTSTORE phReportStore
    )

{
                    /* 0x1350  10  WerStoreGetFirstReportKey
                       0x1350  11  WerStoreGetNextReportKey
                       0x1350  12  WerStoreGetReportCount
                       0x1350  13  WerStoreGetSizeOnDisk
                       0x1350  14  WerStoreOpen
                       0x1350  20  WerpGetDebugger
                       0x1350  22  WerpSetIntegratorReportId */
  return E_FAIL;
}

HRESULT WINAPI WerpGetDebugger(void *param_1,wchar_t **param_2)

{
                    /* 0x1350  10  WerStoreGetFirstReportKey
                       0x1350  11  WerStoreGetNextReportKey
                       0x1350  12  WerStoreGetReportCount
                       0x1350  13  WerStoreGetSizeOnDisk
                       0x1350  14  WerStoreOpen
                       0x1350  20  WerpGetDebugger
                       0x1350  22  WerpSetIntegratorReportId */
  return E_FAIL;
}

HRESULT WINAPI WerpSetIntegratorReportId(void *param_1,wchar_t **param_2)

{
                    /* 0x1350  10  WerStoreGetFirstReportKey
                       0x1350  11  WerStoreGetNextReportKey
                       0x1350  12  WerStoreGetReportCount
                       0x1350  13  WerStoreGetSizeOnDisk
                       0x1350  14  WerStoreOpen
                       0x1350  20  WerpGetDebugger
                       0x1350  22  WerpSetIntegratorReportId */
  return E_FAIL;
}

HRESULT
WerStoreQueryReportMetadataV1(
    _In_ HREPORTSTORE hReportStore,
    _In_ PCWSTR pszReportKey,
    _Out_ PWER_REPORT_METADATA_V1 pReportMetadata
    )

{
                    /* 0x13b0  15  WerStoreQueryReportMetadataV1 */
  return E_FAIL;
}
