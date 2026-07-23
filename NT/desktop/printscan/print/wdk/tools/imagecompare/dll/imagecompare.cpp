/*
 * imagecompare.cpp
 *
 * Image comparison DLL using GDI+ C++ wrapper.
 * Exports: LoadImage1, CreateImage, DeleteImage, DownscaleImage, AnalyzeImage
 *
 * Logic is identical to the decompiled binary, but uses GDI+ C++ classes.
 */

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <tchar.h>
#include <stdarg.h>
#include <objidl.h>
#include <gdiplus.h>

#pragma comment(lib, "gdiplus.lib")

extern "C" void __cdecl _CIsqrt(void);

using namespace Gdiplus;

// ------------------------------------------------------------------
// Security cookie (as in decompiled)
// ------------------------------------------------------------------
static ULONG_PTR __security_cookie = 0xBB40E64E;

static void __security_check_cookie(ULONG_PTR cookie) {
    // no-op
}

// ------------------------------------------------------------------
// Compiler helpers for 64-bit arithmetic (as used in decompiled)
// ------------------------------------------------------------------
extern "C" long long __allmul(long long a, long long b) {
    return a * b;
}

extern "C" long long __alldiv(long long a, long long b) {
    return a / b;
}

// ------------------------------------------------------------------
// ImageInfo structure – exact layout from decompiled
// ------------------------------------------------------------------
typedef struct _IMAGE_INFO {
    int width;          // 0x00
    int stride;         // 0x04
    int height;         // 0x08
    int bytesPerPixel;  // 0x0C
    int rowPadding;     // 0x10
    BYTE* data;         // 0x14
} IMAGE_INFO;

// ------------------------------------------------------------------
// Internal function prototypes
// ------------------------------------------------------------------
static BOOL LoadImageInternal(Bitmap* pBitmap, IMAGE_INFO* pInfo);
static int ErrScanLine(const IMAGE_INFO* pSrc, const IMAGE_INFO* pDst, int x, int y, int width);
static int ErrScanLineAliasSafe(const IMAGE_INFO* pSrc, const IMAGE_INFO* pDst, int x, int y, int width);

// ------------------------------------------------------------------
// LoadImageInternal – exact logic, now using C++ wrapper
// ------------------------------------------------------------------
static BOOL LoadImageInternal(Bitmap* pBitmap, IMAGE_INFO* pInfo)
{
    Status status;
    UINT width = 0;
    UINT height = 0;
    UINT stride;
    BYTE* pixels = NULL;
    Rect rect(0, 0, 0, 0);
    BitmapData bitmapData = {0};

    pInfo->data = NULL;
    pInfo->width = 0;
    pInfo->height = 0;

    // Get width
    width = pBitmap->GetWidth(); status = Ok;
    if (status != Ok) {
        return FALSE;
    }
    pInfo->width = (int)width;

    // Get height
    height = pBitmap->GetHeight(); status = Ok;
    if (status != Ok) {
        return FALSE;
    }
    pInfo->height = (int)height;

    // Compute stride: width * 3 (assuming 24-bit RGB)
    stride = width * 3;
    pInfo->stride = (int)stride;
    pInfo->bytesPerPixel = 3;
    pInfo->rowPadding = 0;

    // Allocate buffer: stride * height (using __allmul in original)
    long long totalSize = __allmul((long long)stride, (long long)height);
    if (totalSize >= 0x80000000) {
        SetLastError(ERROR_ARITHMETIC_OVERFLOW);
        return FALSE;
    }

    pixels = (BYTE*)LocalAlloc(LMEM_FIXED, (UINT)totalSize);
    if (!pixels) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    // Lock bits with the buffer
    rect.X = 0;
    rect.Y = 0;
    rect.Width = width;
    rect.Height = height;

    bitmapData.Width = width;
    bitmapData.Height = height;
    bitmapData.Stride = stride;
    bitmapData.PixelFormat = PixelFormat24bppRGB;
    bitmapData.Scan0 = pixels;
    bitmapData.Reserved = 0;

    status = pBitmap->LockBits(&rect, ImageLockModeRead | ImageLockModeWrite,
                               PixelFormat24bppRGB, &bitmapData);
    if (status != Ok) {
        SetLastError(0xffffffff);
        LocalFree(pixels);
        return FALSE;
    }

    // Unlock bits – the data is now in pixels.
    status = pBitmap->UnlockBits(&bitmapData);
    if (status != Ok) {
        SetLastError(0xffffffff);
        LocalFree(pixels);
        return FALSE;
    }

    pInfo->data = pixels;
    return TRUE;
}

// ------------------------------------------------------------------
// LoadImage1 – exported
// ------------------------------------------------------------------
extern "C" __declspec(dllexport) BOOL WINAPI LoadImage1(LPCWSTR filename, IMAGE_INFO* pInfo)
{
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Bitmap* pBitmap = NULL;
    BOOL result = FALSE;

    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    pBitmap = Bitmap::FromFile(filename);
    if (pBitmap && pBitmap->GetLastStatus() == Ok) {
        result = LoadImageInternal(pBitmap, pInfo);
        delete pBitmap;
    } else {
        SetLastError(0xffffffff);
    }

    GdiplusShutdown(gdiplusToken);
    return result;
}

// ------------------------------------------------------------------
// CreateImage – exported
// ------------------------------------------------------------------
extern "C" __declspec(dllexport) BOOL WINAPI CreateImage(IMAGE_INFO* pInfo)
{
    long long totalSize;
    BYTE* pixels;

    if ((long long)pInfo->width * pInfo->bytesPerPixel < 0) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    pInfo->stride = pInfo->width * pInfo->bytesPerPixel;
    pInfo->rowPadding = 0;

    totalSize = (long long)pInfo->stride * pInfo->height;
    if (totalSize < 0 || totalSize >= 0x80000000) {
        SetLastError(ERROR_ARITHMETIC_OVERFLOW);
        return FALSE;
    }

    pixels = (BYTE*)LocalAlloc(LMEM_FIXED, (UINT)totalSize);
    if (!pixels) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    pInfo->data = pixels;
    return TRUE;
}

// ------------------------------------------------------------------
// DeleteImage – exported
// ------------------------------------------------------------------
extern "C" __declspec(dllexport) BOOL WINAPI DeleteImage(IMAGE_INFO* pInfo)
{
    if (pInfo->data) {
        LocalFree(pInfo->data);
        pInfo->data = NULL;
    }
    return TRUE;
}

// ------------------------------------------------------------------
// ErrScanLine – exact decompiled logic
// ------------------------------------------------------------------
static int ErrScanLine(const IMAGE_INFO* pSrc, const IMAGE_INFO* pDst,
                       int x, int y, int widthInPixels)
{
    int error = 0;

    int sy = y;
    if (sy < 0) sy = 0;
    if (sy > pSrc->height - 1) sy = pSrc->height - 1;

    int sx = x;
    if (sx < 0) sx = 0;
    if (sx + widthInPixels > pSrc->width) sx = pSrc->width - widthInPixels;

    BYTE* srcRow = pSrc->data + sy * pSrc->stride + sx * pSrc->bytesPerPixel;
    BYTE* dstRow = pDst->data + y * pDst->stride + x * pDst->bytesPerPixel;

    int srcBpp = pSrc->bytesPerPixel;
    int srcStride = pSrc->stride;
    int dstStride = pDst->stride;

    for (int row = 0; row < widthInPixels; row++) {
        for (int col = 0; col < srcBpp; col++) {
            int diff = (int)srcRow[col] - (int)dstRow[col];
            error += diff * diff;
        }
        srcRow += srcStride;
        dstRow += dstStride;
    }

    return error;
}

// ------------------------------------------------------------------
// ErrScanLineAliasSafe – exact decompiled logic
// ------------------------------------------------------------------
static int ErrScanLineAliasSafe(const IMAGE_INFO* pSrc, const IMAGE_INFO* pDst,
                                int x, int y, int widthInPixels)
{
    int sy = y;
    if (sy < 1) sy = 1;
    if (sy > pSrc->height - 2) sy = pSrc->height - 2;

    int sx = x;
    if (sx < 1) sx = 1;
    if (sx + widthInPixels > pSrc->width - 1) sx = pSrc->width - widthInPixels - 1;

    int srcOffsets[5];
    int dstOffsets[5];

    srcOffsets[0] = -pSrc->stride;
    srcOffsets[1] = -pSrc->bytesPerPixel;
    srcOffsets[2] = 0;
    srcOffsets[3] = pSrc->bytesPerPixel;
    srcOffsets[4] = pSrc->stride;

    dstOffsets[0] = -pDst->stride;
    dstOffsets[1] = -pDst->bytesPerPixel;
    dstOffsets[2] = 0;
    dstOffsets[3] = pDst->bytesPerPixel;
    dstOffsets[4] = pDst->stride;

    BYTE* srcBase = pSrc->data + sy * pSrc->stride + sx * pSrc->bytesPerPixel;
    BYTE* dstBase = pDst->data + y * pDst->stride + x * pDst->bytesPerPixel;

    int error = 0;
    int bpp = pSrc->bytesPerPixel;

    for (int row = 0; row < widthInPixels; row++) {
        for (int col = 0; col < bpp; col++) {
            BYTE minSrc = 255;
            BYTE maxSrc = 0;
            for (int i = 0; i < 5; i++) {
                BYTE val = *(srcBase + srcOffsets[i] + col);
                if (val < minSrc) minSrc = val;
                if (val > maxSrc) maxSrc = val;
            }

            BYTE minDst = 255;
            BYTE maxDst = 0;
            for (int i = 0; i < 5; i++) {
                BYTE val = *(dstBase + dstOffsets[i] + col);
                if (val < minDst) minDst = val;
                if (val > maxDst) maxDst = val;
            }

            BYTE srcCenter = *(srcBase + col);
            BYTE dstCenter = *(dstBase + col);

            if (srcCenter != dstCenter) {
                int diff = 0;
                if (dstCenter > maxSrc) {
                    diff = dstCenter - maxSrc;
                } else if (dstCenter < minSrc) {
                    diff = minSrc - dstCenter;
                }
                if (diff == 0) {
                    if (srcCenter > maxDst) {
                        diff = srcCenter - maxDst;
                    } else if (srcCenter < minDst) {
                        diff = minDst - srcCenter;
                    }
                }
                error += diff * diff;
            }
        }
        srcBase += pSrc->stride;
        dstBase += pDst->stride;
    }

    return error;
}

// ------------------------------------------------------------------
// AnalyzeImage – exported
// ------------------------------------------------------------------
extern "C" __declspec(dllexport) BOOL WINAPI AnalyzeImage(
    const IMAGE_INFO* pSrc,
    const IMAGE_INFO* pDst,
    int blockSize,
    float threshold,
    BOOL useAliasSafe,
    float* pResults
)
{
    if (!pSrc || !pDst || !pResults) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (pSrc->width <= 0 || pSrc->height <= 0 || pDst->width <= 0 || pDst->height <= 0 ||
        pSrc->width > pDst->width || pSrc->height > pDst->height ||
        pSrc->bytesPerPixel != pDst->bytesPerPixel ||
        blockSize < 16 ||
        pSrc->bytesPerPixel != 3) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    float bestError = -1.0f;
    int bestX = 0, bestY = 0;
    int bestEndX = 0, bestEndY = 0;

    for (int y = 0; y <= pSrc->height - blockSize; y += blockSize) {
        for (int x = 0; x <= pSrc->width - blockSize; x += blockSize) {
            int error = 0;
            for (int row = 0; row < blockSize; row++) {
                if (useAliasSafe) {
                    error += ErrScanLineAliasSafe(pSrc, pDst, x, y + row, blockSize);
                } else {
                    error += ErrScanLine(pSrc, pDst, x, y + row, blockSize);
                }
            }
            float fError = (float)error;
            if (bestError < 0.0f || fError < bestError) {
                bestError = fError;
                bestX = x;
                bestY = y;
                bestEndX = x + blockSize;
                bestEndY = y + blockSize;
            }
        }
    }

    // Use _CIsqrt – it takes its argument from the FPU stack and returns there
    float rms;
    float val = bestError / (float)(blockSize * blockSize * pSrc->bytesPerPixel);
    __asm {
        fld val
        call _CIsqrt
        fstp rms
    }

    pResults[0] = rms;
    pResults[1] = (float)bestX;
    pResults[2] = (float)bestY;
    pResults[3] = (float)bestEndX;
    pResults[4] = (float)bestEndY;
    pResults[5] = 0.0f;

    return TRUE;
}

// ------------------------------------------------------------------
// DownscaleImage – exported
// ------------------------------------------------------------------
extern "C" __declspec(dllexport) BOOL WINAPI DownscaleImage(
    const IMAGE_INFO* pSrc,
    IMAGE_INFO* pDst
)
{
    if (!pSrc || !pDst || pSrc->width < pDst->width || pSrc->height < pDst->height ||
        pSrc->bytesPerPixel != pDst->bytesPerPixel) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    int srcWidth = pSrc->width;
    int srcHeight = pSrc->height;
    int dstWidth = pDst->width;
    int dstHeight = pDst->height;

    for (int y = 0; y < dstHeight; y++) {
        long long srcY = __alldiv((long long)y * srcHeight + srcHeight / 2, dstHeight);
        for (int x = 0; x < dstWidth; x++) {
            long long srcX = __alldiv((long long)x * srcWidth + srcWidth / 2, dstWidth);
            BYTE* srcPixel = pSrc->data + (int)srcY * pSrc->stride + (int)srcX * pSrc->bytesPerPixel;
            BYTE* dstPixel = pDst->data + y * pDst->stride + x * pDst->bytesPerPixel;
            memcpy(dstPixel, srcPixel, pSrc->bytesPerPixel);
        }
    }

    return TRUE;
}

