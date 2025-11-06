#include <windows.h>

typedef struct _FMIFSLib {
    HMODULE hModule;
    FARPROC ChkdskEx;
    FARPROC EnableVolumeCompression;
    FARPROC FormatEx2;
    FARPROC GetDefaultFileSystem;
    FARPROC QueryCorruptionState;
} FMIFSLib;

unsigned char __cdecl LoadFMIFS(FMIFSLib* param_1)
{
    if (param_1 != NULL) {
        HMODULE hModule = LoadLibraryW(L"FMIFS.DLL");
        param_1->hModule = hModule;
        if (hModule != NULL) {
            FARPROC proc = GetProcAddress(hModule, "ChkdskEx");
            param_1->ChkdskEx = proc;
            if (proc != NULL) {
                proc = GetProcAddress(hModule, "EnableVolumeCompression");
                param_1->EnableVolumeCompression = proc;
                if (proc != NULL) {
                    proc = GetProcAddress(hModule, "FormatEx2");
                    param_1->FormatEx2 = proc;
                    if (proc != NULL) {
                        proc = GetProcAddress(hModule, "GetDefaultFileSystem");
                        param_1->GetDefaultFileSystem = proc;
                        if (proc != NULL) {
                            proc = GetProcAddress(hModule, "QueryCorruptionState");
                            param_1->QueryCorruptionState = proc;
                            if (param_1->GetDefaultFileSystem != NULL) {
                                return 1;
                            }
                        }
                    }
                }
            }
        }
    }
    if ((param_1 != NULL) && (param_1->hModule != NULL)) {
        if (FreeLibrary(param_1->hModule)) {
            param_1->ChkdskEx = NULL;
            param_1->FormatEx2 = NULL;
            param_1->GetDefaultFileSystem = NULL;
        }
    }
    return 0;
}

