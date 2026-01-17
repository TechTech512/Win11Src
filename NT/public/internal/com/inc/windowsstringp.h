// windowsstringp.h
#pragma once

#include <Windows.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct StringReference {
    HSTRING _hstring;
    HSTRING_HEADER _header;
} StringReference;

// Windows Runtime string functions
HRESULT WINAPI WindowsCreateStringReference(
    PCWSTR sourceString,
    UINT32 length,
    HSTRING_HEADER* header,
    HSTRING* string
);

#ifdef __cplusplus
}

namespace Windows {
namespace Internal {

class StringReference
{
public:
    template<size_t N>
    StringReference(wchar_t (&str)[N])
    {
        HRESULT hr = WindowsCreateStringReference(str, N - 1, &_header, &_hstring);
        if (FAILED(hr))
        {
            RaiseException(STATUS_INVALID_PARAMETER, 1, 0, nullptr);
        }
    }

    ~StringReference()
    {
        if (_hstring)
        {
            WindowsDeleteString(_hstring);
        }
    }

    HSTRING Get() const { return _hstring; }

    HSTRING _hstring;
    HSTRING_HEADER _header;
};

} // namespace Internal
} // namespace Windows

#endif // __cplusplus

