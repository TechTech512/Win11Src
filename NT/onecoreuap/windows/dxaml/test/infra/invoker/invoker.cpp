// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// This program exists for two reasons:
//
// 1)
// To overcome traditional DOS console command argument
// and variable length limitations. It takes 
// us from a measly 'some small length' (~2k) of arg chars to much 
// closer to the ~32k max imposed by the environment block.
//
// Upon invoking this program, it will transparently pass all 
// arguments it receives to TE.exe. It will additionally
// look for an optional file called teParams.txt. If this file
// exists it will append the entire file's contents to TE.exe's
// argument string.
//
// This allows us to pass much longer strings to TAEF, and allows
// for test selection strings to be built containing ~100 tests, 
// instead of the ~10-20 we can pass using the default argument
// length limitations.
//
// 2)
// To allow us to perform a prerun and postrun set of tests, even
// when using TAEF's loop modes. We can't do this using TAEF itself
// because it includes these tests in the loops.

#include <windows.h>
#include <stdio.h>
#include <string.h>

// Maximum length of a Windows command line (CreateProcess limit).
#define ARGS_BUFFER_SIZE 32768

// Append src to dest, never overflowing the fixed-size buffer.
static void SafeAppend(char* dest, const char* src)
{
    size_t destLen = strlen(dest);
    if (destLen >= ARGS_BUFFER_SIZE - 1) {
        return;
    }
    strncat(dest, src, ARGS_BUFFER_SIZE - destLen - 1);
}

// Read the first line of a file into 'out', stripping any trailing
// CR/LF. Returns true if the file existed and a line was read.
static bool ReadFirstLine(const char* filename, char* out)
{
    FILE* f = fopen(filename, "r");
    if (f == NULL) {
        out[0] = '\0';
        return false;
    }

    if (fgets(out, ARGS_BUFFER_SIZE, f) == NULL) {
        fclose(f);
        out[0] = '\0';
        return false;
    }

    // Strip trailing newline characters.
    size_t len = strlen(out);
    while (len > 0 && (out[len - 1] == '\n' || out[len - 1] == '\r')) {
        out[len - 1] = '\0';
        len--;
    }

    fclose(f);
    return true;
}

bool InvokeTaef(const char* args, DWORD& exitCode)
{
    // Create the process... reuse the current
    // console handles so output is piped into the
    // current window.
    STARTUPINFO si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(si);

    // CreateProcessA may modify the command line buffer in place,
    // so it cannot point at the caller's const string. Copy into
    // a local mutable buffer first.
    char cmdLine[ARGS_BUFFER_SIZE];
    strncpy(cmdLine, args, ARGS_BUFFER_SIZE - 1);
    cmdLine[ARGS_BUFFER_SIZE - 1] = '\0';

    BOOL result = CreateProcessA(
        "te.exe",
        cmdLine,
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &si,
        &pi
    );
    if (result == 0) {
        printf("CreateProcess returned 0 (LastError: %lu)\n", GetLastError());
        return false;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    result = GetExitCodeProcess(pi.hProcess, &exitCode);
    if (result == 0) {
        printf("GetExitCodeProcess returned 0 (LastError: %lu)\n", GetLastError());
        return false;
    }
    return true;
}

int __cdecl main(int argc, char** argv)
{
    printf("Invoking TE.exe...\n");

    // For C/C++ argc/argv processing we expect
    // the first argument to be the command name.
    char baseArgs[ARGS_BUFFER_SIZE];
    strcpy(baseArgs, "te.exe");

    // Append all our arguments...
    for (int i = 1; i < argc; i++) {
        SafeAppend(baseArgs, " ");
        SafeAppend(baseArgs, argv[i]);
    }

    char preRunArgStr[ARGS_BUFFER_SIZE];
    char runArgStr[ARGS_BUFFER_SIZE];
    char postRunArgStr[ARGS_BUFFER_SIZE];
    preRunArgStr[0] = '\0';
    postRunArgStr[0] = '\0';
    strcpy(runArgStr, baseArgs);

    {
        bool appendToWtt = false;
        char argString[ARGS_BUFFER_SIZE];

        // And append the contents of teParams.txt
        // if it exists...
        // The arguments to append should all be on one line,
        // so we only read the first line of each file (in case
        // somebody appended a stray '\n').
        if (ReadFirstLine("teParams.pre.txt", argString)) {
            strcpy(preRunArgStr, baseArgs);
            SafeAppend(preRunArgStr, " ");
            SafeAppend(preRunArgStr, argString);
            appendToWtt = true;
        }

        if (ReadFirstLine("teParams.txt", argString)) {
            SafeAppend(runArgStr, " ");
            SafeAppend(runArgStr, argString);
        }
        if (appendToWtt) {
            SafeAppend(runArgStr, " /appendWttLogging");
        }

        if (ReadFirstLine("teParams.post.txt", argString)) {
            strcpy(postRunArgStr, baseArgs);
            SafeAppend(postRunArgStr, " ");
            SafeAppend(postRunArgStr, argString);
            SafeAppend(postRunArgStr, " /appendWttLogging");
        }
    }

    remove("teParams.pre.txt");
    remove("teParams.txt");
    remove("teParams.post.txt");

    // teParams.pre.txt run.
    // ---------------------------------------
    if (preRunArgStr[0] != '\0') {
        printf("\nFound prerun file, invoking with arguments from teParams.pre.txt:\n");
        printf("%s\n\n", preRunArgStr);
        DWORD exitCode = 1;
        if (!InvokeTaef(preRunArgStr, exitCode)) {
            printf("Terminating due to InvokeTaef() failure.\n");
            return 1;
        }
    }

    // Main teParams.txt run.
    // ---------------------------------------
    printf("\nInvoking with arguments from teParams.txt:\n");
    printf("%s\n\n", runArgStr);
    DWORD mainRunExitCode = 1;
    if (!InvokeTaef(runArgStr, mainRunExitCode)) {
        printf("Terminating due to InvokeTaef() failure.\n");
        return 1;
    }

    // teParams.post.txt run.
    // ---------------------------------------
    if (postRunArgStr[0] != '\0') {
        printf("\nFound postrun file, invoking with arguments from teParams.post.txt:\n");
        printf("%s\n\n", postRunArgStr);
        DWORD exitCode = 1;
        if (!InvokeTaef(postRunArgStr, exitCode)) {
            printf("Terminating due to InvokeTaef() failure.\n");
            return 1;
        }
    }

    // And finally return TE.exe's exit code as if it
    // were our own.
    return mainRunExitCode;
}
