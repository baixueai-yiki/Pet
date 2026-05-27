#pragma once

#include <cstdint>
#include <string>

namespace pet::runtime {

struct DateTime {
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;
};

class Scheduler {
public:
    Scheduler() = delete;

    static std::int64_t GetUnixTimeSeconds();
    static int GetLocalHour();
    static DateTime GetLocalDateTime();
    static bool IsSleepHour(int hour);

    static int ScheduleEveryMs(const std::wstring& eventName, unsigned int intervalMs);
    static void CancelSchedule(int id);
    static void Tick();
    static void Clear();
};

} // namespace pet::runtime
