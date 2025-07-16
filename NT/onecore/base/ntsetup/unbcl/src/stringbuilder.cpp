// stringbuilder.cpp

#include <windows.h>

class StringBuilder
{
private:
    wchar_t* m_Buffer;
    int m_Capacity;
    int m_Length;

    // Ensures there's enough space to append new content
    void EnsureCapacity(int required)
    {
        if (required <= m_Capacity)
            return;

        int newCapacity = m_Capacity * 2;
        if (newCapacity < required)
            newCapacity = required + 16;

        wchar_t* newBuffer = (wchar_t*)HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, m_Buffer, newCapacity * sizeof(wchar_t));
        if (newBuffer)
        {
            m_Buffer = newBuffer;
            m_Capacity = newCapacity;
        }
    }

public:
    // Default constructor
    StringBuilder()
        : m_Buffer(nullptr), m_Capacity(0), m_Length(0)
    {
        m_Capacity = 128;
        m_Buffer = (wchar_t*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, m_Capacity * sizeof(wchar_t));
    }

    // Destructor
    ~StringBuilder()
    {
        if (m_Buffer)
        {
            HeapFree(GetProcessHeap(), 0, m_Buffer);
            m_Buffer = nullptr;
        }
        m_Capacity = 0;
        m_Length = 0;
    }

    // Append a string
    void Append(LPCWSTR text)
    {
        if (!text)
            return;

        int len = (int)wcslen(text);
        EnsureCapacity(m_Length + len + 1);

        if (m_Buffer)
        {
            wcscpy_s(m_Buffer + m_Length, m_Capacity - m_Length, text);
            m_Length += len;
        }
    }

    // Append a single character
    void Append(wchar_t ch)
    {
        EnsureCapacity(m_Length + 2); // one for char, one for null

        if (m_Buffer)
        {
            m_Buffer[m_Length] = ch;
            m_Length++;
            m_Buffer[m_Length] = L'\0';
        }
    }

    // Get current length
    int Length() const
    {
        return m_Length;
    }

    // Get underlying buffer
    LPCWSTR ToString() const
    {
        return m_Buffer ? m_Buffer : L"";
    }

    // Clear content
    void Clear()
    {
        if (m_Buffer && m_Capacity > 0)
        {
            ZeroMemory(m_Buffer, m_Capacity * sizeof(wchar_t));
            m_Length = 0;
        }
    }
};

