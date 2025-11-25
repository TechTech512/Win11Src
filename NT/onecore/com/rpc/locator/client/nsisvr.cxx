#include <windows.h>

extern void __cdecl DeprecatedFunctionalityUseError(void);

long __cdecl RpcNsBindingExportA(ULONG EntryNameSyntax, UCHAR *EntryName, void *IfSpec, 
                                void *BindingVec, void *UuidVec)
{
    DeprecatedFunctionalityUseError();
    return 0;
}

long __cdecl RpcNsBindingExportPnPA(ULONG EntryNameSyntax, UCHAR *EntryName, void *IfSpec, 
                                   void *UuidVec)
{
    DeprecatedFunctionalityUseError();
    return 0x6e4;
}

long __cdecl RpcNsBindingExportPnPW(ULONG EntryNameSyntax, UCHAR *EntryName, void *IfSpec, 
                                   void *UuidVec)
{
    DeprecatedFunctionalityUseError();
    return 0x6e4;
}

long __cdecl RpcNsBindingUnexportPnPA(ULONG EntryNameSyntax, UCHAR *EntryName, void *IfSpec, 
                                     void *UuidVec)
{
    DeprecatedFunctionalityUseError();
    return 0x6e4;
}

long __cdecl RpcNsBindingUnexportPnPW(ULONG EntryNameSyntax, UCHAR *EntryName, void *IfSpec, 
                                     void *UuidVec)
{
    DeprecatedFunctionalityUseError();
    return 0x6e4;
}

long __cdecl RpcNsBindingUnexportW(ULONG EntryNameSyntax, UCHAR *EntryName, void *IfSpec, 
                                  void *UuidVec)
{
    DeprecatedFunctionalityUseError();
    return 0x6e4;
}

long __cdecl RpcNsBindingExportW(ULONG EntryNameSyntax, USHORT *EntryName, void *IfSpec, 
                                void *BindingVec, void *UuidVec)
{
    DeprecatedFunctionalityUseError();
    return 0x6e4;
}

long __cdecl RpcNsBindingUnexportA(ULONG EntryNameSyntax, UCHAR *EntryName, void *IfSpec, 
                                  void *UuidVec)
{
    DeprecatedFunctionalityUseError();
    return 0;
}

