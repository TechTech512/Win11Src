// string.cpp

#include <windows.h>

class String
{
private:
    wchar_t* m_Buffer;
    int m_Length;

public:
    // Default constructor
    String()
        : m_Buffer(NULL), m_Length(0)
    {
    }

    // Constructor from LPCWSTR
    String(LPCWSTR str)
        : m_Buffer(NULL), m_Length(0)
    {
        if (str)
        {
            m_Length = static_cast<int>(wcslen(str));
            m_Buffer = (wchar_t*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (m_Length + 1) * sizeof(wchar_t));
            if (m_Buffer)
            {
                wcscpy_s(m_Buffer, m_Length + 1, str);
            }
        }
    }

    // Copy constructor
    String(const String& other)
        : m_Buffer(NULL), m_Length(0)
    {
        if (other.m_Buffer && other.m_Length > 0)
        {
            m_Length = other.m_Length;
            m_Buffer = (wchar_t*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (m_Length + 1) * sizeof(wchar_t));
            if (m_Buffer)
            {
                wcscpy_s(m_Buffer, m_Length + 1, other.m_Buffer);
            }
        }
    }

    // Destructor
    ~String()
    {
        if (m_Buffer)
        {
            HeapFree(GetProcessHeap(), 0, m_Buffer);
            m_Buffer = NULL;
            m_Length = 0;
        }
    }

    // Assignment operator
    String& operator=(const String& other)
    {
        if (this != &other)
        {
            if (m_Buffer)
            {
                HeapFree(GetProcessHeap(), 0, m_Buffer);
            }

            m_Length = 0;
            m_Buffer = NULL;

            if (other.m_Buffer && other.m_Length > 0)
            {
                m_Length = other.m_Length;
                m_Buffer = (wchar_t*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (m_Length + 1) * sizeof(wchar_t));
                if (m_Buffer)
                {
                    wcscpy_s(m_Buffer, m_Length + 1, other.m_Buffer);
                }
            }
        }
        return *this;
    }

    // Get length of string
    int Length() const
    {
        return m_Length;
    }

    // Get raw wchar_t pointer
    LPCWSTR CStr() const
    {
        return m_Buffer;
    }

    // Compare two strings (case-sensitive)
    bool Equals(const String& other) const
    {
        if (m_Buffer == NULL && other.m_Buffer == NULL)
            return true;

        if (m_Buffer == NULL || other.m_Buffer == NULL)
            return false;

        return wcscmp(m_Buffer, other.m_Buffer) == 0;
    }

    // Compare (case-insensitive)
    bool EqualsIgnoreCase(const String& other) const
    {
        if (m_Buffer == NULL && other.m_Buffer == NULL)
            return true;

        if (m_Buffer == NULL || other.m_Buffer == NULL)
            return false;

        return _wcsicmp(m_Buffer, other.m_Buffer) == 0;
    }
};
