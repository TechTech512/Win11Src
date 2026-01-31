#include <windows.h>
#include <shellapi.h>
#include <winnetwk.h>
#include <sddl.h>
#include <wtsapi32.h>
#include <winbase.h>
#include <appmodel.h>
#include <winsafer.h>

// Declare the functions (they'll be resolved from the LIB at link time)
extern "C" {
    BOOL __cdecl CmdBatNotificationStub(DWORD dwNotificationType);
    void __cdecl DoSHChangeNotify(void);
    void* __cdecl FindFirstStreamWStub(wchar_t*, _STREAM_INFO_LEVELS, void*, unsigned long);
    int __cdecl FindNextStreamWStub(void*, void*);
    int __cdecl GetBinaryTypeWStub(wchar_t*, unsigned long*);
    unsigned long __cdecl GetVDMCurrentDirectoriesStub(unsigned long, char*);
    int __cdecl LookupAccountSidWStub(wchar_t*, void*, wchar_t*, unsigned long*, wchar_t*, unsigned long*, _SID_NAME_USE*);
    int __cdecl MessageBeepStub(unsigned int);
    int __cdecl QueryFullProcessImageNameWStub(void*, unsigned long, wchar_t*, unsigned long*);
    int __cdecl SaferWorker(_SAFER_CODE_PROPERTIES_V2*, SAFER_LEVEL_HANDLE__**, void**, wchar_t*, int*);
    int __cdecl ShellExecuteWorker(unsigned long, wchar_t*, wchar_t*, wchar_t*, int, void**, unsigned int*);
    unsigned long __cdecl WNetAddConnection2WStub(_NETRESOURCEW*, wchar_t*, wchar_t*, unsigned long);
    unsigned long __cdecl WNetCancelConnection2WStub(wchar_t*, unsigned long, int);
    unsigned long __cdecl WNetGetConnectionWStub(wchar_t*, wchar_t*, unsigned long*);
}
