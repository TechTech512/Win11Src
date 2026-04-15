/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

        QUERY.C

Abstract:

   HID Querying API.

Environment:

    Kernel & user mode

--*/

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <winternl.h>
#include <hidusage.h>
#include <hidclass.h>
#include <hidpi.h>
#include <hidsdi.h>

// ============================================================================
// Internal structure definitions (reverse-engineered from Windows HID parser)
// These are NOT public - for compilation purposes only
// ============================================================================

typedef struct _CHANNEL_REPORT_HEADER {
    USHORT Offset;
    USHORT Size;
    USHORT Index;
    USHORT ByteLen;
} CHANNEL_REPORT_HEADER, *PCHANNEL_REPORT_HEADER;

typedef struct _HIDP_CHANNEL_DESC {
    USHORT UsagePage;
    USHORT Usage;
    ULONG Flags;
    USHORT LinkCollection;
    USHORT LinkUsagePage;
    USHORT LinkUsage;
    USHORT BitOffset;
    USHORT BitSize;
    USHORT BitField;
    USHORT ReportCount;
    USHORT ReportID;
    UCHAR HasNull;
    UCHAR Reserved;
    ULONG Units;
    ULONG UnitsExp;
    LONG LogicalMin;
    LONG LogicalMax;
    LONG PhysicalMin;
    LONG PhysicalMax;
    USHORT UsageMin;
    USHORT UsageMax;
    USHORT DataIndexMin;
    USHORT DataIndexMax;
    USHORT StringMin;
    USHORT StringMax;
    USHORT DesignatorMin;
    USHORT DesignatorMax;
    UCHAR Reserved2[16];
    ULONG ExtendedAttributes[8];
} HIDP_CHANNEL_DESC, *PHIDP_CHANNEL_DESC;

// Internal preparsed data structure (partial, based on reverse engineering)
typedef struct _HIDP_PREPARSED_DATA_INTERNAL {
    ULONG Signature1;           // 0x50646948
    ULONG Signature2;           // 0x52444B20
    USHORT Usage;
    USHORT UsagePage;
    CHANNEL_REPORT_HEADER Input;
    CHANNEL_REPORT_HEADER Output;
    CHANNEL_REPORT_HEADER Feature;
    USHORT LinkCollectionArrayLength;
    USHORT LinkCollectionArrayOffset;
    USHORT DescriptorOffset;
    USHORT Unknown;
    // ... more internal fields
} HIDP_PREPARSED_DATA_INTERNAL, *PHIDP_PREPARSED_DATA_INTERNAL;

// ============================================================================
// External functions (provided by hid.dll or hal.dll)
// ============================================================================

extern void __cdecl HidP_InsertData(USHORT, USHORT, USHORT, char*, ULONG, ULONG);
extern ULONG __cdecl HidP_ExtractData(USHORT, USHORT, USHORT, UCHAR*);
extern long __cdecl HidP_DeleteArrayEntry(ULONG, USHORT, USHORT, ULONG, char*, ULONG);
USAGE_AND_PAGE __cdecl HidP_Index2Usage(PHIDP_CHANNEL_DESC channelDesc, ULONG dataIndex);

// ============================================================================
// Helper functions
// ============================================================================

static ULONG GetReturnCodeBasedOnFlag(void)
{
    return HIDP_STATUS_INCOMPATIBLE_REPORT_ID;  // HIDP_STATUS_INCOMPATIBLE_REPORT_ID
}

static ULONG __fastcall ProcessChannelDescriptors(ULONG callerParam, int channelArray, UCHAR* reportData)
{
    int channelOffset;
    char flagByte;
    short currentValue;
    short tempValue;
    ULONG extractResult;
    USHORT bitPosition;
    int dataIndex;
    ULONG bitCounter;
    USAGE_AND_PAGE usagePageAndCode;
    short rangeStart;
    ULONG returnValue;
    ULONG tempCounter;
    UCHAR usageMin;
    int channelDescBase = 0;
    PHIDP_CHANNEL_DESC channelDescriptor;
    int stackFramePtr;
    ULONG currentIndex = 0;
    UCHAR* dataPtr;
    USHORT stackWord;
    
    stackFramePtr = (int)&callerParam - 0x20;
    rangeStart = *(short*)(stackFramePtr + 0x10);
    *(ULONG*)(stackFramePtr - 0x18) = 0xFFFF;
    
    do {
        dataIndex = (currentIndex & 0xFFFF) * 0x68;
        channelOffset = dataIndex + channelDescBase;
        
        if ((((*(UCHAR*)(dataIndex + 0x44 + channelDescBase) & 4) != 0) &&
             ((*(short*)(stackFramePtr + 0xC) == 0 || (*(short*)(channelOffset + 0x2C) == *(short*)(stackFramePtr + 0xC)))) &&
            ((rangeStart == 0 ||
             ((tempValue = *(short*)(channelOffset + 0x3E), rangeStart == tempValue) ||
              ((*(short*)(stackFramePtr - 0x18) == rangeStart && (*(ULONG*)(stackFramePtr - 0x14) = 0, *(short*)(stackFramePtr - 0x14) == tempValue)))))))) {
            
            if ((**(char**)(stackFramePtr + 0x20) == '\0') || (*(char*)(channelOffset + 0x2E) == **(char**)(stackFramePtr + 0x20))) {
                rangeStart = *(short*)(channelOffset + 0x30);
                *(UCHAR*)(stackFramePtr - 1) = 1;
                usageMin = *(UCHAR*)(channelOffset + 0x2F);
                *(ULONG*)(stackFramePtr - 0x14) = (ULONG)*(UCHAR*)(channelOffset + 0x2F);
                
                if (rangeStart == 1) {
                    if ((*(ULONG*)(stackFramePtr - 0x14) & 0xFFFF) < (ULONG)usageMin + (ULONG)*(USHORT*)(channelOffset + 0x36)) {
                        usageMin = *(UCHAR*)(channelOffset + 0x2F);
                        bitCounter = *(ULONG*)(stackFramePtr - 0x14);
                        
                        do {
                            bitPosition = *(short*)(channelOffset + 0x34) * 8 + (USHORT)bitCounter;
                            
                            if ((USHORT)((short)*(char*)((ULONG)(bitPosition >> 3) + *(int*)(stackFramePtr + 0x20)) & 1 << ((UCHAR)bitPosition & 7)) != 0) {
                                if ((*(UCHAR*)(channelOffset + 0x44) & 0x10) == 0) {
                                    rangeStart = *(short*)(channelOffset + 0x68);
                                } else {
                                    rangeStart = (*(short*)(channelOffset + 0x68) - (USHORT)usageMin) + (short)bitCounter;
                                }
                                
                                tempCounter = *(ULONG*)(stackFramePtr - 8);
                                extractResult = tempCounter & 0xFFFF;
                                
                                if (extractResult < **(ULONG**)(stackFramePtr + 0x18)) {
                                    if (*(short*)(stackFramePtr + 0xC) == 0) {
                                        dataIndex = *(int*)(stackFramePtr + 0x14);
                                        *(USHORT*)(dataIndex + 2 + extractResult * 4) = *(USHORT*)(channelOffset + 0x2C);
                                        tempCounter = *(ULONG*)(stackFramePtr - 8);
                                        *(short*)(dataIndex + extractResult * 4) = rangeStart;
                                    } else {
                                        *(short*)(*(int*)(stackFramePtr + 0x14) + extractResult * 2) = rangeStart;
                                    }
                                }
                                
                                *(ULONG*)(stackFramePtr - 8) = tempCounter + 1;
                            }
                            
                            usageMin = *(UCHAR*)(channelOffset + 0x2F);
                            bitCounter++;
                        } while ((bitCounter & 0xFFFF) < (ULONG)usageMin + (ULONG)*(USHORT*)(channelOffset + 0x36));
                        
                        currentIndex = *(ULONG*)(stackFramePtr - 0x10);
                        channelDescBase = *(int*)(stackFramePtr + 0x1C);
                        channelArray = *(int*)(stackFramePtr - 0xC);
                    }
                } else {
                    if ((*(ULONG*)(stackFramePtr - 0x14) & 0xFFFF) < (ULONG)usageMin + (ULONG)*(USHORT*)(channelOffset + 0x36)) {
                        channelDescriptor = (PHIDP_CHANNEL_DESC)(ULONG)*(USHORT*)(channelOffset + 0x30);
                        dataPtr = *(UCHAR**)(stackFramePtr - 0x14);
                        
                        do {
                            extractResult = *(ULONG*)(stackFramePtr + 0x20);
                            stackWord = (USHORT)extractResult;
                            extractResult = HidP_ExtractData((USHORT)channelDescriptor, stackWord, (USHORT)channelDescriptor, reportData);
                            
                            if (extractResult != 0) {
                                usagePageAndCode = HidP_Index2Usage(channelDescriptor, extractResult);
                                
                                if (usagePageAndCode.Usage != 0) {
                                    bitCounter = *(ULONG*)(stackFramePtr - 8);
                                    extractResult = bitCounter & 0xFFFF;
                                    
                                    if (extractResult < **(ULONG**)(stackFramePtr + 0x18)) {
                                        if (*(short*)(stackFramePtr + 0xC) == 0) {
                                            *(USAGE_AND_PAGE*)(*(int*)(stackFramePtr + 0x14) + extractResult * 4) = usagePageAndCode;
                                        } else {
                                            *(USHORT*)(*(int*)(stackFramePtr + 0x14) + extractResult * 2) = usagePageAndCode.Usage;
                                        }
                                    }
                                    
                                    *(ULONG*)(stackFramePtr - 8) = bitCounter + 1;
                                }
                            }
                            
                            channelDescriptor = (PHIDP_CHANNEL_DESC)(ULONG)*(USHORT*)(channelOffset + 0x30);
                        } while (((ULONG)dataPtr & 0xFFFF) < (ULONG)*(USHORT*)(channelOffset + 0x36) + (ULONG)*(UCHAR*)(channelOffset + 0x2F));
                        
                        currentIndex = *(ULONG*)(stackFramePtr - 0x10);
                        channelDescBase = *(int*)(stackFramePtr + 0x1C);
                        channelArray = *(int*)(stackFramePtr - 0xC);
                    }
                    
                    usageMin = *(UCHAR*)(channelOffset + 0x44);
                    while ((usageMin & 1) != 0) {
                        currentIndex++;
                        usageMin = *(UCHAR*)((currentIndex & 0xFFFF) * 0x68 + 0x44 + channelDescBase);
                    }
                }
                
                rangeStart = *(short*)(stackFramePtr + 0x10);
            } else {
                *(UCHAR*)(stackFramePtr - 2) = 1;
            }
        }
        
        currentIndex++;
        *(ULONG*)(stackFramePtr - 0x10) = currentIndex;
    } while ((USHORT)currentIndex < *(USHORT*)(channelArray + 4));
    
    returnValue = 0xc0110007;
    bitCounter = *(ULONG*)(stackFramePtr - 8) & 0xFFFF;
    
    if (bitCounter <= **(ULONG**)(stackFramePtr + 0x18)) {
        returnValue = HIDP_STATUS_SUCCESS;
    }
    
    flagByte = *(char*)(stackFramePtr - 1);
    **(ULONG**)(stackFramePtr + 0x18) = bitCounter;
    
    if (flagByte == '\0') {
        returnValue = GetReturnCodeBasedOnFlag();
    }
    
    return returnValue;
}

static ULONG __fastcall ProcessUsageList(ULONG callerParam, USHORT* usageListPtr)
{
    char flagByte;
    USHORT value1;
    ULONG returnCode;
    int stackFramePtr;
    ULONG currentCount = 0;
    UCHAR* returnAddress = NULL;
    
    stackFramePtr = (int)&callerParam - 0x18;
    value1 = usageListPtr[3];
    *(USHORT**)(stackFramePtr - 0xC) = usageListPtr;
    
    if (*(USHORT*)(stackFramePtr + 0x24) == value1) {
        if (value1 == 0) {
            return 0xc0110010;
        }
        
        value1 = *usageListPtr;
        *(ULONG*)(stackFramePtr - 0x10) = (ULONG)value1;
        
        if (value1 < usageListPtr[2]) {
            returnCode = ProcessChannelDescriptors(0, (int)usageListPtr, returnAddress);
            return returnCode;
        }
        
        returnCode = 0xc0110007;
        
        if ((currentCount & 0xFFFF) <= **(ULONG**)(stackFramePtr + 0x18)) {
            returnCode = HIDP_STATUS_SUCCESS;
        }
        
        flagByte = *(char*)(stackFramePtr - 1);
        **(ULONG**)(stackFramePtr + 0x18) = currentCount & 0xFFFF;
        
        if (flagByte == '\0') {
            returnCode = GetReturnCodeBasedOnFlag();
        }
        
        return returnCode;
    } else {
        returnCode = 0xc0110003;
    }
    
    return returnCode;
}

static ULONG ProcessArrayInsertions(ULONG param1)
{
    UCHAR flagByte;
    USHORT value1;
    USHORT value2;
    int offset1;
    int offset2;
    char* charPtr;
    ULONG returnCode;
    ULONG counter;
    int channelBase;
    int stackFramePtr;
    int* channelPtr;
    ULONG currentIndex;
    ULONG paramValue;
    
    stackFramePtr = (int)&param1 - 0x18;
    channelBase = *(int*)(stackFramePtr + 0x14);
    currentIndex = *(ULONG*)(stackFramePtr + 0x10);
    
    while (1) {
        do {
            do {
                channelPtr = (int*)currentIndex;
                value1 = *(USHORT*)(channelBase + 4);
                currentIndex++;
                channelPtr += 0x1A;
                *(ULONG*)(stackFramePtr - 0x14) = currentIndex;
                
                if (value1 <= currentIndex) {
                    return *(ULONG*)(stackFramePtr - 0x18);
                }
            } while (((*(char*)((int)channelPtr + 0x1A) != *(char*)(stackFramePtr - 1)) ||
                     (flagByte = *(UCHAR*)(channelPtr + 0xC), *(ULONG*)(stackFramePtr - 0x18) = 0x110000,
                     (flagByte & 0x26) != 0)) || ((char)channelPtr[0x19] == '\0'));
            
            if ((short)channelPtr[7] == 0x20) {
                *(ULONG*)(stackFramePtr - 0xC) = 0xFFFFFFFF;
            } else {
                *(int*)(stackFramePtr - 0xC) = (1 << ((UCHAR)(short)channelPtr[7] & 0x1F)) - 1;
            }
            
            offset1 = channelPtr[0x1B];
            offset2 = *channelPtr;
            
            if (offset1 < offset2) {
                charPtr = (char*)0x0;
            } else {
                charPtr = (char*)(offset2 - 1U & *(ULONG*)(stackFramePtr - 0xC));
            }
            
            *(char**)(stackFramePtr - 0xC) = charPtr;
        } while (((offset2 <= (int)charPtr) && ((int)charPtr <= offset1)) || (charPtr == (char*)0x0));
        
        if ((*(UCHAR*)(channelPtr + 0xC) & 0x10) == 0) break;
        
        value1 = *(USHORT*)(channelPtr + 8);
        flagByte = *(UCHAR*)((int)channelPtr + 0x1B);
        *(ULONG*)(stackFramePtr - 0x10) = 0;
        *(ULONG*)(stackFramePtr - 0x1C) = (ULONG)flagByte + (ULONG)value1 * 8;
        
        if (*(short*)((int)channelPtr + 0x1E) != 0) {
            charPtr = *(char**)(stackFramePtr - 0xC);
            value1 = *(USHORT*)(channelPtr + 7);
            
            do {
                paramValue = *(ULONG*)(stackFramePtr + 0xC);
                HidP_InsertData(value1, (USHORT)paramValue, (USHORT)*(ULONG*)(stackFramePtr + 0x10), charPtr, (ULONG)value1, param1);
                value1 = *(USHORT*)(channelPtr + 7);
                counter = *(int*)(stackFramePtr - 0x10) + 1;
                value2 = *(USHORT*)((int)channelPtr + 0x1E);
                *(ULONG*)(stackFramePtr - 0x10) = counter;
            } while (counter < value2);
            
            returnCode = ProcessArrayInsertions(paramValue);
            return returnCode;
        }
    }
    
    paramValue = *(ULONG*)(stackFramePtr + 0xC);
    HidP_InsertData(*(USHORT*)(channelPtr + 7), (USHORT)paramValue, (USHORT)*(ULONG*)(stackFramePtr + 0x10), charPtr, (ULONG)0, param1);
    returnCode = ProcessArrayInsertions(paramValue);
    return returnCode;
}

ULONG ProcessChannelData(char param1, ULONG param2)
{
    UCHAR flagByte;
    USHORT value1;
    USHORT value2;
    int offset1;
    int offset2;
    short currentValue;
    char* charPtr;
    ULONG returnCode;
    ULONG counter;
    int channelBase;
    int stackFramePtr;
    int* channelPtr;
    ULONG currentIndex;
    ULONG paramValue;
    
    stackFramePtr = (int)&param1 - 0x1C;
    channelBase = *(int*)(stackFramePtr + 0x18);
    currentValue = *(short*)(stackFramePtr + 0x10);
    currentIndex = *(ULONG*)(stackFramePtr + 0x14);
    channelPtr = (int*)(currentIndex * 0x68 + channelBase + 0x44);
    
    do {
        if (((*(char*)((int)channelPtr - 0x4E) == param1) &&
            (flagByte = *(UCHAR*)(channelPtr - 0xE), *(ULONG*)(stackFramePtr - 0x18) = 0x110000,
            (flagByte & 0x26) == 0)) && ((char)channelPtr[-1] != '\0')) {
            
            if (currentValue == (short)channelPtr[-0x13]) {
                *(ULONG*)(stackFramePtr - 0xC) = 0xFFFFFFFF;
            } else {
                *(int*)(stackFramePtr - 0xC) = (1 << ((UCHAR)(short)channelPtr[-0x13] & 0x1F)) - 1;
            }
            
            offset1 = channelPtr[1];
            offset2 = *channelPtr;
            
            if (offset1 < offset2) {
                charPtr = (char*)0x0;
            } else {
                charPtr = (char*)(offset2 - 1U & *(ULONG*)(stackFramePtr - 0xC));
            }
            
            *(char**)(stackFramePtr - 0xC) = charPtr;
            
            if ((((int)charPtr < offset2) || (offset1 < (int)charPtr)) && (charPtr != (char*)0x0)) {
                if ((*(UCHAR*)(channelPtr - 0xE) & 0x10) == 0) {
                    paramValue = *(ULONG*)(stackFramePtr + 0xC);
                    HidP_InsertData(*(USHORT*)(channelPtr - 0x13), (USHORT)paramValue, (USHORT)*(ULONG*)(stackFramePtr + 0x10), charPtr, (ULONG)0, param2);
                    returnCode = ProcessArrayInsertions(paramValue);
                    return returnCode;
                }
                
                value1 = *(USHORT*)(channelPtr - 0x12);
                flagByte = *(UCHAR*)((int)channelPtr - 0x4D);
                *(ULONG*)(stackFramePtr - 0x10) = 0;
                *(ULONG*)(stackFramePtr - 0x1C) = (ULONG)flagByte + (ULONG)value1 * 8;
                
                if (*(short*)((int)channelPtr - 0x4A) != 0) {
                    charPtr = *(char**)(stackFramePtr - 0xC);
                    value1 = *(USHORT*)(channelPtr - 0x13);
                    
                    do {
                        paramValue = *(ULONG*)(stackFramePtr + 0xC);
                        HidP_InsertData(value1, (USHORT)paramValue, (USHORT)*(ULONG*)(stackFramePtr + 0x10), charPtr, (ULONG)value1, param2);
                        value1 = *(USHORT*)(channelPtr - 0x13);
                        counter = *(int*)(stackFramePtr - 0x10) + 1;
                        value2 = *(USHORT*)((int)channelPtr - 0x4A);
                        *(ULONG*)(stackFramePtr - 0x10) = counter;
                    } while (counter < value2);
                    
                    returnCode = ProcessArrayInsertions(paramValue);
                    return returnCode;
                }
            }
        }
        
        value1 = *(USHORT*)(channelBase + 4);
        currentIndex++;
        channelPtr += 0x1A;
        *(ULONG*)(stackFramePtr - 0x14) = currentIndex;
        currentValue = 0x20;
        param1 = *(char*)(stackFramePtr - 1);
        
        if (value1 <= currentIndex) {
            return *(ULONG*)(stackFramePtr - 0x18);
        }
    } while (1);
}

static ULONG InitializeReportData(void)
{
    char flagByte;
    USHORT value1;
    USHORT value2;
    char* charPtr;
    ULONG returnCode;
    USHORT* usageListPtr = NULL;
    ULONG returnAddress = 0;
    
    value1 = usageListPtr[3];
    
    if (*(USHORT*)((char*)usageListPtr + 0x10) == value1) {
        if (value1 == 0) {
            return 0xc0110010;
        }
        
        charPtr = *(char**)((char*)usageListPtr + 0xC);
        memset(charPtr, 0, (size_t)value1);
        flagByte = 0;
        *charPtr = flagByte;
        value1 = *usageListPtr;
        value2 = usageListPtr[2];
        
        if ((ULONG)value1 < (ULONG)value2) {
            returnCode = ProcessChannelData(flagByte, returnAddress);
            return returnCode;
        }
        
        returnCode = 0;
    } else {
        returnCode = 0xc0110003;
    }
    
    return returnCode;
}

static ULONG __fastcall ExtractUsageValue(USHORT* param1, int param2)
{
    char flagByte = 0;
    short value1 = 0;
    USHORT value2;
    ULONG counter;
    ULONG* ptr;
    ULONG extractResult;
    short tempShort;
    UCHAR* channelBase = NULL;
    ULONG currentIndex = 0;
    USHORT diValue = 0;
    
    if (*(USHORT*)((char*)param1 + 0x24) != param1[3]) {
        return 0xc0110003;
    }
    
    if (param1[3] == 0) {
        return 0xc0110010;
    }
    
    currentIndex = (ULONG)*param1;
    value2 = param1[2];
    
    if (currentIndex < value2) {
        tempShort = 0;
        ptr = (ULONG*)(currentIndex * 0x68 + 0x44 + param2);
        
        do {
            counter = *ptr;
            currentIndex++;
            ptr += 0x1A;
        } while (currentIndex < value2);
        
        if (flagByte != '\0') {
            return 0xc011000a;
        }
    }
    
    return HIDP_STATUS_USAGE_NOT_FOUND;  // HIDP_STATUS_USAGE_NOT_FOUND
}

// ============================================================================
// Public HID API functions
// ============================================================================

// HidP_DeleteArrayEntry
long __cdecl HidP_DeleteArrayEntry(ULONG arrayCount, USHORT bitOffset, USHORT reportId, ULONG usageIndex, char* reportData, ULONG reportLength)
{
    ULONG extractedValue;
    ULONG currentIndex;
    ULONG count = arrayCount & 0xFFFF;
    USHORT currentBitOffset = bitOffset;
    USHORT currentReportId = reportId;
    UCHAR* reportBuffer = (UCHAR*)reportData;
    USHORT targetUsageIndex = (USHORT)usageIndex;
    
    for (currentIndex = 0; currentIndex < count; currentIndex++) {
        extractedValue = HidP_ExtractData(currentBitOffset, currentReportId, currentBitOffset, reportBuffer);
        if (targetUsageIndex == (USHORT)extractedValue) {
            break;
        }
    }
    
    if (currentIndex >= count) {
        return HIDP_STATUS_NOT_VALUE_ARRAY;
    }
    
    if (count - currentIndex > 1) {
        for (ULONG moveIndex = currentIndex; moveIndex < count - 1; moveIndex++) {
            extractedValue = HidP_ExtractData(currentBitOffset, currentReportId, currentBitOffset, reportBuffer);
            HidP_InsertData(currentBitOffset, currentReportId, currentReportId, (char*)(ULONG_PTR)extractedValue, (ULONG)(ULONG_PTR)reportBuffer, (ULONG)(ULONG_PTR)currentBitOffset);
        }
    }
    
    HidP_InsertData(currentBitOffset, currentReportId, currentReportId, NULL, (ULONG)(ULONG_PTR)reportBuffer, (ULONG)(ULONG_PTR)reportBuffer);
    return HIDP_STATUS_SUCCESS;
}

// HidP_ExtractData
ULONG __cdecl HidP_ExtractData(USHORT bitOffset, USHORT reportId, USHORT param3, UCHAR* reportData)
{
    ULONG result = 0;
    USHORT currentBitPos = bitOffset;
    USHORT currentBytePos = bitOffset >> 3;
    UCHAR remainingBits = bitOffset & 7;
    UCHAR byteValue;
    
    if (remainingBits != 0) {
        byteValue = reportData[currentBytePos];
        result = (byteValue >> remainingBits) & ((1 << (8 - remainingBits)) - 1);
        currentBytePos++;
        currentBitPos += 8;
    }
    
    while (currentBitPos >= 8 && currentBytePos < 65535) {
        result |= (reportData[currentBytePos] << (currentBitPos & 7));
        currentBytePos++;
        currentBitPos -= 8;
    }
    
    if (currentBitPos != 0) {
        result = (result << currentBitPos) | ((reportData[currentBytePos] >> (8 - currentBitPos)) & ((1 << currentBitPos) - 1));
    }
    
    return result;
}

// HidP_GetButtonCaps
_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS __stdcall
HidP_GetButtonCaps (
   _In_       HIDP_REPORT_TYPE     ReportType,
   _Out_writes_to_(*ButtonCapsLength, *ButtonCapsLength) PHIDP_BUTTON_CAPS ButtonCaps,
   _Inout_    PUSHORT              ButtonCapsLength,
   _In_       PHIDP_PREPARSED_DATA PreparsedData
)
{
    return HidP_GetSpecificButtonCaps(ReportType, 0, 0, 0, ButtonCaps, ButtonCapsLength, PreparsedData);
}

// HidP_GetCaps
_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS __stdcall
HidP_GetCaps (
   _In_      PHIDP_PREPARSED_DATA      PreparsedData,
   _Out_     PHIDP_CAPS                Capabilities
   )
/*++
Routine Description:
   Returns a list of capabilities of a given hid device as described by its
   preparsed data.

Arguments:
   PreparsedData    The preparsed data returned from HIDCLASS.
   Capabilities     a HIDP_CAPS structure

Return Value:
   HIDP_STATUS_SUCCESS
   HIDP_STATUS_INVALID_PREPARSED_DATA
--*/
{
    PHIDP_PREPARSED_DATA_INTERNAL pData = (PHIDP_PREPARSED_DATA_INTERNAL)PreparsedData;
    
    if (PreparsedData == NULL || pData->Signature1 != 0x50646948 || pData->Signature2 != 0x52444B20) {
        return HIDP_STATUS_INVALID_PREPARSED_DATA;
    }
    
    memset(Capabilities, 0, sizeof(HIDP_CAPS));
    
    Capabilities->Usage = pData->Usage;
    Capabilities->UsagePage = pData->UsagePage;
    Capabilities->InputReportByteLength = pData->Input.ByteLen;
    Capabilities->OutputReportByteLength = pData->Output.ByteLen;
    Capabilities->FeatureReportByteLength = pData->Feature.ByteLen;
    Capabilities->NumberLinkCollectionNodes = pData->LinkCollectionArrayLength;
    
    Capabilities->NumberInputButtonCaps = 0;
    Capabilities->NumberInputValueCaps = 0;
    Capabilities->NumberOutputButtonCaps = 0;
    Capabilities->NumberOutputValueCaps = 0;
    Capabilities->NumberFeatureButtonCaps = 0;
    Capabilities->NumberFeatureValueCaps = 0;
    Capabilities->NumberInputDataIndices = 0;
    Capabilities->NumberOutputDataIndices = 0;
    Capabilities->NumberFeatureDataIndices = 0;
    
    PHIDP_CHANNEL_DESC channelDesc = (PHIDP_CHANNEL_DESC)((BYTE*)PreparsedData + pData->DescriptorOffset);
    
    for (USHORT index = pData->Input.Offset; index < pData->Input.Index; index++) {
        if ((channelDesc[index].Flags & 4) == 0) {
            Capabilities->NumberInputValueCaps++;
        } else {
            Capabilities->NumberInputButtonCaps++;
        }
        Capabilities->NumberInputDataIndices += (channelDesc[index].UsageMax - channelDesc[index].UsageMin) + 1;
    }
    
    for (USHORT index = pData->Output.Offset; index < pData->Output.Index; index++) {
        if ((channelDesc[index].Flags & 4) == 0) {
            Capabilities->NumberOutputValueCaps++;
        } else {
            Capabilities->NumberOutputButtonCaps++;
        }
        Capabilities->NumberOutputDataIndices += (channelDesc[index].UsageMax - channelDesc[index].UsageMin) + 1;
    }
    
    for (USHORT index = pData->Feature.Offset; index < pData->Feature.Index; index++) {
        if ((channelDesc[index].Flags & 4) == 0) {
            Capabilities->NumberFeatureValueCaps++;
        } else {
            Capabilities->NumberFeatureButtonCaps++;
        }
        Capabilities->NumberFeatureDataIndices += (channelDesc[index].UsageMax - channelDesc[index].UsageMin) + 1;
    }
    
    return HIDP_STATUS_SUCCESS;
}

// HidP_GetData
long __stdcall HidP_GetData(HIDP_REPORT_TYPE ReportType, PHIDP_DATA DataList, PULONG DataLength, 
                            PHIDP_PREPARSED_DATA PreparsedData, PCHAR Report, ULONG ReportLength)
{
    PHIDP_PREPARSED_DATA_INTERNAL pData = (PHIDP_PREPARSED_DATA_INTERNAL)PreparsedData;
    
    if (PreparsedData == NULL || pData->Signature1 != 0x50646948 || pData->Signature2 != 0x52444B20) {
        return HIDP_STATUS_INVALID_PREPARSED_DATA;
    }
    
    PCHANNEL_REPORT_HEADER reportHeader;
    
    if (ReportType == HidP_Input) {
        reportHeader = &pData->Input;
    } else if (ReportType == HidP_Output) {
        reportHeader = &pData->Output;
    } else if (ReportType == HidP_Feature) {
        reportHeader = &pData->Feature;
    } else {
        return HIDP_STATUS_INVALID_REPORT_TYPE;
    }
    
    if ((USHORT)ReportLength != reportHeader->ByteLen) {
        return HIDP_STATUS_INVALID_REPORT_LENGTH;
    }
    
    if (reportHeader->ByteLen == 0) {
        return HIDP_STATUS_REPORT_DOES_NOT_EXIST;
    }
    
    PHIDP_CHANNEL_DESC channelDesc = (PHIDP_CHANNEL_DESC)((BYTE*)PreparsedData + pData->DescriptorOffset);
    ULONG dataIndex = 0;
    ULONG maxDataLength = *DataLength;
    
    for (USHORT channelIndex = reportHeader->Offset; channelIndex < reportHeader->Index; channelIndex++) {
        PHIDP_CHANNEL_DESC currentChannel = &channelDesc[channelIndex];
        
        if (Report[0] == 0 || currentChannel->ReportID == (UCHAR)Report[0]) {
            if ((currentChannel->Flags & 4) == 0) {
                USHORT bitCount = currentChannel->BitSize;
                
                for (USHORT bit = 0; bit < bitCount; bit++) {
                    if (dataIndex < maxDataLength) {
                        ULONG value = HidP_ExtractData(currentChannel->BitOffset + bit, (USHORT)(UCHAR)Report[0], currentChannel->BitOffset, (UCHAR*)Report);
                        DataList[dataIndex].RawValue = value;
                        DataList[dataIndex].DataIndex = currentChannel->DataIndexMin + bit;
                    }
                    dataIndex++;
                }
            } else if (currentChannel->BitSize == 1) {
                USHORT startBit = currentChannel->BitOffset;
                USHORT bitCount = currentChannel->UsageMax - currentChannel->UsageMin + 1;
                
                for (USHORT bit = 0; bit < bitCount; bit++) {
                    USHORT bytePosition = (startBit + bit) >> 3;
                    UCHAR bitPosition = (startBit + bit) & 7;
                    
                    if ((Report[bytePosition] & (1 << bitPosition)) != 0) {
                        if (dataIndex < maxDataLength) {
                            DataList[dataIndex].On = 1;
                            if ((currentChannel->Flags & 0x10) == 0) {
                                DataList[dataIndex].DataIndex = currentChannel->DataIndexMin + bit;
                            } else {
                                DataList[dataIndex].DataIndex = currentChannel->DataIndexMin;
                            }
                        }
                        dataIndex++;
                    }
                }
            } else {
                USHORT bitCount = currentChannel->UsageMax - currentChannel->UsageMin + 1;
                USHORT bitStep = currentChannel->BitSize;
                
                for (USHORT bit = 0; bit < bitCount; bit++) {
                    ULONG value = HidP_ExtractData(currentChannel->BitOffset + (bit * bitStep), (USHORT)(UCHAR)Report[0], currentChannel->BitOffset, (UCHAR*)Report);
                    
                    if ((value & 0xFFFF) != 0) {
                        if (dataIndex < maxDataLength) {
                            DataList[dataIndex].On = 1;
                            DataList[dataIndex].DataIndex = currentChannel->DataIndexMin + (USHORT)value - 1;
                            if (currentChannel->Flags & 0x10) {
                                DataList[dataIndex].DataIndex++;
                            }
                        }
                        dataIndex++;
                    }
                }
            }
        }
    }
    
    if (maxDataLength < dataIndex) {
        *DataLength = dataIndex;
        return HIDP_STATUS_BUFFER_TOO_SMALL;
    }
    
    *DataLength = dataIndex;
    return HIDP_STATUS_SUCCESS;
}

// HidP_GetExtendedAttributes
_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS __stdcall
HidP_GetExtendedAttributes (
    _In_      HIDP_REPORT_TYPE            ReportType,
    _In_      USHORT                      DataIndex,
    _In_      PHIDP_PREPARSED_DATA        PreparsedData,
    _Out_writes_to_(*LengthAttributes, *LengthAttributes) PHIDP_EXTENDED_ATTRIBUTES Attributes,
    _Inout_   PULONG                      LengthAttributes
    )
{
    PHIDP_PREPARSED_DATA_INTERNAL pData = (PHIDP_PREPARSED_DATA_INTERNAL)PreparsedData;
    
    if (PreparsedData == NULL || pData->Signature1 != 0x50646948 || pData->Signature2 != 0x52444B20) {
        return HIDP_STATUS_INVALID_PREPARSED_DATA;
    }
    
    PCHANNEL_REPORT_HEADER reportHeader;
    
    if (ReportType == HidP_Input) {
        reportHeader = &pData->Input;
    } else if (ReportType == HidP_Output) {
        reportHeader = &pData->Output;
    } else if (ReportType == HidP_Feature) {
        reportHeader = &pData->Feature;
    } else {
        return HIDP_STATUS_INVALID_REPORT_TYPE;
    }
    
    PHIDP_CHANNEL_DESC channelDesc = (PHIDP_CHANNEL_DESC)((BYTE*)PreparsedData + pData->DescriptorOffset);
    
    for (USHORT channelIndex = reportHeader->Offset; channelIndex < reportHeader->Index; channelIndex++) {
        PHIDP_CHANNEL_DESC currentChannel = &channelDesc[channelIndex];
        
        if (currentChannel->DataIndexMin <= DataIndex && DataIndex <= currentChannel->DataIndexMax) {
            ULONG extendedSize = ((currentChannel->Flags >> 28) * 8) + 8;
            
            memset(Attributes, 0, *LengthAttributes);
            memcpy(Attributes, &currentChannel->ExtendedAttributes, (extendedSize < *LengthAttributes) ? extendedSize : *LengthAttributes);
            
            if (*LengthAttributes < extendedSize) {
                return HIDP_STATUS_BUFFER_TOO_SMALL;
            }
            
            return HIDP_STATUS_SUCCESS;
        }
    }
    
    return HIDP_STATUS_USAGE_NOT_FOUND;
}

// HidP_GetLinkCollectionNodes
_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS __stdcall
HidP_GetLinkCollectionNodes (
   _Out_writes_to_(*LinkCollectionNodesLength, *LinkCollectionNodesLength)     PHIDP_LINK_COLLECTION_NODE LinkCollectionNodes,
   _Inout_   PULONG                     LinkCollectionNodesLength,
   _In_      PHIDP_PREPARSED_DATA       PreparsedData
   )
/*++
Routine Description:
   Return a list of PHIDP_LINK_COLLECTION_NODEs used to describe the link
   collection tree of this hid device.  See the above description of
   struct _HIDP_LINK_COLLECTION_NODE.

Arguments:
   LinkCollectionNodes - a caller allocated array into which
                 HidP_GetLinkCollectionNodes will store the information

   LinKCollectionNodesLength - the caller sets this value to the length of the
                 the array in terms of number of elements.
                 HidP_GetLinkCollectionNodes sets this value to the actual
                 number of elements set. The total number of nodes required to
                 describe this HID device can be found in the
                 NumberLinkCollectionNodes field in the HIDP_CAPS structure.

--*/
{
    PHIDP_PREPARSED_DATA_INTERNAL pData = (PHIDP_PREPARSED_DATA_INTERNAL)PreparsedData;
    
    if (PreparsedData == NULL || pData->Signature1 != 0x50646948 || pData->Signature2 != 0x52444B20) {
        return HIDP_STATUS_INVALID_PREPARSED_DATA;
    }
    
    ULONG nodeCount = pData->LinkCollectionArrayLength;
    ULONG copyCount = nodeCount;
    
    if (*LinkCollectionNodesLength < nodeCount) {
        copyCount = *LinkCollectionNodesLength;
    }
    
    *LinkCollectionNodesLength = nodeCount;
    
    if (copyCount > 0 && LinkCollectionNodes != NULL) {
        PHIDP_LINK_COLLECTION_NODE sourceNodes = (PHIDP_LINK_COLLECTION_NODE)((BYTE*)PreparsedData + pData->LinkCollectionArrayOffset);
        memcpy(LinkCollectionNodes, sourceNodes, copyCount * sizeof(HIDP_LINK_COLLECTION_NODE));
    }
    
    if (*LinkCollectionNodesLength < nodeCount) {
        return HIDP_STATUS_BUFFER_TOO_SMALL;
    }
    
    return HIDP_STATUS_SUCCESS;
}

// HidP_GetScaledUsageValue
_Must_inspect_result_
NTSTATUS __stdcall
HidP_GetScaledUsageValue (
    _In_ HIDP_REPORT_TYPE ReportType,
    _In_ USAGE UsagePage,
    _In_opt_ USHORT LinkCollection,
    _In_ USAGE Usage,
    _Out_ PLONG UsageValue,
    _In_ PHIDP_PREPARSED_DATA PreparsedData,
    _In_reads_bytes_(ReportLength) PCHAR Report,
    _In_ ULONG ReportLength
    )
/*++
Description
    HidP_GetScaledUsageValue retrieves a UsageValue from the HID report packet
    in the field corresponding to the given usage page and usage.  If a report
    packet contains two different fields with the same Usage and UsagePage,
    they can be distinguished with the optional LinkCollection field value.

    If the specified field has a defined physical range, this function converts
    the logical value that exists in the report packet to the corresponding
    physical value.  If a physical range does not exist, the function will
    return the logical value.  This function will check to verify that the
    logical value in the report falls within the declared logical range.

    When doing the conversion between logical and physical values, this
    function assumes a linear extrapolation between the physical max/min and
    the logical max/min. (Where logical is the values reported by the device
    and physical is the value returned by this function).  If the data field
    size is less than 32 bits, then HidP_GetScaledUsageValue will sign extend
    the value to 32 bits.

    If the range checking fails but the field has NULL values, the function
    will set UsageValue to 0 and return HIDP_STATUS_NULL.  Otherwise, it
    returns a HIDP_STATUS_OUT_OF_RANGE error.

Parameters:

    ReportType  One of HidP_Output or HidP_Feature.

    UsagePage   The usage page to which the given usage refers.

    LinkCollection  (Optional)  This value can be used to differentiate
                                between two fields that may have the same
                                UsagePage and Usage but exist in different
                                collections.  If the link collection value
                                is zero, this function will retrieve the first
                                field it finds that matches the usage page
                                and usage.

    Usage       The usage whose value HidP_GetScaledUsageValue will retrieve

    UsageValue  The value retrieved from the report buffer.  See the routine
                description above for the different interpretations of this
                value

    PreparsedData The preparsed data returned from HIDCLASS

    Report      The report packet.

    ReportLength Length (in bytes) of the given report packet.


Return Value:
   HidP_GetScaledUsageValue returns the following error codes:

  HIDP_STATUS_SUCCESS                -- upon successfully retrieving the value
                                        from the report packet
  HIDP_STATUS_NULL                   -- if the report packet had a NULL value
                                        set
  HIDP_STATUS_INVALID_REPORT_TYPE    -- if ReportType is not valid.
  HIDP_STATUS_INVALID_PREPARSED_DATA -- if PreparsedData is not valid
  HIDP_STATUS_INVALID_REPORT_LENGTH  -- the length of the report packet is not
                                        equal to the length specified in
                                        the HIDP_CAPS structure for the given
                                        ReportType
  HIDP_STATUS_VALUE_OUT_OF_RANGE     -- if the value retrieved from the packet
                                        falls outside the logical range and
                                        the field does not support NULL values
  HIDP_STATUS_BAD_LOG_PHY_VALUES     -- if the field has a physical range but
                                        either the logical range is invalid
                                        (max <= min) or the physical range is
                                        invalid
  HIDP_STATUS_INCOMPATIBLE_REPORT_ID -- the specified usage page, usage and
                                        link collection exist but exists in
                                        a report with a different report ID
                                        than the report being passed in.  To
                                        set this value, call
                                        HidP_GetScaledUsageValue with a
                                        different report packet
  HIDP_STATUS_USAGE_NOT_FOUND        -- if the usage page, usage, and link
                                        collection combination does not exist
                                        in any reports for this ReportType
--*/
{
    PHIDP_PREPARSED_DATA_INTERNAL pData = (PHIDP_PREPARSED_DATA_INTERNAL)PreparsedData;
    
    if (PreparsedData == NULL || pData->Signature1 != 0x50646948 || pData->Signature2 != 0x52444B20) {
        return HIDP_STATUS_INVALID_PREPARSED_DATA;
    }
    
    PCHANNEL_REPORT_HEADER reportHeader;
    
    if (ReportType == HidP_Input) {
        reportHeader = &pData->Input;
    } else if (ReportType == HidP_Output) {
        reportHeader = &pData->Output;
    } else if (ReportType == HidP_Feature) {
        reportHeader = &pData->Feature;
    } else {
        return HIDP_STATUS_INVALID_REPORT_TYPE;
    }
    
    if ((USHORT)ReportLength != reportHeader->ByteLen) {
        return HIDP_STATUS_INVALID_REPORT_LENGTH;
    }
    
    if (reportHeader->ByteLen == 0) {
        return HIDP_STATUS_REPORT_DOES_NOT_EXIST;
    }
    
    PHIDP_CHANNEL_DESC channelDesc = (PHIDP_CHANNEL_DESC)((BYTE*)PreparsedData + pData->DescriptorOffset);
    BOOL foundMismatch = FALSE;
    
    for (USHORT channelIndex = reportHeader->Offset; channelIndex < reportHeader->Index; channelIndex++) {
        PHIDP_CHANNEL_DESC currentChannel = &channelDesc[channelIndex];
        
        if (((currentChannel->Flags & 4) == 0) && currentChannel->UsagePage == UsagePage) {
            if ((LinkCollection == 0 || currentChannel->LinkCollection == LinkCollection) &&
                ((Usage == 0 && currentChannel->LinkUsage == 0) || 
                 (LinkCollection != 0 && currentChannel->LinkCollection == LinkCollection))) {
                
                if ((currentChannel->Flags & 0x10) == 0) {
                    if (currentChannel->Usage == Usage) {
                        if (Report[0] == 0 || currentChannel->ReportID == (UCHAR)Report[0]) {
                            ULONG rawValue = HidP_ExtractData(currentChannel->BitOffset, (USHORT)(UCHAR)Report[0], currentChannel->BitOffset, (UCHAR*)Report);
                            ULONG signBit = 1 << (currentChannel->BitSize - 1);
                            
                            if ((rawValue & signBit) != 0) {
                                rawValue = ~rawValue + 1;
                            }
                            
                            if (currentChannel->LogicalMin == 0 && currentChannel->LogicalMax == 0) {
                                if (currentChannel->PhysicalMin == currentChannel->PhysicalMax) {
                                    *UsageValue = 0;
                                    return HIDP_STATUS_VALUE_OUT_OF_RANGE;
                                }
                                *UsageValue = (LONG)rawValue;
                            } else {
                                if (currentChannel->LogicalMax <= currentChannel->LogicalMin || 
                                    currentChannel->PhysicalMax <= currentChannel->PhysicalMin) {
                                    *UsageValue = 0;
                                    return HIDP_STATUS_VALUE_OUT_OF_RANGE;
                                }
                                
                                LONG logicalRange = currentChannel->LogicalMax - currentChannel->LogicalMin;
                                LONG physicalRange = currentChannel->PhysicalMax - currentChannel->PhysicalMin;
                                *UsageValue = currentChannel->PhysicalMin + ((logicalRange + 1) * ((LONG)rawValue - currentChannel->LogicalMin)) / (physicalRange + 1);
                            }
                            
                            if (currentChannel->LogicalMin <= (LONG)rawValue && (LONG)rawValue <= currentChannel->LogicalMax) {
                                return HIDP_STATUS_SUCCESS;
                            }
                            
                            *UsageValue = 0;
                            return HIDP_STATUS_VALUE_OUT_OF_RANGE;
                        }
                        foundMismatch = TRUE;
                    }
                } else if (currentChannel->UsageMin <= Usage && Usage <= currentChannel->UsageMax) {
                    if (Report[0] == 0 || currentChannel->ReportID == (UCHAR)Report[0]) {
                        return HIDP_STATUS_INCOMPATIBLE_REPORT_ID;
                    }
                    foundMismatch = TRUE;
                }
            }
        }
    }
    
    if (foundMismatch) {
        return HIDP_STATUS_INCOMPATIBLE_REPORT_ID;
    }
    
    return HIDP_STATUS_USAGE_NOT_FOUND;
}

// HidP_GetSpecificButtonCaps
_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS __stdcall
HidP_GetSpecificButtonCaps (
   _In_       HIDP_REPORT_TYPE     ReportType,
   _In_opt_   USAGE                UsagePage,      // Optional (0 => ignore)
   _In_opt_   USHORT               LinkCollection, // Optional (0 => ignore)
   _In_opt_   USAGE                Usage,          // Optional (0 => ignore)
   _Out_writes_to_(*ButtonCapsLength, *ButtonCapsLength) PHIDP_BUTTON_CAPS ButtonCaps,
   _Inout_    PUSHORT              ButtonCapsLength,
   _In_       PHIDP_PREPARSED_DATA PreparsedData
   )
/*++
Description:
   HidP_GetButtonCaps returns all the buttons (binary values) that are a part
   of the given report type for the Hid device represented by the given
   preparsed data.

Parameters:
   ReportType  One of HidP_Input, HidP_Output, or HidP_Feature.

   UsagePage   A usage page value used to limit the button caps returned to
                those on a given usage page.  If set to 0, this parameter is
                ignored.  Can be used with LinkCollection and Usage parameters
                to further limit the number of button caps structures returned.

   LinkCollection HIDP_LINK_COLLECTION node array index used to limit the
                  button caps returned to those buttons in a given link
                  collection.  If set to 0, this parameter is
                  ignored.  Can be used with UsagePage and Usage parameters
                  to further limit the number of button caps structures
                  returned.

   Usage       A usage value used to limit the button caps returned to those
               with the specified usage value.  If set to 0, this parameter
               is ignored.  Can be used with LinkCollection and UsagePage
               parameters to further limit the number of button caps
               structures returned.

   ButtonCaps  A _HIDP_BUTTON_CAPS array containing information about all the
               binary values in the given report.  This buffer is provided by
               the caller.

   ButtonCapsLength   As input, this parameter specifies the length of the
                  ButtonCaps parameter (array) in number of array elements.
                  As output, this value is set to indicate how many of those
                  array elements were filled in by the function.  The maximum number of
                  button caps that can be returned is found in the HIDP_CAPS
                  structure.  If HIDP_STATUS_BUFFER_TOO_SMALL is returned,
                  this value contains the number of array elements needed to
                  successfully complete the request.

   PreparsedData  The preparsed data returned from HIDCLASS.


Return Value
HidP_GetSpecificButtonCaps returns the following error codes:
  HIDP_STATUS_SUCCESS.
  HIDP_STATUS_INVALID_REPORT_TYPE
  HIDP_STATUS_INVALID_PREPARSED_DATA
  HIDP_STATUS_BUFFER_TOO_SMALL (all given entries however have been filled in)
  HIDP_STATUS_USAGE_NOT_FOUND
--*/
{
    PHIDP_PREPARSED_DATA_INTERNAL pData = (PHIDP_PREPARSED_DATA_INTERNAL)PreparsedData;
    
    if (PreparsedData == NULL || pData->Signature1 != 0x50646948 || pData->Signature2 != 0x52444B20) {
        return HIDP_STATUS_INVALID_PREPARSED_DATA;
    }
    
    PCHANNEL_REPORT_HEADER reportHeader;
    
    if (ReportType == HidP_Input) {
        reportHeader = &pData->Input;
    } else if (ReportType == HidP_Output) {
        reportHeader = &pData->Output;
    } else if (ReportType == HidP_Feature) {
        reportHeader = &pData->Feature;
    } else {
        return HIDP_STATUS_INVALID_REPORT_TYPE;
    }
    
    PHIDP_CHANNEL_DESC channelDesc = (PHIDP_CHANNEL_DESC)((BYTE*)PreparsedData + pData->DescriptorOffset);
    USAGE matchedCount = 0;
    long returnStatus = HIDP_STATUS_USAGE_NOT_FOUND;
    
    for (USHORT channelIndex = reportHeader->Offset; channelIndex < reportHeader->Index; channelIndex++) {
        PHIDP_CHANNEL_DESC currentChannel = &channelDesc[channelIndex];
        
        if ((currentChannel->Flags & 4) != 0) {
            if ((UsagePage == 0 || currentChannel->UsagePage == UsagePage) &&
                (LinkCollection == 0 || currentChannel->LinkCollection == LinkCollection) &&
                (Usage == 0 || (Usage == 0xFFFF && currentChannel->LinkUsage == 0) || 
                 (currentChannel->LinkCollection == LinkCollection))) {
                
                returnStatus = HIDP_STATUS_SUCCESS;
                
                if (matchedCount < *ButtonCapsLength) {
                    PHIDP_BUTTON_CAPS dest = &ButtonCaps[matchedCount];
                    
                    dest->UsagePage = currentChannel->UsagePage;
                    dest->ReportID = (UCHAR)currentChannel->ReportID;
                    dest->IsAlias = (currentChannel->Flags >> 5) & 1;
                    dest->BitField = currentChannel->BitField;
                    dest->LinkCollection = currentChannel->LinkCollection;
                    dest->LinkUsagePage = currentChannel->LinkUsagePage;
                    dest->LinkUsage = currentChannel->LinkUsage;
                    dest->IsRange = (currentChannel->Flags >> 4) & 1;
                    dest->IsStringRange = (currentChannel->Flags >> 6) & 1;
                    dest->IsDesignatorRange = (currentChannel->Flags >> 7) & 1;
                    dest->IsAbsolute = (currentChannel->Flags >> 3) & 1;
                    dest->ReportCount = currentChannel->ReportCount;
                    
                    dest->Range.UsageMin = currentChannel->UsageMin;
                    dest->Range.UsageMax = currentChannel->UsageMax;
                    dest->Range.DataIndexMin = currentChannel->DataIndexMin;
                    dest->Range.DataIndexMax = currentChannel->DataIndexMax;
                    dest->Range.StringMin = currentChannel->StringMin;
                    dest->Range.StringMax = currentChannel->StringMax;
                    dest->Range.DesignatorMin = currentChannel->DesignatorMin;
                    dest->Range.DesignatorMax = currentChannel->DesignatorMax;
                }
                matchedCount++;
            }
        }
    }
    
    *ButtonCapsLength = matchedCount;
    
    if (matchedCount == 0) {
        return returnStatus;
    }
    
    if (*ButtonCapsLength < matchedCount) {
        return HIDP_STATUS_BUFFER_TOO_SMALL;
    }
    
    return HIDP_STATUS_SUCCESS;
}

// HidP_GetSpecificValueCaps
_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS __stdcall
HidP_GetSpecificValueCaps (
   _In_       HIDP_REPORT_TYPE     ReportType,
   _In_opt_   USAGE                UsagePage,      // Optional (0 => ignore)
   _In_opt_   USHORT               LinkCollection, // Optional (0 => ignore)
   _In_opt_   USAGE                Usage,          // Optional (0 => ignore)
   _Out_writes_to_(*ValueCapsLength, *ValueCapsLength)      PHIDP_VALUE_CAPS     ValueCaps,
   _Inout_    PUSHORT              ValueCapsLength,
   _In_       PHIDP_PREPARSED_DATA PreparsedData
   )
/*++
Description:
   HidP_GetValueCaps returns all the values (non-binary) that are a part
   of the given report type for the Hid device represented by the given
   preparsed data.

Parameters:
   ReportType  One of HidP_Input, HidP_Output, or HidP_Feature.

   UsagePage   A usage page value used to limit the value caps returned to
                those on a given usage page.  If set to 0, this parameter is
                ignored.  Can be used with LinkCollection and Usage parameters
                to further limit the number of value caps structures returned.

   LinkCollection HIDP_LINK_COLLECTION node array index used to limit the
                  value caps returned to those buttons in a given link
                  collection.  If set to 0, this parameter is
                  ignored.  Can be used with UsagePage and Usage parameters
                  to further limit the number of value caps structures
                  returned.

   Usage      A usage value used to limit the value caps returned to those
               with the specified usage value.  If set to 0, this parameter
               is ignored.  Can be used with LinkCollection and UsagePage
               parameters to further limit the number of value caps
               structures returned.

   ValueCaps  A _HIDP_VALUE_CAPS array containing information about all the
               non-binary values in the given report.  This buffer is provided
               by the caller.

   ValueLength   As input, this parameter specifies the length of the ValueCaps
                  parameter (array) in number of array elements.  As output,
                  this value is set to indicate how many of those array elements
                  were filled in by the function.  The maximum number of
                  value caps that can be returned is found in the HIDP_CAPS
                  structure.  If HIDP_STATUS_BUFFER_TOO_SMALL is returned,
                  this value contains the number of array elements needed to
                  successfully complete the request.

   PreparsedData  The preparsed data returned from HIDCLASS.


Return Value
HidP_GetValueCaps returns the following error codes:
  HIDP_STATUS_SUCCESS.
  HIDP_STATUS_INVALID_REPORT_TYPE
  HIDP_STATUS_INVALID_PREPARSED_DATA
  HIDP_STATUS_BUFFER_TOO_SMALL (all given entries however have been filled in)
  HIDP_STATUS_USAGE_NOT_FOUND

--*/
{
    PHIDP_PREPARSED_DATA_INTERNAL pData = (PHIDP_PREPARSED_DATA_INTERNAL)PreparsedData;
    
    if (PreparsedData == NULL || pData->Signature1 != 0x50646948 || pData->Signature2 != 0x52444B20) {
        return HIDP_STATUS_INVALID_PREPARSED_DATA;
    }
    
    PCHANNEL_REPORT_HEADER reportHeader;
    
    if (ReportType == HidP_Input) {
        reportHeader = &pData->Input;
    } else if (ReportType == HidP_Output) {
        reportHeader = &pData->Output;
    } else if (ReportType == HidP_Feature) {
        reportHeader = &pData->Feature;
    } else {
        return HIDP_STATUS_INVALID_REPORT_TYPE;
    }
    
    PHIDP_CHANNEL_DESC channelDesc = (PHIDP_CHANNEL_DESC)((BYTE*)PreparsedData + pData->DescriptorOffset);
    USAGE matchedCount = 0;
    long returnStatus = HIDP_STATUS_USAGE_NOT_FOUND;
    
    for (USHORT channelIndex = reportHeader->Offset; channelIndex < reportHeader->Index; channelIndex++) {
        PHIDP_CHANNEL_DESC currentChannel = &channelDesc[channelIndex];
        
        if ((currentChannel->Flags & 4) == 0) {
            if ((UsagePage == 0 || currentChannel->UsagePage == UsagePage) &&
                (LinkCollection == 0 || currentChannel->LinkCollection == LinkCollection) &&
                (Usage == 0 || (Usage == 0xFFFF && currentChannel->LinkUsage == 0) || 
                 (currentChannel->LinkCollection == LinkCollection))) {
                
                returnStatus = HIDP_STATUS_SUCCESS;
                
                if (matchedCount < *ValueCapsLength) {
                    PHIDP_VALUE_CAPS dest = &ValueCaps[matchedCount];
                    
                    dest->UsagePage = currentChannel->UsagePage;
                    dest->ReportID = (UCHAR)currentChannel->ReportID;
                    dest->IsAlias = (currentChannel->Flags >> 5) & 1;
                    dest->BitField = currentChannel->BitField;
                    dest->LinkCollection = currentChannel->LinkCollection;
                    dest->LinkUsagePage = currentChannel->LinkUsagePage;
                    dest->LinkUsage = currentChannel->LinkUsage;
                    dest->IsRange = (currentChannel->Flags >> 4) & 1;
                    dest->IsStringRange = (currentChannel->Flags >> 6) & 1;
                    dest->IsDesignatorRange = (currentChannel->Flags >> 7) & 1;
                    dest->IsAbsolute = (currentChannel->Flags >> 3) & 1;
                    dest->HasNull = currentChannel->HasNull;
                    dest->BitSize = currentChannel->BitSize;
                    
                    if ((currentChannel->Flags & 0x10) == 0) {
                        dest->ReportCount = currentChannel->ReportCount;
                    } else {
                        dest->ReportCount = 1;
                    }
                    
                    dest->Units = currentChannel->Units;
                    dest->UnitsExp = currentChannel->UnitsExp;
                    dest->LogicalMin = currentChannel->LogicalMin;
                    dest->LogicalMax = currentChannel->LogicalMax;
                    dest->PhysicalMin = currentChannel->PhysicalMin;
                    dest->PhysicalMax = currentChannel->PhysicalMax;
                    
                    dest->Range.UsageMin = currentChannel->UsageMin;
                    dest->Range.UsageMax = currentChannel->UsageMax;
                    dest->Range.DataIndexMin = currentChannel->DataIndexMin;
                    dest->Range.DataIndexMax = currentChannel->DataIndexMax;
                    dest->Range.StringMin = currentChannel->StringMin;
                    dest->Range.StringMax = currentChannel->StringMax;
                    dest->Range.DesignatorMin = currentChannel->DesignatorMin;
                    dest->Range.DesignatorMax = currentChannel->DesignatorMax;
                }
                matchedCount++;
            }
        }
    }
    
    *ValueCapsLength = matchedCount;
    
    if (matchedCount == 0) {
        return returnStatus;
    }
    
    if (*ValueCapsLength < matchedCount) {
        return HIDP_STATUS_BUFFER_TOO_SMALL;
    }
    
    return HIDP_STATUS_SUCCESS;
}

// HidP_GetUsages
_Must_inspect_result_
NTSTATUS __stdcall
HidP_GetUsages (
   _In_ HIDP_REPORT_TYPE    ReportType,
   _In_ USAGE   UsagePage,
   _In_opt_ USHORT  LinkCollection,
   _Out_writes_to_(*UsageLength, *UsageLength)  PUSAGE UsageList,
   _Inout_    PULONG UsageLength,
   _In_ PHIDP_PREPARSED_DATA PreparsedData,
   _Out_writes_bytes_(ReportLength)    PCHAR Report,
   _In_ ULONG   ReportLength
   )
/*++

Routine Description:
    This function returns the binary values (buttons) that are set in a HID
    report.  Given a report packet of correct length, it searches the report
    packet for each usage for the given usage page and returns them in the
    usage list.

Parameters:
    ReportType One of HidP_Input, HidP_Output or HidP_Feature.

    UsagePage  All of the usages in the usage list, which HidP_GetUsages will
               retrieve in the report, refer to this same usage page.
               If the client wishes to get usages in a packet for multiple
               usage pages then that client needs to make multiple calls
               to HidP_GetUsages.

    LinkCollection  An optional value which can limit which usages are returned
                    in the UsageList to those usages that exist in a specific
                    LinkCollection.  A non-zero value indicates the index into
                    the HIDP_LINK_COLLECITON_NODE list returned by
                    HidP_GetLinkCollectionNodes of the link collection the
                    usage should belong to.  A value of 0 indicates this
                    should value be ignored.

    UsageList  The usage array that will contain all the usages found in
               the report packet.

    UsageLength The length of the given usage array in array elements.
                On input, this value describes the length of the usage list.
                On output, HidP_GetUsages sets this value to the number of
                usages that was found.  Use HidP_MaxUsageListLength to
                determine the maximum length needed to return all the usages
                that a given report packet may contain.

    PreparsedData Preparsed data structure returned by HIDCLASS

    Report       The report packet.

    ReportLength  Length (in bytes) of the given report packet


Return Value
    HidP_GetUsages returns the following error codes:

  HIDP_STATUS_SUCCESS                -- upon successfully retrieving all the
                                        usages from the report packet
  HIDP_STATUS_INVALID_REPORT_TYPE    -- if ReportType is not valid.
  HIDP_STATUS_INVALID_PREPARSED_DATA -- if PreparsedData is not valid
  HIDP_STATUS_INVALID_REPORT_LENGTH  -- the length of the report packet is not
                                        equal to the length specified in
                                        the HIDP_CAPS structure for the given
                                        ReportType
  HIDP_STATUS_REPORT_DOES_NOT_EXIST  -- if there are no reports on this device
                                        for the given ReportType
  HIDP_STATUS_BUFFER_TOO_SMALL       -- if the UsageList is not big enough to
                                        hold all the usages found in the report
                                        packet.  If this is returned, the buffer
                                        will contain UsageLength number of
                                        usages.  Use HidP_MaxUsageListLength to
                                        find the maximum length needed
  HIDP_STATUS_INCOMPATIBLE_REPORT_ID -- if no usages were found but usages
                                        that match the UsagePage and
                                        LinkCollection specified could be found
                                        in a report with a different report ID
  HIDP_STATUS_USAGE_NOT_FOUND        -- if there are no usages in a reports for
                                        the device and ReportType that match the
                                        UsagePage and LinkCollection that were
                                        specified
--*/
{
    PHIDP_PREPARSED_DATA_INTERNAL pData = (PHIDP_PREPARSED_DATA_INTERNAL)PreparsedData;
    
    if (PreparsedData == NULL || pData->Signature1 != 0x50646948 || pData->Signature2 != 0x52444B20) {
        return HIDP_STATUS_INVALID_PREPARSED_DATA;
    }
    
    memset(UsageList, 0, *UsageLength * sizeof(USAGE));
    
    if (ReportType == HidP_Input) {
        return ProcessUsageList(0, &pData->Input.Offset);
    } else if (ReportType == HidP_Output) {
        return ProcessUsageList(0, &pData->Output.Offset);
    } else if (ReportType == HidP_Feature) {
        return ProcessUsageList(0, &pData->Feature.Offset);
    } else {
        return HIDP_STATUS_INVALID_REPORT_TYPE;
    }
}

// HidP_GetUsagesEx
_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS __stdcall
HidP_GetUsagesEx (
    _In_    HIDP_REPORT_TYPE    ReportType,
    _In_opt_  USHORT  LinkCollection, // Optional
    _Inout_updates_to_(*UsageLength,*UsageLength) PUSAGE_AND_PAGE  ButtonList,
    _Inout_   ULONG * UsageLength,
    _In_ PHIDP_PREPARSED_DATA PreparsedData,
    _In_reads_bytes_(ReportLength)   PCHAR   Report,
    _In_ ULONG  ReportLength
   )
/*++

Routine Description:
    This function returns the binary values (buttons) in a HID report.
    Given a report packet of correct length, it searches the report packet
    for all buttons and returns the UsagePage and Usage for each of the buttons
    it finds.

Parameters:
    ReportType  One of HidP_Input, HidP_Output or HidP_Feature.

    LinkCollection  An optional value which can limit which usages are returned
                    in the ButtonList to those usages that exist in a specific
                    LinkCollection.  A non-zero value indicates the index into
                    the HIDP_LINK_COLLECITON_NODE list returned by
                    HidP_GetLinkCollectionNodes of the link collection the
                    usage should belong to.  A value of 0 indicates this
                    should value be ignored.

    ButtonList  An array of USAGE_AND_PAGE structures describing all the
                buttons currently ``down'' in the device.

    UsageLength The length of the given array in terms of elements.
                On input, this value describes the length of the list.  On
                output, HidP_GetUsagesEx sets this value to the number of
                usages that were found.  Use HidP_MaxUsageListLength to
                determine the maximum length needed to return all the usages
                that a given report packet may contain.

    PreparsedData Preparsed data returned by HIDCLASS

    Report       The report packet.

    ReportLength Length (in bytes) of the given report packet.


Return Value
    HidP_GetUsagesEx returns the following error codes:

  HIDP_STATUS_SUCCESS                -- upon successfully retrieving all the
                                        usages from the report packet
  HIDP_STATUS_INVALID_REPORT_TYPE    -- if ReportType is not valid.
  HIDP_STATUS_INVALID_PREPARSED_DATA -- if PreparsedData is not valid
  HIDP_STATUS_INVALID_REPORT_LENGTH  -- the length of the report packet is not
                                        equal to the length specified in
                                        the HIDP_CAPS structure for the given
                                        ReportType
  HIDP_STATUS_REPORT_DOES_NOT_EXIST  -- if there are no reports on this device
                                        for the given ReportType
  HIDP_STATUS_BUFFER_TOO_SMALL       -- if ButtonList is not big enough to
                                        hold all the usages found in the report
                                        packet.  If this is returned, the buffer
                                        will contain UsageLength number of
                                        usages.  Use HidP_MaxUsageListLength to
                                        find the maximum length needed
  HIDP_STATUS_INCOMPATIBLE_REPORT_ID -- if no usages were found but usages
                                        that match the specified LinkCollection
                                        exist in report with a different report
                                        ID.
  HIDP_STATUS_USAGE_NOT_FOUND        -- if there are no usages in any reports that
                                        match the LinkCollection parameter
--*/
{
    return HidP_GetUsages(ReportType, 0, LinkCollection, &ButtonList->Usage, UsageLength, PreparsedData, Report, ReportLength);
}

// HidP_GetUsageValue
_Must_inspect_result_
NTSTATUS __stdcall
HidP_GetUsageValue (
    _In_ HIDP_REPORT_TYPE ReportType,
    _In_ USAGE UsagePage,
    _In_opt_ USHORT LinkCollection,
    _In_ USAGE Usage,
    _Out_ PULONG UsageValue,
    _In_ PHIDP_PREPARSED_DATA PreparsedData,
    _In_reads_bytes_(ReportLength) PCHAR Report,
    _In_ ULONG ReportLength
    )
/*
Description
    HidP_GetUsageValue retrieves the value from the HID Report for the usage
    specified by the combination of usage page, usage and link collection.
    If a report packet contains two different fields with the same
    Usage and UsagePage, they can be distinguished with the optional
    LinkCollection field value.

Parameters:

    ReportType  One of HidP_Input or HidP_Feature.

    UsagePage   The usage page to which the given usage refers.

    LinkCollection  (Optional)  This value can be used to differentiate
                                between two fields that may have the same
                                UsagePage and Usage but exist in different
                                collections.  If the link collection value
                                is zero, this function will set the first field
                                it finds that matches the usage page and
                                usage.

    Usage       The usage whose value HidP_GetUsageValue will retrieve

    UsageValue  The raw value that is set for the specified field in the report
                buffer. This value will either fall within the logical range
                or if NULL values are allowed, a number outside the range to
                indicate a NULL

    PreparsedData The preparsed data returned for HIDCLASS

    Report      The report packet.

    ReportLength Length (in bytes) of the given report packet.


Return Value:
    HidP_GetUsageValue returns the following error codes:

  HIDP_STATUS_SUCCESS                -- upon successfully retrieving the value
                                        from the report packet
  HIDP_STATUS_INVALID_REPORT_TYPE    -- if ReportType is not valid.
  HIDP_STATUS_INVALID_PREPARSED_DATA -- if PreparsedData is not valid
  HIDP_STATUS_INVALID_REPORT_LENGTH  -- the length of the report packet is not
                                        equal to the length specified in
                                        the HIDP_CAPS structure for the given
                                        ReportType
  HIDP_STATUS_REPORT_DOES_NOT_EXIST  -- if there are no reports on this device
                                        for the given ReportType
  HIDP_STATUS_INCOMPATIBLE_REPORT_ID -- the specified usage page, usage and
                                        link collection exist but exists in
                                        a report with a different report ID
                                        than the report being passed in.  To
                                        set this value, call HidP_GetUsageValue
                                        again with a different report packet
  HIDP_STATUS_USAGE_NOT_FOUND        -- if the usage page, usage, and link
                                        collection combination does not exist
                                        in any reports for this ReportType
--*/
{
    PHIDP_PREPARSED_DATA_INTERNAL pData = (PHIDP_PREPARSED_DATA_INTERNAL)PreparsedData;
    
    if (PreparsedData == NULL || pData->Signature1 != 0x50646948 || pData->Signature2 != 0x52444B20) {
        return HIDP_STATUS_INVALID_PREPARSED_DATA;
    }
    
    if (ReportType == HidP_Input) {
        return ExtractUsageValue(&pData->Input.Offset, (int)PreparsedData);
    } else if (ReportType == HidP_Output) {
        return ExtractUsageValue(&pData->Output.Offset, (int)PreparsedData);
    } else if (ReportType == HidP_Feature) {
        return ExtractUsageValue(&pData->Feature.Offset, (int)PreparsedData);
    } else {
        return HIDP_STATUS_INVALID_REPORT_TYPE;
    }
}

// HidP_GetUsageValueArray
_Must_inspect_result_
NTSTATUS __stdcall
HidP_GetUsageValueArray (
    _In_ HIDP_REPORT_TYPE ReportType,
    _In_ USAGE UsagePage,
    _In_opt_ USHORT LinkCollection,
    _In_ USAGE Usage,
    _Inout_updates_bytes_(UsageValueByteLength) PCHAR UsageValue,
    _In_ USHORT UsageValueByteLength,
    _In_ PHIDP_PREPARSED_DATA PreparsedData,
    _In_reads_bytes_(ReportLength) PCHAR Report,
    _In_ ULONG ReportLength
    )
/*++
Routine Descripton:
    A usage value array occurs when the last usage in the list of usages
    describing a main item must be repeated because there are less usages defined
    than there are report counts declared for the given main item.  In this case
    a single value cap is allocated for that usage and the report count of that
    value cap is set to reflect the number of fields to which that usage refers.

    HidP_GetUsageValueArray returns the raw bits for that usage which spans
    more than one field in a report.

    NOTE: This function currently does not support value arrays where the
          ReportSize for each of the fields in the array is not a multiple
          of 8 bits.

          The UsageValue buffer will have the raw values as they are set
          in the report packet.

Parameters:

    ReportType  One of HidP_Input, HidP_Output or HidP_Feature.

    UsagePage   The usage page to which the given usage refers.

    LinkCollection  (Optional)  This value can be used to differentiate
                                between two fields that may have the same
                                UsagePage and Usage but exist in different
                                collections.  If the link collection value
                                is zero, this function will set the first field
                                it finds that matches the usage page and
                                usage.

   Usage       The usage whose value HidP_GetUsageValueArray will retreive.

   UsageValue  A pointer to an array of characters where the value will be
               placed.  The number of BITS required is found by multiplying the
               BitSize and ReportCount fields of the Value Cap for this
               control.  The least significant bit of this control found in the
               given report will be placed in the least significant bit location
               of the buffer (little-endian format), regardless of whether
               or not the field is byte aligned or if the BitSize is a multiple
               of sizeof (CHAR).

               See note above about current implementation limitations

   UsageValueByteLength
               the length of the given UsageValue buffer.

   PreparsedData The preparsed data returned by the HIDCLASS

   Report      The report packet.

   ReportLength   Length of the given report packet.

Return Value:

  HIDP_STATUS_SUCCESS                -- upon successfully retrieving the value
                                        from the report packet
  HIDP_STATUS_INVALID_REPORT_TYPE    -- if ReportType is not valid.
  HIDP_STATUS_INVALID_PREPARSED_DATA -- if PreparsedData is not valid
  HIDP_STATUS_INVALID_REPORT_LENGTH  -- the length of the report packet is not
                                        equal to the length specified in
                                        the HIDP_CAPS structure for the given
                                        ReportType
  HIDP_STATUS_NOT_VALUE_ARRAY        -- if the control specified is not a
                                        value array -- a value array will have
                                        a ReportCount field in the
                                        HIDP_VALUE_CAPS structure that is > 1
                                        Use HidP_GetUsageValue instead
  HIDP_STATUS_BUFFER_TOO_SMALL       -- if the size of the passed in buffer in
                                        which to return the array is too small
                                        (ie. has fewer values than the number of
                                        fields in the array
  HIDP_STATUS_NOT_IMPLEMENTED        -- if the usage value array has field sizes
                                        that are not multiples of 8 bits, this
                                        error code is returned since the function
                                        currently does not handle getting values
                                        from such arrays.
  HIDP_STATUS_INCOMPATIBLE_REPORT_ID -- the specified usage page, usage and
                                        link collection exist but exists in
                                        a report with a different report ID
                                        than the report being passed in.  To
                                        set this value, call
                                        HidP_GetUsageValueArray with a
                                        different report packet
  HIDP_STATUS_USAGE_NOT_FOUND        -- if the usage page, usage, and link
                                        collection combination does not exist
                                        in any reports for this ReportType
--*/
{
    PHIDP_PREPARSED_DATA_INTERNAL pData = (PHIDP_PREPARSED_DATA_INTERNAL)PreparsedData;
    
    if (PreparsedData == NULL || pData->Signature1 != 0x50646948 || pData->Signature2 != 0x52444B20) {
        return HIDP_STATUS_INVALID_PREPARSED_DATA;
    }
    
    PCHANNEL_REPORT_HEADER reportHeader;
    
    if (ReportType == HidP_Input) {
        reportHeader = &pData->Input;
    } else if (ReportType == HidP_Output) {
        reportHeader = &pData->Output;
    } else if (ReportType == HidP_Feature) {
        reportHeader = &pData->Feature;
    } else {
        return HIDP_STATUS_INVALID_REPORT_TYPE;
    }
    
    if ((USHORT)ReportLength != reportHeader->ByteLen) {
        return HIDP_STATUS_INVALID_REPORT_LENGTH;
    }
    
    if (reportHeader->ByteLen == 0) {
        return HIDP_STATUS_REPORT_DOES_NOT_EXIST;
    }
    
    PHIDP_CHANNEL_DESC channelDesc = (PHIDP_CHANNEL_DESC)((BYTE*)PreparsedData + pData->DescriptorOffset);
    BOOL foundMismatch = FALSE;
    
    for (USHORT channelIndex = reportHeader->Offset; channelIndex < reportHeader->Index; channelIndex++) {
        PHIDP_CHANNEL_DESC currentChannel = &channelDesc[channelIndex];
        
        if (((currentChannel->Flags & 4) == 0) && currentChannel->UsagePage == UsagePage) {
            if ((LinkCollection == 0 || currentChannel->LinkCollection == LinkCollection) &&
                ((Usage == 0 && currentChannel->LinkUsage == 0) || 
                 (LinkCollection != 0 && currentChannel->LinkCollection == LinkCollection))) {
                
                if ((currentChannel->Flags & 0x10) == 0) {
                    if (currentChannel->Usage == Usage) {
                        if (currentChannel->BitSize == 1) {
                            return HIDP_STATUS_NOT_VALUE_ARRAY;
                        }
                        
                        if (Report[0] == 0 || currentChannel->ReportID == (UCHAR)Report[0]) {
                            USHORT bitOffset = currentChannel->BitOffset;
                            USHORT totalBits = currentChannel->BitSize * currentChannel->ReportCount;
                            USHORT requiredBytes = (totalBits + 7) / 8;
                            
                            if (requiredBytes > UsageValueByteLength) {
                                return HIDP_STATUS_BUFFER_TOO_SMALL;
                            }
                            
                            if ((bitOffset & 7) != 0) {
                                return HIDP_STATUS_INVALID_REPORT_LENGTH;
                            }
                            
                            ULONG byteOffset = bitOffset >> 3;
                            ULONG bytesPerValue = currentChannel->BitSize / 8;
                            
                            for (USHORT valueIndex = 0; valueIndex < currentChannel->ReportCount; valueIndex++) {
                                for (ULONG byteIndex = 0; byteIndex < bytesPerValue; byteIndex++) {
                                    UsageValue[valueIndex * bytesPerValue + byteIndex] = Report[byteOffset + valueIndex * bytesPerValue + byteIndex];
                                }
                            }
                            
                            return HIDP_STATUS_SUCCESS;
                        }
                        foundMismatch = TRUE;
                    }
                } else if (currentChannel->UsageMin <= Usage && Usage <= currentChannel->UsageMax) {
                    return HIDP_STATUS_NOT_VALUE_ARRAY;
                }
            }
        }
    }
    
    if (foundMismatch) {
        return HIDP_STATUS_INCOMPATIBLE_REPORT_ID;
    }
    
    return HIDP_STATUS_USAGE_NOT_FOUND;
}

// HidP_GetValueCaps
_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS __stdcall
HidP_GetValueCaps (
   _In_       HIDP_REPORT_TYPE     ReportType,
   _Out_writes_to_(*ValueCapsLength, *ValueCapsLength) PHIDP_VALUE_CAPS ValueCaps,
   _Inout_    PUSHORT              ValueCapsLength,
   _In_       PHIDP_PREPARSED_DATA PreparsedData
)
/*++
Description:
   HidP_GetValueCaps returns all the values (non-binary) that are a part
   of the given report type for the Hid device represented by the given
   preparsed data.

Parameters:
   ReportType  One of HidP_Input, HidP_Output, or HidP_Feature.

   UsagePage   A usage page value used to limit the value caps returned to
                those on a given usage page.  If set to 0, this parameter is
                ignored.  Can be used with LinkCollection and Usage parameters
                to further limit the number of value caps structures returned.

   LinkCollection HIDP_LINK_COLLECTION node array index used to limit the
                  value caps returned to those buttons in a given link
                  collection.  If set to 0, this parameter is
                  ignored.  Can be used with UsagePage and Usage parameters
                  to further limit the number of value caps structures
                  returned.

   Usage      A usage value used to limit the value caps returned to those
               with the specified usage value.  If set to 0, this parameter
               is ignored.  Can be used with LinkCollection and UsagePage
               parameters to further limit the number of value caps
               structures returned.

   ValueCaps  A _HIDP_VALUE_CAPS array containing information about all the
               non-binary values in the given report.  This buffer is provided
               by the caller.

   ValueLength   As input, this parameter specifies the length of the ValueCaps
                  parameter (array) in number of array elements.  As output,
                  this value is set to indicate how many of those array elements
                  were filled in by the function.  The maximum number of
                  value caps that can be returned is found in the HIDP_CAPS
                  structure.  If HIDP_STATUS_BUFFER_TOO_SMALL is returned,
                  this value contains the number of array elements needed to
                  successfully complete the request.

   PreparsedData  The preparsed data returned from HIDCLASS.


Return Value
HidP_GetValueCaps returns the following error codes:
  HIDP_STATUS_SUCCESS.
  HIDP_STATUS_INVALID_REPORT_TYPE
  HIDP_STATUS_INVALID_PREPARSED_DATA
  HIDP_STATUS_BUFFER_TOO_SMALL (all given entries however have been filled in)
  HIDP_STATUS_USAGE_NOT_FOUND

--*/
{
    return HidP_GetSpecificValueCaps(ReportType, 0, 0, 0, ValueCaps, ValueCapsLength, PreparsedData);
}

// HidP_Index2Usage
USAGE_AND_PAGE __cdecl HidP_Index2Usage(PHIDP_CHANNEL_DESC channelDesc, ULONG dataIndex)
{
    USAGE_AND_PAGE result;
    PHIDP_CHANNEL_DESC currentDesc = channelDesc;
    UCHAR flags;
    
    result.Usage = 0;
    result.UsagePage = 0;
    
    if (dataIndex == 0 || channelDesc == NULL) {
        return result;
    }
    
    flags = (UCHAR)channelDesc->Flags;
    
    while ((flags & 1) != 0) {
        flags = (UCHAR)currentDesc[0x34].Flags;
        currentDesc += 0x34;
    }
    
    while (1) {
        if ((currentDesc->Flags & 0x10) == 0) {
            if (dataIndex == 1) {
                result.Usage = currentDesc->Usage;
                result.UsagePage = currentDesc->UsagePage;
                return result;
            }
            dataIndex--;
        } else {
            USHORT usageMin = currentDesc->UsageMin;
            if (usageMin == 0) {
                usageMin = 1;
            }
            ULONG rangeCount = (currentDesc->UsageMax - usageMin) + 1;
            
            if (dataIndex <= rangeCount) {
                result.Usage = (usageMin - 1) + (USHORT)dataIndex;
                result.UsagePage = currentDesc->UsagePage;
                return result;
            }
            dataIndex -= rangeCount;
        }
        
        if (channelDesc == currentDesc) {
            return result;
        }
        
        currentDesc -= 0x34;
        
        if (dataIndex == 0) {
            return result;
        }
    }
}

// HidP_InitializeFields
long __cdecl HidP_InitializeFields(HIDP_REPORT_TYPE reportType, UCHAR reportId, PHIDP_PREPARSED_DATA preparsedData, 
                                    char* report, ULONG reportLength, UCHAR param6)
{
    if (reportType == HidP_Input || reportType == HidP_Output || reportType == HidP_Feature) {
        return InitializeReportData();
    }
    return HIDP_STATUS_INVALID_REPORT_TYPE;
}

// HidP_InitializeReportForID
_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS __stdcall
HidP_InitializeReportForID (
   _In_ HIDP_REPORT_TYPE ReportType,
   _In_ UCHAR ReportID,
   _In_ PHIDP_PREPARSED_DATA PreparsedData,
   _Out_writes_bytes_(ReportLength) PCHAR Report,
   _In_ ULONG ReportLength
   )
/*++

Routine Description:

    Initialize a report based on the given report ID.

Parameters:

    ReportType  One of HidP_Input, HidP_Output, or HidP_Feature.

    PreparasedData  Preparsed data structure returned by HIDCLASS

    Report      Buffer which to set the data into.

    ReportLength Length of Report...Report should be at least as long as the
                value indicated in the HIDP_CAPS structure for the device and
                the corresponding ReportType

Return Value

  HIDP_STATUS_INVALID_REPORT_TYPE    -- if ReportType is not valid.
  HIDP_STATUS_INVALID_PREPARSED_DATA -- if PreparsedData is not valid
  HIDP_STATUS_INVALID_REPORT_LENGTH  -- the length of the report packet is not equal
                                        to the length specified in HIDP_CAPS
                                        structure for the given ReportType
  HIDP_STATUS_REPORT_DOES_NOT_EXIST  -- if there are no reports on this device
                                        for the given ReportType

--*/
{
    return HidP_InitializeFields(ReportType, ReportID, PreparsedData, Report, ReportLength, 0);
}

// HidP_InsertData
void __cdecl HidP_InsertData(USHORT bitOffset, USHORT reportId, USHORT reportLength, char* data, ULONG bufferOffset, ULONG reportDataPtr)
{
    UCHAR* reportData = (UCHAR*)reportDataPtr;
    USHORT currentBytePos = bitOffset >> 3;
    UCHAR remainingBits = bitOffset & 7;
    ULONG dataValue = (ULONG)(ULONG_PTR)data;
    USHORT currentBitOffset = bitOffset;
    
    if (currentBytePos < reportLength) {
        if (remainingBits != 0) {
            UCHAR bitsInFirstByte = 8 - remainingBits;
            UCHAR mask = ((1 << bitsInFirstByte) - 1) << remainingBits;
            reportData[currentBytePos] = (reportData[currentBytePos] & ~mask) | 
                                          (((UCHAR)dataValue << remainingBits) & mask);
            currentBitOffset -= bitsInFirstByte;
            dataValue >>= bitsInFirstByte;
            currentBytePos++;
        }
        
        while (currentBitOffset >= 8 && currentBytePos < reportLength) {
            reportData[currentBytePos] = (UCHAR)dataValue;
            currentBitOffset -= 8;
            dataValue >>= 8;
            currentBytePos++;
        }
        
        if (currentBitOffset > 0 && currentBytePos < reportLength) {
            UCHAR mask = (1 << currentBitOffset) - 1;
            reportData[currentBytePos] = (reportData[currentBytePos] & ~mask) | (dataValue & mask);
        }
    }
}

// HidP_MaxDataListLength
_IRQL_requires_max_(DISPATCH_LEVEL) 
ULONG __stdcall
HidP_MaxDataListLength (
   _In_ HIDP_REPORT_TYPE      ReportType,
   _In_ PHIDP_PREPARSED_DATA  PreparsedData
   )
/*++
Routine Description:

    This function returns the maximum length of HIDP_DATA elements that
    HidP_GetData could return for the given report type.

Parameters:

    ReportType  One of HidP_Input, HidP_Output or HidP_Feature.

    PreparsedData    Preparsed data structure returned by HIDCLASS

Return Value:

    The length of the data list array required for the HidP_GetData function
    call.  If an error occurs (either HIDP_STATUS_INVALID_REPORT_TYPE or
    HIDP_STATUS_INVALID_PREPARSED_DATA), this function returns 0.

--*/
{
    PHIDP_PREPARSED_DATA_INTERNAL pData = (PHIDP_PREPARSED_DATA_INTERNAL)PreparsedData;
    ULONG maxLength = 0;
    
    if (PreparsedData == NULL || pData->Signature1 != 0x50646948 || pData->Signature2 != 0x52444B20) {
        return 0;
    }
    
    PCHANNEL_REPORT_HEADER reportHeader;
    
    if (ReportType == HidP_Input) {
        reportHeader = &pData->Input;
    } else if (ReportType == HidP_Output) {
        reportHeader = &pData->Output;
    } else if (ReportType == HidP_Feature) {
        reportHeader = &pData->Feature;
    } else {
        return 0;
    }
    
    PHIDP_CHANNEL_DESC channelDesc = (PHIDP_CHANNEL_DESC)((BYTE*)PreparsedData + pData->DescriptorOffset);
    
    for (USHORT channelIndex = reportHeader->Offset; channelIndex < reportHeader->Index; channelIndex++) {
        if ((channelDesc[channelIndex].Flags & 4) == 0) {
            if ((channelDesc[channelIndex].Flags & 0x10) == 0) {
                maxLength++;
            } else {
                maxLength += channelDesc[channelIndex].ReportCount;
            }
        } else {
            maxLength += channelDesc[channelIndex].ReportCount;
        }
    }
    
    return maxLength;
}

// HidP_MaxUsageListLength
_IRQL_requires_max_(PASSIVE_LEVEL) 
ULONG __stdcall
HidP_MaxUsageListLength (
   _In_ HIDP_REPORT_TYPE      ReportType,
   _In_opt_ USAGE                 UsagePage, // Optional
   _In_ PHIDP_PREPARSED_DATA  PreparsedData
   )
/*++
Routine Description:
    This function returns the maximum number of usages that a call to
    HidP_GetUsages or HidP_GetUsagesEx could return for a given HID report.
    If calling for number of usages returned by HidP_GetUsagesEx, use 0 as
    the UsagePage value.

Parameters:
    ReportType  One of HidP_Input, HidP_Output or HidP_Feature.

    UsagePage   Specifies the optional UsagePage to query for.  If 0, will
                return all the maximum number of usage values that could be
                returned for a given ReportType.   If non-zero, will return
                the maximum number of usages that would be returned for the
                ReportType with the given UsagePage.

    PreparsedData Preparsed data returned from HIDCLASS

Return Value:
    The length of the usage list array required for the HidP_GetUsages or
    HidP_GetUsagesEx function call.  If an error occurs (such as
    HIDP_STATUS_INVALID_REPORT_TYPE or HIDP_INVALID_PREPARSED_DATA, this
    returns 0.
--*/
{
    PHIDP_PREPARSED_DATA_INTERNAL pData = (PHIDP_PREPARSED_DATA_INTERNAL)PreparsedData;
    ULONG maxLength = 0;
    
    if (PreparsedData == NULL || pData->Signature1 != 0x50646948 || pData->Signature2 != 0x52444B20) {
        return 0;
    }
    
    PCHANNEL_REPORT_HEADER reportHeader;
    
    if (ReportType == HidP_Input) {
        reportHeader = &pData->Input;
    } else if (ReportType == HidP_Output) {
        reportHeader = &pData->Output;
    } else if (ReportType == HidP_Feature) {
        reportHeader = &pData->Feature;
    } else {
        return 0;
    }
    
    PHIDP_CHANNEL_DESC channelDesc = (PHIDP_CHANNEL_DESC)((BYTE*)PreparsedData + pData->DescriptorOffset);
    
    for (USHORT channelIndex = reportHeader->Offset; channelIndex < reportHeader->Index; channelIndex++) {
        if (((channelDesc[channelIndex].Flags & 4) != 0) && (UsagePage == 0 || channelDesc[channelIndex].UsagePage == UsagePage)) {
            maxLength += channelDesc[channelIndex].ReportCount;
        }
    }
    
    return maxLength;
}

// HidP_SetData
_Must_inspect_result_
NTSTATUS __stdcall
HidP_SetData (
    _In_ HIDP_REPORT_TYPE ReportType,
    _Inout_updates_to_(*DataLength,*DataLength) PHIDP_DATA DataList,
    _Inout_ PULONG DataLength,
    _In_ PHIDP_PREPARSED_DATA PreparsedData,
    _In_reads_bytes_(ReportLength) PCHAR Report,
    _In_ ULONG ReportLength
    )
/*++

Routine Description:

    Please Note: Since usage value arrays deal with multiple fields for
                 for one usage value, they cannot be used with HidP_SetData
                 and HidP_GetData.  In this case,
                 HIDP_STATUS_IS_USAGE_VALUE_ARRAY will be returned.

Parameters:

    ReportType  One of HidP_Input, HidP_Output, or HidP_Feature.

    DataList    Array of HIDP_DATA structures that contains the data values
                that are to be set into the given report

    DataLength  As input, length in array elements of DataList.  As output,
                contains the number of data elements set on successful
                completion or an index into the DataList array to identify
                the faulting HIDP_DATA value if an error code is returned.

    PreparasedData  Preparsed data structure returned by HIDCLASS

    Report      Buffer which to set the data into.

    ReportLength Length of Report...Report should be at least as long as the
                value indicated in the HIDP_CAPS structure for the device and
                the corresponding ReportType

Return Value
    HidP_SetData returns the following error codes.  The report packet will
        have all the data set up until the HIDP_DATA structure that caused the
        error.  DataLength, in the error case, will return this problem index.

  HIDP_STATUS_SUCCESS                -- upon successful insertion of all data
                                        into the report packet.
  HIDP_STATUS_INVALID_REPORT_TYPE    -- if ReportType is not valid.
  HIDP_STATUS_INVALID_PREPARSED_DATA -- if PreparsedData is not valid
  HIDP_STATUS_DATA_INDEX_NOT_FOUND   -- if a HIDP_DATA structure referenced a
                                        data index that does not exist for this
                                        device's ReportType
  HIDP_STATUS_INVALID_REPORT_LENGTH  -- the length of the report packet is not equal
                                        to the length specified in HIDP_CAPS
                                        structure for the given ReportType
  HIDP_STATUS_REPORT_DOES_NOT_EXIST  -- if there are no reports on this device
                                        for the given ReportType
  HIDP_STATUS_IS_USAGE_VALUE_ARRAY   -- if one of the HIDP_DATA structures
                                        references a usage value array.
                                        DataLength will contain the index into
                                        the array that was invalid
  HIDP_STATUS_BUTTON_NOT_PRESSED     -- if a HIDP_DATA structure attempted
                                        to unset a button that was not already
                                        set in the Report
  HIDP_STATUS_INCOMPATIBLE_REPORT_ID -- a HIDP_DATA structure was found with
                                        a valid index value but is contained
                                        in a different report than the one
                                        currently being processed
  HIDP_STATUS_BUFFER_TOO_SMALL       -- if there are not enough entries in
                                        a given Main Array Item to report all
                                        buttons that have been requested to be
                                        set
--*/
{
    PHIDP_PREPARSED_DATA_INTERNAL pData = (PHIDP_PREPARSED_DATA_INTERNAL)PreparsedData;
    
    if (PreparsedData == NULL || pData->Signature1 != 0x50646948 || pData->Signature2 != 0x52444B20) {
        return HIDP_STATUS_INVALID_PREPARSED_DATA;
    }
    
    PCHANNEL_REPORT_HEADER reportHeader;
    
    if (ReportType == HidP_Input) {
        reportHeader = &pData->Input;
    } else if (ReportType == HidP_Output) {
        reportHeader = &pData->Output;
    } else if (ReportType == HidP_Feature) {
        reportHeader = &pData->Feature;
    } else {
        return HIDP_STATUS_INVALID_REPORT_TYPE;
    }
    
    if ((USHORT)ReportLength != reportHeader->ByteLen) {
        return HIDP_STATUS_INVALID_REPORT_LENGTH;
    }
    
    if (reportHeader->ByteLen == 0) {
        return HIDP_STATUS_REPORT_DOES_NOT_EXIST;
    }
    
    PHIDP_CHANNEL_DESC channelDesc = (PHIDP_CHANNEL_DESC)((BYTE*)PreparsedData + pData->DescriptorOffset);
    
    for (ULONG i = 0; i < *DataLength; i++) {
        PHIDP_DATA dataItem = &DataList[i];
        BOOL found = FALSE;
        
        for (USHORT channelIndex = reportHeader->Offset; channelIndex < reportHeader->Index; channelIndex++) {
            PHIDP_CHANNEL_DESC current = &channelDesc[channelIndex];
            
            if (current->DataIndexMin <= dataItem->DataIndex && dataItem->DataIndex <= current->DataIndexMax) {
                USHORT bitOffset = current->BitOffset + ((dataItem->DataIndex - current->DataIndexMin) * current->BitSize);
                ULONG value;
                
                if ((current->Flags & 4) == 0) {
                    value = dataItem->RawValue;
                } else {
                    value = dataItem->On ? 1 : 0;
                }
                
                HidP_InsertData(bitOffset, current->ReportID, (USHORT)ReportLength, (char*)(ULONG_PTR)value, 0, (ULONG)(ULONG_PTR)Report);
                found = TRUE;
                break;
            }
        }
        
        if (!found) {
            *DataLength = i;
            return HIDP_STATUS_DATA_INDEX_NOT_FOUND;
        }
    }
    
    return HIDP_STATUS_SUCCESS;
}

// HidP_SetScaledUsageValue
_Must_inspect_result_
NTSTATUS __stdcall
HidP_SetScaledUsageValue (
    _In_ HIDP_REPORT_TYPE ReportType,
    _In_ USAGE UsagePage,
    _In_opt_ USHORT LinkCollection,
    _In_ USAGE Usage,
    _In_ LONG UsageValue,
    _In_ PHIDP_PREPARSED_DATA PreparsedData,
    _Inout_updates_bytes_(ReportLength) PCHAR Report,
    _In_ ULONG ReportLength
    )
/*++
Description:
    HidP_SetScaledUsageValue inserts the UsageValue into the HID report packet
    in the field corresponding to the given usage page and usage.  If a report
    packet contains two different fields with the same Usage and UsagePage,
    they can be distinguished with the optional LinkCollection field value.

    If the specified field has a defined physical range, this function converts
    the physical value specified to the corresponding logical value for the
    report.  If a physical value does not exist, the function will verify that
    the value specified falls within the logical range and set according.

    If the range checking fails but the field has NULL values, the function will
    set the field to the defined NULL value (most negative number possible) and
    return HIDP_STATUS_NULL.  In other words, use this function to set NULL
    values for a given field by passing in a value that falls outside the
    physical range if it is defined or the logical range otherwise.

    If the field does not support NULL values, an out of range error will be
    returned instead.

Parameters:

    ReportType  One of HidP_Output or HidP_Feature.

    UsagePage   The usage page to which the given usage refers.

    LinkCollection  (Optional)  This value can be used to differentiate
                                between two fields that may have the same
                                UsagePage and Usage but exist in different
                                collections.  If the link collection value
                                is zero, this function will set the first field
                                it finds that matches the usage page and
                                usage.

    Usage       The usage whose value HidP_SetScaledUsageValue will set.

    UsageValue  The value to set in the report buffer.  See the routine
                description above for the different interpretations of this
                value

    PreparsedData The preparsed data returned from HIDCLASS

    Report      The report packet.

    ReportLength Length (in bytes) of the given report packet.


Return Value:
   HidP_SetScaledUsageValue returns the following error codes:

  HIDP_STATUS_SUCCESS                -- upon successfully setting the value
                                        in the report packet
  HIDP_STATUS_NULL                   -- upon successfully setting the value
                                        in the report packet as a NULL value
  HIDP_STATUS_INVALID_REPORT_TYPE    -- if ReportType is not valid.
  HIDP_STATUS_INVALID_PREPARSED_DATA -- if PreparsedData is not valid
  HIDP_STATUS_INVALID_REPORT_LENGTH  -- the length of the report packet is not
                                        equal to the length specified in
                                        the HIDP_CAPS structure for the given
                                        ReportType
  HIDP_STATUS_VALUE_OUT_OF_RANGE     -- if the value specified failed to fall
                                        within the physical range if it exists
                                        or within the logical range otherwise
                                        and the field specified by the usage
                                        does not allow NULL values
  HIDP_STATUS_BAD_LOG_PHY_VALUES     -- if the field has a physical range but
                                        either the logical range is invalid
                                        (max <= min) or the physical range is
                                        invalid
  HIDP_STATUS_INCOMPATIBLE_REPORT_ID -- the specified usage page, usage and
                                        link collection exist but exists in
                                        a report with a different report ID
                                        than the report being passed in.  To
                                        set this value, call
                                        HidP_SetScaledUsageValue again with
                                        a zero-initialized report packet
  HIDP_STATUS_USAGE_NOT_FOUND        -- if the usage page, usage, and link
                                        collection combination does not exist
                                        in any reports for this ReportType
--*/
{
    PHIDP_PREPARSED_DATA_INTERNAL pData = (PHIDP_PREPARSED_DATA_INTERNAL)PreparsedData;
    
    if (PreparsedData == NULL || pData->Signature1 != 0x50646948 || pData->Signature2 != 0x52444B20) {
        return HIDP_STATUS_INVALID_PREPARSED_DATA;
    }
    
    PCHANNEL_REPORT_HEADER reportHeader;
    
    if (ReportType == HidP_Input) {
        reportHeader = &pData->Input;
    } else if (ReportType == HidP_Output) {
        reportHeader = &pData->Output;
    } else if (ReportType == HidP_Feature) {
        reportHeader = &pData->Feature;
    } else {
        return HIDP_STATUS_INVALID_REPORT_TYPE;
    }
    
    if ((USHORT)ReportLength != reportHeader->ByteLen) {
        return HIDP_STATUS_INVALID_REPORT_LENGTH;
    }
    
    if (reportHeader->ByteLen == 0) {
        return HIDP_STATUS_REPORT_DOES_NOT_EXIST;
    }
    
    PHIDP_CHANNEL_DESC channelDesc = (PHIDP_CHANNEL_DESC)((BYTE*)PreparsedData + pData->DescriptorOffset);
    BOOL foundMismatch = FALSE;
    
    for (USHORT channelIndex = reportHeader->Offset; channelIndex < reportHeader->Index; channelIndex++) {
        PHIDP_CHANNEL_DESC currentChannel = &channelDesc[channelIndex];
        
        if (((currentChannel->Flags & 4) == 0) && currentChannel->UsagePage == UsagePage) {
            if ((LinkCollection == 0 || currentChannel->LinkCollection == LinkCollection) &&
                ((Usage == 0 && currentChannel->LinkUsage == 0) || 
                 (LinkCollection != 0 && currentChannel->LinkCollection == LinkCollection))) {
                
                if ((currentChannel->Flags & 0x10) == 0) {
                    if (currentChannel->Usage == Usage) {
                        if (Report[0] == 0 || currentChannel->ReportID == (UCHAR)Report[0]) {
                            ULONG rawValue;
                            ULONG signBit;
                            
                            if (currentChannel->LogicalMin == 0 && currentChannel->LogicalMax == 0) {
                                if (currentChannel->PhysicalMin == currentChannel->PhysicalMax) {
                                    return HIDP_STATUS_VALUE_OUT_OF_RANGE;
                                }
                                rawValue = (ULONG)UsageValue;
                            } else {
                                if (currentChannel->LogicalMax <= currentChannel->LogicalMin || 
                                    currentChannel->PhysicalMax <= currentChannel->PhysicalMin) {
                                    return HIDP_STATUS_VALUE_OUT_OF_RANGE;
                                }
                                
                                LONG logicalRange = currentChannel->LogicalMax - currentChannel->LogicalMin;
                                LONG physicalRange = currentChannel->PhysicalMax - currentChannel->PhysicalMin;
                                rawValue = currentChannel->LogicalMin + ((physicalRange + 1) * (UsageValue - currentChannel->PhysicalMin)) / (logicalRange + 1);
                            }
                            
                            signBit = 1 << (currentChannel->BitSize - 1);
                            
                            if ((LONG)rawValue < 0) {
                                rawValue = rawValue | signBit;
                            } else {
                                rawValue = rawValue & (signBit - 1);
                            }
                            
                            HidP_InsertData(currentChannel->BitOffset, (USHORT)(UCHAR)Report[0], (USHORT)ReportLength, (char*)(ULONG_PTR)rawValue, 0, (ULONG)(ULONG_PTR)Report);
                            return HIDP_STATUS_SUCCESS;
                        }
                        foundMismatch = TRUE;
                    }
                } else if (currentChannel->UsageMin <= Usage && Usage <= currentChannel->UsageMax) {
                    return HIDP_STATUS_NOT_VALUE_ARRAY;
                }
            }
        }
    }
    
    if (foundMismatch) {
        return HIDP_STATUS_INCOMPATIBLE_REPORT_ID;
    }
    
    return HIDP_STATUS_USAGE_NOT_FOUND;
}

// HidP_SetUsages
_Must_inspect_result_
NTSTATUS __stdcall
HidP_SetUsages (
   _In_ HIDP_REPORT_TYPE    ReportType,
   _In_ USAGE   UsagePage,
   _In_opt_ USHORT  LinkCollection,
   _Inout_updates_to_(*UsageLength,*UsageLength) PUSAGE  UsageList,
   _Inout_  PULONG  UsageLength,
   _In_ PHIDP_PREPARSED_DATA  PreparsedData,
   _In_reads_bytes_(ReportLength) PCHAR   Report,
   _In_ ULONG   ReportLength 
   )
/*++

Routine Description:
    This function sets binary values (buttons) in a report.  Given an
    initialized packet of correct length, it modifies the report packet so that
    each element in the given list of usages has been set in the report packet.
    For example, in an output report with 5 LED's, each with a given usage,
    an application could turn on any subset of these lights by placing their
    usages in any order into the usage array (UsageList).  HidP_SetUsages would,
    in turn, set the appropriate bit or add the corresponding byte into the
    HID Main Array Item.

    A properly initialized Report packet is one of the correct byte length,
    and all zeros.

    NOTE: A packet that has already been set with a call to a HidP_Set routine
          can also be passed in.  This routine then sets processes the UsageList
          in the same fashion but verifies that the ReportID already set in
          Report matches the report ID for the given usages.

Parameters:
    ReportType  One of HidP_Input, HidP_Output or HidP_Feature.

    UsagePage   All of the usages in the usage array, which HidP_SetUsages will
                set in the report, refer to this same usage page.
                If a client wishes to set usages in a report for multiple
                usage pages then that client needs to make multiple calls to
                HidP_SetUsages for each of the usage pages.

    UsageList   A usage array containing the usages that HidP_SetUsages will set in
                the report packet.

    UsageLength The length of the given usage array in array elements.
                The parser will set this value to the position in the usage
                array where it stopped processing.  If successful, UsageLength
                will be unchanged.  In any error condition, this parameter
                reflects how many of the usages in the usage list have
                actually been set by the parser.  This is useful for finding
                the usage in the list which caused the error.

    PreparsedData The preparsed data recevied from HIDCLASS

    Report      The report packet.

    ReportLength   Length of the given report packet...Must be equal to the
                   value reported in the HIDP_CAPS structure for the device
                   and corresponding report type.

Return Value
    HidP_SetUsages returns the following error codes.  On error, the report packet
    will be correct up until the usage element that caused the error.

  HIDP_STATUS_SUCCESS                -- upon successful insertion of all usages
                                        into the report packet.
  HIDP_STATUS_INVALID_REPORT_TYPE    -- if ReportType is not valid.
  HIDP_STATUS_INVALID_PREPARSED_DATA -- if PreparsedData is not valid
  HIDP_STATUS_INVALID_REPORT_LENGTH  -- the length of the report packet is not
                                        equal to the length specified in
                                        the HIDP_CAPS structure for the given
                                        ReportType
  HIDP_STATUS_REPORT_DOES_NOT_EXIST  -- if there are no reports on this device
                                        for the given ReportType
  HIDP_STATUS_INCOMPATIBLE_REPORT_ID -- if a usage was found that exists in a
                                        different report.  If the report is
                                        zero-initialized on entry the first
                                        usage in the list will determine which
                                        report ID is used.  Otherwise, the
                                        parser will verify that usage matches
                                        the passed in report's ID
  HIDP_STATUS_USAGE_NOT_FOUND        -- if the usage does not exist for any
                                        report (no matter what the report ID)
                                        for the given report type.
  HIDP_STATUS_BUFFER_TOO_SMALL       -- if there are not enough entries in a
                                        given Main Array Item to list all of
                                        the given usages.  The caller needs
                                        to split his request into more than
                                        one call
--*/
{
    PHIDP_PREPARSED_DATA_INTERNAL pData = (PHIDP_PREPARSED_DATA_INTERNAL)PreparsedData;
    
    if (PreparsedData == NULL || pData->Signature1 != 0x50646948 || pData->Signature2 != 0x52444B20) {
        return HIDP_STATUS_INVALID_PREPARSED_DATA;
    }
    
    PCHANNEL_REPORT_HEADER reportHeader;
    
    if (ReportType == HidP_Input) {
        reportHeader = &pData->Input;
    } else if (ReportType == HidP_Output) {
        reportHeader = &pData->Output;
    } else if (ReportType == HidP_Feature) {
        reportHeader = &pData->Feature;
    } else {
        return HIDP_STATUS_INVALID_REPORT_TYPE;
    }
    
    if ((USHORT)ReportLength != reportHeader->ByteLen) {
        return HIDP_STATUS_INVALID_REPORT_LENGTH;
    }
    
    if (reportHeader->ByteLen == 0) {
        return HIDP_STATUS_REPORT_DOES_NOT_EXIST;
    }
    
    PHIDP_CHANNEL_DESC channelDesc = (PHIDP_CHANNEL_DESC)((BYTE*)PreparsedData + pData->DescriptorOffset);
    
    for (ULONG i = 0; i < *UsageLength; i++) {
        USAGE currentUsage = UsageList[i];
        BOOL found = FALSE;
        
        if (currentUsage == 0) continue;
        
        for (USHORT channelIndex = reportHeader->Offset; channelIndex < reportHeader->Index; channelIndex++) {
            PHIDP_CHANNEL_DESC current = &channelDesc[channelIndex];
            
            if ((current->Flags & 4) != 0 && current->UsagePage == UsagePage && 
                (LinkCollection == 0 || current->LinkCollection == LinkCollection)) {
                
                if ((current->Flags & 0x10) != 0) {
                    if (current->UsageMin <= currentUsage && currentUsage <= current->UsageMax) {
                        if (Report[0] == 0 || current->ReportID == (UCHAR)Report[0]) {
                            Report[0] = (char)current->ReportID;
                            
                            if (current->BitSize == 1) {
                                USHORT bitOffset = current->BitOffset + (currentUsage - current->UsageMin);
                                UCHAR byteMask = 1 << (bitOffset & 7);
                                Report[bitOffset >> 3] |= byteMask;
                            } else {
                                ULONG dataIndex = currentUsage - current->UsageMin;
                                HidP_InsertData(current->BitOffset, current->ReportID, (USHORT)ReportLength, (char*)(ULONG_PTR)dataIndex, 0, (ULONG)(ULONG_PTR)Report);
                            }
                            found = TRUE;
                            break;
                        }
                    }
                }
            }
        }
        
        if (!found) {
            *UsageLength = i;
            return HIDP_STATUS_USAGE_NOT_FOUND;
        }
    }
    
    return HIDP_STATUS_SUCCESS;
}

// HidP_SetUsageValue
_Must_inspect_result_
NTSTATUS __stdcall
HidP_SetUsageValue (
    _In_ HIDP_REPORT_TYPE ReportType,
    _In_ USAGE UsagePage,
    _In_opt_ USHORT LinkCollection,
    _In_ USAGE Usage,
    _In_ ULONG UsageValue,
    _In_ PHIDP_PREPARSED_DATA PreparsedData,
    _Inout_updates_bytes_(ReportLength) PCHAR Report,
    _In_ ULONG ReportLength
    )
/*++
Description:
    HidP_SetUsageValue inserts a value into the HID Report Packet in the field
    corresponding to the given usage page and usage.  HidP_SetUsageValue
    casts this value to the appropriate bit length.  If a report packet
    contains two different fields with the same Usage and UsagePage,
    they can be distinguished with the optional LinkCollection field value.
    Using this function sets the raw value into the report packet with
    no checking done as to whether it actually falls within the logical
    minimum/logical maximum range.  Use HidP_SetScaledUsageValue for this...

    NOTE: Although the UsageValue parameter is a ULONG, any casting that is
          done will preserve or sign-extend the value.  The value being set
          should be considered a LONG value and will be treated as such by
          this function.

Parameters:

    ReportType  One of HidP_Output or HidP_Feature.

    UsagePage   The usage page to which the given usage refers.

    LinkCollection  (Optional)  This value can be used to differentiate
                                between two fields that may have the same
                                UsagePage and Usage but exist in different
                                collections.  If the link collection value
                                is zero, this function will set the first field
                                it finds that matches the usage page and
                                usage.

    Usage       The usage whose value HidP_SetUsageValue will set.

    UsageValue  The raw value to set in the report buffer.  This value must be within
                the logical range or if a NULL value this value should be the
                most negative value that can be represented by the number of bits
                for this field.

    PreparsedData The preparsed data returned for HIDCLASS

    Report      The report packet.

    ReportLength Length (in bytes) of the given report packet.


Return Value:
    HidP_SetUsageValue returns the following error codes:

  HIDP_STATUS_SUCCESS                -- upon successfully setting the value
                                        in the report packet
  HIDP_STATUS_INVALID_REPORT_TYPE    -- if ReportType is not valid.
  HIDP_STATUS_INVALID_PREPARSED_DATA -- if PreparsedData is not valid
  HIDP_STATUS_INVALID_REPORT_LENGTH  -- the length of the report packet is not
                                        equal to the length specified in
                                        the HIDP_CAPS structure for the given
                                        ReportType
  HIDP_STATUS_REPORT_DOES_NOT_EXIST  -- if there are no reports on this device
                                        for the given ReportType
  HIDP_STATUS_INCOMPATIBLE_REPORT_ID -- the specified usage page, usage and
                                        link collection exist but exists in
                                        a report with a different report ID
                                        than the report being passed in.  To
                                        set this value, call HidP_SetUsageValue
                                        again with a zero-initizialed report
                                        packet
  HIDP_STATUS_USAGE_NOT_FOUND        -- if the usage page, usage, and link
                                        collection combination does not exist
                                        in any reports for this ReportType
--*/
{
    PHIDP_PREPARSED_DATA_INTERNAL pData = (PHIDP_PREPARSED_DATA_INTERNAL)PreparsedData;
    
    if (PreparsedData == NULL || pData->Signature1 != 0x50646948 || pData->Signature2 != 0x52444B20) {
        return HIDP_STATUS_INVALID_PREPARSED_DATA;
    }
    
    PCHANNEL_REPORT_HEADER reportHeader;
    
    if (ReportType == HidP_Input) {
        reportHeader = &pData->Input;
    } else if (ReportType == HidP_Output) {
        reportHeader = &pData->Output;
    } else if (ReportType == HidP_Feature) {
        reportHeader = &pData->Feature;
    } else {
        return HIDP_STATUS_INVALID_REPORT_TYPE;
    }
    
    if ((USHORT)ReportLength != reportHeader->ByteLen) {
        return HIDP_STATUS_INVALID_REPORT_LENGTH;
    }
    
    if (reportHeader->ByteLen == 0) {
        return HIDP_STATUS_REPORT_DOES_NOT_EXIST;
    }
    
    PHIDP_CHANNEL_DESC channelDesc = (PHIDP_CHANNEL_DESC)((BYTE*)PreparsedData + pData->DescriptorOffset);
    BOOL foundMismatch = FALSE;
    
    for (USHORT channelIndex = reportHeader->Offset; channelIndex < reportHeader->Index; channelIndex++) {
        PHIDP_CHANNEL_DESC currentChannel = &channelDesc[channelIndex];
        
        if (((currentChannel->Flags & 4) == 0) && currentChannel->UsagePage == UsagePage) {
            if ((LinkCollection == 0 || currentChannel->LinkCollection == LinkCollection) &&
                ((Usage == 0 && currentChannel->LinkUsage == 0) || 
                 (LinkCollection != 0 && currentChannel->LinkCollection == LinkCollection))) {
                
                if ((currentChannel->Flags & 0x10) == 0) {
                    if (currentChannel->Usage == Usage) {
                        if (Report[0] == 0 || currentChannel->ReportID == (UCHAR)Report[0]) {
                            Report[0] = (char)currentChannel->ReportID;
                            HidP_InsertData(currentChannel->BitOffset, currentChannel->ReportID, (USHORT)ReportLength, (char*)(ULONG_PTR)UsageValue, 0, (ULONG)(ULONG_PTR)Report);
                            return HIDP_STATUS_SUCCESS;
                        }
                        foundMismatch = TRUE;
                    }
                } else if (currentChannel->UsageMin <= Usage && Usage <= currentChannel->UsageMax) {
                    return HIDP_STATUS_NOT_VALUE_ARRAY;
                }
            }
        }
    }
    
    if (foundMismatch) {
        return HIDP_STATUS_INCOMPATIBLE_REPORT_ID;
    }
    
    return HIDP_STATUS_USAGE_NOT_FOUND;
}

// HidP_SetUsageValueArray
_Must_inspect_result_
NTSTATUS __stdcall
HidP_SetUsageValueArray (
    _In_ HIDP_REPORT_TYPE ReportType,
    _In_ USAGE UsagePage,
    _In_opt_ USHORT LinkCollection,
    _In_ USAGE Usage,
    _In_reads_bytes_(UsageValueByteLength) PCHAR UsageValue,
    _In_ USHORT UsageValueByteLength,
    _In_ PHIDP_PREPARSED_DATA PreparsedData,
    _Inout_updates_bytes_(ReportLength) PCHAR Report,
    _In_ ULONG ReportLength
    )
/*++
Routine Descripton:
    A usage value array occurs when the last usage in the list of usages
    describing a main item must be repeated because there are less usages defined
    than there are report counts declared for the given main item.  In this case
    a single value cap is allocated for that usage and the report count of that
    value cap is set to reflect the number of fields to which that usage refers.

    HidP_SetUsageValueArray sets the raw bits for that usage which spans
    more than one field in a report.

    NOTE: This function currently does not support value arrays where the
          ReportSize for each of the fields in the array is not a multiple
          of 8 bits.

          The UsageValue buffer should have the values set as they would appear
          in the report buffer.  If this function supported non 8-bit multiples
          for the ReportSize then caller should format the input buffer so that
          each new value begins at the bit immediately following the last bit
          of the previous value

Parameters:

    ReportType  One of HidP_Output or HidP_Feature.

    UsagePage   The usage page to which the given usage refers.

    LinkCollection  (Optional)  This value can be used to differentiate
                                between two fields that may have the same
                                UsagePage and Usage but exist in different
                                collections.  If the link collection value
                                is zero, this function will set the first field
                                it finds that matches the usage page and
                                usage.

    Usage       The usage whose value array HidP_SetUsageValueArray will set.

    UsageValue  The buffer with the values to set into the value array.
                The number of BITS required is found by multiplying the
                BitSize and ReportCount fields of the Value Cap for this
                control.  The least significant bit of this control found in the
                given report will be placed in the least significan bit location
                of the array given (little-endian format), regardless of whether
                or not the field is byte alligned or if the BitSize is a multiple
                of sizeof (CHAR).

                See the above note for current implementation limitations.

    UsageValueByteLength  Length of the UsageValue buffer (in bytes)

    PreparsedData The preparsed data returned from HIDCLASS

    Report      The report packet.

    ReportLength Length (in bytes) of the given report packet.


Return Value:
  HIDP_STATUS_SUCCESS                -- upon successfully setting the value
                                        array in the report packet
  HIDP_STATUS_INVALID_REPORT_TYPE    -- if ReportType is not valid.
  HIDP_STATUS_INVALID_PREPARSED_DATA -- if PreparsedData is not valid
  HIDP_STATUS_INVALID_REPORT_LENGTH  -- the length of the report packet is not
                                        equal to the length specified in
                                        the HIDP_CAPS structure for the given
                                        ReportType
  HIDP_STATUS_REPORT_DOES_NOT_EXIST  -- if there are no reports on this device
                                        for the given ReportType
  HIDP_STATUS_NOT_VALUE_ARRAY        -- if the control specified is not a
                                        value array -- a value array will have
                                        a ReportCount field in the
                                        HIDP_VALUE_CAPS structure that is > 1
                                        Use HidP_SetUsageValue instead
  HIDP_STATUS_BUFFER_TOO_SMALL       -- if the size of the passed in buffer with
                                        the values to set is too small (ie. has
                                        fewer values than the number of fields in
                                        the array
  HIDP_STATUS_NOT_IMPLEMENTED        -- if the usage value array has field sizes
                                        that are not multiples of 8 bits, this
                                        error code is returned since the function
                                        currently does not handle setting into
                                        such arrays.
  HIDP_STATUS_INCOMPATIBLE_REPORT_ID -- the specified usage page, usage and
                                        link collection exist but exists in
                                        a report with a different report ID
                                        than the report being passed in.  To
                                        set this value, call
                                        HidP_SetUsageValueArray again with
                                        a zero-initialized report packet
  HIDP_STATUS_USAGE_NOT_FOUND        -- if the usage page, usage, and link
                                        collection combination does not exist
                                        in any reports for this ReportType
--*/
{
    PHIDP_PREPARSED_DATA_INTERNAL pData = (PHIDP_PREPARSED_DATA_INTERNAL)PreparsedData;
    
    if (PreparsedData == NULL || pData->Signature1 != 0x50646948 || pData->Signature2 != 0x52444B20) {
        return HIDP_STATUS_INVALID_PREPARSED_DATA;
    }
    
    PCHANNEL_REPORT_HEADER reportHeader;
    
    if (ReportType == HidP_Input) {
        reportHeader = &pData->Input;
    } else if (ReportType == HidP_Output) {
        reportHeader = &pData->Output;
    } else if (ReportType == HidP_Feature) {
        reportHeader = &pData->Feature;
    } else {
        return HIDP_STATUS_INVALID_REPORT_TYPE;
    }
    
    if ((USHORT)ReportLength != reportHeader->ByteLen) {
        return HIDP_STATUS_INVALID_REPORT_LENGTH;
    }
    
    if (reportHeader->ByteLen == 0) {
        return HIDP_STATUS_REPORT_DOES_NOT_EXIST;
    }
    
    PHIDP_CHANNEL_DESC channelDesc = (PHIDP_CHANNEL_DESC)((BYTE*)PreparsedData + pData->DescriptorOffset);
    BOOL foundMismatch = FALSE;
    
    for (USHORT channelIndex = reportHeader->Offset; channelIndex < reportHeader->Index; channelIndex++) {
        PHIDP_CHANNEL_DESC currentChannel = &channelDesc[channelIndex];
        
        if (((currentChannel->Flags & 4) == 0) && currentChannel->UsagePage == UsagePage) {
            if ((LinkCollection == 0 || currentChannel->LinkCollection == LinkCollection) &&
                ((Usage == 0 && currentChannel->LinkUsage == 0) || 
                 (LinkCollection != 0 && currentChannel->LinkCollection == LinkCollection))) {
                
                if ((currentChannel->Flags & 0x10) == 0) {
                    if (currentChannel->Usage == Usage) {
                        if (currentChannel->BitSize == 1) {
                            return HIDP_STATUS_NOT_VALUE_ARRAY;
                        }
                        
                        if (Report[0] == 0 || currentChannel->ReportID == (UCHAR)Report[0]) {
                            USHORT bitOffset = currentChannel->BitOffset;
                            ULONG bytesPerValue = currentChannel->BitSize / 8;
                            ULONG arrayBytes = currentChannel->ReportCount * bytesPerValue;
                            
                            if (arrayBytes > UsageValueByteLength) {
                                return HIDP_STATUS_BUFFER_TOO_SMALL;
                            }
                            
                            if ((bitOffset & 7) != 0) {
                                return HIDP_STATUS_INVALID_REPORT_LENGTH;
                            }
                            
                            ULONG byteOffset = bitOffset >> 3;
                            Report[0] = (char)currentChannel->ReportID;
                            
                            for (USHORT valueIndex = 0; valueIndex < currentChannel->ReportCount; valueIndex++) {
                                for (ULONG byteIndex = 0; byteIndex < bytesPerValue; byteIndex++) {
                                    Report[byteOffset + valueIndex * bytesPerValue + byteIndex] = UsageValue[valueIndex * bytesPerValue + byteIndex];
                                }
                            }
                            
                            return HIDP_STATUS_SUCCESS;
                        }
                        foundMismatch = TRUE;
                    }
                } else if (currentChannel->UsageMin <= Usage && Usage <= currentChannel->UsageMax) {
                    return HIDP_STATUS_NOT_VALUE_ARRAY;
                }
            }
        }
    }
    
    if (foundMismatch) {
        return HIDP_STATUS_INCOMPATIBLE_REPORT_ID;
    }
    
    return HIDP_STATUS_USAGE_NOT_FOUND;
}

// HidP_UnsetUsages
_Must_inspect_result_
NTSTATUS __stdcall
HidP_UnsetUsages (
   _In_ HIDP_REPORT_TYPE      ReportType,
   _In_ USAGE   UsagePage,
   _In_opt_ USHORT  LinkCollection,
   _Inout_updates_to_(*UsageLength,*UsageLength) PUSAGE  UsageList,
   _Inout_  PULONG  UsageLength,
   _In_ PHIDP_PREPARSED_DATA  PreparsedData,
   _In_reads_bytes_(ReportLength) PCHAR   Report,
   _In_ ULONG   ReportLength
   )
/*++

Routine Description:
    This function unsets (turns off) binary values (buttons) in the report.  Given
    an initialized packet of correct length, it modifies the report packet so
    that each element in the given list of usages has been unset in the
    report packet.

    This function is the "undo" operation for SetUsages.  If the given usage
    is not already set in the Report, it will return an error code of
    HIDP_STATUS_BUTTON_NOT_PRESSED.  If the button is pressed, HidP_UnsetUsages
    will unset the appropriate bit or remove the corresponding index value from
    the HID Main Array Item.

    A properly initialized Report packet is one of the correct byte length,
    and all zeros..

    NOTE: A packet that has already been set with a call to a HidP_Set routine
          can also be passed in.  This routine then processes the UsageList
          in the same fashion but verifies that the ReportID already set in
          Report matches the report ID for the given usages.

Parameters:
    ReportType  One of HidP_Input, HidP_Output or HidP_Feature.

    UsagePage   All of the usages in the usage array, which HidP_UnsetUsages will
                unset in the report, refer to this same usage page.
                If a client wishes to unset usages in a report for multiple
                usage pages then that client needs to make multiple calls to
                HidP_UnsetUsages for each of the usage pages.

    UsageList   A usage array containing the usages that HidP_UnsetUsages will
                unset in the report packet.

    UsageLength The length of the given usage array in array elements.
                The parser will set this value to the position in the usage
                array where it stopped processing.  If successful, UsageLength
                will be unchanged.  In any error condition, this parameter
                reflects how many of the usages in the usage list have
                actually been unset by the parser.  This is useful for finding
                the usage in the list which caused the error.

    PreparsedData The preparsed data recevied from HIDCLASS

    Report      The report packet.

    ReportLength   Length of the given report packet...Must be equal to the
                   value reported in the HIDP_CAPS structure for the device
                   and corresponding report type.

Return Value
    HidP_UnsetUsages returns the following error codes.  On error, the report
    packet will be correct up until the usage element that caused the error.

  HIDP_STATUS_SUCCESS                -- upon successful "unsetting" of all usages
                                        in the report packet.
  HIDP_STATUS_INVALID_REPORT_TYPE    -- if ReportType is not valid.
  HIDP_STATUS_INVALID_PREPARSED_DATA -- if PreparsedData is not valid
  HIDP_STATUS_INVALID_REPORT_LENGTH  -- the length of the report packet is not
                                        equal to the length specified in
                                        the HIDP_CAPS structure for the given
                                        ReportType
  HIDP_STATUS_REPORT_DOES_NOT_EXIST  -- if there are no reports on this device
                                        for the given ReportType
  HIDP_STATUS_INCOMPATIBLE_REPORT_ID -- if a usage was found that exists in a
                                        different report.  If the report is
                                        zero-initialized on entry the first
                                        usage in the list will determine which
                                        report ID is used.  Otherwise, the
                                        parser will verify that usage matches
                                        the passed in report's ID
  HIDP_STATUS_USAGE_NOT_FOUND        -- if the usage does not exist for any
                                        report (no matter what the report ID)
                                        for the given report type.
  HIDP_STATUS_BUTTON_NOT_PRESSED     -- if a usage corresponds to a button that
                                        is not already set in the given report
--*/
{
    PHIDP_PREPARSED_DATA_INTERNAL pData = (PHIDP_PREPARSED_DATA_INTERNAL)PreparsedData;
    
    if (PreparsedData == NULL || pData->Signature1 != 0x50646948 || pData->Signature2 != 0x52444B20) {
        return HIDP_STATUS_INVALID_PREPARSED_DATA;
    }
    
    PCHANNEL_REPORT_HEADER reportHeader;
    
    if (ReportType == HidP_Input) {
        reportHeader = &pData->Input;
    } else if (ReportType == HidP_Output) {
        reportHeader = &pData->Output;
    } else if (ReportType == HidP_Feature) {
        reportHeader = &pData->Feature;
    } else {
        return HIDP_STATUS_INVALID_REPORT_TYPE;
    }
    
    if ((USHORT)ReportLength != reportHeader->ByteLen) {
        return HIDP_STATUS_INVALID_REPORT_LENGTH;
    }
    
    if (reportHeader->ByteLen == 0) {
        return HIDP_STATUS_REPORT_DOES_NOT_EXIST;
    }
    
    PHIDP_CHANNEL_DESC channelDesc = (PHIDP_CHANNEL_DESC)((BYTE*)PreparsedData + pData->DescriptorOffset);
    
    for (ULONG i = 0; i < *UsageLength; i++) {
        USAGE currentUsage = UsageList[i];
        BOOL found = FALSE;
        
        if (currentUsage == 0) continue;
        
        for (USHORT channelIndex = reportHeader->Offset; channelIndex < reportHeader->Index; channelIndex++) {
            PHIDP_CHANNEL_DESC current = &channelDesc[channelIndex];
            
            if ((current->Flags & 4) != 0 && current->UsagePage == UsagePage && 
                (LinkCollection == 0 || current->LinkCollection == LinkCollection)) {
                
                if ((current->Flags & 0x10) != 0) {
                    if (current->UsageMin <= currentUsage && currentUsage <= current->UsageMax) {
                        if (Report[0] == 0 || current->ReportID == (UCHAR)Report[0]) {
                            Report[0] = (char)current->ReportID;
                            
                            if (current->BitSize == 1) {
                                USHORT bitOffset = current->BitOffset + (currentUsage - current->UsageMin);
                                UCHAR byteMask = 1 << (bitOffset & 7);
                                Report[bitOffset >> 3] &= ~byteMask;
                            } else {
                                ULONG dataIndex = currentUsage - current->UsageMin;
                                // For arrays, unset means remove from the array
                                HidP_InsertData(current->BitOffset, current->ReportID, (USHORT)ReportLength, NULL, 0, (ULONG)(ULONG_PTR)Report);
                            }
                            found = TRUE;
                            break;
                        }
                    }
                }
            }
        }
        
        if (!found) {
            *UsageLength = i;
            return HIDP_STATUS_USAGE_NOT_FOUND;
        }
    }
    
    return HIDP_STATUS_SUCCESS;
}

// HidP_Usage2Index
ULONG __cdecl HidP_Usage2Index(PHIDP_CHANNEL_DESC channelDesc, USHORT usage)
{
    PHIDP_CHANNEL_DESC currentDesc = channelDesc;
    PHIDP_CHANNEL_DESC tempDesc;
    USHORT* ptr;
    USHORT usageMin;
    ULONG index = 0;
    
    if (channelDesc == NULL) {
        return 0;
    }
    
    while ((currentDesc->Flags & 1) != 0) {
        tempDesc = currentDesc + 0x34;
        ptr = (USHORT*)((char*)currentDesc + 0x40);
        currentDesc = tempDesc;
        if ((*ptr & 1) == 0) {
            break;
        }
    }
    
    if (currentDesc < channelDesc) {
        return 0;
    }
    
    currentDesc += 0x1E;
    
    do {
        if ((currentDesc[-0x12].Flags & 0x10) == 0) {
            index++;
            if (usage == currentDesc->Usage) {
                return index;
            }
        } else {
            usageMin = currentDesc->UsageMin;
            if (usageMin == 0) {
                usageMin = 1;
            }
            if (usageMin <= usage && usage <= currentDesc[1].UsageMax) {
                return index + 1 + (usage - usageMin);
            }
            index += 1 + (currentDesc[1].UsageMax - usageMin);
        }
        
        currentDesc -= 0x34;
        
        if ((currentDesc + 0x52) < channelDesc) {
            return 0;
        }
    } while (1);
}

// HidP_GetButtonArray
_Must_inspect_result_
NTSTATUS __stdcall
HidP_GetButtonArray(
    _In_ HIDP_REPORT_TYPE ReportType,
    _In_ USAGE UsagePage,
    _In_opt_ USHORT LinkCollection,
    _In_ USAGE Usage,
    _Out_writes_to_(*ButtonDataLength, *ButtonDataLength) PHIDP_BUTTON_ARRAY_DATA ButtonData,
    _Inout_ PUSHORT ButtonDataLength,
    _In_ PHIDP_PREPARSED_DATA PreparsedData,
    _In_reads_bytes_(ReportLength) PCHAR Report,
    _In_ ULONG ReportLength
)
{
    USHORT currentButtonIndex;
    USHORT channelIndex;
    USHORT bitCount;
    USHORT bitPosition;
    USHORT dataIndexMin;
    USHORT startBit;
    USHORT channelCount;
    USHORT buttonPosition;
    ULONG byteOffset;
    UCHAR bitMask;
    UCHAR flagByte;
    SHORT linkCollectionValue;
    SHORT tempShort;
    ULONG returnValue;
    ULONG extractedValue;
    UCHAR foundMismatch;
    PHIDP_PREPARSED_DATA_INTERNAL pData;
    PCHANNEL_REPORT_HEADER reportHeader;
    PHIDP_CHANNEL_DESC channelDesc;
    PHIDP_CHANNEL_DESC currentChannel;
    
    currentButtonIndex = 0;
    returnValue = HIDP_STATUS_SUCCESS;
    foundMismatch = 0;
    
    pData = (PHIDP_PREPARSED_DATA_INTERNAL)PreparsedData;
    
    if ((PreparsedData == NULL) || (pData->Signature1 != 0x50646948) || (pData->Signature2 != 0x52444B20)) {
        return HIDP_STATUS_INVALID_PREPARSED_DATA;
    }
    
    memset(ButtonData, 0, (ULONG_PTR)*ButtonDataLength * sizeof(HIDP_BUTTON_ARRAY_DATA));
    
    if (ReportType == HidP_Input) {
        reportHeader = &pData->Input;
    } else if (ReportType == HidP_Output) {
        reportHeader = &pData->Output;
    } else if (ReportType == HidP_Feature) {
        reportHeader = &pData->Feature;
    } else {
        return HIDP_STATUS_INVALID_REPORT_TYPE;
    }
    
    if (reportHeader->ByteLen == 0) {
        return HIDP_STATUS_REPORT_DOES_NOT_EXIST;
    }
    
    if ((USHORT)ReportLength != reportHeader->ByteLen) {
        return HIDP_STATUS_INVALID_REPORT_LENGTH;
    }
    
    channelDesc = (PHIDP_CHANNEL_DESC)((BYTE*)PreparsedData + pData->DescriptorOffset);
    
    for (channelIndex = reportHeader->Offset; channelIndex < reportHeader->Index; channelIndex++) {
        currentChannel = &channelDesc[channelIndex];
        
        if (((currentChannel->Flags & 4) == 0) && (currentChannel->BitSize == 1)) {
            continue;
        }
        
        if (((currentChannel->Flags & 4) != 0) &&
            ((UsagePage == 0) || (currentChannel->UsagePage == UsagePage)) &&
            ((LinkCollection == 0) || (currentChannel->LinkCollection == LinkCollection)) &&
            ((LinkCollection == 0) || (tempShort = currentChannel->LinkCollection, LinkCollection == tempShort) ||
             ((LinkCollection == -1) && (tempShort == 0)))) {
            
            if ((currentChannel->Flags & 0x10) == 0) {
                if (currentChannel->Usage == Usage) {
                    if (currentChannel->BitSize == 1) {
                        return HIDP_STATUS_NOT_BUTTON_ARRAY;
                    }
                    
                    if ((Report[0] == 0) || (currentChannel->ReportID == (UCHAR)Report[0])) {
                        dataIndexMin = currentChannel->DataIndexMin;
                        startBit = currentChannel->BitOffset;
                        bitCount = currentChannel->BitSize;
                        
                        if (bitCount != 0) {
                            for (buttonPosition = 0; buttonPosition < bitCount; buttonPosition++) {
                                bitPosition = startBit + buttonPosition;
                                byteOffset = bitPosition >> 3;
                                bitMask = 1 << (bitPosition & 7);
                                
                                if ((Report[byteOffset] & bitMask) != 0) {
                                    if (currentButtonIndex < *ButtonDataLength) {
                                        ButtonData[currentButtonIndex].ArrayIndex = buttonPosition;
                                        ButtonData[currentButtonIndex].On = 1;
                                    } else {
                                        returnValue = HIDP_STATUS_BUFFER_TOO_SMALL;
                                    }
                                    currentButtonIndex++;
                                }
                            }
                        }
                        
                        *ButtonDataLength = currentButtonIndex;
                        return returnValue;
                    }
                    foundMismatch = 1;
                }
            } else if ((currentChannel->UsageMin <= Usage) && (Usage <= currentChannel->UsageMax)) {
                return HIDP_STATUS_NOT_BUTTON_ARRAY;
            }
        }
    }
    
    returnValue = (foundMismatch ? HIDP_STATUS_INCOMPATIBLE_REPORT_ID : HIDP_STATUS_USAGE_NOT_FOUND);
    return returnValue;
}

// HidP_SetButtonArray
_Must_inspect_result_
NTSTATUS __stdcall
HidP_SetButtonArray(
    _In_ HIDP_REPORT_TYPE ReportType,
    _In_ USAGE UsagePage,
    _In_opt_ USHORT LinkCollection,
    _In_ USAGE Usage,
    _In_reads_(ButtonDataLength) PHIDP_BUTTON_ARRAY_DATA ButtonData,
    _In_ USHORT ButtonDataLength,
    _In_ PHIDP_PREPARSED_DATA PreparsedData,
    _Inout_updates_bytes_(ReportLength) PCHAR Report,
    _In_ ULONG ReportLength
)
{
    USHORT channelIndex;
    USHORT buttonIndex;
    USHORT bitCount;
    USHORT startBit;
    USHORT arrayIndex;
    ULONG byteOffset;
    UCHAR bitMask;
    UCHAR flagByte;
    SHORT linkCollectionValue;
    SHORT tempShort;
    ULONG returnValue;
    ULONG dataIndexMin;
    UCHAR foundMismatch;
    UCHAR baseOffset;
    UCHAR currentValue;
    PHIDP_PREPARSED_DATA_INTERNAL pData;
    PCHANNEL_REPORT_HEADER reportHeader;
    PHIDP_CHANNEL_DESC channelDesc;
    PHIDP_CHANNEL_DESC currentChannel;
    
    buttonIndex = 0;
    foundMismatch = 0;
    
    pData = (PHIDP_PREPARSED_DATA_INTERNAL)PreparsedData;
    
    if ((PreparsedData == NULL) || (pData->Signature1 != 0x50646948) || (pData->Signature2 != 0x52444B20)) {
        return HIDP_STATUS_INVALID_PREPARSED_DATA;
    }
    
    if (ReportType == HidP_Input) {
        reportHeader = &pData->Input;
    } else if (ReportType == HidP_Output) {
        reportHeader = &pData->Output;
    } else if (ReportType == HidP_Feature) {
        reportHeader = &pData->Feature;
    } else {
        return HIDP_STATUS_INVALID_REPORT_TYPE;
    }
    
    if (reportHeader->ByteLen == 0) {
        return HIDP_STATUS_REPORT_DOES_NOT_EXIST;
    }
    
    if ((USHORT)ReportLength != reportHeader->ByteLen) {
        return HIDP_STATUS_INVALID_REPORT_LENGTH;
    }
    
    channelDesc = (PHIDP_CHANNEL_DESC)((BYTE*)PreparsedData + pData->DescriptorOffset);
    
    for (channelIndex = reportHeader->Offset; channelIndex < reportHeader->Index; channelIndex++) {
        currentChannel = &channelDesc[channelIndex];
        
        if (((currentChannel->Flags & 4) == 0) && (currentChannel->BitSize == 1)) {
            continue;
        }
        
        if (((currentChannel->Flags & 4) != 0) &&
            ((UsagePage == 0) || (currentChannel->UsagePage == UsagePage)) &&
            ((LinkCollection == 0) || (currentChannel->LinkCollection == LinkCollection)) &&
            ((LinkCollection == 0) || (tempShort = currentChannel->LinkCollection, LinkCollection == tempShort) ||
             ((LinkCollection == -1) && (tempShort == 0)))) {
            
            if ((currentChannel->Flags & 0x10) != 0) {
                if ((currentChannel->UsageMin <= Usage) && (Usage <= currentChannel->UsageMax)) {
                    return HIDP_STATUS_NOT_BUTTON_ARRAY;
                }
            }
            
            if (currentChannel->Usage != Usage) {
                continue;
            }
            
            if (currentChannel->BitSize == 1) {
                return HIDP_STATUS_NOT_BUTTON_ARRAY;
            }
            
            if ((Report[0] != 0) && (currentChannel->ReportID != (UCHAR)Report[0])) {
                foundMismatch = 1;
                continue;
            }
            
            dataIndexMin = currentChannel->DataIndexMin;
            baseOffset = (UCHAR)currentChannel->BitOffset;
            bitCount = currentChannel->BitSize;
            
            while (buttonIndex < ButtonDataLength) {
                arrayIndex = ButtonData[buttonIndex].ArrayIndex;
                
                if (bitCount <= arrayIndex) {
                    return HIDP_STATUS_DATA_INDEX_OUT_OF_RANGE;
                }
                
                byteOffset = (baseOffset + arrayIndex) >> 3;
                bitMask = 1 << ((baseOffset + arrayIndex) & 7);
                
                if (ReportLength <= byteOffset) {
                    break;
                }
                
                if (ButtonData[buttonIndex].On == 0) {
                    currentValue = Report[byteOffset] & ~bitMask;
                } else {
                    currentValue = Report[byteOffset] | bitMask;
                }
                
                Report[byteOffset] = (CHAR)currentValue;
                buttonIndex++;
            }
            
            return HIDP_STATUS_SUCCESS;
        }
        
    }
    
    returnValue = (foundMismatch ? HIDP_STATUS_INCOMPATIBLE_REPORT_ID : HIDP_STATUS_USAGE_NOT_FOUND);
    return returnValue;
}

// HidP_GetVersion
_Must_inspect_result_
NTSTATUS __stdcall
HidP_GetVersionInternal(
    _Out_ PULONG Version
)
{
    *Version = 2;
    return HIDP_STATUS_SUCCESS;
}
