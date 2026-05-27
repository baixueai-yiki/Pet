#pragma once

#include <string>

namespace pet::systems::ui::panels::chat {

class BubbleChatPanel {
public:
    static void ShowMessage(const std::wstring& text);
    static const std::wstring& LastMessage();
};

} // namespace pet::systems::ui::panels::chat
