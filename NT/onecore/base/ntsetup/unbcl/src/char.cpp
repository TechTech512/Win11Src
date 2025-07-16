// char.cpp

#include <windows.h>

class Char
{
private:
    wchar_t m_Value;

public:
    // Default constructor
    Char()
        : m_Value(L'\0')
    {
    }

    // Constructor from wchar_t
    Char(wchar_t ch)
        : m_Value(ch)
    {
    }

    // Copy constructor
    Char(const Char& other)
        : m_Value(other.m_Value)
    {
    }

    // Assignment operator
    Char& operator=(const Char& other)
    {
        if (this != &other)
        {
            m_Value = other.m_Value;
        }
        return *this;
    }

    // Implicit conversion to wchar_t
    operator wchar_t() const
    {
        return m_Value;
    }

    // Accessor
    wchar_t Value() const
    {
        return m_Value;
    }
};

