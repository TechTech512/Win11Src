/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    pch.h

Abstract:

    Network Kernel Debugger Extensibility DLL precompiled headers.

Author:

    Joe Ballantyne (joeball)

--*/

#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning(disable:4201 4214)

#include <ntddk.h>
#include <ntstatus.h>
#include <process.h>
#include "rt_def.h"
#include "kdnetshareddata.h"
#include "kdrealtek.h"
#include "kdnetextensibility.h"
#include "rt_equ.h"
#include "rt_reg.h"

#pragma warning(default:4201 4214)
#if _MSC_VER >= 1200
#pragma warning(pop)
#endif

