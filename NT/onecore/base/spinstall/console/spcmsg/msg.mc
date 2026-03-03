MessageIdTypedef=DWORD

SeverityNames=(
    Success=0x0
    Informational=0x1
    Warning=0x2
    Error=0x3
    )

FacilityNames=(
    System=0x0
    ServicePack=0xABC:FACILITY_SERVICE_PACK
    ServicePackProvider=0x2:FACILITY_SERVICE_PACK_PROVIDER
    )

; // Service Pack Installer Event Messages

MessageId=0x1
Severity=Success
Facility=System
SymbolicName=MSG_SERVICE_PACK_STARTED
Language=English
Service Pack %1 installation started.
.

MessageId=0x2
Severity=Success
Facility=System
SymbolicName=MSG_DRIVER_COMPATIBILITY_KNOWN
Language=English
An installed driver has known compatibility problems.  Try updating the driver to a more recent version.%n
   Name:   %1%n
   Reason: %2%n
   GUID:   %3
.

MessageId=0x3
Severity=Success
Facility=System
SymbolicName=MSG_DRIVER_COMPATIBILITY_POSSIBLE
Language=English
An installed driver might cause compatibility problems.%n
   Name:   %1%n
   Reason: %2%n
   GUID:   %3
.

MessageId=0x4
Severity=Success
Facility=System
SymbolicName=MSG_UPDATE_COMPATIBILITY
Language=English
An update (KB%1) installed on the system might stop working correctly after the installation of the Service Pack.
.

MessageId=0x5
Severity=Success
Facility=System
SymbolicName=MSG_DISK_SPACE_INSUFFICIENT
Language=English
There is not enough free disk space to install the Service Pack. Required=%1 MB.
.

MessageId=0x6
Severity=Success
Facility=System
SymbolicName=MSG_BATTERY_POWER
Language=English
The Service Pack cannot be installed when the computer is running on battery power.
.

MessageId=0x7
Severity=Success
Facility=System
SymbolicName=MSG_UPDATE_CHANGE_FAILED
Language=English
Changes to an update(%1) failed during Service Pack installation.%n
   Identity:     %2%n
   Error Code:   %3%n
   Target State: %4
.

MessageId=0x8
Severity=Success
Facility=System
SymbolicName=MSG_INSTALLATION_FAILED
Language=English
Service Pack installation failed with error code %1.
.

MessageId=0x9
Severity=Success
Facility=System
SymbolicName=MSG_INSTALLATION_SUCCEEDED
Language=English
Service Pack %1 installation succeeded.
.
