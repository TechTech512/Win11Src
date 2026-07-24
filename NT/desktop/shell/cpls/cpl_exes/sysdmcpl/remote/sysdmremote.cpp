#include <windows.h>

// Function declaration for DisplaySYSDMCPL (assuming wide char version)
extern void __cdecl DisplaySYSDMCPL(wchar_t *param_1);

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    // Use wide string literal with L prefix
    DisplaySYSDMCPL(L"@remotepg.dll,527");
    return 0;
}

