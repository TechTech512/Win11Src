/*
 * ls.c
 * A directory listing utility that displays the complete directory tree
 * with file sizes and subdirectories, without any artificial limits.
 */

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <strsafe.h>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "advapi32.lib")

// Global totals
long long TotalFiles = 0;
unsigned long long TotalSize = 0;

// Forward declaration
void BuildDirectory(wchar_t* Path);

// Linked list node for file/directory entries
typedef struct _FileEntry {
    wchar_t FileName[260];
    unsigned int FileSize;
    int IsDirectory;
    struct _FileEntry* Next;
} FileEntry;

// Add an entry to the local list
void AddEntry(FileEntry** head, FileEntry** tail, wchar_t* Name, unsigned int Size, int IsDir) {
    FileEntry* New = (FileEntry*)malloc(sizeof(FileEntry));
    if (!New) return;
    wcscpy_s(New->FileName, sizeof(New->FileName) / sizeof(wchar_t), Name);
    New->FileSize = Size;
    New->IsDirectory = IsDir;
    New->Next = NULL;
    if (*head == NULL) {
        *head = New;
        *tail = New;
    } else {
        (*tail)->Next = New;
        *tail = New;
    }
}

// Free the entire list
void FreeList(FileEntry* head) {
    FileEntry* cur = head;
    while (cur) {
        FileEntry* next = cur->Next;
        free(cur);
        cur = next;
    }
}

// Count entries in list
int CountList(FileEntry* head) {
    int cnt = 0;
    while (head) { cnt++; head = head->Next; }
    return cnt;
}

// Sort files by size (largest first)
void SortFiles(FileEntry* head) {
    // Collect all file entries into an array for sorting
    int count = CountList(head);
    if (count < 2) return;
    FileEntry** arr = (FileEntry**)malloc(sizeof(FileEntry*) * count);
    if (!arr) return;
    int i = 0;
    FileEntry* cur = head;
    while (cur) {
        arr[i++] = cur;
        cur = cur->Next;
    }
    // Bubble sort (only files, ignore directories)
    for (int j = 0; j < count - 1; j++) {
        for (int k = j + 1; k < count; k++) {
            if (!arr[j]->IsDirectory && !arr[k]->IsDirectory) {
                if (arr[j]->FileSize < arr[k]->FileSize) {
                    FileEntry* tmp = arr[j];
                    arr[j] = arr[k];
                    arr[k] = tmp;
                }
            }
        }
    }
    // Rebuild linked list in sorted order
    for (i = 0; i < count - 1; i++) {
        arr[i]->Next = arr[i + 1];
    }
    arr[count - 1]->Next = NULL;
    free(arr);
}

// Build directory tree recursively
void BuildDirectory(wchar_t* Path) {
    WIN32_FIND_DATAW FindData;
    HANDLE FindHandle;
    wchar_t SearchPath[MAX_PATH];
    wchar_t FullPath[MAX_PATH];
    wchar_t DisplayPath[MAX_PATH];
    FileEntry* head = NULL;
    FileEntry* tail = NULL;
    int FileCount = 0;
    int HasContent = 0;
    int HasDirectories = 0;
    unsigned long long DirTotal = 0;

    // Build search path and display path
    if (Path == NULL || Path[0] == L'\0') {
        wcscpy_s(SearchPath, sizeof(SearchPath) / sizeof(wchar_t), L"\\*.*");
        wcscpy_s(DisplayPath, sizeof(DisplayPath) / sizeof(wchar_t), L"\\");
    } else {
        size_t len = wcslen(Path);
        if (len > 0 && Path[len - 1] == L'\\') {
            wcscpy_s(SearchPath, sizeof(SearchPath) / sizeof(wchar_t), Path);
            StringCchCatW(SearchPath, sizeof(SearchPath) / sizeof(wchar_t), L"*.*");
            wcscpy_s(DisplayPath, sizeof(DisplayPath) / sizeof(wchar_t), Path);
        } else {
            wcscpy_s(SearchPath, sizeof(SearchPath) / sizeof(wchar_t), Path);
            StringCchCatW(SearchPath, sizeof(SearchPath) / sizeof(wchar_t), L"\\*.*");
            wcscpy_s(DisplayPath, sizeof(DisplayPath) / sizeof(wchar_t), Path);
            StringCchCatW(DisplayPath, sizeof(DisplayPath) / sizeof(wchar_t), L"\\");
        }
    }

    wprintf(L"%s\n", SearchPath);

    // Enumerate files/directories
    FindHandle = FindFirstFileExW(SearchPath, FindExInfoStandard, &FindData, FindExSearchNameMatch, NULL, 0);
    if (FindHandle == INVALID_HANDLE_VALUE) {
        wprintf(L"no files\n\n");
        return;
    }

    do {
        if (wcscmp(FindData.cFileName, L".") == 0 || wcscmp(FindData.cFileName, L"..") == 0)
            continue;

        HasContent = 1;
        if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            AddEntry(&head, &tail, FindData.cFileName, 0, 1);
            HasDirectories = 1;
        } else {
            unsigned int size = FindData.nFileSizeLow;
            AddEntry(&head, &tail, FindData.cFileName, size, 0);
            TotalSize += size;
            TotalFiles++;
            FileCount++;
        }
    } while (FindNextFileW(FindHandle, &FindData) != 0);
    FindClose(FindHandle);

    if (!HasContent) {
        wprintf(L"no files\n\n");
        FreeList(head);
        return;
    }

    // Print directories first
    FileEntry* cur = head;
    while (cur) {
        if (cur->IsDirectory) {
            wprintf(L"        [%s]\n", cur->FileName);
        }
        cur = cur->Next;
    }

    // Sort and print files
    SortFiles(head);
    cur = head;
    while (cur) {
        if (!cur->IsDirectory && cur->FileSize > 0) {
            wprintf(L"%7d %s\n", cur->FileSize, cur->FileName);
        }
        cur = cur->Next;
    }

    // Compute total size in this directory
    cur = head;
    while (cur) {
        if (!cur->IsDirectory) DirTotal += cur->FileSize;
        cur = cur->Next;
    }
    if (FileCount > 0) {
        wprintf(L"    %llu bytes in %d files\n", DirTotal, FileCount);
    }
    wprintf(L"\n");

    // Recurse into subdirectories (using the local list)
    if (HasDirectories) {
        cur = head;
        while (cur) {
            if (cur->IsDirectory) {
                // Build full path
                wcscpy_s(FullPath, sizeof(FullPath) / sizeof(wchar_t), DisplayPath);
                StringCchCatW(FullPath, sizeof(FullPath) / sizeof(wchar_t), cur->FileName);
                BuildDirectory(FullPath);
            }
            cur = cur->Next;
        }
    }

    // Free the local list
    FreeList(head);
}

// Main
int __cdecl wmain(void) {
    BuildDirectory(NULL);
    wprintf(L"Total of %llu bytes (%llu.%01lluMB) in %lld files\n",
            TotalSize,
            TotalSize >> 20,
            ((TotalSize * 10) >> 20) % 10,
            TotalFiles);
    return 0;
}

