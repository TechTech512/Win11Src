#ifndef UNBCL_ENCODING_H
#define UNBCL_ENCODING_H

#include <windows.h>

namespace UnBCL {

class Decoder {
public:
    virtual ~Decoder() {}
    
    // Add decoding-related virtual methods as needed
    virtual int Decode(const BYTE* bytes, int byteCount, WCHAR* chars, int charCount) {
        return 0;  // Stub
    }
};

class Encoding {
public:
    virtual ~Encoding() {}

    // Retrieve a decoder for this encoding
    virtual Decoder* GetDecoder() {
        return nullptr;  // Stub
    }

    // Encode Unicode characters into bytes
    virtual int GetBytes(const WCHAR* chars, int charCount, BYTE* bytes, int byteCount) {
        return 0;  // Stub
    }

    // Decode bytes into Unicode characters
    virtual int GetChars(const BYTE* bytes, int byteCount, WCHAR* chars, int charCount) {
        return 0;  // Stub
    }

    // Optional: get the byte count needed to encode characters
    virtual int GetByteCount(const WCHAR* chars, int charCount) {
        return 0;  // Stub
    }

    // Optional: get the character count from bytes
    virtual int GetCharCount(const BYTE* bytes, int byteCount) {
        return 0;  // Stub
    }
};

} // namespace UnBCL

#endif // UNBCL_ENCODING_H

