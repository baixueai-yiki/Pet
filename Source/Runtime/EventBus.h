#pragma once
#include <functional>
#include <string>

// 轻量级进程内事件总线（假定单线程使用）。
struct Event
{
    std::wstring name;
    std::wstring payload;
};

using EventHandler = std::function<void(const Event&)>;

// 订阅事件名，返回订阅编号以便取消。
int EventSubscribe(const std::wstring& eventName, EventHandler handler);

// 按事件名 + 编号取消订阅（不存在也安全）。
void EventUnsubscribe(const std::wstring& eventName, int id);

// 触发事件，广播给当前订阅者。
void EventEmit(const std::wstring& eventName, const std::wstring& payload = L"");

// 清空所有订阅（用于退出清理）。
void EventClear();
