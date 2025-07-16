#ifndef UNBCL_SERIALIZATIONSTREAM_H
#define UNBCL_SERIALIZATIONSTREAM_H

#include <windows.h>

class SerializationStream
{
public:
    virtual ~SerializationStream() {}

    // Raw byte access
    virtual BOOL ReadBytes(BYTE* buffer, int count) = 0;
    virtual BOOL WriteBytes(const BYTE* buffer, int count) = 0;

    // Integers
    virtual BOOL ReadInt32(int* value) = 0;
    virtual BOOL WriteInt32(int value) = 0;

    virtual BOOL ReadUInt16(WORD* value) = 0;
    virtual BOOL WriteUInt16(WORD value) = 0;

    // Wide strings (length-prefixed or null-terminated)
    virtual BOOL ReadString(wchar_t* buffer, int maxChars) = 0;
    virtual BOOL WriteString(const wchar_t* str) = 0;
};

#endif // UNBCL_SERIALIZATIONSTREAM_H

