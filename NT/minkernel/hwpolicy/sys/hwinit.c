#include <ntddk.h>

NTSTATUS
DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath
);

VOID
DriverUnload(
    PDRIVER_OBJECT DriverObject
);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,DriverEntry)

#pragma alloc_text(PAGE,DriverUnload)
#endif // ALLOC_PRAGMA

// DllInitialize - DLL initialization routine
NTSTATUS
DllInitialize(
    PUNICODE_STRING RegistryPath
)
{
    return STATUS_SUCCESS;
}

// DllUnload - DLL unload routine
NTSTATUS
DllUnload(VOID)
{
    return STATUS_SUCCESS;
}

// DriverUnload - Driver unload routine
VOID
DriverUnload(
    PDRIVER_OBJECT DriverObject
)
{
    return;
}

// DriverEntry - Driver entry point
NTSTATUS
DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath
)
{
    DriverObject->DriverUnload = DriverUnload;
    return STATUS_NOT_IMPLEMENTED;
}

