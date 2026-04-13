
#include "stdafx.h"

// Class definition
class CResDDACL {
public:
	int _padding_;
    void* vftable;
    void *m_hLogFile;
	BOOL m_bErrorsDetected;
	BOOL m_bFix;
	HRESULT m_hrReturn;
	wchar_t m_szSystemPartition[260];
	wchar_t m_szLogDir[260];
	unsigned long m_ExecutionCount;
	
    // Methods
    void* __thiscall scalar_deleting_destructor(unsigned int param_1);
    unsigned long __thiscall CreateLogFile(void);
    unsigned long __thiscall EnumerateVolume(int param_1);
    unsigned long __thiscall FixACL(wchar_t* param_1);
    unsigned long __thiscall HandleEachVolume(int param_1, wchar_t* param_2);
    unsigned long __thiscall HasOtherOSVersion(wchar_t* param_1, int* param_2);
    unsigned long __thiscall HasXPDefaultDACL(wchar_t* param_1, int* param_2);
    unsigned long __thiscall Initialize(void);
    unsigned long __thiscall IsVolumeInteresting(wchar_t* param_1, int* param_2);
    unsigned long __thiscall IsVolumeSystem(wchar_t* param_1, int* param_2);
    void __thiscall logPrintA(unsigned long param_1, char* param_2);
    void __thiscall ReadSysPrepOptOutKey(int* param_1);
    unsigned long __thiscall ReadSystemPartition(wchar_t* param_1);
    unsigned long __thiscall ResetDirectoryTreeSecurity(wchar_t* param_1, wchar_t* param_2);
    unsigned long __thiscall TestACL(wchar_t* param_1, int* param_2);
    
    // Destructor
    ~CResDDACL();
};

