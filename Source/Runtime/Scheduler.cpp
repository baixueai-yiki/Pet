#include "Scheduler.h"
#include <ctime>

long long GetUnixTimeSeconds()
{
    return static_cast<long long>(time(nullptr));
}

int GetLocalHour()
{
    time_t now = time(nullptr);
    struct tm localTime = {};
    localtime_s(&localTime, &now);
    return localTime.tm_hour;
}

DateTime GetLocalDateTime()
{
    time_t now = time(nullptr);
    struct tm localTime = {};
    localtime_s(&localTime, &now);
    DateTime out = {};
    out.year = localTime.tm_year + 1900;
    out.month = localTime.tm_mon + 1;
    out.day = localTime.tm_mday;
    out.hour = localTime.tm_hour;
    out.minute = localTime.tm_min;
    out.second = localTime.tm_sec;
    return out;
}

bool IsSleepHour(int hour)
{
    return hour >= 0 && hour < 6;
}
