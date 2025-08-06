#include "precomp.h"
#include <tchar.h>
#include <stdarg.h>
#include <objidl.h>
#include <gdiplus.h>
#include <shlwapi.h>
#include <msi.h>
#include <msiquery.h>
#include <commctrl.h>
#include <shellapi.h>
#include <vector>
#include <string>
#include <cwchar>
#include <cstdio>
#include <iostream>
#include <wdslog.h>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "msi.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")

using namespace Gdiplus;

// --- Logging stubs for specific functions ---
void WdsSetupLogMessageW(void* msg, DWORD a, const wchar_t* b, DWORD c, DWORD d,
                        const wchar_t* file, const wchar_t* func,
                        DWORD ip, DWORD err, DWORD arg1, DWORD arg2)
{
    wchar_t buffer[1024];
    // Use StringCchPrintfW instead of swprintf_s for better safety
    StringCchPrintfW(buffer, _countof(buffer),
                    L"[LOG] %s in %s, error: %d",
                    func, file, err);
    
    std::wcerr << buffer << std::endl;
}

// --- Path helpers ---
void PathRemoveBlanksW(wchar_t* pszPath) { ::PathRemoveBlanksW(pszPath); }

// --- CheckColorBitsNonZeroAlpha ---
bool CheckColorBitsNonZeroAlpha(const RGBQUAD* bits, int width, int height) {
    for (int y = 0; y < height; ++y) {
        const RGBQUAD* row = bits + y * width;
        for (int x = 0; x < width; ++x)
            if (row[x].rgbReserved != 0) return true;
    }
    return false;
}

// --- CreatePngGraphicFromMonochrome ---
Bitmap* CreatePngGraphicFromMonochrome(HICON hIcon) {
    if (!hIcon) return nullptr;
    UINT width = GetSystemMetrics(SM_CXICON);
    UINT height = GetSystemMetrics(SM_CYICON);
    Bitmap* bmp = new Bitmap(width, height, PixelFormat32bppARGB);

    // Draw icon using GDI
    HDC hDC = CreateCompatibleDC(nullptr);
    HBITMAP hBitmap = CreateCompatibleBitmap(hDC, width, height);
    HBITMAP oldBmp = (HBITMAP)SelectObject(hDC, hBitmap);
    RECT rect = {0, 0, (LONG)width, (LONG)height};
    FillRect(hDC, &rect, (HBRUSH)GetStockObject(WHITE_BRUSH));
    DrawIconEx(hDC, 0, 0, hIcon, width, height, 0, nullptr, DI_NORMAL);
    SelectObject(hDC, oldBmp);

    // Transfer to GDI+ Bitmap
    Bitmap bitmap(hBitmap, nullptr);
    Gdiplus::Graphics graphics(bmp);
    graphics.DrawImage(&bitmap, 0, 0);

    DeleteObject(hBitmap);
    DeleteDC(hDC);
    return bmp;
}

// --- GetEncoderClsid ---
int GetEncoderClsid(const WCHAR* format, CLSID* pClsid) {
    UINT num = 0, size = 0;
    if (GetImageEncodersSize(&num, &size) != Ok || num == 0 || size == 0)
        return -1;
    std::vector<ImageCodecInfo> codecs(num);
    if (GetImageEncoders(num, size, &codecs[0]) != Ok)
        return -1;
    for (UINT j = 0; j < num; ++j) {
        if (wcscmp(codecs[j].MimeType, format) == 0) {
            *pClsid = codecs[j].Clsid;
            return static_cast<int>(j);
        }
    }
    return -1;
}

// --- CreatePngGraphic (fully implemented) ---
int CreatePngGraphic(HICON hIcon, const wchar_t* outPath) {
    if (!hIcon || !outPath) return -1;

    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    if (GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr) != Ok) return -1;

    ICONINFO iconInfo = {0};
    if (!GetIconInfo(hIcon, &iconInfo)) {
        GdiplusShutdown(gdiplusToken);
        return -1;
    }

    BITMAP bmpInfo = {0};
    if (!GetObject(iconInfo.hbmColor, sizeof(BITMAP), &bmpInfo)) {
        if (iconInfo.hbmColor) DeleteObject(iconInfo.hbmColor);
        if (iconInfo.hbmMask) DeleteObject(iconInfo.hbmMask);
        GdiplusShutdown(gdiplusToken);
        return -1;
    }

    Bitmap* bmp = nullptr;
    if (bmpInfo.bmBitsPixel == 32) {
        bmp = new Bitmap(bmpInfo.bmWidth, bmpInfo.bmHeight, PixelFormat32bppARGB);

        // Draw icon using GDI
        HDC hDC = CreateCompatibleDC(nullptr);
        HBITMAP hBitmap = CreateCompatibleBitmap(hDC, bmpInfo.bmWidth, bmpInfo.bmHeight);
        HBITMAP oldBmp = (HBITMAP)SelectObject(hDC, hBitmap);
        RECT rect = {0, 0, bmpInfo.bmWidth, bmpInfo.bmHeight};
        FillRect(hDC, &rect, (HBRUSH)GetStockObject(WHITE_BRUSH));
        DrawIconEx(hDC, 0, 0, hIcon, bmpInfo.bmWidth, bmpInfo.bmHeight, 0, nullptr, DI_NORMAL);
        SelectObject(hDC, oldBmp);

        // Transfer to GDI+ Bitmap
        Bitmap bitmap(hBitmap, nullptr);
        Gdiplus::Graphics graphics(bmp);
        graphics.DrawImage(&bitmap, 0, 0);

        DeleteObject(hBitmap);
        DeleteDC(hDC);
    } else {
        bmp = CreatePngGraphicFromMonochrome(hIcon);
    }

    int ret = -1;
    if (bmp) {
        CLSID pngClsid;
        if (GetEncoderClsid(L"image/png", &pngClsid) >= 0) {
            if (bmp->Save(outPath, &pngClsid, nullptr) == Ok)
                ret = 0;
        }
        delete bmp;
    }

    if (iconInfo.hbmColor) DeleteObject(iconInfo.hbmColor);
    if (iconInfo.hbmMask) DeleteObject(iconInfo.hbmMask);
    GdiplusShutdown(gdiplusToken);
    return ret;
}

// --- GetIconFromBinary ---
int GetIconFromBinary(const wchar_t* filePath, int iconIndex, const wchar_t* outPath) {
    HICON hIcon = nullptr;
    UINT count = ::ExtractIconExW(filePath, iconIndex, nullptr, &hIcon, 1);
    if (count > 0 && hIcon) {
        int result = CreatePngGraphic(hIcon, outPath);
        DestroyIcon(hIcon);
        return result;
    }
    return -1;
}

// --- GetIconFromMsi (with MSI internals and error logging) ---
int GetIconFromMsi(const wchar_t* msiPath, int iconIndex, const wchar_t* outPath) {
    MSIHANDLE hDatabase = 0, hView = 0, hRecord = 0;
    int result = -1;
    wchar_t iconName[256] = {0};
    wchar_t query[512] = {0};

    // Open database (second argument L"MSIDBOPEN_READONLY")
    if (MsiOpenDatabaseW(msiPath, L"MSIDBOPEN_READONLY", &hDatabase) != ERROR_SUCCESS) {
        WdsSetupLogMessageW(nullptr, 0, L"D", 0, 0x1E3,
            L"onecore\\base\\appcompat\\shared\\unmanaged\\appdeviceicon\\lib\\appicon.cpp",
            L"GetIconFromMsi", 0, GetLastError(), 0, 0);
        return -1;
    }

    // Get ARP PRODUCTICON property name
    StringCchPrintfW(query, _countof(query),
        L"SELECT `Property`.`Value` FROM `Property` WHERE `Property`.`Property`='ARP PRODUCTICON'");
    if (MsiDatabaseOpenViewW(hDatabase, query, &hView) != ERROR_SUCCESS) goto cleanup;
    if (MsiViewExecute(hView, 0) != ERROR_SUCCESS) goto cleanup;
    if (MsiViewFetch(hView, &hRecord) != ERROR_SUCCESS) goto cleanup;
    DWORD cchIconName = 255;
    if (MsiRecordGetStringW(hRecord, 1, iconName, &cchIconName) != ERROR_SUCCESS) goto cleanup;
    MsiCloseHandle(hRecord);
    hRecord = 0;
    MsiCloseHandle(hView);
    hView = 0;

    // Now, query icon binary stream
    StringCchPrintfW(query, _countof(query),
        L"SELECT `Icon`.`Data` FROM `Icon` WHERE `Icon`.`Name`='%s'", iconName);
    if (MsiDatabaseOpenViewW(hDatabase, query, &hView) != ERROR_SUCCESS) goto cleanup;
    if (MsiViewExecute(hView, 0) != ERROR_SUCCESS) goto cleanup;
    if (MsiViewFetch(hView, &hRecord) != ERROR_SUCCESS) goto cleanup;

    // Create temp file to store icon data
    wchar_t tempPath[MAX_PATH], tempFile[MAX_PATH];
    GetTempPathW(MAX_PATH, tempPath);
    GetTempFileNameW(tempPath, L"MSI", 0, tempFile);
    HANDLE hFile = CreateFileW(tempFile, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, 0);
    if (hFile == INVALID_HANDLE_VALUE) goto cleanup;

    // Read stream from MSI and write to temp file
    char buf[1024];
    DWORD cbRead = sizeof(buf);
    UINT rc;
    do {
        rc = MsiRecordReadStream(hRecord, 1, buf, &cbRead);
        if (cbRead > 0) {
            DWORD cbWritten;
            WriteFile(hFile, buf, cbRead, &cbWritten, nullptr);
        }
    } while (rc == ERROR_SUCCESS && cbRead == sizeof(buf));
    CloseHandle(hFile);

    // Use ExtractIconExW to get icon from the temp file
    result = GetIconFromBinary(tempFile, iconIndex, outPath);

    // Clean up temp file
    DeleteFileW(tempFile);

cleanup:
    if (hRecord) MsiCloseHandle(hRecord);
    if (hView) MsiCloseHandle(hView);
    if (hDatabase) MsiCloseHandle(hDatabase);
    if (result < 0) {
        WdsSetupLogMessageW(nullptr, 0, L"D", 0, 0x1E3,
            L"onecore\\base\\appcompat\\shared\\unmanaged\\appdeviceicon\\lib\\appicon.cpp",
            L"GetIconFromMsi", 0, GetLastError(), 0, 0);
    }
    return result;
}

// --- GetIconFromUnknown ---
int GetIconFromUnknown(const wchar_t* filePath, int iconIndex, const wchar_t* outPath) {
    SHFILEINFOW sfi = {0};
    DWORD dwFlags = SHGFI_ICON | SHGFI_LARGEICON | SHGFI_USEFILEATTRIBUTES;
    DWORD dwAttr = FILE_ATTRIBUTE_NORMAL;
    if (!SHGetFileInfoW(filePath, dwAttr, &sfi, sizeof(sfi), dwFlags)) {
        return -1;
    }

    int result = -1;
    if (sfi.hIcon) {
        result = CreatePngGraphic(sfi.hIcon, outPath);
        DestroyIcon(sfi.hIcon);
    } else {
        HICON hIcon = nullptr;
        UINT count = ExtractIconExW(filePath, iconIndex, nullptr, &hIcon, 1);
        if (count > 0 && hIcon) {
            result = CreatePngGraphic(hIcon, outPath);
            DestroyIcon(hIcon);
        }
    }
    return result;
}

// --- GetIconFromProduct ---
int GetIconFromProduct(const wchar_t* productCode, int iconIndex, const wchar_t* outPath) {
    DWORD localPackageLen = 0;
    UINT res = MsiGetProductInfoW(productCode, L"LocalPackage", nullptr, &localPackageLen);
    std::wstring localPackage;
    if (res == ERROR_MORE_DATA) {
        localPackage.resize(localPackageLen);
        if (MsiGetProductInfoW(productCode, L"LocalPackage", &localPackage[0], &localPackageLen) != ERROR_SUCCESS) {
            return -1;
        }
    } else {
        return -1;
    }
    return GetIconFromMsi(localPackage.c_str(), iconIndex, outPath);
}

// --- GetApplicationIcon (with source file error logging) ---
int GetApplicationIcon(const wchar_t* filePath, int iconIndex, const wchar_t* outPath, const wchar_t* productCode) {
    if (!filePath || !outPath) return -1;

    wchar_t localPath[MAX_PATH];
    wcsncpy_s(localPath, filePath, _TRUNCATE);
    PathUnquoteSpacesW(localPath);
    PathRemoveBlanksW(localPath);

    const wchar_t* ext = PathFindExtensionW(localPath);
    if (!ext) return -1;

    int result = -1;

    if (_wcsicmp(ext, L".ico") == 0 || _wcsicmp(ext, L".dll") == 0) {
        result = GetIconFromBinary(localPath, iconIndex, outPath);
    } else if (_wcsicmp(ext, L".msi") == 0) {
        result = GetIconFromMsi(localPath, iconIndex, outPath);
    } else if (_wcsicmp(localPath, L"msiexec.exe") == 0 && productCode) {
        result = GetIconFromProduct(productCode, iconIndex, outPath);
    } else {
        result = GetIconFromUnknown(localPath, iconIndex, outPath);
    }

    if (result < 0) {
        WdsSetupLogMessageW(nullptr, 0, L"D", 0, 0x34F,
            L"onecore\\base\\appcompat\\shared\\unmanaged\\appdeviceicon\\lib\\appicon.cpp",
            L"GetApplicationIcon", 0, GetLastError(), 0, 0);
    }

    return result;
}

