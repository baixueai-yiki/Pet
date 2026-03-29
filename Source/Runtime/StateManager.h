#pragma once
#include <functional>
#include <string>
#include <vector>
#include "EventBus.h"

// 多维状态管理：按“状态组合”进行事件订阅。
struct StateCondition
{
    std::wstring dimension;
    std::wstring value;
};

struct SubscriptionSpec
{
    std::wstring eventName;
    std::function<void(const Event&)> handler;
};

// 设置某个状态维度的值（例如某维度从一种值切换到另一种值）。
void StateSet(const std::wstring& dimension, const std::wstring& value);

// 获取某个状态维度的值（不存在则返回空串）。
std::wstring StateGet(const std::wstring& dimension);

// 注册一个条件订阅：全部条件满足时激活订阅，任一条件不满足则失效。
void StateRegisterProfile(const std::wstring& id,
                          const std::vector<StateCondition>& conditions,
                          const std::vector<SubscriptionSpec>& subscriptions);

// 移除一个配置，并取消其激活的订阅。
void StateUnregisterProfile(const std::wstring& id);

// 清空所有配置及其订阅。
void StateClearProfiles();

// 强制重新评估所有配置（通常由状态设置函数自动触发）。
void StateRefresh();

// 开始批量状态更新，避免重复刷新。
void StateBeginUpdate();
// 结束批量状态更新，并统一刷新一次。
void StateEndUpdate();
