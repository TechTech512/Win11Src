/*++

Copyright (C) 1989-1998 Microsoft Corporation, All rights reserved

Module:
    tscfgex.h

Abstract:
    Terminal Server Connection Configuration DLL extension data structures
    and function prototypes.

Author:
    Brad Graziadio (BradG) 4-Feb-98

--*/

#undef _CRT_DEPRECATE_TEXT

#ifndef _RDPCFGEX_
#define _RDPCFGEX_

//
// Constants used for string table entries
//
#define IDS_LOW                         1000
#define IDS_COMPATIBLE                  1001
#define IDS_HIGH                        1002
#define IDS_FIPS                        1003

#define IDS_LOW_DESCR                   1010
#define IDS_COMPATIBLE_DESCR            1011
#define IDS_HI_DESCR                    1012
#define IDS_FIPS_DESCR                  1013

#define IDS_RDPSECLAYER                11000
#define IDS_NEGOTIATE                  11001
#define IDS_SSL                        11002
#define IDS_RDPSECDESCR                11003
#define IDS_NEGOTDESC                  11004
#define IDS_SSLDESC                    11005
#define IDS_MAX_DESCRIPT               11006
#define IDS_MAX_PROTECT                11007

#define IDS_HI_DESCR2                  11008
#define IDS_FIPS_DESCR2                11009

//
// DWORD values that get stored in the registry to represent the
// encryption level.
//
#define REG_LOW                         0x00000001
#define REG_MEDIUM                      0x00000002
#define REG_HIGH                        0x00000003
#define REG_FIPS                        0x00000004

//
// Number of encryption levels the RDP protocol uses
//
#define NUM_RDP_ENCRYPTION_LEVELS       4

#endif

