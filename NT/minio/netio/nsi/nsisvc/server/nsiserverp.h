// ignore this file. this is a decompile and we're using msvcrt's malloc


void * __cdecl MALLOC(ulong param_1)

{
  void *pvVar1;
  
  pvVar1 = (void *)HeapAlloc(HeapHandle,0);
  if (pvVar1 != (void *)0x0) {
    return pvVar1;
  }
  SetLastError(8);
  return (void *)0x0;
}



