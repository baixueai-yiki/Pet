#include "Scheduler.h"
#include "EventBus.h"
#include <windows.h>
#include <ctime>
#include <vector>

// 获取当前 Unix 时间戳（秒）
long long GetUnixTimeSeconds()
{
    return static_cast<long long>(time(nullptr));
}

// 获取当前本地小时
int GetLocalHour()
{
    time_t now = time(nullptr);
    struct tm localTime = {};
    localtime_s(&localTime, &now);
    return localTime.tm_hour;
}

// 获取当前本地日期时间
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

// 判断是否处于睡眠时段
bool IsSleepHour(int hour)
{
    return hour >= 0 && hour < 6;
}

// 周期调度记录
struct ScheduleEntry
{
    int id;
    std::wstring eventName;
    unsigned int intervalMs;
    DWORD lastTick;
};

static std::vector<ScheduleEntry> s_entries;
static int s_nextScheduleId = 1;

// 注册周期性事件
int ScheduleEveryMs(const std::wstring& eventName, unsigned int intervalMs)
{
    if (intervalMs == 0)
        return 0;

    const int id = s_nextScheduleId++;
    ScheduleEntry e;
    e.id = id;
    e.eventName = eventName;
    e.intervalMs = intervalMs;
    e.lastTick = GetTickCount();
    s_entries.push_back(e);
    return id;
}

// 取消一个调度任务
void CancelSchedule(int id)
{
    for (size_t i = 0; i < s_entries.size(); ++i)
    {
        if (s_entries[i].id == id)
        {
            s_entries.erase(s_entries.begin() + static_cast<long long>(i));
            return;
        }
    }
}

// 轮询调度器并触发到期事件
void SchedulerTick()
{
    if (s_entries.empty())
        return;

    const DWORD now = GetTickCount();
    for (auto& e : s_entries)
    {
        const DWORD elapsed = now - e.lastTick;
        if (elapsed >= e.intervalMs)
        {
            e.lastTick = now;
            EventEmit(e.eventName);
        }
    }
}

// 清空所有调度任务
void SchedulerClear()
{
    s_entries.clear();
    s_nextScheduleId = 1;
}
