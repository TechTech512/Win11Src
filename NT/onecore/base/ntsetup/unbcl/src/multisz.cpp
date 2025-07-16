// multisz.cpp

#include <windows.h>

namespace UnBCL {

class Object {
public:
    virtual ~Object() {}
};

class Allocator {
public:
    static void* Alloc(size_t size, int /*flags*/) {
        return HeapAlloc(GetProcessHeap(), 0, size);
    }

    static void Free(void* p, int /*flags*/) {
        HeapFree(GetProcessHeap(), 0, p);
    }
};

class MultiSz : public Object {
private:
    wchar_t* m_pszBuffer;
    wchar_t* m_pszTerminator;
    unsigned int m_cchCapacity;

    // === Iterator class ===
public:
    class Enumerator {
    private:
        const wchar_t* m_pszCurrent;
        const wchar_t* m_pszFirst;

    public:
        Enumerator(wchar_t* p)
            : m_pszCurrent(nullptr), m_pszFirst(p) {}

        const wchar_t* Next() {
            if (!m_pszFirst || *m_pszFirst == L'\0')
                return nullptr;

            if (!m_pszCurrent)
                m_pszCurrent = m_pszFirst;
            else
                m_pszCurrent += wcslen(m_pszCurrent) + 1;

            if (*m_pszCurrent == L'\0')
                return nullptr;

            return m_pszCurrent;
        }
    };

public:
    // === Default constructor
    MultiSz()
        : m_pszBuffer(nullptr), m_pszTerminator(nullptr), m_cchCapacity(0) {}

    // === Constructor from wchar_t*
    MultiSz(const wchar_t* multiSz)
        : m_pszBuffer(nullptr), m_pszTerminator(nullptr), m_cchCapacity(0)
    {
        if (!multiSz || *multiSz == L'\0')
            return;

        const wchar_t* p = multiSz;
        int count = 0;

        while (*p) {
            int len = wcslen(p);
            p += len + 1;
            count += len + 1;
        }

        m_cchCapacity = count + 1;
        m_pszBuffer = (wchar_t*)Allocator::Alloc(m_cchCapacity * sizeof(wchar_t), 0);
        memcpy(m_pszBuffer, multiSz, m_cchCapacity * sizeof(wchar_t));
        m_pszTerminator = m_pszBuffer + m_cchCapacity - 1;
    }

    // === Constructor from byte array (for deserialization)
    MultiSz(const unsigned char* data, unsigned int length, unsigned long* flagsOut = nullptr)
        : m_pszBuffer(nullptr), m_pszTerminator(nullptr), m_cchCapacity(0)
    {
        if (flagsOut) *flagsOut = 0;
        if (!data || length == 0) return;

        unsigned int cch = length / sizeof(wchar_t);
        unsigned int flags = 0;

        if (length % 2) flags |= 1;

        if (cch > 0) {
            if (((wchar_t*)data)[cch - 1] != 0) {
                cch += 2;
                flags |= 6;
            } else if (cch > 1 && ((wchar_t*)data)[cch - 2] != 0) {
                cch += 1;
                flags |= 2;
            }
        }

        m_cchCapacity = cch;
        m_pszBuffer = (wchar_t*)Allocator::Alloc(m_cchCapacity * sizeof(wchar_t), 0);
        memcpy(m_pszBuffer, data, length);
        m_pszBuffer[m_cchCapacity - 1] = L'\0';
        if (m_cchCapacity > 1) m_pszBuffer[m_cchCapacity - 2] = L'\0';
        m_pszTerminator = m_pszBuffer + m_cchCapacity - 1;

        if (flagsOut) *flagsOut = flags;
    }

    // === Copy constructor
    MultiSz(const MultiSz& other)
        : m_pszBuffer(nullptr), m_pszTerminator(nullptr), m_cchCapacity(0)
    {
        if (other.m_pszBuffer) {
            m_cchCapacity = other.m_cchCapacity;
            m_pszBuffer = (wchar_t*)Allocator::Alloc(m_cchCapacity * sizeof(wchar_t), 0);
            memcpy(m_pszBuffer, other.m_pszBuffer, m_cchCapacity * sizeof(wchar_t));
            m_pszTerminator = m_pszBuffer + (other.m_pszTerminator - other.m_pszBuffer);
        }
    }

    // === Destructor
    ~MultiSz() {
        if (m_pszBuffer)
            Allocator::Free(m_pszBuffer, 0);
    }

    // === Accessors
    wchar_t* Buffer() const { return m_pszBuffer; }
    unsigned int Length() const { return m_cchCapacity; }

    Enumerator GetEnumerator() {
        return Enumerator(m_pszBuffer);
    }
};

} // namespace UnBCL

