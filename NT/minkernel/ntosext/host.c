#pragma warning (disable:4716)

#include <ntddk.h>

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {}

__declspec(dllexport) long __cdecl DllUnload(void)

{
                    /* 0x4006  1  DllUnload */
  return 0;
}
