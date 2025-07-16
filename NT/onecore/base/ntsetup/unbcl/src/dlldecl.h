#ifndef UNBCL_DLLDECL_H
#define UNBCL_DLLDECL_H

#include <windows.h>

namespace UnBCL {

// Base exception class
class Exception {
public:
    virtual ~Exception() {}
    virtual void AddStackTrace(const char* context) {
        OutputDebugStringA("[AddStackTrace] ");
        OutputDebugStringA(context ? context : "(no context)");
        OutputDebugStringA("\n");
    }
};

// All other exception types inherit from Exception
class ArgumentException : public Exception {};
class PathTooLongException : public Exception {};
class Win32Exception : public Exception {};
class AbandonedMutexException : public Exception {};
class ArgumentOutOfRangeException : public Exception {};
class SecurityException : public Exception {};
class SystemException : public Exception {};
class ThreadStateException : public Exception {};

// Generic AddStackTraceToException overloads
inline ArgumentException* AddStackTraceToException(ArgumentException* ex, char* context) {
    if (ex) ex->AddStackTrace(context);
    return ex;
}

inline PathTooLongException* AddStackTraceToException(PathTooLongException* ex, char* context) {
    if (ex) ex->AddStackTrace(context);
    return ex;
}

inline Win32Exception* AddStackTraceToException(Win32Exception* ex, char* context) {
    if (ex) ex->AddStackTrace(context);
    return ex;
}

inline AbandonedMutexException* AddStackTraceToException(AbandonedMutexException* ex, char* context) {
    if (ex) ex->AddStackTrace(context);
    return ex;
}

inline ArgumentOutOfRangeException* AddStackTraceToException(ArgumentOutOfRangeException* ex, char* context) {
    if (ex) ex->AddStackTrace(context);
    return ex;
}

inline SecurityException* AddStackTraceToException(SecurityException* ex, char* context) {
    if (ex) ex->AddStackTrace(context);
    return ex;
}

inline SystemException* AddStackTraceToException(SystemException* ex, char* context) {
    if (ex) ex->AddStackTrace(context);
    return ex;
}

inline ThreadStateException* AddStackTraceToException(ThreadStateException* ex, char* context) {
    if (ex) ex->AddStackTrace(context);
    return ex;
}

} // namespace UnBCL

#endif // UNBCL_DLLDECL_H

