#pragma once
#include "../../Core/PetState.h"
#include <windows.h>
#include <string>
#include <vector>
#include <map>
#include <memory>

// 桌宠的内在状态，跨系统共享（闲聊频率等）
struct PetMindState
{
    long long lastInteraction;
    int valence;
    int arousal;
};

// 生理期周期状态
struct PetCycleState
{
    int phase = 0;    // 0=上升期, 1=下降期, 2=经期
    int day = 1;      // 当前阶段的第几天（1-based）
};
extern PetCycleState g_cycle;

// 周期阶段长度
constexpr int kRisingDays = 12;
constexpr int kFallingDays = 12;
constexpr int kPeriodDays = 4;
constexpr int kCycleTotalDays = kRisingDays + kFallingDays + kPeriodDays; // 28

// 生理期接口
void PetCycleInit();                          // 启动时初始化/推进周期 + 计算兴奋度
void PetCycleAdvance(int days);                // 推进 N 天（正向）
void PetCycleAdvanceDay(int delta);            // 调整 ±1 天（睡眠触发用）
int  PetGetCyclePhase();                       // 0=上升期, 1=下降期, 2=经期
int  PetGetCycleDay();                         // 当前阶段第几天 (1-based)
const wchar_t* PetGetCyclePhaseName();         // 中文名称
int  PetGetArousal();                          // 当前兴奋度 (1-12)
void PetFirstLine(HWND hwnd);                   // 启动时说的第一句话


// 每小时随机触发
bool PetCheckHourlyTrigger(int hour, int minute); // 检查是否命中触发点
unsigned int PetGetTriggeredMask();                // 已触发小时 bitmask
const wchar_t* PetGetLastTriggerDate();            // 上次触发日期

// 获取可变的内在状态引用
PetMindState& PetMindStateRef();
// 监控状态（0=无、1=游戏、2=工作）
int& PetMonitorStateRef();
// 确保内在状态已从配置加载
void PetEnsureMindStateLoaded();
// 读取配置文件中的内在状态（不会修改全局）
bool PetTryLoadMindState(PetMindState& out);

// 用于监控匹配的轻量应用信息
struct PetAppEntry
{
    std::wstring title;
    std::wstring processName;
};

// 读取监控关键词映射（monitor_*.txt）
bool PetLoadMonitorMap(const std::wstring& file, std::map<std::wstring, std::wstring>& out);
// 根据当前可见应用更新 monitor 状态
void PetUpdateMonitorStateFromEntries(const std::vector<PetAppEntry>& entries);
// 检测关键词命中，返回要说的话与分类（life/game/work）
bool PetCheckMonitorKeywords(const std::vector<PetAppEntry>& entries,
                             std::wstring& outKey,
                             std::wstring& outText,
                             std::wstring& outCategory);

// 允许外部在运行时通过修改 state.json 调整情绪、互动等维度
void PetApplyMindStateOverrides();

// 宠物当前的情绪枚举
enum class PetMood
{
    Idle,
    Curious,
    Sleepy,
};

// 把情绪枚举转换为可显示的中文字符串
inline const wchar_t* PetMoodToString(PetMood mood)
{
    switch (mood)
    {
    case PetMood::Curious: return L"好奇";
    case PetMood::Sleepy: return L"想睡觉";
    default: return L"安静";
    }
}

// 初始化宠物位置与交互状态
void PetInit();
// 后续可以在此驱动宠物行为（如动画或状态切换）
void PetBehaviorTick();

// 统一初始化各系统（类似 Actor 生命周期入口）
void PetInitSystems(HWND hwnd, unsigned int idleCheckMs);
// 程序退出时统一收尾
void PetOnExit();

class PetActor;

class PetComponent
{
public:
    virtual ~PetComponent() = default;
    virtual void OnInit(PetActor& actor) {}
    virtual void OnShutdown(PetActor& actor) {}
};

class PetActor
{
public:
    static PetActor& Get();

    HWND GetWindow() const;
    unsigned int GetIdleCheckMs() const;

    void Initialize(HWND hwnd, unsigned int idleCheckMs);
    void Shutdown();

private:
    PetActor() = default;
    PetActor(const PetActor&) = delete;
    PetActor& operator=(const PetActor&) = delete;

    void AddComponent(std::unique_ptr<PetComponent> component);
    void EnsureDefaultComponents();

    HWND m_mainHwnd = nullptr;
    unsigned int m_idleCheckMs = 0;
    bool m_initialized = false;
    bool m_componentsRegistered = false;
    std::vector<std::unique_ptr<PetComponent>> m_components;
};
