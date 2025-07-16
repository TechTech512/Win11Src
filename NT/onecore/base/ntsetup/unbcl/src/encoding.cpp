// encoding.cpp

#include <windows.h>

namespace UnBCL {

// Base Object
class Object {
public:
    virtual ~Object() {}
};

// Stub exception
class ArgumentOutOfRangeException : public Object {
public:
    ArgumentOutOfRangeException(const wchar_t* message) {
        OutputDebugStringW(L"[ArgumentOutOfRangeException] ");
        OutputDebugStringW(message);
        OutputDebugStringW(L"\n");
    }
};

// AddStackTrace stub
template <typename T>
T* AddStackTraceToException(T* ex, char* context) {
    // Stub: just return it
    return ex;
}

// Encoding class
class Encoding : public Object {
private:
    int m_CodePage;

public:
    Encoding(int codePage)
    {
        if (codePage < 0) {
            ArgumentOutOfRangeException* ex =
                new ArgumentOutOfRangeException(L"negative codepage to Encoding constructor");
            throw AddStackTraceToException(ex, nullptr);
        }

        m_CodePage = codePage;
    }

    virtual ~Encoding() {}
};

} // namespace UnBCL

