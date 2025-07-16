#ifndef UNBCL_ARRAYLIST_H
#define UNBCL_ARRAYLIST_H

#include <windows.h>

class ArrayList
{
private:
    void** m_Elements;
    int m_Size;
    int m_Capacity;

public:
    ArrayList();
    ~ArrayList();

    BOOL Add(void* item);
    BOOL Remove(void* item);
    void* Get(int index) const;
    int IndexOf(void* item) const;
    int Count() const;
    void Clear();
};

#endif // UNBCL_ARRAYLIST_H

