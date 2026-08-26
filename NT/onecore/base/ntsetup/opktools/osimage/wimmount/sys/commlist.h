#include "fltrmsg.h"

typedef struct _WM_COMM_LIST_ENTRY {
	LIST_ENTRY Link;
	unsigned long RefCount;
	GUID Guid;
	PFLT_PORT PortHandle;
	PVOID *ProcessHandle;
	unsigned long ProcessId;
	PF_VERSION Version;
} WM_COMM_LIST_ENTRY, *PWM_COMM_LIST_ENTRY;

typedef struct _WM_COMM_LIST {
	LIST_ENTRY ListHead;
	ERESOURCE ListLock;
} WM_COMM_LIST, *PWM_COMM_LIST;
