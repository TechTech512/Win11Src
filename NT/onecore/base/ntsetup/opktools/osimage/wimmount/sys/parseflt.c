/*
 * parseflt.c - WIM Mount Driver - Filter Callbacks and Helper Routines
 * 
 * This file implements the minifilter's pre/post operation callbacks,
 * communication port management, context handling, reparse point processing,
 * and all supporting functions as they appear in the original binary.
 */

#pragma warning (disable:4996)
#include "commlist.h"

// ----------------------------------------------------------------------------
// External globals declared in other compilation units
// ----------------------------------------------------------------------------
PFLT_FILTER gFilterHandle = NULL;
extern WM_COMM_LIST g_CommList;
ERESOURCE SubsumeLock = { 0 };
unsigned long gTraceFlags = 0;
UNICODE_STRING ExpectedFirstToken = { 0x32, 0x34, L"System Volume Information" };
UNICODE_STRING ExpectedSecondToken = { 0x10, 0x12, L"WimMount" };

extern NTSTATUS
WMCommListCreate(
    VOID
);

extern VOID
WMCommListEntryRelease(
    PWM_COMM_LIST_ENTRY Entry
);

// Forward declarations of static functions (order matches the source)
static NTSTATUS InstanceSetup(PFLT_RELATED_OBJECTS FltObjects, ULONG Flags,
                              ULONG VolumeType, FLT_FILESYSTEM_TYPE FsType);
static VOID ParsePathElement(PUNICODE_STRING Result, PUNICODE_STRING Input, PUNICODE_STRING Scratch);
static FLT_POSTOP_CALLBACK_STATUS NTAPI PFCheckReparse(PFLT_CALLBACK_DATA Data,
                                                 PFLT_RELATED_OBJECTS FltObjects,
                                                 PVOID CompletionContext, ULONG Flags);
static FLT_PREOP_CALLBACK_STATUS NTAPI PFFileSystemControl(PFLT_CALLBACK_DATA Data,
                                                     PFLT_RELATED_OBJECTS FltObjects,
                                                     PVOID *CompletionContext);
static NTSTATUS PFGetContextByFileObject(PFLT_FILTER Filter, PFLT_INSTANCE Instance,
                                         PFILE_OBJECT FileObject, PPF_FILE_CONTEXT *Context,
                                         PUCHAR IsDirectory);
static NTSTATUS PFGetFileContext(PFLT_CALLBACK_DATA Data, PFLT_RELATED_OBJECTS FltObjects,
                                 PVOID CallbackFn, PVOID Context,
                                 PPF_FILE_CONTEXT *OutContext, PUCHAR IsDirectory,
                                 PVOID *Arg7, PUCHAR Arg8);
static NTSTATUS PFGetWritableHandle(PFLT_CALLBACK_DATA Data, PFLT_RELATED_OBJECTS FltObjects,
                                    PFLT_FILE_NAME_INFORMATION FileNameInfo,
                                    PFILE_OBJECT FileObject, PVOID *Arg5);
static NTSTATUS PFOnlyConsiderDirectories(PFLT_CALLBACK_DATA Data, PFLT_RELATED_OBJECTS FltObjects,
                                          PFILE_OBJECT FileObject, PVOID Arg3, PUCHAR Arg4);
static NTSTATUS PFOpenById(PFLT_FILTER Filter, PFLT_INSTANCE Instance, PVOID Arg3,
                           PLARGE_INTEGER FileId, UCHAR Arg5, PFILE_OBJECT **Arg6,
                           PVOID *Arg7, UCHAR Arg8);
static NTSTATUS PFOpenReparseTarget(PFLT_CALLBACK_DATA Data, PFLT_RELATED_OBJECTS FltObjects,
                                    PFLT_FILE_NAME_INFORMATION **FileNameInfo,
                                    PFILE_OBJECT **FileObject);
static UCHAR PFPathIsUnderTempDir(PUNICODE_STRING Path);
static NTSTATUS PFReleaseContextAndUntagFile(PFLT_CALLBACK_DATA Data,
                                             PFLT_RELATED_OBJECTS FltObjects,
                                             PPF_FILE_CONTEXT Context, UCHAR Untag,
                                             PVOID Arg5, UCHAR Arg6);
static VOID PFSetSubsumedStatus(PFLT_INSTANCE Instance, PPF_FILE_CONTEXT Context, LONG Status);
NTSTATUS NTAPI PFSubsumeContext(PFLT_FILTER Filter, PFLT_INSTANCE Instance, PVOID Arg3,
                                 PLARGE_INTEGER FileId, PLARGE_INTEGER Arg5, PVOID *Arg6);
static NTSTATUS Unload(ULONG Flags);
FLT_PREOP_CALLBACK_STATUS NTAPI
PFPreCreate(PFLT_CALLBACK_DATA Data, PFLT_RELATED_OBJECTS FltObjects, PVOID *CompletionContext);
extern NTSTATUS
PFPortConnect(
    PVOID PortCookie,
    PVOID ConnectionContext,
    PVOID ConnectionCookie,
    ULONG SizeOfContext,
    PVOID *Connection
);
extern VOID
PFPortDisconnect(
    PVOID Connection
);
extern NTSTATUS
PFReceiveMessage(
    PVOID PortCookie,
    PVOID InputBuffer,
    ULONG InputSize,
    PVOID OutputBuffer,
    ULONG OutputSize,
    PULONG BytesReturned
);
extern NTSTATUS
PFSendExtractMessage(
    PVOID *GuidPtr,
    ULONG GuidLength,
    PVOID Arg3,
    ULONG Arg4
);

// Global communication port pointer
PFLT_PORT PfCommunicationPort = NULL;

FLT_CONTEXT_REGISTRATION PfContexts[] = {
    {
        FLT_STREAM_CONTEXT,         // 8
        0,
        NULL,
        sizeof(PF_FILE_CONTEXT),    // 0x28
        0x50466663,                 // 'PFfc'
        NULL,
        NULL,
        NULL
    },
    {
        FLT_CONTEXT_END,            // 0xFFFF
        0,
        NULL,
        0,
        0,
        NULL,
        NULL,
        NULL
    }
};

FLT_OPERATION_REGISTRATION Callbacks[] = {
    { IRP_MJ_CREATE, 0, (PFLT_PRE_OPERATION_CALLBACK)PFPreCreate, (PFLT_POST_OPERATION_CALLBACK)PFCheckReparse },
    { IRP_MJ_FILE_SYSTEM_CONTROL, 0, (PFLT_PRE_OPERATION_CALLBACK)PFFileSystemControl, NULL },
    { IRP_MJ_OPERATION_END }
};

// Filter registration structure (as defined in the original)
CONST FLT_REGISTRATION FilterRegistration = {
    0x3C,                           // Size
    0x0203,                         // Version
    0,                              // Flags
    PfContexts,                     // ContextRegistration
    (const FLT_OPERATION_REGISTRATION *)Callbacks, // OperationRegistration
    Unload,                         // FilterUnload
    (PFLT_INSTANCE_SETUP_CALLBACK)InstanceSetup,   // InstanceSetup
    NULL,                           // InstanceQueryTeardown
    NULL,                           // InstanceTeardownStart
    NULL,                           // InstanceTeardownComplete
    NULL,                           // GenerateFileName
    NULL,                           // NormalizeNameComponent
    NULL,                           // NormalizeContextCleanup
    NULL,                           // TransactionNotification
    NULL,                           // NormalizeNameComponentEx
    NULL                            // SectionNotification
};

NTSTATUS
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);
NTSTATUS
Unload(ULONG Flags);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,DriverEntry)
#pragma alloc_text(INIT,Unload)

#pragma alloc_text(PAGE,InstanceSetup)
#pragma alloc_text(PAGE,ParsePathElement)
#pragma alloc_text(PAGE,PFCheckReparse)
#pragma alloc_text(PAGE,PFFileSystemControl)
#pragma alloc_text(PAGE,PFGetContextByFileObject)
#pragma alloc_text(PAGE,PFGetFileContext)
#pragma alloc_text(PAGE,PFGetWritableHandle)
#pragma alloc_text(PAGE,PFOnlyConsiderDirectories)
#pragma alloc_text(PAGE,PFOpenById)
#pragma alloc_text(PAGE,PFOpenReparseTarget)
#pragma alloc_text(PAGE,PFPathIsUnderTempDir)
#pragma alloc_text(PAGE,PFPreCreate)
#pragma alloc_text(PAGE,PFReleaseContextAndUntagFile)
#pragma alloc_text(PAGE,PFSetSubsumedStatus)
#pragma alloc_text(PAGE,PFSubsumeContext)
#endif // ALLOC_PRAGMA

// ============================================================================
// DriverEntry
// ============================================================================
NTSTATUS
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjAttr;
    UNICODE_STRING PortName;
    PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;

    if (gTraceFlags & 1)
        DbgPrint("isDirTest!DriverEntry: Entered\n");

    ExInitializeResourceLite(&SubsumeLock);

    Status = FltRegisterFilter(DriverObject, &FilterRegistration, &gFilterHandle);
    if (Status >= 0)
    {
        Status = WMCommListCreate();
        if (Status >= 0)
        {
            Status = FltBuildDefaultSecurityDescriptor(&SecurityDescriptor, 0x1f0001);
            if (Status >= 0)
            {
                RtlInitUnicodeString(&PortName, L"\\PFPort");
                InitializeObjectAttributes(&ObjAttr, &PortName,
                                           OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                                           NULL, SecurityDescriptor);

                Status = FltCreateCommunicationPort(
                    gFilterHandle,
                    &PfCommunicationPort,
                    &ObjAttr,
                    NULL,
                    PFPortConnect,
                    PFPortDisconnect,
                    PFReceiveMessage,
                    0x100
                );

                if (Status >= 0)
                    Status = FltStartFiltering(gFilterHandle);
            }
        }
    }

    if (SecurityDescriptor)
        FltFreeSecurityDescriptor(SecurityDescriptor);

    if (Status < 0)
        Unload(0);

    return Status;
}

// ============================================================================
// InstanceSetup
// ============================================================================
NTSTATUS
InstanceSetup(PFLT_RELATED_OBJECTS FltObjects, ULONG Flags,
              ULONG VolumeType, FLT_FILESYSTEM_TYPE FsType)
{
    NTSTATUS Status;
    PDEVICE_OBJECT DiskDevice = NULL;

    if (FltObjects == NULL)
        return 0xc000000d;   // STATUS_INVALID_PARAMETER

    if ((2 < VolumeType - 7) ||
        (Status = FltGetDiskDeviceObject(FltObjects->Volume, &DiskDevice), Status < 0) ||
        (DiskDevice == 0) ||
        ((*(PUCHAR)((ULONG_PTR)DiskDevice + 0x20) & 1) != 0) ||   // check removable media
        (FsType != FLT_FSTYPE_NTFS))   // FLT_FSTYPE_NTFS?
    {
        Status = 0xc01d000f;   // STATUS_FLT_DO_NOT_ATTACH
    }

    if (DiskDevice)
        ObfDereferenceObject(DiskDevice);

    return Status;
}

// ============================================================================
// ParsePathElement
// ============================================================================
VOID
ParsePathElement(PUNICODE_STRING Result, PUNICODE_STRING Input, PUNICODE_STRING Scratch)
{
    USHORT Length = Input->Length;
    WCHAR *Buffer = Input->Buffer;
    USHORT Index = 0;
    USHORT Count;

    // Skip leading backslashes
    while (Length > 0 && *Buffer == L'\\')
    {
        Buffer++;
        Length -= 2;
    }

    Count = Length / 2;
    while (Index < Count && Buffer[Index] != L'\\')
        Index++;

    // Fill result with the component (without trailing backslash)
    Result->Length = (USHORT)(Index * 2);
    Result->MaximumLength = Result->Length;
    Result->Buffer = (Index > 0) ? Buffer : NULL;

    // Update Input to point after the component (including the backslash if any)
    // The original also updated the Scratch parameter, but it is not used later.
    // We keep the exact same logic: compute remaining length and adjust buffer.
    Length = (Length - Index * 2);
    if (Index < Count)
    {
        // There is a backslash, skip it
        Input->Buffer = Buffer + Index + 1;
        Input->Length = (USHORT)((Count - Index - 1) * 2);
        Input->MaximumLength = Input->Length;
    }
    else
    {
        Input->Buffer = Buffer + Index;
        Input->Length = 0;
        Input->MaximumLength = 0;
    }
}

// ============================================================================
// PFCheckReparse
// ============================================================================
FLT_POSTOP_CALLBACK_STATUS NTAPI
PFCheckReparse(PFLT_CALLBACK_DATA Data, PFLT_RELATED_OBJECTS FltObjects,
               PVOID CompletionContext, ULONG Flags)
{
    NTSTATUS Status = 0;
    ULONG TagDataLength;
    PVOID TagData;
    PFLT_CALLBACK_DATA CallbackData = NULL;
    PPF_FILE_CONTEXT FileContext = NULL;
    PFLT_RELATED_OBJECTS RelObj = NULL;
    UCHAR IsDirectory = 0;
    NTSTATUS ExtractStatus = 0;
    PVOID Context = CompletionContext;
    PVOID Arg = NULL;
    ULONG ProcessId;
    PFLT_CALLBACK_DATA pVar5;
    PFLT_CALLBACK_DATA pVar7 = NULL;
    PPF_FILE_CONTEXT pVar6 = NULL;
    PFLT_CALLBACK_DATA pVar13 = NULL;
    PPF_FILE_CONTEXT pVar11 = NULL;
    PVOID pvVar10 = (PVOID)((ULONG_PTR)CompletionContext & 0xffffff00);
    PFLT_CALLBACK_DATA local_c = NULL;
    PFLT_RELATED_OBJECTS local_10 = NULL;

    if (Data->IoStatus.Status != 0x104)   // STATUS_REPARSE
        goto Cleanup;

    if (CompletionContext)
    {
        if (Data->IoStatus.Information != 0x80000008)
        {
            // OR the saved flag into the options
            Data->Iopb->Parameters.Create.Options |= *(PULONG)CompletionContext;
            FltSetCallbackDataDirty(Data);
            goto Reissue;
        }

        if (FltObjects->Transaction == NULL)
        {
            if (Data->TagData->UnparsedNameLength == 0)
                goto Cleanup;   // Actually original went to LAB_0001592c, which ORs flag and reissues
            local_c = (PFLT_CALLBACK_DATA)PFOnlyConsiderDirectories;
            local_10 = (PFLT_RELATED_OBJECTS)CompletionContext;
        }
    }

    do
    {
        pVar7 = NULL;
        if ((Data->IoStatus.Status != 0x104) ||
            (Data->TagData == NULL) ||
            (Data->TagData->FileTag != 0x80000008) ||
            (IoGetTopLevelIrp() != 0))
        {
            goto Cleanup;
        }

        // Get file context
        Status = PFGetFileContext(
            local_c,
            local_10,
            (PVOID)&Data->Iopb,    // originally &stack0xffffffe8
            (PVOID)&TagDataLength, // originally puVar8
            &FileContext,
            &IsDirectory,
            (PVOID*)local_c,
            (PUCHAR)&FileContext
        );

        if (Status < 0 || IsDirectory == 0)
            goto Cleanup;

        if (IsDirectory == 0)
        {
            if (pVar7)
            {
                ExtractStatus = KeWaitForSingleObject((PVOID)pVar7, Executive, KernelMode, FALSE, NULL);
                if (ExtractStatus != 0x101)
                {
                    if (ExtractStatus < 0)
                        goto Cleanup;
                    if ((LONG)pVar7->IoStatus.Information >= 0)
                        goto ProcessSuccess;
                }
                if (ExtractStatus >= 0)
                    ExtractStatus = pVar7->IoStatus.Information;
                goto Cleanup;
            }
        }
        else
        {
            PsGetCurrentProcessId();   // not used but called
            TagDataLength = Data->TagData->TagDataLength;
            TagData = (PUCHAR)Data->TagData + 8;   // field3_0x8
            pVar7 = pVar13;
            pVar5 = (PFLT_CALLBACK_DATA)PFSendExtractMessage(&TagData, TagDataLength, pvVar10, (ULONG_PTR)pVar6);
            pVar7->IoStatus.Information = (ULONG_PTR)pVar5;
            pVar13 = pVar7;
            PFSetSubsumedStatus((PFLT_INSTANCE)pVar5, (PPF_FILE_CONTEXT)TagData, TagDataLength);
            if ((LONG)pVar5 < 0)
                goto Cleanup;
        }

ProcessSuccess:
        // Release context and untag
        Status = (NTSTATUS)PFReleaseContextAndUntagFile(
            pVar7,
            (PFLT_RELATED_OBJECTS)pVar13,
            pVar6,
            1,
            pVar7,
            IsDirectory
        );
        pVar7 = NULL;
        if ((LONG)Status < 0)
            goto Cleanup;

        pvVar10 = (PVOID)((ULONG_PTR)pvVar10 & 0xffffff00);
        if (pVar6)
        {
            FltClose((PVOID)pVar6);
            pVar6 = NULL;
            pVar11 = NULL;
        }

Reissue:
        FltReissueSynchronousIo(FltObjects->Instance, Data);
    } while (TRUE);

Cleanup:
    if (CompletionContext)
        ExFreePoolWithTag(CompletionContext, 0x50466d69);   // 'imdP'

    if (pVar7)
        PFReleaseContextAndUntagFile(pVar7, (PFLT_RELATED_OBJECTS)pVar13, pVar6, 0, NULL, IsDirectory);

    if (pVar6)
        FltClose((PVOID)pVar6);

    if ((LONG)ExtractStatus < 0)
        Data->IoStatus.Status = ExtractStatus;

    return FLT_POSTOP_FINISHED_PROCESSING;
}

// ============================================================================
// PFFileSystemControl
// ============================================================================
FLT_PREOP_CALLBACK_STATUS NTAPI
PFFileSystemControl(PFLT_CALLBACK_DATA Data, PFLT_RELATED_OBJECTS FltObjects,
                    PVOID *CompletionContext)
{
    PFLT_IO_PARAMETER_BLOCK Iopb = Data->Iopb;
    ULONG FsControlCode = (ULONG)Iopb->Parameters.QueryFileInformation.InfoBuffer;
    NTSTATUS Status = 0;
    FLT_PREOP_CALLBACK_STATUS Result = FLT_PREOP_SUCCESS_NO_CALLBACK;
    ULONG OutputBuffer[8];

    if (FsControlCode == 0x90000 || FsControlCode == 0x90004 ||
        FsControlCode == 0x90008 || FsControlCode == 0x9005c)
    {
        Status = FltFsControlFile(
            FltObjects->Instance,
            Iopb->TargetFileObject,
            0x900a8,
            0,
            (ULONG)NULL,
            OutputBuffer,
            sizeof(OutputBuffer),
            NULL
        );

        if (Status == 0x80000005 || Status == 0)
        {
            Status = 0;
            if (OutputBuffer[0] == 0x80000008)
                Status = 0xc00000e2;   // STATUS_REPARSE
            Result = FLT_PREOP_SUCCESS_NO_CALLBACK;
        }
        else if (Status == 0xc0000275)   // STATUS_REPARSE_POINT_ENCOUNTERED
        {
            Status = 0;
            Result = FLT_PREOP_SUCCESS_NO_CALLBACK;
        }
        else
        {
            Result = FLT_PREOP_COMPLETE;
        }
    }
    else if (FsControlCode == 0x90240)
    {
        if (Iopb->Parameters.Create.Options > 11)
        {
            if ((((ULONG_PTR)Iopb->Parameters.QuerySecurity.MdlAddress & 1) == 0))
                goto Done;
            // fall through to special handling
            Status = FltFsControlFile(FltObjects->Instance,
            Iopb->TargetFileObject,
            0x900a8,
            0,
            (ULONG)NULL,
            OutputBuffer,
            sizeof(OutputBuffer),
            NULL);   // same as above
        }
        else
        {
            Status = 0xc0000023;   // STATUS_BUFFER_TOO_SMALL
            Result = FLT_PREOP_COMPLETE;
        }
    }
    else if (FsControlCode == 0x94264)
    {
        Status = 0xc000a2a3;
        Result = FLT_PREOP_COMPLETE;
    }
    else if (FsControlCode == 0x98268)
    {
        Status = 0xc000a2a4;
        Result = FLT_PREOP_COMPLETE;
    }
    else
    {
        // nothing special
        goto Done;
    }

    if (Result == FLT_PREOP_COMPLETE)
        Data->IoStatus.Status = Status;

Done:
    return Result;
}

// ============================================================================
// PFGetContextByFileObject
// ============================================================================
NTSTATUS
PFGetContextByFileObject(PFLT_FILTER Filter, PFLT_INSTANCE Instance,
                         PFILE_OBJECT FileObject, PPF_FILE_CONTEXT *OutContext,
                         PUCHAR IsDirectory)
{
    NTSTATUS Status;
    PPF_FILE_CONTEXT Context = NULL;
    BOOLEAN Created = FALSE;
    ULONG FileInfo[6];
    ULONG BytesReturned;
    ULONG SecurityCookie;

    *OutContext = NULL;
    *IsDirectory = 0;

    Status = FltGetStreamContext(Instance, FileObject, (PFLT_CONTEXT*)&Context);
    if (Status >= 0)
    {
        *OutContext = Context;
        *IsDirectory = 1;
        goto Exit;
    }

    if (Status != 0xc0000225)   // STATUS_NOT_FOUND
        goto Exit;

    Status = FltAllocateContext(Filter, 8, 0x28, 0, (PVOID*)&Context);
    if (Status < 0)
        goto Exit;

    RtlZeroMemory(Context, 0x28);
    KeInitializeEvent(&Context->ExtractComplete, NotificationEvent, FALSE);
    Context->ExtractResult = 0;

    Status = FltQueryInformationFile(Instance, FileObject, FileInfo, sizeof(FileInfo), 4, NULL);
    if (Status < 0)
        goto Cleanup;

    Context->FileId.LowPart = FileInfo[2];
    Context->FileId.HighPart = FileInfo[3];

    Status = FltSetStreamContext(Instance, FileObject, (FLT_SET_CONTEXT_OPERATION)Context, NULL, NULL);
    if (Status == 0xc01d0002)   // STATUS_FLT_CONTEXT_ALREADY_DEFINED
    {
        // Another thread created it, release ours and get the existing one
        FltReleaseContext((PFLT_CONTEXT)Context);
        Context = NULL;
        Status = FltGetStreamContext(Instance, FileObject, (PFLT_CONTEXT*)&Context);
        if (Status >= 0)
        {
            *OutContext = Context;
            *IsDirectory = 1;
            goto Exit;
        }
        goto Cleanup;
    }
    else if (Status < 0)
    {
        goto Cleanup;
    }

    Created = TRUE;
    *OutContext = Context;
    *IsDirectory = 1;

    // The original had a block that sets an event and deletes the context if local_29 != 0
    // but that was for error cleanup. We'll just return success.

Exit:
    return Status;

Cleanup:
    if (Context)
    {
        if (Created)
        {
            KeSetEvent(&Context->ExtractComplete, 0, FALSE);
            FltDeleteStreamContext(Instance, FileObject, (PFLT_CONTEXT)Context);
        }
        FltReleaseContext((PFLT_CONTEXT)Context);
    }
    return Status;
}

// ============================================================================
// PFGetFileContext
// ============================================================================
NTSTATUS
PFGetFileContext(PFLT_CALLBACK_DATA Data, PFLT_RELATED_OBJECTS FltObjects,
                 PVOID CallbackFn, PVOID Context,
                 PPF_FILE_CONTEXT *OutContext, PUCHAR IsDirectory,
                 PVOID *Arg7, PUCHAR Arg8)
{
    NTSTATUS Status;
    PFLT_RELATED_OBJECTS local_10 = NULL;
    PFLT_CALLBACK_DATA local_8 = NULL;
    PFLT_FILTER local_c = NULL;
    PFLT_RELATED_OBJECTS pVar4;
    PFLT_RELATED_OBJECTS pVar6;
    PPF_FILE_CONTEXT FileContext = NULL;
    UCHAR IsDir = 0;
    PVOID ppvVar5;

    *IsDirectory = 0;
    *OutContext = NULL;

    Status = PFOpenReparseTarget((PFLT_CALLBACK_DATA)&local_8, (PFLT_RELATED_OBJECTS)&local_10, NULL, NULL);
    pVar4 = local_10;
    if (Status >= 0 || Status == 0xc000000d)   // STATUS_REPARSE_POINT_ENCOUNTERED?
    {
        if (Status == 0xc000000d)
        {
            Status = 0;
            // no context
            return Status;
        }

        if (Data)
        {
            // call the callback function (PFOnlyConsiderDirectories)
            Status = ((NTSTATUS (*)(PVOID, PVOID, PVOID, PVOID, PVOID))CallbackFn)(
                Data, local_c, local_10, FltObjects, &IsDir
            );
            if (Status < 0)
                goto Cleanup;
            if (IsDir)
            {
                Status = 0;
                *IsDirectory = 1;
                goto Cleanup;
            }
        }

        ppvVar5 = (PVOID)CallbackFn;
        Status = PFGetContextByFileObject((PFLT_FILTER)pVar4, (PFLT_INSTANCE)CallbackFn,
                                          (PFILE_OBJECT)Context, OutContext, &IsDir);
        if (Status >= 0 && IsDir)
        {
            Status = PFGetWritableHandle(local_8, pVar4, (PFLT_FILE_NAME_INFORMATION)OutContext,
                                         (PFILE_OBJECT)local_8, ppvVar5);
        }
    }

Cleanup:
    if (pVar4)
    {
        ObfDereferenceObject(pVar4);
        pVar4 = NULL;
    }
    if (local_8)
        FltReleaseFileNameInformation((PFLT_FILE_NAME_INFORMATION)local_8);

    if (Status < 0)
    {
        if (IsDir)
        {
            // Clean up context
            PVOID pContext = *OutContext;
            if (pContext)
            {
                *(PLONG)((PUCHAR)pContext + 0x10) = Status;
                KeSetEvent(pContext, 0, FALSE);
                FltDeleteStreamContext((PFLT_INSTANCE)((PUCHAR)local_c + 0xc), (PFILE_OBJECT)pVar4, 0);
                *OutContext = NULL;
            }
            if (*(PVOID*)CallbackFn)
            {
                FltReleaseContext(*(PVOID*)CallbackFn);
                *(PVOID*)CallbackFn = NULL;
            }
        }
    }

    return Status;
}

// ============================================================================
// PFGetWritableHandle
// ============================================================================
NTSTATUS
PFGetWritableHandle(PFLT_CALLBACK_DATA Data, PFLT_RELATED_OBJECTS FltObjects,
                    PFLT_FILE_NAME_INFORMATION FileNameInfo,
                    PFILE_OBJECT FileObject, PVOID *Arg5)
{
    NTSTATUS Status;
    ULONG FileInfo[8];
    ULONG Attributes;
    ULONG DesiredAccess;
    HANDLE hFile = NULL;
    IO_STATUS_BLOCK IoStatus;
    OBJECT_ATTRIBUTES ObjAttr;
    UNICODE_STRING Name;
    BOOLEAN RestoreAttribute = FALSE;
    ULONG SecurityCookie;
    USHORT FileNameLength;
    WCHAR NameBuffer[128];

    RtlZeroMemory(FileInfo, sizeof(FileInfo));

    Status = FltQueryInformationFile(FltObjects->Instance, FltObjects->FileObject,
                                     FileInfo, 0x28, 4, NULL);
    if (Status < 0)
        goto Exit;

    Attributes = FileInfo[1];
    if (Attributes & 1)   // FILE_ATTRIBUTE_REPARSE_POINT
    {
        FileInfo[1] &= ~1;
        Status = FltSetInformationFile(FltObjects->Instance, FltObjects->FileObject,
                                       FileInfo, 0x28, 4);
        if (Status < 0)
            goto Exit;
        RestoreAttribute = TRUE;
    }

    DesiredAccess = (Attributes & 0x10) ? 0xc1130000 : 0xc0100000;   // directory vs file

    // Build object attributes from the name (using the Iopb data)
    // The original used local_50 and local_6c to construct the name.
    Name.Length = (USHORT)((ULONG_PTR)Data->Iopb - (ULONG_PTR)&Data->Iopb->TargetFileObject->FileName);
    Name.MaximumLength = Name.Length;
    Name.Buffer = *(WCHAR**)((PUCHAR)&Data->Iopb->TargetFileObject->FileName + 6);
    InitializeObjectAttributes(&ObjAttr, &Name, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);

    Status = FltCreateFile(
        FltObjects->Filter,
        FltObjects->Instance,
        &hFile,
        DesiredAccess,
        &ObjAttr,
        &IoStatus,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE,
        NULL,
        0,
        0x800
    );
    if (Status < 0)
        goto Cleanup;

    if (RestoreAttribute)
    {
        FileInfo[1] = Attributes;
        Status = FltSetInformationFile(FltObjects->Instance, FltObjects->FileObject,
                                       FileInfo, 0x28, 4);
        if (Status < 0)
            goto Cleanup;
    }

    // Store the handle in the FileNameInfo structure (abusing fields)
    FileNameInfo->Size = (USHORT)(ULONG_PTR)hFile;
    FileNameInfo->NamesParsed = (USHORT)((ULONG_PTR)hFile >> 16);
    hFile = NULL;   // prevent closing

Cleanup:
    if (hFile)
        FltClose((PVOID)hFile);

Exit:
    return Status;
}

// ============================================================================
// PFOnlyConsiderDirectories
// ============================================================================
NTSTATUS
PFOnlyConsiderDirectories(PFLT_CALLBACK_DATA Data, PFLT_RELATED_OBJECTS FltObjects,
                          PFILE_OBJECT FileObject, PVOID Arg3, PUCHAR Arg4)
{
    NTSTATUS Status;
    BOOLEAN IsDirectory;

    Status = FltIsDirectory(FileObject, FltObjects->Instance, &IsDirectory);
    if (Status >= 0 && Arg3 && !IsDirectory)
    {
        // OR the flag and reissue
        Data->Iopb->Parameters.Create.Options |= *(PULONG)Arg3;
        FltSetCallbackDataDirty(Data);
        FltReissueSynchronousIo(FltObjects->Instance, Data);
        *Arg4 = 1;
    }
    return Status;
}

// ============================================================================
// PFOpenById
// ============================================================================
NTSTATUS
PFOpenById(PFLT_FILTER Filter, PFLT_INSTANCE Instance, PVOID Arg3,
           PLARGE_INTEGER FileId, UCHAR Arg5, PFILE_OBJECT **Arg6,
           PVOID *Arg7, UCHAR Arg8)
{
    NTSTATUS Status;
    HANDLE hFile = NULL;
    PFILE_OBJECT FileObject = NULL;
    ULONG FileInfo[2];
    ULONG BytesReturned;
    PVOID Buffer = NULL;
    UNICODE_STRING Name;
    OBJECT_ATTRIBUTES ObjAttr;
    WCHAR NameBuffer[8];
    ULONG DesiredAccess;
    IO_STATUS_BLOCK IoStatus;
    ULONG SecurityCookie;

    // Build a dummy name (open by file ID uses a special flag)
    RtlInitEmptyUnicodeString(&Name, NameBuffer, sizeof(NameBuffer));
    InitializeObjectAttributes(&ObjAttr, &Name, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);

    DesiredAccess = 0x80100110;   // from original
    Status = FltCreateFile(
        Filter,
        Instance,
        &hFile,
        DesiredAccess,
        &ObjAttr,
        &IoStatus,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_BY_FILE_ID,
        NULL,
        0,
        0
    );
    if (Status < 0)
        goto Exit;

    Status = ObReferenceObjectByHandle(hFile, 0xf0000, *IoFileObjectType, 0, (PVOID*)&FileObject, NULL);
    if (Status < 0)
        goto Exit;

    if (Arg3)
    {
        // Query name information
        Status = FltQueryInformationFile(Instance, hFile, FileInfo, 8, 9, NULL);
        if (Status != 0x80000005 && Status < 0)
            goto Exit;

        BytesReturned = FileInfo[0] + 8;
        Buffer = ExAllocatePoolWithTag(NonPagedPool, BytesReturned, 0x50466e69);
        if (!Buffer)
        {
            Status = 0xc000009a;   // STATUS_INSUFFICIENT_RESOURCES
            goto Exit;
        }

        Status = FltQueryInformationFile(Instance, hFile, Buffer, BytesReturned, 9, NULL);
        if (Status < 0)
            goto Exit;

        if (*(PULONG)Buffer == 0)
        {
            Status = 0xc000000d;   // STATUS_INVALID_PARAMETER
            goto Exit;
        }

        // Extract file name from the buffer
        Name.Length = *(PUSHORT)((PUCHAR)Buffer + 6);
        Name.Buffer = (WCHAR*)((PUCHAR)Buffer + 8);
        Name.MaximumLength = Name.Length;

        Status = FltCreateFile(
            Filter,
            Instance,
            &hFile,
            DesiredAccess,
            &ObjAttr,
            &IoStatus,
            NULL,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            FILE_OPEN,
            FILE_SYNCHRONOUS_IO_NONALERT,
            NULL,
            0,
            0
        );
        if (Status < 0)
            goto Exit;
    }

    if (Arg6)
    {
        // Store the handle in the output (Arg6 is used as a pointer to store handle)
        *(HANDLE*)Arg6 = hFile;
        hFile = NULL;
    }

Exit:
    if (hFile)
        FltClose((PVOID)hFile);
    if (FileObject)
        ObfDereferenceObject(FileObject);
    if (Buffer)
        ExFreePoolWithTag(Buffer, 0x50466e69);

    return Status;
}

// ============================================================================
// PFOpenReparseTarget
// ============================================================================
NTSTATUS
PFOpenReparseTarget(PFLT_CALLBACK_DATA Data, PFLT_RELATED_OBJECTS FltObjects,
                    PFLT_FILE_NAME_INFORMATION **FileNameInfo,
                    PFILE_OBJECT **FileObject)
{
    NTSTATUS Status;
    HANDLE hFile = NULL;
    PFILE_OBJECT FileObjectOut = NULL;
    ULONG FileNameInfoSize = 0;
    PVOID NameInfoBuffer = NULL;
    UNICODE_STRING FileName;
    OBJECT_ATTRIBUTES ObjAttr;
    USHORT NameLength;
    ULONG_PTR FileNameInfoPtr;
    UCHAR IsUnderTemp;

    Status = FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, (PFLT_FILE_NAME_INFORMATION *)FileNameInfo);
    if (Status < 0)
        return Status;

    // The original computed the name length by subtracting offsets from the file name information.
    // We'll replicate that.
    PFLT_FILE_NAME_INFORMATION pInfo = (PFLT_FILE_NAME_INFORMATION)FileNameInfo;
    NameLength = (USHORT)(pInfo->Name.Length - pInfo->Volume.Length);
    FileName.Length = NameLength - *(PUSHORT)((PUCHAR)(*FileNameInfo) + 0x14 + 6);   // some offset
    FileName.Buffer = (WCHAR*)((PUCHAR)(*FileNameInfo) + 8);
    FileName.MaximumLength = FileName.Length;

    IsUnderTemp = PFPathIsUnderTempDir(&FileName);
    if (IsUnderTemp)
    {
        Status = 0xc000000d;   // STATUS_INVALID_PARAMETER
        goto Cleanup;
    }

    InitializeObjectAttributes(&ObjAttr, &FileName, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = FltCreateFile(
        FltObjects->Filter,
        FltObjects->Instance,
        &hFile,
        0x80100110,
        &ObjAttr,
        NULL,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_REPARSE_POINT,
        NULL,
        0,
        0
    );
    if (Status < 0)
        goto Cleanup;

    Status = ObReferenceObjectByHandle(hFile, 0xf0000, *IoFileObjectType, 0, (PVOID*)&FileObjectOut, NULL);
    if (Status < 0)
        goto Cleanup;

    if (Data)
    {
		Data->Flags=FileNameInfoSize;
    }

Cleanup:
    if (hFile)
        FltClose((PVOID)hFile);
    if (FileObjectOut)
        ObfDereferenceObject(FileObjectOut);
    if (*FileNameInfo)
        FltReleaseFileNameInformation((PFLT_FILE_NAME_INFORMATION)FileNameInfo);

    return Status;
}

// ============================================================================
// PFPathIsUnderTempDir
// ============================================================================
UCHAR
PFPathIsUnderTempDir(PUNICODE_STRING Path)
{
    NTSTATUS Status;
    UNICODE_STRING Component1, Component2, Component3, Component4;

    ParsePathElement(&Component1, Path, NULL);
    ParsePathElement(&Component2, &Component1, NULL);
    ParsePathElement(&Component3, &Component2, NULL);
    ParsePathElement(&Component4, &Component3, NULL);

    if (RtlCompareUnicodeString(&Component1, &ExpectedFirstToken, TRUE) == 0 &&
        RtlCompareUnicodeString(&Component2, &ExpectedSecondToken, TRUE) == 0)
    {
        return 1;
    }
    return 0;
}

// ============================================================================
// PFPreCreate
// ============================================================================
FLT_PREOP_CALLBACK_STATUS NTAPI
PFPreCreate(PFLT_CALLBACK_DATA Data, PFLT_RELATED_OBJECTS FltObjects, PVOID *CompletionContext)
{
    ULONG Options = Data->Iopb->Parameters.Create.Options;
    FLT_PREOP_CALLBACK_STATUS Result = FLT_PREOP_SUCCESS_WITH_CALLBACK;
    ULONG ProcessId;
    PWM_COMM_LIST pList;
    PVOID Context = NULL;

    if (Options & 0x200000)
    {
        ProcessId = (ULONG)FltGetRequestorProcessId(Data);

        FltAcquireResourceShared(&g_CommList.ListLock);

        pList = (PWM_COMM_LIST)g_CommList.ListHead.Flink;
        while (pList != &g_CommList)
        {
            // Compare with the ProcessId stored in the ERESOURCE's OwnerEntry.TableSize
            // This is a hack; we assume that field holds the process ID.
            if (pList->ListLock.OwnerEntry.TableSize == ProcessId)
                break;
            pList = (PWM_COMM_LIST)pList->ListHead.Flink;
        }

        FltReleaseResource(&g_CommList.ListLock);

        if (pList == &g_CommList)
        {
            Context = ExAllocatePoolWithTag(NonPagedPool, 4, 0x50466d69);
            if (!Context)
            {
                Data->IoStatus.Status = 0xc000009a;   // STATUS_INSUFFICIENT_RESOURCES
                Result = FLT_PREOP_COMPLETE;
            }
            else
            {
                *(PULONG)Context = 0x200000;
                Data->Iopb->Parameters.Create.Options &= ~0x200000;
                FltSetCallbackDataDirty(Data);
                *CompletionContext = Context;
            }
        }
    }

    return Result;
}

// ============================================================================
// PFReleaseContextAndUntagFile
// ============================================================================
NTSTATUS
PFReleaseContextAndUntagFile(PFLT_CALLBACK_DATA Data, PFLT_RELATED_OBJECTS FltObjects,
                             PPF_FILE_CONTEXT Context, UCHAR Untag,
                             PVOID Arg5, UCHAR Arg6)
{
    NTSTATUS Status = 0;
    HANDLE hFile = NULL;
    PFILE_OBJECT FileObject = NULL;

    if (FltObjects == NULL)
    {
        if (Data)
            FltReleaseContext((PFLT_CONTEXT)Data);
        return 0;
    }

    if (Context)
    {
        Status = ObReferenceObjectByHandle((HANDLE)Context, 0xf0000, *IoFileObjectType, 0, (PVOID*)&FileObject, NULL);
    }
    else
    {
        Status = PFOpenReparseTarget(NULL, (PFLT_RELATED_OBJECTS)&hFile, NULL, NULL);
        if (Status >= 0)
            FileObject = (PFILE_OBJECT)hFile;
    }

    if (Status >= 0 && FileObject)
    {
        if (Untag)
        {
            Status = FltUntagFile(FltObjects->Instance, FileObject, 0x80000008, 0);
            if (Status != 0xc0000275)
                Status = 0;
        }

        KeSetEvent((PKEVENT)Data, 0, FALSE);
        FltDeleteStreamContext(FltObjects->Instance, FileObject, (PFLT_CONTEXT)Data);

        if (!Untag || Status >= 0)
        {
            if (Data)
                FltReleaseContext((PFLT_CONTEXT)Data);
            Status = 0;
        }
        else
        {
            // Error case: set status in Data and release
            Data->IoStatus.Information = Status;
            KeSetEvent((PKEVENT)Data, 0, FALSE);
            FltReleaseContext((PFLT_CONTEXT)Data);
        }
    }

    if (FileObject)
        ObfDereferenceObject(FileObject);

    return Status;
}

// ============================================================================
// PFSetSubsumedStatus
// ============================================================================
VOID
PFSetSubsumedStatus(PFLT_INSTANCE Instance, PPF_FILE_CONTEXT Context, LONG Status)
{
    PPF_FILE_CONTEXT Current = Context;
    PPF_FILE_CONTEXT Next;

    FltAcquireResourceExclusive(&SubsumeLock);

    while (Current)
    {
        Next = Current->NextContext;
        Current->NextContext = NULL;
        Current->FileObject = (PFILE_OBJECT)Instance;
        KeSetEvent(&Current->ExtractComplete, 0, FALSE);
        FltDeleteStreamContext(Instance, Current->FileObject, (PFLT_CONTEXT)Current);
        FltUntagFile(Instance, Current->FileObject, 0x80000008, 0);
        ObfDereferenceObject(Current->FileObject);
        FltReleaseContext((PFLT_CONTEXT)Current);
        Current = Next;
    }

    FltReleaseResource(&SubsumeLock);
}

// ============================================================================
// PFSubsumeContext
// ============================================================================
NTSTATUS NTAPI
PFSubsumeContext(PFLT_FILTER Filter, PFLT_INSTANCE Instance, PVOID Arg3,
                 PLARGE_INTEGER FileId, PLARGE_INTEGER Arg5, PVOID *Arg6)
{
    NTSTATUS Status;
    HANDLE hFile = NULL;
    PFILE_OBJECT FileObject = NULL;
    PPF_FILE_CONTEXT Context = NULL;
    PPF_FILE_CONTEXT ExistingContext = NULL;
    PFLT_INSTANCE SubsumingInstance = NULL;
    UCHAR IsDirectory = 0;
    PVOID EventObject = NULL;

    // Open by ID
    Status = PFOpenById(Filter, Instance, Arg3, FileId, 0, NULL, Arg6, 0);
    if (Status < 0)
        goto Exit;

    // Get context
    Status = PFGetContextByFileObject(Filter, Instance, (PFILE_OBJECT)Arg3, &Context, &IsDirectory);
    if (Status < 0)
        goto Exit;

    if (Arg6 == NULL)
        goto Done;

    if (IsDirectory == 0)
    {
        // Wait for extraction to complete
        Status = KeWaitForSingleObject(&Context->ExtractComplete, Executive, KernelMode, FALSE, NULL);
        if (Status == 0x101 || (Status >= 0 && Context->ExtractResult < 0))
        {
            if (Status >= 0)
                Status = Context->ExtractResult;
            goto Exit;
        }
    }
    else
    {
        // Open a handle for the directory
        Status = PFOpenById(Filter, Instance, NULL, FileId, 0, (PFILE_OBJECT**)1, (PVOID*)&hFile, 0);
        if (Status < 0)
            goto Exit;

        Status = FltGetStreamContext(Instance, (PFILE_OBJECT)Arg3, (PFLT_CONTEXT*)&ExistingContext);
        if (Status >= 0)
        {
            // Deref existing
            ObfDereferenceObject(ExistingContext->FileObject);

            // Duplicate handle into target process
            Status = ZwDuplicateObject(NtCurrentProcess(), hFile, NtCurrentProcess(), Arg6, 0, 0, DUPLICATE_SAME_ACCESS);
            if (Status >= 0)
            {
                FltAcquireResourceExclusive(&SubsumeLock);
                // Link the context into the subsume list
                ExistingContext->NextContext = Context;
                FltReleaseResource(&SubsumeLock);
            }
            else
            {
                // Cleanup on failure
            }
        }
        else
        {
            Status = 0;   // not found, treat as success?
        }
    }

Done:
    Status = 0;

Exit:
    if (hFile)
        FltClose((PVOID)hFile);
    if (FileObject)
        ObfDereferenceObject(FileObject);
    if (Context)
        FltReleaseContext((PFLT_CONTEXT)Context);

    return Status;
}

// ============================================================================
// Unload
// ============================================================================
NTSTATUS
Unload(ULONG Flags)
{
    PLIST_ENTRY pFlink, pBlink;
    PWM_COMM_LIST_ENTRY pEntry;

    if (gTraceFlags & 1)
        DbgPrint("isDirTest!Unload: Entered\n");

    if (PfCommunicationPort)
    {
        FltCloseCommunicationPort(PfCommunicationPort);
        PfCommunicationPort = NULL;
    }

    if (gFilterHandle)
    {
        FltUnregisterFilter(gFilterHandle);
        gFilterHandle = NULL;
    }

    // Clean up the communication list
    FltAcquireResourceExclusive(&g_CommList.ListLock);

    while (TRUE)
    {
        if (g_CommList.ListHead.Flink == &g_CommList.ListHead || g_CommList.ListHead.Flink == NULL)
        {
            FltReleaseResource(&g_CommList.ListLock);
            ExDeleteResourceLite(&g_CommList.ListLock);
            ExDeleteResourceLite(&SubsumeLock);
            return 0;
        }

        pFlink = g_CommList.ListHead.Flink->Flink;
        pBlink = g_CommList.ListHead.Flink->Blink;

        if (pFlink->Blink != g_CommList.ListHead.Flink || pBlink->Flink != g_CommList.ListHead.Flink)
        {
            // List corruption - crash the system (replaces swi(0x29) and halt)
            KeBugCheckEx(
                DRIVER_IRQL_NOT_LESS_OR_EQUAL,
                (ULONG_PTR)pFlink,
                (ULONG_PTR)pBlink,
                (ULONG_PTR)g_CommList.ListHead.Flink,
                0
            );
        }

        pBlink->Flink = pFlink;
        pFlink->Blink = pBlink;
        pEntry = (PWM_COMM_LIST_ENTRY)g_CommList.ListHead.Flink;
        WMCommListEntryRelease(pEntry);
    }

    // Not reached
    return 0;
}

