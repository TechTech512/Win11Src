// object.cpp
#include <windows.h>

namespace UnBCL {

class Object {
public:
    Object();
    virtual ~Object();

    virtual int GetHashCode();
    virtual bool Equals(Object* obj);
    static bool ReferenceEquals(Object* a, Object* b);
};

// Constructor
Object::Object() {
}

// Destructor
Object::~Object() {
}

// GetHashCode returns the pointer value as int
int Object::GetHashCode() {
    return reinterpret_cast<int>(this);
}

// Default equality check: pointer comparison
bool Object::Equals(Object* obj) {
    return (this == obj);
}

// Static method to compare two object pointers
bool Object::ReferenceEquals(Object* a, Object* b) {
    return a == b;
}

} // namespace UnBCL

// CRT-safe operator new/delete definitions
void* __cdecl operator new(unsigned int size) {
    return HeapAlloc(GetProcessHeap(), 0, size);
}

void __cdecl operator delete(void* ptr, unsigned int) {
    if (ptr) HeapFree(GetProcessHeap(), 0, ptr);
}

void __cdecl operator delete(void* ptr) {
    if (ptr) HeapFree(GetProcessHeap(), 0, ptr);
}

void* __cdecl operator new[](unsigned int size) {
    return HeapAlloc(GetProcessHeap(), 0, size);
}

void __cdecl operator delete[](void* ptr) {
    if (ptr) HeapFree(GetProcessHeap(), 0, ptr);
}

