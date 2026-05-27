#include "Systems/UI/Panels/ChatPanel/OptionChatPanel.h"

#include "Runtime/EventBus.h"

namespace pet::systems::ui::panels::chat {
namespace {
bool g_open = false;
std::wstring g_opt1;
std::wstring g_opt2;
}

void OptionChatPanel::Open(const std::wstring& key1, const std::wstring& key2) {
    g_open = true;
    g_opt1 = key1;
    g_opt2 = key2;
    runtime::EventBus::Emit(L"ui.panel.chat.option.open");
}

void OptionChatPanel::Close() {
    g_open = false;
    runtime::EventBus::Emit(L"ui.panel.chat.option.close");
}

bool OptionChatPanel::IsOpen() {
    return g_open;
}

const std::wstring& OptionChatPanel::GetOption1() {
    return g_opt1;
}

const std::wstring& OptionChatPanel::GetOption2() {
    return g_opt2;
}

void OptionChatPanel::ChooseOption1() {
    runtime::EventBus::Emit(L"ui.panel.chat.option.choose", g_opt1);
}

void OptionChatPanel::ChooseOption2() {
    runtime::EventBus::Emit(L"ui.panel.chat.option.choose", g_opt2);
}

} // namespace pet::systems::ui::panels::chat
