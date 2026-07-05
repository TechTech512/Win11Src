#include "hwn.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <initguid.h>

// ------------------------------------------------------------------
// Actual GUIDs (from binary, little‑endian order)
// NLED:  {0x6b2a25e2, 0xaa, 0xf5, 0x48, 0x2c, 0x99, 0xa5, 0x62, 0x05, 0xcd, 0xcc, 0x17, 0x6a}
// VIBRA: {0x825e8fd3, 0x64, 0x67, 0x4a, 0x5b, 0xab, 0xfb, 0xbb, 0x01, 0x07, 0x34, 0xff, 0x4d}
// ------------------------------------------------------------------
DEFINE_GUID(HWN_DEVINTERFACE_NLED,
    0x6b2a25e2, 0xaaf5, 0x482c, 0x99, 0xa5, 0x62, 0x05, 0xcd, 0xcc, 0x17, 0x6a);

DEFINE_GUID(HWN_DEVINTERFACE_VIBRATOR,
    0x825e8fd3, 0x6467, 0x4a5b, 0xab, 0xfb, 0xbb, 0x01, 0x07, 0x34, 0xff, 0x4d);

// ------------------------------------------------------------------
// Internal logging buffer (matches original)
// ------------------------------------------------------------------
static wchar_t m_wszBuff[4096];

void LogW(unsigned short* format, ...)
{
    va_list args;
    va_start(args, format);
    int len = _vsnwprintf_s(m_wszBuff, _countof(m_wszBuff), _TRUNCATE, (const wchar_t *)format, args);
    va_end(args);
    if (len < 0 || len >= (int)_countof(m_wszBuff)) {
        m_wszBuff[_countof(m_wszBuff) - 1] = L'\0';
        OutputDebugStringW(L"ERR: failed to write wchar to internal buffer\n");
        wprintf(L"ERR: failed to write wchar to internal buffer\n");
    } else {
        OutputDebugStringW(m_wszBuff);
        wprintf(m_wszBuff);
        fflush(stdout);
    }
}

// ------------------------------------------------------------------
// Get device interface path
// ------------------------------------------------------------------
int GetDevicePath(const GUID* guid, wchar_t* buffer, DWORD bufferSize)
{
    DWORD requiredSize = 0;
    CONFIGRET cr = CM_Get_Device_Interface_List_SizeW(
        &requiredSize,
        (LPGUID)guid,
        NULL,
        CM_GET_DEVICE_INTERFACE_LIST_ALL_DEVICES
    );
    if (cr != CR_SUCCESS) {
        LogW((unsigned short *)L"CM_Get_Device_Interface_List_Size ERROR. failed with error: 0x%08X\r\n", cr);
        return 0;
    }
    if (requiredSize > bufferSize) {
        LogW((unsigned short *)L"CM_Get_Device_Interface_List_Size ERROR. Buffer size not large enough to store path\r\n");
        return 0;
    }
    cr = CM_Get_Device_Interface_ListW(
        (LPGUID)guid,
        NULL,
        buffer,
        bufferSize,
        CM_GET_DEVICE_INTERFACE_LIST_ALL_DEVICES
    );
    if (cr != CR_SUCCESS) {
        LogW((unsigned short *)L"CM_Get_Device_Interface_List ERROR. failed with error: 0x%08X\r\n", cr);
        return 0;
    }
    return 1;
}

// ------------------------------------------------------------------
// Control NLED
// ------------------------------------------------------------------
int TurnNLEDOnOff(HANDLE hDevice, BOOL on)
{
    DWORD bytesReturned = 0;
    DWORD lastError = 0;

    typedef struct {
        DWORD size;       // 0x30
        DWORD version;    // 1
        DWORD unknown1;   // 1
        DWORD unknown2;   // 1
        DWORD unknown3;   // 1
        DWORD unknown4;   // 100
        DWORD unknown5;   // 5000
        DWORD unknown6;   // 100
        DWORD unknown7;   // 1
        DWORD unknown8;   // 1
        DWORD unknown9;   // 0
        DWORD state;      // 1 = on, 0 = off
    } NLED_BUFFER;

    NLED_BUFFER* pBuffer = (NLED_BUFFER*)malloc(sizeof(NLED_BUFFER));
    if (!pBuffer) {
        lastError = GetLastError();
        LogW((unsigned short *)L"malloc ERROR. Insufficient memory to allocate inBuffer for NLED. 0x%08X\r\n", lastError);
        return lastError;
    }

    memset(pBuffer, 0, sizeof(NLED_BUFFER));
    pBuffer->size = sizeof(NLED_BUFFER);
    pBuffer->version = 1;
    pBuffer->unknown1 = 1;
    pBuffer->unknown2 = 1;
    pBuffer->unknown3 = 1;
    pBuffer->unknown4 = 100;
    pBuffer->unknown5 = 5000;
    pBuffer->unknown6 = 100;
    pBuffer->unknown7 = 1;
    pBuffer->unknown8 = 1;
    pBuffer->state = on ? 1 : 0;

    BOOL bResult = DeviceIoControl(
        hDevice,
        0x228000,
        pBuffer,
        sizeof(NLED_BUFFER),
        NULL,
        0,
        &bytesReturned,
        NULL
    );
    if (!bResult) {
        lastError = GetLastError();
        LogW((unsigned short *)L"DeviceIoControl ERROR. failed for NLED with error . 0x%08X\r\n", lastError);
    }
    free(pBuffer);
    return lastError;
}

// ------------------------------------------------------------------
// Control Vibration
// ------------------------------------------------------------------
int TurnVibraOnOff(HANDLE hDevice, BOOL on)
{
    DWORD bytesReturned = 0;
    DWORD lastError = 0;

    typedef struct {
        DWORD size;       // 0x30
        DWORD version;    // 1
        DWORD unknown1;   // 1
        DWORD unknown2;   // 1
        DWORD unknown3;   // 2
        DWORD unknown4;   // 2
        DWORD unknown5;   // 0
        DWORD unknown6;   // 0
        DWORD unknown7;   // 0
        DWORD unknown8;   // 0
        DWORD unknown9;   // 0
        DWORD state;      // 1 = on, 0 = off
    } VIBRA_BUFFER;

    VIBRA_BUFFER* pBuffer = (VIBRA_BUFFER*)malloc(sizeof(VIBRA_BUFFER));
    if (!pBuffer) {
        lastError = GetLastError();
        LogW((unsigned short *)L"malloc ERROR. Insufficient memory to allocate inBuffer for vibra. 0x%08X\r\n", lastError);
        return lastError;
    }

    memset(pBuffer, 0, sizeof(VIBRA_BUFFER));
    pBuffer->size = sizeof(VIBRA_BUFFER);
    pBuffer->version = 1;
    pBuffer->unknown1 = 1;
    pBuffer->unknown2 = 1;
    pBuffer->unknown3 = 2;
    pBuffer->unknown4 = 2;
    pBuffer->state = on ? 1 : 0;

    BOOL bResult = DeviceIoControl(
        hDevice,
        0x228000,
        pBuffer,
        sizeof(VIBRA_BUFFER),
        NULL,
        0,
        &bytesReturned,
        NULL
    );
    if (!bResult) {
        lastError = GetLastError();
        LogW((unsigned short *)L"DeviceIoControl ERROR. failed for vibra with error . 0x%08X\r\n", lastError);
    }
    free(pBuffer);
    return lastError;
}

// ------------------------------------------------------------------
// NLED Test
// ------------------------------------------------------------------
int NLEDTest(void)
{
    wchar_t devicePath[264];
    memset(devicePath, 0, sizeof(devicePath));

    LogW((unsigned short *)L"TEST_NAME:%s\r\n",L"NLED");
    LogW((unsigned short *)L"TEST_DESC:%s\r\n",L"Turns on NLED for 5 seconds");

    if (!GetDevicePath(&HWN_DEVINTERFACE_NLED, devicePath, _countof(devicePath))) {
        return 0;
    }

    HANDLE hDevice = CreateFileW(
        devicePath,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );
    if (hDevice == INVALID_HANDLE_VALUE) {
        LogW((unsigned short *)L"CreateFile ERROR. NLED driver could not be opened. 0x%08X\r\n", GetLastError());
        return 0;
    }

    LogW((unsigned short *)L"DEVICE_STATE: ON\r\n");
    DWORD err = TurnNLEDOnOff(hDevice, TRUE);
    if (err == 0) {
        Sleep(5000);
        LogW((unsigned short *)L"DEVICE_STATE: OFF\r\n");
        err = TurnNLEDOnOff(hDevice, FALSE);
        if (err != 0) {
            LogW((unsigned short *)L"TurnNLEDOnOff ERROR Turning NLED Off: 0x%08x\n", err);
        }
    } else {
        LogW((unsigned short *)L"TurnNLEDOnOff ERROR Turning NLED : 0x%08x\n", err);
    }

    if (hDevice != INVALID_HANDLE_VALUE) {
        CloseHandle(hDevice);
    }
    return err;  // 0 on success
}

// ------------------------------------------------------------------
// Vibration Test
// ------------------------------------------------------------------
int VibraTest(void)
{
    wchar_t devicePath[264];
    memset(devicePath, 0, sizeof(devicePath));

    LogW((unsigned short *)L"TEST_NAME:%s\r\n",L"VIBRA");
    LogW((unsigned short *)L"TEST_DESC:%s\r\n",L"Turns on VIBRA for 5 seconds");

    if (!GetDevicePath(&HWN_DEVINTERFACE_VIBRATOR, devicePath, _countof(devicePath))) {
        return 0;
    }

    HANDLE hDevice = CreateFileW(
        devicePath,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );
    if (hDevice == INVALID_HANDLE_VALUE) {
        LogW((unsigned short *)L"CreateFile ERROR. Vibra driver could not be opened. 0x%08X\r\n", GetLastError());
        return 0;
    }

    LogW((unsigned short *)L"DEVICE_STATE: ON\r\n");
    DWORD err = TurnVibraOnOff(hDevice, TRUE);
    if (err == 0) {
        Sleep(5000);
        LogW((unsigned short *)L"DEVICE_STATE: OFF\r\n");
        err = TurnVibraOnOff(hDevice, FALSE);
        if (err != 0) {
            LogW((unsigned short *)L"TurnVibraOnOff ERROR Turning Vibra Off: 0x%08x\n", err);
        }
    } else {
        LogW((unsigned short *)L"TurnVibraOnOff ERROR Turning Vibra On: 0x%08x\r\n", err);
    }

    if (hDevice != INVALID_HANDLE_VALUE) {
        CloseHandle(hDevice);
    }
    return err;  // 0 on success
}

