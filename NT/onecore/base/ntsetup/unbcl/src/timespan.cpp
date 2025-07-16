// timespan.cpp

#include <windows.h>
#include <tchar.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>     // for malloc, free, _wtoi, _ultow
#include <stdint.h>     // for int64_t, uint8_t, etc.
#include <stdexcept>

namespace UnBCL {

class Object {
public:
    virtual ~Object() {}
};

class ArgumentOutOfRangeException : public std::runtime_error {
public:
    ArgumentOutOfRangeException(const char* msg)
        : std::runtime_error(msg) {}
};

// === TimeSpan ===

class TimeSpan : public Object {
private:
    int64_t m_Ticks; // 1 tick = 1 millisecond

    static const int64_t MaxTicks = INT64_C(9223372036854775807);  // placeholder for safe bounds

public:
    // Tick constructor
    TimeSpan(int64_t ticks)
        : m_Ticks(ticks) {}

    // Copy constructor
    TimeSpan(const TimeSpan& other)
        : m_Ticks(other.m_Ticks) {}

    // HMS constructor
    TimeSpan(int hours, int minutes, int seconds) {
        InitFromTime(0, hours, minutes, seconds, 0);
    }

    // DHMS constructor
    TimeSpan(int days, int hours, int minutes, int seconds) {
        InitFromTime(days, hours, minutes, seconds, 0);
    }

    // DHMS + milliseconds constructor
    TimeSpan(int days, int hours, int minutes, int seconds, int milliseconds) {
        InitFromTime(days, hours, minutes, seconds, milliseconds);
    }

    // Convert to double (milliseconds)
    operator double() const {
        return static_cast<double>(m_Ticks) / 10000.0;
    }

    int64_t GetTicks() const {
        return m_Ticks;
    }

private:
    void InitFromTime(int d, int h, int m, int s, int ms) {
        int64_t totalSeconds = ((((int64_t)d * 24 + h) * 60 + m) * 60 + s);
        int64_t totalMillis = totalSeconds * 1000 + ms;

        if (totalMillis < 0 || totalMillis > MaxTicks / 10000) {
            throw ArgumentOutOfRangeException("overflow: TimeSpan too long");
        }

        m_Ticks = totalMillis * 10000; // Convert to 100-nanosecond ticks
    }
};

} // namespace UnBCL

