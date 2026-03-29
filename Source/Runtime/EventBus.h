#pragma once
#include <functional>
#include <string>

// Lightweight in-process event bus (single-threaded use assumed).
struct Event
{
    std::wstring name;
    std::wstring payload;
};

using EventHandler = std::function<void(const Event&)>;

// Subscribe to an event name. Returns a subscription id for removal.
int EventSubscribe(const std::wstring& eventName, EventHandler handler);

// Unsubscribe by event name and id (safe to call even if not found).
void EventUnsubscribe(const std::wstring& eventName, int id);

// Emit event to all current subscribers of the name.
void EventEmit(const std::wstring& eventName, const std::wstring& payload = L"");

// Remove all subscriptions (useful for shutdown).
void EventClear();
