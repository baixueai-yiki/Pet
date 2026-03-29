#include "EventBus.h"
#include <unordered_map>
#include <vector>

// 订阅记录：保存回调和编号
struct HandlerEntry
{
    int id;
    EventHandler handler;
};

static std::unordered_map<std::wstring, std::vector<HandlerEntry>> s_handlers;
static int s_nextId = 1;

// 注册订阅，返回订阅编号
int EventSubscribe(const std::wstring& eventName, EventHandler handler)
{
    if (!handler)
        return 0;

    const int id = s_nextId++;
    s_handlers[eventName].push_back({ id, std::move(handler) });
    return id;
}

// 取消订阅（按事件名 + 编号）
void EventUnsubscribe(const std::wstring& eventName, int id)
{
    auto it = s_handlers.find(eventName);
    if (it == s_handlers.end())
        return;

    auto& list = it->second;
    for (size_t i = 0; i < list.size(); ++i)
    {
        if (list[i].id == id)
        {
            list.erase(list.begin() + static_cast<long long>(i));
            break;
        }
    }

    if (list.empty())
        s_handlers.erase(it);
}

// 触发事件，按事件名分发给订阅者
void EventEmit(const std::wstring& eventName, const std::wstring& payload)
{
    auto it = s_handlers.find(eventName);
    if (it == s_handlers.end())
        return;

    const Event evt{ eventName, payload };
    // 复制订阅列表，避免回调中取消订阅造成迭代失效。
    const auto list = it->second;
    for (const auto& entry : list)
    {
        if (entry.handler)
            entry.handler(evt);
    }
}

// 清空所有订阅
void EventClear()
{
    s_handlers.clear();
    s_nextId = 1;
}
