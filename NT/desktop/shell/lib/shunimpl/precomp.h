#define NOSHLWAPI

#include <windows.h>
#include <winternl.h>
#include <evntprov.h>
#include <dpa_dsa.h>
#include <ntetw.h>
#include <shlwapip.h>
#include <shellapi.h>
#include <shtypes.h>
#include <exdisp.h>
#include <urlmon.h>
#include <tlog.h>
#include <shlobj.h>
#include <shlobjp.h>
#include <shdocvw.h>
#include <siginfop.h>
#include <shobjidl.h>
#include <filteredfolder.h>
#include <propsys.h>
#include <windowsstoragecom.h>
#include <oleauto.h>
#include <functiondiscoveryapi.h>
#include <functiondiscoverynotification.h>
#include <scopeitem.h>
#include <nap.h>
#include <servprov.h>
#ifdef FACILITY_VISUALCPP
#undef FACILITY_VISUALCPP
#include <delayimp.h>
#endif
#include <shpriv.h>
#include <msxml.h>

struct IVisibleInList;
struct IPropertyKeyStore;

typedef enum {
	ALF_DEFAULT = 0,
	ALF_FORCEFULLTEXT = 2,
	ALF_SHOWHIDDEN = 4,
	ALF_SUBFOLDERS = 8,
	ALF_NO_ADVANCED_QUERY_SYNTAX = 32,
	ALF_FETCH_COLUMNS_IN_ENUM = 64,
	ALF_USE_SORTBY_IN_ENUM = 128,
	ALF_DYNAMIC_FILTERS = 256
} AUTOLISTFLAGS;

typedef struct {
	wchar_t *pszParsingName;
	wchar_t *pszDisplayName;
	GUID ftid;
	AUTOLISTFLAGS dwAutoListFlags;
	unsigned long dwFolderFlags;
	unsigned long dwUITaskFlags;
	FOLDERLOGICALVIEWMODE flvm;
	int iIconSize;
	FOLDERLOGICALVIEWMODE flvmStack;
	int iIconSizeStack;
	PROPERTYKEY keyGroup;
	int fSortGroupAscending;
	unsigned int cStackKeys;
	PROPERTYKEY *rgStackKeys;
	unsigned int cSortColumns;
	SORTCOLUMN *rgSortColumns;
	struct IColumnList *pcl;
	struct IVisibleInList *pvl;
	struct IScope *pscope;
	IShellItemArray *psiaSubQueries;
	ICondition *pc;
	IPropertyKeyStore *pksRelevance;
	unsigned int cRowsGroupSubset;
	IUnknown *punkStackData;
} AUTOLISTINIT;
