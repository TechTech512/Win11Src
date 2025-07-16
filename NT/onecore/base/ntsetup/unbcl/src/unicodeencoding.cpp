// unicodeencoding.cpp

#include <windows.h>

namespace UnBCL {

// === Base classes ===

class Object {
public:
    virtual ~Object() {}
};

class Decoder : public Object {
public:
    virtual ~Decoder() {}
};

class Encoding : public Object {
protected:
    int m_CharSize;

public:
    Encoding() : m_CharSize(0) {}

    virtual ~Encoding() {}
};

class ASCIIEncoding : public Encoding {
public:
    ASCIIEncoding() {
        m_CharSize = 0x41;  // ASCII default (placeholder)
    }
};

// === UnicodeDecoder ===

class UnicodeDecoder : public Decoder {
private:
    int m_Prev;

public:
    UnicodeDecoder()
        : m_Prev(-1) {
        OutputDebugStringW(L"[UnicodeDecoder] Initialized with m_Prev = -1\n");
    }

    ~UnicodeDecoder() override {}
};

// === UnicodeEncoding ===

class UnicodeEncoding : public ASCIIEncoding {
public:
    UnicodeEncoding() {
        m_CharSize = 0x4b0;  // 1200 (likely UTF-16 encoding identifier)
        OutputDebugStringW(L"[UnicodeEncoding] Initialized with m_CharSize = 0x4b0\n");
    }

    ~UnicodeEncoding() override {}
};

} // namespace UnBCL

