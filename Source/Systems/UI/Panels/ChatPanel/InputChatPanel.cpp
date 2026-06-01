#include "Systems/UI/Panels/ChatPanel/InputChatPanel.h"

#include "Core/TextFile.h"
#include "Runtime/EventBus.h"

#include <cstring>
#include <map>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

namespace pet::systems::ui::panels::chat {
namespace {

std::wstring Trim(const std::wstring& text) {
    const wchar_t* ws = L" \t\r\n";
    const size_t start = text.find_first_not_of(ws);
    const size_t end = text.find_last_not_of(ws);
    if (start == std::wstring::npos) {
        return L"";
    }
    return text.substr(start, end - start + 1);
}

std::map<std::wstring, std::wstring> s_textResponses;
std::map<std::wstring, std::wstring> s_buttonResponses;
std::wstring s_defaultResponse;
bool s_open = false;
std::wstring s_input;

#ifdef _WIN32
std::wstring s_activeConfigPath;
FILETIME s_configWriteTime = {};
bool s_hasConfigTime = false;

bool UpdateConfigMetadata(const std::wstring& configPath) {
    WIN32_FILE_ATTRIBUTE_DATA data;
    if (!GetFileAttributesExW(configPath.c_str(), GetFileExInfoStandard, &data)) {
        return false;
    }

    s_configWriteTime = data.ftLastWriteTime;
    s_activeConfigPath = configPath;
    s_hasConfigTime = true;
    return true;
}

bool ConfigFileChanged() {
    if (!s_hasConfigTime || s_activeConfigPath.empty()) {
        return false;
    }

    WIN32_FILE_ATTRIBUTE_DATA data;
    if (!GetFileAttributesExW(s_activeConfigPath.c_str(), GetFileExInfoStandard, &data)) {
        return false;
    }

    return memcmp(&data.ftLastWriteTime, &s_configWriteTime, sizeof(FILETIME)) != 0;
}
#else
bool UpdateConfigMetadata(const std::wstring&) { return true; }
bool ConfigFileChanged() { return false; }
#endif

} // namespace

ChatInputAction InputChatPanel::ProcessInputChar(std::wstring& text, WPARAM key) {
    if (key == VK_RETURN) {
        return ChatInputAction::Submit;
    }

    if (key == VK_BACK) {
        if (!text.empty()) {
            text.pop_back();
            return ChatInputAction::TextChanged;
        }
        return ChatInputAction::None;
    }

    if (key < 0x20 || key == VK_ESCAPE) {
        return ChatInputAction::None;
    }

    text.push_back(static_cast<wchar_t>(key));
    return ChatInputAction::TextChanged;
}

bool InputChatPanel::LoadConfig(const std::wstring& configPath) {
    s_textResponses.clear();
    s_buttonResponses.clear();
    s_defaultResponse.clear();

    std::vector<std::wstring> lines;
    if (!core::TextFile::ReadLines(configPath, lines)) {
        return false;
    }

    for (const auto& lineRaw : lines) {
        std::wstring line = Trim(lineRaw);
        if (line.empty() || line.front() == L'#') {
            continue;
        }

        const size_t eq = line.find(L'=');
        if (eq == std::wstring::npos) {
            continue;
        }

        std::wstring left = Trim(line.substr(0, eq));
        std::wstring value = Trim(line.substr(eq + 1));
        if (left.empty()) {
            continue;
        }

        std::wstring type = L"text";
        std::wstring key = left;
        const size_t colon = left.find(L':');
        if (colon != std::wstring::npos) {
            type = Trim(left.substr(0, colon));
            key = Trim(left.substr(colon + 1));
        }

        if (type == L"default" || key == L"default") {
            s_defaultResponse = value;
            continue;
        }

        if (type == L"button") {
            s_buttonResponses[key] = value;
        } else {
            s_textResponses[key] = value;
        }
    }

    return UpdateConfigMetadata(configPath);
}

const std::wstring* InputChatPanel::GetTextResponse(const std::wstring& key) {
    const auto it = s_textResponses.find(key);
    return it != s_textResponses.end() ? &it->second : nullptr;
}

const std::wstring* InputChatPanel::GetButtonResponse(const std::wstring& buttonId) {
    const auto it = s_buttonResponses.find(buttonId);
    return it != s_buttonResponses.end() ? &it->second : nullptr;
}

const std::wstring* InputChatPanel::GetDefaultResponse() {
    return s_defaultResponse.empty() ? nullptr : &s_defaultResponse;
}

void InputChatPanel::MaybeReloadConfig() {
#ifdef _WIN32
    if (ConfigFileChanged() && !s_activeConfigPath.empty()) {
        LoadConfig(s_activeConfigPath);
    }
#endif
}

void InputChatPanel::Open() {
    s_open = true;
    runtime::EventBus::Emit(L"ui.panel.chat.input.open");
}

void InputChatPanel::Close() {
    s_open = false;
    runtime::EventBus::Emit(L"ui.panel.chat.input.close");
}

bool InputChatPanel::IsOpen() {
    return s_open;
}

void InputChatPanel::SetInputText(const std::wstring& text) {
    s_input = text;
    runtime::EventBus::Emit(L"ui.panel.chat.input.change", text);
}

const std::wstring& InputChatPanel::GetInputText() {
    return s_input;
}

void InputChatPanel::Submit() {
    runtime::EventBus::Emit(L"ui.panel.chat.input.submit", s_input);
}

} // namespace pet::systems::ui::panels::chat
