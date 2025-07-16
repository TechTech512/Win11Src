// serializationstream.cpp

#include <windows.h>
#include <tchar.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>     // for malloc, free, _wtoi, _ultow
#include <stdint.h>     // for int64_t, uint8_t, etc.
#include <stdexcept>
#include <string>

namespace UnBCL {

enum Mode {
    Mode_Load = 0,
    Mode_Store = 1
};

class Object {
public:
    virtual ~Object() {}
};

class Stream : public Object {
public:
    virtual void Read(uint8_t* buffer, size_t count) = 0;
    virtual void Write(const uint8_t* buffer, size_t count) = 0;
};

class ISerializable {
public:
    virtual ~ISerializable() {}
};

class SerializationStream : public Object {
private:
    Mode m_Mode;
    Stream* m_Stream;

    void EnsureMode(Mode expected, const char* opName) {
        if (m_Mode != expected) {
            throw std::runtime_error(
                (std::string("Invalid operation in mode: ") + opName).c_str());
        }
    }

public:
    SerializationStream(Stream* stream, Mode mode)
        : m_Mode(mode), m_Stream(stream) {
        if (!stream)
            throw std::invalid_argument("Stream cannot be null.");
    }

    // Primitive read/write
    void ReadBytes(uint8_t* buffer, size_t count) {
        EnsureMode(Mode_Load, "Read");
        m_Stream->Read(buffer, count);
    }

    void WriteBytes(const uint8_t* buffer, size_t count) {
        EnsureMode(Mode_Store, "Write");
        m_Stream->Write(buffer, count);
    }

    // Primitive overloads
    template<typename T>
    SerializationStream& ReadPrimitive(T* value) {
        ReadBytes(reinterpret_cast<uint8_t*>(value), sizeof(T));
        return *this;
    }

    template<typename T>
    SerializationStream& WritePrimitive(const T& value) {
        WriteBytes(reinterpret_cast<const uint8_t*>(&value), sizeof(T));
        return *this;
    }

    // Operators >>
    SerializationStream& operator>>(int& value) { return ReadPrimitive(&value); }
    SerializationStream& operator>>(uint32_t& value) { return ReadPrimitive(&value); }
    SerializationStream& operator>>(int64_t& value) { return ReadPrimitive(&value); }
    SerializationStream& operator>>(uint64_t& value) { return ReadPrimitive(&value); }
    SerializationStream& operator>>(float& value) { return ReadPrimitive(&value); }
    SerializationStream& operator>>(double& value) { return ReadPrimitive(&value); }
    SerializationStream& operator>>(uint8_t& value) { return ReadPrimitive(&value); }
    SerializationStream& operator>>(int16_t& value) { return ReadPrimitive(&value); }
    SerializationStream& operator>>(uint16_t& value) { return ReadPrimitive(&value); }
    SerializationStream& operator>>(wchar_t& value) { return ReadPrimitive(&value); }

    // Operators <<
    SerializationStream& operator<<(int value) { return WritePrimitive(value); }
    SerializationStream& operator<<(uint32_t value) { return WritePrimitive(value); }
    SerializationStream& operator<<(int64_t value) { return WritePrimitive(value); }
    SerializationStream& operator<<(uint64_t value) { return WritePrimitive(value); }
    SerializationStream& operator<<(float value) { return WritePrimitive(value); }
    SerializationStream& operator<<(double value) { return WritePrimitive(value); }
    SerializationStream& operator<<(uint8_t value) { return WritePrimitive(value); }
    SerializationStream& operator<<(int16_t value) { return WritePrimitive(value); }
    SerializationStream& operator<<(uint16_t value) { return WritePrimitive(value); }
    SerializationStream& operator<<(wchar_t value) { return WritePrimitive(value); }

    // Example interface overloads
    SerializationStream& operator>>(ISerializable*& value) {
        EnsureMode(Mode_Load, "ReadObject");
        // Stub: ReadObject logic here
        value = nullptr;
        return *this;
    }

    SerializationStream& operator<<(ISerializable* value) {
        EnsureMode(Mode_Store, "WriteObject");
        // Stub: WriteObject logic here
        return *this;
    }

    // Destructor
    virtual ~SerializationStream() {}
};

} // namespace UnBCL

