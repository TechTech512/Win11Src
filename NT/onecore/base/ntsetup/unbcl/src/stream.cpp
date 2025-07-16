// stream.cpp

#include <windows.h>

class Object {
public:
    virtual ~Object() {}
};

class IDisposable {
public:
    virtual void Dispose() = 0;
};

class Stream : public Object, public IDisposable {
protected:
    int m_Counter;

public:
    Stream() : m_Counter(0) {}
    Stream(const Stream& other) : m_Counter(other.m_Counter) {}

    virtual ~Stream() {}

    virtual int Read(BYTE* buffer, int count) = 0;
    virtual int Write(const BYTE* buffer, int count) = 0;
    virtual void Close() = 0;

    void Dispose() override { Close(); }
};

