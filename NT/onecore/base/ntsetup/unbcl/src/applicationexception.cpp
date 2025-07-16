// applicationexception.cpp

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

class ApplicationException : public SystemException
{
public:
    ApplicationException()
        : SystemException(L"Application exception.")
    {
    }

    ApplicationException(LPCWSTR message)
        : SystemException(message)
    {
    }

    ~ApplicationException()
    {
    }
};

