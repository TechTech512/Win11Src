// exception.cpp

#include <windows.h>

namespace UnBCL {

static const wchar_t* DEFAULT_ERROR = L"Unknown exception";

// Forward declarations
class String;
template <typename T = void>
class ArrayList;

// === Supporting Types ===

class String {
private:
    wchar_t* m_Buffer;

public:
    String(const wchar_t* msg)
    {
        if (!msg) msg = DEFAULT_ERROR;
        size_t len = wcslen(msg);
        m_Buffer = new wchar_t[len + 1];
        wcscpy(m_Buffer, msg);
    }

    String(const String& other)
    {
        size_t len = wcslen(other.m_Buffer);
        m_Buffer = new wchar_t[len + 1];
        wcscpy(m_Buffer, other.m_Buffer);
    }

    ~String()
    {
        delete[] m_Buffer;
    }

    const wchar_t* CStr() const { return m_Buffer; }
};

template <typename T>
class ArrayList {
public:
    ~ArrayList() {}
};

// === Exception class ===

class Exception
{
private:
    String* m_Message;
    String* m_Source;
    int m_Result;
    Exception* m_Inner;
    ArrayList<>* m_PartialStackTrace;

public:
    Exception();
    Exception(wchar_t* msg);
    Exception(String* msg);
    Exception(Exception* inner, wchar_t* msg);
    Exception(String* msg, Exception* inner);
    virtual ~Exception();
};

// === Constructor 1: Default ===
Exception::Exception()
    : m_Message(new String(DEFAULT_ERROR)),
      m_Source(nullptr),
      m_Result(0),
      m_Inner(nullptr),
      m_PartialStackTrace(nullptr)
{}

// === Constructor 2: wchar_t* message ===
Exception::Exception(wchar_t* msg)
    : m_Message(new String(msg ? msg : DEFAULT_ERROR)),
      m_Source(nullptr),
      m_Result(0),
      m_Inner(nullptr),
      m_PartialStackTrace(nullptr)
{}

// === Constructor 3: String* message ===
Exception::Exception(String* msg)
    : m_Message(new String(msg ? msg->CStr() : DEFAULT_ERROR)),
      m_Source(nullptr),
      m_Result(0),
      m_Inner(nullptr),
      m_PartialStackTrace(nullptr)
{}

// === Constructor 4: Exception* inner, wchar_t* msg ===
Exception::Exception(Exception* inner, wchar_t* msg)
    : m_Message(new String(msg ? msg : DEFAULT_ERROR)),
      m_Source(nullptr),
      m_Result(0),
      m_Inner(inner),
      m_PartialStackTrace(nullptr)
{}

// === Constructor 5: String* msg, Exception* inner ===
Exception::Exception(String* msg, Exception* inner)
    : m_Message(new String(msg ? msg->CStr() : DEFAULT_ERROR)),
      m_Source(nullptr),
      m_Result(0),
      m_Inner(inner),
      m_PartialStackTrace(nullptr)
{}

// === Destructor ===
Exception::~Exception()
{
    delete m_Inner;
    delete m_Message;
    delete m_Source;
    delete m_PartialStackTrace;
}

} // namespace UnBCL

