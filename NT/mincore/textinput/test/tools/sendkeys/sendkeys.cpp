/*
 * sendkeys.cpp – exact reconstruction from decompiled binary
 * Includes all five error format strings (1, 2, 3, 4, 5).
 */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <string.h>

// ------------------------------------------------------------------
// Security cookie stubs
// ------------------------------------------------------------------
static void __security_check_cookie(uintptr_t cookie) { /* no-op */ }
static uintptr_t __security_cookie = 0xBB40E64E;

// ------------------------------------------------------------------
// Guard check nop
// ------------------------------------------------------------------
#define _guard_check_icall_nop(x) ((void)0)

// ------------------------------------------------------------------
// Stubs for MSVCRT functions
// ------------------------------------------------------------------
int __cdecl _callnewh(size_t size) { return 0; }
void __cdecl __report_rangecheckfailure() { OutputDebugStringA("range check failure\n"); }

// ------------------------------------------------------------------
// Custom new/delete (exact from decompiled)
// ------------------------------------------------------------------
void* __cdecl operator_new(size_t size) {
    void* p;
    while ((p = malloc(size)) == NULL) {
        if (_callnewh(size) == 0)
            break;
    }
    return p;
}

void __cdecl operator_delete(void* p) { free(p); }

// ------------------------------------------------------------------
// Function pointer types for inputprocessorclient.dll
// ------------------------------------------------------------------
typedef void (WINAPI *PFN_Initialize)(void);
typedef void (WINAPI *PFN_Deinitialize)(void);
typedef void (WINAPI *PFN_OnStringKey)(int param, wchar_t key);
typedef void (WINAPI *PFN_OnVirtualKey)(int param, int key);

// ------------------------------------------------------------------
// usage()
// ------------------------------------------------------------------
void __cdecl usage(void) {
    printf("SendKeys: [-v] \"text or key events to send\"\r\n");
    printf("-v will send virtual key code instead of string events in the form of 0x26 0x25 ...\r\n");
    printf("    for quick reference:\r\n");
    printf("       VK_TAB            0x09\r\n");
    printf("       VK_RETURN         0x0D\r\n");
    printf("       VK_LEFT           0x25\r\n");
    printf("       VK_UP             0x26\r\n");
    printf("       VK_RIGHT          0x27\r\n");
    printf("       VK_DOWN           0x28\r\n");
}

// ------------------------------------------------------------------
// SendKeys – exact decompiled logic with all error messages
// ------------------------------------------------------------------
void __cdecl SendKeys(int virtualMode, wchar_t* lpString) {
    if (lpString == NULL)
        return;

    size_t len = wcslen(lpString);
    size_t numChars = (len > 0) ? len : 0;

    HMODULE hMod = LoadLibraryW(L"inputprocessorclient.dll");
    if (!hMod) {
        printf("Failed to load inputprocessorclient.dll\n");
        return;
    }

    PFN_Initialize pfnInit = (PFN_Initialize)GetProcAddress(hMod, "Initialize");
    PFN_Deinitialize pfnDeinit = (PFN_Deinitialize)GetProcAddress(hMod, "Deinitialize");
    PFN_OnStringKey pfnStringKey = (PFN_OnStringKey)GetProcAddress(hMod, "OnStringKey");
    PFN_OnVirtualKey pfnVirtualKey = (PFN_OnVirtualKey)GetProcAddress(hMod, "OnVirtualKey");

    if (!pfnInit || !pfnDeinit || !pfnStringKey || !pfnVirtualKey) {
        printf("Failed to get function pointers from inputprocessorclient.dll\n");
        FreeLibrary(hMod);
        return;
    }

    wchar_t** keyArray = (wchar_t**)operator_new(numChars * sizeof(wchar_t*));
    if (!keyArray) {
        printf("Memory allocation failed\n");
        FreeLibrary(hMod);
        return;
    }

    // Initialize
    pfnInit();

    // State machine variables
    int state = 0;           // local_30 – 0:idle, 1:got '0', 2:reading hex, 3:send key, 4:done
    int hexIndex = 0;        // local_34 – number of characters in hex buffer
    wchar_t hexBuffer[16];   // local_26[7] + local_28 (8 shorts) but we'll use larger
    wchar_t savedChar = 0;   // local_28 – stored '0' character
    int keyValue = 0;
    wchar_t* p = lpString;
    BOOL done = FALSE;

    if (virtualMode != 0) {
        while (!done && *p) {
            wchar_t ch = *p;

            switch (state) {
                case 0: // idle – skip whitespace, look for '0' or quote
                    if (iswspace(ch)) {
                        p++;
                        break;
                    }
                    if (ch == L'"') {
                        p++;
                        break;
                    }
                    if (ch == L'0') {
                        // Start of virtual key token
                        state = 1;
                        hexBuffer[hexIndex++] = ch;
                        savedChar = ch;
                        p++;
                    } else {
                        // Not a valid start – format 1
                        printf("error: virtual keys in incorrect format 1\r\n");
                        usage();
                        goto cleanup;
                    }
                    break;

                case 1: // expect 'x' or 'X'
                    if (ch == L'x' || ch == L'X') {
                        hexBuffer[hexIndex++] = ch;
                        state = 2;
                        p++;
                    } else {
                        // No 'x' – format 2
                        printf("error: virtual keys in incorrect format 2\r\n");
                        usage();
                        goto cleanup;
                    }
                    break;

                case 2: // reading hex digits
                    if (iswxdigit(ch)) {
                        hexBuffer[hexIndex++] = ch;
                        p++;
                        // Check if we have too many digits (max 4)
                        if (hexIndex > 6) { // "0x" + 4 digits = 6
                            // Too many digits – format 5
                            printf("error: virtual keys in incorrect format 5\r\n");
                            usage();
                            goto cleanup;
                        }
                    } else {
                        // End of hex token: space, quote, or end of string
                        // But also check for format 3: if the character is not a valid terminator
                        if (!iswspace(ch) && ch != L'"' && ch != L'\0') {
                            // Invalid character in hex token – format 3
                            printf("error: virtual keys in incorrect format 3\r\n");
                            usage();
                            goto cleanup;
                        }
                        state = 3;
                    }
                    break;

                case 3: // send the key
                    if (hexIndex > 2) { // at least "0x" + digit
                        hexBuffer[hexIndex] = L'\0';
                        wchar_t* endptr;
                        keyValue = (int)wcstol(hexBuffer, &endptr, 16);
                        if (keyValue < 1 || keyValue > 0xFE) {
                            printf("error: invalid virtual key value\r\n");
                            usage();
                            goto cleanup;
                        }
                        printf("sending virtual key: 0x%x\r\n", keyValue);
                        pfnVirtualKey(5, keyValue);
                        Sleep(250);
                    } else {
                        // Not enough hex digits – format 4
                        printf("error: virtual keys in incorrect format 4\r\n");
                        usage();
                        goto cleanup;
                    }
                    // Reset state
                    state = 0;
                    hexIndex = 0;
                    // Skip whitespace
                    while (iswspace(*p)) p++;
                    break;

                default:
                    break;
            }
        }

        // If we still have a pending hex token at end of string
        if (state == 2 && hexIndex > 0) {
            // We have a hex token without a closing delimiter
            hexBuffer[hexIndex] = L'\0';
            keyValue = (int)wcstol(hexBuffer, NULL, 16);
            if (keyValue >= 1 && keyValue <= 0xFE) {
                printf("sending virtual key: 0x%x\r\n", keyValue);
                pfnVirtualKey(5, keyValue);
                Sleep(250);
            } else {
                printf("error: invalid virtual key value at end of string\r\n");
                usage();
                goto cleanup;
            }
        }

    } else {
        // String mode: send each character as a string key
        wchar_t* pStr = lpString;
        int index = 0;
        while (*pStr) {
            wchar_t ch = *pStr;
            wchar_t* buf = (wchar_t*)operator_new(sizeof(wchar_t) * 2);
            if (!buf) {
                printf("Memory allocation failed\n");
                goto cleanup;
            }
            keyArray[index] = buf;
            buf[0] = ch;
            buf[1] = L'\0';
            printf("sending string key: %S\r\n", buf);
            pfnStringKey(5, ch);
            Sleep(250);
            pStr++;
            index++;
        }
    }

cleanup:
    // Deinitialize
    pfnDeinit();

    // Free allocated buffers
    for (size_t i = 0; i < numChars; i++) {
        if (keyArray[i]) {
            operator_delete(keyArray[i]);
        }
    }
    operator_delete(keyArray);

    if (hMod)
        FreeLibrary(hMod);
}

// ------------------------------------------------------------------
// main entry point
// ------------------------------------------------------------------
int __cdecl main(int argc, char* argv[]) {
    int ret = 0;
    wchar_t wstrBuffer[1024] = {0};

    if (argc == 2) {
        swprintf_s(wstrBuffer, _countof(wstrBuffer), L"%S", argv[1]);
        SendKeys(0, wstrBuffer);
        ret = 0;
    } else if (argc == 3) {
        if (_strcmpi(argv[1], "-v") == 0) {
            swprintf_s(wstrBuffer, _countof(wstrBuffer), L"%S", argv[2]);
            SendKeys(1, wstrBuffer);
            ret = 0;
        } else {
            usage();
            ret = 1;
        }
    } else {
        usage();
        ret = 1;
    }

    return ret;
}

