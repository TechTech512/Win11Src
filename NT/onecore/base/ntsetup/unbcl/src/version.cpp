// version.cpp

#include <windows.h>
#include <tchar.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>     // for malloc, free, _wtoi, _ultow
#include <stdint.h>     // for int64_t, uint8_t, etc.
#include <stdexcept>

namespace UnBCL {

// === Basic String class ===

class String {
private:
    wchar_t* m_Data;

public:
    String(const wchar_t* str) {
        if (!str) str = L"";
        size_t len = wcslen(str);
        m_Data = new wchar_t[len + 1];
        wcscpy(m_Data, str);
    }

    String(const String& other) {
        size_t len = wcslen(other.m_Data);
        m_Data = new wchar_t[len + 1];
        wcscpy(m_Data, other.m_Data);
    }

    ~String() {
        delete[] m_Data;
    }

    const wchar_t* CStr() const { return m_Data; }
};

// === Version class ===

class Version {
private:
    int m_Major;
    int m_Minor;
    int m_Build;
    String* m_CSDVersion;

public:
    // Default constructor
    Version()
        : m_Major(0), m_Minor(0), m_Build(-1), m_CSDVersion(nullptr) {}

    // Copy constructor
    Version(const Version& other)
        : m_Major(other.m_Major), m_Minor(other.m_Minor),
          m_Build(other.m_Build), m_CSDVersion(nullptr) {
        if (other.m_CSDVersion) {
            m_CSDVersion = new String(*other.m_CSDVersion);
        }
    }

    // Major.Minor constructor
    Version(int major, int minor)
        : m_Major(major), m_Minor(minor), m_Build(-1), m_CSDVersion(nullptr) {
        if (major < 0 || minor < 0) {
            throw std::out_of_range("negative value to Version constructor");
        }
    }

    // Major.Minor.Build constructor
    Version(int major, int minor, int build)
        : m_Major(major), m_Minor(minor), m_Build(build), m_CSDVersion(nullptr) {
        if (major < 0 || minor < 0 || build < 0) {
            throw std::out_of_range("negative value to Version constructor");
        }
    }

    // Major.Minor.Build + CSD string
    Version(int major, int minor, int build, String* csd)
        : m_Major(major), m_Minor(minor), m_Build(build), m_CSDVersion(nullptr) {
        if (major < 0 || minor < 0 || build < 0) {
            throw std::out_of_range("negative value to Version constructor");
        }
        if (csd) {
            m_CSDVersion = new String(*csd);
        }
    }

    // Parse from "major.minor[.build[.revision]]"
    Version(const wchar_t* versionStr)
        : m_Major(0), m_Minor(0), m_Build(-1), m_CSDVersion(nullptr) {
        if (!versionStr) {
            throw std::invalid_argument("null version string to Version constructor");
        }

        int parts[3] = {0};
        int count = 0;

        wchar_t buffer[64];
        wcscpy_s(buffer, versionStr);
        wchar_t* token = wcstok(buffer, L".");
        while (token && count < 3) {
            parts[count++] = _wtoi(token);
            token = wcstok(nullptr, L".");
        }

        if (count < 2) {
            throw std::invalid_argument("not enough parts in version string");
        }

        m_Major = parts[0];
        m_Minor = parts[1];
        m_Build = (count >= 3) ? parts[2] : -1;

        if (m_Major < 0 || m_Minor < 0 || m_Build < -1) {
            throw std::out_of_range("negative value to Version constructor");
        }
    }

    ~Version() {
        delete m_CSDVersion;
    }

    // Optional: accessors
    int Major() const { return m_Major; }
    int Minor() const { return m_Minor; }
    int Build() const { return m_Build; }
    const String* CSDVersion() const { return m_CSDVersion; }
};

} // namespace UnBCL

