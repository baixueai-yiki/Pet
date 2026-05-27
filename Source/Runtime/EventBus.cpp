#include "Runtime/EventBus.h"

#include <unordered_map>
#include <utility>
#include <vector>

namespace pet::runtime {
namespace {

struct HandlerEntry {
    int id = 0;
    EventHandler handler;
};

std::unordered_map<std::wstring, std::vector<HandlerEntry>> g_handlers;
int g_nextId = 1;

} // namespace

int EventBus::Subscribe(const std::wstring& eventName, EventHandler handler) {
    if (!handler) {
        return 0;
    }

    const int id = g_nextId++;
    g_handlers[eventName].push_back({id, std::move(handler)});
    return id;
}

void EventBus::Unsubscribe(const std::wstring& eventName, int id) {
    auto it = g_handlers.find(eventName);
    if (it == g_handlers.end()) {
        return;
    }

    auto& list = it->second;
    for (size_t i = 0; i < list.size(); ++i) {
        if (list[i].id == id) {
            list.erase(list.begin() + static_cast<long long>(i));
            break;
        }
    }

    if (list.empty()) {
        g_handlers.erase(it);
    }
}

void EventBus::Emit(const std::wstring& eventName, const std::wstring& payload) {
    auto it = g_handlers.find(eventName);
    if (it == g_handlers.end()) {
        return;
    }

    const Event evt{eventName, payload};
    const auto listCopy = it->second;
    for (const auto& entry : listCopy) {
        if (entry.handler) {
            entry.handler(evt);
        }
    }
}

void EventBus::Clear() {
    g_handlers.clear();
    g_nextId = 1;
}

} // namespace pet::runtime
