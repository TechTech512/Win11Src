#pragma once

#include "exception.h"
#include "_string.h"

namespace UnBCL {

class ArgumentException : public Exception {
public:
    ArgumentException()
        : Exception() {}

    ArgumentException(String* message)
        : Exception(message) {}

    ArgumentException(String* message, String* paramName)
        : Exception(message), m_paramName(paramName) {}

    virtual ~ArgumentException() {}

    String* GetParamName() const {
        return m_paramName;
    }

protected:
    String* m_paramName = nullptr;
};

} // namespace UnBCL

