// argumentoutofrangeexception.cpp

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

class ArgumentException : public SystemException
{
public:
    ArgumentException()
        : SystemException(L"Argument exception.")
    {
    }

    ArgumentException(LPCWSTR message)
        : SystemException(message)
    {
    }

    virtual ~ArgumentException()
    {
    }
};

class ArgumentOutOfRangeException : public ArgumentException
{
public:
    ArgumentOutOfRangeException()
        : ArgumentException(L"Argument is out of range.")
    {
    }

    ArgumentOutOfRangeException(LPCWSTR message)
        : ArgumentException(message)
    {
    }

    ~ArgumentOutOfRangeException()
    {
    }
};


