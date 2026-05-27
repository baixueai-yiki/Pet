#include "Core/Timer.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace pet::core {

std::int64_t Timer::NowUnixSeconds() {
    return static_cast<std::int64_t>(std::time(nullptr));
}

std::int64_t Timer::NowUnixMilliseconds() {
    const auto now = std::chrono::system_clock::now();
    return static_cast<std::int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
}

DateTime Timer::GetLocalDateTime() {
    std::time_t t = std::time(nullptr);
    std::tm lt{};
    localtime_s(&lt, &t);
    return DateTime{lt.tm_year + 1900, lt.tm_mon + 1, lt.tm_mday, lt.tm_hour, lt.tm_min, lt.tm_sec};
}

std::wstring Timer::NowLocalTimeString() {
    const auto dt = GetLocalDateTime();
    std::wstringstream ss;
    ss << dt.year << L"-" << std::setw(2) << std::setfill(L'0') << dt.month
       << L"-" << std::setw(2) << std::setfill(L'0') << dt.day
       << L" " << std::setw(2) << std::setfill(L'0') << dt.hour
       << L":" << std::setw(2) << std::setfill(L'0') << dt.minute
       << L":" << std::setw(2) << std::setfill(L'0') << dt.second;
    return ss.str();
}

} // namespace pet::core
