#include "StateManager.h"
#include "EventBus.h"
#include <unordered_map>

// 条件订阅配置
struct Profile
{
    std::wstring id;
    std::vector<StateCondition> conditions;
    std::vector<SubscriptionSpec> subscriptions;
    std::vector<int> subIds;
    bool active = false;
};

static std::unordered_map<std::wstring, std::wstring> s_state;
static std::vector<Profile> s_profiles;
static int s_updateDepth = 0;

// 判断当前状态是否满足某个配置
static bool Matches(const Profile& p)
{
    for (const auto& cond : p.conditions)
    {
        auto it = s_state.find(cond.dimension);
        if (it == s_state.end() || it->second != cond.value)
            return false;
    }
    return true;
}

// 激活配置：注册其所有事件订阅
static void Activate(Profile& p)
{
    if (p.active)
        return;

    p.subIds.clear();
    p.subIds.reserve(p.subscriptions.size());
    for (const auto& sub : p.subscriptions)
    {
        int id = EventSubscribe(sub.eventName, sub.handler);
        p.subIds.push_back(id);
    }
    p.active = true;
}

// 失活配置：取消其所有事件订阅
static void Deactivate(Profile& p)
{
    if (!p.active)
        return;

    for (size_t i = 0; i < p.subscriptions.size() && i < p.subIds.size(); ++i)
        EventUnsubscribe(p.subscriptions[i].eventName, p.subIds[i]);

    p.subIds.clear();
    p.active = false;
}

// 设置某个状态维度的值
void StateSet(const std::wstring& dimension, const std::wstring& value)
{
    s_state[dimension] = value;
    if (s_updateDepth == 0)
        StateRefresh();
}

// 获取某个状态维度的值
std::wstring StateGet(const std::wstring& dimension)
{
    auto it = s_state.find(dimension);
    return (it == s_state.end()) ? L"" : it->second;
}

// 注册条件订阅配置（满足条件时激活订阅）
void StateRegisterProfile(const std::wstring& id,
                          const std::vector<StateCondition>& conditions,
                          const std::vector<SubscriptionSpec>& subscriptions)
{
    // 如果已存在同名配置，则替换
    for (auto& p : s_profiles)
    {
        if (p.id == id)
        {
            Deactivate(p);
            p.conditions = conditions;
            p.subscriptions = subscriptions;
            p.subIds.clear();
            p.active = false;
            if (s_updateDepth == 0)
                StateRefresh();
            return;
        }
    }

    Profile p;
    p.id = id;
    p.conditions = conditions;
    p.subscriptions = subscriptions;
    s_profiles.push_back(std::move(p));
    if (s_updateDepth == 0)
        StateRefresh();
}

// 移除指定配置
void StateUnregisterProfile(const std::wstring& id)
{
    for (size_t i = 0; i < s_profiles.size(); ++i)
    {
        if (s_profiles[i].id == id)
        {
            Deactivate(s_profiles[i]);
            s_profiles.erase(s_profiles.begin() + static_cast<long long>(i));
            return;
        }
    }
}

// 清空全部配置
void StateClearProfiles()
{
    for (auto& p : s_profiles)
        Deactivate(p);
    s_profiles.clear();
}

// 重新评估所有配置并按需激活/失活
void StateRefresh()
{
    for (auto& p : s_profiles)
    {
        const bool shouldActive = Matches(p);
        if (shouldActive)
            Activate(p);
        else
            Deactivate(p);
    }
}

// 开始批量更新
void StateBeginUpdate()
{
    ++s_updateDepth;
}

// 结束批量更新并触发刷新
void StateEndUpdate()
{
    if (s_updateDepth > 0)
        --s_updateDepth;
    if (s_updateDepth == 0)
        StateRefresh();
}
