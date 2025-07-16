#pragma once

#include "_string.h"

namespace UnBCL {

class Exception {
protected:
    String* m_message;
    Exception* m_inner;

public:
    Exception(String* msg)
        : m_message(msg), m_inner(nullptr) {}

    Exception(String* msg, Exception* inner)
        : m_message(msg), m_inner(inner) {}

    virtual ~Exception() {}

    virtual String* Message() const { return m_message; }
    virtual Exception* InnerException() const { return m_inner; }
};

} // namespace UnBCL

