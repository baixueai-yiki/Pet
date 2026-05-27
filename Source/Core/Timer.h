#pragma once

#include <cstdint>
#include <string>

namespace pet::core {

struct DateTime {
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;
};

class Timer {
public:
    Timer() = delete;

    static std::int64_t NowUnixSeconds();
    static std::int64_t NowUnixMilliseconds();
    static DateTime GetLocalDateTime();
    static std::wstring NowLocalTimeString();
};

} // namespace pet::core
