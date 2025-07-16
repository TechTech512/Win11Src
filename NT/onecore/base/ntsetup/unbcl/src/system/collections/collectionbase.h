#ifndef UNBCL_COLLECTIONBASE_H
#define UNBCL_COLLECTIONBASE_H

#include <windows.h>

class CollectionBase
{
protected:
    void** m_Items;
    int m_Count;
    int m_Capacity;

public:
    CollectionBase();
    virtual ~CollectionBase();

    int Count() const;
    void* GetAt(int index) const;
    void SetAt(int index, void* item);

    BOOL RemoveAt(int index);
    void Clear();

    BOOL Contains(void* item) const;
};

#endif // UNBCL_COLLECTIONBASE_H

