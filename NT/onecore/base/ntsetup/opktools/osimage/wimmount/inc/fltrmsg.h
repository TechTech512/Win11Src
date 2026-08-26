typedef struct _COPY_INFORMATION COPY_INFORMATION, *PCOPY_INFORMATION;
#include <fltkernel.h>

typedef struct _PF_FILE_CONTEXT PF_FILE_CONTEXT, *PPF_FILE_CONTEXT;

typedef struct _PF_VERSION {
	unsigned short MajorVersion;
	unsigned short MinorVersion;
} PF_VERSION, *PPF_VERSION;

typedef struct _PF_CONNECTION_COOKIE {
	PF_VERSION Version;
	PFLT_PORT PortHandle;
	PFLT_FILTER FilterPointer;
	GUID ServicedGuid;
	long GuidSet;
} PF_CONNECTION_COOKIE, *PPF_CONNECTION_COOKIE;

typedef enum _PF_MESSAGE_TYPE {
	ExtractFile = 0,
	RunTests = 1,
	RegisterGuid = 2,
	VersionCheck = 3,
	ExtractFileAsync = 4,
	StubCreateChild = 5,
	StubRename = 6,
	SubsumeContext = 7,
	ExchangeDirs = 8,
	ExchangeCloseHandles = 9
} PF_MESSAGE_TYPE, *PPF_MESSAGE_TYPE;

typedef struct _PF_MESSAGE_HEADER {
	unsigned long Signature;
	unsigned short MajorVersion;
	unsigned short MinorVersion;
	unsigned long StructureSize;
	PF_MESSAGE_TYPE ActionType;
} PF_MESSAGE_HEADER, *PPF_MESSAGE_HEADER;

typedef struct _PF_MESSAGE_REGISTER_GUID {
	PF_MESSAGE_HEADER MessageHeader;
	GUID ConnectionGuid;
	unsigned long ProcessId;
} PF_MESSAGE_REGISTER_GUID, *PPF_MESSAGE_REGISTER_GUID;

typedef struct _PF_FILE_CONTEXT {
	KEVENT ExtractComplete;
	long ExtractResult;
	LARGE_INTEGER FileId;
	PPF_FILE_CONTEXT NextContext;
	PFILE_OBJECT FileObject;
} PF_FILE_CONTEXT, *PPF_FILE_CONTEXT;
