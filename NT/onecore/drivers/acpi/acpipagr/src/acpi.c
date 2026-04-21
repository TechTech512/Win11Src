#pragma warning (disable:4996)

#include "acpi.h"

// GUID for ACPI interface standard from hex dump
// {B091A08A-BA97-11D0-BD14-00AABB7B2A??}
EXTERN_C const GUID GUID_ACPI_INTERFACE_STANDARD = 
    { 0xb091a08a, 0xba97, 0x11d0, { 0xbd, 0x14, 0x00, 0xaa, 0x00, 0xb7, 0xb3, 0x2a } };

NTSTATUS AdAcpiAcquireInterfaces(PAD_EXTENSION Extension);
NTSTATUS AdAcpiEval_OST(PAD_EXTENSION Extension, ULONG Param2, ULONG Param3);
NTSTATUS AdAcpiEval_PUR(PAD_EXTENSION Extension, PULONG Result);
NTSTATUS AdAcpiEvaluateMethod(PAD_EXTENSION Extension, PVOID Buffer, ULONG BufferSize, PVOID Result, ULONG ResultSize, PULONG Param6);
void AdAcpiNotifyCallback(PVOID Context, ULONG NotificationType);
void AdInsertWorkItem(PAD_EXTENSION Extension, ULONG WorkType);

// Acquire ACPI interfaces
NTSTATUS AdAcpiAcquireInterfaces(
    PAD_EXTENSION Extension
)
{
    NTSTATUS status;
    
    status = (*(long (__cdecl *)(void *, void *, void *, void *, int, int, int))WdfFunctions[0x83])(
        WdfDriverGlobals,
        Extension->WdfSelf,
        (PGUID)&GUID_ACPI_INTERFACE_STANDARD,
        &Extension->AcpiInterfaces,
        0x2c,
        1,
        0
    );
    
    if (NT_SUCCESS(status))
    {
        status = ((NTSTATUS (__cdecl *)(PVOID, PVOID))Extension->AcpiInterfaces.RegisterForDeviceNotifications)(
            Extension->AcpiInterfaces.Context,
            AdAcpiNotifyCallback
        );
        
        if (!NT_SUCCESS(status))
        {
            if (Extension->AcpiInterfaces.InterfaceDereference)
            {
                ((void (__cdecl *)(PVOID))Extension->AcpiInterfaces.InterfaceDereference)(Extension->AcpiInterfaces.Context);
            }
            Extension->AcpiInterfaces.RegisterForDeviceNotifications = 0;
            Extension->AcpiInterfaces.InterfaceDereference = 0;
        }
    }
    
    return status;
}

// Evaluate OST method
NTSTATUS AdAcpiEval_OST(
    PAD_EXTENSION Extension,
    ULONG Param2,
    ULONG Param3
)
{
    NTSTATUS status;
    PUCHAR evalBuffer;
    ULONG bufferSize = 0x28;
    ULONG param6 = 1;
    
    evalBuffer = (PUCHAR)ExAllocatePoolWithTag(NonPagedPool, 0x28, 'ADPT');
    
    if (!evalBuffer)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlZeroMemory(evalBuffer, 0x28);
    
    *(PULONG)(evalBuffer + 0x00) = 0x43696541;
    *(PULONG)(evalBuffer + 0x04) = 0x54534f5f;
    *(PULONG)(evalBuffer + 0x08) = 0x28;
    *(PULONG)(evalBuffer + 0x0c) = 3;
    *(USHORT*)(evalBuffer + 0x10) = 0;
    *(USHORT*)(evalBuffer + 0x12) = 0;
    *(PULONG)(evalBuffer + 0x1c) = (Param2 == 0);
    *(USHORT*)(evalBuffer + 0x20) = 2;
    *(PAD_EXTENSION*)(evalBuffer + 0x24) = Extension;
    *(USHORT*)(evalBuffer + 0x12) = 4;
    *(USHORT*)(evalBuffer + 0x1a) = 4;
    *(USHORT*)(evalBuffer + 0x22) = 4;
    *(PULONG)(evalBuffer + 0x14) = 0x80;
    
    status = AdAcpiEvaluateMethod(Extension, evalBuffer, 0x14, NULL, param6, &bufferSize);
    
    ExFreePoolWithTag(evalBuffer, 'ADPT');
    
    return status;
}

// Evaluate PUR method
NTSTATUS AdAcpiEval_PUR(
    PAD_EXTENSION Extension,
    PULONG Result
)
{
    NTSTATUS status;
    PUCHAR evalBuffer;
    ULONG resultValue;
    ULONG bufferSize = 0x1c;
    ULONG param6 = 1;
    
    evalBuffer = (PUCHAR)ExAllocatePoolWithTag(NonPagedPool, 0x1c, 'ADPT');
    
    if (!evalBuffer)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlZeroMemory(evalBuffer, 0x1c);
    
    status = AdAcpiEvaluateMethod(Extension, evalBuffer, 0x1c, &resultValue, param6, &bufferSize);
    
    if (NT_SUCCESS(status))
    {
        if (resultValue == 0)
        {
            status = STATUS_DEVICE_BUSY;
        }
        else if (*(PULONG)(evalBuffer + 8) == 2)
        {
            if (*(USHORT*)(evalBuffer + 12) == 0)
            {
                if (*(PULONG)(evalBuffer + 16) > 1)
                {
                    status = STATUS_UNSUCCESSFUL;
                }
                else if (*(USHORT*)(evalBuffer + 20) == 0)
                {
                    *Result = *(PULONG)(evalBuffer + 24);
                }
                else
                {
                    status = STATUS_INVALID_PARAMETER;
                }
            }
            else
            {
                status = STATUS_INVALID_PARAMETER;
            }
        }
        else
        {
            status = STATUS_DEVICE_BUSY;
        }
    }
    
    ExFreePoolWithTag(evalBuffer, 'ADPT');
    
    return status;
}

// Evaluate ACPI method
NTSTATUS AdAcpiEvaluateMethod(
    PAD_EXTENSION Extension,
    PVOID Buffer,
    ULONG BufferSize,
    PVOID Result,
    ULONG ResultSize,
    PULONG Param6
)
{
    NTSTATUS status;
    ULONG reserved[2];
    
    reserved[0] = 1;
    
    status = (*(long (__cdecl *)(void *, void *, int, int, void *, void *, int, void *))WdfFunctions[0xba])(
        WdfDriverGlobals,
        Extension->IoTarget,
        0,
        0x32c004,
        reserved,
        &Buffer,
        0,
        &Buffer
    );
    
    if (Result)
    {
        *(PVOID*)Result = Buffer;
    }
    
    return status;
}

// ACPI notify callback
void AdAcpiNotifyCallback(
    PVOID Context,
    ULONG NotificationType
)
{
    PAD_EXTENSION Extension = (PAD_EXTENSION)Context;
    
    if (NotificationType == 0x80)
    {
        AdInsertWorkItem(Extension, 0x80);
    }
}

