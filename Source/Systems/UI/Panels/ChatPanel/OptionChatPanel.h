#pragma once

#include <string>

namespace pet::systems::ui::panels::chat {

class OptionChatPanel {
public:
    static void Open(const std::wstring& key1, const std::wstring& key2);
    static void Close();
    static bool IsOpen();

    static const std::wstring& GetOption1();
    static const std::wstring& GetOption2();

    static void ChooseOption1();
    static void ChooseOption2();
    static bool LoadConfig(const std::wstring& configPath);
    static const std::wstring* GetResponse(const std::wstring& optionId);
};

} // namespace pet::systems::ui::panels::chat
