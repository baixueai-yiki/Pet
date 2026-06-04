#include "RunComponent.h"
#include "AudioComponent.h"
#include "ChatComponent.h"
#include "Systems/UI/UIActor.h"
#include "Systems/UI/UIPanels/ChatPanel/BubbleChatPanel.h"
#include "Systems/UI/UIPanels/ChatPanel/InputChatPanel.h"
#include "Systems/UI/UIPanels/ChatPanel/OptionChatPanel.h"
#include "Systems/UI/UIPanels/ToolPanel/SettingToolPanel.h"
#include "Systems/UI/UIPanels/ToolPanel/MusicToolPanel.h"
#include "Systems/UI/UIPanels/DialoguePanel.h"
#include "Core/PetState.h"
#include "Core/Path.h"
#include "Core/TextFile.h"
#include "Engine/Input/InputDispatcher.h"
#include "Runtime/EventBus.h"
#include "Runtime/StateManager.h"
#include "Runtime/Scheduler.h"
#include <cstdlib>
#include <fstream>
#include <map>
#include <string>
#include <vector>
#include <algorithm>

// --- 启动模式检测 + 条目加载（从 PetActor 迁移至此） ---

static std::wstring GetCurrentDateStr()
{
    SYSTEMTIME st; GetLocalTime(&st);
    return std::to_wstring(st.wYear) + L"-" +
        (st.wMonth < 10 ? L"0" : L"") + std::to_wstring(st.wMonth) + L"-" +
        (st.wDay < 10 ? L"0" : L"") + std::to_wstring(st.wDay);
}

static bool ParseDateStr(const std::wstring& s, int& y, int& m, int& d)
{
    if (s.size() != 10 || s[4] != L'-' || s[7] != L'-') return false;
    y = _wtoi(s.substr(0,4).c_str()); m = _wtoi(s.substr(5,2).c_str()); d = _wtoi(s.substr(8,2).c_str());
    return (y >= 2020 && m >= 1 && m <= 12 && d >= 1 && d <= 31);
}

static int DaysBetween(const std::wstring& d1, const std::wstring& d2)
{
    int y1,m1,dd1,y2,m2,dd2;
    if (!ParseDateStr(d1,y1,m1,dd1) || !ParseDateStr(d2,y2,m2,dd2)) return 0;
    SYSTEMTIME st1={},st2={}; st1.wYear=y1;st1.wMonth=m1;st1.wDay=dd1; st2.wYear=y2;st2.wMonth=m2;st2.wDay=dd2;
    FILETIME ft1,ft2; SystemTimeToFileTime(&st1,&ft1); SystemTimeToFileTime(&st2,&ft2);
    ULARGE_INTEGER u1={ft1.dwLowDateTime,ft1.dwHighDateTime}, u2={ft2.dwLowDateTime,ft2.dwHighDateTime};
    return (int)((u2.QuadPart - u1.QuadPart) / 864000000000LL);
}

static bool ParseJsonInt64(const std::wstring& text, const std::wstring& key, long long& out);

enum class StartupMode { Normal=0, TimeBug=1, AbnormalExit=2, LongAway=3, SpecialDate=4, Birthday=5 };

static StartupMode DetectStartupMode()
{
    // 读 last_run_date："not" = 上次非正常退出
    bool normalExit = false;
    std::wstring lastDate;
    {
        std::wstring text;
        if (TextFile::ReadText(GetConfigPath(L"state.json"), text))
        {
            size_t pos = text.find(L"\"last_run_date\"");
            if (pos != std::wstring::npos) {
                pos = text.find(L'"', pos + 15);
                if (pos != std::wstring::npos) {
                    size_t end = text.find(L'"', pos + 1);
                    if (end != std::wstring::npos) {
                        lastDate = text.substr(pos + 1, end - pos - 1);
                        if (!lastDate.empty() && lastDate != L"not" && lastDate.find(L"000000") != 0)
                            normalExit = true;
                        // 立即改为 "not"——本会话若被杀则保留
                        text.replace(pos + 1, end - pos - 1, L"not");
                        std::wstring path = GetConfigPath(L"state.json");
                        std::ofstream out(path, std::ios::binary | std::ios::trunc);
                        if (out.is_open()) {
                            std::string utf8; int len = WideCharToMultiByte(CP_UTF8,0,text.c_str(),-1,nullptr,0,nullptr,nullptr);
                            utf8.resize(len); WideCharToMultiByte(CP_UTF8,0,text.c_str(),-1,&utf8[0],len,nullptr,nullptr);
                            out << utf8; out.close();
                        }
                    }
                }
            }
        }
    }

    // 检查时间倒退（用已保存的 lastDate）
    bool timeBug = false;
    int  elapsedDays = 0;
    if (!lastDate.empty() && lastDate != L"not") {
        elapsedDays = DaysBetween(lastDate, GetCurrentDateStr());
        if (elapsedDays < 0) timeBug = true;
    }

    // 检查今天是否有特殊日期条目
    bool hasSpecial = false;
    {
        SYSTEMTIME st; GetLocalTime(&st);
        std::map<std::wstring, std::wstring> map;
        if (LoadIdleMap(map))
            for (const auto& kv : map)
                if (kv.first.rfind(L"run_" + std::to_wstring(st.wMonth) + L"_" + std::to_wstring(st.wDay), 0) == 0)
                { hasSpecial = true; break; }
    }

    // 检查生日
    bool isBirthday = false;
    {
        SYSTEMTIME st; GetLocalTime(&st); wchar_t today[8]; swprintf_s(today,8,L"%02d-%02d",st.wMonth,st.wDay);
        std::wstring text;
        if (TextFile::ReadText(GetConfigPath(L"state.json"), text)) {
            size_t pos = text.find(L"\"birthday\"");
            if (pos != std::wstring::npos) { pos = text.find(L'"', pos+10);
                if (pos != std::wstring::npos) { size_t end = text.find(L'"', pos+1);
                    if (end != std::wstring::npos) {
                    std::wstring bd = text.substr(pos+1,end-pos-1);
                    // 支持 "MM-DD" 和 "YYYY-MM-DD" 两种格式
                    if (bd.size() == 5)      isBirthday = (bd == today);          // "06-04"
                    else if (bd.size() == 10) isBirthday = (bd.substr(5) == today); // "2026-06-04"
                } } }
        }
    }

    if (isBirthday)  return StartupMode::Birthday;
    if (hasSpecial)  return StartupMode::SpecialDate;
    if (elapsedDays > 2) return StartupMode::LongAway;
    if (timeBug)     return StartupMode::TimeBug;
    if (!normalExit) return StartupMode::AbnormalExit;
    return StartupMode::Normal;
}

static bool IsDateKey(const std::wstring& key, int& m, int& d)
{
    if (key.rfind(L"run_",0) != 0) return false;
    size_t pos=4; m=0;
    while(pos<key.size() && key[pos]>=L'0' && key[pos]<=L'9') m=m*10+(key[pos++]-L'0');
    if(pos>=key.size()||key[pos]!=L'_'||m<1||m>12) return false;
    pos++; d=0;
    while(pos<key.size() && key[pos]>=L'0' && key[pos]<=L'9') d=d*10+(key[pos++]-L'0');
    if(d<1||d>31) return false;
    return (pos>=key.size() || key[pos]==L'_');
}

static std::vector<DialoguePanel::Entry> LoadSequence(StartupMode mode)
{
    std::map<std::wstring, std::wstring> map;
    if (!LoadIdleMap(map)) return {};

    std::vector<std::wstring> keys;
    SYSTEMTIME st; GetLocalTime(&st);

    if (mode == StartupMode::Normal) {
        static bool seeded = false; if (!seeded) { seeded = true; srand((unsigned)GetTickCount()); }
        // 硬编码三条，随机选一
        const wchar_t* pool[] = { L"run_fondle_me", L"run_happy", L"run_take_care_me" };
        int idx = rand() % 3;
        for (int i = 0; i < 3; ++i) {
            int j = (idx + i) % 3; // 从随机起点开始，取第一个存在的
            if (map.find(pool[j]) != map.end()) { keys = { pool[j] }; break; }
        }
    }
    else if (mode == StartupMode::TimeBug) {
        keys = { L"run_time_bug" };
    }
    else if (mode == StartupMode::AbnormalExit) {
        for (int i=1;i<=6;++i) keys.push_back(L"run_time_off_"+std::to_wstring(i));
    }
    else if (mode == StartupMode::LongAway) {
        keys.push_back(L"run_time_day");
        for (int i=1;i<=3;++i) keys.push_back(L"run_time_week_"+std::to_wstring(i));
        for (int i=1;i<=3;++i) keys.push_back(L"run_time_year_"+std::to_wstring(i));
    }
    else if (mode == StartupMode::Birthday) {
        for (int i=1;i<=9;++i) keys.push_back(L"run_birthday_"+std::to_wstring(i));
    }
    else if (mode == StartupMode::SpecialDate) {
        for (const auto& kv : map) {
            int m,d; if (IsDateKey(kv.first,m,d) && m==(int)st.wMonth && d==(int)st.wDay) keys.push_back(kv.first);
        }
        std::sort(keys.begin(), keys.end());
    }

    // 过滤不存在的键 + 填充
    std::vector<DialoguePanel::Entry> entries;
    for (const auto& k : keys) {
        auto it = map.find(k);
        if (it == map.end()) continue;
        std::wstring raw = it->second, text = raw, mood = L"happy", action;
        size_t comma = text.rfind(L'，'); if (comma==std::wstring::npos) comma=text.rfind(L',');
        if (comma!=std::wstring::npos) { mood=text.substr(comma+1); text=text.substr(0,comma); }
        size_t pipe = mood.find(L'|'); if (pipe!=std::wstring::npos) { action=mood.substr(pipe+1); mood=mood.substr(0,pipe); }
        entries.push_back({k, text, mood, action});
    }
    return entries;
}

// --- json 轻量解析（复用 PetActor 的 ParseJsonInt64） ---
static bool ParseJsonInt64(const std::wstring& text, const std::wstring& key, long long& out)
{
    std::wstring pattern = L"\"" + key + L"\"";
    size_t pos = text.find(pattern);
    if (pos == std::wstring::npos) return false;
    pos = text.find(L':', pos + pattern.size());
    if (pos == std::wstring::npos) return false;
    ++pos;
    while (pos < text.size() && iswspace(text[pos])) ++pos;
    if (pos >= text.size()) return false;
    bool neg = false;
    if (text[pos] == L'-') { neg = true; ++pos; }
    if (pos >= text.size() || !iswdigit(text[pos])) return false;
    long long value = 0;
    while (pos < text.size() && iswdigit(text[pos])) value = value * 10 + (text[pos++] - L'0');
    out = neg ? -value : value;
    return true;
}

// --- RunComponent ---
void RunComponent::OnInit(PetActor& actor)
{
    HWND hwnd = actor.GetWindow();

    RegisterOnPoke([]() {
        ChatRecordInteraction();
        static bool seeded=false; if(!seeded){seeded=true;srand((unsigned)GetTickCount());}
        static const wchar_t* sounds[]={L"audio\\poke_nya",L"audio\\poke_find",L"audio\\poke_ah",L"audio\\poke_poke"};
        AudioComponent::PlayAudioAuto(sounds[rand()%4]);
    });
    RegisterOnDragUpdate([]() {
        BubbleChatPanel::UpdatePosition(); InputChatPanel::UpdatePosition();
        OptionChatPanel::UpdatePosition(); MusicToolPanel::UpdatePosition();
    });
    RegisterOnRightClick([]() { UIActor::GetInstance().NotifyMouseClick(0,0); });
    RegisterOnDoubleClick([]() { Setting::ToggleOverlay(); });
    RegisterOnZhiZhi([]() { AudioComponent::PlayAudioAuto((rand()%2)?L"audio\\zhizhi1":L"audio\\zhizhi2"); });

    int sleepHour = Setting::GetInt(L"睡觉时间", -1);
    if (sleepHour < 0 || sleepHour > 23) {
        int nh = Setting::GetInt(L"晚安时间", 22);
        sleepHour = (nh + 2) % 24;
        Setting::TryApplyInlineValue(L"睡觉时间=" + std::to_wstring(sleepHour));
    }

    static bool s_done = false;
    EventSubscribe(L"tick.minute", [hwnd, sleepHour](const Event&) {
        SYSTEMTIME st; GetLocalTime(&st); int h = (int)st.wHour;
        if (!s_done && h == sleepHour) {
            s_done = true; ChatTrySleepLine(hwnd);
            CreateThread(nullptr,0,[](LPVOID c)->DWORD{Sleep(60000);PostMessageW((HWND)c,WM_CLOSE,0,0);return 0;},(LPVOID)hwnd,0,nullptr);
            return;
        }
        if (!PetCheckHourlyTrigger(h, (int)st.wMinute)) return;
        ChatTryIdleLine(hwnd);
    });

    // 启动模式检测 + DialoguePanel
    auto mode = DetectStartupMode();
    auto entries = LoadSequence(mode);
    if (!entries.empty())
        DialoguePanel::Show(entries);
}

void RunComponent::OnShutdown(PetActor&) {}
