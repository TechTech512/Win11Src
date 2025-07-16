// memorystream.cpp

#include <windows.h>

namespace UnBCL {

// === Base Classes ===

class Object {
public:
    virtual ~Object() {}
};

class IDisposable {
public:
    virtual void Dispose() = 0;
};

class Stream : public Object, public IDisposable {
public:
    virtual ~Stream() {}
    void Dispose() override {}
};

// === Exception Types ===

class ArgumentNullException : public Object {
public:
    ArgumentNullException(const wchar_t* msg) {
        OutputDebugStringW(L"[ArgumentNullException] ");
        OutputDebugStringW(msg);
        OutputDebugStringW(L"\n");
    }
};

class ArgumentException : public Object {
public:
    ArgumentException(const wchar_t* msg) {
        OutputDebugStringW(L"[ArgumentException] ");
        OutputDebugStringW(msg);
        OutputDebugStringW(L"\n");
    }
};

template <typename T>
T* AddStackTraceToException(T* ex, char*) {
    return ex;
}

// === Byte Array Wrapper ===

class ByteArray : public Object {
private:
    BYTE* m_Data;
    int m_Length;

public:
    ByteArray(int size) {
        m_Length = size;
        m_Data = new BYTE[size];
        memset(m_Data, 0, size);
    }

    ~ByteArray() {
        delete[] m_Data;
    }

    int Length() const { return m_Length; }
    BYTE* Data() { return m_Data; }
};

// === MemoryStream ===

class MemoryStream : public Stream {
private:
    ByteArray* m_Buf;
    int m_Pos;
    int m_Length;
    int m_Writable;
    int m_Closed;
    int m_Growable;
    int m_FreeBuf;

public:
    // Default constructor
    MemoryStream()
        : m_Buf(new ByteArray(0x200)),
          m_Pos(0),
          m_Length(0),
          m_Writable(1),
          m_Closed(0),
          m_Growable(1),
          m_FreeBuf(1)
    {}

    // Constructor from external buffer
    MemoryStream(ByteArray* buffer, int writable, int freeBuf)
        : m_Buf(buffer),
          m_Pos(0),
          m_Writable(writable),
          m_Closed(0),
          m_Growable(0),
          m_FreeBuf(freeBuf)
    {
        if (!buffer) {
            auto* ex = new ArgumentNullException(L"null byteArray to MemoryStream constructor");
            throw AddStackTraceToException(ex, nullptr);
        }

        m_Length = buffer->Length();
    }

    // Destructor
    ~MemoryStream() override {
        Dispose();
    }

    // Dispose
    void Dispose() override {
        if (m_FreeBuf && m_Buf) {
            delete m_Buf;
            m_Buf = nullptr;
        }
    }
};

} // namespace UnBCL

