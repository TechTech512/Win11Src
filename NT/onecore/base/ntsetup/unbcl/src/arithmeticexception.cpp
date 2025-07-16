// arithmeticexception.cpp

#include <windows.h>

class SystemException
{
protected:
    LPCWSTR m_Message;

public:
    SystemException()
        : m_Message(L"System exception occurred.")
    {
    }

    SystemException(LPCWSTR message)
        : m_Message(message)
    {
    }

    virtual ~SystemException()
    {
    }

    virtual LPCWSTR Message() const
    {
        return m_Message;
    }
};

class ArithmeticException : public SystemException
{
public:
    ArithmeticException()
        : SystemException(L"Arithmetic operation resulted in an overflow or divide by zero.")
    {
    }

    ArithmeticException(LPCWSTR message)
        : SystemException(message)
    {
    }

    ~ArithmeticException()
    {
    }
};
