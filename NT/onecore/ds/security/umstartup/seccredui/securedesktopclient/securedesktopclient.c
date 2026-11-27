#include <windows.h>
#include <rpc.h>
#include <rpcndr.h>

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
	unsigned char Format[293];
} _securedesktoprpc_MIDL_TYPE_FORMAT_STRING;
	
extern long __cdecl WlSecureDesktoppRpcClientInit(unsigned long param_1, void **param_2);

_securedesktoprpc_MIDL_TYPE_FORMAT_STRING securedesktoprpc__MIDL_TypeFormatString = {0,{0,0,0x15,0x03,0x04,0,'\b','[',0x1C,0,0x01,0,0x19,0,',',0,0x01,0,0x19,0,',',0,0x01,0,0x02,'[',0x1A,0x03,'<',0,0,0,0x14,0,'\r','6','6','6','6','6','\b',0x06,0x06,'\r','\b','\b','\b','6','\b','\b','\\','[',0x12,0,0xCC,0xFF,0x12,'\b','%','\\',0x12,'\b','%','\\',0x12,'\b','%','\\',0x12,'\b','%','\\',0x12,' ',0xBE,0xFF,0x1B,0,0x01,0,0x19,0,0x18,0,0,0,0x02,'[',0x1D,0,'\b',0,0x01,'[',0x15,0x03,0x10,0,'\b',0x06,0x06,'L',0,0xF1,0xFF,'[',0x16,0x03,'(',0,'\\','F','\\',0,0,0,0,0x12,'\b','%','\\','F','\\',0x04,0,0x04,0,0x12,'\b','%','\\','F','\\','\f',0,'\f',0,0x12,'\b','%','\\','F','\\',0x10,0,0x10,0,0x12,'\b','%','\\','F','\\',0x14,0,0x14,0,0x12,' ',0xAC,0xFF,'F','\\',0x1C,0,0x1C,0,0x12,0,0xB4,0xFF,'F','\\',' ',0,' ',0,0x12,'\b','%','\\','F','\\','$',0,'$',0,0x12,'\b','%','\\','[','\b','\b','\b','\b','\b','\b','\b','\b','\b','\b','[',0x12,' ',0x02,0,0x1B,0,0x01,0,')',0,'|',0,0,0,0x02,'[',0x11,0x14,0x02,0,0x12,' ',0x02,0,0x1B,0,0x01,0,')','T',0x8C,0,0,0,0x02,'[',0x11,'\f','\b','\\',0x11,'\b','\b','\\',0x12,'\b','%','\\',0x11,0x14,0x02,0,0x12,' ',0x02,0,0x1B,0x01,0x02,0,')','T',0x1C,0,0,0,0x05,'[',0x11,0x14,0x02,0,0x12,' ',0x02,0,0x1B,0,0x01,0,')','T','$',0,0,0,0x02,'[',0}};
RPC_CLIENT_INTERFACE ISecureDesktop___RpcClientInterface = {0x44,{{0x12E65DD8,0x887F,0x41EF,{0x91,0xBF,0x8D,0x81,'l','B',0xC2,0xE7}},{0x1,0}},{{0x8A885D04,0x1CEB,0x11C9,{0x9F,0xE8,'\b',0,'+',0x10,'H','`'}},{0x2,0}},NULL,0,NULL,0,NULL,0};
MIDL_STUB_DESC ISecureDesktop_StubDesc = {
    &ISecureDesktop___RpcClientInterface,      // void * RpcInterfaceInformation
    MIDL_user_allocate,                 // void *(*)(size_t)
    MIDL_user_free,                     // void (*)(void *)
    0,                                  // IMPLICIT_HANDLE_INFO (auto handle)
    NULL,                               // apfnNdrRundownRoutines
    NULL,                               // aGenericBindingRoutinePairs
    NULL,                               // apfnExprEval
    NULL,                               // aXmitQuintuple
    securedesktoprpc__MIDL_TypeFormatString.Format,  // const unsigned char * pFormatTypes
    0x1,                                // fCheckBounds
    0xA0000,                            // Version
    NULL,                               // pMallocFreeStruct
    0x801026A,                          // MIDLVersion
    NULL,                               // CommFaultOffsets
    NULL,                               // aUserMarshalQuadruple
    NULL,                               // NotifyRoutineTable
    0x1,                                // mFlags
    NULL,                               // CsRoutineTables
    NULL,                               // ProxyServerInfo
    NULL                                // pExprInfo
};

/* WARNING: Function: __SEH_prolog4 replaced with injection: SEH_prolog4 */
/* WARNING: Function: __SEH_epilog4 replaced with injection: EH_epilog3 */
/* WARNING: Removing unreachable block (ram,0x10001801) */

int __cdecl WlSecureDesktopSimulateSAS(void)

{
  long lVar1;
  void **unaff_EDI = 0;
  unsigned long in_stack_ffffffc8 = 0;
  
  lVar1 = WlSecureDesktoppRpcClientInit(in_stack_ffffffc8,unaff_EDI);
  if (lVar1 == 0) {
    NdrClientCall4(&ISecureDesktop_StubDesc,(PFORMAT_STRING)0x1000137e,0);
  }
  if (lVar1 != 0) {
    SetLastError(lVar1);
  }
  return (unsigned int)(lVar1 == 0);
}
