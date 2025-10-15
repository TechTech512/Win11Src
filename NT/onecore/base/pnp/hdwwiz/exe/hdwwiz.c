#include <windows.h>

// External function declarations
extern int pSetupInitializeUtils(void);
extern void pSetupUninitializeUtils(void);

int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    FARPROC pAddHardwareWizard;
    HMODULE hLibrary;
    DWORD errorCode = 0;

    if (pSetupInitializeUtils() != 0) {
        hLibrary = LoadLibraryW(L"hdwwiz.cpl");
        if (hLibrary != NULL) {
            pAddHardwareWizard = GetProcAddress(hLibrary, "AddHardwareWizard");
            if (pAddHardwareWizard == NULL) {
                errorCode = GetLastError();
            } else {
                pAddHardwareWizard(NULL, NULL);
            }
            FreeLibrary(hLibrary);
        } else {
            errorCode = GetLastError();
        }
    } else {
        errorCode = GetLastError();
    }

    pSetupUninitializeUtils();
    ExitProcess(errorCode);
    
    return 0;
}

