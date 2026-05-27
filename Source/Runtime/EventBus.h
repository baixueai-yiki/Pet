#pragma once

#include <functional>
#include <string>

namespace pet::runtime {

struct Event {
    std::wstring name;
    std::wstring payload;
};

using EventHandler = std::function<void(const Event&)>;

class EventBus {
public:
    EventBus() = delete;

    static int Subscribe(const std::wstring& eventName, EventHandler handler);
    static void Unsubscribe(const std::wstring& eventName, int id);
    static void Emit(const std::wstring& eventName, const std::wstring& payload = L"");
    static void Clear();
};

} // namespace pet::runtime
