extern "C" unsigned long GetFontOverrides(
    void** ppFontTableWin8,
    int* pFontTableWin8Size,
    void** ppFontTableWinBlue, 
    int* pFontTableWinBlueSize,
    void** ppFontTableWin10Desktop,
    int* pFontTableWin10DesktopSize
)
{
    // These font tables are defined in other source files and linked externally
    *ppFontTableWin8 = 0;
    *pFontTableWin8Size = 0x34;
    *ppFontTableWinBlue = 0;
    *pFontTableWinBlueSize = 0x3A;
    *ppFontTableWin10Desktop = 0;
    *pFontTableWin10DesktopSize = 0x4A;
    return 0;
}

