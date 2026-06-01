#include "Systems/UI/Panels/ChatPanel/OptionChatPanel.h"

#include "Core/TextFile.h"
#include "Runtime/EventBus.h"

#include <map>
#include <vector>

namespace pet::systems::ui::panels::chat {
namespace {
bool g_open = false;
std::wstring g_opt1;
std::wstring g_opt2;
std::map<std::wstring, std::wstring> s_buttonResponses;

std::wstring Trim(const std::wstring& text) {
    const wchar_t* ws = L" \t\r\n";
    const size_t start = text.find_first_not_of(ws);
    const size_t end = text.find_last_not_of(ws);
    if (start == std::wstring::npos) {
        return L"";
    }
    return text.substr(start, end - start + 1);
}
} // namespace

bool OptionChatPanel::LoadConfig(const std::wstring& configPath) {
    s_buttonResponses.clear();

    std::vector<std::wstring> lines;
    if (!core::TextFile::ReadLines(configPath, lines)) {
        return false;
    }

    for (const auto& raw : lines) {
        std::wstring line = Trim(raw);
        if (line.empty() || line.front() == L'#') {
            continue;
        }

        const size_t eq = line.find(L'=');
        if (eq == std::wstring::npos) {
            continue;
        }

        std::wstring left = Trim(line.substr(0, eq));
        std::wstring value = Trim(line.substr(eq + 1));
        if (left.empty() || value.empty()) {
            continue;
        }

        s_buttonResponses[left] = value;
    }

    return !s_buttonResponses.empty();
}

const std::wstring* OptionChatPanel::GetResponse(const std::wstring& optionId) {
    const auto it = s_buttonResponses.find(optionId);
    return it != s_buttonResponses.end() ? &it->second : nullptr;
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
