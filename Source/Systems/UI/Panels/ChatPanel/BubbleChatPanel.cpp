#include "Systems/UI/Panels/ChatPanel/BubbleChatPanel.h"

#include "Runtime/EventBus.h"

namespace pet::systems::ui::panels::chat {
namespace {
std::wstring g_lastMessage;
}

void BubbleChatPanel::ShowMessage(const std::wstring& text) {
    g_lastMessage = text;
    runtime::EventBus::Emit(L"ui.panel.chat.bubble.show", text);
}

const std::wstring& BubbleChatPanel::LastMessage() {
    return g_lastMessage;
}

} // namespace pet::systems::ui::panels::chat
