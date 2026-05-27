#include "Runtime/StateManager.h"

#include <unordered_map>
#include <utility>

namespace pet::runtime {
namespace {

struct Profile {
    std::wstring id;
    std::vector<StateCondition> conditions;
    std::vector<SubscriptionSpec> subscriptions;
    std::vector<int> subIds;
    bool active = false;
};

std::unordered_map<std::wstring, std::wstring> g_state;
std::vector<Profile> g_profiles;
int g_updateDepth = 0;

bool Matches(const Profile& p) {
    for (const auto& cond : p.conditions) {
        auto it = g_state.find(cond.dimension);
        if (it == g_state.end() || it->second != cond.value) {
            return false;
        }
    }
    return true;
}

void Activate(Profile& p) {
    if (p.active) {
        return;
    }

    p.subIds.clear();
    p.subIds.reserve(p.subscriptions.size());
    for (const auto& sub : p.subscriptions) {
        const int id = EventBus::Subscribe(sub.eventName, sub.handler);
        p.subIds.push_back(id);
    }
    p.active = true;
}

void Deactivate(Profile& p) {
    if (!p.active) {
        return;
    }

    for (size_t i = 0; i < p.subscriptions.size() && i < p.subIds.size(); ++i) {
        EventBus::Unsubscribe(p.subscriptions[i].eventName, p.subIds[i]);
    }

    p.subIds.clear();
    p.active = false;
}

} // namespace

void StateManager::Set(const std::wstring& dimension, const std::wstring& value) {
    g_state[dimension] = value;
    if (g_updateDepth == 0) {
        Refresh();
    }
}

std::wstring StateManager::Get(const std::wstring& dimension) {
    const auto it = g_state.find(dimension);
    return (it == g_state.end()) ? L"" : it->second;
}

void StateManager::RegisterProfile(const std::wstring& id,
                                   const std::vector<StateCondition>& conditions,
                                   const std::vector<SubscriptionSpec>& subscriptions) {
    for (auto& p : g_profiles) {
        if (p.id == id) {
            Deactivate(p);
            p.conditions = conditions;
            p.subscriptions = subscriptions;
            p.subIds.clear();
            p.active = false;
            if (g_updateDepth == 0) {
                Refresh();
            }
            return;
        }
    }

    Profile p;
    p.id = id;
    p.conditions = conditions;
    p.subscriptions = subscriptions;
    g_profiles.push_back(std::move(p));

    if (g_updateDepth == 0) {
        Refresh();
    }
}

void StateManager::UnregisterProfile(const std::wstring& id) {
    for (size_t i = 0; i < g_profiles.size(); ++i) {
        if (g_profiles[i].id == id) {
            Deactivate(g_profiles[i]);
            g_profiles.erase(g_profiles.begin() + static_cast<long long>(i));
            return;
        }
    }
}

void StateManager::ClearProfiles() {
    for (auto& p : g_profiles) {
        Deactivate(p);
    }
    g_profiles.clear();
}

void StateManager::Refresh() {
    for (auto& p : g_profiles) {
        if (Matches(p)) {
            Activate(p);
        } else {
            Deactivate(p);
        }
    }
}

void StateManager::BeginUpdate() {
    ++g_updateDepth;
}

void StateManager::EndUpdate() {
    if (g_updateDepth > 0) {
        --g_updateDepth;
    }
    if (g_updateDepth == 0) {
        Refresh();
    }
}

} // namespace pet::runtime
