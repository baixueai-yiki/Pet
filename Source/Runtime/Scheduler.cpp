#include "Runtime/Scheduler.h"

#include "Runtime/EventBus.h"

#include <ctime>
#include <vector>
#include <windows.h>

namespace pet::runtime {
namespace {

struct ScheduleEntry {
    int id = 0;
    std::wstring eventName;
    unsigned int intervalMs = 0;
    DWORD lastTick = 0;
};

std::vector<ScheduleEntry> g_entries;
int g_nextScheduleId = 1;

} // namespace

std::int64_t Scheduler::GetUnixTimeSeconds() {
    return static_cast<std::int64_t>(std::time(nullptr));
}

int Scheduler::GetLocalHour() {
    std::time_t now = std::time(nullptr);
    std::tm localTime = {};
    localtime_s(&localTime, &now);
    return localTime.tm_hour;
}

DateTime Scheduler::GetLocalDateTime() {
    std::time_t now = std::time(nullptr);
    std::tm localTime = {};
    localtime_s(&localTime, &now);

    DateTime out;
    out.year = localTime.tm_year + 1900;
    out.month = localTime.tm_mon + 1;
    out.day = localTime.tm_mday;
    out.hour = localTime.tm_hour;
    out.minute = localTime.tm_min;
    out.second = localTime.tm_sec;
    return out;
}

bool Scheduler::IsSleepHour(int hour) {
    return hour >= 0 && hour < 6;
}

int Scheduler::ScheduleEveryMs(const std::wstring& eventName, unsigned int intervalMs) {
    if (intervalMs == 0) {
        return 0;
    }

    const int id = g_nextScheduleId++;
    ScheduleEntry e;
    e.id = id;
    e.eventName = eventName;
    e.intervalMs = intervalMs;
    e.lastTick = GetTickCount();
    g_entries.push_back(e);
    return id;
}

void Scheduler::CancelSchedule(int id) {
    for (size_t i = 0; i < g_entries.size(); ++i) {
        if (g_entries[i].id == id) {
            g_entries.erase(g_entries.begin() + static_cast<long long>(i));
            return;
        }
    }
}

void Scheduler::Tick() {
    if (g_entries.empty()) {
        return;
    }

    const DWORD now = GetTickCount();
    for (auto& e : g_entries) {
        const DWORD elapsed = now - e.lastTick;
        if (elapsed >= e.intervalMs) {
            e.lastTick = now;
            EventBus::Emit(e.eventName);
        }
    }
}

void Scheduler::Clear() {
    g_entries.clear();
    g_nextScheduleId = 1;
}

} // namespace pet::runtime
