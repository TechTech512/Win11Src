#include "precomp.h"

GUID Microsoft_Windows_Shell_Core_Provider = {0x30336ED4,0xE327,0x447C,{0x9D,0xE0,'Q',0xB6,'R',0xC8,'a','\b'}};
EVENT_DESCRIPTOR Fail_Obsolete_Shell_Api = {0xF6E0,0x00,0x11,0x04,0x00,0,0x4000000002000000};
int __cdecl func();
struct IAutoListDescription;
struct IVisibleInDescription;
struct IBrowserThreadHandshake;
typedef int (__cdecl *_func___cdecl_int)(void);

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, PVOID lpReserved) {
	
	short sVar1;
	
	if (fdwReason == 1) {
		DisableThreadLibraryCalls(hInstance);
		sVar1 = FindAtomW(L"FailObsoleteShellAPIs");
		if (sVar1 == 0) {
			EtwEventWriteNoRegistration(&Microsoft_Windows_Shell_Core_Provider,&Fail_Obsolete_Shell_Api,0,0);
			return FALSE;
		}
	}
	return TRUE;
}

void __cdecl RETIRED__DeletePidlDPA(HDPA param_1) {
	return;
}

int __cdecl RETIRED_SHStartDDEThread(IETHREADPARAM *param_1)

{
                    /* 0x2a50  233   */
  return FALSE;
}

void * __cdecl RETIRED_Alloc(long param_1)

{
                    /* 0x2170  105   */
  return (void *)0x0;
}

HRESULT __cdecl RETIRED_ApplyTheme(PVOID param_1,unsigned long param_2,HWND param_3)

{
                    /* 0x3970  445   */
  return E_NOTIMPL;
}

HRESULT __cdecl
RETIRED_AssocMakeFileExtsToApplicationA(unsigned long param_1,char *param_2,char *param_3)

{
                    /* 0x2b00  239   */
  return E_UNEXPECTED;
}

HRESULT __cdecl
RETIRED_AssocMakeFileExtsToApplicationW(unsigned long param_1,wchar_t *param_2,wchar_t *param_3)

{
                    /* 0x2af0  238   */
  return E_UNEXPECTED;
}

int __cdecl RETIRED_SHIsBadInterfacePtr(void *param_1,unsigned int param_2)

{
                    /* 0x1d20  40   */
  return FALSE;
}

int __cdecl RETIRED_SHIsCurrentProcessConsoleSession(void)

{
                    /* 0x34e0  378   */
  return FALSE;
}

int __cdecl RETIRED_SHIsGlobalOffline(void)

{
                    /* 0x3030  295   */
  return FALSE;
}

int __cdecl RETIRED_SHIsRegisteredClient(wchar_t *param_1)

{
                    /* 0x30c0  301   */
  return FALSE;
}

int __cdecl RETIRED_SHIsRestricted(HWND param_1,RESTRICTIONS param_2)

{
                    /* 0x3020  294   */
  return TRUE;
}

int __cdecl RETIRED_SHIsSameObject(IUnknown *param_1,IUnknown *param_2)

{
                    /* 0x2b90  246   */
  return FALSE;
}

int __cdecl RETIRED_SHIsTempDisplayMode(void)

{
                    /* 0x3520  381   */
  return FALSE;
}

HRESULT __cdecl RETIRED_SHKnownFolderToCSIDL(GUID *param_1,int *param_2)

{
                    /* 0x35c0  387   */
  *param_2 = -1;
  return E_NOTIMPL;
}

HRESULT __cdecl RETIRED_SHLoadOLE(long param_1)

{
                    /* 0x1de0  51   */
  return FALSE;
}

HRESULT __cdecl RETIRED_SHLoadPersistedDataObject(IDataObject *param_1,IDataObject **param_2)

{
                    /* 0x2b60  242   */
  if (param_2 != (IDataObject **)0x0) {
    *param_2 = (IDataObject *)0x0;
  }
  return E_NOTIMPL;
}

void * __cdecl RETIRED_SHLoadRawAccelerators(HINSTANCE param_1,wchar_t *param_2)

{
                    /* 0x32d0  338   */
  return (void *)0x0;
}

void * __cdecl RETIRED_SHLockShared(void *param_1,unsigned long param_2)

{
                    /* 0x1e90  61   */
  return (void *)0x0;
}

int __cdecl RETIRED_SHLookupIconIndexA(char *param_1,int param_2,unsigned int param_3)

{
                    /* 0x27b0  192   */
  return -1;
}

int __cdecl RETIRED_SHLookupIconIndexW(wchar_t *param_1,int param_2,unsigned int param_3)

{
                    /* 0x27c0  193   */
  return -1;
}

HRESULT __cdecl
RETIRED_SHMapIDListToImageListIndexAsync
          (IShellTaskScheduler *param_1,IShellFolder *param_2,ITEMIDLIST *param_3,unsigned int param_4,
          int (__cdecl *param_5)(void),void *param_6,void *param_7,int *param_8,int *param_9)

{
                    /* 0x1dd0  50   */
  return E_NOTIMPL;
}

HRESULT __cdecl RETIRED_SHMapNbspToSp(wchar_t *param_1,char *param_2,int param_3)

{
                    /* 0x2a00  226   */
  return E_NOTIMPL;
}

int __cdecl
RETIRED_SHMessageBoxHelpA
          (HWND param_1,char *param_2,char *param_3,unsigned int param_4,HRESULT param_5,
          HRESULTHELPMAPPING *param_6,unsigned long param_7)

{
                    /* 0x3400  369   */
  return -1;
}

int __cdecl
RETIRED_SHMessageBoxHelpW
          (HWND param_1,wchar_t *param_2,wchar_t *param_3,unsigned int param_4,HRESULT param_5,
          HRESULTHELPMAPPING *param_6,unsigned long param_7)

{
                    /* 0x3410  370   */
  return -1;
}

int __cdecl RETIRED_SHMirrorIcon(HICON *param_1,HICON *param_2)

{
                    /* 0x2bc0  247   */
  return FALSE;
}

int __cdecl RETIRED_SHOnCWMCommandLine(long param_1)

{
                    /* 0x2200  113   */
  return FALSE;
}

int __cdecl RETIRED_SHOpenEffectiveToken_shell32(void **param_1)

{
                    /* 0x2930  212   */
  *param_1 = (void *)0x0;
  return FALSE;
}

int __cdecl RETIRED_SHOpenFolderWindow(IETHREADPARAM *param_1)

{
                    /* 0x2e00  266   */
  return FALSE;
}

HRESULT __cdecl
RETIRED_SHOpenNewFrame(ITEMIDLIST *param_1,ITravelLog *param_2,unsigned long param_3,unsigned int param_4)

{
                    /* 0x2e10  267   */
  return E_NOTIMPL;
}

int __cdecl
RETIRED_SHOpenPropSheetA
          (char *param_1,HKEY *param_2,unsigned int param_3,GUID *param_4,IDataObject *param_5,
          IShellBrowser *param_6,char *param_7)

{
                    /* 0x1f10  69   */
  return FALSE;
}

int __cdecl RETIRED_SHOutOfMemoryMessageBox(HWND param_1,wchar_t *param_2,unsigned int param_3)

{
                    /* 0x1da0  47   */
  return -1;
}

int __cdecl RETIRED_SHParseIECommandLine(wchar_t **param_1,IETHREADPARAM *param_2)

{
                    /* 0x2e70  273   */
  return FALSE;
}

HRESULT __cdecl RETIRED_SHPersistDataObject(IDataObject *param_1,tagSTGMEDIUM *param_2)

{
                    /* 0x2b50  241   */
  return E_NOTIMPL;
}

HRESULT __cdecl
RETIRED_SHPidlFromDataObject
          (IDataObject *param_1,ITEMIDLIST **param_2,wchar_t *param_3,unsigned long param_4)

{
                    /* 0x21e0  112   */
  *param_2 = (ITEMIDLIST *)0x0;
  return E_NOTIMPL;
}

int __cdecl RETIRED_SHPropertyBag_ReadBOOLOld(IPropertyBag *param_1,wchar_t *param_2,int param_3)

{
                    /* 0x3350  348   */
  return FALSE;
}

int __cdecl
RETIRED_SHQueryRawAccelerator(void *param_1,unsigned char param_2,unsigned char param_3,unsigned int param_4,unsigned int *param_5)

{
                    /* 0x32e0  339   */
  return FALSE;
}

int __cdecl RETIRED_SHQueryRawAcceleratorMsg(void *param_1,tagMSG *param_2,unsigned int *param_3)

{
                    /* 0x32f0  340   */
  return FALSE;
}

HRESULT __cdecl RETIRED_SHSafeQueryInterface(IUnknown *param_1,GUID *param_2,void **param_3)

{
                    /* 0x3390  352   */
  *param_3 = (void *)0x0;
  return E_NOTIMPL;
}

HRESULT __cdecl
RETIRED_SHRegGetCLSIDKeyA(GUID *param_1,char *param_2,int param_3,int param_4,HKEY *param_5)

{
                    /* 0x25d0  166   */
  return E_NOTIMPL;
}

HRESULT __cdecl RETIRED_SHRegisterDragDrop(HWND param_1,IDropTarget *param_2)

{
                    /* 0x1d30  41   */
  return E_NOTIMPL;
}

void * __cdecl
RETIRED_SHRegisterWaitForSingleObject
          (void *param_1,void (__cdecl *param_2)(void *, unsigned char),void *param_3,unsigned long param_4,
          char *param_5,unsigned long param_6)

{
                    /* 0x2550  158   */
  return (void *)0x0;
}

HRESULT __cdecl RETIRED_SHRevokeDragDrop(HWND param_1)

{
                    /* 0x1d40  42   */
  return E_NOTIMPL;
}

int __cdecl RETIRED_SHRunControlPanel(wchar_t *param_1,HWND param_2)

{
                    /* 0x1fb0  214   */
  return FALSE;
}

int __cdecl
RETIRED_SHSetIniStringUTF7W(wchar_t *param_1,wchar_t *param_2,wchar_t *param_3,wchar_t *param_4)

{
                    /* 0x2630  172   */
  return FALSE;
}

int __cdecl RETIRED_SHSetShellWindowEx(HWND param_1,HWND param_2)

{
                    /* 0x34a0  374   */
  return FALSE;
}

int __cdecl RETIRED_SHSetThreadPoolLimits(PSH_THREAD_POOL_LIMITS param_1)

{
                    /* 0x25a0  163   */
  return FALSE;
}

HRESULT __cdecl RETIRED_SHSetUserPicturePathW(wchar_t *param_1,unsigned long param_2,wchar_t *param_3)

{
                    /* 0x1e70  59   */
  return E_NOTIMPL;
}

void __cdecl
RETIRED_Briefcase_CreateA(HWND param_1,HINSTANCE param_2,char *param_3,int param_4)

{
                    /* 0x2700  184   */
  return;
}

void __cdecl
RETIRED_Briefcase_CreateW(HWND param_1,HINSTANCE param_2,wchar_t *param_3,int param_4)

{
                    /* 0x2710  185   */
  return;
}

HRESULT __cdecl
RETIRED_CDefFolderMenu_Create
          (ITEMIDLIST *param_1,HWND param_2,unsigned int param_3,ITEMIDLIST **param_4,
          IShellFolder *param_5,
          HRESULT (__cdecl *param_6)(IShellFolder *, HWND *, IDataObject *, unsigned int, unsigned int, long),HKEY param_7,HKEY param_8,IContextMenu **param_9)

{
                    /* 0x3500  380   */
  *param_9 = (IContextMenu *)0x0;
  return E_NOTIMPL;
}

ITEMIDLIST * __cdecl RETIRED_Channel_GetFolderPidl(void)

{
                    /* 0x2210  114   */
  return (ITEMIDLIST *)0x0;
}

HRESULT __cdecl RETIRED_Channel_QuickLaunch(void)

{
                    /* 0x2250  118   */
  return E_NOTIMPL;
}

IDeskBand * __cdecl RETIRED_ChannelBand_Create(ITEMIDLIST *param_1)

{
                    /* 0x2220  115   */
  return (IDeskBand *)0x0;
}

void __cdecl RETIRED_Channels_SetBandInfoSFB(IUnknown *param_1)

{
                    /* 0x2230  116   */
  return;
}

HRESULT __cdecl RETIRED_CheckThemeSignature(wchar_t *param_1)

{
                    /* 0x2780  213   */
  return E_NOTIMPL;
}

HRESULT __cdecl RETIRED_ClearAutoSuggestForForms(unsigned long param_1)

{
                    /* 0x2850  201   */
  return E_NOTIMPL;
}

HRESULT __cdecl RETIRED_ClearExtensionRecordsBlockReason(void)

{
                    /* 0x28b0  207   */
  return E_NOTIMPL;
}

void __cdecl RETIRED_ClearStartMenuItem(void *param_1)

{
                    /* 0x3bc0  475   */
  return;
}

HRESULT __cdecl RETIRED_CloseThemeFile(void *param_1)

{
                    /* 0x3950  441   */
  return E_NOTIMPL;
}

HRESULT __cdecl
RETIRED_CopyStreamUI(IStream *param_1,IStream *param_2,IProgressDialog *param_3,unsigned long long param_4)

{
                    /* 0x1f80  75   */
  return E_NOTIMPL;
}

HRESULT __cdecl
RETIRED_CPL_CreateCondition(wchar_t *param_1,IXMLDOMNode *param_2,GUID *param_3,void **param_4)

{
                    /* 0x3430  426   */
  *param_4 = (void *)0x0;
  return E_NOTIMPL;
}

HRESULT __cdecl RETIRED_CPL_ExecuteTask(HWND param_1,wchar_t *param_2)

{
                    /* 0x3420  423   */
  return E_NOTIMPL;
}

HRESULT __cdecl RETIRED_Create_IUIElement(WVTASKITEM *param_1,IUIElement **param_2)

{
                    /* 0x3530  382   */
  *param_2 = (IUIElement *)0x0;
  return E_NOTIMPL;
}

int __cdecl RETIRED_CreateFromDesktop(NEWFOLDERINFO *param_1)

{
                    /* 0x2260  119   */
  return FALSE;
}

HWND __cdecl RETIRED_CreatePage(void *param_1,HWND param_2)

{
                    /* 0x2160  104   */
  return (HWND)0x0;
}

HRESULT __cdecl RETIRED_CreateSingleVisibleInList(wchar_t *param_1,GUID *param_2,void **param_3)

{
                    /* 0x2d10  259   */
  if (param_3 != (void **)0x0) {
    *param_3 = (void *)0x0;
  }
  return E_UNEXPECTED;
}

HRESULT __cdecl RETIRED_CreateURLFileContentsA(char *param_1,char **param_2)

{
                    /* 0x3200  315   */
  *param_2 = (char *)0x0;
  return E_NOTIMPL;
}

HRESULT __cdecl RETIRED_CreateURLFileContentsW(wchar_t *param_1,char **param_2)

{
                    /* 0x31e0  314   */
  *param_2 = (char *)0x0;
  return E_NOTIMPL;
}

HRESULT __cdecl
RETIRED_CreateVisibleInDescription(wchar_t *param_1,GUID *param_2,void **param_3)

{
                    /* 0x2d90  263   */
  if (param_3 != (void **)0x0) {
    *param_3 = (void *)0x0;
  }
  return E_UNEXPECTED;
}

HRESULT __cdecl RETIRED_CreateVisibleInList(IUnknown *param_1,GUID *param_2,void **param_3)

{
                    /* 0x2db0  264   */
  if (param_3 != (void **)0x0) {
    *param_3 = (void *)0x0;
  }
  return E_UNEXPECTED;
}

HRESULT __cdecl
RETIRED_CShellUIHelper_CreateInstance2
          (IUnknown **param_1,GUID *param_2,IUnknown *param_3,IDispatch *param_4)

{
                    /* 0x27e0  195   */
  *param_1 = (IUnknown *)0x0;
  return E_NOTIMPL;
}

int __cdecl RETIRED_DAD_DragEnter(HWND param_1)

{
                    /* 0x1db0  48   */
  return FALSE;
}

int __cdecl RETIRED_DAD_SetDragImageFromListView(HWND param_1,tagPOINT param_2)

{
                    /* 0x1e20  54   */
  return FALSE;
}

void * __cdecl RETIRED_DDECreatePostNotify(LPNMVIEWFOLDER param_1)

{
                    /* 0x1d10  39   */
  return (void *)0x0;
}

int __cdecl
RETIRED_DDEHandleViewFolderNotify(IShellBrowser *param_1,HWND param_2,LPNMVIEWFOLDER param_3)

{
                    /* 0x1e50  57   */
  return FALSE;
}

_func___cdecl_int * __cdecl RETIRED_DelayLoadFailureHook(unsigned int param_1,DelayLoadInfo *param_2)

{
                    /* 0x2640  173   */
  return (_func___cdecl_int *)0x0;
}

HRESULT __cdecl RETIRED_DeleteFileThumbnail(wchar_t *param_1)

{
                    /* 0x1fa0  77   */
  return E_NOTIMPL;
}

void __cdecl RETIRED_DetectAndFixAssociations(void)

{
                    /* 0x2830  199   */
  return;
}

HRESULT __cdecl RETIRED_DisplayUrlParseError(HWND param_1,HRESULT param_2,wchar_t *param_3)

{
                    /* 0x30e0  303   */
  return E_NOTIMPL;
}

HRESULT __cdecl
RETIRED_DragDrop(HWND param_1,IShellFolder *param_2,ITEMIDLIST *param_3,unsigned long param_4,
                unsigned long *param_5)

{
                    /* 0x2f80  287   */
  if (param_5 != (unsigned long *)0x0) {
    *param_5 = 0;
  }
  return E_NOTIMPL;
}

HRESULT __cdecl
RETIRED_DrawNCPreview
          (HDC param_1,unsigned long param_2,tagRECT *param_3,wchar_t *param_4,wchar_t *param_5,
          wchar_t *param_6,tagNONCLIENTMETRICSW *param_7,unsigned long *param_8)

{
                    /* 0x3960  444   */
  return E_NOTIMPL;
}

int __cdecl RETIRED_DriveTypeExtendedFlags(int param_1)

{
                    /* 0x2aa0  234   */
  return FALSE;
}

void __cdecl RETIRED_EnsureWebViewRegSettings(void)

{
                    /* 0x2840  200   */
  return;
}

void __cdecl RETIRED_EventTraceHandler(unsigned char param_1,void *param_2)

{
                    /* 0x2540  157   */
  return;
}

void __cdecl RETIRED_FileMenu_AbortInitMenu(void)

{
                    /* 0x1b90  17   */
  return;
}

unsigned int __cdecl RETIRED_FileMenu_AppendFilesForPidl(HMENU param_1,ITEMIDLIST *param_2,int param_3)

{
                    /* 0x1ba0  18   */
  return FALSE;
}

int __cdecl
RETIRED_FileMenu_AppendItem
          (HMENU param_1,wchar_t *param_2,unsigned int param_3,int param_4,HMENU param_5,unsigned int param_6)

{
                    /* 0x1b50  13   */
  return FALSE;
}

HRESULT __cdecl RETIRED_FileMenu_ComposeA(HMENU param_1,unsigned int param_2,struct FMCOMPOSEA *param_3)

{
                    /* 0x1b00  8   */
  return E_FAIL;
}

HRESULT __cdecl RETIRED_FileMenu_ComposeW(HMENU param_1,unsigned int param_2,struct FMCOMPOSEW *param_3)

{
                    /* 0x1b30  11   */
  return E_FAIL;
}

HMENU __cdecl
RETIRED_FileMenu_Create(unsigned long param_1,int param_2,HBITMAP param_3,int param_4,unsigned long param_5)

{
                    /* 0x1b40  12   */
  return (HMENU)0x0;
}

int __cdecl
RETIRED_FileMenu_CreateFromMenu
          (HMENU param_1,unsigned long param_2,int param_3,HBITMAP param_4,int param_5,unsigned long param_6)

{
                    /* 0x1ca0  34   */
  return FALSE;
}

void __cdecl RETIRED_FileMenu_DelayedInvalidate(HMENU param_1)

{
                    /* 0x1c80  32   */
  return;
}

unsigned int __cdecl RETIRED_FileMenu_DeleteAllItems(HMENU param_1)

{
                    /* 0x1aa0  2   */
  return FALSE;
}

int __cdecl RETIRED_FileMenu_DeleteItemByCmd(HMENU param_1,unsigned int param_2)

{
                    /* 0x1b70  15   */
  return FALSE;
}

int __cdecl RETIRED_FileMenu_DeleteItemByIndex(HMENU param_1,unsigned int param_2)

{
                    /* 0x1bb0  19   */
  return FALSE;
}

int __cdecl RETIRED_FileMenu_DeleteMenuItemByFirstID(HMENU param_1,unsigned int param_2)

{
                    /* 0x1bc0  20   */
  return FALSE;
}

int __cdecl RETIRED_FileMenu_DeleteSeparator(HMENU param_1)

{
                    /* 0x1bd0  21   */
  return FALSE;
}

void __cdecl RETIRED_FileMenu_Destroy(HMENU param_1)

{
                    /* 0x1b80  16   */
  return;
}

long __cdecl RETIRED_FileMenu_DrawItem(HWND param_1,tagDRAWITEMSTRUCT *param_2)

{
                    /* 0x1ab0  3   */
  return FALSE;
}

void __cdecl RETIRED_FileMenu_EditMode(int param_1)

{
                    /* 0x1c50  29   */
  return;
}

int __cdecl RETIRED_FileMenu_EnableItemByCmd(HMENU param_1,unsigned int param_2,int param_3)

{
                    /* 0x1be0  22   */
  return FALSE;
}

HMENU __cdecl RETIRED_FileMenu_FindSubMenuByPidl(HMENU param_1,_ITEMIDLIST *param_2)

{
                    /* 0x1ac0  4   */
  return (HMENU)0x0;
}

unsigned long __cdecl RETIRED_FileMenu_GetItemExtent(HMENU param_1,unsigned int param_2)

{
                    /* 0x1bf0  23   */
  return FALSE;
}

int __cdecl
RETIRED_FileMenu_GetLastSelectedItemPidls
          (HMENU param_1,ITEMIDLIST **param_2,ITEMIDLIST **param_3)

{
                    /* 0x1ad0  5   */
  return FALSE;
}

int __cdecl RETIRED_FileMenu_GetPidl(HMENU param_1,unsigned int param_2,ITEMIDLIST **param_3)

{
                    /* 0x1c40  28   */
  return FALSE;
}

long __cdecl RETIRED_FileMenu_HandleMenuChar(HMENU param_1,wchar_t param_2)

{
                    /* 0x1ae0  6   */
  return FALSE;
}

int __cdecl RETIRED_FileMenu_HandleMenuSelect(HMENU param_1,unsigned int param_2,long param_3)

{
                    /* 0x1c60  30   */
  return FALSE;
}

int __cdecl RETIRED_FileMenu_HandleNotify(HMENU param_1,ITEMIDLIST **param_2,long param_3)

{
                    /* 0x1a90  1   */
  return FALSE;
}

int __cdecl RETIRED_FileMenu_InitMenuPopup(HMENU param_1)

{
                    /* 0x1af0  7   */
  return FALSE;
}

int __cdecl
RETIRED_FileMenu_InsertItem
          (HMENU param_1,wchar_t *param_2,unsigned int param_3,int param_4,HMENU param_5,unsigned int param_6,
          unsigned int param_7)

{
                    /* 0x1c20  26   */
  return FALSE;
}

int __cdecl RETIRED_FileMenu_InsertSeparator(HMENU param_1,unsigned int param_2)

{
                    /* 0x1c30  27   */
  return FALSE;
}

void __cdecl RETIRED_FileMenu_Invalidate(HMENU param_1)

{
                    /* 0x1b10  9   */
  return;
}

int __cdecl RETIRED_FileMenu_IsDelayedInvalid(HMENU param_1)

{
                    /* 0x1c90  33   */
  return FALSE;
}

int __cdecl RETIRED_FileMenu_IsFileMenu(HMENU param_1)

{
                    /* 0x1c00  24   */
  return FALSE;
}

int __cdecl RETIRED_FileMenu_IsUnexpanded(HMENU param_1)

{
                    /* 0x1c70  31   */
  return FALSE;
}

long __cdecl RETIRED_FileMenu_MeasureItem(HWND param_1,tagMEASUREITEMSTRUCT *param_2)

{
                    /* 0x1b20  10   */
  return FALSE;
}

int __cdecl
RETIRED_FileMenu_ProcessCommand
          (HWND param_1,HMENU param_2,unsigned int param_3,HMENU param_4,unsigned int param_5)

{
                    /* 0x1c10  25   */
  return FALSE;
}

int __cdecl
RETIRED_FileMenu_TrackPopupMenuEx
          (HMENU param_1,unsigned int param_2,int param_3,int param_4,HWND param_5,
          tagTPMPARAMS *param_6)

{
                    /* 0x1b60  14   */
  return FALSE;
}

void __cdecl RETIRED_FireEvent_Quit(IExpDispSupport *param_1)

{
                    /* 0x30b0  300   */
  return;
}

HRESULT __cdecl RETIRED_FirstUserLogon(char *param_1,char *param_2)

{
                    /* 0x3480  372   */
  return E_NOTIMPL;
}

int __cdecl RETIRED_Free(void *param_1)

{
                    /* 0x2190  107   */
  return FALSE;
}

int __cdecl RETIRED_GetDefaultInternetSearchUrlA(char *param_1,int param_2,int param_3)

{
                    /* 0x2400  137   */
  if (param_2 != 0) {
    *param_1 = '\0';
  }
  return FALSE;
}

int __cdecl RETIRED_GetDefaultInternetSearchUrlW(wchar_t *param_1,int param_2,int param_3)

{
                    /* 0x23e0  136   */
  *param_1 = L'\0';
  return FALSE;
}

HRESULT __cdecl
RETIRED_GetDefaultLocationKindsForExtension(wchar_t *param_1,wchar_t *param_2,wchar_t **param_3)

{
                    /* 0x3860  427   */
  if (param_3 != (wchar_t **)0x0) {
    *param_3 = (wchar_t *)0x0;
  }
  return E_NOTIMPL;
}

HRESULT __cdecl RETIRED_GetExtensionRecordBlockReason(GUID *param_1)

{
                    /* 0x28a0  206   */
  return E_NOTIMPL;
}

HRESULT __cdecl RETIRED_GetExtensionRecords(GUID **param_1,int *param_2)

{
                    /* 0x2890  205   */
  return E_NOTIMPL;
}

_FILEDESCRIPTORW * __cdecl
RETIRED_GetFileDescriptor(_FILEGROUPDESCRIPTORW *param_1,int param_2,int param_3,wchar_t *param_4)

{
                    /* 0x1f60  74   */
  if (param_4 != (wchar_t *)0x0) {
    *param_4 = L'\0';
  }
  return param_1->fgd;
}

int __cdecl
RETIRED_GetInfoTip(IShellFolder *param_1,ITEMIDLIST *param_2,wchar_t *param_3,int param_4)

{
                    /* 0x2ec0  276   */
  return FALSE;
}

int __cdecl RETIRED_GetLeakDetectionFunctionTable(void *param_1)

{
                    /* 0x2370  131   */
  return FALSE;
}

HRESULT __cdecl
RETIRED_GetLinkInfo(IShellFolder *param_1,ITEMIDLIST *param_2,int *param_3,int *param_4)

{
                    /* 0x2990  221   */
  if (param_3 != (int *)0x0) {
    *param_3 = 1;
  }
  if (param_4 != (int *)0x0) {
    *param_4 = 0;
  }
  return E_NOTIMPL;
}

int __cdecl RETIRED_GetMIMETypeSubKeyA(char *param_1,char *param_2,unsigned int param_3)

{
                    /* 0x3af0  462   */
  return FALSE;
}

int __cdecl RETIRED_GetMIMETypeSubKeyW(wchar_t *param_1,wchar_t *param_2,unsigned int param_3)

{
                    /* 0x3b00  463   */
  return FALSE;
}

int __cdecl RETIRED_GetNewChangeNumber(void *param_1)

{
                    /* 0x2770  215   */
  return FALSE;
}

unsigned long __cdecl RETIRED_GetPerfTime(void)

{
                    /* 0x24f0  152   */
  return FALSE;
}

int __cdecl RETIRED_GetSearchAssistantUrlA(wchar_t *param_1,int param_2,int param_3,int param_4)

{
                    /* 0x23c0  135   */
  if (param_2 != 0) {
    *param_1 = L'\0';
  }
  return FALSE;
}

int __cdecl RETIRED_GetSearchAssistantUrlW(wchar_t *param_1,int param_2,int param_3,int param_4)

{
                    /* 0x23a0  134   */
  *param_1 = L'\0';
  return FALSE;
}

PISECURITY_DESCRIPTOR __cdecl
RETIRED_GetShellSecurityDescriptor(PSHELL_USER_PERMISSION *param_1,int param_2)

{
                    /* 0x2920  211   */
  return (PISECURITY_DESCRIPTOR)0x0;
}

unsigned long __cdecl RETIRED_GetSize(void *param_1)

{
                    /* 0x21a0  108   */
  return FALSE;
}

int __cdecl RETIRED_GetSqmableFileName(wchar_t *param_1,wchar_t *param_2,unsigned long param_3)

{
                    /* 0x3bd0  476   */
  *param_2 = L'\0';
  return FALSE;
}

HRESULT __cdecl
RETIRED_GetSqmableIdentifiersForApp(wchar_t *param_1,wchar_t **param_2,wchar_t **param_3)

{
                    /* 0x3bf0  477   */
  *param_2 = (wchar_t *)0x0;
  *param_3 = (wchar_t *)0x0;
  return E_UNEXPECTED;
}

HRESULT __cdecl RETIRED_GetTemplateInfoFromHandle(void *param_1,unsigned char *param_2,unsigned long *param_3)

{
                    /* 0x2650  174   */
  return E_NOTIMPL;
}

HRESULT __cdecl RETIRED_GetThemeSysFont96(void *param_1,int param_2,tagLOGFONTW *param_3)

{
                    /* 0x2740  188   */
  return E_NOTIMPL;
}

int __cdecl RETIRED_GetThemeSysSize96(void *param_1,int param_2)

{
                    /* 0x2750  189   */
  return FALSE;
}

HRESULT __cdecl
RETIRED_GetTryHarderIDList
          (IFolderView2 *param_1,ITEMIDLIST *param_2,_GUID param_3,wchar_t *param_4,int param_5,
          ITEMIDLIST **param_6)

{
                    /* 0x2d70  262   */
  if (param_6 != (ITEMIDLIST **)0x0) {
    *param_6 = (ITEMIDLIST *)0x0;
  }
  return E_UNEXPECTED;
}

int __cdecl RETIRED_GetUserNameWrapW(wchar_t *param_1,unsigned long *param_2)

{
                    /* 0x3b20  465   */
  return FALSE;
}

HRESULT __cdecl
RETIRED_IContextMenu_Invoke(IContextMenu *param_1,HWND param_2,char *param_3,unsigned int param_4)

{
                    /* 0x3a60  453   */
  return E_NOTIMPL;
}

unsigned long __cdecl RETIRED_IDataObject_GetDeskBandState(IDataObject *param_1)

{
                    /* 0x2e40  270   */
  return FALSE;
}

HRESULT __cdecl RETIRED_IEBindToObject(ITEMIDLIST *param_1,IShellFolder **param_2)

{
                    /* 0x22e0  126   */
  *param_2 = (IShellFolder *)0x0;
  return E_NOTIMPL;
}

HRESULT __cdecl
RETIRED_IEBindToObjectEx(ITEMIDLIST *param_1,IBindCtx *param_2,GUID *param_3,void **param_4)

{
                    /* 0x2340  129   */
  *param_4 = (void *)0x0;
  return E_NOTIMPL;
}

void __cdecl RETIRED_IECleanUpAutomationObject(void)

{
                    /* 0x2a60  229   */
  return;
}

HRESULT __cdecl
RETIRED_IECreateFromPathCPWithBCA
          (unsigned int param_1,char *param_2,IBindCtx *param_3,ITEMIDLIST **param_4)

{
                    /* 0x3150  308   */
  *param_4 = (_ITEMIDLIST *)0x0;
  return E_NOTIMPL;
}

HRESULT __cdecl
RETIRED_IECreateFromPathCPWithBCW
          (unsigned int param_1,wchar_t *param_2,IBindCtx *param_3,ITEMIDLIST **param_4)

{
                    /* 0x3170  309   */
  *param_4 = (_ITEMIDLIST *)0x0;
  return E_NOTIMPL;
}

void __cdecl RETIRED_IEDDE_AutomationStarted(void)

{
                    /* 0x2820  198   */
  return;
}

int __cdecl RETIRED_IEDDE_NewWindow(HWND param_1)

{
                    /* 0x2810  197   */
  return FALSE;
}

int __cdecl RETIRED_IEDDE_WindowDestroyed(HWND param_1)

{
                    /* 0x2800  196   */
  return FALSE;
}

HRESULT __cdecl RETIRED_IEGetAttributesOf(ITEMIDLIST *param_1,unsigned long *param_2)

{
                    /* 0x22d0  125   */
  return E_NOTIMPL;
}

HRESULT __cdecl RETIRED_IEGetDisplayName(ITEMIDLIST *param_1,wchar_t *param_2,unsigned int param_3)

{
                    /* 0x2320  128   */
  if (param_2 != (wchar_t *)0x0) {
    *param_2 = L'\0';
  }
  return E_NOTIMPL;
}

HRESULT __cdecl RETIRED_IEGetDisplayNameW(ITEMIDLIST *param_1,wchar_t *param_2,unsigned int param_3)

{
                    /* 0x2a80  231   */
  if (param_2 != (wchar_t *)0x0) {
    *param_2 = L'\0';
  }
  return E_NOTIMPL;
}

HRESULT __cdecl
RETIRED_IEGetNameAndFlags
          (ITEMIDLIST *param_1,unsigned int param_2,wchar_t *param_3,unsigned long param_4,unsigned long *param_5)

{
                    /* 0x2300  127   */
  if (param_3 != (wchar_t *)0x0) {
    *param_3 = L'\0';
  }
  return E_NOTIMPL;
}

int __cdecl RETIRED_IEILIsEqual(ITEMIDLIST *param_1,ITEMIDLIST *param_2,int param_3)

{
                    /* 0x2420  138   */
  return TRUE;
}

int __cdecl RETIRED_IEIsLinkSafe(HWND param_1,ITEMIDLIST *param_2,ILS_ACTION param_3)

{
                    /* 0x31b0  311   */
  return FALSE;
}

void __cdecl RETIRED_IEOnFirstBrowserCreation(IUnknown *param_1)

{
                    /* 0x30d0  302   */
  return;
}

HRESULT __cdecl
RETIRED_IEParseDisplayNameWithBCW
          (unsigned int param_1,wchar_t *param_2,IBindCtx *param_3,ITEMIDLIST **param_4)

{
                    /* 0x3130  307   */
  *param_4 = (ITEMIDLIST *)0x0;
  return E_NOTIMPL;
}

int __cdecl RETIRED_IEWinMain(wchar_t *param_1,int param_2)

{
                    /* 0x27d0  194   */
  return FALSE;
}

int __cdecl
RETIRED_ILGetPseudoNameW(ITEMIDLIST *param_1,ITEMIDLIST *param_2,wchar_t *param_3,int param_4)

{
                    /* 0x2ab0  235   */
  if (param_3 != (wchar_t *)0x0) {
    *param_3 = L'\0';
  }
  return FALSE;
}

int __cdecl RETIRED_ILIsWeb(ITEMIDLIST *param_1)

{
                    /* 0x2fa0  288   */
  return FALSE;
}

HRESULT __cdecl RETIRED_ILLoadFromStream(IStream *param_1,ITEMIDLIST **param_2)

{
                    /* 0x1cc0  36   */
  return E_NOTIMPL;
}

void __cdecl RETIRED_InitializeStartMenuItem(void *param_1)

{
                    /* 0x3bb0  474   */
  return;
}

int __cdecl RETIRED_InitiateInteractiveLogon(wchar_t *param_1,wchar_t *param_2,unsigned long param_3)

{
                    /* 0x2730  187   */
  return FALSE;
}

int __cdecl RETIRED_InitOCHostClass(SHDRC *param_1)

{
                    /* 0x21b0  109   */
  return FALSE;
}

HRESULT __cdecl RETIRED_InitUserTheme(void **param_1,void **param_2,int param_3,int param_4)

{
                    /* 0x3920  438   */
  return E_NOTIMPL;
}

int __cdecl RETIRED_IntlStrEqWorkerA(int param_1,char *param_2,char *param_3,int param_4)

{
                    /* 0x2140  102   */
  return TRUE;
}

int __cdecl RETIRED_IntlStrEqWorkerW(int param_1,wchar_t *param_2,wchar_t *param_3,int param_4)

{
                    /* 0x2150  103   */
  return TRUE;
}

int __cdecl RETIRED_IsErrorUrl(wchar_t *param_1)

{
                    /* 0x29c0  222   */
  return FALSE;
}

int __cdecl RETIRED_IsFileUrl(char *param_1)

{
                    /* 0x2f40  283   */
  return FALSE;
}

int __cdecl RETIRED_IsFileUrlW(wchar_t *param_1)

{
                    /* 0x2f50  284   */
  return FALSE;
}

HRESULT __cdecl
RETIRED_IShellFolder_CompareIDs
          (IShellFolder *param_1,long param_2,ITEMIDLIST *param_3,ITEMIDLIST *param_4)

{
                    /* 0x2660  175   */
  return E_NOTIMPL;
}

HRESULT __cdecl
RETIRED_IShellFolder_EnumObjects
          (IShellFolder *param_1,HWND param_2,unsigned long param_3,IEnumIDList **param_4)

{
                    /* 0x31c0  313   */
  *param_4 = (IEnumIDList *)0x0;
  return E_NOTIMPL;
}

HRESULT __cdecl
RETIRED_IShellFolder_ParseDisplayNameC
          (IShellFolder *param_1,HWND param_2,IBindCtx *param_3,wchar_t *param_4,unsigned long *param_5,
          ITEMIDLIST **param_6,unsigned long *param_7)

{
                    /* 0x3360  349   */
  return E_NOTIMPL;
}

int __cdecl RETIRED_IsResetWebSettingsRequired(void)

{
                    /* 0x2870  203   */
  return FALSE;
}

int __cdecl RETIRED_IsSearchEnabled(void)

{
                    /* 0x2df0  473   */
  return TRUE;
}

HRESULT __cdecl RETIRED_IsShellItemInSearchIndex(IShellItem *param_1,int *param_2)

{
                    /* 0x2d30  260   */
  if (param_2 != (int *)0x0) {
    *param_2 = 0;
  }
  return E_UNEXPECTED;
}

int __cdecl RETIRED_IsSuspendAllowed(void)

{
                    /* 0x35e0  388   */
  return FALSE;
}

int __cdecl RETIRED_IsURLChild(ITEMIDLIST *param_1,int param_2)

{
                    /* 0x2fb0  289   */
  return FALSE;
}

HRESULT __cdecl RETIRED_IUnknown_OnFocusOCS(IUnknown *param_1,int param_2)

{
                    /* 0x3a50  452   */
  return E_NOTIMPL;
}

HRESULT __cdecl
RETIRED_IUnknown_ProfferServiceOld
          (IUnknown *param_1,GUID *param_2,GUID *param_3,IServiceProvider *param_4,unsigned long *param_5)

{
                    /* 0x3330  346   */
  return E_NOTIMPL;
}

HRESULT __cdecl
RETIRED_IUnknown_QueryServiceQueryStatus
          (IUnknown *param_1,GUID *param_2,GUID *param_3,unsigned long param_4,_tagOLECMD *param_5,
          _tagOLECMDTEXT *param_6)

{
                    /* 0x3380  351   */
  return E_NOTIMPL;
}

HRESULT __cdecl RETIRED_IUnknown_SetBandInfoSFB(IUnknown *param_1,PBANDINFOSFB param_2)

{
                    /* 0x2240  117   */
  return E_NOTIMPL;
}

HRESULT __cdecl
RETIRED_IUnknown_TranslateAcceleratorOCS(IUnknown *param_1,tagMSG *param_2,unsigned long param_3)

{
                    /* 0x3a40  451   */
  return E_NOTIMPL;
}

HRESULT __cdecl
RETIRED_IURLQualify(wchar_t *param_1,unsigned long param_2,wchar_t *param_3,int *param_4,int *param_5)

{
                    /* 0x3010  293   */
  return E_NOTIMPL;
}

HRESULT __cdecl
RETIRED_IWebBrowser_Navigate
          (IUnknown *param_1,wchar_t *param_2,tagVARIANT *param_3,tagVARIANT *param_4,
          tagVARIANT *param_5,tagVARIANT *param_6)

{
                    /* 0x3370  350   */
  return E_NOTIMPL;
}

HRESULT __cdecl
RETIRED_JITCoCreateInstance
          (GUID *param_1,IUnknown *param_2,unsigned long param_3,GUID *param_4,void **param_5,
          HWND param_6,unsigned long param_7)

{
                    /* 0x2380  132   */
  return E_NOTIMPL;
}

HRESULT __cdecl RETIRED_LoadTheme(void *param_1)

{
                    /* 0x38c0  432   */
  return E_NOTIMPL;
}

HRESULT __cdecl RETIRED_LoadUserTheme(void *param_1,void **param_2,void **param_3)

{
                    /* 0x3930  439   */
  return E_NOTIMPL;
}

int __cdecl RETIRED_MakeShellURLFromPathA(char *param_1,char *param_2,unsigned long param_3)

{
                    /* 0x1f40  72   */
  return FALSE;
}

int __cdecl RETIRED_MakeShellURLFromPathW(wchar_t *param_1,wchar_t *param_2,unsigned long param_3)

{
                    /* 0x1f50  73   */
  return FALSE;
}

int __cdecl RETIRED_MIME_GetExtensionA(char *param_1,char *param_2,unsigned int param_3)

{
                    /* 0x3b10  464   */
  return FALSE;
}

HRESULT __cdecl
RETIRED_MLBuildResURLA
          (char *param_1,HINSTANCE param_2,unsigned long param_3,char *param_4,char *param_5,int param_6)

{
                    /* 0x3b30  466   */
  return E_NOTIMPL;
}

HRESULT __cdecl
RETIRED_MLBuildResURLW
          (wchar_t *param_1,HINSTANCE param_2,unsigned long param_3,wchar_t *param_4,wchar_t *param_5,
          int param_6)

{
                    /* 0x3b40  467   */
  return E_NOTIMPL;
}

HRESULT __cdecl RETIRED_MLClearMLHInstance(HINSTANCE param_1)

{
                    /* 0x32c0  333   */
  return E_NOTIMPL;
}

int __cdecl RETIRED_MLFreeLibrary(HINSTANCE param_1)

{
                    /* 0x3b50  468   */
  return FALSE;
}

HWND __cdecl
RETIRED_MLHtmlHelpA(HWND param_1,char *param_2,unsigned int param_3,unsigned long param_4,unsigned long param_5)

{
                    /* 0x3240  320   */
  return (HWND)0x0;
}

HWND __cdecl
RETIRED_MLHtmlHelpW(HWND param_1,wchar_t *param_2,unsigned int param_3,unsigned long param_4,unsigned long param_5)

{
                    /* 0x3250  321   */
  return (HWND)0x0;
}

int __cdecl RETIRED_MLIsMLHInstance(HINSTANCE param_1)

{
                    /* 0x32a0  331   */
  return FALSE;
}

HRESULT __cdecl RETIRED_MLSetMLHInstance(HINSTANCE param_1,unsigned short param_2)

{
                    /* 0x32b0  332   */
  return E_NOTIMPL;
}

HRESULT __cdecl RETIRED_NavToUrlUsingIEA(char *param_1,int param_2)

{
                    /* 0x2970  219   */
  return E_NOTIMPL;
}

HRESULT __cdecl RETIRED_NavToUrlUsingIEW(wchar_t *param_1,int param_2)

{
                    /* 0x2980  220   */
  return E_NOTIMPL;
}

int __cdecl RETIRED_NTSHChangeNotifyDeregister(unsigned long param_1)

{
                    /* 0x1ed0  65   */
  return FALSE;
}

unsigned long __cdecl
RETIRED_NTSHChangeNotifyRegister
          (HWND param_1,int param_2,long param_3,unsigned int param_4,int param_5,
          _SHChangeNotifyEntry *param_6)

{
                    /* 0x1ec0  64   */
  return FALSE;
}

int __cdecl RETIRED_OleStrToStrN(wchar_t *param_1,int param_2,wchar_t *param_3,int param_4)

{
                    /* 0x1cd0  37   */
  *param_1 = L'\0';
  return FALSE;
}

IStream * __cdecl
RETIRED_OpenPidlOrderStream(ITEMIDLIST *param_1,ITEMIDLIST *param_2,char *param_3,unsigned long param_4)

{
                    /* 0x2f70  286   */
  return (IStream *)0x0;
}

HRESULT __cdecl
RETIRED_OpenThemeFile(wchar_t *param_1,wchar_t *param_2,wchar_t *param_3,void **param_4,int param_5)

{
                    /* 0x3940  440   */
  return E_NOTIMPL;
}

int __cdecl RETIRED_PackWizRunFromExe(void)

{
                    /* 0x2720  186   */
  return FALSE;
}

HRESULT __cdecl
RETIRED_ParseThemeIniFile(wchar_t *param_1,unsigned long param_2,int (__cdecl *param_3)(void),long param_4)

{
                    /* 0x2760  190   */
  return E_NOTIMPL;
}

int __cdecl
RETIRED_ParseURLFromOutsideSourceA(char *param_1,char *param_2,unsigned long *param_3,int *param_4)

{
                    /* 0x2fc0  290   */
  *param_3 = 0;
  return FALSE;
}

int __cdecl
RETIRED_ParseURLFromOutsideSourceW(wchar_t *param_1,wchar_t *param_2,unsigned long *param_3,int *param_4)

{
                    /* 0x2fe0  291   */
  *param_3 = 0;
  return FALSE;
}

wchar_t * __cdecl RETIRED_PathGetExtension(wchar_t *param_1,wchar_t *param_2,int param_3)

{
                    /* 0x35f0  389   */
  return (wchar_t *)0x0;
}

HRESULT __cdecl RETIRED_PathGetPathDisplayNameAlloc(wchar_t *param_1,wchar_t **param_2)

{
                    /* 0x3570  384   */
  *param_2 = (wchar_t *)0x0;
  return E_NOTIMPL;
}

int __cdecl RETIRED_PathIsFilePath(wchar_t *param_1)

{
                    /* 0x2f60  285   */
  return FALSE;
}

long __cdecl RETIRED_PathProcessCommand(wchar_t *param_1,wchar_t *param_2,int param_3,unsigned long param_4)

{
                    /* 0x1f00  68   */
  return -1;
}

HRESULT __cdecl
RETIRED_POOBE_CreateIndirectGraphic(wchar_t *param_1,tagSIZE param_2,HBITMAP *param_3)

{
                    /* 0x3450  392   */
  *param_3 = (HBITMAP)0x0;
  return E_NOTIMPL;
}

HRESULT __cdecl
RETIRED_PrepareURLForDisplayUTF8W(wchar_t *param_1,wchar_t *param_2,unsigned long *param_3,int param_4)

{
                    /* 0x3190  310   */
  *param_3 = 0;
  return E_NOTIMPL;
}

void __cdecl RETIRED_PrettifyFileDescriptionW(wchar_t *param_1,wchar_t *param_2)

{
                    /* 0x33f0  357   */
  return;
}

void * __cdecl RETIRED_ReAlloc(void *param_1,long param_2)

{
                    /* 0x2180  106   */
  return (void *)0x0;
}

HRESULT __cdecl RETIRED_RecordExtensionCreation(_GUID *param_1,HRESULT param_2)

{
                    /* 0x2880  204   */
  return E_NOTIMPL;
}

long __cdecl RETIRED_RegCreateKeyWrapW(HKEY param_1,wchar_t *param_2,HKEY *param_3)

{
                    /* 0x3a10  448   */
  return ERROR_CALL_NOT_IMPLEMENTED;
}

long __cdecl RETIRED_RegEnumKeyWrapW(HKEY param_1,unsigned long param_2,wchar_t *param_3,unsigned long param_4)

{
                    /* 0x3a20  449   */
  return ERROR_CALL_NOT_IMPLEMENTED;
}

HRESULT __cdecl RETIRED_RegisterDefaultAcceptHeaders(IBindCtx *param_1,IShellBrowser *param_2)

{
                    /* 0x3890  429   */
  return E_NOTIMPL;
}

int __cdecl RETIRED_RegisterExtensionForMIMETypeA(char *param_1,char *param_2)

{
                    /* 0x3ab0  458   */
  return FALSE;
}

int __cdecl RETIRED_RegisterExtensionForMIMETypeW(wchar_t *param_1,wchar_t *param_2)

{
                    /* 0x3ac0  459   */
  return FALSE;
}

HRESULT __cdecl RETIRED_RegisterGlobalHotkeyA(unsigned short param_1,unsigned short param_2,char *param_3)

{
                    /* 0x3230  317   */
  return E_NOTIMPL;
}

HRESULT __cdecl RETIRED_RegisterGlobalHotkeyW(unsigned short param_1,unsigned short param_2,wchar_t *param_3)

{
                    /* 0x3220  316   */
  return E_NOTIMPL;
}

int __cdecl RETIRED_RegisterMIMETypeForExtensionA(char *param_1,char *param_2)

{
                    /* 0x3a70  454   */
  return FALSE;
}

int __cdecl RETIRED_RegisterMIMETypeForExtensionW(wchar_t *param_1,wchar_t *param_2)

{
                    /* 0x3a80  455   */
  return FALSE;
}

long __cdecl
RETIRED_RegSetValueWrapW
          (HKEY param_1,wchar_t *param_2,unsigned long param_3,wchar_t *param_4,unsigned long param_5)

{
                    /* 0x3a30  450   */
  return ERROR_CALL_NOT_IMPLEMENTED;
}

void __cdecl RETIRED_Remote_ClearStockObjects(void *param_1)

{
                    /* 0x3760  409   */
  return;
}

HRESULT __cdecl RETIRED_Remote_InitUserRegistry(void)

{
                    /* 0x3730  406   */
  return E_NOTIMPL;
}

HRESULT __cdecl RETIRED_Remote_InitUserTheme(int param_1)

{
                    /* 0x38b0  431   */
  return E_NOTIMPL;
}

void __cdecl RETIRED_Remote_LoadTheme(void *param_1)

{
                    /* 0x3770  410   */
  return;
}

unsigned long __cdecl RETIRED_Remote_ThemeHooksInstall(void *param_1)

{
                    /* 0x3740  407   */
  return FALSE;
}

unsigned long __cdecl RETIRED_Remote_ThemeHooksRemove(void *param_1)

{
                    /* 0x3750  408   */
  return FALSE;
}

HRESULT __cdecl RETIRED_ResetWebSettings(HWND param_1,int *param_2)

{
                    /* 0x2860  202   */
  return E_NOTIMPL;
}

int __cdecl RETIRED_RLBuildListOfPaths(void)

{
                    /* 0x1dc0  49   */
  return FALSE;
}

void __cdecl
RETIRED_RunDll_CallEntry16
          (int (__cdecl *param_1)(void),HWND param_2,HINSTANCE param_3,char *param_4,int param_5
          )

{
                    /* 0x1d80  45   */
  return;
}

void __cdecl RETIRED_RunInstallUninstallStubs(wchar_t *param_1)

{
                    /* 0x2f30  282   */
  return;
}

void __cdecl RETIRED_RunInstallUninstallStubsXP(void)

{
                    /* 0x22a0  123   */
  return;
}

int __cdecl RETIRED_SafeOpenPromptForPackager(HWND param_1,wchar_t *param_2,int param_3)

{
                    /* 0x2a10  227   */
  return FALSE;
}

HRESULT __cdecl
RETIRED_SearchForElementInHead
          (IUnknown *param_1,wchar_t *param_2,wchar_t *param_3,_GUID *param_4,IUnknown **param_5)

{
                    /* 0x3100  305   */
  *param_5 = (IUnknown *)0x0;
  return E_NOTIMPL;
}

int __cdecl RETIRED_Service_AreThemeHooksActive(void *param_1)

{
                    /* 0x36a0  399   */
  return FALSE;
}

HRESULT __cdecl RETIRED_Service_ClearStockObjects(void *param_1,void *param_2)

{
                    /* 0x3700  405   */
  return E_NOTIMPL;
}

HRESULT __cdecl
RETIRED_Service_CopyTheme
          (void *param_1,void *param_2,void **param_3,wchar_t *param_4,wchar_t *param_5,
          wchar_t *param_6,unsigned long param_7,unsigned long param_8,void *param_9)

{
                    /* 0x3710  403   */
  if (param_3 != (void **)0x0) {
    *param_3 = (void *)0x0;
  }
  return E_NOTIMPL;
}

int __cdecl RETIRED_Service_GetCurrentChangeNumber(void *param_1)

{
                    /* 0x3680  400   */
  return FALSE;
}

HRESULT __cdecl RETIRED_Service_GetGlobalTheme(void *param_1,void **param_2)

{
                    /* 0x36e0  402   */
  if (param_2 != (void **)0x0) {
    *param_2 = (void *)0x0;
  }
  return E_NOTIMPL;
}

void __cdecl RETIRED_Service_MarkSection(void *param_1,unsigned long param_2,unsigned long param_3)

{
                    /* 0x3690  404   */
  return;
}

void * __cdecl
RETIRED_Service_SessionAllocate
          (void *param_1,unsigned long param_2,void *param_3,void *param_4,void *param_5,void *param_6,
          unsigned long param_7,unsigned long param_8)

{
                    /* 0x3660  395   */
  return (void *)0x0;
}

void __cdecl RETIRED_Service_SessionFree(void *param_1)

{
                    /* 0x3670  396   */
  return;
}

HRESULT __cdecl RETIRED_Service_SetGlobalTheme(void *param_1,void *param_2)

{
                    /* 0x36d0  401   */
  return E_NOTIMPL;
}

HRESULT __cdecl RETIRED_Service_ThemeHooksOff(void *param_1)

{
                    /* 0x36c0  398   */
  return E_NOTIMPL;
}

HRESULT __cdecl RETIRED_Service_ThemeHooksOn(void *param_1)

{
                    /* 0x36b0  397   */
  return E_NOTIMPL;
}

void __cdecl RETIRED_SetShellOfflineState(int param_1)

{
                    /* 0x2a70  230   */
  return;
}

void __cdecl RETIRED_SHAbortInvokeCommand(void)

{
                    /* 0x1e40  56   */
  return;
}

int __cdecl RETIRED_SHAboutInfoW(wchar_t *param_1,unsigned long param_2)

{
                    /* 0x3ba0  472   */
  return FALSE;
}

void * __cdecl RETIRED_SHAllocShared(void *param_1,unsigned long param_2,unsigned long param_3)

{
                    /* 0x1e80  60   */
  return (void *)0x0;
}

unsigned long __cdecl RETIRED_SHCancelUserWorkItems(unsigned long param_1,int param_2)

{
                    /* 0x25c0  165   */
  return 0xffffffff;
}

void __cdecl
RETIRED_SHChangeNotifyReceive(long param_1,unsigned int param_2,ITEMIDLIST *param_3,ITEMIDLIST *param_4)

{
                    /* 0x1ee0  66   */
  return;
}

int __cdecl
RETIRED_SHChangeNotifyUpdateEntryList
          (unsigned long param_1,int param_2,int param_3,_SHChangeNotifyEntry *param_4)

{
                    /* 0x1cb0  35   */
  return FALSE;
}

int __cdecl RETIRED_SHChangeRegistrationReceive(void *param_1,unsigned long param_2)

{
                    /* 0x1ef0  67   */
  return FALSE;
}

int __cdecl RETIRED_SHChangeTimerQueueTimer(void *param_1,void *param_2,unsigned long param_3,unsigned long param_4)

{
                    /* 0x2590  162   */
  return FALSE;
}

int __cdecl
RETIRED_SHCheckDiskForMediaA(HWND param_1,IUnknown *param_2,char *param_3,unsigned int param_4)

{
                    /* 0x3310  344   */
  return FALSE;
}

int __cdecl
RETIRED_SHCheckDiskForMediaW(HWND param_1,IUnknown *param_2,wchar_t *param_3,unsigned int param_4)

{
                    /* 0x3320  345   */
  return FALSE;
}

IETHREADPARAM * __cdecl RETIRED_SHCloneIETHREADPARAM(IETHREADPARAM *param_1)

{
                    /* 0x2e60  272   */
  return (IETHREADPARAM *)0x0;
}

HRESULT __cdecl
RETIRED_SHCoCreateExtension
          (GUID *param_1,IUnknown *param_2,unsigned long param_3,unsigned long param_4,GUID *param_5,
          void **param_6)

{
                    /* 0x26e0  183   */
  *param_6 = (void *)0x0;
  return E_NOTIMPL;
}

int __cdecl RETIRED_SHCoExtensionAllowed(GUID *param_1,unsigned long param_2)

{
                    /* 0x26d0  182   */
  return FALSE;
}

void __cdecl RETIRED_SHCoExtensionCollectStats(GUID *param_1,unsigned long param_2,unsigned long param_3)

{
                    /* 0x26c0  181   */
  return;
}

HRESULT __cdecl RETIRED_SHConvertGraphicsFile(wchar_t *param_1,wchar_t *param_2,unsigned long param_3)

{
                    /* 0x3340  347   */
  return E_NOTIMPL;
}

HRESULT __cdecl
RETIRED_SHCopyMonikerToTemp(IMoniker *param_1,wchar_t *param_2,wchar_t *param_3,int param_4)

{
                    /* 0x1f30  71   */
  return E_NOTIMPL;
}

HRESULT __cdecl
RETIRED_SHCopyStreamWithProgress
          (IUnknown *param_1,IUnknown *param_2,IUnknown *param_3,ULARGE_INTEGER param_4,
          ULARGE_INTEGER param_5)

{
                    /* 0x3590  385   */
  return E_NOTIMPL;
}

/* WARNING: Enum "FOLDERLOGICALVIEWMODE": Some values do not have unique names */

HRESULT __cdecl RETIRED_SHCreateAutoList(AUTOLISTINIT *param_1,GUID *param_2,void **param_3)

{
                    /* 0x2bd0  249   */
  if (param_3 != (void **)0x0) {
    *param_3 = (void *)0x0;
  }
  return E_UNEXPECTED;
}

HRESULT __cdecl
RETIRED_SHCreateAutoListWithID(GUID param_1,unsigned long param_2,GUID *param_3,void **param_4)

{
                    /* 0x2c30  252   */
  if (param_4 != (void **)0x0) {
    *param_4 = (void *)0x0;
  }
  return E_UNEXPECTED;
}

HRESULT __cdecl RETIRED_SHCreateBandForPidl(ITEMIDLIST *param_1,IUnknown **param_2,int param_3)

{
                    /* 0x2a20  232   */
  *param_2 = (IUnknown *)0x0;
  return E_NOTIMPL;
}

HRESULT __cdecl
RETIRED_SHCreateBrowserProgressAggregator
          (HWND param_1,unsigned int param_2,IUnknown *param_3,GUID *param_4,void **param_5)

{
                    /* 0x2ef0  279   */
  if (param_5 != (void **)0x0) {
    *param_5 = (void *)0x0;
  }
  return E_NOTIMPL;
}

HRESULT __cdecl
RETIRED_SHCreateCategoryEnum
          (IItemStore *param_1,IItemKeyArray *param_2,_tagpropertykey *param_3,
          tagCATENUMTYPE param_4,unsigned long param_5,tagFILTERIDLISTTYPE param_6,GUID *param_7,
          void **param_8)

{
                    /* 0x2b30  237   */
  if (param_8 != (void **)0x0) {
    *param_8 = (void *)0x0;
  }
  return E_UNEXPECTED;
}

void * __cdecl RETIRED_SHCreateDesktop(IDeskTray *param_1)

{
                    /* 0x2280  121   */
  return (void *)0x0;
}

int __cdecl RETIRED_SHCreateFromDesktop(NEWFOLDERINFO *param_1)

{
                    /* 0x21c0  110   */
  return FALSE;
}

IETHREADPARAM * __cdecl
RETIRED_SHCreateIETHREADPARAM
          (wchar_t *param_1,int param_2,ITravelLog *param_3,IBrowserThreadHandshake *param_4)

{
                    /* 0x2e50  271   */
  return (IETHREADPARAM *)0x0;
}

HRESULT __cdecl RETIRED_SHCreateInstance(GUID *param_1,GUID *param_2,void **param_3)

{
                    /* 0x2ad0  236   */
  if (param_3 != (void **)0x0) {
    *param_3 = (void *)0x0;
  }
  return E_NOTIMPL;
}

HRESULT __cdecl
RETIRED_SHCreateKindFilter(IVisibleInDescription *param_1,GUID *param_2,void **param_3)

{
                    /* 0x2dd0  265   */
  if (param_3 != (void **)0x0) {
    *param_3 = (void *)0x0;
  }
  return E_UNEXPECTED;
}

HRESULT __cdecl RETIRED_SHCreatePropertyBag(GUID *param_1,void **param_2)

{
                    /* 0x2790  191   */
  *param_2 = (void *)0x0;
  return E_NOTIMPL;
}

void __cdecl RETIRED_SHCreateSavedWindows(void)

{
                    /* 0x2e20  268   */
  return;
}

HRESULT __cdecl
RETIRED_SHCreateScopeFromIDLists(unsigned int param_1,ITEMIDLIST **param_2,GUID *param_3,void **param_4)

{
                    /* 0x2c70  254   */
  if (param_4 != (void **)0x0) {
    *param_4 = (void *)0x0;
  }
  return E_UNEXPECTED;
}

HRESULT __cdecl
RETIRED_SHCreateScopeFromShellItemArray(IShellItemArray *param_1,GUID *param_2,void **param_3)

{
                    /* 0x2cb0  256   */
  if (param_3 != (void **)0x0) {
    *param_3 = (void *)0x0;
  }
  return E_UNEXPECTED;
}

HRESULT __cdecl
RETIRED_SHCreateScopeItemFromIDList(ITEMIDLIST *param_1,int param_2,GUID *param_3,void **param_4)

{
                    /* 0x2c90  255   */
  if (param_4 != (void **)0x0) {
    *param_4 = (void *)0x0;
  }
  return E_UNEXPECTED;
}

HRESULT __cdecl
RETIRED_SHCreateScopeItemFromKnownFolder(GUID *param_1,int param_2,GUID *param_3,void **param_4)

{
                    /* 0x2cf0  258   */
  if (param_4 != (void **)0x0) {
    *param_4 = (void *)0x0;
  }
  return E_UNEXPECTED;
}

HRESULT __cdecl
RETIRED_SHCreateScopeItemFromShellItem
          (IShellItem *param_1,int param_2,GUID *param_3,void **param_4)

{
                    /* 0x2cd0  257   */
  if (param_4 != (void **)0x0) {
    *param_4 = (void *)0x0;
  }
  return E_UNEXPECTED;
}

HRESULT __cdecl
RETIRED_SHCreateSearchIDList
          (wchar_t *param_1,wchar_t *param_2,IScope *param_3,IShellItemArray *param_4,
          ITEMIDLIST **param_5)

{
                    /* 0x2c10  251   */
  if (param_5 != (ITEMIDLIST **)0x0) {
    *param_5 = (ITEMIDLIST *)0x0;
  }
  return E_UNEXPECTED;
}

HRESULT __cdecl
RETIRED_SHCreateSearchIDListFromAutoList
          (IAutoListDescription *param_1,INamedPropertyStore *param_2,ITEMIDLIST **param_3)

{
                    /* 0x2bf0  250   */
  if (param_3 != (ITEMIDLIST **)0x0) {
    *param_3 = (ITEMIDLIST *)0x0;
  }
  return E_UNEXPECTED;
}

HRESULT __cdecl RETIRED_SHCreateSplashScreen(ISplashScreen **param_1)

{
                    /* 0x22b0  124   */
  *param_1 = (ISplashScreen *)0x0;
  return E_NOTIMPL;
}

HRESULT __cdecl
RETIRED_SHCreateStreamWrapper(IStream **param_1,unsigned int param_2,unsigned long param_3,IStream **param_4)

{
                    /* 0x3b80  471   */
  *param_4 = (IStream *)0x0;
  return E_FAIL;
}

void * __cdecl RETIRED_SHCreateTimerQueue(void)

{
                    /* 0x2570  160   */
  return (void *)0x0;
}

HRESULT __cdecl
RETIRED_SHCreateTransientVFolderIDList
          (IScope *param_1,IShellItemArray *param_2,ITEMIDLIST **param_3)

{
                    /* 0x2c50  253   */
  if (param_3 != (ITEMIDLIST **)0x0) {
    *param_3 = (ITEMIDLIST *)0x0;
  }
  return E_UNEXPECTED;
}

long __cdecl RETIRED_SHDeleteOrphanKeyA(HKEY param_1,char *param_2)

{
                    /* 0x39f0  446   */
  return ERROR_CALL_NOT_IMPLEMENTED;
}

long __cdecl RETIRED_SHDeleteOrphanKeyW(HKEY param_1,wchar_t *param_2)

{
                    /* 0x3a00  447   */
  return ERROR_CALL_NOT_IMPLEMENTED;
}

int __cdecl RETIRED_SHDeleteTimerQueue(void *param_1)

{
                    /* 0x2580  161   */
  return FALSE;
}

int __cdecl RETIRED_SHDesktopMessageLoop(void *param_1)

{
                    /* 0x2290  122   */
  return FALSE;
}

void RETIRED_SHDestroyIETHREADPARAM(void)

{
                    /* WARNING: Could not recover jumptable at 0x4eb82e80. Too many branches */
                    /* WARNING: Treating indirect jump as call */
                    /* 0x2e80  274   */
  LocalFree((HLOCAL)1);
  return;
}

HRESULT __cdecl
RETIRED_SHDGetPageLocation
          (HWND param_1,unsigned int param_2,wchar_t *param_3,unsigned int param_4,ITEMIDLIST **param_5)

{
                    /* 0x29d0  223   */
  return E_NOTIMPL;
}

int __cdecl
RETIRED_SHDialogBox(HINSTANCE param_1,wchar_t *param_2,HWND param_3,
                   int (__cdecl *param_4)(void *, HWND *, unsigned int, unsigned int, long),void *param_5)

{
                    /* 0x2b10  240   */
  SetLastError(1);
  return -1;
}

HRESULT __cdecl
RETIRED_SHDisplayNameFromScopeAndSubQueries
          (IScope *param_1,IShellItemArray *param_2,unsigned long param_3,unsigned long param_4,_SIGDN param_5,
          wchar_t **param_6)

{
                    /* 0x2d50  261   */
  if (param_6 != (wchar_t **)0x0) {
    *param_6 = (wchar_t *)0x0;
  }
  return E_UNEXPECTED;
}

HRESULT __cdecl
RETIRED_SHEnumClassesOfCategories
          (unsigned long param_1,GUID *param_2,unsigned long param_3,GUID *param_4,IEnumGUID **param_5)

{
                    /* 0x2ed0  277   */
  return E_NOTIMPL;
}

HRESULT __cdecl
RETIRED_SHEvaluateSystemCommandTemplate
          (wchar_t *param_1,wchar_t **param_2,wchar_t **param_3,wchar_t **param_4)

{
                    /* 0x2670  176   */
  return E_NOTIMPL;
}

int __cdecl RETIRED_SHExplorerParseCmdLine(PNEWFOLDERINFO param_1)

{
                    /* 0x21d0  111   */
  return FALSE;
}

HRESULT __cdecl
RETIRED_SHFileSysBindToStorage
          (wchar_t *param_1,unsigned long param_2,unsigned long param_3,unsigned long param_4,GUID *param_5,void **param_6)

{
                    /* 0x28e0  216   */
  *param_6 = (void *)0x0;
  return E_NOTIMPL;
}

HRESULT __cdecl RETIRED_SHFilterConditionFromString(wchar_t *param_1,IFilterCondition **param_2)

{
                    /* 0x3620  424   */
  *param_2 = (IFilterCondition *)0x0;
  return E_NOTIMPL;
}

HRESULT __cdecl RETIRED_SHFilterConditionToString(IFilterCondition *param_1,wchar_t **param_2)

{
                    /* 0x3640  425   */
  *param_2 = (wchar_t *)0x0;
  return E_NOTIMPL;
}

HRESULT __cdecl RETIRED_SHFlushClipboard(void)

{
                    /* 0x1d70  44   */
  return E_NOTIMPL;
}

int __cdecl RETIRED_SHFreeShared(void *param_1,unsigned long param_2)

{
                    /* 0x1eb0  63   */
  return FALSE;
}

void __cdecl RETIRED_SHFreeUnusedLibraries(void)

{
                    /* 0x1d90  46   */
  return;
}

unsigned long __cdecl RETIRED_SHGetActiveConsoleSessionId(void)

{
                    /* 0x34c0  376   */
  return FALSE;
}

LPSECURITY_ATTRIBUTES __cdecl RETIRED_SHGetAllAccessSA(void)

{
                    /* 0x25e0  167   */
  return (LPSECURITY_ATTRIBUTES)0x0;
}

unsigned long __cdecl RETIRED_SHGetAppCompatFlags(unsigned long param_1)

{
                    /* 0x3880  428   */
  return FALSE;
}

HRESULT __cdecl RETIRED_SHGetHistoryPIDL(ITEMIDLIST **param_1)

{
                    /* 0x29e0  224   */
  return E_NOTIMPL;
}

HRESULT __cdecl RETIRED_SHGetIdentityItem(IShellItem *param_1,IShellItem **param_2)

{
                    /* 0x35a0  386   */
  *param_2 = (IShellItem *)0x0;
  return E_NOTIMPL;
}

unsigned long __cdecl
RETIRED_SHGetIniStringUTF7W
          (wchar_t *param_1,wchar_t *param_2,wchar_t *param_3,unsigned long param_4,wchar_t *param_5)

{
                    /* 0x2620  171   */
  return FALSE;
}

HRESULT __cdecl
RETIRED_SHGetNavigateTarget
          (IShellFolder *param_1,ITEMIDLIST *param_2,ITEMIDLIST **param_3,unsigned long *param_4)

{
                    /* 0x2e90  275   */
  if (param_3 != (ITEMIDLIST **)0x0) {
    *param_3 = (ITEMIDLIST *)0x0;
  }
  if (param_4 != (unsigned long *)0x0) {
    *param_4 = 0;
  }
  return E_NOTIMPL;
}

unsigned long __cdecl
RETIRED_SHGetPrivateProfileSection(wchar_t *param_1,wchar_t *param_2,unsigned long param_3,wchar_t *param_4)

{
                    /* 0x2680  177   */
  return FALSE;
}

unsigned long __cdecl
RETIRED_SHGetPrivateProfileSectionNames(wchar_t *param_1,unsigned long param_2,wchar_t *param_3)

{
                    /* 0x2690  178   */
  return FALSE;
}

unsigned long __cdecl
RETIRED_SHGetPrivateProfileString
          (wchar_t *param_1,wchar_t *param_2,wchar_t *param_3,wchar_t *param_4,unsigned long param_5,
          wchar_t *param_6)

{
                    /* 0x26a0  179   */
  return FALSE;
}

int __cdecl
RETIRED_SHGetPrivateProfileStruct
          (wchar_t *param_1,wchar_t *param_2,void *param_3,unsigned int param_4,wchar_t *param_5)

{
                    /* 0x26b0  180   */
  return FALSE;
}

unsigned long __cdecl RETIRED_SHGetProcessDword(unsigned long param_1,long param_2)

{
                    /* 0x3490  373   */
  return FALSE;
}

HRESULT __cdecl
RETIRED_SHGetSetFolderCustomSettingsA(LPSHFOLDERCUSTOMSETTINGSA param_1,char *param_2,unsigned long param_3)

{
                    /* 0x1f20  70   */
  return E_NOTIMPL;
}

HINSTANCE __cdecl RETIRED_SHGetShellStyleHInstance(void)

{
                    /* 0x1f90  76   */
  return (HINSTANCE)0x0;
}

HRESULT __cdecl
RETIRED_SHGetSignatureInfo(wchar_t *param_1,unsigned long param_2,SIGNATURE_INFO *param_3)

{
  unsigned long uVar1;
  
                    /* 0x3980  443   */
  uVar1 = param_3->cbSize;
  memset(NULL,0,NULL);
  param_3->cbSize = uVar1;
  return E_NOTIMPL;
}

HRESULT __cdecl
RETIRED_SHGetSignatureInfoEx
          (wchar_t *param_1,void *param_2,unsigned long param_3,SIGNATURE_INFO *param_4,void **param_5,
          void **param_6)

{
  unsigned long uVar1;
  
                    /* 0x39b0  442   */
  uVar1 = param_4->cbSize;
  memset(param_2,0,NULL);
  param_4->cbSize = uVar1;
  if (param_5 != (void **)0x0) {
    *param_5 = (void *)0x0;
  }
  if (param_6 != (void **)0x0) {
    *param_6 = (void *)0x0;
  }
  return E_NOTIMPL;
}

HRESULT __cdecl RETIRED_SHGetUserPicturePathW(wchar_t *param_1,unsigned long param_2,wchar_t *param_3)

{
                    /* 0x1e60  58   */
  return E_NOTIMPL;
}

unsigned long __cdecl RETIRED_SHGetUserSessionId(void *param_1)

{
                    /* 0x34d0  377   */
  return FALSE;
}

HRESULT __cdecl RETIRED_SHGetUserToken_shlwapi(void *param_1,PTOKEN_USER *param_2)

{
                    /* 0x2900  210   */
  *param_2 = (PTOKEN_USER)0x0;
  return E_NOTIMPL;
}

IStream * __cdecl
RETIRED_SHGetViewStream
          (ITEMIDLIST *param_1,unsigned long param_2,wchar_t *param_3,wchar_t *param_4,wchar_t *param_5)

{
                    /* 0x30f0  304   */
  return (IStream *)0x0;
}

HRESULT __cdecl RETIRED_SHGetWebFolderFilePathA(char *param_1,char *param_2,unsigned int param_3)

{
                    /* 0x3b60  469   */
  return E_NOTIMPL;
}

HRESULT __cdecl RETIRED_SHGetWebFolderFilePathW(wchar_t *param_1,wchar_t *param_2,unsigned int param_3)

{
                    /* 0x3b70  470   */
  return E_NOTIMPL;
}

void * __cdecl RETIRED_SHGlobalCounterCreate(GUID *param_1)

{
                    /* 0x2430  139   */
  return (void *)0x0;
}

void * __cdecl RETIRED_SHGlobalCounterCreateNamedA(char *param_1,long param_2)

{
                    /* 0x25f0  168   */
  return (void *)0x0;
}

void * __cdecl RETIRED_SHGlobalCounterCreateNamedW(wchar_t *param_1,long param_2)

{
                    /* 0x2600  169   */
  return (void *)0x0;
}

long __cdecl RETIRED_SHGlobalCounterDecrement(void *param_1)

{
                    /* 0x2610  170   */
  return FALSE;
}

long __cdecl RETIRED_SHGlobalCounterGetValue(void *param_1)

{
                    /* 0x2440  140   */
  return FALSE;
}

long __cdecl RETIRED_SHGlobalCounterIncrement(void *param_1)

{
                    /* 0x2450  141   */
  return FALSE;
}

void __cdecl RETIRED_SHGlobalDefect(unsigned long param_1)

{
                    /* 0x1e30  55   */
  return;
}

void __cdecl RETIRED_SHHandleDiskFull(HWND param_1,int param_2)

{
                    /* 0x3600  390   */
  return;
}

HWND __cdecl
RETIRED_SHHtmlHelpOnDemandA
          (HWND param_1,char *param_2,unsigned int param_3,unsigned long param_4,unsigned long param_5,int param_6)

{
                    /* 0x3260  322   */
  return (HWND__ *)0x0;
}

HWND  __cdecl
RETIRED_SHHtmlHelpOnDemandW
          (HWND param_1,wchar_t *param_2,unsigned int param_3,unsigned long param_4,unsigned long param_5,int param_6)

{
                    /* 0x3270  323   */
  return (HWND__ *)0x0;
}

unsigned int __cdecl
RETIRED_SHIEErrorMsgBox
          (IShellBrowser *param_1,HWND param_2,HRESULT param_3,wchar_t *param_4,
          wchar_t *param_5,unsigned int param_6,unsigned int param_7)

{
                    /* 0x29f0  225   */
  return 2;
}

HRESULT __cdecl RETIRED_SHInitializeControlPanelRegkeys(void)

{
                    /* 0x3610  391   */
  return E_NOTIMPL;
}

HRESULT __cdecl
RETIRED_SHInvokePrivilegedFunctionW(wchar_t *param_1,int (__cdecl *param_2)(void),void *param_3)

{
                    /* 0x34b0  375   */
  return E_NOTIMPL;
}

long __cdecl
RETIRED_CallCPLEntry16
          (HINSTANCE param_1,int (__cdecl *param_2)(void),HWND param_3,unsigned int param_4,long param_5
          ,long param_6)

{
                    /* 0x1e10  53   */
  return FALSE;
}

HRESULT __cdecl
RETIRED_WinList_FindFolderWindow
          (ITEMIDLIST *param_1,ITEMIDLIST *param_2,HWND *param_3,IWebBrowserApp **param_4)

{
                    /* 0x3050  297   */
  if (param_3 != (HWND*)0x0) {
    *param_3 = (HWND)0x0;
  }
  if (param_4 != (IWebBrowserApp **)0x0) {
    *param_4 = (IWebBrowserApp *)0x0;
  }
  return E_NOTIMPL;
}

unsigned long __cdecl RETIRED_WNetRestoreConnectionWrapW(HWND param_1,wchar_t *param_2)

{
                    /* 0x28d0  209   */
  return ERROR_PROC_NOT_FOUND;
}


IShellWindows * __cdecl RETIRED_WinList_GetShellWindows(int param_1)

{
                    /* 0x2a40  228   */
  return (IShellWindows *)0x0;
}

void __cdecl RETIRED_WinList_Terminate(void)

{
                    /* 0x2f20  281   */
  return;
}

int __cdecl RETIRED_WinList_Init(void)

{
                    /* 0x2f10  280   */
  return 1;
}

HRESULT __cdecl RETIRED_WinList_Revoke(long param_1)

{
                    /* 0x30a0  299   */
  return E_NOTIMPL;
}

HRESULT __cdecl
RETIRED_WinList_NotifyNewLocation(IShellWindows *param_1,long param_2,ITEMIDLIST *param_3)

{
                    /* 0x3040  296   */
  return E_NOTIMPL;
}

HRESULT __cdecl
RETIRED_WinList_RegisterPending
          (unsigned long param_1,ITEMIDLIST *param_2,ITEMIDLIST *param_3,long *param_4)

{
                    /* 0x3080  298   */
  if (param_4 != (long *)0x0) {
    *param_4 = 0;
  }
  return E_NOTIMPL;
}

HRESULT __cdecl
RETIRED_ZoneCheckPathW
          (wchar_t *param_1,unsigned long param_2,unsigned long param_3,IInternetSecurityMgrSite *param_4)

{
                    /* 0x2bb0  245   */
  return E_NOTIMPL;
}

HRESULT __cdecl
RETIRED_ZoneCheckPathA(char *param_1,unsigned long param_2,unsigned long param_3,IInternetSecurityMgrSite *param_4)

{
                    /* 0x2ba0  244   */
  return E_NOTIMPL;
}

short __cdecl RETIRED_UseCustomInternetSearch(void)

{
                    /* 0x2390  133   */
  return FALSE;
}

void __cdecl RETIRED_ThemesOnCreateSessionWithPID(unsigned long param_1)

{
                    /* 0x37e0  393   */
  return;
}

int __cdecl RETIRED_StrToOleStr(wchar_t *param_1,wchar_t *param_2)

{
                    /* 0x1df0  52   */
  *param_1 = L'\0';
  return FALSE;
}

wchar_t * __cdecl RETIRED_StrChrW(wchar_t *param_1,wchar_t param_2)

{
                    /* 0x2040  86   */
  return (wchar_t *)0x0;
}

unsigned long __cdecl RETIRED_StopWatchFlush(void)

{
                    /* 0x2470  143   */
  return 0xd;
}

unsigned long __cdecl RETIRED_StopWatchMode(void)

{
                    /* 0x2460  142   */
  return FALSE;
}

unsigned long __cdecl
RETIRED_StopWatchW(unsigned long param_1,wchar_t *param_2,unsigned long param_3,unsigned long param_4,unsigned long param_5)

{
                    /* 0x2490  145   */
  return FALSE;
}

HRESULT __cdecl RETIRED_ShowUrlInNewBrowserInstance(wchar_t *param_1)

{
                    /* 0x2950  217   */
  return E_FAIL;
}

void __cdecl RETIRED_ShellDDEInit(int param_1)

{
                    /* 0x2270  120   */
  return;
}

int __cdecl RETIRED_SHTerminateThreadPool(void)

{
                    /* 0x25b0  164   */
  return FALSE;
}

int __cdecl RETIRED_SHUnlockShared(void *param_1)

{
                    /* 0x1ea0  62   */
  return FALSE;
}

int __cdecl RETIRED_SHUnregisterWait(void *param_1)

{
                    /* 0x2560  159   */
  return FALSE;
}

int __cdecl RETIRED_SHUseClassicToolbarGlyphs(void)

{
                    /* 0x28c0  208   */
  return FALSE;
}

int __cdecl RETIRED_SHVerbExistsNA(char *param_1,char *param_2,char *param_3,unsigned long param_4)

{
                    /* 0x2b80  243   */
  return FALSE;
}

void __cdecl RETIRED_SHWaitOp_Operate(void *param_1,unsigned long param_2)

{
                    /* 0x34f0  379   */
  return;
}

int __cdecl RETIRED_SHWinHelp(HWND param_1,wchar_t *param_2,unsigned int param_3,unsigned long param_4)

{
                    /* 0x3470  371   */
  return FALSE;
}

int __cdecl
RETIRED_SHWinHelpOnDemandA(HWND param_1,char *param_2,unsigned int param_3,unsigned long param_4,int param_5)

{
                    /* 0x3280  324   */
  return FALSE;
}

int __cdecl
RETIRED_SHWinHelpOnDemandW(HWND param_1,wchar_t *param_2,unsigned int param_3,unsigned long param_4,int param_5)

{
                    /* 0x3290  325   */
  return FALSE;
}

HRESULT __cdecl
RETIRED_SHWriteClassesOfCategories
          (unsigned long param_1,GUID *param_2,unsigned long param_3,GUID *param_4,int param_5,int param_6,
          void *param_7)

{
                    /* 0x2ee0  278   */
  return E_NOTIMPL;
}

void __cdecl RETIRED_StopWatch_CheckMsg(HWND param_1,tagMSG param_2,char *param_3)

{
                    /* 0x24b0  147   */
  return;
}

unsigned long __cdecl RETIRED_StopWatch_DispatchTime(int param_1,tagMSG param_2,unsigned long param_3)

{
                    /* 0x2500  153   */
  return FALSE;
}

void __cdecl RETIRED_StopWatch_MarkFrameStart(char *param_1)

{
                    /* 0x24c0  148   */
  return;
}

void __cdecl RETIRED_StopWatch_MarkJavaStop(char *param_1,HWND param_2,int param_3)

{
                    /* 0x24e0  150   */
  return;
}

void __cdecl RETIRED_StopWatch_MarkSameFrameStart(HWND param_1)

{
                    /* 0x24d0  149   */
  return;
}

void __cdecl RETIRED_StopWatch_SetMsgLastLocation(unsigned long param_1)

{
                    /* 0x2510  154   */
  return;
}

int __cdecl
RETIRED_StopWatch_TimerHandler(HWND param_1,unsigned int param_2,unsigned long param_3,tagMSG *param_4)

{
                    /* 0x24a0  146   */
  return FALSE;
}

unsigned long __cdecl
RETIRED_StopWatchA(unsigned long param_1,char *param_2,unsigned long param_3,unsigned long param_4,unsigned long param_5)

{
                    /* 0x2480  144   */
  return FALSE;
}

unsigned long __cdecl
RETIRED_StopWatchExA
          (unsigned long param_1,wchar_t *param_2,unsigned long param_3,unsigned long param_4,unsigned long param_5,unsigned long param_6)

{
                    /* 0x2520  155   */
  return ERROR_CALL_NOT_IMPLEMENTED;
}

unsigned long __cdecl
RETIRED_StopWatchExW
          (unsigned long param_1,wchar_t *param_2,unsigned long param_3,unsigned long param_4,unsigned long param_5,unsigned long param_6)

{
                    /* 0x2530  156   */
  return ERROR_CALL_NOT_IMPLEMENTED;
}

char * __cdecl RETIRED_StrChrA(char *param_1,unsigned short param_2)

{
                    /* 0x1fc0  78   */
  return (char *)0x0;
}

char * __cdecl RETIRED_StrChrIA(char *param_1,unsigned short param_2)

{
                    /* 0x20c0  94   */
  return (char *)0x0;
}

wchar_t * __cdecl RETIRED_StrChrIW(wchar_t *param_1,wchar_t param_2)

{
                    /* 0x20d0  95   */
  return (wchar_t *)0x0;
}

int __cdecl RETIRED_StrCmpNA(char *param_1,char *param_2,int param_3)

{
                    /* 0x1fe0  80   */
  return FALSE;
}

int __cdecl RETIRED_StrCmpNIA(char *param_1,char *param_2,int param_3)

{
                    /* 0x1ff0  81   */
  return FALSE;
}

int __cdecl RETIRED_StrCmpNIW(wchar_t *param_1,wchar_t *param_2,int param_3)

{
                    /* 0x2070  89   */
  return FALSE;
}

int __cdecl RETIRED_StrCmpNW(wchar_t *param_1,wchar_t *param_2,int param_3)

{
                    /* 0x2060  88   */
  return FALSE;
}

int __cdecl RETIRED_StrCSpnA(char *param_1,char *param_2)

{
                    /* 0x2020  84   */
  return FALSE;
}

int __cdecl RETIRED_StrCSpnIA(char *param_1,char *param_2)

{
                    /* 0x2120  100   */
  return FALSE;
}

int __cdecl RETIRED_StrCSpnIW(wchar_t *param_1,wchar_t *param_2)

{
                    /* 0x2130  101   */
  return FALSE;
}

int __cdecl RETIRED_StrCSpnW(wchar_t *param_1,wchar_t *param_2)

{
                    /* 0x20a0  92   */
  return FALSE;
}

int __cdecl RETIRED_StrIsCanonicalEqualA(char *param_1,char *param_2)

{
                    /* 0x33d0  355   */
  return -2;
}

int __cdecl RETIRED_StrIsCanonicalEqualNA(char *param_1,char *param_2,int param_3)

{
                    /* 0x33b0  353   */
  return -2;
}

int __cdecl RETIRED_StrIsCanonicalEqualNW(wchar_t *param_1,wchar_t *param_2,int param_3)

{
                    /* 0x33c0  354   */
  return -2;
}

int __cdecl RETIRED_StrIsCanonicalEqualW(wchar_t *param_1,wchar_t *param_2)

{
                    /* 0x33e0  356   */
  return -2;
}

char * __cdecl RETIRED_StrRChrA(char *param_1,char *param_2,unsigned short param_3)

{
                    /* 0x1fd0  79   */
  return (char *)0x0;
}

char * __cdecl RETIRED_StrRChrIA(char *param_1,char *param_2,unsigned short param_3)

{
                    /* 0x20e0  96   */
  return (char *)0x0;
}

wchar_t * __cdecl RETIRED_StrRChrIW(wchar_t *param_1,wchar_t *param_2,wchar_t param_3)

{
                    /* 0x20f0  97   */
  return (wchar_t *)0x0;
}

wchar_t * __cdecl RETIRED_StrRChrW(wchar_t *param_1,wchar_t *param_2,wchar_t param_3)

{
                    /* 0x2050  87   */
  return (wchar_t *)0x0;
}

int __cdecl
RETIRED_StrRetToStrN(wchar_t *param_1,unsigned int param_2,LPSTRRET param_3,ITEMIDLIST *param_4)

{
                    /* 0x1d50  43   */
  if (param_2 != 0) {
    *param_1 = L'\0';
  }
  return 0;
}

char * __cdecl RETIRED_StrRStrIA(char *param_1,char *param_2,char *param_3)

{
                    /* 0x2100  98   */
  return (char *)0x0;
}

wchar_t * __cdecl RETIRED_StrRStrIW(wchar_t *param_1,wchar_t *param_2,wchar_t *param_3)

{
                    /* 0x2110  99   */
  return (wchar_t *)0x0;
}

char * __cdecl RETIRED_StrStrA(char *param_1,char *param_2)

{
                    /* 0x2000  82   */
  return (char *)0x0;
}

char * __cdecl RETIRED_StrStrIA(char *param_1,char *param_2)

{
                    /* 0x2010  83   */
  return (char *)0x0;
}

wchar_t * __cdecl RETIRED_StrStrIW(wchar_t *param_1,wchar_t *param_2)

{
                    /* 0x2090  91   */
  return (wchar_t *)0x0;
}

wchar_t * __cdecl RETIRED_StrStrW(wchar_t *param_1,wchar_t *param_2)

{
                    /* 0x2080  90   */
  return (wchar_t *)0x0;
}

int __cdecl RETIRED_StrToIntA(char *param_1)

{
                    /* 0x2030  85   */
  return 0;
}

int __cdecl RETIRED_StrToIntW(wchar_t *param_1)

{
                    /* 0x20b0  93   */
  return 0;
}

int __cdecl RETIRED_StrToOleStrN(wchar_t *param_1,int param_2,wchar_t *param_3,int param_4)

{
                    /* 0x1cf0  38   */
  *param_1 = L'\0';
  return FALSE;
}

void __cdecl RETIRED_ThemeSection_Close(void *param_1)

{
                    /* 0x38e0  434   */
  return;
}

void __cdecl RETIRED_ThemeSection_CreateFromSection(void *param_1,void **param_2)

{
                    /* 0x38a0  430   */
  return;
}

void * __cdecl RETIRED_ThemeSection_Get(void *param_1)

{
                    /* 0x3900  436   */
  return (void *)0x0;
}

void * __cdecl RETIRED_ThemeSection_GetData(void *param_1)

{
                    /* 0x3910  437   */
  return (void *)0x0;
}

HRESULT __cdecl
RETIRED_ThemeSection_Open(void *param_1,void *param_2,unsigned long param_3,void **param_4)

{
                    /* 0x38d0  433   */
  return E_NOTIMPL;
}

HRESULT __cdecl RETIRED_ThemeSection_ValidateData(void *param_1,int param_2)

{
                    /* 0x38f0  435   */
  return E_NOTIMPL;
}

void __cdecl RETIRED_ThemesOnCreateSession(void)

{
                    /* 0x37f0  417   */
  return;
}

void __cdecl RETIRED_ThemesOnLogoff(void)

{
                    /* 0x3820  420   */
  return;
}

void __cdecl RETIRED_ThemesOnDisconnect(void)

{
                    /* 0x3840  422   */
  return;
}

void __cdecl RETIRED_ThemesOnLogon(int param_1)

{
                    /* 0x3810  419   */
  return;
}

void __cdecl RETIRED_ThemesOnReconnect(void)

{
                    /* 0x3830  421   */
  return;
}

void __cdecl RETIRED_ThemesOnTerminateSession(void)

{
                    /* 0x3800  418   */
  return;
}

void __cdecl RETIRED_ThemesOnWinlogonSetup(void)

{
                    /* 0x3850  394   */
  return;
}

int __cdecl RETIRED_ThemeUserLogoff(void)

{
                    /* 0x37b0  413   */
  return FALSE;
}

int __cdecl RETIRED_ThemeUserLogon(void *param_1,int param_2)

{
                    /* 0x37a0  414   */
  return FALSE;
}

int __cdecl RETIRED_ThemeUserStartShell(void)

{
                    /* 0x37d0  415   */
  return FALSE;
}

int __cdecl RETIRED_ThemeUserTSReconnect(void)

{
                    /* 0x37c0  416   */
  return FALSE;
}

unsigned long __cdecl RETIRED_ThemeWaitForServiceReady(unsigned long param_1)

{
                    /* 0x3780  412   */
  return ERROR_CALL_NOT_IMPLEMENTED;
}

int __cdecl RETIRED_ThemeWatchForStart(void)

{
                    /* 0x3790  411   */
  return FALSE;
}

HRESULT __cdecl RETIRED_UEMRegisterNotify(int (__cdecl *param_1)(void),void *param_2,int param_3)

{
                    /* 0x2e30  269   */
  return E_NOTIMPL;
}

int __cdecl RETIRED_UnregisterExtensionForMIMETypeA(char *param_1)

{
                    /* 0x3ad0  460   */
  return FALSE;
}

int __cdecl RETIRED_UnregisterExtensionForMIMETypeW(wchar_t *param_1)

{
                    /* 0x3ae0  461   */
  return FALSE;
}

int __cdecl RETIRED_UnregisterMIMETypeForExtensionA(char *param_1)

{
                    /* 0x3a90  456   */
  return FALSE;
}

int __cdecl RETIRED_UnregisterMIMETypeForExtensionW(wchar_t *param_1)

{
                    /* 0x3aa0  457   */
  return FALSE;
}

int __cdecl
RETIRED_UrlCrackW(wchar_t *param_1,unsigned long param_2,unsigned long param_3,struct SHURL_COMPONENTSW *param_4)

{
                    /* 0x3300  341   */
  return FALSE;
}

int __cdecl RETIRED_UrlHitsNetW(wchar_t *param_1)

{
                    /* 0x3120  306   */
  return FALSE;
}

HRESULT __cdecl
RETIRED_URLSubLoadString
          (HINSTANCE param_1,unsigned int param_2,wchar_t *param_3,unsigned long param_4,unsigned int param_5)

{
                    /* 0x2960  218   */
  return E_NOTIMPL;
}

HRESULT __cdecl
RETIRED_URLSubRegQueryA
          (char *param_1,char *param_2,int param_3,char *param_4,unsigned long param_5,unsigned long param_6)

{
                    /* 0x2360  130   */
  return E_NOTIMPL;
}

