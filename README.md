# Windows 11 source code

This is the (partially) leaked source code of Microsoft's latest and greatest operating system, Windows 11.

Be aware that due to the closed-source state of the operating system, and the fact it's the latest Windows version that gets updates everyday which shows that Microsoft constantly takes care of it, and the fact there aren't any EXE/DLL/OCX/etc decompilers that have the capability of restoring at least the majority of the Windows 11 system files to their EXACT source code before they got compiled, it's really impossible to get the source code for the majority of the Windows 11 files and all the files in this repository are based on extracting resources from DLLs using ResHacker, some modified files from the Windows XP/Server 2003 source code, and finally a portion of the NT 10.0 source code that is officially leaked by Microsoft THEMSELVES (check Windows 11 24H2 SDK). This also includes parts of the Windows conhost source code.


No copyright infringement intended as well and the Windows 11 operating system is Microsoft's trademark.

\NT\ is your starting folder. Will update you once I get access to more components.

(c) 2021 Microsoft Corporation.  All rights reserved.

Some code might also be derived from other non-Microsoft open-source projects, such as ReactOS. All other non-Microsoft trademarks are property of their respective owners.

Source code compiled by TechTech512 on GitHub.

## Directory Guide

The table below describes `NT/` and its top-level source directories.

| Folder | Purpose |
| --- | --- |
| `NT/` | Main directory containing the Windows source tree, build files, and supporting materials. |
| `NT/avcore/` | Audio and video components shared by Windows media features. |
| `NT/binplace/` | Build rules and files for placing compiled binaries in output locations. |
| `NT/clientcore/` | Core components and services for Windows client editions. |
| `NT/desktop/` | Desktop Windows components, including administration, drivers, networking, printing, shell, and user-facing features. |
| `NT/iot/` | Components and drivers for Windows IoT editions and devices. |
| `NT/mincore/` | Minimal Windows core components, including API sets, kernel-base functionality, managed support, and text input. |
| `NT/minio/` | Minimal I/O and networking components, including API sets and network I/O. |
| `NT/minkernel/` | Low-level operating-system components, including boot, kernel, file-system, storage, and driver code. |
| `NT/moderncore/` | Components for the modern Windows platform. |
| `NT/modernpc/` | Components and merged code specific to modern PC platforms. |
| `NT/onecore/` | Shared OneCore components used across Windows device families. |
| `NT/onecoreuap/` | OneCore components for the Universal Application Platform (UAP), including platform, networking, driver, and Windows components. |
| `NT/pcshell/` | PC shell components, including Remote Desktop Services-related code. |
| `NT/public/` | Public development interfaces and kits, including DDK, SDK, and related headers. |
| `NT/published/` | Published Windows component interfaces and supporting files, grouped by subsystem. |
| `NT/servercommon/` | Components shared by Windows Server features and services. |
| `NT/shellcommon/` | Common Windows shell components and supporting code. |
| `NT/tools/` | Build, packaging, and engineering scripts and utilities. |

### Second-Level Directories

| Folder | Purpose |
| --- | --- |
| `NT/avcore/apiset/` | API-set definitions and interfaces for audio and video components. |
| `NT/avcore/media/` | Shared media and audio/video implementation components. |
| `NT/binplace/base/` | Binary-placement rules and support files for base components. |
| `NT/binplace/shell/` | Binary-placement rules and support files for shell components. |
| `NT/clientcore/base/` | Base operating-system components for Windows clients. |
| `NT/clientcore/ds/` | Directory-services components for Windows clients. |
| `NT/clientcore/windows/` | Windows-specific client components. |
| `NT/desktop/admin/` | Desktop administration components and utilities. |
| `NT/desktop/base/` | Base components for desktop Windows. |
| `NT/desktop/drivers/` | Drivers for desktop Windows. |
| `NT/desktop/inetcore/` | Internet and network-client components for desktop Windows. |
| `NT/desktop/inetsrv/` | Internet server components for desktop Windows. |
| `NT/desktop/multimedia/` | Desktop multimedia components. |
| `NT/desktop/net/` | Desktop networking components. |
| `NT/desktop/printscan/` | Printing and scanning components for desktop Windows. |
| `NT/desktop/shell/` | Desktop shell components and utilities. |
| `NT/desktop/windows/` | Windows-specific desktop components. |
| `NT/iot/base/` | Base operating-system components for IoT editions. |
| `NT/iot/drivers/` | Drivers for IoT devices and editions. |
| `NT/mincore/apiset/` | API-set definitions for minimal Windows core components. |
| `NT/mincore/kernelbase/` | Kernel-base APIs and supporting components. |
| `NT/mincore/managed/` | Managed-runtime and managed-code support for the minimal core. |
| `NT/mincore/textinput/` | Core text-input components. |
| `NT/minio/apiset/` | API-set definitions for minimal I/O components. |
| `NT/minio/netio/` | Minimal network I/O components. |
| `NT/minkernel/apiset/` | API-set definitions for kernel components. |
| `NT/minkernel/boot/` | Boot and early-startup components. |
| `NT/minkernel/filters/` | Kernel filter-driver components. |
| `NT/minkernel/fs/` | File-system components. |
| `NT/minkernel/fsrec/` | File-system recognition components. |
| `NT/minkernel/hwpolicy/` | Hardware policy components. |
| `NT/minkernel/ium/` | Isolated User Mode components. |
| `NT/minkernel/lzx/` | LZX compression components. |
| `NT/minkernel/minwin/` | Minimal Windows and Win32 subsystem components. |
| `NT/minkernel/mobilecore/` | Mobile Windows core components. |
| `NT/minkernel/ntos/` | Windows NT operating-system kernel components. |
| `NT/minkernel/ntosext/` | NT kernel extension components. |
| `NT/minkernel/storage/` | Kernel storage components. |
| `NT/minkernel/strsafe/` | Safe string-handling library components. |
| `NT/minkernel/tm/` | Transaction-manager components. |
| `NT/minkernel/uart/` | UART support components. |
| `NT/minkernel/wdmlib/` | Windows Driver Model library components. |
| `NT/minkernel/wmilib/` | Windows Management Instrumentation library components. |
| `NT/moderncore/windows/` | Windows components for the modern core platform. |
| `NT/modernpc/merged/` | Merged components for modern PC platforms. |
| `NT/onecore/admin/` | Administration components shared by OneCore platforms. |
| `NT/onecore/base/` | Base components shared by OneCore platforms. |
| `NT/onecore/com/` | Component Object Model (COM) components for OneCore. |
| `NT/onecore/drivers/` | Drivers shared by OneCore platforms. |
| `NT/onecore/ds/` | Directory-services components for OneCore. |
| `NT/onecore/enduser/` | End-user features and components for OneCore. |
| `NT/onecore/hv/` | Hypervisor-related components for OneCore. |
| `NT/onecore/inetcore/` | Internet and network-client components for OneCore. |
| `NT/onecore/merged/` | Merged components shared across OneCore platforms. |
| `NT/onecore/multimedia/` | Multimedia components shared by OneCore platforms. |
| `NT/onecore/net/` | Networking components for OneCore. |
| `NT/onecore/printscan/` | Printing and scanning components for OneCore. |
| `NT/onecore/sdktools/` | SDK and development tools for OneCore components. |
| `NT/onecore/shell/` | Shell components for OneCore platforms. |
| `NT/onecore/termsrv/` | Terminal Services and remote-session components for OneCore. |
| `NT/onecore/vm/` | Virtual-machine components for OneCore. |
| `NT/onecore/windows/` | Windows-specific components shared by OneCore platforms. |
| `NT/onecoreuap/base/` | Base components for the OneCore UAP platform. |
| `NT/onecoreuap/drivers/` | Drivers for the OneCore UAP platform. |
| `NT/onecoreuap/inetcore/` | Internet and network-client components for OneCore UAP. |
| `NT/onecoreuap/merged/` | Merged components for OneCore UAP. |
| `NT/onecoreuap/net/` | Networking components for OneCore UAP. |
| `NT/onecoreuap/windows/` | Windows-specific components for OneCore UAP. |
| `NT/onecoreuap/xbox/` | Xbox-specific components for OneCore UAP. |
| `NT/pcshell/termsrv/` | Terminal Services components for the PC shell. |
| `NT/public/ddk/` | Public Driver Development Kit interfaces and headers. |
| `NT/public/ifskit/` | Public Installable File System Kit interfaces and headers. |
| `NT/public/internal/` | Internal interfaces and supporting files in the public tree. |
| `NT/public/private/` | Private interfaces and supporting files in the public tree. |
| `NT/public/sdk/` | Public Software Development Kit interfaces and headers. |
| `NT/published/base/` | Published interfaces and files for base components. |
| `NT/published/ds/` | Published interfaces and files for directory-services components. |
| `NT/published/multimedia/` | Published interfaces and files for multimedia components. |
| `NT/published/windows/` | Published interfaces and files for Windows components. |
| `NT/servercommon/base/` | Base components shared by Windows Server. |
| `NT/servercommon/inetsrv/` | Internet server components shared by Windows Server. |
| `NT/shellcommon/base/` | Base components shared by Windows shell implementations. |
| `NT/shellcommon/merged/` | Merged components shared across Windows shells. |
| `NT/shellcommon/shell/` | Common Windows shell components. |
| `NT/tools/amd64/` | Build tools and support files for AMD64 targets. |
| `NT/tools/arm/` | Build tools and support files for ARM targets. |
| `NT/tools/popfilter/` | Utilities and support files for the popfilter tool. |
| `NT/tools/qfelists/` | QFE-related lists and supporting build files. |
| `NT/tools/samples/` | Sample code and examples for development tools. |
| `NT/tools/tools16/` | Legacy 16-bit development tools and supporting files. |
| `NT/tools/wppconfig/` | Configuration files for WPP tracing tools. |
| `NT/tools/x86/` | Build tools and support files for x86 targets. |
