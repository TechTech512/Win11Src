// logging.cpp

#include <windows.h>

namespace UnBCL {

// === Interface ===
class ILogHandler {
public:
    virtual ~ILogHandler() {}
    virtual void Log(const wchar_t* message) = 0;
};

// === Static logging system ===
class Logging {
private:
    static ILogHandler* s_Handler;

public:
    static void SetHandler(ILogHandler* handler) {
        s_Handler = handler;
    }

    static void Write(const wchar_t* message) {
        if (s_Handler && message)
            s_Handler->Log(message);
    }
};

// === Static field definition ===
ILogHandler* Logging::s_Handler = nullptr;

} // namespace UnBCL

