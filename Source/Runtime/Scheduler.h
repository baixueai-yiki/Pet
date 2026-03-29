#pragma once
#include <string>

// 简单的本地时间快照（24小时制）。
struct DateTime
{
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
};

// 当前时间戳（秒）。
long long GetUnixTimeSeconds();

// 当前本地小时（0-23）。
int GetLocalHour();

// 当前本地日期/时间快照。
DateTime GetLocalDateTime();

// 判断是否处于睡眠时段（00:00 - 05:59）。
bool IsSleepHour(int hour);

// 按事件名注册周期性触发（毫秒）。
int ScheduleEveryMs(const std::wstring& eventName, unsigned int intervalMs);

// 取消一个调度任务。
void CancelSchedule(int id);

// 调度器更新（主循环或定时器里调用）。
void SchedulerTick();

// 清空所有调度任务。
void SchedulerClear();
