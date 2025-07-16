#ifndef UNBCL_STACK_H
#define UNBCL_STACK_H

#include <windows.h>

class Stack
{
private:
    void** m_Items;
    int m_Top;
    int m_Capacity;

public:
    Stack();
    ~Stack();

    BOOL Push(void* item);
    void* Pop();
    void* Peek() const;

    BOOL IsEmpty() const;
    int Count() const;

    void Clear();
};

#endif // UNBCL_STACK_H
