// environ.c

#include <windows.h>
#include <strsafe.h>
#include <stdlib.h>
#include <string.h>

// Structure for environment variable entry
typedef struct _ENV_ENTRY {
    struct _ENV_ENTRY* Flink;
    struct _ENV_ENTRY* Blink;
    wchar_t* Name;
    wchar_t* Value;
} ENV_ENTRY;

// Forward declarations
int BfspEnvAddVariable(struct _ENV_ENTRY* head, const wchar_t* name, const wchar_t* value);
int BfspEnvDeleteVariable(struct _ENV_ENTRY* head, const wchar_t* name);
wchar_t* BfspEnvGetValue(struct _ENV_ENTRY* head, const wchar_t* name);
void BfspEnvDestroy(struct _ENV_ENTRY* head);
wchar_t* BfspEnvExpandString(struct _ENV_ENTRY* head, const wchar_t* input);

// Initialize a circular doubly linked list head
#define INIT_ENV_LIST(head) \
    do { (head)->Flink = (head); (head)->Blink = (head); } while(0)

//
// Add or update a variable in the list.
//
int BfspEnvAddVariable(ENV_ENTRY* head, const wchar_t* name, const wchar_t* value) {
    if (!name || !value || wcslen(name) == 0) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    // Delete existing entry if present
    BfspEnvDeleteVariable(head, name);

    size_t nameLen = wcslen(name) + 1;
    size_t valueLen = wcslen(value) + 1;

    ENV_ENTRY* entry = (ENV_ENTRY*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(ENV_ENTRY));
    if (!entry) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }

    entry->Name = (wchar_t*)HeapAlloc(GetProcessHeap(), 0, nameLen * sizeof(wchar_t));
    entry->Value = (wchar_t*)HeapAlloc(GetProcessHeap(), 0, valueLen * sizeof(wchar_t));

    if (!entry->Name || !entry->Value) {
        if (entry->Name) HeapFree(GetProcessHeap(), 0, entry->Name);
        if (entry->Value) HeapFree(GetProcessHeap(), 0, entry->Value);
        HeapFree(GetProcessHeap(), 0, entry);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }

    StringCchCopyW(entry->Name, nameLen, name);
    StringCchCopyW(entry->Value, valueLen, value);

    // Insert at the end
    entry->Flink = head;
    entry->Blink = head->Blink;
    head->Blink->Flink = entry;
    head->Blink = entry;

    return 1;
}

//
// Delete a variable by name.
//
int BfspEnvDeleteVariable(ENV_ENTRY* head, const wchar_t* name) {
    if (!name) return 0;

    ENV_ENTRY* current = head->Flink;
    while (current != head) {
        if (_wcsicmp(current->Name, name) == 0) {
            current->Blink->Flink = current->Flink;
            current->Flink->Blink = current->Blink;

            if (current->Name) HeapFree(GetProcessHeap(), 0, current->Name);
            if (current->Value) HeapFree(GetProcessHeap(), 0, current->Value);
            HeapFree(GetProcessHeap(), 0, current);
            return 1;
        }
        current = current->Flink;
    }

    SetLastError(ERROR_ENVVAR_NOT_FOUND);
    return 0;
}

//
// Get the value of a variable.
//
wchar_t* BfspEnvGetValue(ENV_ENTRY* head, const wchar_t* name) {
    if (!name) return NULL;

    ENV_ENTRY* current = head->Flink;
    while (current != head) {
        if (_wcsicmp(current->Name, name) == 0) {
            return current->Value;
        }
        current = current->Flink;
    }

    return NULL;
}

//
// Destroy the entire environment list.
//
void BfspEnvDestroy(ENV_ENTRY* head) {
    ENV_ENTRY* current = head->Flink;
    while (current != head) {
        ENV_ENTRY* next = current->Flink;

        if (current->Name) HeapFree(GetProcessHeap(), 0, current->Name);
        if (current->Value) HeapFree(GetProcessHeap(), 0, current->Value);
        HeapFree(GetProcessHeap(), 0, current);

        current = next;
    }

    INIT_ENV_LIST(head);
}

//
// Expand a string with |var| delimiters.
//
wchar_t* BfspEnvExpandString(ENV_ENTRY* head, const wchar_t* input) {
    if (!input) return NULL;

    size_t totalLen = 0;
    const wchar_t* p = input;
    BOOL inVar = FALSE;
    const wchar_t* start = p;

    // First pass: calculate size
    while (*p) {
        if (*p == L'|') {
            if (inVar) {
                // closing delimiter
                size_t nameLen = p - start;
                wchar_t varName[128];
                if (nameLen >= _countof(varName)) return NULL;

                wcsncpy_s(varName, _countof(varName), start, nameLen);
                wchar_t* value = BfspEnvGetValue(head, varName);
                totalLen += value ? wcslen(value) : 0;
                inVar = FALSE;
            } else {
                inVar = TRUE;
                start = p + 1;
            }
        } else if (!inVar) {
            totalLen++;
        }
        p++;
    }

    wchar_t* output = (wchar_t*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (totalLen + 1) * sizeof(wchar_t));
    if (!output) return NULL;

    // Second pass: build output
    wchar_t* out = output;
    p = input;
    inVar = FALSE;
    start = p;

    while (*p) {
        if (*p == L'|') {
            if (inVar) {
                size_t nameLen = p - start;
                wchar_t varName[128];
                wcsncpy_s(varName, _countof(varName), start, nameLen);
                wchar_t* value = BfspEnvGetValue(head, varName);
                if (value) {
                    StringCchCatW(out, totalLen + 1, value);
                }
                inVar = FALSE;
            } else {
                inVar = TRUE;
                start = p + 1;
            }
        } else if (!inVar) {
            *out++ = *p;
        }
        p++;
    }

    return output;
}

