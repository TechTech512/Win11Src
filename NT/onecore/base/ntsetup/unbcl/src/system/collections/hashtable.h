#ifndef UNBCL_HASHTABLE_H
#define UNBCL_HASHTABLE_H

#include <windows.h>

class Hashtable
{
private:
    struct Entry {
        void* Key;
        void* Value;
        Entry* Next;
    };

    Entry** m_Table;
    int m_Size;
    int m_Capacity;

public:
    Hashtable();
    ~Hashtable();

    BOOL Insert(void* key, void* value);
    void* Lookup(void* key);
    BOOL Remove(void* key);

    int Count() const;
    void Clear();
};

#endif // UNBCL_HASHTABLE_H

