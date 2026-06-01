#pragma once

#include <windows.h>
#include <string>

namespace pet::systems::ui::panels::chat {

enum class ChatInputAction {
    None,
    TextChanged,
    Submit,
};

class InputChatPanel {
public:
    static void Open();
    static void Close();
    static bool IsOpen();

    static void SetInputText(const std::wstring& text);
    static const std::wstring& GetInputText();

    static void Submit();

    static ChatInputAction ProcessInputChar(std::wstring& text, WPARAM key);
    static bool LoadConfig(const std::wstring& configPath);
    static const std::wstring* GetTextResponse(const std::wstring& key);
    static const std::wstring* GetButtonResponse(const std::wstring& buttonId);
    static const std::wstring* GetDefaultResponse();
    static void MaybeReloadConfig();
};

} // namespace pet::systems::ui::panels::chat
