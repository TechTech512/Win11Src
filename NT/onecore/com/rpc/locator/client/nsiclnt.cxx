#include <windows.h>

extern void __cdecl DeprecatedFunctionalityUseError(void);

long __cdecl RpcNsBindingImportBeginA(ULONG EntryNameSyntax, USHORT *EntryName, void *IfSpec, 
                                     void *BindingVec, void *UuidVec)
{
    DeprecatedFunctionalityUseError();
    return 0x6e4;
}

long __cdecl RpcNsBindingImportBeginW(ULONG EntryNameSyntax, USHORT *EntryName, void *IfSpec, 
                                     void *BindingVec, void *UuidVec)
{
    DeprecatedFunctionalityUseError();
    return 0x6e4;
}

long __cdecl RpcNsBindingImportDone(void **ImportContext)
{
    DeprecatedFunctionalityUseError();
    return 0x6e4;
}

long __cdecl RpcNsBindingLookupDone(void **LookupContext)
{
    DeprecatedFunctionalityUseError();
    return 0x6e4;
}

long __cdecl RpcNsBindingImportNext(void *ImportContext, void **Binding)
{
    DeprecatedFunctionalityUseError();
    return 0x6e4;
}

long __cdecl RpcNsBindingLookupNext(void *LookupContext, void **Binding)
{
    DeprecatedFunctionalityUseError();
    return 0x6e4;
}

long __cdecl RpcNsBindingSelect(void *LookupContext, void **Binding)
{
    DeprecatedFunctionalityUseError();
    return 0x6e4;
}

long __cdecl RpcNsMgmtHandleSetExpAge(void *NsHandle, void **ExpAge)
{
    DeprecatedFunctionalityUseError();
    return 0x6e4;
}

long __cdecl RpcNsBindingLookupBeginA(ULONG EntryNameSyntax, UCHAR *EntryName, void *IfSpec, 
                                     void *ObjUuid, ULONG BindingMaxCount, void **LookupContext)
{
    DeprecatedFunctionalityUseError();
    return 0x6e4;
}

long __cdecl RpcNsBindingLookupBeginW(ULONG EntryNameSyntax, UCHAR *EntryName, void *IfSpec, 
                                     void *ObjUuid, ULONG BindingMaxCount, void **LookupContext)
{
    DeprecatedFunctionalityUseError();
    return 0x6e4;
}

