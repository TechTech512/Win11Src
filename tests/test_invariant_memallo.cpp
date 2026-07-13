#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <cstring>
#include <windows.h>

// Forward declare the vulnerable function from memallo.cpp
// The actual implementation uses wsprintf with fixed Buffer size
extern "C" void ReportError(const char* pszFile, int line, int iErrCode);

class BufferBoundsTest : public ::testing::TestWithParam<std::pair<std::string, int>> {};

TEST_P(BufferBoundsTest, WsprintfBufferDoesNotOverflow) {
    // Invariant: Buffer reads never exceed declared length
    // wsprintf must not write beyond fixed buffer boundary regardless of input length
    
    auto [payload, errCode] = GetParam();
    
    // Capture any crash or memory violation by calling with adversarial input
    // The function internally uses wsprintf(Buffer, format, payload, line)
    // where Buffer is typically 256-512 bytes on stack
    
    // This test verifies the function handles oversized file paths without crashing
    // If buffer overflow occurs, this will trigger AddressSanitizer or crash
    EXPECT_NO_FATAL_FAILURE({
        ReportError(payload.c_str(), 42, errCode);
    });
}

INSTANTIATE_TEST_SUITE_P(
    AdversarialInputs,
    BufferBoundsTest,
    ::testing::Values(
        // Valid input: normal file path
        std::make_pair("C:\\normal\\file.chm", 1),
        
        // Boundary case: exactly at typical buffer limit
        std::make_pair(std::string(256, 'A'), 2),
        
        // Exploit payload: 10x buffer size with long path
        std::make_pair(std::string(2560, 'B'), 3),
        
        // Exploit payload: 2x buffer size with path-like string
        std::make_pair(std::string(512, 'C') + "\\malicious.chm", 4),
        
        // Boundary: single character over typical limit
        std::make_pair(std::string(513, 'D'), 5)
    )
);

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}