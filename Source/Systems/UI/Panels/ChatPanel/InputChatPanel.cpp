#include "Systems/UI/Panels/ChatPanel/InputChatPanel.h"

#include "Runtime/EventBus.h"

namespace pet::systems::ui::panels::chat {
namespace {
bool g_open = false;
std::wstring g_input;
}

void InputChatPanel::Open() {
    g_open = true;
    runtime::EventBus::Emit(L"ui.panel.chat.input.open");
}

void InputChatPanel::Close() {
    g_open = false;
    runtime::EventBus::Emit(L"ui.panel.chat.input.close");
}

bool InputChatPanel::IsOpen() {
    return g_open;
}

void InputChatPanel::SetInputText(const std::wstring& text) {
    g_input = text;
    runtime::EventBus::Emit(L"ui.panel.chat.input.change", text);
}

const std::wstring& InputChatPanel::GetInputText() {
    return g_input;
}

void InputChatPanel::Submit() {
    runtime::EventBus::Emit(L"ui.panel.chat.input.submit", g_input);
}

} // namespace pet::systems::ui::panels::chat
