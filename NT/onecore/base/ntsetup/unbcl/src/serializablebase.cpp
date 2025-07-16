// serializablebase.cpp

#include <windows.h>

namespace UnBCL {

class SerializableBase {
protected:
    static SerializableBase* s_RegisteredList;
    static unsigned int s_NextTypeID;

    unsigned int m_TypeID;
    SerializableBase* m_Next;

public:
    SerializableBase()
        : m_TypeID(++s_NextTypeID), m_Next(nullptr)
    {
        m_Next = s_RegisteredList;
        s_RegisteredList = this;
    }

    virtual ~SerializableBase() {}

    static SerializableBase* GetFirstRegistered() {
        return s_RegisteredList;
    }

    SerializableBase* GetNext() const {
        return m_Next;
    }

    unsigned int GetTypeID() const {
        return m_TypeID;
    }
};

// === Static members ===
SerializableBase* SerializableBase::s_RegisteredList = nullptr;
unsigned int SerializableBase::s_NextTypeID = 0;

} // namespace UnBCL

