/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    pch.h

Abstract:

    Serial Network Kernel Debugger Extensibility DLL precompiled headers.

Author:

    Bill Messmer (wmessmer)

--*/

#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning(disable:4201 4214)

#include <ntddk.h>
#include <process.h>
#include <kdcom.h>
#include "kdnetshareddata.h"
#include "kdnetextensibility.h"
#include "kdox16pci95x.h"

#pragma warning(default:4201 4214)
#if _MSC_VER >= 1200
#pragma warning(pop)
#endif

