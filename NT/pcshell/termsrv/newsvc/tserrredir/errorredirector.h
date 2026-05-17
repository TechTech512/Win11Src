#ifndef ERRORREDIRECTOR_H
#define ERRORREDIRECTOR_H

#include <unknwn.h>

extern "C" const IID IID_IUnknown;
extern "C" const IID IID_ILogonErrorRedirector;

extern long g_cRef;

// Define LOGON_ERROR_REDIRECTOR_RESPONSE if not already defined
#ifndef LOGON_ERROR_REDIRECTOR_RESPONSE_DEFINED
#define LOGON_ERROR_REDIRECTOR_RESPONSE_DEFINED
typedef struct _LOGON_ERROR_REDIRECTOR_RESPONSE {
    DWORD dwResponse;
} LOGON_ERROR_REDIRECTOR_RESPONSE;
#endif

class CErrorRedirector : public IUnknown
{
public:
    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, void** ppvObject) override;
    STDMETHOD_(ULONG, AddRef)() override;
    STDMETHOD_(ULONG, Release)() override;
	HRESULT ConvertResponse(UINT* pResponse);
	HRESULT GetInstance(REFIID riid, void** ppvObject);
	HRESULT IsActive();
	HRESULT OnBeginPainting();
	HRESULT RedirectLogonError(
    long param1,
    long param2,
    wchar_t* param3,
    wchar_t* param4,
    UINT param5,
    LOGON_ERROR_REDIRECTOR_RESPONSE* param6,
    UINT* param7
    );
	HRESULT RedirectMessage(
    wchar_t* param1,
    wchar_t* param2,
    UINT param3,
    LOGON_ERROR_REDIRECTOR_RESPONSE* param4,
    UINT* param5
    );
	HRESULT RedirectStatus(
    wchar_t* param1,
    LOGON_ERROR_REDIRECTOR_RESPONSE* param2,
    UINT* param3
    );
	long m_cRef;
	static const GUID m_MyCLSID;
};

#endif // ERRORREDIRECTOR_H

