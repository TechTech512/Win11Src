#include <windows.h>

extern void __cdecl DeprecatedFunctionalityUseError(void);

void __cdecl I_RpcNsRaiseException(void *Message, long Status)
{
    DeprecatedFunctionalityUseError();
    return;
}

long __cdecl I_RpcNsGetBuffer(void **Buffer)
{
    DeprecatedFunctionalityUseError();
    return 0x6e4;
}

long __cdecl I_RpcNsNegotiateTransferSyntax(void **Message)
{
    DeprecatedFunctionalityUseError();
    return 0x6e4;
}

long __cdecl I_RpcReBindBuffer(void **Buffer)
{
    DeprecatedFunctionalityUseError();
    return 0x6e4;
}

long __cdecl I_RpcNsSendReceive(void *Message, void **Buffer)
{
    DeprecatedFunctionalityUseError();
    return 0x6e4;
}

