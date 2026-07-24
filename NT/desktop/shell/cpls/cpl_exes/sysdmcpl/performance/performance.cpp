#include <windows.h>

// Function declaration for DisplaySYSDMCPL (assuming wide char version)
extern void __cdecl DisplaySYSDMCPL(wchar_t *param_1);

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    int StringOrdinal = 0;
	wchar_t *SYSDMParam;
	
	StringOrdinal = CompareStringOrdinal(lpCmdLine,0xffffffff,L"/pagefile",0xffffffff,1);
	if (StringOrdinal == 2) {
		SYSDMParam = L"PAGEFILE";
	} else {
		SYSDMParam = L"-1";
	}
    DisplaySYSDMCPL(SYSDMParam);
    return 0;
}

