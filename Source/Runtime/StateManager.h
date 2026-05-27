#pragma once

#include "Runtime/EventBus.h"

#include <string>
#include <vector>

namespace pet::runtime {

struct StateCondition {
    std::wstring dimension;
    std::wstring value;
};

struct SubscriptionSpec {
    std::wstring eventName;
    EventHandler handler;
};

class StateManager {
public:
    StateManager() = delete;

    static void Set(const std::wstring& dimension, const std::wstring& value);
    static std::wstring Get(const std::wstring& dimension);

    static void RegisterProfile(const std::wstring& id,
                                const std::vector<StateCondition>& conditions,
                                const std::vector<SubscriptionSpec>& subscriptions);
    static void UnregisterProfile(const std::wstring& id);

    static void ClearProfiles();
    static void Refresh();

    static void BeginUpdate();
    static void EndUpdate();
};

} // namespace pet::runtime
