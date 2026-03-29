#pragma once

// Simple local time snapshot (24-hour clock).
struct DateTime
{
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
};

// Current Unix timestamp (seconds).
long long GetUnixTimeSeconds();

// Current local hour (0-23).
int GetLocalHour();

// Current local date/time snapshot.
DateTime GetLocalDateTime();

// Sleep hours check (00:00 - 05:59).
bool IsSleepHour(int hour);
