#include <windows.h>
#include "resource.h"

HINSTANCE g_hDll;
DWORD NETUI_Elimination_Dialog(HWND hWnd, UINT uMessageID, UINT uTitleID);

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hInstance);
        g_hDll = hInstance;
    }
    return TRUE;
}

void EditAuditInfo(HWND hWnd)
{
    NETUI_Elimination_Dialog(hWnd, IDS_CANNOTMODIFYAUDIT, IDS_WINDOWS);
}

void EditOwnerInfo(HWND hWnd)
{
    NETUI_Elimination_Dialog(hWnd, IDS_CANNOTTAKEOWNERSHIP, IDS_WINDOWS);
}

void EditPermissionInfo(HWND hWnd)
{
    NETUI_Elimination_Dialog(hWnd, IDS_CANNOTEDITPERMS, IDS_WINDOWS);
}

long FMExtensionProcW(HWND hWnd, USHORT uMsg, long lParam)
{
    return 0;
}

DWORD NETUI_Elimination_Dialog(HWND hWnd, UINT uMessageID, UINT uTitleID)
{
    WCHAR szMessage[512];
    WCHAR szTitle[256];
    
    LoadStringW(g_hDll, uMessageID, szMessage, sizeof(szMessage)/sizeof(WCHAR));
    LoadStringW(g_hDll, uTitleID, szTitle, sizeof(szTitle)/sizeof(WCHAR));
    
    MessageBoxW(hWnd, szMessage, szTitle, MB_ICONERROR | MB_OK);
    
    return 0;
}

DWORD SedDiscretionaryAclEditor(HWND hWnd,
                               void* pObject,
                               WCHAR* pObjectName,
                               void* pObjectType,
                               void* pAccesses,
                               WCHAR* pServer,
                               void* pfnCallback,
                               DWORD dwCallbackContext,
                               void* pApplyToSubObjects,
                               BYTE bContainer,
                               BYTE bCreateNewPage,
                               DWORD* pdwStatus,
                               DWORD dwFlags)
{
    NETUI_Elimination_Dialog(hWnd, IDS_CANNOTEDITPERMS, IDS_WINDOWS);
    if (pdwStatus) {
        *pdwStatus = 4;
    }
    return 0;
}

DWORD SedSystemAclEditor(HWND hWnd,
                        void* pObject,
                        WCHAR* pObjectName,
                        void* pObjectType,
                        void* pAccesses,
                        WCHAR* pServer,
                        void* pfnCallback,
                        DWORD dwCallbackContext,
                        void* pApplyToSubObjects,
                        BYTE bCreateNewPage,
                        DWORD* pdwStatus,
                        DWORD dwFlags)
{
    NETUI_Elimination_Dialog(hWnd, IDS_CANNOTMODIFYAUDIT, IDS_WINDOWS);
    if (pdwStatus) {
        *pdwStatus = 4;
    }
    return 0;
}

DWORD SedTakeOwnership(HWND hWnd,
                      void* pObject,
                      WCHAR* pObjectName,
                      WCHAR* pServer,
                      WCHAR* pPageTitle,
                      DWORD dwFlags,
                      void* pfnCallback,
                      DWORD dwCallbackContext,
                      void* pApplyToSubObjects,
                      BYTE bContainer,
                      BYTE bCreateNewPage,
                      DWORD* pdwStatus,
                      void* pHelpInfo,
                      DWORD dwHelpContext)
{
    NETUI_Elimination_Dialog(hWnd, IDS_CANNOTTAKEOWNERSHIP, IDS_WINDOWS);
    if (pdwStatus) {
        *pdwStatus = 4;
    }
    return 0;
}

