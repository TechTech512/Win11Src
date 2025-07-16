// unbclbase.h
#pragma once

namespace UnBCL {

class Object {
public:
    virtual ~Object() {}
};

class IDisposable {
public:
    virtual ~IDisposable() {}
    virtual void Dispose() = 0;
};

class TextReader : public Object {
public:
    virtual ~TextReader() {}
};

class TextWriter : public Object {
public:
    virtual ~TextWriter() {}
};

}

