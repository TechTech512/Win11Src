#pragma once

#include <windows.h>
#include <setupapi.h>
#include <cfgmgr32.h>

#ifdef __cplusplus
extern "C" {
#endif

// ------------------------------------------------------------------
// Actual HWN device interface GUIDs (from binary)
// ------------------------------------------------------------------
extern const GUID HWN_DEVINTERFACE_NLED;
extern const GUID HWN_DEVINTERFACE_VIBRATOR;

// Logging function
void LogW(unsigned short* format, ...);

// Device path retrieval
int GetDevicePath(const GUID* guid, wchar_t* buffer, DWORD bufferSize);

// Device control functions
int TurnNLEDOnOff(HANDLE hDevice, BOOL on);
int TurnVibraOnOff(HANDLE hDevice, BOOL on);

// Test entry points
int NLEDTest(void);
int VibraTest(void);

#ifdef __cplusplus
}
#endif
