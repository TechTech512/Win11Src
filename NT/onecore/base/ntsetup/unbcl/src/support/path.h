#pragma once

#include "_string.h"

namespace UnBCL {

class Path {
public:
    static bool EndsWithSeparator(const String* path) {
        const wchar_t* buf = path->Buffer();
        size_t len = 0;
        while (buf[len] != 0) len++;
        if (len == 0) return false;
        return buf[len - 1] == L'\\' || buf[len - 1] == L'/';
    }

    static String* Combine(const String* base, const wchar_t* tail) {
        static wchar_t combined[512];
        const wchar_t* baseStr = base->Buffer();

        // Copy base
        int i = 0;
        while (baseStr[i] != 0 && i < 510) {
            combined[i] = baseStr[i];
            i++;
        }

        // Ensure separator
        if (i > 0 && baseStr[i - 1] != L'\\') {
            combined[i++] = L'\\';
        }

        // Append tail
        int j = 0;
        while (tail[j] != 0 && i < 511) {
            combined[i++] = tail[j++];
        }

        combined[i] = 0;
        return new String(combined);
    }

    static String* Combine(const String* base, const String* tail) {
        return Combine(base, tail->Buffer());
    }
};

} // namespace UnBCL

