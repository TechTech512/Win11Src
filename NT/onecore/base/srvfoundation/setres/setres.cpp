// setres.cpp - Display settings configuration utility
// Reconstructed from Microsoft's setres.exe (Windows 10 build 26100)

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include "resource.h"

// Global variables
HINSTANCE__* appHandle = nullptr;
_devicemodeA* currentModes = nullptr;
int displayCount = 0;
bool force = false;

// Function declarations
bool __cdecl checkFlag(wchar_t* argument, int resourceId);
int __cdecl GetMonitorInfoCallback(HMONITOR monitor, HDC hdc, LPRECT rect, LPARAM lParam);
void __cdecl Usage(void);
void __cdecl wmain(int argc, wchar_t* argv[]);

bool __cdecl checkFlag(wchar_t* argument, int resourceId)
{
    wchar_t hyphenDash[3] = L"--";
    wchar_t singleSlash[2] = L"/";
    wchar_t singleHyphen[2] = L"-";
    
    wchar_t buffer1[256];
    wchar_t buffer2[256];
    
    wchar_t combined1[256] = L"-";
    wchar_t combined2[256] = L"-";
    wchar_t combined3[256] = L"/";
    wchar_t combined4[256] = L"/";
    wchar_t combined5[4] = L"--";
    
    LoadStringW(appHandle, resourceId, buffer1, 256);
    LoadStringW(appHandle, resourceId + 1, buffer2, 256);
    
    wcsncat_s(combined1, 256, buffer1, _TRUNCATE);
    wcsncat_s(combined2, 256, buffer2, _TRUNCATE);
    wcsncat_s(combined3, 256, buffer1, _TRUNCATE);
    wcsncat_s(combined4, 256, buffer2, _TRUNCATE);
    wcsncat_s(combined5, 4, buffer2, _TRUNCATE);
    
    if (_wcsicmp(argument, combined1) == 0 ||
        _wcsicmp(argument, combined2) == 0 ||
        _wcsicmp(argument, combined3) == 0 ||
        _wcsicmp(argument, combined4) == 0 ||
        _wcsicmp(argument, combined5) == 0) {
        return true;
    }
    
    return false;
}

int __cdecl GetMonitorInfoCallback(HMONITOR monitor, HDC hdc, LPRECT rect, LPARAM lParam)
{
    MONITORINFOEXW monitorInfo;
    memset(&monitorInfo, 0, sizeof(monitorInfo));
    monitorInfo.cbSize = sizeof(monitorInfo);
    
    DEVMODEA* currentMode = &currentModes[displayCount];
    memset(currentMode, 0, sizeof(DEVMODEA));
    currentMode->dmSize = sizeof(DEVMODEA);
    currentMode->dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY | DM_BITSPERPEL | DM_POSITION | DM_DISPLAYFLAGS | DM_DISPLAYORIENTATION;
    currentMode->dmPelsWidth = rect->right - rect->left;
    currentMode->dmPelsHeight = rect->bottom - rect->top;
    
    displayCount++;
    return 1;
}

void __cdecl Usage(void)
{
    wchar_t buffer[256];
    
    for (int resourceId = IDS_EXAMPLEPARAMS; resourceId < IDS_CHANGEREFRESHRATEARG; resourceId++) {
        LoadStringW(appHandle, resourceId, buffer, 256);
        wprintf_s(buffer);
    }
}

void __cdecl wmain(int argc, wchar_t* argv[])
{
    int argIndex = 1;
    int width = 0;
    int height = 0;
    int refreshRate = 0;
	int monitor = 0;
    bool widthSpecified = false;
    bool heightSpecified = false;
    bool refreshSpecified = false;
	bool monitorSpecified = false;
    
    // Set console to UTF-16 mode
    if (_setmode(_fileno(__acrt_iob_func(1)), _O_U16TEXT) == -1) {
        wprintf_s(L"Warning: Failed to set console mode via _setmode(_fileno(stdout), _O_U16TEXT \n");
    }
    
    // Set thread UI language
    if (SetThreadPreferredUILanguages(MUI_CONSOLE_FILTER, nullptr, nullptr) == 0) {
        wprintf_s(L"Warning: Call to SetThreadPreferredUILanguages(MUI_CONSOLE_FILTER, NULL, NULL) failed. \n");
    }
    
    appHandle = (HINSTANCE__*)GetModuleHandleA(nullptr);
    
    // Check for help arguments first
    if (argc >= 2) {
        if (checkFlag(argv[1], IDS_Q) || checkFlag(argv[1], IDS_HELP)) {
            Usage();
            return;
        }
    }
    
    if (argc < 2) {
        Usage();
        return;
    }
    
    // Parse command line arguments
    while (argIndex < argc) {
        if (argIndex < argc - 1) {
            if (checkFlag(argv[argIndex], IDS_W)) {
                width = _wtoi(argv[argIndex + 1]);
                widthSpecified = true;
                argIndex++; // Skip the value
            }
            else if (checkFlag(argv[argIndex], IDS_H)) {
                height = _wtoi(argv[argIndex + 1]);
                heightSpecified = true;
                argIndex++; // Skip the value
            }
            else {
                // Check if it's a flag without a value
                goto CheckSingleArgs;
            }
        }
        else {
CheckSingleArgs:
            if (checkFlag(argv[argIndex], IDS_I)) {
                // Enumerate displays
                currentModes = (DEVMODEA*)operator new(sizeof(DEVMODEA) * 32);
                displayCount = 0;
                EnumDisplayMonitors(nullptr, nullptr, (MONITORENUMPROC)GetMonitorInfoCallback, 0);
                
                // Display current resolutions
                for (int i = 0; i < displayCount; i++) {
                    wprintf_s(L"%dx%d\n", currentModes[i].dmPelsWidth, currentModes[i].dmPelsHeight);
                }
                return;
            }
            else if (checkFlag(argv[argIndex], IDS_F)) {
                force = true;
            }
            else if (checkFlag(argv[argIndex], IDS_Q) || checkFlag(argv[argIndex], IDS_HELP)) {
                Usage();
                return;
            }
            else {
                // Invalid argument
                wchar_t buffer[256];
                LoadStringW(appHandle, IDS_INVALIDARG, buffer, 256);
                wprintf_s(buffer, argv[argIndex]);
                Usage();
                return;
            }
        }
        argIndex++;
    }
    
    // Validate that if either width or height is specified, both must be specified
    if ((widthSpecified && !heightSpecified) || (!widthSpecified && heightSpecified)) {
        wchar_t buffer[256];
        LoadStringW(appHandle, IDS_HEIGHTANDWIDTHAREREQUIRED, buffer, 256);
        wprintf_s(buffer);
        return;
    }
    
    // If no width/height specified, just show current settings and exit
    if (!widthSpecified && !heightSpecified) {
        // Enumerate displays
        currentModes = (DEVMODEA*)operator new(sizeof(DEVMODEA) * 32);
        displayCount = 0;
        EnumDisplayMonitors(nullptr, nullptr, (MONITORENUMPROC)GetMonitorInfoCallback, 0);
        
        wchar_t buffer[256];
        LoadStringW(appHandle, IDS_CONNECTEDMONITORS, buffer, 256);
        wprintf_s(buffer);
        
        for (int i = 0; i < displayCount; i++) {
            wprintf_s(L"  %dx%d\n", currentModes[i].dmPelsWidth, currentModes[i].dmPelsHeight);
        }
        return;
    }
    
    // We have valid width and height, proceed with resolution change
    DEVMODEA* newSettings = (DEVMODEA*)operator new(sizeof(DEVMODEA));
    memset(newSettings, 0, sizeof(DEVMODEA));
    newSettings->dmSize = sizeof(DEVMODEA);
    newSettings->dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;
    newSettings->dmPelsWidth = width;
    newSettings->dmPelsHeight = height;
    
    if (refreshSpecified) {
        newSettings->dmFields |= DM_DISPLAYFREQUENCY;
        newSettings->dmDisplayFrequency = refreshRate;
    }
    
    // Display summary header
    wchar_t buffer[256];
    LoadStringW(appHandle, IDS_SUMMARYHEADER, buffer, 256);
    wprintf_s(buffer);
    
    LoadStringW(appHandle, IDS_NEWWIDTHSETTINGTOBEAPPLIED, buffer, 256);
    wprintf_s(buffer, width);
    
    LoadStringW(appHandle, IDS_NEWHEIGHTSETTINGTOBEAPPLIED, buffer, 256);
    wprintf_s(buffer, height);

    // Show warning unless force flag is set
    if (!force) {
        LoadStringW(appHandle, IDS_SCREENCANFLICKERPRESSN, buffer, 256);
        wprintf_s(buffer);
        LoadStringW(appHandle, IDS_SCREENCANFLICKERRESTARTPC, buffer, 256);
        wprintf_s(buffer);
    }
    
    // Apply the new settings
    int result = ChangeDisplaySettingsA(newSettings, 0);
    
    if (result == DISP_CHANGE_SUCCESSFUL) {
        DWORD flags = 0;
        DWORD successResourceId = IDS_NEWSETTINGSAVEDSUCCESSFULLY;
        
        if (!force) {
            LoadStringW(appHandle, IDS_WANNASAVENEWSETTINGS, buffer, 256);
            wprintf_s(buffer);
            
            wchar_t choiceBuffer[256];
            wchar_t commandBuffer[512];
            
            LoadStringW(appHandle, IDS_RANDOMPARAMS, choiceBuffer, 256);
            wcscpy_s(commandBuffer, 512, L"CHOICE ");
            wcscat_s(commandBuffer, 512, choiceBuffer);
            
            int choice = _wsystem(commandBuffer);
            if (choice == 1) {  // User pressed Y
                flags = CDS_UPDATEREGISTRY;
                successResourceId = IDS_NEWSETTINGSAVEDSUCCESSFULLY;
            } else {  // User pressed N or timed out
                flags = 0;
                successResourceId = IDS_NEWSETTINGSREVERTED;
            }
            
            // Revert or save based on user choice
            ChangeDisplaySettingsA(newSettings, flags);
        }
        
        LoadStringW(appHandle, successResourceId, buffer, 256);
        wprintf_s(buffer);
    }
    else if (result == DISP_CHANGE_BADMODE) {
        LoadStringW(appHandle, IDS_UNSUPPORTEDSETTINGSBYHARDWARE, buffer, 256);
        wprintf_s(buffer);
    }
    else {
        LoadStringW(appHandle, IDS_COULDNOTAPPLYTOHARDWARE, buffer, 256);
        wprintf_s(buffer);
    }
}

