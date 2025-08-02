// string.cpp

#include <windows.h>
#include <string.h>
#include <memory.h>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <cctype>

namespace UnBCL {

class Object {
    // Base object implementation
};

class SerializationStream {
    // Serialization stream implementation
};

template<typename T>
class SmartPtr {
    T* m_pObj;
public:
    SmartPtr() : m_pObj(nullptr) {}
    explicit SmartPtr(T* obj) : m_pObj(obj) {}
    ~SmartPtr() { if (m_pObj) delete m_pObj; }
    
    T* operator->() const { return m_pObj; }
    T& operator*() const { return *m_pObj; }
    explicit operator bool() const { return m_pObj != nullptr; }
};

class String : public Object {
    std::wstring m_str;
    
public:
    // Constructors
    String() : m_str(L"") {}
    
    String(const wchar_t* str) : m_str(str ? str : L"") {}
    
    String(wchar_t ch, int count) {
        if (count < 0) {
            throw std::out_of_range("negative count to String constructor");
        }
        m_str = std::wstring(count, ch);
    }
    
    String(const wchar_t* str, int startIndex, int length) {
        if (!str) {
            throw std::invalid_argument("null value to String constructor");
        }
        if (startIndex < 0 || length < 0 || startIndex + length > wcslen(str)) {
            throw std::out_of_range("startIndex and/or value out of range");
        }
        m_str = std::wstring(str + startIndex, length);
    }
    
    String(const String& other) : m_str(other.m_str) {}
    
    String(SerializationStream& stream) {
        // Deserialization implementation
    }
    
    // Destructor
    ~String() = default;
    
    // Comparison methods
    static int Compare(const SmartPtr<String>& str1, const SmartPtr<String>& str2, bool ignoreCase = false) {
        const wchar_t* s1 = str1 ? str1->m_str.c_str() : nullptr;
        const wchar_t* s2 = str2 ? str2->m_str.c_str() : nullptr;
        
        if (!s1) return s2 ? -1 : 0;
        if (!s2) return 1;
        
        if (ignoreCase) {
            return _wcsicmp(s1, s2);
        }
        return wcscmp(s1, s2);
    }
    
    int CompareTo(const SmartPtr<String>& other) const {
        return Compare(SmartPtr<String>(const_cast<String*>(this)), other, false);
    }
    
    int CompareTo(const SmartPtr<String>& other, bool ignoreCase) const {
        return Compare(SmartPtr<String>(const_cast<String*>(this)), other, ignoreCase);
    }
    
    // Concatenation methods
    static SmartPtr<String> Concat(const SmartPtr<String>& str1, const SmartPtr<String>& str2) {
        std::wstring result;
        if (str1) result += str1->m_str;
        if (str2) result += str2->m_str;
        return SmartPtr<String>(new String(result.c_str()));
    }
    
    // Similar implementations for other Concat overloads
    
    // Search methods
    bool EndsWith(const SmartPtr<String>& value) const {
        if (!value) throw std::invalid_argument("value cannot be null");
        
        const std::wstring& val = value->m_str;
        if (val.length() > m_str.length()) return false;
        
        return std::equal(val.rbegin(), val.rend(), m_str.rbegin());
    }
    
    int IndexOf(const SmartPtr<String>& value) const {
        return IndexOf(value, 0);
    }
    
    int IndexOf(const SmartPtr<String>& value, int startIndex) const {
        if (!value) throw std::invalid_argument("value cannot be null");
        if (startIndex < 0 || startIndex > static_cast<int>(m_str.length())) {
            throw std::out_of_range("startIndex out of range");
        }
        
        size_t pos = m_str.find(value->m_str, startIndex);
        return pos == std::wstring::npos ? -1 : static_cast<int>(pos);
    }
    
    // Similar implementations for other IndexOf methods
    
    // Manipulation methods
    SmartPtr<String> Insert(int startIndex, const SmartPtr<String>& value) const {
        if (!value) throw std::invalid_argument("value cannot be null");
        if (startIndex < 0 || startIndex > static_cast<int>(m_str.length())) {
            throw std::out_of_range("startIndex out of range");
        }
        
        std::wstring result = m_str;
        result.insert(startIndex, value->m_str);
        return SmartPtr<String>(new String(result.c_str()));
    }
    
    SmartPtr<String> Remove(int startIndex, int count) const {
        if (startIndex < 0 || count < 0 || startIndex + count > static_cast<int>(m_str.length())) {
            throw std::out_of_range("Invalid startIndex or count");
        }
        
        std::wstring result = m_str;
        result.erase(startIndex, count);
        return SmartPtr<String>(new String(result.c_str()));
    }
    
    SmartPtr<String> Replace(const SmartPtr<String>& oldValue, const SmartPtr<String>& newValue) const {
        if (!oldValue) throw std::invalid_argument("oldValue cannot be null");
        
        std::wstring result = m_str;
        size_t pos = 0;
        const std::wstring& oldStr = oldValue->m_str;
        const std::wstring& newStr = newValue ? newValue->m_str : L"";
        
        while ((pos = result.find(oldStr, pos)) != std::wstring::npos) {
            result.replace(pos, oldStr.length(), newStr);
            pos += newStr.length();
        }
        
        return SmartPtr<String>(new String(result.c_str()));
    }
    
    // Substring methods
    SmartPtr<String> Substring(int startIndex) const {
        return Substring(startIndex, m_str.length() - startIndex);
    }
    
    SmartPtr<String> Substring(int startIndex, int length) const {
        if (startIndex < 0 || length < 0 || startIndex + length > static_cast<int>(m_str.length())) {
            throw std::out_of_range("Invalid startIndex or length");
        }
        
        return SmartPtr<String>(new String(m_str.substr(startIndex, length).c_str()));
    }
    
    // Case conversion
    SmartPtr<String> ToLower() const {
        std::wstring result = m_str;
        std::transform(result.begin(), result.end(), result.begin(), towlower);
        return SmartPtr<String>(new String(result.c_str()));
    }
    
    SmartPtr<String> ToUpper() const {
        std::wstring result = m_str;
        std::transform(result.begin(), result.end(), result.begin(), towupper);
        return SmartPtr<String>(new String(result.c_str()));
    }
    
    // Trim methods
    SmartPtr<String> Trim() const {
        static const wchar_t defaultTrimChars[] = L" \t\n\r";
        return Trim(SmartPtr<String>(new String(defaultTrimChars)));
    }
    
    SmartPtr<String> Trim(const SmartPtr<String>& trimChars) const {
        const wchar_t* chars = trimChars ? trimChars->m_str.c_str() : L" \t\n\r";
        size_t start = m_str.find_first_not_of(chars);
        if (start == std::wstring::npos) return SmartPtr<String>(new String(L""));
        
        size_t end = m_str.find_last_not_of(chars);
        return Substring(static_cast<int>(start), static_cast<int>(end - start + 1));
    }
    
    // Similar implementations for TrimStart and TrimEnd
    
    // Access to underlying string
    const wchar_t* c_str() const { return m_str.c_str(); }
    size_t length() const { return m_str.length(); }
    
    // Operator overloads
    bool operator==(const String& other) const { return m_str.compare(other.m_str) == 0; }
    bool operator!=(const String& other) const { return m_str.compare(other.m_str) != 0; }
};

} // namespace UnBCL

