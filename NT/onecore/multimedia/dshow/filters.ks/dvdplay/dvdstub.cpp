#include <windows.h>
#include <winreg.h>
#include <strsafe.h>
#include <memory.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    HKEY hKey;
    DWORD dwType = 0;
    DWORD dwSize = MAX_PATH;
    WCHAR szPath[MAX_PATH];
    WCHAR szPlayerPath[MAX_PATH];
    WCHAR szExePath[MAX_PATH];
    WCHAR szCmdArgs[] = L" /device:dvd";
    LPWSTR pFilePart;
    
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    
    // Initialize structures
    SecureZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    SecureZeroMemory(&pi, sizeof(pi));
    
    SecureZeroMemory(szPath, sizeof(szPath));
    SecureZeroMemory(szPlayerPath, sizeof(szPlayerPath));
    SecureZeroMemory(szExePath, sizeof(szExePath));
    
    // Get Windows Media Player path from registry
    DWORD dwResult = RegGetValueW(HKEY_LOCAL_MACHINE,
                                  L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\wmplayer.exe",
                                  L"Path",
                                  RRF_RT_ANY,
                                  &dwType,
                                  szPath,
                                  &dwSize);
    
    if (dwResult == ERROR_SUCCESS && (dwType == REG_SZ || dwType == REG_EXPAND_SZ)) {
        // Search for wmplayer.exe in the path we found
        if (SearchPathW(szPath, L"wmplayer", L".exe", MAX_PATH, szExePath, &pFilePart) != 0) {
            // Create process with DVD device argument
            CreateProcessW(szExePath,
                          szCmdArgs,
                          NULL,
                          NULL,
                          FALSE,
                          0,
                          NULL,
                          NULL,
                          &si,
                          &pi);
            
            // Close process handles if created
            if (pi.hProcess) CloseHandle(pi.hProcess);
            if (pi.hThread) CloseHandle(pi.hThread);
        }
    }
    
    return 0;
}

