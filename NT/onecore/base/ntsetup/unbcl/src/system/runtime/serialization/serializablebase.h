#ifndef UNBCL_SERIALIZABLEBASE_H
#define UNBCL_SERIALIZABLEBASE_H

#include <windows.h>

// Forward declaration to avoid dependency loop
class SerializationStream;

class SerializableBase
{
public:
    virtual ~SerializableBase() {}

    // Serialize the object into a stream
    virtual BOOL Serialize(SerializationStream* stream) = 0;

    // Deserialize the object from a stream
    virtual BOOL Deserialize(SerializationStream* stream) = 0;
};

#endif // UNBCL_SERIALIZABLEBASE_H

