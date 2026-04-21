#include <ntddk.h>
#include <wdm.h>
#include <evntrace.h>

// GUID for ETW provider from hex dump
// {CBA16CF2-2FAB-49F8-89AE-894E718649E7}
EXTERN_C const GUID AD_ETW_PROVIDER = 
    { 0xcba16cf2, 0x2fab, 0x49f8, { 0x89, 0xae, 0x89, 0x4e, 0x71, 0x86, 0x49, 0xe7 } };

// Event descriptor for AD_ETW_OST from hex dump
// Id=2, Version=0, Channel=0, Level=4, Opcode=0, Task=2, Keyword=0
static const EVENT_DESCRIPTOR AD_ETW_OST = {
    0x02,       // Id
    0x00,       // Version
    0x00,       // Channel
    0x04,       // Level (Informational)
    0x00,       // Opcode
    0x02,       // Task
    0x0000000000000000ULL  // Keyword
};

// Global variables
ULONG64 AdEtwHandle = 0;
BOOLEAN AdEtwRegistered = FALSE;

// Log OST event
void AdEtwOSTEvent(
    ULONG Param1,
    ULONG Param2,
    ULONG Param3
)
{
    EVENT_DATA_DESCRIPTOR eventData[3];
    
    if (AdEtwRegistered)
    {
        // Check if the event is enabled
        if (EtwEventEnabled(AdEtwHandle, &AD_ETW_OST))
        {
            // Initialize event data descriptors
            EventDataDescCreate(&eventData[0], &Param1, sizeof(ULONG));
            EventDataDescCreate(&eventData[1], &Param2, sizeof(ULONG));
            EventDataDescCreate(&eventData[2], &Param3, sizeof(ULONG));
            
            // Write the event
            EtwWrite(
                AdEtwHandle,
                &AD_ETW_OST,
                NULL,
                3,
                &eventData[0]
            );
        }
    }
}

// Register ETW provider
NTSTATUS AdRegisterEtw(void)
{
    NTSTATUS status;
    
    status = EtwRegister(
        &AD_ETW_PROVIDER,
        NULL,
        NULL,
        &AdEtwHandle
    );
    
    if (NT_SUCCESS(status))
    {
        AdEtwRegistered = TRUE;
    }
    
    return status;
}

