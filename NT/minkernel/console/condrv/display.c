#pragma warning (disable:4996)
#include "condrvp.h"

/* ------------------------------------------------------------------ */
/* External functions defined in other source files                   */
/* ------------------------------------------------------------------ */
NTSTATUS BgkGetConsoleState(PVOID State);
extern BOOLEAN SePrivilegeCheck(PLUID_AND_ATTRIBUTES RequiredPrivileges, PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext, KPROCESSOR_MODE AccessMode);
NTSTATUS BgkSetCursor(ULONG Column, ULONG Row, ULONG Height);
VOID     BgkDisplayCharacter(WCHAR Character, ULONG Column, ULONG Row,
                             ULONG Foreground, ULONG Background);

/* ------------------------------------------------------------------ */
/* Forward declarations                                               */
/* ------------------------------------------------------------------ */
NTSTATUS __cdecl CdCreateDisplayObject(PIRP  Irp,
                                       ULONG Parameter);
NTSTATUS __fastcall CdpGetDisplaySize(UCHAR                 Mode,
                                      PCD_IO_DISPLAY_SIZE   OutputBuffer,
                                      ULONG                 Length,
                                      PULONG                RequiredSize);
UCHAR __cdecl CdpDisplayFastIoctl(PVOID             Context,
                                  PVOID             InputBuffer,
                                  ULONG             InputLength,
                                  PVOID             OutputBuffer,
                                  ULONG             OutputLength,
                                  ULONG             IoctlCode,
                                  PIO_STATUS_BLOCK  IoStatus);
NTSTATUS __fastcall CdpSetCursorInformation(UCHAR                      Mode,
                                             PCD_IO_CURSOR_INFORMATION CursorInfo,
                                             ULONG                      Length);
NTSTATUS __cdecl CdpDisplayClose(PCD_DISPLAY Display,
                                 PIRP        Irp);
NTSTATUS __fastcall CdpUpdateDisplay(UCHAR                   Mode,
                                      PCD_IO_ROW_INFORMATION  RowInfo,
                                      ULONG                   Length);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,CdCreateDisplayObject)
#pragma alloc_text(PAGE,CdpDisplayClose)
#pragma alloc_text(PAGE,CdpDisplayFastIoctl)
#pragma alloc_text(PAGE,CdpGetDisplaySize)
#pragma alloc_text(PAGE,CdpSetCursorInformation)
#pragma alloc_text(PAGE,CdpUpdateDisplay)
#endif // ALLOC_PRAGMA

/* ------------------------------------------------------------------ */
/* External symbols defined in other source files                     */
/* ------------------------------------------------------------------ */
extern PDEVICE_OBJECT  CdDeviceObject;
CD_CONTEXT_TYPE CdDisplayType = {0,NULL,NULL,NULL,NULL,CdpDisplayClose,CdpDisplayFastIoctl,NULL,NULL,NULL};
RTL_RUN_ONCE    CdpDisplayInitialized = { 0 };
BGFX_CONSOLE_STATE CdpDisplayState = { 0 };
ULONG CdpPalette[16] = { 0x000000, 0x000080, 0x008000, 0x008080, 0x800000, 0x800080, 0x808000, 0xC0C0C0, 0x808080, 0x0000FF, 0x00FF00, 0x00FFFF, 0xFF0000, 0xFF00FF, 0xFFFF00, 0xFFFFFF };

/* ------------------------------------------------------------------ */
/* CdCreateDisplayObject                                              */
/* ------------------------------------------------------------------ */
NTSTATUS __cdecl CdCreateDisplayObject(PIRP  Irp,
                                       ULONG Parameter)
{
    PIO_STACK_LOCATION   stackLocation;
    PIO_SECURITY_CONTEXT securityContext;
    PACCESS_STATE        accessState;
    LUID_AND_ATTRIBUTES  requiredPrivilege;
    PCD_DISPLAY          display;
    NTSTATUS             status;

    stackLocation = IoGetCurrentIrpStackLocation(Irp);

    if (PsIsCurrentThreadInServerSilo()) {
        status = STATUS_NOT_SUPPORTED;
        goto Complete;
    }

    requiredPrivilege.Luid.LowPart = 1;

    securityContext = stackLocation->Parameters.Create.SecurityContext;
    accessState     = securityContext->AccessState;

    if (!SePrivilegeCheck(&requiredPrivilege,
                          (PSECURITY_SUBJECT_CONTEXT)((PUCHAR)accessState + 0x1c),
                          Irp->RequestorMode)) {
        status = STATUS_PRIVILEGE_NOT_HELD;
        goto Complete;
    }

    KeEnterCriticalRegion();
    status = RtlRunOnceBeginInitialize(&CdpDisplayInitialized, 0, NULL);
    if (status == STATUS_PENDING) {
        status = BgkGetConsoleState(&CdpDisplayState);
        if (!NT_SUCCESS(status)) {
            RtlRunOnceComplete(&CdpDisplayInitialized, RTL_RUN_ONCE_INIT_FAILED, NULL);
            KeLeaveCriticalRegion();
            goto Complete;
        }
        status = RtlRunOnceComplete(&CdpDisplayInitialized, 0, NULL);
    }
    KeLeaveCriticalRegion();

    if (!NT_SUCCESS(status)) {
        goto Complete;
    }

    display = ExAllocatePoolWithQuotaTag(
        (POOL_TYPE)(PagedPool | POOL_QUOTA_FAIL_INSTEAD_OF_RAISE),
        sizeof(CD_DISPLAY),
        'iCdD');

    if (display == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Complete;
    }

    display->Header.ObjectType            = &CdDisplayType;
    stackLocation->FileObject->FsContext  = display;

    Irp->IoStatus.Status      = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IofCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;

Complete:
    Irp->IoStatus.Status      = status;
    Irp->IoStatus.Information = 0;
    IofCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

/* ------------------------------------------------------------------ */
/* CdpDisplayClose                                                    */
/* ------------------------------------------------------------------ */
NTSTATUS __cdecl CdpDisplayClose(PCD_DISPLAY Display,
                                 PIRP        Irp)
{
    ExFreePoolWithTag(Display, 0);

    Irp->IoStatus.Status      = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IofCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

/* ------------------------------------------------------------------ */
/* CdpDisplayFastIoctl                                                */
/* ------------------------------------------------------------------ */
UCHAR __cdecl CdpDisplayFastIoctl(PVOID             Context,
                                  PVOID             InputBuffer,
                                  ULONG             InputLength,
                                  PVOID             OutputBuffer,
                                  ULONG             OutputLength,
                                  ULONG             IoctlCode,
                                  PIO_STATUS_BLOCK  IoStatus)
{
    KPROCESSOR_MODE mode;
    NTSTATUS        status;

    IoStatus->Information = 0;

    switch (IoctlCode) {
    case 0x500027:  /* IOCTL_CONDRV_GET_DISPLAY_SIZE */
        mode   = ExGetPreviousMode();
        status = CdpGetDisplaySize((UCHAR)mode,
                                   (PCD_IO_DISPLAY_SIZE)OutputBuffer,
                                   OutputLength,
                                   &IoStatus->Information);
        break;

    case 0x50002b:  /* IOCTL_CONDRV_UPDATE_DISPLAY */
        mode   = ExGetPreviousMode();
        status = CdpUpdateDisplay((UCHAR)mode,
                                  (PCD_IO_ROW_INFORMATION)InputBuffer,
                                  InputLength);
        break;

    case 0x50002f:  /* IOCTL_CONDRV_SET_CURSOR */
        mode   = ExGetPreviousMode();
        status = CdpSetCursorInformation((UCHAR)mode,
                                         (PCD_IO_CURSOR_INFORMATION)InputBuffer,
                                         InputLength);
        break;

    default:
        IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
        return TRUE;
    }

    IoStatus->Status = status;
    return TRUE;
}

/* ------------------------------------------------------------------ */
/* CdpGetDisplaySize                                                  */
/* ------------------------------------------------------------------ */
NTSTATUS __fastcall CdpGetDisplaySize(UCHAR                 Mode,
                                      PCD_IO_DISPLAY_SIZE   OutputBuffer,
                                      ULONG                 Length,
                                      PULONG                RequiredSize)
{
    if (Length < sizeof(CD_IO_DISPLAY_SIZE)) {
        return STATUS_INVALID_BUFFER_SIZE;
    }

    __try {
        if (Mode != KernelMode) {
            ProbeForWrite(OutputBuffer, sizeof(CD_IO_DISPLAY_SIZE),
                          sizeof(ULONG));
        }

        OutputBuffer->Width  = CdpDisplayState.Columns;
        OutputBuffer->Height = CdpDisplayState.Rows;

        *RequiredSize = sizeof(CD_IO_DISPLAY_SIZE);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return STATUS_ACCESS_VIOLATION;
    }

    return STATUS_SUCCESS;
}

/* ------------------------------------------------------------------ */
/* CdpSetCursorInformation                                            */
/* ------------------------------------------------------------------ */
NTSTATUS __fastcall CdpSetCursorInformation(UCHAR                      Mode,
                                             PCD_IO_CURSOR_INFORMATION CursorInfo,
                                             ULONG                      Length)
{
    ULONG packed;
    ULONG height;
    ULONG visible;

    if (Length < sizeof(CD_IO_CURSOR_INFORMATION)) {
        return STATUS_INVALID_BUFFER_SIZE;
    }

    __try {
        if (Mode != KernelMode) {
            ProbeForRead(CursorInfo, Length, sizeof(ULONG));
        }

        packed  = *(PULONG)CursorInfo;
        height  = CursorInfo->Height;
        visible = CursorInfo->IsVisible;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return STATUS_ACCESS_VIOLATION;
    }

    return BgkSetCursor(packed & 0xffff,
                        packed >> 16,
                        visible ? height : 0);
}

/* ------------------------------------------------------------------ */
/* CdpUpdateDisplay                                                   */
/* ------------------------------------------------------------------ */
NTSTATUS __fastcall CdpUpdateDisplay(UCHAR                   Mode,
                                      PCD_IO_ROW_INFORMATION  RowInfo,
                                      ULONG                   Length)
{
    ULONG            column;
    SHORT            row;
    PCD_IO_CHARACTER oldChars;
    PCD_IO_CHARACTER newChars;
    ULONG            attribute;

    if (Length < sizeof(CD_IO_ROW_INFORMATION)) {
        return STATUS_INVALID_BUFFER_SIZE;
    }

    __try {
        if (Mode != KernelMode) {
            ProbeForRead(RowInfo, Length, sizeof(ULONG));
        }

        row      = RowInfo->Index;
        oldChars = RowInfo->Old;
        newChars = RowInfo->New;

        if (Mode != KernelMode) {
            ProbeForWrite(oldChars,
                          CdpDisplayState.Columns * sizeof(CD_IO_CHARACTER),
                          sizeof(ULONG));
        }

        if (row < 0 || (ULONG)row >= CdpDisplayState.Rows) {
            return STATUS_INVALID_PARAMETER;
        }

        for (column = 0; column < CdpDisplayState.Columns; column++) {
            if (*(PULONG)&newChars[column] != *(PULONG)&oldChars[column]) {
                attribute = newChars[column].Atribute;
                BgkDisplayCharacter(newChars[column].Character,
                                    column,
                                    (ULONG)row,
                                    CdpPalette[attribute & 0x0f],
                                    CdpPalette[(attribute >> 4) & 0x0f]);
                *(PULONG)&oldChars[column] = *(PULONG)&newChars[column];
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return STATUS_ACCESS_VIOLATION;
    }

    return STATUS_SUCCESS;
}
