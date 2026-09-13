#include <windows.h>
#include <pdh.h>
#include <string.h>
#include "ftpd.h"

int __cdecl FtpQueryCounters(LPFTP_STATISTICS_0 pStats)
{
    PDH_STATUS status;
    HQUERY hQuery = NULL;
    HCOUNTER hCounters[15];
    DWORD dwCounterIndex;
    DWORD dwSize;
    WCHAR szCounterPath[260];
    PDH_FMT_COUNTERVALUE fmtValue;

    PDH_COUNTER_PATH_ELEMENTS_W counterPathElements[15] = {
        { NULL, L"Microsoft FTP Service", L"_Total", NULL, 0xFFFFFFFF, L"Bytes Sent/sec" },
        { NULL, L"Microsoft FTP Service", L"_Total", NULL, 0xFFFFFFFF, L"Bytes Received/sec" },
        { NULL, L"Microsoft FTP Service", L"_Total", NULL, 0xFFFFFFFF, L"Total Files Sent" },
        { NULL, L"Microsoft FTP Service", L"_Total", NULL, 0xFFFFFFFF, L"Total Files Received" },
        { NULL, L"Microsoft FTP Service", L"_Total", NULL, 0xFFFFFFFF, L"Current Anonymous Users" },
        { NULL, L"Microsoft FTP Service", L"_Total", NULL, 0xFFFFFFFF, L"Current NonAnonymous Users" },
        { NULL, L"Microsoft FTP Service", L"_Total", NULL, 0xFFFFFFFF, L"Total Anonymous Users" },
        { NULL, L"Microsoft FTP Service", L"_Total", NULL, 0xFFFFFFFF, L"Total NonAnonymous Users" },
        { NULL, L"Microsoft FTP Service", L"_Total", NULL, 0xFFFFFFFF, L"Maximum Anonymous Users" },
        { NULL, L"Microsoft FTP Service", L"_Total", NULL, 0xFFFFFFFF, L"Maximum NonAnonymous Users" },
        { NULL, L"Microsoft FTP Service", L"_Total", NULL, 0xFFFFFFFF, L"Current Connections" },
        { NULL, L"Microsoft FTP Service", L"_Total", NULL, 0xFFFFFFFF, L"Maximum Connections" },
        { NULL, L"Microsoft FTP Service", L"_Total", NULL, 0xFFFFFFFF, L"Total Connection Attempts" },
        { NULL, L"Microsoft FTP Service", L"_Total", NULL, 0xFFFFFFFF, L"Total Logon Attempts" },
        { NULL, L"Microsoft FTP Service", L"_Total", NULL, 0xFFFFFFFF, L"Microsoft FTP Service Uptime" }
    };

    memset(&fmtValue, 0, sizeof(fmtValue));

    status = PdhOpenQueryW(NULL, 0, &hQuery);
    if (status == ERROR_SUCCESS) {
        for (dwCounterIndex = 0; dwCounterIndex < 15; dwCounterIndex++) {
            dwSize = 0x104;
            status = PdhMakeCounterPathW(&counterPathElements[dwCounterIndex], szCounterPath, &dwSize, 0);
            if (status != ERROR_SUCCESS) {
                goto cleanup;
            }

            status = PdhAddEnglishCounterW(hQuery, szCounterPath, 0, &hCounters[dwCounterIndex]);
            if (status != ERROR_SUCCESS) {
                goto cleanup;
            }
        }

        status = PdhCollectQueryData(hQuery);
        if (status == ERROR_SUCCESS) {
            for (dwCounterIndex = 0; dwCounterIndex < 15; dwCounterIndex++) {
                if (dwCounterIndex < 2) {
                    status = PdhGetFormattedCounterValue(hCounters[dwCounterIndex], PDH_FMT_LARGE, NULL, &fmtValue);
                    if (status != ERROR_SUCCESS) {
                        break;
                    }
                    ((LARGE_INTEGER *)pStats)[dwCounterIndex].QuadPart = fmtValue.largeValue;
                } else {
                    status = PdhGetFormattedCounterValue(hCounters[dwCounterIndex], PDH_FMT_LONG, NULL, &fmtValue);
                    if (status != ERROR_SUCCESS) {
                        break;
                    }
                    ((DWORD *)pStats)[dwCounterIndex + 2] = fmtValue.longValue;
                }
            }
        }
    }

cleanup:
    if (hQuery != NULL) {
        PdhCloseQuery(hQuery);
    }

    return TRUE;
}

