#include "pch.h"
#include <cstring>
#include <cstdint>        // Add this for uint16_t, uint32_t
#include "utlstring.h"

// Type definitions
typedef unsigned char uchar;
typedef unsigned long ulong;
typedef unsigned int uint;
typedef unsigned short ushort;

// Forward declarations
struct tagDLGTEMPLATEEX;
struct tagDLGTMP_FONTINFO;
class CDlgInfo;

// Structure definitions
struct tagDLGTEMPLATEEX {
    uint16_t dlgVer;
    uint16_t signature;
    uint32_t helpID;
    uint32_t exStyle;
    uint32_t style;
    uint16_t cDlgItems;
    short x;
    short y;
    short cx;
    short cy;
};

struct tagDLGTMP_FONTINFO {
    wchar_t* typeface;
    uint16_t pointsize;
    uint16_t weight;
    uchar italic;
    uchar charset;
};

class CDlgInfo {
public:
    uint32_t m_fState;
    tagDLGTEMPLATEEX* m_pdt;
    wchar_t* m_pszFontName;
    uint32_t m_dwResCharSet;
    uint32_t m_cbSize;
    int _padding_;
    
    CDlgInfo(HINSTANCE__* instance, uint resourceId);
    ~CDlgInfo();
    tagDLGTMP_FONTINFO* GetFontInfo();
    tagDLGTEMPLATEEX* MakeTemplate();
    wchar_t* GetFontName();
    void GetScalingFactors(float* scaleX, float* scaleY, int* textMetrics);
};

int __cdecl dlt_GetDefInfo(uchar* outputBuffer, tagDLGTEMPLATEEX* dialogTemplate, int* param3, uchar** param4)
{
    short* templateData = reinterpret_cast<short*>(param4);
    int outputOffset = reinterpret_cast<int>(param3);
    
    memset(outputBuffer, 0, 4);
    dialogTemplate->dlgVer = 0;
    dialogTemplate->signature = 0;
    
    int result = 1;
    
    if (templateData[1] == -1) {
        if (templateData[0] == 1) {
            outputBuffer[0] = 1;
            memset(outputBuffer + 1, 0, 3);
            *reinterpret_cast<uint32_t*>(outputOffset + 0xC) = *reinterpret_cast<uint32_t*>(templateData + 6);
            *reinterpret_cast<uint32_t*>(outputOffset + 8) = *reinterpret_cast<uint32_t*>(templateData + 4);
            *reinterpret_cast<short*>(outputOffset + 0x12) = templateData[9];
            *reinterpret_cast<short*>(outputOffset + 0x14) = templateData[10];
            *reinterpret_cast<short*>(outputOffset + 0x16) = templateData[11];
            *reinterpret_cast<short*>(outputOffset + 0x18) = templateData[12];
            *reinterpret_cast<short*>(outputOffset + 0x10) = templateData[8];
            *reinterpret_cast<short**>(dialogTemplate) = templateData + 13;
        } else {
            result = 0;
        }
    } else {
        uint32_t tempValue = *reinterpret_cast<uint32_t*>(templateData);
        *reinterpret_cast<short*>(outputOffset + 0x16) = templateData[7];
        short tempShort1 = templateData[8];
        *reinterpret_cast<uint32_t*>(outputOffset + 0xC) = tempValue;
        *reinterpret_cast<uint32_t*>(outputOffset + 8) = *reinterpret_cast<uint32_t*>(templateData + 2);
        short tempShort2 = templateData[5];
        *reinterpret_cast<short*>(outputOffset + 0x18) = tempShort1;
        tempShort1 = templateData[4];
        *reinterpret_cast<short*>(outputOffset + 0x12) = tempShort2;
        tempShort2 = templateData[6];
        *reinterpret_cast<short*>(outputOffset + 0x10) = tempShort1;
        *reinterpret_cast<short*>(outputOffset + 0x14) = tempShort2;
        *reinterpret_cast<short**>(dialogTemplate) = templateData + 9;
    }
    return result;
}

uchar* __cdecl dlt_SkipDlgString(uchar* stringPointer)
{
    short* currentPos = reinterpret_cast<short*>(stringPointer);
    
    if (currentPos[0] == -1) {
        return reinterpret_cast<uchar*>(currentPos + 2);
    }
    
    short currentChar;
    do {
        currentChar = *currentPos;
        currentPos = currentPos + 1;
    } while (currentChar != 0);
    
    return reinterpret_cast<uchar*>(currentPos);
}

ulong __cdecl dlt_GetDlgSize(uchar* dialogData, int* param2)
{
    ulong resultSize = 0;
    int* dataPointer = reinterpret_cast<int*>(param2);
    
    int defInfoResult = dlt_GetDefInfo(reinterpret_cast<uchar*>(dataPointer), reinterpret_cast<tagDLGTEMPLATEEX*>(&resultSize), nullptr, nullptr);
    
    if (defInfoResult != 0) {
        dlt_SkipDlgString(reinterpret_cast<uchar*>(dataPointer));
        dlt_SkipDlgString(reinterpret_cast<uchar*>(dataPointer));
        uchar* skippedString = dlt_SkipDlgString(reinterpret_cast<uchar*>(dataPointer));
        
        resultSize = (reinterpret_cast<ulong>(skippedString + 3) & 0xFFFFFFFC) - reinterpret_cast<ulong>(dialogData);
    }
    return resultSize;
}

tagDLGTEMPLATEEX* __cdecl dlt_MakeCopy(uchar* sourceData, ulong dataSize)
{
    tagDLGTEMPLATEEX* copiedTemplate = static_cast<tagDLGTEMPLATEEX*>(GlobalAlloc(GMEM_ZEROINIT, dataSize));
    if (copiedTemplate != nullptr) {
        memcpy(copiedTemplate, sourceData, dataSize);
    }
    return copiedTemplate;
}

tagDLGTEMPLATEEX* __cdecl dlt_LoadTemplate(HINSTANCE__* instance, uint resourceId, ulong* templateSize)
{
    tagDLGTEMPLATEEX* resultTemplate = nullptr;
    
    if (instance == nullptr) {
        SetLastError(0x57);
        return resultTemplate;
    }
    
    HRSRC resourceHandle = FindResourceW(instance, MAKEINTRESOURCEW(resourceId), (LPCWSTR)RT_DIALOG);
    if (resourceHandle != nullptr) {
        HGLOBAL loadedResource = LoadResource(instance, resourceHandle);
        if (loadedResource != nullptr) {
            uchar* resourceData = reinterpret_cast<uchar*>(LockResource(loadedResource));
            if (resourceData != nullptr) {
                ulong dialogSize = dlt_GetDlgSize(resourceData, nullptr);
                if (dialogSize != 0) {
                    resultTemplate = dlt_MakeCopy(resourceData, dialogSize);
                }
            }
        }
    }
    return resultTemplate;
}

// CDlgInfo class implementation
CDlgInfo::CDlgInfo(HINSTANCE__* instance, uint resourceId)
{
    m_fState = 0;
    _padding_ = 0;
    m_pszFontName = nullptr;
    m_dwResCharSet = 0;
    m_pdt = dlt_LoadTemplate(instance, resourceId, nullptr);
    
    if (m_pdt != nullptr) {
        m_fState |= 1;
    }
}

CDlgInfo::~CDlgInfo()
{
    if (m_pdt != nullptr) {
        GlobalFree(m_pdt);
    }
    if (m_pszFontName != nullptr) {
        GlobalFree(m_pszFontName);
    }
}

tagDLGTMP_FONTINFO* CDlgInfo::GetFontInfo()
{
    if ((m_fState & 1) == 0) {
        return nullptr;
    }
    
    if ((m_pdt->style & 0x48) != 0) {
        uchar* currentPos = reinterpret_cast<uchar*>(m_pdt);
        dlt_SkipDlgString(currentPos);
        dlt_SkipDlgString(currentPos);
        return reinterpret_cast<tagDLGTMP_FONTINFO*>(dlt_SkipDlgString(currentPos));
    }
    return nullptr;
}

tagDLGTEMPLATEEX* CDlgInfo::MakeTemplate()
{
    tagDLGTMP_FONTINFO* fontInfo = GetFontInfo();
    if (fontInfo == nullptr || m_pszFontName == nullptr) {
        return dlt_MakeCopy(reinterpret_cast<uchar*>(m_pdt), m_cbSize);
    }
    
    wchar_t* fontNamePtr = m_pszFontName;
    wchar_t* currentChar = fontNamePtr;
    while (*currentChar != L'\0') {
        currentChar++;
    }
    
    int fontNameLength = (currentChar - fontNamePtr);
    if (fontNameLength != 0) {
        wchar_t* fontTypeface = fontInfo->typeface;
        wchar_t* typefaceCurrent = fontTypeface;
        while (*typefaceCurrent != L'\0') {
            typefaceCurrent++;
        }
        
        ulong newSize = fontNameLength * 2 + 5 + (m_cbSize - ((typefaceCurrent - fontTypeface) * 2 + 2)) & 0xFFFFFFFC;
        m_dwResCharSet = fontInfo->charset;
        fontInfo->charset = 0;
        
        tagDLGTEMPLATEEX* newTemplate = static_cast<tagDLGTEMPLATEEX*>(GlobalAlloc(GMEM_ZEROINIT, newSize));
        if (newTemplate == nullptr) {
            return nullptr;
        }
        
        memcpy(newTemplate, m_pdt, m_cbSize);
        StringCchCopyW(reinterpret_cast<wchar_t*>(newTemplate + m_cbSize), newSize, m_pszFontName);
        memcpy(reinterpret_cast<uchar*>(newTemplate) + m_cbSize, m_pszFontName, fontNameLength * 2 + 2);
        
        return newTemplate;
    }
    
    return dlt_MakeCopy(reinterpret_cast<uchar*>(m_pdt), m_cbSize);
}

wchar_t* CDlgInfo::GetFontName()
{
    wchar_t* result = nullptr;
    
    if ((m_fState & 1) != 0) {
        tagDLGTMP_FONTINFO* fontInfo = GetFontInfo();
        if (fontInfo != nullptr) {
            result = fontInfo->typeface;
        }
    }
    return result;
}

void CDlgInfo::GetScalingFactors(float* scaleX, float* scaleY, int* textMetrics)
{
    if ((m_fState & 1) != 0) {
        tagDLGTMP_FONTINFO* fontInfo = GetFontInfo();
        if (fontInfo != nullptr) {
            LOGFONTW logFont = {0};
            logFont.lfHeight = -MulDiv(fontInfo->pointsize, GetDeviceCaps(GetDC(nullptr), LOGPIXELSY), 72);
            logFont.lfWeight = fontInfo->weight;
            logFont.lfItalic = fontInfo->italic;
            logFont.lfCharSet = fontInfo->charset;
            wcscpy_s(logFont.lfFaceName, LF_FACESIZE, m_pszFontName ? m_pszFontName : fontInfo->typeface);

            HFONT hFont = CreateFontIndirectW(&logFont);
            if (hFont != nullptr) {
                HDC hDC = CreateCompatibleDC(nullptr);
                if (hDC != nullptr) {
                    HGDIOBJ hOldFont = SelectObject(hDC, hFont);
                    
                    TEXTMETRICW tm;
                    GetTextMetricsW(hDC, &tm);
                    
                    SIZE textSize;
                    const wchar_t* testString = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
                    GetTextExtentPoint32W(hDC, testString, 52, &textSize);
                    
                    if (textMetrics != nullptr) {
                        *textMetrics = tm.tmPitchAndFamily == TMPF_TRUETYPE ? 1 : 0;
                    }
                    
                    if (scaleX != nullptr) {
                        *scaleX = static_cast<float>(textSize.cx) / 520.0f;
                    }
                    
                    if (scaleY != nullptr) {
                        *scaleY = static_cast<float>(tm.tmHeight) / 15.0f;
                    }
                    
                    SelectObject(hDC, hOldFont);
                    DeleteDC(hDC);
                }
                DeleteObject(hFont);
            }
        }
    }
}

