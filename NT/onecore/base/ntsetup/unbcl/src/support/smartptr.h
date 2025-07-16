#pragma once

namespace UnBCL {

template <typename T>
class SmartPtr {
public:
    SmartPtr() : m_pObj(nullptr) {}
    SmartPtr(T* ptr) : m_pObj(ptr) { AddRef(); }
    SmartPtr(const SmartPtr<T>& other) : m_pObj(other.m_pObj) { AddRef(); }

    ~SmartPtr() { Release(); }

    SmartPtr<T>& operator=(const SmartPtr<T>& other) {
        if (this != &other) {
            Release();
            m_pObj = other.m_pObj;
            AddRef();
        }
        return *this;
    }

    T* operator->() const { return m_pObj; }
    T* get() const { return m_pObj; }
    T& operator*() const { return *m_pObj; }
    operator bool() const { return m_pObj != nullptr; }

    void DeAssign() { Release(); m_pObj = nullptr; }
    void Assign(T* ptr) {
        if (m_pObj != ptr) {
            Release();
            m_pObj = ptr;
            AddRef();
        }
    }

    static SmartPtr<T> Steal(SmartPtr<T>& source) {
        SmartPtr<T> result(source.m_pObj);
        source.m_pObj = nullptr;
        return result;
    }

private:
    void AddRef() {
        if (m_pObj)
            m_pObj->m_Ref++;  // assumes T has m_Ref field
    }

    void Release() {
        if (m_pObj && --m_pObj->m_Ref == 0) {
            delete m_pObj;
        }
    }

public:
    T* m_pObj;
    int _padding_;  // used for layout compatibility
};

} // namespace UnBCL
