
/* WARNING: Function: _chkstk replaced with injection: alloca_probe */

uint __cdecl CallOE(wchar_t *param_1,int *param_2,bool param_3,ulong param_4)

{
  wchar_t *pwVar1;
  uint uVar2;
  uint *in_EDX;
  ulong unaff_ESI;
  int *unaff_EDI;
  uint uVar3;
  wchar_t local_26b0 [4688];
  undefined2 local_210 [260];
  uint local_8;
  
  local_8 = __security_cookie ^ (uint)&stack0xfffffffc;
  memset();
  memset();
  uVar3 = 0;
  pwVar1 = L"%ProgramW6432%\\Windows Mail\\WinMail";
  *in_EDX = 0;
  if ((char)param_1 == '\0') {
    pwVar1 = L"%ProgramFiles%\\Windows Mail\\WinMail";
  }
  uVar2 = ExpandEnvironmentStringsW(pwVar1,local_210,0x104);
  if (((int)uVar2 < 1) || (0x104 < uVar2)) {
    local_210[0] = 0;
    if (uVar2 == 0) {
      uVar2 = GetLastError();
      if (0 < (int)uVar2) {
        uVar2 = uVar2 & 0xffff | 0x80070000;
      }
    }
    else {
      uVar2 = 0x80004005;
    }
    if ((int)uVar2 < 0) goto LAB_00403012;
  }
  uVar2 = StringCchPrintfW(local_26b0,0x1250,L"\"%s\" %s");
  if ((-1 < (int)uVar2) &&
     (uVar2 = CreateProcessAndWait((wchar_t *)param_2,unaff_EDI,unaff_ESI), -1 < (int)uVar2)) {
    *in_EDX = uVar3;
  }
LAB_00403012:
  __security_check_cookie(uVar3);
  return uVar2;
}

int __cdecl CreateDefaultStore(int *param_1)

{
  INITSTATE IVar1;
  int iVar2;
  uint *unaff_EBX;
  uint unaff_ESI;
  int iVar3;
  bool bVar4;
  undefined1 uVar5;
  undefined4 local_214;
  undefined1 local_210 [520];
  uint local_8;
  wchar_t *pwVar6;
  
  local_8 = __security_cookie ^ (uint)&stack0xfffffffc;
  iVar3 = 0;
  uVar5 = 0xa5;
  IVar1 = GetInitState();
  bVar4 = IVar1 != isInitComplete;
  if (IVar1 == isInitInProgress) {
    local_214 = 0;
    memset();
    GetModuleFileNameW(0,local_210,0x104);
    PathStripPathW(local_210);
    pwVar6 = (wchar_t *)&local_214;
    iVar2 = StringLengthWorkerW(pwVar6,unaff_ESI,unaff_EBX);
    uVar5 = SUB41(pwVar6,0);
    if (-1 < iVar2) {
      iVar2 = StrCmpNIW(local_210,L"WinMail.exe",local_214);
      bVar4 = bVar4 && iVar2 != 0;
    }
  }
  if (bVar4) {
    iVar3 = CallOE((wchar_t *)0x0,(int *)0x4e20,(bool)uVar5,unaff_ESI);
    if (-1 < iVar3) {
      IVar1 = GetInitState();
      if (IVar1 != isInitComplete) {
        iVar3 = -0x7fff0001;
      }
    }
  }
  __security_check_cookie((uint)unaff_EBX);
  return iVar3;
}

uint __cdecl CreateProcessAndWait(wchar_t *param_1,int *param_2,ulong param_3)

{
  int iVar1;
  int iVar2;
  undefined4 in_ECX;
  undefined4 *in_EDX;
  uint uVar3;
  undefined4 local_60 [17];
  undefined4 local_1c;
  undefined4 local_18;
  undefined4 uStack_14;
  undefined4 uStack_10;
  undefined4 *local_c;
  undefined4 local_8;
  
  uVar3 = 0;
  local_1c = 0;
  local_18 = 0;
  uStack_14 = 0;
  uStack_10 = 0;
  local_c = in_EDX;
  memset();
  local_8 = 0;
  local_60[0] = 0x44;
  *local_c = 0;
  iVar1 = CreateProcessW(0,in_ECX,0,0,0,0,0,0,local_60,&local_1c);
  if (iVar1 != 0) {
    iVar1 = WaitForSingleObject(local_1c,param_1);
    if ((iVar1 == -1) || (iVar2 = GetExitCodeProcess(local_1c,&local_8), iVar2 == 0)) {
      uVar3 = GetLastError();
      if (0 < (int)uVar3) {
        uVar3 = uVar3 & 0xffff | 0x80070000;
      }
    }
    else {
      *local_c = local_8;
      if (iVar1 == 0x102) {
        uVar3 = 0x8010000a;
      }
    }
    CloseHandle(local_18);
    CloseHandle(local_1c);
    return uVar3;
  }
  uVar3 = GetLastError();
  if ((int)uVar3 < 1) {
    return uVar3;
  }
  return uVar3 & 0xffff | 0x80070000;
}

INITSTATE __cdecl GetInitState(void)

{
  int iVar1;
  undefined4 local_c;
  int local_8;
  
  local_8 = 0;
  local_c = 4;
  iVar1 = RegGetValueW(0x80000001,L"Software\\Microsoft\\Windows Mail Setup",L"DelayInitialized",
                       0x10,0,&local_8,&local_c);
  if (iVar1 == 0) {
    if (local_8 == 4) {
      return isInitComplete;
    }
    if (local_8 == 2) {
      return isInitDelayed;
    }
    if (local_8 == 3) {
      return isInitInProgress;
    }
  }
  return isFirstRun;
}
