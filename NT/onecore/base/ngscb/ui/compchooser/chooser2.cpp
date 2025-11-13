#include <windows.h>
#include <objbase.h>
#include <shlobj.h>
#include <objsel.h>
#include <dsclient.h>
#include <guiddef.h>
#include <lm.h>
#include "resource.h"

// Global variables
HINSTANCE g_hInstance = NULL;
CLIPFORMAT g_cfDsObjectPicker = 0;

struct IDsObjectPicker;

// Function prototypes
HRESULT CHOOSER2_InitObjectPickerForComputers(IDsObjectPicker* pObjectPicker);
HRESULT CHOOSER2_ProcessSelectedObjects(IDataObject* pDataObject, wchar_t** ppComputerName);
HRESULT CHOOSER2_ComputerNameFromObjectPicker(HWND hWnd, wchar_t** ppComputerName);
BOOL CHOOSER2_PickTargetComputer(HWND hWnd, wchar_t** ppComputerName);
INT_PTR CALLBACK CHOOSER2_TargetComputerDialogFunc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

HRESULT CHOOSER2_ComputerNameFromObjectPicker(HWND hWnd, wchar_t** ppComputerName)
{
    HRESULT hr = S_OK;
    IDsObjectPicker* pObjectPicker = NULL;
    IDataObject* pDataObject = NULL;
    
    if (ppComputerName == NULL) {
        return E_INVALIDARG;
    }
    
    *ppComputerName = NULL;
    
    // Create DS Object Picker instance
    hr = CoCreateInstance(CLSID_DsObjectPicker, NULL, CLSCTX_INPROC_SERVER, 
                         IID_IDsObjectPicker, (void**)&pObjectPicker);
    if (FAILED(hr)) {
        return hr;
    }
    
    // Initialize object picker for computers
    hr = CHOOSER2_InitObjectPickerForComputers(pObjectPicker);
    if (SUCCEEDED(hr)) {
        // Invoke the object picker dialog
        hr = pObjectPicker->InvokeDialog(hWnd, &pDataObject);
        if (hr == S_OK) {
            // Process the selected objects
            hr = CHOOSER2_ProcessSelectedObjects(pDataObject, ppComputerName);
        }
        
        if (pDataObject) {
            pDataObject->Release();
        }
    }
    
    if (pObjectPicker) {
        pObjectPicker->Release();
    }
    
    return hr;
}

HRESULT CHOOSER2_InitObjectPickerForComputers(IDsObjectPicker* pObjectPicker)
{
    if (pObjectPicker == NULL) {
        return E_INVALIDARG;
    }
    
    DSOP_SCOPE_INIT_INFO scopeInfo;
    DSOP_INIT_INFO initInfo;
    
    ZeroMemory(&scopeInfo, sizeof(scopeInfo));
    ZeroMemory(&initInfo, sizeof(initInfo));
    
    // Set up scope to look for computers
    scopeInfo.cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
    scopeInfo.flType = DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN | 
                      DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN |
                      DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN |
                      DSOP_SCOPE_TYPE_GLOBAL_CATALOG |
                      DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN |
                      DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN |
                      DSOP_SCOPE_TYPE_WORKGROUP |
                      DSOP_SCOPE_TYPE_USER_ENTERED_UPLEVEL_SCOPE |
                      DSOP_SCOPE_TYPE_USER_ENTERED_DOWNLEVEL_SCOPE;
    scopeInfo.flScope = DSOP_SCOPE_FLAG_STARTING_SCOPE;
    scopeInfo.FilterFlags.Uplevel.flBothModes = DSOP_FILTER_COMPUTERS;
    scopeInfo.FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_COMPUTERS;
    
    // Set up initialization info
    initInfo.cbSize = sizeof(DSOP_INIT_INFO);
    initInfo.pwzTargetComputer = NULL;
    initInfo.cDsScopeInfos = 1;
    initInfo.aDsScopeInfos = &scopeInfo;
    initInfo.flOptions = 0;
    
    return pObjectPicker->Initialize(&initInfo);
}

HRESULT CHOOSER2_ProcessSelectedObjects(IDataObject* pDataObject, wchar_t** ppComputerName)
{
    HRESULT hr = S_OK;
    FORMATETC formatetc;
    STGMEDIUM stgmedium;
    PDS_SELECTION_LIST pSelectionList = NULL;
    
    if (pDataObject == NULL || ppComputerName == NULL) {
        return E_INVALIDARG;
    }
    
    // Register clipboard format if not already done
    if (g_cfDsObjectPicker == 0) {
        g_cfDsObjectPicker = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_DSOP_DS_SELECTION_LIST);
    }
    
    // Set up data format request
    ZeroMemory(&formatetc, sizeof(formatetc));
    ZeroMemory(&stgmedium, sizeof(stgmedium));
    
    formatetc.cfFormat = (CLIPFORMAT)g_cfDsObjectPicker;
    formatetc.ptd = NULL;
    formatetc.dwAspect = DVASPECT_CONTENT;
    formatetc.lindex = -1;
    formatetc.tymed = TYMED_HGLOBAL;
    
    // Get the data from the data object
    hr = pDataObject->GetData(&formatetc, &stgmedium);
    if (FAILED(hr)) {
        return hr;
    }
    
    // Lock the global memory and get selection list
    pSelectionList = (PDS_SELECTION_LIST)GlobalLock(stgmedium.hGlobal);
    if (pSelectionList == NULL) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        ReleaseStgMedium(&stgmedium);
        return hr;
    }
    
    // Process the selection
    if (pSelectionList->cItems == 1) {
        PDS_SELECTION pSelection = &pSelectionList->aDsSelection[0];
        
        // Check if we have a DNS name (preferred) or NetBIOS name
        if (pSelection->pwzUPN != NULL && pSelection->pwzUPN[0] != L'\0') {
            // Use the distinguished name
            *ppComputerName = SysAllocString(pSelection->pwzUPN);
        } else if (pSelection->pwzName != NULL && pSelection->pwzName[0] != L'\0') {
            // Use the display name
            *ppComputerName = SysAllocString(pSelection->pwzName);
        } else {
            hr = E_UNEXPECTED;
        }
        
        // Try to get server info to validate computer
        if (*ppComputerName != NULL) {
            LPSERVER_INFO_101 pServerInfo = NULL;
            NET_API_STATUS netStatus = NetServerGetInfo(*ppComputerName, 101, (LPBYTE*)&pServerInfo);
            
            if (netStatus == NERR_Success) {
                // Computer is valid and accessible
                if (pServerInfo != NULL) {
                    NetApiBufferFree(pServerInfo);
                }
            } else {
                // Try with the other name format
                if (pSelection->pwzName != NULL && wcscmp(*ppComputerName, pSelection->pwzName) != 0) {
                    wchar_t* tempName = SysAllocString(pSelection->pwzName);
                    netStatus = NetServerGetInfo(tempName, 101, (LPBYTE*)&pServerInfo);
                    
                    if (netStatus == NERR_Success) {
                        // Switch to the working name
                        SysFreeString(*ppComputerName);
                        *ppComputerName = tempName;
                        if (pServerInfo != NULL) {
                            NetApiBufferFree(pServerInfo);
                        }
                    } else {
                        SysFreeString(tempName);
                    }
                }
            }
        }
    } else {
        hr = E_UNEXPECTED; // Expected exactly one selection
    }
    
    GlobalUnlock(stgmedium.hGlobal);
    ReleaseStgMedium(&stgmedium);
    
    return hr;
}

BOOL CHOOSER2_PickTargetComputer(HWND hWnd, wchar_t** ppComputerName)
{
    if (ppComputerName == NULL) {
        return FALSE;
    }
    
    // Use MAKEINTRESOURCEW to ensure wide string for dialog template
    INT_PTR result = DialogBoxParamW(g_hInstance, 
                                    MAKEINTRESOURCEW(IDD_SELECTCOMPUTER), 
                                    hWnd, 
                                    CHOOSER2_TargetComputerDialogFunc, 
                                    (LPARAM)ppComputerName);
    return (result > 0);
}

INT_PTR CALLBACK CHOOSER2_TargetComputerDialogFunc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static wchar_t** s_ppComputerName = NULL;
    
    switch (uMsg) {
        case WM_INITDIALOG:
            s_ppComputerName = (wchar_t**)lParam;
            if (s_ppComputerName != NULL) {
                *s_ppComputerName = NULL;
            }
            
            // Set default to local computer
            CheckRadioButton(hwndDlg, IDC_CURRENTPC, IDC_ANOTHERPC, IDC_CURRENTPC);
            
            // Disable computer name controls initially
            EnableWindow(GetDlgItem(hwndDlg, IDC_PCNAMETEXTBOX), FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_BROWSECOMPUTERS), FALSE);
            
            // Set focus to OK button
            SetFocus(GetDlgItem(hwndDlg, IDOK));
            return FALSE; // Didn't set focus
            
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_CURRENTPC:
                    // Local computer selected - disable name entry
                    EnableWindow(GetDlgItem(hwndDlg, IDC_PCNAMETEXTBOX), FALSE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_BROWSECOMPUTERS), FALSE);
                    break;
                    
                case IDC_ANOTHERPC:
                    // Another computer selected - enable name entry
                    EnableWindow(GetDlgItem(hwndDlg, IDC_PCNAMETEXTBOX), TRUE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_BROWSECOMPUTERS), TRUE);
                    SetFocus(GetDlgItem(hwndDlg, IDC_PCNAMETEXTBOX));
                    break;
                    
                case IDC_BROWSECOMPUTERS:
                    // Browse for computers
                    if (s_ppComputerName != NULL) {
                        wchar_t* computerName = NULL;
                        HRESULT hr = CHOOSER2_ComputerNameFromObjectPicker(hwndDlg, &computerName);
                        if (SUCCEEDED(hr) && computerName != NULL) {
                            SetDlgItemTextW(hwndDlg, IDC_PCNAMETEXTBOX, computerName);
                            SysFreeString(computerName);
                        }
                    }
                    break;
                    
                case IDOK:
                    if (s_ppComputerName != NULL) {
                        if (IsDlgButtonChecked(hwndDlg, IDC_ANOTHERPC) == BST_CHECKED) {
                            // Get computer name from text box
                            wchar_t computerName[MAX_PATH] = {0};
                            GetDlgItemTextW(hwndDlg, IDC_PCNAMETEXTBOX, computerName, MAX_PATH);
                            
                            // Remove leading backslashes if present
                            wchar_t* namePtr = computerName;
                            while (*namePtr == L'\\') {
                                namePtr++;
                            }
                            
                            if (*namePtr != L'\0') {
                                *s_ppComputerName = SysAllocString(namePtr);
                            }
                        }
                        // If local computer selected or no name entered, *s_ppComputerName remains NULL
                    }
                    EndDialog(hwndDlg, 1);
                    break;
                    
                case IDCANCEL:
                    EndDialog(hwndDlg, 0);
                    break;
                    
                default:
                    return FALSE;
            }
            break;
            
        case WM_HELP:
            // Show help
            if (lParam != NULL) {
                WinHelpW((HWND)((LPHELPINFO)lParam)->hItemHandle, L"chooser.hlp", HELP_WM_HELP, (ULONG_PTR)NULL);
            }
            break;
            
        case WM_CONTEXTMENU:
            // Show context help
            WinHelpW((HWND)wParam, L"chooser.hlp", HELP_CONTEXTMENU, (ULONG_PTR)NULL);
            break;
            
        default:
            return FALSE;
    }
    
    return TRUE;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        g_hInstance = hinstDLL;
    }
    return TRUE;
}

