#pragma warning (disable:4995)

#include "resacl.h"

// External functions
extern void _guard_check_icall_nop(int param);

// CallFixACL function
unsigned long __cdecl CallFixACL(int param_1, wchar_t* param_2)
{
    CResDDACL *pObject;
    CResDDACL *thisPtr;
    unsigned long result;
    int disableFlag;
    unsigned int securityCookie;
    
    securityCookie = __security_cookie ^ (unsigned int)&securityCookie;
    disableFlag = 0;
    
    pObject = (CResDDACL*)operator new(0x424);
    
    if (pObject == (CResDDACL*)0x0) {
        thisPtr = (CResDDACL*)0x0;
    }
    else {
        pObject->m_hLogFile = INVALID_HANDLE_VALUE;
        pObject->m_szSystemPartition[0] = L'\0';
        pObject->m_szLogDir[0] = L'\0';
        pObject->m_bErrorsDetected = false;
        pObject->m_ExecutionCount = 5;
        thisPtr = pObject;
    }
    
    if (thisPtr == (CResDDACL*)0x0) {
        result = 0xe;
    }
    else {
        result = thisPtr->Initialize();
        
        if (result == 0) {
            thisPtr->ReadSysPrepOptOutKey(&disableFlag);
            
            if (disableFlag == 0) {
                result = thisPtr->EnumerateVolume(param_1);
            }
        }
    }
    
    if (thisPtr != (CResDDACL*)0x0) {
        void (*destructor)(CResDDACL*) = *(void (**)(CResDDACL*))thisPtr->_padding_;
        if (destructor) {
            destructor(thisPtr);
        }
    }
    
    __security_check_cookie(securityCookie);
    return result;
}

// DDACLSys_Offline_Specialize function
unsigned long __cdecl DDACLSys_Offline_Specialize(struct _SYSPREP_OS_OFFLINE* param_1)
{
    if ((param_1 != (_SYSPREP_OS_OFFLINE*)0x0) && 
        (*(HKEY__**)((char*)param_1 + 0x30) != (HKEY__*)0x0)) {
        return CallFixACL((int)param_1, *(wchar_t**)param_1);
    }
    return 0x57;
}

// DDACLSys_Specialize function
unsigned long __cdecl DDACLSys_Specialize(void)
{
    CResDDACL* pObject;
    CResDDACL* thisPtr;
    unsigned long result;
    int disableFlag;
    unsigned int securityCookie;
    
    securityCookie = __security_cookie ^ (unsigned int)&securityCookie;
    disableFlag = 0;
    
    pObject = (CResDDACL*)operator new(0x424);
    
    if (pObject == (CResDDACL*)0x0) {
        thisPtr = (CResDDACL*)0x0;
    }
    else {
        pObject->m_hLogFile = INVALID_HANDLE_VALUE;
        pObject->m_szSystemPartition[0] = L'\0';
        pObject->m_szLogDir[0] = L'\0';
        pObject->m_bErrorsDetected = false;
        pObject->m_ExecutionCount = 5;
        thisPtr = pObject;
    }
    
    if (thisPtr == (CResDDACL*)0x0) {
        result = 0xe;
    }
    else {
        result = thisPtr->Initialize();
        
        if (result == 0) {
            thisPtr->ReadSysPrepOptOutKey(&disableFlag);
            
            if (disableFlag == 0) {
                result = thisPtr->EnumerateVolume(0);
            }
        }
    }
    
    if (thisPtr != (CResDDACL*)0x0) {
        void (*destructor)(CResDDACL*) = *(void (**)(CResDDACL*))thisPtr->_padding_;
        if (destructor) {
            destructor(thisPtr);
        }
    }
    
    __security_check_cookie(securityCookie);
    return result;
}

// DllMain
int DllMain(void *hInstance, ULONG dwReason, void *lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls((HMODULE)hInstance);
    }
    return TRUE;
}

