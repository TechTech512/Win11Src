#include <windows.h>

typedef struct _CompatCheckContext {
    void *Reserved;
} CompatCheckContext;

int __cdecl HyperVUpgradeComplianceCheck(void *param_1, CompatCheckContext *param_2)
{
    return 1;
}

