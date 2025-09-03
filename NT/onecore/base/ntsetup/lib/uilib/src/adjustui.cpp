#include "pch.h"
#include <cstdint>
#include <new>

typedef unsigned int uint;

// Forward declarations for classes and structs
struct tagDLGTEMPLATEEX;
struct tagDLGTMP_FONTINFO;

// Structure definitions
struct tagDLGTEMPLATEEX
{
    uint32_t style;
    uint32_t exStyle;
    // Other members would be defined here based on the actual structure
};

struct tagDLGTMP_FONTINFO
{
    uint16_t pointsize;
    // Other members would be defined here based on the actual structure
};

class CDlgInfo
{
public:
    uint32_t m_fState;
    tagDLGTEMPLATEEX* m_pdt;
    wchar_t* m_pszFontName;
    char _padding_[4];
    
    // Declare constructor and methods
    CDlgInfo(HINSTANCE__* hInstance, uint param);
    tagDLGTMP_FONTINFO* v_GetFontInfo();
    tagDLGTEMPLATEEX* v_MakeTemplate();
    virtual ~CDlgInfo(); // Virtual destructor
};

int __cdecl FreeTemplate(tagDLGTEMPLATEEX* param_1)
{
    if (param_1 != nullptr)
    {
        GlobalFree(param_1);
    }
    return (param_1 != nullptr) ? 1 : 0;
}

tagDLGTEMPLATEEX* __cdecl MakeTemplate(HINSTANCE__* param_1, uint param_2, uint param_3, int param_4)
{
    tagDLGTEMPLATEEX* result = nullptr;
    
    if (param_1 == nullptr)
    {
        SetLastError(0x57);
        return result;
    }
    
    CDlgInfo* pDlgInfo = static_cast<CDlgInfo*>(operator new(0x18));
    if (pDlgInfo != nullptr)
    {
        pDlgInfo->CDlgInfo::CDlgInfo(param_1, param_2);
    }
    
    if (pDlgInfo == nullptr)
    {
        return result;
    }
    
    uint32_t dialogStyle = 0;
    uint32_t dialogExStyle = 0;
    
    if ((pDlgInfo->m_fState & 1) != 0)
    {
        dialogStyle = pDlgInfo->m_pdt->style;
        dialogExStyle = pDlgInfo->m_pdt->exStyle;
    }
    
    uint32_t adjustedStyle = dialogStyle & 0xEF3FFF7F;
    
    if ((dialogStyle & 0x48) == 0)
    {
        if (((pDlgInfo->m_fState & 1) != 0) &&
            (pDlgInfo->m_pdt->style = adjustedStyle, (pDlgInfo->m_fState & 1) != 0) &&
            (pDlgInfo->m_pdt->exStyle = dialogExStyle | 0x10000, (pDlgInfo->m_fState & 1) != 0))
        {
            result = pDlgInfo->v_MakeTemplate();
        }
    }
    else
    {
        if ((pDlgInfo->m_fState & 1) != 0)
        {
            tagDLGTMP_FONTINFO* fontInfo = pDlgInfo->v_GetFontInfo();
            if (fontInfo != nullptr)
            {
                fontInfo->pointsize = 9;
            }
        }
        
        if ((pDlgInfo->m_fState & 1) != 0)
        {
            if (pDlgInfo->m_pszFontName != nullptr)
            {
                GlobalFree(pDlgInfo->m_pszFontName);
            }
            
            wchar_t* fontName = static_cast<wchar_t*>(GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, 0x12));
            if (fontName != nullptr)
            {
                wcsncpy(fontName, L"Segoe UI", 9);
            }
            pDlgInfo->m_pszFontName = fontName;
            
            if (((pDlgInfo->m_fState & 1) != 0) &&
                (pDlgInfo->m_pdt->style = adjustedStyle, (pDlgInfo->m_fState & 1) != 0) &&
                (pDlgInfo->m_pdt->exStyle = dialogExStyle | 0x10000, (pDlgInfo->m_fState & 1) != 0))
            {
                result = pDlgInfo->v_MakeTemplate();
            }
        }
    }
    
    // Call virtual destructor
    pDlgInfo->~CDlgInfo();
    operator delete(pDlgInfo);
    
    return result;
}

