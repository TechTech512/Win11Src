#pragma once

namespace UnBCL {

class String {
private:
    wchar_t* m_Data;

public:
    String() : m_Data(nullptr) {}

    String(const wchar_t* str) {
        if (!str) str = L"";
        size_t len = wcslen(str);
        m_Data = new wchar_t[len + 1];
        wcscpy(m_Data, str);
    }

    ~String() {
        delete[] m_Data;
    }

    const wchar_t* CStr() const {
        return m_Data;
    }

    static String* Format(const wchar_t* fmt, const wchar_t* a = nullptr, unsigned long code = 0) {
        wchar_t buffer[512];
        if (a)
            swprintf(buffer, L"%s: %s [0x%08X]", fmt, a, code);
        else
            swprintf(buffer, L"%s [0x%08X]", fmt, code);
        return new String(buffer);
    }

    static String* Trim(const String* str, const wchar_t* /*unused*/) {
        if (!str || !str->m_Data) return new String();

        const wchar_t* input = str->m_Data;
        while (*input && *input <= 0x20) ++input;

        const wchar_t* end = input + wcslen(input);
        while (end > input && *(end - 1) <= 0x20) --end;

        size_t len = end - input;
        wchar_t* trimmed = new wchar_t[len + 1];
        wcsncpy(trimmed, input, len);
        trimmed[len] = L'\0';

        String* result = new String(trimmed);
        delete[] trimmed;
        return result;
    }
};

} // namespace UnBCL

