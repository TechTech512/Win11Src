#include "hwn.h"

// ------------------------------------------------------------------
// Main entry point – matches original decompiled logic exactly
// ------------------------------------------------------------------
int __cdecl wmain(int argc, wchar_t* argv[])
{
    unsigned int testResult = 0;  // uVar2 (NLEDTest/VibraTest result)
    unsigned long long finalResult; // uVar4 (final return value)

    if (argc == 1) {
        LogW((unsigned short *)L"Invalid Test Parameter...\r\n");
        finalResult = 0x57;   // 87
        goto print_result;
    }

    if (argc == 2) {
        if (_wcsnicmp(argv[1], L"NLED", 4) == 0) {
            testResult = NLEDTest();
        } else if (_wcsnicmp(argv[1], L"VIBRA", 5) == 0) {
            testResult = VibraTest();
        } else {
            LogW((unsigned short *)L"Invalid Test Name...\r\n");
            finalResult = 0x57;
            goto print_result;
        }

        finalResult = testResult;
        if (testResult != 0) {
            goto print_result;
        }
        // fall through if testResult == 0
    }

    // If we get here, either:
    //   - argc == 2 and test succeeded (testResult == 0)
    //   - argc > 2  (i.e., more arguments, which should be ignored)
    LogW((unsigned short *)L"Test Success...\r\n");
    finalResult = 0;

print_result:
    LogW((unsigned short *)L"TEST_RESULT:%d\r\n", (int)finalResult);
    return (int)finalResult;
}

