// asciiencoding.cpp

#include <windows.h>

class Object
{
public:
    virtual ~Object() {}
};

class Encoding : public Object
{
protected:
    BOOL m_IsReadOnly;

public:
    Encoding()
        : m_IsReadOnly(TRUE)
    {
    }

    virtual ~Encoding() {}

    virtual int GetByteCount(const wchar_t* chars, int count)
    {
        return count;
    }

    virtual int GetBytes(const wchar_t* chars, int charCount, unsigned char* bytes, int byteCount)
    {
        int converted = 0;
        for (int i = 0; i < charCount && i < byteCount; ++i)
        {
            wchar_t ch = chars[i];
            bytes[i] = (ch < 0x80) ? (unsigned char)ch : '?';
            ++converted;
        }
        return converted;
    }

    BOOL IsReadOnly() const
    {
        return m_IsReadOnly;
    }
};

class ASCIIEncoding : public Encoding
{
public:
    ASCIIEncoding()
    {
        m_IsReadOnly = TRUE;
    }

    ~ASCIIEncoding() override {}

    int GetByteCount(const wchar_t* chars, int count) override
    {
        return count;
    }

    int GetBytes(const wchar_t* chars, int charCount, unsigned char* bytes, int byteCount) override
    {
        int converted = 0;
        for (int i = 0; i < charCount && i < byteCount; ++i)
        {
            wchar_t ch = chars[i];
            bytes[i] = (ch < 0x80) ? (unsigned char)ch : '?';
            ++converted;
        }
        return converted;
    }
};
