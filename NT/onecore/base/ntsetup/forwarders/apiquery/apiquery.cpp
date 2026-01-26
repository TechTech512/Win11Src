#pragma warning (disable:4005)

#include <windows.h>
#include <winternl.h>
#include <ntstatus.h>

extern "C" {
    NTSTATUS NTAPI LdrLoadDll(
        PCWSTR DllPath,
        ULONG Flags,
        PUNICODE_STRING DllName,
        PHANDLE DllHandle
    );

    NTSTATUS NTAPI LdrUnloadDll(
        HANDLE DllHandle
    );
}

long __cdecl ApiSetQueryApiSetPresence(PUNICODE_STRING ModuleName, unsigned char* PresenceFlag)
{
    NTSTATUS LoadStatus;
    NTSTATUS UnloadStatus;
    NTSTATUS FinalStatus;
    HANDLE ModuleHandle;

    if (ModuleName == NULL || PresenceFlag == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    LoadStatus = LdrLoadDll(NULL, 0, ModuleName, &ModuleHandle);

    if ((LoadStatus >= 0) || (LoadStatus == STATUS_DLL_NOT_FOUND))
    {
        *PresenceFlag = (LoadStatus >= 0) ? 1 : 0;
        FinalStatus = STATUS_SUCCESS;

        if (LoadStatus >= 0)
        {
            UnloadStatus = LdrUnloadDll(ModuleHandle);
            FinalStatus = UnloadStatus;
        }
    }
    else
    {
        FinalStatus = LoadStatus;
    }

    return FinalStatus;
}

