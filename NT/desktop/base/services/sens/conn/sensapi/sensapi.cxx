#include "common.hxx"
#include <rpcndr.h>
#include <sensapi.h>

extern VOID
MIDL_user_free (
    IN void *MemPointer
    );

extern PVOID
MIDL_user_allocate (
    IN size_t NumBytes
    );
	
typedef struct {
	short Pad;
	unsigned char Format[29];
} _api_MIDL_TYPE_FORMAT_STRING;

typedef struct _SENS_CACHE {
	unsigned long dwCacheVer;
	unsigned long dwCacheSize;
	unsigned long dwCacheInitTime;
	unsigned long dwLastUpdateTime;
	unsigned long dwLastUpdateState;
	long dwLANState;
	unsigned long dwLastLANTime;
	long dwWANState;
	unsigned long dwLastWANTime;
	unsigned long dwLocked;
} SENS_CACHE, *PSENS_CACHE;

unsigned long gdwCacheFirstReadTime = 0;
unsigned long gdwCacheLastUpdatedTime = 0;
PSENS_CACHE gpSensCache = NULL;
COMM_FAULT_OFFSETS SensApi_CommFaultOffsets[3] = {
    { (short)0xFFFF, (short)0xFFFF },
    { (short)0xFFFF, (short)0xFFFF },
    { (short)0xFFFF, (short)0xFFFF }
};
_api_MIDL_TYPE_FORMAT_STRING api__MIDL_TypeFormatString = {0,{0,0,0x11,'\f','\b',0x11,'\b','%','\\',0x12,0,0x02,0,0x15,0x03,0x10,0,'\b','\b','\b','\b','\\','[',0x11,'\b','"','\\',0}};
RTL_CRITICAL_SECTION gSensapiLock;
PVOID ghSens = 0;
RPC_CLIENT_INTERFACE SensApi___RpcClientInterface = {0x44,{{0x63FBE424,0x2029,0x11D1,{0x8D,0xB8,0,0xAA,0,'J',0xBD,'^'}},{0x1,0}},{{0x8A885D04,0x1CEB,0x11C9,{0x9F,0xE8,'\b',0,'+',0x10,'H','`'}},{0x2,0}},NULL,0,NULL,0,NULL,0};
MIDL_STUB_DESC SensApi_StubDesc = {
    &SensApi___RpcClientInterface,      // void * RpcInterfaceInformation
    MIDL_user_allocate,                 // void *(*)(size_t)
    MIDL_user_free,                     // void (*)(void *)
    0,                                  // IMPLICIT_HANDLE_INFO (auto handle)
    NULL,                               // apfnNdrRundownRoutines
    NULL,                               // aGenericBindingRoutinePairs
    NULL,                               // apfnExprEval
    NULL,                               // aXmitQuintuple
    api__MIDL_TypeFormatString.Format,  // const unsigned char * pFormatTypes
    0x1,                                // fCheckBounds
    0xA0000,                            // Version
    NULL,                               // pMallocFreeStruct
    0x801026A,                          // MIDLVersion
    SensApi_CommFaultOffsets,           // CommFaultOffsets
    NULL,                               // aUserMarshalQuadruple
    NULL,                               // NotifyRoutineTable
    0x1,                                // mFlags
    NULL,                               // CsRoutineTables
    NULL,                               // ProxyServerInfo
    NULL                                // pExprInfo
};

void __cdecl UnmapSensCacheView(void)

{
  if (gpSensCache != (PSENS_CACHE)0x0) {
    UnmapViewOfFile(gpSensCache);
  }
  gpSensCache = (PSENS_CACHE)0x0;
  return;
}

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, PVOID lpReserved)

{
  if (fdwReason == DLL_PROCESS_DETACH) {
    DeleteCriticalSection(&gSensapiLock);
    UnmapSensCacheView();
    if (ghSens != (void *)0x0) {
      RpcBindingFree(&ghSens);
    }
  }
  else if (fdwReason == DLL_PROCESS_ATTACH) {
    DisableThreadLibraryCalls(hInstance);
    InitializeCriticalSection(&gSensapiLock);
  }
  return TRUE;
}

long __cdecl DoRpcSetup(void)

{
  long lVar1;
  PRTL_CRITICAL_SECTION p_Var2;
  
  lVar1 = 0;
  if (ghSens == (void *)0x0) {
    p_Var2 = &gSensapiLock;
    EnterCriticalSection(&gSensapiLock);
    if (ghSens == (void *)0x0) {
      lVar1 = BindToSensService((void **)&p_Var2->DebugInfo);
      if (lVar1 == 0) {
        ghSens = (void *)0x0;
      }
    }
    LeaveCriticalSection(&gSensapiLock);
  }
  else {
    lVar1 = 0;
  }
  return lVar1;
}

BOOL APIENTRY
IsDestinationReachableA(
    LPCSTR lpszDestination,
    LPQOCINFO lpQOCInfo
    )

{
                    /* 0x16a0  1  IsDestinationReachableA
                       0x16a0  2  IsDestinationReachableW */
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

BOOL APIENTRY
IsDestinationReachableW(
    LPCWSTR lpszDestination,
    LPQOCINFO lpQOCInfo
    )

{
                    /* 0x16a0  1  IsDestinationReachableA
                       0x16a0  2  IsDestinationReachableW */
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

int __cdecl MapSensCacheView(void)

{
  int iVar1;
  
  iVar1 = (int)OpenFileMappingW(4,0,L"SENS Information Cache");
  if (iVar1 != 0) {
    gpSensCache = (_SENS_CACHE *)MapViewOfFile((HANDLE)iVar1,4,0,0,0);
    CloseHandle((HANDLE)iVar1);
    if (gpSensCache != (PSENS_CACHE)0x0) {
      return 1;
    }
  }
  UnmapSensCacheView();
  return 0;
}

int __cdecl ReadConnectivityCache(unsigned long *param_1)

{
  unsigned long uVar1;
  unsigned long uVar2;
  unsigned long uVar3;
  int iVar4;
  unsigned long *in_ECX = 0;
  
  uVar3 = GetTickCount();
  EnterCriticalSection(&gSensapiLock);
  if ((((gpSensCache == (PSENS_CACHE)0x0) &&
       (iVar4 = MapSensCacheView(), uVar2 = gdwCacheLastUpdatedTime, iVar4 == 0)) ||
      (uVar1 = gdwCacheFirstReadTime, uVar2 = gpSensCache->dwLastUpdateTime,
      gpSensCache->dwLastUpdateTime != gdwCacheLastUpdatedTime)) ||
     (((uVar2 = gdwCacheLastUpdatedTime, gdwCacheFirstReadTime != 0 &&
       (180000 < uVar3 - gdwCacheFirstReadTime)) || (gpSensCache->dwLastUpdateState == 0)))) {
    gdwCacheLastUpdatedTime = uVar2;
    LeaveCriticalSection(&gSensapiLock);
    iVar4 = 0;
  }
  else {
    *in_ECX = gpSensCache->dwLastUpdateState;
    if (uVar1 == 0) {
      gdwCacheFirstReadTime = uVar3;
    }
    LeaveCriticalSection(&gSensapiLock);
    SetLastError(0);
    iVar4 = 1;
  }
  return iVar4;
}

BOOL APIENTRY
IsNetworkAlive(
    LPDWORD lpdwFlags
    )

{
  int iVar1;
  long lVar2;
  int local_14;
  int local_10;
  unsigned long local_c;
  int local_8;
  
                    /* 0x15e0  3  IsNetworkAlive */
  if (lpdwFlags == (unsigned long *)0x0) {
    SetLastError(ERROR_INVALID_PARAMETER);
    iVar1 = FALSE;
  }
  else {
    *lpdwFlags = 0;
    local_c = 0;
    local_8 = 0;
    local_10 = 0;
    /* was: ReadConnectivityCache(unaff_EBX) -- pass the real out-param */
    iVar1 = ReadConnectivityCache(&local_c);
    if (iVar1 != 1) {
      lVar2 = DoRpcSetup();
      if (((lVar2 != 0) ||
          /* CLIENT_CALL_RETURN is a union; take .Simple and narrow to long */
          (lVar2 = (long)NdrClientCall4(&SensApi_StubDesc,
                                        (PFORMAT_STRING)0x7600113a,
                                        ghSens,
                                        &local_c,
                                        &local_10,
                                        &local_8).Simple,
          lVar2 != 0)) || (local_8 == 0x6ba)) {
        local_14 = 0;
        GetOsSafeBootMode((PDWORD)&local_14);
        if (local_14 != 1) {
          local_10 = 1;
          local_c = 1;
          if (lVar2 != 0) {
            local_8 = lVar2;          /* long -> int; deliberate, matches original RPC_STATUS path */
          }
        }
      }
      *lpdwFlags = local_c;
      SetLastError(local_8);
      gdwCacheFirstReadTime = 0;
      iVar1 = local_10;
    }
  }
  return iVar1;
}
