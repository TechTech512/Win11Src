// stringptr.cpp

#include <windows.h>

class StringPtr
{
private:
    LPCWSTR m_Pointer;

public:
    // Default constructor
    StringPtr()
        : m_Pointer(NULL)
    {
    }

    // Constructor from LPCWSTR
    StringPtr(LPCWSTR ptr)
        : m_Pointer(ptr)
    {
    }

    // Copy constructor
    StringPtr(const StringPtr& other)
        : m_Pointer(other.m_Pointer)
    {
    }

    // Assignment operator
    StringPtr& operator=(const StringPtr& other)
    {
        if (this != &other)
        {
            m_Pointer = other.m_Pointer;
        }
        return *this;
    }

    // Compare for equality (case-sensitive)
    bool Equals(const StringPtr& other) const
    {
        if (m_Pointer == NULL && other.m_Pointer == NULL)
            return true;

        if (m_Pointer == NULL || other.m_Pointer == NULL)
            return false;

        return wcscmp(m_Pointer, other.m_Pointer) == 0;
    }

    // Compare for equality (case-insensitive)
    bool EqualsIgnoreCase(const StringPtr& other) const
    {
        if (m_Pointer == NULL && other.m_Pointer == NULL)
            return true;

        if (m_Pointer == NULL || other.m_Pointer == NULL)
            return false;

        return _wcsicmp(m_Pointer, other.m_Pointer) == 0;
    }

    // Get the raw pointer
    LPCWSTR Ptr() const
    {
        return m_Pointer;
    }

    // Get length of string
    int Length() const
    {
        return (m_Pointer != NULL) ? (int)wcslen(m_Pointer) : 0;
    }

    // Check for null
    bool IsNull() const
    {
        return m_Pointer == NULL;
    }
};

