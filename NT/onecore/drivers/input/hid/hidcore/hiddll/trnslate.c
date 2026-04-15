#include <windows.h>
#include <hidclass.h>
#include <winternl.h>
#include <hidusage.h>
#include <hidpi.h>

// Global tables (defined elsewhere)
// HidP_KeyboardToScanCodeTable - 256 entries (0x100)
// Maps keyboard usages to scan codes
ULONG HidP_KeyboardToScanCodeTable[256] = {
    // 0x00-0x03
    0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF,
    // 0x04-0x07
    0x0000001E, 0x00000030, 0x0000002E, 0x00000020,
    // 0x08-0x0B
    0x00000012, 0x00000021, 0x00000022, 0x00000023,
    // 0x0C-0x0F
    0x00000017, 0x00000024, 0x00000025, 0x00000026,
    // 0x10-0x13
    0x00000032, 0x00000031, 0x00000018, 0x00000019,
    // 0x14-0x17
    0x00000010, 0x00000013, 0x0000001F, 0x00000014,
    // 0x18-0x1B
    0x00000016, 0x0000002F, 0x00000011, 0x0000002D,
    // 0x1C-0x1F
    0x00000015, 0x0000002C, 0x00000002, 0x00000003,
    // 0x20-0x23
    0x00000004, 0x00000005, 0x00000006, 0x00000007,
    // 0x24-0x27
    0x00000008, 0x00000009, 0x0000000A, 0x0000000B,
    // 0x28-0x2B
    0x0000001C, 0x00000001, 0x0000000E, 0x0000000F,
    // 0x2C-0x2F
    0x00000039, 0x0000000C, 0x0000000D, 0x0000001A,
    // 0x30-0x33
    0x0000001B, 0x0000002B, 0x0000002B, 0x00000027,
    // 0x34-0x37
    0x00000028, 0x00000029, 0x00000033, 0x00000034,
    // 0x38-0x3B
    0x00000035, 0x000008F1, 0x0000003B, 0x0000003C,
    // 0x3C-0x3F
    0x0000003D, 0x0000003E, 0x0000003F, 0x00000040,
    // 0x40-0x43
    0x00000041, 0x00000042, 0x00000043, 0x00000044,
    // 0x44-0x47
    0x00000057, 0x00000058, 0x000000F3, 0x000009F1,
    // 0x48-0x4B
    0x00451DE1, 0x000000F0, 0x000001F0, 0x000002F0,
    // 0x4C-0x4F
    0x000003F0, 0x000004F0, 0x000005F0, 0x000006F0,
    // 0x50-0x53
    0x000007F0, 0x000008F0, 0x000009F0, 0x00000AF1,
    // 0x54-0x57
    0x000035E0, 0x00000037, 0x0000004A, 0x0000004E,
    // 0x58-0x5B
    0x00001CE0, 0x0000004F, 0x00000050, 0x00000051,
    // 0x5C-0x5F
    0x0000004B, 0x0000004C, 0x0000004D, 0x00000047,
    // 0x60-0x63
    0x00000048, 0x00000049, 0x00000052, 0x00000053,
    // 0x64-0x67
    0x00000056, 0x00005DE0, 0x00005EE0, 0x00000059,
    // 0x68-0x6B
    0x00000064, 0x00000065, 0x00000066, 0x00000067,
    // 0x6C-0x6F
    0x00000068, 0x00000069, 0x0000006A, 0x0000006B,
    // 0x70-0x73
    0x0000006C, 0x0000006D, 0x0000006E, 0x00000076,
    // 0x74-0x77
    0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF,
    // 0x78-0x7B
    0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF,
    // 0x7C-0x7F
    0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF,
    // 0x80-0x83
    0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF,
    // 0x84-0x87
    0x000000FF, 0x0000007E, 0x000000FF, 0x00000073,
    // 0x88-0x8B
    0x00000070, 0x0000007D, 0x00000079, 0x0000007B,
    // 0x8C-0x8F
    0x0000005C, 0x000000FF, 0x000000FF, 0x000000FF,
    // 0x90-0x93
    0x000000F2, 0x000001F2, 0x00000078, 0x00000077,
    // 0x94-0x97
    0x00000076, 0x000000FF, 0x000000FF, 0x000000FF,
    // 0x98-0x9B
    0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF,
    // 0x9C-0x9F
    0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF,
    // 0xA0-0xA3
    0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF,
    // 0xA4-0xA7
    0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF,
    // 0xA8-0xAB
    0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF,
    // 0xAC-0xAF
    0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF,
    // 0xB0-0xB3
    0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF,
    // 0xB4-0xB7
    0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF,
    // 0xB8-0xBB
    0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF,
    // 0xBC-0xBF
    0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF,
    // 0xC0-0xC3
    0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF,
    // 0xC4-0xC7
    0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF,
    // 0xC8-0xCB
    0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF,
    // 0xCC-0xCF
    0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF,
    // 0xD0-0xD3
    0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF,
    // 0xD4-0xD7
    0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF,
    // 0xD8-0xDB
    0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF,
    // 0xDC-0xDF
    0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF,
    // 0xE0-0xE3
    0x000000F1, 0x000001F1, 0x000002F1, 0x000003F1,
    // 0xE4-0xE7
    0x000004F1, 0x000005F1, 0x000006F1, 0x000007F1,
    // 0xE8-0xEB
    0x000000FF, 0x00005EE0, 0x00005FE0, 0x000063E0,
    // 0xEC-0xEF
    0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF,
    // 0xF0-0xF3
    0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF,
    // 0xF4-0xF7
    0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF,
    // 0xF8-0xFB
    0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF,
    // 0xFC-0xFF
    0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF
};
// Structure definition for keyboard sub-table entries
typedef struct _HIDP_KEYBOARD_SUBTABLE_ENTRY {
    void* ScanCodeFcn;      // Function pointer for scan code translation
    ULONG* Table;           // Lookup table pointer
} HIDP_KEYBOARD_SUBTABLE_ENTRY, *PHIDP_KEYBOARD_SUBTABLE_ENTRY;

// External function declarations referenced in the table
extern unsigned char __cdecl HidP_KeyboardKeypadCode(ULONG* param1, unsigned char param2, void* callback, void* context, HIDP_KEYBOARD_DIRECTION direction, HIDP_KEYBOARD_MODIFIER_STATE* modifierState);
extern ULONG* HidP_XlateKbdPadCodesS;  // Lookup table for keyboard pad codes

// HidP_KeyboardSubTables array - 16 entries
// In trnslate.c, replace the array declaration with:
HIDP_KEYBOARD_SUBTABLE_ENTRY HidP_KeyboardSubTables[16] = {
    { (void*)0x58e85030, (ULONG*)0x58e864c8 },
    { (void*)0x58e850a0, (ULONG*)0x58e86090 },
    { (void*)0x58e85420, (ULONG*)0x58e860c0 },
    { (void*)0x58e85140, (ULONG*)0x58e860bc },
    { NULL, NULL }, { NULL, NULL }, { NULL, NULL }, { NULL, NULL },
    { NULL, NULL }, { NULL, NULL }, { NULL, NULL }, { NULL, NULL },
    { NULL, NULL }, { NULL, NULL }, { NULL, NULL }, { NULL, NULL }
};

// Forward declarations
unsigned char __cdecl HidP_ModifierCode(ULONG* param1, unsigned char param2, void* callback, void* context, HIDP_KEYBOARD_DIRECTION direction, HIDP_KEYBOARD_MODIFIER_STATE* modifierState);
unsigned char __cdecl HidP_PrintScreenCode(ULONG* param1, unsigned char param2, void* callback, void* context, HIDP_KEYBOARD_DIRECTION direction, HIDP_KEYBOARD_MODIFIER_STATE* modifierState);
unsigned char __cdecl HidP_VendorBreakCodesAsMakeCodes(ULONG* param1, unsigned char param2, void* callback, void* context, HIDP_KEYBOARD_DIRECTION direction, HIDP_KEYBOARD_MODIFIER_STATE* modifierState);
ULONG __cdecl HidP_StraightLookup(ULONG* table, ULONG index);

// HidP_KbdPutKey wrapper
void __cdecl HidP_KbdPutKey(ULONG callback, HIDP_KEYBOARD_DIRECTION direction, void* context1, void* context2)
{
    unsigned int index = 0;
    unsigned char keyBuffer[4] = {0};
    
    do {
        if (keyBuffer[index] == 0)
            break;
        if (direction == HidP_Keyboard_Break) {
            keyBuffer[index] = keyBuffer[index] | 0x80;
        }
        index++;
    } while (index < 4);
    
    if (index != 0) {
        ((void(__cdecl*)(HIDP_KEYBOARD_DIRECTION, unsigned char*, unsigned int))callback)(direction, keyBuffer, index);
    }
}

// HidP_KeyboardKeypadCode
unsigned char __cdecl HidP_KeyboardKeypadCode(ULONG* param1, unsigned char param2, void* callback, void* context, HIDP_KEYBOARD_DIRECTION direction, HIDP_KEYBOARD_MODIFIER_STATE* modifierState)
{
    void* tempContext1;
    void* tempContext2;
    
    if (((modifierState->ul & 0x400) != 0) && (direction == HidP_Keyboard_Make)) {
        HidP_KbdPutKey((ULONG)callback, (HIDP_KEYBOARD_DIRECTION)context, callback, context);
    }
    
    HidP_KbdPutKey((ULONG)callback, (HIDP_KEYBOARD_DIRECTION)context, callback, context);
    
    if (((modifierState->ul & 0x400) != 0) && (direction == HidP_Keyboard_Break)) {
        HidP_KbdPutKey((ULONG)callback, (HIDP_KEYBOARD_DIRECTION)context, callback, context);
    }
    
    return 1;
}

// HidP_ModifierCode
unsigned char __cdecl HidP_ModifierCode(ULONG* param1, unsigned char modifierCode, void* callback, void* context, HIDP_KEYBOARD_DIRECTION direction, HIDP_KEYBOARD_MODIFIER_STATE* modifierState)
{
    if ((modifierCode & 0xf8) == 0) {
        if (direction == HidP_Keyboard_Break) {
            modifierState->ul = modifierState->ul & ~(1 << (modifierCode & 0x1f));
        }
        else if (direction == HidP_Keyboard_Make) {
            modifierState->ul = modifierState->ul | (1 << (modifierCode & 0x1f));
        }
    }
    else {
        if (direction == HidP_Keyboard_Break) {
            modifierState->ul = modifierState->ul & ~(1 << ((modifierCode + 0x10) & 0x1f));
        }
        else if (direction == HidP_Keyboard_Make) {
            unsigned int shiftedBit = modifierCode + 0x10;
            if ((modifierState->ul & (1 << (shiftedBit & 0x1f))) == 0) {
                modifierState->ul = (modifierState->ul | (1 << (shiftedBit & 0x1f))) ^ (1 << (modifierCode & 0x1f));
            }
        }
    }
    
    HidP_KbdPutKey((ULONG)callback, (HIDP_KEYBOARD_DIRECTION)context, callback, context);
    return 1;
}

// HidP_PrintScreenCode
unsigned char __cdecl HidP_PrintScreenCode(ULONG* param1, unsigned char param2, void* callback, void* context, HIDP_KEYBOARD_DIRECTION direction, HIDP_KEYBOARD_MODIFIER_STATE* modifierState)
{
    if ((modifierState->ul & 0x44) == 0) {
        if ((modifierState->ul & 0x33) == 0) {
            if (direction == HidP_Keyboard_Make) {
                HidP_KbdPutKey((ULONG)callback, (HIDP_KEYBOARD_DIRECTION)context, callback, context);
            }
            HidP_KbdPutKey((ULONG)callback, (HIDP_KEYBOARD_DIRECTION)context, callback, context);
            if (direction == HidP_Keyboard_Break) {
                HidP_KbdPutKey((ULONG)callback, (HIDP_KEYBOARD_DIRECTION)context, callback, context);
            }
        }
        else {
            HidP_KbdPutKey((ULONG)callback, (HIDP_KEYBOARD_DIRECTION)context, callback, context);
        }
    }
    else {
        HidP_KbdPutKey((ULONG)callback, (HIDP_KEYBOARD_DIRECTION)context, callback, context);
    }
    
    return 1;
}

// HidP_StraightLookup
ULONG __cdecl HidP_StraightLookup(ULONG* table, ULONG index)
{
    if (index < 0x100) {
        return table[index];
    }
    return 0;
}

// HidP_TranslateUsage
long __cdecl HidP_TranslateUsage(unsigned short usage, HIDP_KEYBOARD_DIRECTION direction, HIDP_KEYBOARD_MODIFIER_STATE* modifierState, void* callback1, ULONG* param5, void* param6, void* callback2, void* context)
{
    ULONG scanCode;
    unsigned char result;
    long returnValue;
    
    scanCode = HidP_StraightLookup(HidP_KeyboardToScanCodeTable, usage);
    
    if (scanCode == 0) {
        return -0x3feefff7;
    }
    
    if (((modifierState->ul & 0x11) != 0) && (scanCode == 0x451de1)) {
        scanCode = 0x46e0;
    }
    
    if ((scanCode & 0xf0) == 0xf0) {
        void* handler = HidP_KeyboardSubTables[scanCode & 0xf].ScanCodeFcn;
        if (handler != NULL) {
            unsigned char subIndex = (unsigned char)(scanCode >> 8);
            ULONG* subTable = HidP_KeyboardSubTables[scanCode & 0xf].Table;
            result = ((unsigned char(__cdecl*)(ULONG*, unsigned char, void*, void*, HIDP_KEYBOARD_DIRECTION, HIDP_KEYBOARD_MODIFIER_STATE*))handler)(subTable, subIndex, callback1, param6, direction, modifierState);
            if (result != 0) {
                return 0x110000;
            }
        }
        return -0x3feefff7;
    }
    else {
        HidP_KbdPutKey((ULONG)callback1, (HIDP_KEYBOARD_DIRECTION)param6, callback2, context);
        return 0x110000;
    }
}

// HidP_TranslateUsagesToI8042ScanCodes
_Must_inspect_result_
NTSTATUS __stdcall
HidP_TranslateUsagesToI8042ScanCodes (
    _In_reads_(UsageListLength)     PUSAGE ChangedUsageList,
    _In_     ULONG                         UsageListLength,
    _In_     HIDP_KEYBOARD_DIRECTION       KeyAction,
    _Inout_  PHIDP_KEYBOARD_MODIFIER_STATE ModifierState,
    _In_     PHIDP_INSERT_SCANCODES        InsertCodesProcedure,
    _In_opt_ PVOID                         InsertCodesContext
    )
/*++
Routine Description:
Parameters:
--*/
{
    unsigned int i;
    long result;
    
    if (UsageListLength == 0) {
        return 0x110000;
    }
    
    for (i = 0; i < UsageListLength; i++) {
        if (ChangedUsageList[i] == 0) {
            return 0x110000;
        }
        
        result = HidP_TranslateUsage(ChangedUsageList[i], KeyAction, ModifierState, InsertCodesProcedure, (ULONG*)InsertCodesContext, NULL, InsertCodesProcedure, InsertCodesContext);
        if (result != 0x110000) {
            return result;
        }
    }
    
    return 0x110000;
}

// HidP_UsageListDifference
_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS __stdcall
HidP_UsageListDifference (
   _In_reads_(UsageListLength) PUSAGE  PreviousUsageList,
   _In_reads_(UsageListLength) PUSAGE  CurrentUsageList,
   _Out_writes_(UsageListLength) PUSAGE  BreakUsageList,
   _Out_writes_(UsageListLength) PUSAGE  MakeUsageList,
   _In_ ULONG    UsageListLength
    )
/*++
Routine Description:
    This function will return the difference between a two lists of usages
    (as might be returned from HidP_GetUsages),  In other words, it will return
    return a list of usages that are in the current list but not the previous
    list as well as a list of usages that are in the previous list but not
    the current list.

Parameters:

    PreviousUsageList   The list of usages before.
    CurrentUsageList    The list of usages now.
    BreakUsageList      Previous - Current.
    MakeUsageList       Current - Previous.
    UsageListLength     Represents the length of the usage lists in array
                        elements.  If comparing two lists with a differing
                        number of array elements, this value should be
                        the size of the larger of the two lists.  Any
                        zero found with a list indicates an early termination
                        of the list and any usages found after the first zero
                        will be ignored.
--*/
{
    unsigned int i, j;
    unsigned int addedCount = 0;
    unsigned int removedCount = 0;
    unsigned short usage;
    
    // Find added usages (in new but not in old)
    for (i = 0; i < UsageListLength; i++) {
        usage = CurrentUsageList[i];
        if (usage == 0)
            break;
        
        for (j = 0; j < UsageListLength; j++) {
            if (PreviousUsageList[j] == 0)
                break;
            if (PreviousUsageList[j] == usage)
                goto next_added;
        }
        MakeUsageList[addedCount++] = usage;
    next_added:;
    }
    
    // Find removed usages (in old but not in new)
    for (i = 0; i < UsageListLength; i++) {
        usage = PreviousUsageList[i];
        if (usage == 0)
            break;
        
        for (j = 0; j < UsageListLength; j++) {
            if (CurrentUsageList[j] == 0)
                break;
            if (CurrentUsageList[j] == usage)
                goto next_removed;
        }
        BreakUsageList[removedCount++] = usage;
    next_removed:;
    }
    
    // Zero out remaining entries in MakeUsageList
    if ((ULONG_PTR)MakeUsageList < UsageListLength) {
        for ((PUSAGE)i = MakeUsageList; i < UsageListLength; i++) {
            MakeUsageList[i] = 0;
        }
    }
    
    // Zero out remaining entries in BreakUsageList
    if (removedCount < UsageListLength) {
        for (i = removedCount; i < UsageListLength; i++) {
            BreakUsageList[i] = 0;
        }
    }
    
    return 0x110000;
}

// HidP_VendorBreakCodesAsMakeCodes
unsigned char __cdecl HidP_VendorBreakCodesAsMakeCodes(ULONG* param1, unsigned char param2, void* callback, void* context, HIDP_KEYBOARD_DIRECTION direction, HIDP_KEYBOARD_MODIFIER_STATE* modifierState)
{
    if (direction != HidP_Keyboard_Break) {
        if (direction != HidP_Keyboard_Make) {
            return 0;
        }
        HidP_KbdPutKey((ULONG)callback, (HIDP_KEYBOARD_DIRECTION)context, callback, context);
    }
    
    return 1;
}

