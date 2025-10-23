#include <windows.h>
#include <winuser.h>
#include <winnls.h>
#include <wingdi.h>
#include <shellapi.h>
#include <shlwapi.h>

#define TRIBIT_UNDEFINED 2
#define TRIBIT_FALSE 0
#define TRIBIT_TRUE 1

static int s_tbBiDi = TRIBIT_UNDEFINED;

extern void WINAPI Control_RunDLL(HWND hwnd, HINSTANCE hinst, LPCWSTR lpszCmdLine, int nCmdShow);

int IsBiDiLocalizedSystemEx(USHORT* param_1);
BOOL CALLBACK Mirror_EnumUILanguagesProc(WCHAR* language, LONG param_2);
HWND Stub_CreateStubWindow(POINT position, HWND parent);
LRESULT CALLBACK StubWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, WCHAR* cmdLine, int nCmdShow);

int IsBiDiLocalizedSystemEx(USHORT* param_1)
{
    LANGID languageId;
    int localeInfoResult;
    WCHAR localeInfo[16];
    DWORD stackData;
    
    if (s_tbBiDi == TRIBIT_UNDEFINED) {
        languageId = GetUserDefaultUILanguage();
        if (languageId != 0) {
            localeInfoResult = GetLocaleInfoW(languageId, 0x58, localeInfo, 16);
            if (localeInfoResult != 0) {
                stackData = languageId;
                EnumUILanguagesW(Mirror_EnumUILanguagesProc, 0, (LONG_PTR)&stackData);
            }
        }
        s_tbBiDi = TRIBIT_FALSE;
    }
    return (s_tbBiDi == TRIBIT_TRUE);
}

BOOL CALLBACK Mirror_EnumUILanguagesProc(WCHAR* language, LONG param_2)
{
    WCHAR* currentChar = language;
    int value = 0;
    UINT charValue;
    BOOL continueEnum;
    
    while (*currentChar != L'\0') {
        charValue = (UINT)*currentChar;
        
        if (charValue < 0x30 || charValue > 0x39) {
            if (charValue > 0x60) {
                charValue = charValue - 0x20;
            }
            if ((charValue - 0x41) > 5) {
                continueEnum = (value != *(short*)param_2);
                if (!continueEnum) {
                    *(int*)(param_2 + 4) = 1;
                }
                return continueEnum;
            }
            value = value * 0x10 + (charValue - 0x37);
        } else {
            value = value * 0x10 + (charValue - 0x30);
        }
        currentChar = CharNextW(currentChar);
    }
    return TRUE;
}

HWND Stub_CreateStubWindow(POINT position, HWND parent)
{
    HWND hwnd;
    int i;
    DWORD windowStyle;
    int isBiDi;
    WCHAR className[20];
    WNDCLASSW wc;
    
    for (i = 0; i < 20; i++) {
        className[i] = L'\0';
    }
    
    wc.lpfnWndProc = StubWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 4;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hIcon = NULL;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = L"StubWindow32";
    wc.style = CS_HREDRAW | CS_VREDRAW;
    
    RegisterClassW(&wc);
    
    windowStyle = WS_OVERLAPPED;
    isBiDi = IsBiDiLocalizedSystemEx(NULL);
    if (isBiDi != 0) {
        windowStyle |= WS_EX_LAYOUTRTL;
    }
    
    hwnd = CreateWindowExW(
        windowStyle,
        L"StubWindow32",
        L"",
        WS_OVERLAPPEDWINDOW,
        position.x,
        position.y,
        0,
        0,
        parent,
        NULL,
        GetModuleHandle(NULL),
        NULL
    );
    
    return hwnd;
}

LRESULT CALLBACK StubWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LRESULT result;
    int messageResult;
    HICON icon;
    int windowData;
    DWORD processId;
    HWND childWindow;
    HWND nextWindow;
    WCHAR className[80];
    int compareResult;
    DWORD windowStyle;
    
    if (msg == WM_DESTROY) {
        icon = (HICON)SendMessageW(hwnd, 0x7F, 1, 0);
        if (icon != NULL) {
            DestroyIcon(icon);
        }
        
        windowData = GetWindowLongW(hwnd, 0);
        if (windowData != 0) {
            SetWindowLongW(hwnd, 0, 0);
            processId = GetCurrentProcessId();
            SHFreeShared((HANDLE)windowData, processId);
        }
        result = 0;
    } else if (msg == 0x1C) {
        childWindow = GetWindow(hwnd, GW_CHILD);
        memset(className, 0, sizeof(className));
        GetClassNameW(childWindow, className, 80);
        
        CharUpperBuffW(className, 80);
        compareResult = lstrcmpW(className, L"IME");
        if (compareResult == 0) {
            childWindow = GetWindow(childWindow, GW_CHILD);
        }
        
        nextWindow = GetWindow(childWindow, GW_HWNDNEXT);
        if (nextWindow == hwnd) {
            windowStyle = GetWindowLongW(childWindow, GWL_EXSTYLE);
            if ((windowStyle & 0x40080) == 0) {
                SetWindowLongW(childWindow, GWL_EXSTYLE, windowStyle | 0x40000);
            }
        }
        result = 0;
    } else {
        result = DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    
    return result;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, WCHAR* cmdLine, int nCmdShow)
{
    HWND stubWindow;
    POINT position = {0x80000000, 0x80000000};
    
    stubWindow = Stub_CreateStubWindow(position, NULL);
    if (stubWindow != NULL) {
        ShellExecuteW(NULL, L"open", L"rundll32.exe", L"shell32.dll,Control_RunDLL iscsicpl.dll,,0", NULL, SW_SHOW);
        DestroyWindow(stubWindow);
    }
    
    return 0;
}

