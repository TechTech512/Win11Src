#include <ntddk.h>
#include <ntstrsafe.h>
#ifdef PEPROCESS
#ifdef PETHREAD
#undef PEPROCESS
#undef PETHREAD
#include <ntifs.h>
#endif
#endif
#ifndef BOOL
typedef int BOOL;
#endif
#ifndef MAX_PATH
#define MAX_PATH 260
#endif
#include "ntpsapi.h"

#define RTL_MAX_DRIVE_LETTERS 32

typedef struct _CURDIR
{
    UNICODE_STRING DosPath;
    HANDLE Handle;
} CURDIR, *PCURDIR;

typedef struct _RTL_DRIVE_LETTER_CURDIR
{
    USHORT Flags;
    USHORT Length;
    ULONG TimeStamp;
    STRING DosPath;
} RTL_DRIVE_LETTER_CURDIR, *PRTL_DRIVE_LETTER_CURDIR;

typedef struct _RTL_USER_PROCESS_PARAMETERS
{
    ULONG MaximumLength;
    ULONG Length;

    ULONG Flags;
    ULONG DebugFlags;

    HANDLE ConsoleHandle;
    ULONG ConsoleFlags;
    HANDLE StandardInput;
    HANDLE StandardOutput;
    HANDLE StandardError;

    CURDIR CurrentDirectory;
    UNICODE_STRING DllPath;
    UNICODE_STRING ImagePathName;
    UNICODE_STRING CommandLine;
    PVOID Environment;

    ULONG StartingX;
    ULONG StartingY;
    ULONG CountX;
    ULONG CountY;
    ULONG CountCharsX;
    ULONG CountCharsY;
    ULONG FillAttribute;

    ULONG WindowFlags;
    ULONG ShowWindowFlags;
    UNICODE_STRING WindowTitle;
    UNICODE_STRING DesktopInfo;
    UNICODE_STRING ShellInfo;
    UNICODE_STRING RuntimeData;
    RTL_DRIVE_LETTER_CURDIR CurrentDirectories[RTL_MAX_DRIVE_LETTERS];

    ULONG_PTR EnvironmentSize;
    ULONG_PTR EnvironmentVersion;

    PVOID PackageDependencyData;
    ULONG ProcessGroupId;
    ULONG LoaderThreads;
    UNICODE_STRING RedirectionDllName; // REDSTONE4
    UNICODE_STRING HeapPartitionName; // 19H1
    PULONGLONG DefaultThreadpoolCpuSetMasks;
    ULONG DefaultThreadpoolCpuSetMaskCount;
    ULONG DefaultThreadpoolThreadMaximum;
    ULONG HeapMemoryTypeMask; // WIN11
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;

typedef enum _TOKEN_INFORMATION_CLASS {
    TokenUser = 1,
    TokenGroups,
    TokenPrivileges,
    TokenOwner,
    TokenPrimaryGroup,
    TokenDefaultDacl,
    TokenSource,
    TokenType,
    TokenImpersonationLevel,
    TokenStatistics,
    TokenRestrictedSids,
    TokenSessionId,
    TokenGroupsAndPrivileges,
    TokenSessionReference,
    TokenSandBoxInert,
    TokenAuditPolicy,
    TokenOrigin,
    TokenElevationType,
    TokenLinkedToken,
    TokenElevation,
    TokenHasRestrictions,
    TokenAccessInformation,
    TokenVirtualizationAllowed,
    TokenVirtualizationEnabled,
    TokenIntegrityLevel,
    TokenUIAccess,
    TokenMandatoryPolicy,
    TokenLogonSid,
    TokenIsAppContainer,
    TokenCapabilities,
    TokenAppContainerSid,
    TokenAppContainerNumber,
    TokenUserClaimAttributes,
    TokenDeviceClaimAttributes,
    TokenRestrictedUserClaimAttributes,
    TokenRestrictedDeviceClaimAttributes,
    TokenDeviceGroups,
    TokenRestrictedDeviceGroups,
    TokenSecurityAttributes,
    TokenIsRestricted,
    TokenProcessTrustLevel,
    TokenPrivateNameSpace,
    TokenSingletonAttributes,
    TokenBnoIsolation,
    TokenChildProcessFlags,
    TokenIsLessPrivilegedAppContainer,
    TokenIsSandboxed,
    TokenIsAppSilo,
    TokenLoggingInformation,
    MaxTokenInfoClass  // MaxTokenInfoClass should always be the last enum
} TOKEN_INFORMATION_CLASS, *PTOKEN_INFORMATION_CLASS;

typedef struct _KAPC_STATE {
    LIST_ENTRY ApcListHead[MaximumMode];
    struct _KPROCESS *Process;
    BOOLEAN KernelApcInProgress;
    BOOLEAN KernelApcPending;
    BOOLEAN UserApcPending;
} KAPC_STATE, *PKAPC_STATE, *RESTRICTED_POINTER PRKAPC_STATE;

NTKERNELAPI
VOID
KeStackAttachProcess (
    IN PRKPROCESS PROCESS,
    OUT PRKAPC_STATE ApcState
    );

NTKERNELAPI
VOID
KeUnstackDetachProcess (
    IN PRKAPC_STATE ApcState
    );

typedef LONG  (__cdecl *PFN_CD_DISPATCH)(PVOID Context, PIRP Irp);

typedef UCHAR (__cdecl *PFN_CD_FAST_IOCTL)(
    PVOID            Context,
    PVOID            InputBuffer,
    ULONG            InputLength,
    PVOID            OutputBuffer,
    ULONG            OutputLength,
    ULONG            IoctlCode,
    PIO_STATUS_BLOCK IoStatus
);

typedef struct _CD_CONTEXT_TYPE
{
    ULONG                HeaderOffset;   // 0x00
    PFN_CD_DISPATCH      Read;           // 0x04
    PFN_CD_DISPATCH      Write;          // 0x08
    PFN_CD_DISPATCH      Ioctl;          // 0x0C
    PFN_CD_DISPATCH      Cleanup;        // 0x10
    PFN_CD_DISPATCH      Close;          // 0x14
    PFN_CD_FAST_IOCTL    FastIoctl;      // 0x18
    PFN_CD_DISPATCH      FlushBuffers;   // 0x1C
    PFN_CD_DISPATCH      QuerySecurity;  // 0x20
    PFN_CD_DISPATCH      SetSecurity;    // 0x24
} CD_CONTEXT_TYPE, *PCD_CONTEXT_TYPE;

typedef struct _CD_OBJECT_CREATION_ENTRY {
    UNICODE_STRING   Name;
    PFN_CD_DISPATCH  Function;
    ULONG            Argument;
} CD_OBJECT_CREATION_ENTRY, *PCD_OBJECT_CREATION_ENTRY;

typedef struct _CD_CONTEXT_HEADER {
	PCD_CONTEXT_TYPE ObjectType;
} CD_CONTEXT_HEADER, *PCD_CONTEXT_HEADER;

typedef struct _CD_ATTACH_INFORMATION {
	void *ProcessId;
} CD_ATTACH_INFORMATION; *PCD_ATTACH_INFORMATION;

typedef struct _CD_ATTACH_INFORMATION64 {
	void *ProcessId;
} CD_ATTACH_INFORMATION64; *PCD_ATTACH_INFORMATION64;

typedef struct _CD_IO_CHARACTER {
	wchar_t Character;
	unsigned short Atribute;
} CD_IO_CHARACTER, *PCD_IO_CHARACTER;

typedef struct _CD_IO_BUFFER {
	unsigned long Size;
	void *Buffer;
} CD_IO_BUFFER, *PCD_IO_BUFFER;

typedef struct _CD_IO_BUFFER64 {
	unsigned long Size;
	void *Buffer;
} CD_IO_BUFFER64, *PCD_IO_BUFFER64;

typedef struct _CD_IO_BUFFER_DESCRIPTOR {
	void *Data;
	unsigned long Size;
	unsigned long Offset;
} CD_IO_BUFFER_DESCRIPTOR, *PCD_IO_BUFFER_DESCRIPTOR;

typedef struct _CD_IO_COMPLETE {
	LUID Identifier;
	IO_STATUS_BLOCK IoStatus;
	CD_IO_BUFFER_DESCRIPTOR Write;
} CD_IO_COMPLETE, *PCD_IO_COMPLETE;

typedef struct _CD_IO_CURSOR_INFORMATION {
	unsigned short Column;
	unsigned short Row;
	unsigned long Height;
	unsigned char IsVisible;
} CD_IO_CURSOR_INFORMATION, *PCD_IO_CURSOR_INFORMATION;

typedef struct _CD_IO_DESCRIPTOR {
	LUID Identifier;
	unsigned long Process;
	unsigned long Object;
	unsigned long Function;
	unsigned long InputSize;
	unsigned long OutputSize;
	unsigned long Reserved;
} CD_IO_DESCRIPTOR, *PCD_IO_DESCRIPTOR;

typedef struct _CD_IO_DISPLAY_SIZE {
	unsigned long Width;
	unsigned long Height;
} CD_IO_DISPLAY_SIZE, *PCD_IO_DISPLAY_SIZE;

typedef struct _CD_IO_OPERATION {
	LUID Identifier;
	CD_IO_BUFFER_DESCRIPTOR Buffer;
} CD_IO_OPERATION, *PCD_IO_OPERATION;

typedef struct _CD_IO_TABLE {
	LIST_ENTRY Slot[5];
} CD_IO_TABLE, *PCD_IO_TABLE;

typedef struct _CD_IO_PIPE {
	PEX_PUSH_LOCK Lock;
	unsigned char Connected;
	PIRP ReadIoIrp;
	LIST_ENTRY DeliveryQueue;
	CD_IO_TABLE DeliveredTable;
} CD_IO_PIPE, *PCD_IO_PIPE;

typedef struct _CD_IO_PARAMETERS {
	CD_IO_PIPE Pipe;
	unsigned long ObjectContext;
	PMDL InputMdlChain;
	PMDL OutputMdlChain;
} CD_IO_PARAMETERS, *PCD_IO_PARAMETERS;

typedef struct _CD_IO_PARAMETERS2 {
	unsigned long ProcessContext;
	void *Spare;
	LUID Identifier;
} CD_IO_PARAMETERS2, *PCD_IO_PARAMETERS2;

typedef struct _CD_IO_ROW_INFORMATION {
	short Index;
	PCD_IO_CHARACTER Old;
	PCD_IO_CHARACTER New;
} CD_IO_ROW_INFORMATION, *PCD_IO_ROW_INFORMATION;

typedef struct _CD_IO_SERVER_INFORMATION {
	void *InputAvailableEvent;
} CD_IO_SERVER_INFORMATION, *PCD_IO_SERVER_INFORMATION;

typedef struct _CD_SERVER {
	CD_CONTEXT_HEADER Header;
	LIST_ENTRY Links;
	unsigned int RefCount;
	PFILE_OBJECT FileObject;
	EX_PUSH_LOCK Lock;
	unsigned long Flags;
	void *SecurityDescriptor;
	CD_IO_PIPE Pipe;
	void *ServerProcessPid;
	PKEVENT InputEvent;
	LIST_ENTRY ConnectionList;
} CD_SERVER, *PCD_SERVER, *PRCD_SERVER;

typedef struct _CD_REFERENCE {
	CD_CONTEXT_HEADER Header;
	PCD_SERVER Server;
} CD_REFERENCE, *PCD_REFERENCE, *PRCDREFERENCE;

typedef struct _CD_USER_DEFINED_IO {
	void *Client;
	unsigned long InputCount;
	unsigned long OutputCount;
	CD_IO_BUFFER Buffers[1];
} CD_USER_DEFINED_IO, *PCD_USER_DEFINED_IO;

typedef struct _CD_USER_DEFINED_IO64 {
	void *Client;
	unsigned long InputCount;
	unsigned long OutputCount;
	CD_IO_BUFFER64 Buffers[1];
} CD_USER_DEFINED_IO64, *PCD_USER_DEFINED_IO64;

typedef struct _CD_CREATE_SERVER {
	void *BrokerHandle;
	LUID BrokerRequest;
} CD_CREATE_SERVER, *PCD_CREATE_SERVER;

typedef struct _CD_DEBUG_STACKS {
	void *Object;
	EX_PUSH_LOCK Lock;
	unsigned long NextIndex;
	void *Stacks[64][16];
} CD_DEBUG_STACKS, *PCD_DEBUG_STACKS;

typedef struct _CD_DISPLAY {
	CD_CONTEXT_HEADER Header;
} CD_DISPLAY, *PCD_DISPLAY;

typedef struct _BGFX_CONSOLE_STATE {
	unsigned long Width;
	unsigned long Height;
	unsigned long Columns;
	unsigned long Rows;
	unsigned long FontWidth;
	unsigned long FontHeight;
	unsigned long PadX;
	unsigned long PadY;
} BGFX_CONSOLE_STATE, *PBGFX_CONSOLE_STATE;

typedef struct _CD_BROKER {
	CD_CONTEXT_HEADER Header;
	LIST_ENTRY Links;
	unsigned int RefCount;
	EX_PUSH_LOCK Lock;
	CD_IO_PIPE Pipe;
	PEJOB Silo;
} CD_BROKER, *PCD_BROKER;

typedef struct _CD_CLIENT {
	CD_CONTEXT_HEADER Header;
	PCD_SERVER Server;
	union {
		unsigned long ObjectContext;
		unsigned long ObjectType;
	} u;
	PKEVENT InputEvent;
} CD_CLIENT, *PCD_CLIENT;

typedef struct _CD_CREATE_OBJECT_INFORMATION {
	unsigned long ObjectType;
	unsigned long ShareMode;
	unsigned long DesiredAccess;
} CD_CREATE_OBJECT_INFORMATION, *PCD_CREATE_OBJECT_INFORMATION;

typedef struct _CD_CLIENT_CREATE_CONTEXT {
	PCD_CLIENT Client;
	CD_CREATE_OBJECT_INFORMATION Information;
} CD_CLIENT_CREATE_CONTEXT, *PCD_CLIENT_CREATE_CONTEXT;

typedef struct _CD_CLIENT_TARGET {
	unsigned long ProcessContext;
	unsigned long ObjectContext;
	PCD_SERVER Server;
	PFILE_OBJECT ConnectionFile;
} CD_CLIENT_TARGET, *PCD_CLIENT_TARGET;

typedef struct _CD_CONNECTION {
	CD_CONTEXT_HEADER Header;
	LIST_ENTRY Links;
	PCD_SERVER Server;
	PEPROCESS SourceProcess;
	unsigned long ServerContext;
	unsigned long DefaultInputContext;
	unsigned long DefaultOutputContext;
} CD_CONNECTION, *PCD_CONNECTION, *PRCD_CONNECTION;

typedef struct _CD_CONNECTION_INFORMATION {
	unsigned long Process;
	unsigned long Input;
	unsigned long Output;
} CD_CONNECTION_INFORMATION, *PCD_CONNECTION_INFORMATION;

typedef struct _CD_CONNECTION_CREATE_CONTEXT {
	PCD_CONNECTION Connection;
	CD_CONNECTION_INFORMATION Reply;
} CD_CONNECTION_CREATE_CONTEXT, *PCD_CONNECTION_CREATE_CONTEXT;
