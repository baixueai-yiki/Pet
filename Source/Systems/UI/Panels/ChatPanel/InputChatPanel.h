#pragma once

#include <string>

namespace pet::systems::ui::panels::chat {

class InputChatPanel {
public:
    static void Open();
    static void Close();
    static bool IsOpen();

    static void SetInputText(const std::wstring& text);
    static const std::wstring& GetInputText();

    static void Submit();
};

} // namespace pet::systems::ui::panels::chat
