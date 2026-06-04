#include "PetActor.h"
#include "PetComponents/AudioComponent.h"
#include "PetComponents/ChatComponent.h"
#include "PetComponents/DiaryComponent.h"
#include "PetComponents/PetRenderComponent.h"
#include "PetComponents/RunComponent.h"
#include "../UI/UIActor.h"
#include "../UI/UIPanels/ToolPanel/SettingToolPanel.h"
#include "../../Core/Path.h"
#include "../../Core/TextFile.h"
#include "../../Runtime/Scheduler.h"
#include "../../Runtime/EventBus.h"
#include "../../Engine/Input/InputDispatcher.h"
#include <cstdio>
#include <cstdlib>
#include <cwctype>
#include <map>
#include <fstream>
#include <set>
#include <memory>

static bool ParseJsonInt64(const std::wstring& text, const std::wstring& key, long long& out);
static void GenerateDailyTriggers();

static PetMindState s_mindState = { 0, 8, 8 };
static int s_monitorState = 0;
static bool s_mindStateLoaded = false;

// 生理期全局状态
PetCycleState g_cycle = { 0, 1 };

// 获取当前日期字符串 "YYYY-MM-DD"
static std::wstring GetCurrentDateStr()
{
    SYSTEMTIME st; GetLocalTime(&st);
    return std::to_wstring(st.wYear) + L"-" +
        (st.wMonth < 10 ? L"0" : L"") + std::to_wstring(st.wMonth) + L"-" +
        (st.wDay < 10 ? L"0" : L"") + std::to_wstring(st.wDay);
}

// 解析 "YYYY-MM-DD" 为年月日，成功返回 true
static bool ParseDateStr(const std::wstring& s, int& y, int& m, int& d)
{
    if (s.size() != 10 || s[4] != L'-' || s[7] != L'-')
        return false;
    y = _wtoi(s.substr(0, 4).c_str());
    m = _wtoi(s.substr(5, 2).c_str());
    d = _wtoi(s.substr(8, 2).c_str());
    return (y >= 2020 && m >= 1 && m <= 12 && d >= 1 && d <= 31);
}

// 计算两个日期的相差天数（date2 - date1）
static int DaysBetween(const std::wstring& d1, const std::wstring& d2)
{
    int y1, m1, d1v, y2, m2, d2v;
    if (!ParseDateStr(d1, y1, m1, d1v) || !ParseDateStr(d2, y2, m2, d2v))
        return 0;
    SYSTEMTIME st1 = {}, st2 = {};
    st1.wYear = y1; st1.wMonth = m1; st1.wDay = d1v;
    st2.wYear = y2; st2.wMonth = m2; st2.wDay = d2v;
    FILETIME ft1, ft2;
    SystemTimeToFileTime(&st1, &ft1);
    SystemTimeToFileTime(&st2, &ft2);
    ULARGE_INTEGER u1 = { ft1.dwLowDateTime, ft1.dwHighDateTime };
    ULARGE_INTEGER u2 = { ft2.dwLowDateTime, ft2.dwHighDateTime };
    return (int)((u2.QuadPart - u1.QuadPart) / 864000000000LL);
}

// 从 state.json 加载上次运行日期和周期状态
static void LoadCycleState()
{
    std::wstring text;
    if (!TextFile::ReadText(GetConfigPath(L"state.json"), text))
        return;

    long long val = 0;
    if (ParseJsonInt64(text, L"cycle_phase", val))
        g_cycle.phase = (int)val;
    if (ParseJsonInt64(text, L"cycle_day", val))
        g_cycle.day = (int)val;

    // 读取上次运行日期，计算经过天数并推进周期
    std::wstring lastDate;
    // 简单解析 JSON 字符串值
    size_t pos = text.find(L"\"last_run_date\"");
    if (pos != std::wstring::npos)
    {
        pos = text.find(L'"', pos + 15);
        if (pos != std::wstring::npos)
        {
            size_t end = text.find(L'"', pos + 1);
            if (end != std::wstring::npos)
                lastDate = text.substr(pos + 1, end - pos - 1);
        }
    }

    std::wstring today = GetCurrentDateStr();
    if (!lastDate.empty())
    {
        int elapsed = DaysBetween(lastDate, today);
        if (elapsed > 0)
            PetCycleAdvance(elapsed);
    }
}

void PetCycleAdvance(int days)
{
    while (days > 0)
    {
        int phaseLen = (g_cycle.phase == 0) ? kRisingDays :
                       (g_cycle.phase == 1) ? kFallingDays : kPeriodDays;
        int remaining = phaseLen - g_cycle.day + 1;
        if (days < remaining)
        {
            g_cycle.day += days;
            days = 0;
        }
        else
        {
            days -= remaining;
            g_cycle.phase = (g_cycle.phase + 1) % 3;
            g_cycle.day = 1;
        }
    }
}

void PetCycleAdvanceDay(int delta)
{
    if (delta == 0) return;

    int phaseLen = (g_cycle.phase == 0) ? kRisingDays :
                   (g_cycle.phase == 1) ? kFallingDays : kPeriodDays;

    g_cycle.day += delta;

    if (g_cycle.day > phaseLen)
    {
        // 前进到下一阶段
        g_cycle.day = 1;
        g_cycle.phase = (g_cycle.phase + 1) % 3;
    }
    else if (g_cycle.day < 1)
    {
        // 后退到上一阶段
        g_cycle.phase = (g_cycle.phase + 2) % 3; // -1 mod 3
        int prevLen = (g_cycle.phase == 0) ? kRisingDays :
                      (g_cycle.phase == 1) ? kFallingDays : kPeriodDays;
        g_cycle.day = prevLen;
    }
}

int PetGetCyclePhase()  { return g_cycle.phase; }
int PetGetCycleDay()    { return g_cycle.day; }

const wchar_t* PetGetCyclePhaseName()
{
    static const wchar_t* names[] = { L"上升期", L"下降期", L"经期" };
    return names[g_cycle.phase];
}

void PetCycleInit()
{
    LoadCycleState();

    // 根据周期计算兴奋度
    if (g_cycle.phase == 0)       // 上升期: day = arousal
        s_mindState.arousal = g_cycle.day;
    else if (g_cycle.phase == 1)  // 下降期: 12 - day = arousal
        s_mindState.arousal = 12 - g_cycle.day;
    else                          // 经期: 固定 1
        s_mindState.arousal = 1;

    // 生成每日触发点 / 检查触发
    GenerateDailyTriggers();
}

// ---- 每小时随机触发系统 ----

static int  s_triggerMinutes[24] = {};
static unsigned int s_triggeredMask = 0;      // bitmask: bit N = hour N triggered
static std::wstring s_lastTriggerDate;
static DWORD s_triggerStartTime = 0;           // 启动后 5 分钟才允许触发

// 日期种子 → 为 24 小时各生成一个 0-59 的随机分钟
static void GenerateDailyTriggers()
{
    std::wstring today = GetCurrentDateStr();

    // 尝试从 state.json 恢复今天的触发状态
    std::wstring savedDate;
    unsigned int savedMask = 0;
    {
        std::wstring text;
        if (TextFile::ReadText(GetConfigPath(L"state.json"), text))
        {
            long long val = 0;
            if (ParseJsonInt64(text, L"triggered_mask", val))
                savedMask = (unsigned int)val;
            size_t pos = text.find(L"\"last_trigger_date\"");
            if (pos != std::wstring::npos)
            {
                pos = text.find(L'"', pos + 19);
                if (pos != std::wstring::npos)
                {
                    size_t end = text.find(L'"', pos + 1);
                    if (end != std::wstring::npos)
                        savedDate = text.substr(pos + 1, end - pos - 1);
                }
            }
        }
    }

    // 每次启动都重置触发状态，5 分钟后才生效
    s_lastTriggerDate = today;
    s_triggeredMask = 0;
    s_triggerStartTime = GetTickCount() + 300000; // 5 分钟

    int seed = 0;
    for (wchar_t c : today)
        if (c >= L'0' && c <= L'9')
            seed = seed * 10 + (c - L'0');

    srand(seed);
    for (int h = 0; h < 24; ++h)
        s_triggerMinutes[h] = rand() % 60;
}

// 检查当前时间是否命中触发点；命中则返回 true 并标记已触发
bool PetCheckHourlyTrigger(int hour, int minute)
{
    if (s_triggerStartTime != 0 && GetTickCount() < s_triggerStartTime)
        return false; // 启动后 5 分钟内不触发
    if (hour < 0 || hour > 23)
        return false;
    if (s_triggeredMask & (1u << hour))
        return false; // 本小时已触发
    if (minute != s_triggerMinutes[hour])
        return false;

    s_triggeredMask |= (1u << hour);
    return true;
}

int PetGetArousal()
{
    return s_mindState.arousal;
}


void PetFirstLine(HWND hwnd)
{
    // 根据当前时段决定第一句话
    int hour = GetLocalHour();
    std::wstring key;

    if (IsSleepHour(hour))
        key = L"sleep";
    else
    {
        const int morning   = Setting::GetInt(L"早安时间", 7);
        const int lunch     = Setting::GetInt(L"午餐时间", 12);
        const int afternoon = Setting::GetInt(L"下午时间", 15);
        const int dinner    = Setting::GetInt(L"晚餐时间", 18);
        const int night     = Setting::GetInt(L"晚安时间", 22);

        if (hour == morning)        key = L"morning";
        else if (hour == lunch)     key = L"lunch";
        else if (hour == afternoon) key = L"afternoon";
        else if (hour == dinner)    key = L"dinner";
        else if (hour == night)     key = L"night";
        else                        key = L"idle";
    }

    std::wstring text, keyUsed;
    if (GetIdleTextByKey(key, text, keyUsed))
        ChatTalk(hwnd, text.c_str());
    else
        ChatTalk(hwnd, L"我回来了~");
    ChatRecordInteraction();
}

unsigned int PetGetTriggeredMask()
{
    return s_triggeredMask;
}

const wchar_t* PetGetLastTriggerDate()
{
    return s_lastTriggerDate.c_str();
}
static std::set<std::wstring> s_gameActiveKeys;
static std::set<std::wstring> s_gameActiveGameKeys;
static std::set<std::wstring> s_gameActiveWorkKeys;

// 从 state.json 加载上次保存的位置
static void LoadPetPosition()
{
    std::wstring text;
    if (!TextFile::ReadText(GetConfigPath(L"state.json"), text))
        return;

    long long val = 0;
    if (ParseJsonInt64(text, L"pet_x", val))
        g_pet.x = (int)val;
    if (ParseJsonInt64(text, L"pet_y", val))
        g_pet.y = (int)val;
}

// 初始化宠物位置与拖拽状态
void PetInit()
{
    g_pet.x = 120;
    g_pet.y = 120;
    g_pet.w = 0;
    g_pet.h = 0;
    g_pet.isDragging = false;
    g_pet.dragOffsetX = 0;
    g_pet.dragOffsetY = 0;

    // 尝试恢复上次位置
    LoadPetPosition();
    // 初始化生理期周期
    PetCycleInit();
}

// 占位：未来可在此添加宠物自动行为/动画逻辑
void PetBehaviorTick()
{
    // 占位符，未来可在此添加宠物自动行为、状态切换等逻辑
}

// 返回心智状态的全局引用，用于闲聊/状态订阅共享
PetMindState& PetMindStateRef()
{
    return s_mindState;
}

// 监控状态的引用（0=无、1=游戏、2=工作）
int& PetMonitorStateRef()
{
    return s_monitorState;
}

// 统一的 state.json 存储路径
static std::wstring GetMindStatePath()
{
    return GetConfigPath(L"state.json");
}

// 简单 JSON 解析，抽取数值字段
static bool ParseJsonInt64(const std::wstring& text, const std::wstring& key, long long& out)
{
    std::wstring pattern = L"\"" + key + L"\"";
    size_t pos = text.find(pattern);
    if (pos == std::wstring::npos)
        return false;
    pos = text.find(L':', pos + pattern.size());
    if (pos == std::wstring::npos)
        return false;
    ++pos;
    while (pos < text.size() && iswspace(text[pos]))
        ++pos;
    if (pos >= text.size())
        return false;
    bool neg = false;
    if (text[pos] == L'-')
    {
        neg = true;
        ++pos;
    }
    if (pos >= text.size() || !iswdigit(text[pos]))
        return false;
    long long value = 0;
    while (pos < text.size() && iswdigit(text[pos]))
    {
        value = value * 10 + (text[pos] - L'0');
        ++pos;
    }
    out = neg ? -value : value;
    return true;
}

// 限制 valence/arousal 在有效范围
static void ClampMindState(PetMindState& state)
{
    if (state.valence < 1)
        state.valence = 1;
    if (state.valence > 15)
        state.valence = 15;
    if (state.arousal < 1)
        state.arousal = 1;
    if (state.arousal > 15)
        state.arousal = 15;
    if (state.lastInteraction < 0)
        state.lastInteraction = 0;
}

// 尝试从配置加载心智状态，成功返回 true
bool PetTryLoadMindState(PetMindState& out)
{
    std::wstring text;
    if (!TextFile::ReadText(GetMindStatePath(), text))
        return false;

    bool any = false;
    PetMindState tmp = out;
    long long value = 0;
    if (ParseJsonInt64(text, L"last_interaction_time", value))
    {
        tmp.lastInteraction = value;
        any = true;
    }
    if (ParseJsonInt64(text, L"valence", value))
    {
        tmp.valence = static_cast<int>(value);
        any = true;
    }
    if (ParseJsonInt64(text, L"arousal", value))
    {
        tmp.arousal = static_cast<int>(value);
        any = true;
    }
    if (!any)
        return false;

    ClampMindState(tmp);
    if (tmp.lastInteraction == 0)
        tmp.lastInteraction = GetUnixTimeSeconds();

    out = tmp;
    return true;
}

// 单例式确保心智状态只初始化一次
void PetEnsureMindStateLoaded()
{
    if (s_mindStateLoaded)
        return;

    PetMindState loaded = s_mindState;
    if (!PetTryLoadMindState(loaded))
    {
        loaded.lastInteraction = GetUnixTimeSeconds();
        ClampMindState(loaded);
    }
    s_mindState = loaded;
    s_mindStateLoaded = true;
}

void PetApplyMindStateOverrides()
{
    PetEnsureMindStateLoaded();
    PetMindState& state = PetMindStateRef();
    PetMindState fileState = state;
    if (PetTryLoadMindState(fileState))
        state = fileState;
}

// 去除行两端空白，供配置解析使用
static std::wstring Trim(const std::wstring& text)
{
    const wchar_t* ws = L" \t\r\n";
    size_t start = text.find_first_not_of(ws);
    size_t end = text.find_last_not_of(ws);
    if (start == std::wstring::npos)
        return L"";
    return text.substr(start, end - start + 1);
}

// 复制并转小写
static std::wstring ToLowerCopy(std::wstring s)
{
    for (auto& ch : s)
        ch = static_cast<wchar_t>(towlower(ch));
    return s;
}

// 从 monitor_* 文本读取关键词与回复映射
bool PetLoadMonitorMap(const std::wstring& file, std::map<std::wstring, std::wstring>& out)
{
    std::vector<std::wstring> lines;
    if (!TextFile::ReadLines(GetAssetPath(L"chat\\" + file), lines))
        return false;

    out.clear();
    for (const auto& lineRaw : lines)
    {
        std::wstring line = Trim(lineRaw);
        if (line.empty() || line[0] == L'#')
            continue;
        size_t sep = line.find(L'=');
        if (sep == std::wstring::npos)
            sep = line.find(L':');
        if (sep == std::wstring::npos)
            continue;
        std::wstring key = Trim(line.substr(0, sep));
        if (!key.empty() && key.front() == L'\ufeff')
            key.erase(0, 1);
        std::wstring value = Trim(line.substr(sep + 1));
        if (!key.empty() && !value.empty())
            out[key] = value;
    }
    return !out.empty();
}

// 判断当前活动进程标题/名字是否包含任意关键词，用于 monitor 识别
static bool MatchAnyKeyword(const std::map<std::wstring, std::wstring>& map,
                            const std::vector<PetAppEntry>& entries)
{
    for (const auto& kv : map)
    {
        const std::wstring keyLower = ToLowerCopy(kv.first);
        for (const auto& entry : entries)
        {
            const std::wstring nameLower = ToLowerCopy(entry.processName);
            const std::wstring titleLower = ToLowerCopy(entry.title);
            if ((!nameLower.empty() && nameLower.find(keyLower) != std::wstring::npos) ||
                (!titleLower.empty() && titleLower.find(keyLower) != std::wstring::npos))
                return true;
        }
    }
    return false;
}

// 更新 monitorState（0/1/2）以便 ChatTick 了解当前用户在干什么
void PetUpdateMonitorStateFromEntries(const std::vector<PetAppEntry>& entries)
{
    if (entries.empty())
    {
        s_monitorState = 0;
        return;
    }

    std::map<std::wstring, std::wstring> gameMap;
    std::map<std::wstring, std::wstring> workMap;
    PetLoadMonitorMap(L"monitor_game.txt", gameMap);
    PetLoadMonitorMap(L"monitor_work.txt", workMap);

    const bool hasWork = MatchAnyKeyword(workMap, entries);
    const bool hasGame = MatchAnyKeyword(gameMap, entries);
    if (hasWork)
        s_monitorState = 2;
    else if (hasGame)
        s_monitorState = 1;
    else
        s_monitorState = 0;
}

// 扫描 monitor_life/game/work 文本，找到未曾触发过的关键词并返回提示
bool PetCheckMonitorKeywords(const std::vector<PetAppEntry>& entries,
                             std::wstring& outKey,
                             std::wstring& outText,
                             std::wstring& outCategory)
{
    outKey.clear();
    outText.clear();
    outCategory.clear();

    std::map<std::wstring, std::wstring> lifeMap;
    PetLoadMonitorMap(L"monitor_life.txt", lifeMap);
    std::map<std::wstring, std::wstring> gameMap;
    std::map<std::wstring, std::wstring> workMap;
    PetLoadMonitorMap(L"monitor_game.txt", gameMap);
    PetLoadMonitorMap(L"monitor_work.txt", workMap);

    PetUpdateMonitorStateFromEntries(entries);

    if (entries.empty())
    {
        s_gameActiveKeys.clear();
        s_gameActiveGameKeys.clear();
        s_gameActiveWorkKeys.clear();
        return false;
    }

    std::set<std::wstring> current;
    std::set<std::wstring> currentGame;
    std::set<std::wstring> currentWork;
    const std::wstring* reply = nullptr;
    std::wstring matchedKey;
    std::wstring matchedCategory;
    auto ScanMap = [&](const std::map<std::wstring, std::wstring>& map,
                       std::set<std::wstring>& currentOut,
                       std::set<std::wstring>& activeSet,
                       const std::wstring& category,
                       const std::wstring** outReply,
                       std::wstring* outKeyLocal,
                       std::wstring* outCategoryLocal)
    {
        for (const auto& kv : map)
        {
            const std::wstring keyLower = ToLowerCopy(kv.first);
            bool found = false;
            for (const auto& entry : entries)
            {
                const std::wstring nameLower = ToLowerCopy(entry.processName);
                const std::wstring titleLower = ToLowerCopy(entry.title);
                if ((!nameLower.empty() && nameLower.find(keyLower) != std::wstring::npos) ||
                    (!titleLower.empty() && titleLower.find(keyLower) != std::wstring::npos))
                {
                    found = true;
                    break;
                }
            }
            if (found)
            {
                currentOut.insert(kv.first);
                if (!*outReply && activeSet.find(kv.first) == activeSet.end())
                {
                    *outReply = &kv.second;
                    *outKeyLocal = kv.first;
                    *outCategoryLocal = category;
                }
            }
        }
    };

    if (!workMap.empty())
        ScanMap(workMap, currentWork, s_gameActiveWorkKeys, L"work", &reply, &matchedKey, &matchedCategory);
    if (!reply && !gameMap.empty())
        ScanMap(gameMap, currentGame, s_gameActiveGameKeys, L"game", &reply, &matchedKey, &matchedCategory);
    if (!reply && !lifeMap.empty())
        ScanMap(lifeMap, current, s_gameActiveKeys, L"life", &reply, &matchedKey, &matchedCategory);

    s_gameActiveKeys = std::move(current);
    s_gameActiveGameKeys = std::move(currentGame);
    s_gameActiveWorkKeys = std::move(currentWork);

    if (reply)
    {
        outKey = matchedKey;
        outText = *reply;
        outCategory = matchedCategory;
        return true;
    }
    return false;
}

// 在主窗口的 WM_CREATE 时由 Window 调用，启动 Actor+各组件
void PetInitSystems(HWND hwnd, unsigned int idleCheckMs)
{
    DiaryInit();
    OnProgramStart();

    UIActor::InitializeSingleton(hwnd);
    PetActor::Get().Initialize(hwnd, idleCheckMs);
}
// 程序退出或会话结束时调用，用于逐个 Shutdown 组件
void PetOnExit()
{
    OnProgramExit();
    UIActor::ShutdownSingleton();
    PetActor::Get().Shutdown();
}

namespace
{
    // 负责 初始化聊天系统，提供闲聊/输入/按钮窗口
    class ChatComponent : public PetComponent
    {
    public:
        void OnInit(PetActor& actor) override
        {
            if (HWND hwnd = actor.GetWindow())
                ChatSystem::Get().Init(hwnd);
        }
    };

    // 注册定时任务 tick.minute 用于触发 ChatSystem 的检查
    class SchedulerComponent : public PetComponent
    {
    public:
        void OnInit(PetActor& actor) override
        {
            SchedulerClear();
            ScheduleEveryMs(L"tick.minute", actor.GetIdleCheckMs());
        }
    };
}

PetActor& PetActor::Get()
{
    static PetActor instance;
    return instance;
}

HWND PetActor::GetWindow() const
{
    return m_mainHwnd;
}

unsigned int PetActor::GetIdleCheckMs() const
{
    return m_idleCheckMs;
}

// 将组件加入 Actor 的列表
void PetActor::AddComponent(std::unique_ptr<PetComponent> component)
{
    if (component)
        m_components.push_back(std::move(component));
}

// 注册默认组件（聊天/音频/日记/调度器），仅执行一次
void PetActor::EnsureDefaultComponents()
{
    if (m_componentsRegistered)
        return;

    AddComponent(std::make_unique<ChatComponent>());
    AddComponent(std::make_unique<AudioComponent>());
    AddComponent(std::make_unique<PetRenderComponent>());
    AddComponent(std::make_unique<RunComponent>());
    AddComponent(std::make_unique<SchedulerComponent>());
    m_componentsRegistered = true;
}

// 初始化 Actor，并按注册顺序启动各组件
void PetActor::Initialize(HWND hwnd, unsigned int idleCheckMs)
{
    if (m_initialized)
        return;

    m_mainHwnd = hwnd;
    m_idleCheckMs = idleCheckMs;
    EnsureDefaultComponents();

    for (auto& component : m_components)
        component->OnInit(*this);

    m_initialized = true;
}

// 程序退出时反向关闭组件，释放资源
void PetActor::Shutdown()
{
    if (!m_initialized)
        return;

    for (auto& component : m_components)
        component->OnShutdown(*this);

    m_initialized = false;
}


