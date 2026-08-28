
void __cdecl ShellPrivateTraceEvent(_GUID *param_1,ulong param_2,ulong64 param_3,uchar param_4)

{
  undefined2 local_40 [2];
  undefined1 local_3c;
  ulong local_28;
  undefined4 uStack_24;
  uchar auStack_20 [12];
  undefined4 local_14;
  
  memset();
  local_14 = 0x20000;
  local_40[0] = 0x38;
  local_3c = (undefined1)param_3;
  local_28 = 0xa86c2a7e;
  uStack_24._0_2_ = 0xeda8;
  uStack_24._2_2_ = 0x445f;
  auStack_20[0] = 0xbd;
  auStack_20[1] = '6';
  auStack_20[2] = 0xa2;
  auStack_20[3] = 0xac;
  auStack_20[4] = '^';
  auStack_20[5] = 0x96;
  auStack_20[6] = 0xd8;
  auStack_20[7] = '\v';
  TraceEvent(param_1,param_2,local_40);
  return;
}
