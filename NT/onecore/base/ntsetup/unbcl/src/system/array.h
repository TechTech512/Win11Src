#ifndef UNBCL_ARRAY_H
#define UNBCL_ARRAY_H

#include <windows.h>

class Array
{
private:
    void** m_Data;
    int m_Length;

public:
    Array();
    explicit Array(int length);
    Array(const Array& other);
    ~Array();

    void* GetAt(int index) const;
    void SetAt(int index, void* value);

    int Length() const;

    void Clear();
};

#endif // UNBCL_ARRAY_H

