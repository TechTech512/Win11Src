// textreader.cpp

#include <windows.h>

namespace UnBCL {

class Object {
public:
    virtual ~Object() {}
};

class String {
private:
    wchar_t* m_Data;
public:
    String(const wchar_t* str) {
        if (!str) str = L"";
        size_t len = wcslen(str);
        m_Data = new wchar_t[len + 1];
        wcscpy(m_Data, str);
    }

    ~String() { delete[] m_Data; }

    const wchar_t* CStr() const { return m_Data; }
};

class StringBuilder {
private:
    wchar_t* m_Buffer;
    size_t m_Size;
    size_t m_Capacity;

public:
    StringBuilder()
        : m_Size(0), m_Capacity(128) {
        m_Buffer = new wchar_t[m_Capacity];
        m_Buffer[0] = 0;
    }

    ~StringBuilder() {
        delete[] m_Buffer;
    }

    void Append(wchar_t ch) {
        if (m_Size + 1 >= m_Capacity) {
            m_Capacity *= 2;
            wchar_t* newBuf = new wchar_t[m_Capacity];
            memcpy(newBuf, m_Buffer, m_Size * sizeof(wchar_t));
            delete[] m_Buffer;
            m_Buffer = newBuf;
        }
        m_Buffer[m_Size++] = ch;
        m_Buffer[m_Size] = 0;
    }

    String* ToString() {
        return new String(m_Buffer);
    }

    size_t Length() const { return m_Size; }
};

class TextReader : public Object {
public:
    virtual int Read() = 0;

    String* ReadLine() {
        StringBuilder builder;
        int ch;

        while (true) {
            ch = this->Read();
            if (ch == -1) {
                return builder.Length() > 0 ? builder.ToString() : nullptr;
            }

            if (ch == '\r') {
                int next = this->Read();
                if (next != '\n' && next != -1) {
                    // unread next char if needed
                }
                return builder.ToString();
            }

            if (ch == '\n') {
                return builder.ToString();
            }

            builder.Append((wchar_t)ch);
        }
    }
};

} // namespace UnBCL

