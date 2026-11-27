#include <windows.h>
#include <strsafe.h>
#include <rpc.h>

#pragma comment(lib, "Rpcrt4.lib")

long __cdecl WlSecureDesktoppRpcClientInit(unsigned long param_1, void **param_2)
{
    RPC_STATUS status;
    RPC_WSTR stringBinding = NULL;
    RPC_BINDING_HANDLE binding = NULL;
    wchar_t uuidString[38];
    RPC_SECURITY_QOS securityQos = { 0 };

    status = StringCchPrintfW(
        uuidString,
        0x25,
        L"52EF130C-08FD-4388-86B3-6EDF%08X",
        param_1);

    if (SUCCEEDED(status)) {
        status = RpcStringBindingComposeW(
            (RPC_WSTR)uuidString,
            (RPC_WSTR)L"ncalrpc",
            NULL,
            NULL,
            NULL,
            &stringBinding);

        if (status == RPC_S_OK) {
            status = RpcBindingFromStringBindingW(stringBinding, &binding);

            if (status == RPC_S_OK) {
                securityQos.Version = RPC_C_SECURITY_QOS_VERSION_1;
                securityQos.Capabilities = RPC_C_QOS_CAPABILITIES_DEFAULT;
                securityQos.IdentityTracking = RPC_C_QOS_IDENTITY_DYNAMIC;
                securityQos.ImpersonationType = RPC_C_IMP_LEVEL_DEFAULT;

                status = RpcBindingSetAuthInfoExW(
                    binding,
                    NULL,
                    RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                    RPC_C_AUTHN_WINNT,
                    NULL,
                    RPC_C_AUTHZ_NONE,
                    &securityQos);

                if (status == RPC_S_OK) {
                    *param_2 = binding;
                }
            }
        }
    }

    if (stringBinding != NULL) {
        RpcStringFreeW(&stringBinding);
    }

    if (status != RPC_S_OK) {
        RpcBindingFree(&binding);
    }

    return (long)status;
}
