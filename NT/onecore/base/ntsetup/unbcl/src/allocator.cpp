// allocator.cpp

int g_RunningState = 0;

int __cdecl IsRunning(void)
{
    return g_RunningState;
}

void __cdecl SetRunningFlag(int param_1)
{
    g_RunningState = param_1;
}

