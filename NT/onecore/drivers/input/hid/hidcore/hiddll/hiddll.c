/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    HIDDLL.C

Abstract:

    This module contains the PUBLIC definitions for the
    code that implements the HID dll.

Environment:

    Kernel & user mode

--*/


#include <windows.h>
#include <stdlib.h>
#include <hidclass.h>
#include <winternl.h>
#include <hidsdi.h>

// Forward declaration
int __cdecl HidD_Hello(char* buffer, int size);

// DllMain
int DllMain(void *hInstance, ULONG dwReason, void *lpReserved)
{
    return TRUE;
}

// HidD_FlushQueue
_Success_(return==TRUE)
BOOLEAN __stdcall
HidD_FlushQueue (
   _In_    HANDLE                HidDeviceObject
   )
/*++
Routine Description:
    Flush the input queue for the given HID device.

Arguments:
   HidDeviceObject A handle to a Hid Device that the client obtains using 
                   a call to CreateFile on a valid Hid device string name.
                   The string name can be obtained using standard PnP calls.

Return Value:
   TRUE if successful
   FALSE otherwise  -- Use GetLastError() to get extended error information
--*/
{
    DWORD bytesReturned;
    return DeviceIoControl(HidDeviceObject, 0xb0197, NULL, 0, NULL, 0, &bytesReturned, NULL) != 0;
}

// HidD_FreePreparsedData
_Success_(return==TRUE)
BOOLEAN __stdcall
HidD_FreePreparsedData (
   _In_ __drv_freesMem(Mem) PHIDP_PREPARSED_DATA PreparsedData
   )
{
    // The actual allocation has a 16-byte header before the returned pointer
    void** allocationHeader = (void**)PreparsedData - 1;
    BOOL isValid = (*allocationHeader == HidD_Hello);
    if (isValid)
        LocalFree(allocationHeader);
    return (unsigned char)isValid;
}

// HidD_GetAttributes
_Must_inspect_result_
_Success_(return==TRUE)
BOOLEAN __stdcall
HidD_GetAttributes (
    _In_  HANDLE              HidDeviceObject,
    _Out_ PHIDD_ATTRIBUTES    Attributes
    )
/*++
Routine Description:
    Fill in the given HIDD_ATTRIBUTES structure with the attributes of the
    given hid device.

--*/
{
    DWORD bytesReturned;
    BYTE buffer[12];
    
    BOOL success = DeviceIoControl(HidDeviceObject, 0xb01a8, NULL, 0, buffer, 12, &bytesReturned, NULL);
    if (success)
    {
        Attributes->VendorID = *(unsigned short*)(buffer + 6);
        Attributes->ProductID = *(unsigned short*)(buffer + 4);
        Attributes->VersionNumber = *(unsigned short*)(buffer + 2);
        Attributes->Size = 12;
    }
    return (unsigned char)success;
}

// HidD_GetConfiguration
_Must_inspect_result_
_Success_(return==TRUE)
BOOLEAN __stdcall
HidD_GetConfiguration (
   _In_   HANDLE               HidDeviceObject,
   _Out_writes_bytes_(ConfigurationLength) PHIDD_CONFIGURATION Configuration,
   _In_   ULONG                ConfigurationLength
   )
/*++
Routine Description:
    Get the configuration information for this Hid device

Arguments:
   HidDeviceObject      A handle to a Hid Device Object.

   Configuration        A configuration structure.  HidD_GetConfiguration MUST
                        be called before the configuration can be modified and
                        set using HidD_SetConfiguration

   ConfigurationLength  That is ``sizeof (HIDD_CONFIGURATION)''. Using this
                        parameter, we can later increase the length of the 
                        configuration array and not break older apps.

Return Value:
   TRUE if successful
   FALSE otherwise  -- Use GetLastError() to get extended error information
--*/
{
    DWORD bytesReturned;
    
    if (ConfigurationLength < 12)
    {
        SetLastError(0x57);
        return 0;
    }
    
    Configuration->cookie = HidD_GetConfiguration;
    return DeviceIoControl(HidDeviceObject, 0xb0190, NULL, 0, &Configuration->size, ConfigurationLength - 4, &bytesReturned, NULL) != 0;
}

// HidD_GetFeature
_Must_inspect_result_
_Success_(return==TRUE)
BOOLEAN __stdcall
HidD_GetFeature (
   _In_    HANDLE   HidDeviceObject,
   _Out_writes_bytes_(ReportBufferLength) PVOID ReportBuffer,
   _In_    ULONG    ReportBufferLength
   )
/*++
Routine Description:
    Retrieve a feature report from a HID device.

Arguments:
    HidDeviceObject      A handle to a Hid Device Object.
 
    ReportBuffer         The buffer that the feature report should be placed 
                         into.  The first byte of the buffer should be set to
                         the report ID of the desired report
 
    ReportBufferLength   The size (in bytes) of ReportBuffer.  This value 
                         should be greater than or equal to the 
                         FeatureReportByteLength field as specified in the 
                         HIDP_CAPS structure for the device
Return Value:
    TRUE if successful
    FALSE otherwise  -- Use GetLastError() to get extended error information
--*/
{
    DWORD bytesReturned;
    return DeviceIoControl(HidDeviceObject, 0xb0192, NULL, 0, ReportBuffer, ReportBufferLength, &bytesReturned, NULL) != 0;
}

// HidD_GetHidGuid
void __stdcall
HidD_GetHidGuid (
   _Out_  LPGUID   HidGuid
   )
{
    HidGuid->Data1 = 0x4d1e55b2;
    HidGuid->Data2 = 0xf16f;
    HidGuid->Data3 = 0x11cf;
    HidGuid->Data4[0] = 0x88;
    HidGuid->Data4[1] = 0xcb;
    HidGuid->Data4[2] = 0;
    HidGuid->Data4[3] = 0x11;
    HidGuid->Data4[4] = 0x11;
    HidGuid->Data4[5] = 0;
    HidGuid->Data4[6] = 0;
    HidGuid->Data4[7] = 0x30;
}

// HidD_GetIndexedString
_Must_inspect_result_
_Success_(return==TRUE)
BOOLEAN __stdcall
HidD_GetIndexedString (
   _In_    HANDLE   HidDeviceObject,
   _In_    ULONG    StringIndex,
   _Out_writes_bytes_(BufferLength) PVOID Buffer,
   _In_    ULONG    BufferLength
   )
/*++
Routine Description:
    This function retrieves a string from the specified Hid device that is
    specified with a certain string index.

Arguments:
    HidDeviceObject      A handle to a Hid Device Object.
 
    StringIndex          Index of the string to retrieve

    Buffer               Buffer which on return will contain the product
                         string returned from the device.  This string is a 
                         wide-character string

    BufferLength         Length of Buffer (in bytes)

Return Value:
    TRUE if successful
    FALSE otherwise  -- Use GetLastError() to get extended error information
--*/
{
    DWORD bytesReturned;
    return DeviceIoControl(HidDeviceObject, 0xb01e2, &StringIndex, 4, Buffer, BufferLength, &bytesReturned, NULL) != 0;
}

// HidD_GetInputReport
_Must_inspect_result_
_Success_(return==TRUE)
BOOLEAN __stdcall
HidD_GetInputReport (
   _In_    HANDLE   HidDeviceObject,
   _Out_writes_bytes_(ReportBufferLength) PVOID ReportBuffer,
   _In_    ULONG    ReportBufferLength
   )
/*++
Routine Description:
    Retrieve an input report from a HID device.

Arguments:
    HidDeviceObject      A handle to a Hid Device Object.
 
    ReportBuffer         The buffer that the input report should be placed 
                         into.  The first byte of the buffer should be set to
                         the report ID of the desired report
 
    ReportBufferLength   The size (in bytes) of ReportBuffer.  This value 
                         should be greater than or equal to the 
                         InputReportByteLength field as specified in the 
                         HIDP_CAPS structure for the device
Return Value:
    TRUE if successful
    FALSE otherwise  -- Use GetLastError() to get extended error information
--*/
{
    DWORD bytesReturned;
    return DeviceIoControl(HidDeviceObject, 0xb01a2, NULL, 0, ReportBuffer, ReportBufferLength, &bytesReturned, NULL) != 0;
}

// HidD_GetManufacturerString
_Must_inspect_result_
_Success_(return==TRUE)
BOOLEAN __stdcall
HidD_GetManufacturerString (
   _In_    HANDLE   HidDeviceObject,
   _Out_writes_bytes_(BufferLength) PVOID Buffer,
   _In_    ULONG    BufferLength
   )
/*++
Routine Description:
    This function retrieves the manufacturer string from the specified 
    Hid device.  

Arguments:
    HidDeviceObject      A handle to a Hid Device Object.
 
    Buffer               Buffer which on return will contain the manufacturer
                         string returned from the device.  This string is a 
                         wide-character string

    BufferLength         Length of Buffer (in bytes)


Return Value:
    TRUE if successful
    FALSE otherwise  -- Use GetLastError() to get extended error information
--*/
{
    DWORD bytesReturned;
    return DeviceIoControl(HidDeviceObject, 0xb01ba, NULL, 0, Buffer, BufferLength, &bytesReturned, NULL) != 0;
}

// HidD_GetMsGenreDescriptor
_Must_inspect_result_
_Success_(return==TRUE)
BOOLEAN __stdcall
HidD_GetMsGenreDescriptor (
   _In_    HANDLE   HidDeviceObject,
   _Out_writes_bytes_(BufferLength) PVOID Buffer,
   _In_    ULONG    BufferLength
   )
/*++
Routine Description:
    This function retrieves the Microsoft Genre descriptor from the specified 
    Hid device.  

Arguments:
    HidDeviceObject      A handle to a Hid Device Object.
 
    Buffer               Buffer which on return will contain the descriptor
                         returned from the device.
                         
    BufferLength         Length of Buffer (in bytes)

Return Value:
    TRUE if successful
    FALSE otherwise  -- Use GetLastError() to get extended error information
--*/
{
    DWORD bytesReturned;
    return DeviceIoControl(HidDeviceObject, 0xb01e6, NULL, 0, Buffer, BufferLength, &bytesReturned, NULL) != 0;
}

// HidD_GetNumInputBuffers
_Must_inspect_result_
_Success_(return==TRUE)
BOOLEAN __stdcall
HidD_GetNumInputBuffers (
    _In_  HANDLE  HidDeviceObject,
    _Out_ PULONG  NumberBuffers
    )
/*++
Routine Description:
    This function returns the number of input buffers used by the specified
    file handle to the Hid device.  Each file object has a number of buffers
    associated with it to queue reports read from the device but which have
    not yet been read by the user-mode app with a handle to that device.

Arguments:
    HidDeviceObject      A handle to a Hid Device Object.
 
    NumberBuffers        Number of buffers currently being used for this file
                         handle to the Hid device

Return Value:
    TRUE if successful
    FALSE otherwise  -- Use GetLastError() to get extended error information
--*/
{
    DWORD bytesReturned;
    return DeviceIoControl(HidDeviceObject, 0xb01a0, NULL, 0, NumberBuffers, 4, &bytesReturned, NULL) != 0;
}

// HidD_GetPhysicalDescriptor
_Must_inspect_result_
_Success_(return==TRUE)
BOOLEAN __stdcall
HidD_GetPhysicalDescriptor (
   _In_    HANDLE   HidDeviceObject,
   _Out_writes_bytes_(BufferLength) PVOID Buffer,
   _In_    ULONG    BufferLength
   )
/*++
Routine Description:
    This function retrieves the raw physical descriptor for the specified
    Hid device.  

Arguments:
    HidDeviceObject      A handle to a Hid Device Object.
 
    Buffer               Buffer which on return will contain the physical
                         descriptor if one exists for the specified device
                         handle

    BufferLength         Length of buffer (in bytes)


Return Value:
    TRUE if successful
    FALSE otherwise  -- Use GetLastError() to get extended error information
--*/
{
    DWORD bytesReturned;
    return DeviceIoControl(HidDeviceObject, 0xb019a, NULL, 0, Buffer, BufferLength, &bytesReturned, NULL) != 0;
}

// HidD_GetPreparsedData
_Must_inspect_result_
_Success_(return==TRUE)
BOOLEAN __stdcall
HidD_GetPreparsedData (
   _In_   HANDLE                  HidDeviceObject,
   _Out_ _When_(return!=0, __drv_allocatesMem(Mem))  PHIDP_PREPARSED_DATA  * PreparsedData
   )
/*++
Routine Description:
    Given a handle to a valid Hid Class Device Object, retrieve the preparsed
    data for the device.  This routine will allocate the appropriately 
    sized buffer to hold this preparsed data.  It is up to client to call
    HidP_FreePreparsedData to free the memory allocated to this structure when
    it is no longer needed.

Arguments:
   HidDeviceObject A handle to a Hid Device that the client obtains using 
                   a call to CreateFile on a valid Hid device string name.
                   The string name can be obtained using standard PnP calls.

   PreparsedData   An opaque data structure used by other functions in this 
                   library to retrieve information about a given device.

Return Value:
   TRUE if successful.
   FALSE otherwise  -- Use GetLastError() to get extended error information
--*/
{
    DWORD bytesReturned;
    BYTE buffer[12];
    unsigned long dataSize;
    void* allocation;
    
    BOOL success = DeviceIoControl(HidDeviceObject, 0xb01a8, NULL, 0, buffer, 12, &bytesReturned, NULL);
    if (!success)
        return 0;
    
    dataSize = *(unsigned long*)buffer;
    if (dataSize >= 0xfffffff0)
    {
        SetLastError(8);
        return 0;
    }
    
    allocation = LocalAlloc(0x40, dataSize + 16);
    if (!allocation)
        return 0;
    
    *(void**)allocation = HidD_Hello;
    *PreparsedData = (PHIDP_PREPARSED_DATA)((BYTE*)allocation + 16);
    
    success = DeviceIoControl(HidDeviceObject, 0xb0193, NULL, 0, *PreparsedData, dataSize, &bytesReturned, NULL);
    if (!success)
        return 0;
    
    return 1;
}

// HidD_GetProductString
_Must_inspect_result_
_Success_(return==TRUE)
BOOLEAN __stdcall
HidD_GetProductString (
   _In_    HANDLE   HidDeviceObject,
   _Out_writes_bytes_(BufferLength) PVOID Buffer,
   _In_    ULONG    BufferLength
   )
/*++
Routine Description:
    This function retrieves the product string from the specified 
    Hid device.  

Arguments:
    HidDeviceObject      A handle to a Hid Device Object.
 
    Buffer               Buffer which on return will contain the product
                         string returned from the device.  This string is a 
                         wide-character string

    BufferLength         Length of Buffer (in bytes)


Return Value:
    TRUE if successful
    FALSE otherwise  -- Use GetLastError() to get extended error information
--*/
{
    DWORD bytesReturned;
    return DeviceIoControl(HidDeviceObject, 0xb01be, NULL, 0, Buffer, BufferLength, &bytesReturned, NULL) != 0;
}

// HidD_GetSerialNumberString
_Must_inspect_result_
_Success_(return==TRUE)
BOOLEAN __stdcall
HidD_GetSerialNumberString (
   _In_    HANDLE   HidDeviceObject,
   _Out_writes_bytes_(BufferLength) PVOID Buffer,
   _In_    ULONG    BufferLength
   )
/*++
Routine Description:
    This function retrieves the serial number string from the specified 
    Hid device.  

Arguments:
    HidDeviceObject      A handle to a Hid Device Object.
 
    Buffer               Buffer which on return will contain the serial number
                         string returned from the device.  This string is a 
                         wide-character string

    BufferLength         Length of Buffer (in bytes)

Return Value:
    TRUE if successful
    FALSE otherwise  -- Use GetLastError() to get extended error information
--*/
{
    DWORD bytesReturned;
    return DeviceIoControl(HidDeviceObject, 0xb01c2, NULL, 0, Buffer, BufferLength, &bytesReturned, NULL) != 0;
}

// HidD_Hello
int __cdecl HidD_Hello(char* destination, int size)
{
    memcpy(destination, "Hello", 6);
    return 7;
}

// HidD_SetConfiguration
_Must_inspect_result_
_Success_(return==TRUE)
BOOLEAN __stdcall
HidD_SetConfiguration (
   _In_   HANDLE               HidDeviceObject,
   _In_reads_bytes_(ConfigurationLength) PHIDD_CONFIGURATION Configuration,
   _In_   ULONG                ConfigurationLength
   )
/*++
Routine Description:
   Set the configuration information for this Hid device...
   
   NOTE: HidD_GetConfiguration must be called to retrieve the current 
         configuration information before this information can be modified 
         and set.

Arguments:
    HidDeviceObject      A handle to a Hid Device Object.
 
    Configuration        A configuration structure.  HidD_GetConfiguration MUST
                         be called before the configuration can be modified and
                         set using HidD_SetConfiguration
 
    ConfigurationLength  That is ``sizeof (HIDD_CONFIGURATION)''. Using this
                         parameter, we can later increase the length of the 
                         configuration array and not break older apps.

Return Value:
    TRUE if successful
    FALSE otherwise  -- Use GetLastError() to get extended error information
--*/
{
    DWORD bytesReturned;
    
    if (Configuration->cookie != HidD_GetConfiguration || ConfigurationLength < 12)
    {
        SetLastError(0x57);
        return 0;
    }
    
    return DeviceIoControl(HidDeviceObject, 0xb0194, NULL, 0, &Configuration->size, ConfigurationLength - 4, &bytesReturned, NULL) != 0;
}

// HidD_SetFeature
_Must_inspect_result_
_Success_(return==TRUE)
BOOLEAN __stdcall
HidD_SetFeature (
   _In_    HANDLE   HidDeviceObject,
   _In_reads_bytes_(ReportBufferLength) PVOID ReportBuffer,
   _In_    ULONG    ReportBufferLength
   )
/*++
Routine Description:
    Send a feature report to a HID device.

Arguments:
    HidDeviceObject      A handle to a Hid Device Object.
 
    ReportBuffer         The buffer of the feature report to send to the device
 
    ReportBufferLength   The size (in bytes) of ReportBuffer.  This value 
                         should be greater than or equal to the 
                         FeatureReportByteLength field as specified in the 
                         HIDP_CAPS structure for the device
Return Value:
    TRUE if successful
    FALSE otherwise  -- Use GetLastError() to get extended error information
--*/
{
    DWORD bytesReturned;
    return DeviceIoControl(HidDeviceObject, 0xb0191, ReportBuffer, ReportBufferLength, NULL, 0, &bytesReturned, NULL) != 0;
}

// HidD_SetNumInputBuffers
_Must_inspect_result_
_Success_(return==TRUE)
BOOLEAN __stdcall
HidD_SetNumInputBuffers (
    _In_ HANDLE HidDeviceObject,
    _In_ ULONG  NumberBuffers
    )
/*++

Routine Description:
    This function sets the number of input buffers used by the specified
    file handle to the Hid device.  Each file object has a number of buffers
    associated with it to queue reports read from the device but which have
    not yet been read by the user-mode app with a handle to that device.

Arguments:
    HidDeviceObject      A handle to a Hid Device Object.
 
    NumberBuffers        New number of buffers to use for this file handle to
                         the Hid device

Return Value:
    TRUE if successful
    FALSE otherwise  -- Use GetLastError() to get extended error information
--*/
{
    DWORD bytesReturned;
    return DeviceIoControl(HidDeviceObject, 0xb01a4, &NumberBuffers, 4, NULL, 0, &bytesReturned, NULL) != 0;
}

// HidD_SetOutputReport
_Must_inspect_result_
_Success_(return==TRUE)
BOOLEAN __stdcall
HidD_SetOutputReport (
   _In_    HANDLE   HidDeviceObject,
   _In_reads_bytes_(ReportBufferLength) PVOID ReportBuffer,
   _In_    ULONG    ReportBufferLength
   )
/*++
Routine Description:
    Send an output report to a HID device.

Arguments:
    HidDeviceObject      A handle to a Hid Device Object.
 
    ReportBuffer         The buffer of the output report to send to the device
 
    ReportBufferLength   The size (in bytes) of ReportBuffer.  This value 
                         should be greater than or equal to the 
                         OutputReportByteLength field as specified in the 
                         HIDP_CAPS structure for the device
Return Value:
    TRUE if successful
    FALSE otherwise  -- Use GetLastError() to get extended error information
--*/
{
    DWORD bytesReturned;
    return DeviceIoControl(HidDeviceObject, 0xb0195, ReportBuffer, ReportBufferLength, NULL, 0, &bytesReturned, NULL) != 0;
}

