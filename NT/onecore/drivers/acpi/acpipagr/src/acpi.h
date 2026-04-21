#include <ntddk.h>
#include <wdm.h>
#include <wdf.h>

// Forward declarations
typedef struct _AD_EXTENSION AD_EXTENSION, *PAD_EXTENSION;

// ACPI interface structure forward declaration
typedef struct _ACPI_INTERFACE_STANDARD ACPI_INTERFACE_STANDARD, *PACPI_INTERFACE_STANDARD;

// Work item state bitfield
typedef struct _AD_WORKITEM_STATE_s_0 {
    unsigned long Pur : 1;
    unsigned long Thermal : 1;
    unsigned long Inserted : 1;
} AD_WORKITEM_STATE_s_0, *PAD_WORKITEM_STATE_s_0;

// Work item state union
typedef union _AD_WORKITEM_STATE {
    AD_WORKITEM_STATE_s_0 _s_0;
    long State;
} AD_WORKITEM_STATE, *PAD_WORKITEM_STATE;

// AD_EXTENSION structure
struct _AD_EXTENSION {
    WDFDEVICE *WdfSelf;
    PDEVICE_OBJECT WdmSelf;
    PDEVICE_OBJECT WdmPdo;
    WDFIOTARGET *IoTarget;
    ACPI_INTERFACE_STANDARD AcpiInterfaces;
    unsigned long DeviceStopped;
    EX_PUSH_LOCK DeviceStopLock;
    WDFWORKITEM *WorkItem;
    AD_WORKITEM_STATE WorkItemState;
    unsigned long AcpiPURNumCpus;
    unsigned long AcpiValidNumCpus;
    unsigned long ThermalCap;
    unsigned long ThermalIdledCpus;
    unsigned long ProcessorCount;
};

// Work type enumeration
typedef enum _AD_WORK_TYPE {
    AdWorkInitialize = 0,
    AdWorkPur = 1,
    AdWorkThermal = 2
} AD_WORK_TYPE;
