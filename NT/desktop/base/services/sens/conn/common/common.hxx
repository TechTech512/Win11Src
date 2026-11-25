#include <windows.h>
#include <rpc.h>

#pragma comment(lib, "Rpcrt4.lib")

extern "C" {

SID LocalSystem = {
    1,
    1,
    {{0, 0, 0, 0, 0, 5}},
    { SECURITY_LOCAL_SYSTEM_RID }
};

long __cdecl BindToLRpcService(void **param_1, wchar_t *param_2)
{
    (void)param_2;

    RPC_BINDING_HANDLE *binding = reinterpret_cast<RPC_BINDING_HANDLE *>(param_1);
    *binding = nullptr;

    RPC_STATUS status = RpcBindingFromStringBindingW(
        reinterpret_cast<RPC_WSTR>(const_cast<wchar_t *>(L"ncalrpc:[senssvc]")),
        binding);

    if (status == RPC_S_OK) {
        RPC_SECURITY_QOS securityQos = { 3, 1, 1, 3 };

        status = RpcBindingSetAuthInfoExW(
            *binding,
            nullptr,
            6,
            10,
            nullptr,
            0,
            &securityQos);

        if (status != RPC_S_OK) {
            RpcBindingFree(binding);
        }
    }

    return static_cast<long>(status);
}

long __cdecl BindToSensService(void **param_1)
{
    RPC_BINDING_HANDLE *binding = reinterpret_cast<RPC_BINDING_HANDLE *>(param_1);
    *binding = nullptr;

    RPC_STATUS status = RpcBindingFromStringBindingW(
        reinterpret_cast<RPC_WSTR>(const_cast<wchar_t *>(L"ncalrpc:[senssvc]")),
        binding);

    if (status == RPC_S_OK) {
        RPC_SECURITY_QOS securityQos = { 3, 1, 1, 3 };

        status = RpcBindingSetAuthInfoExW(
            *binding,
            nullptr,
            6,
            10,
            nullptr,
            0,
            &securityQos);

        if (status != RPC_S_OK) {
            RpcBindingFree(binding);
        }
    }

    return static_cast<long>(status);
}

} // extern "C"

