// registrykey.cpp

#include <windows.h>

namespace UnBCL {

class RegistryKey {
private:
    HKEY m_hKey;
    bool m_OwnsHandle;

public:
    RegistryKey()
        : m_hKey(nullptr), m_OwnsHandle(false) {}

    RegistryKey(HKEY handle, bool ownsHandle)
        : m_hKey(handle), m_OwnsHandle(ownsHandle) {}

    ~RegistryKey() {
        Close();
    }

    void Close() {
        if (m_OwnsHandle && m_hKey) {
            RegCloseKey(m_hKey);
            m_hKey = nullptr;
            m_OwnsHandle = false;
        }
    }

    bool IsValid() const {
        return m_hKey != nullptr;
    }

    HKEY GetHandle() const {
        return m_hKey;
    }

    // Add additional wrappers for RegQueryValueEx, RegSetValueEx, etc. as needed.
};

} // namespace UnBCL

