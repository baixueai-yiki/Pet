#include "Core/Config.h"

#include "Core/TextFile.h"

#include <sstream>
#include <unordered_map>
#include <vector>

namespace pet::core {
namespace {
std::unordered_map<std::wstring, std::wstring> g_cfg;

std::wstring Trim(const std::wstring& s) {
    const wchar_t* ws = L" \t\r\n";
    const size_t b = s.find_first_not_of(ws);
    if (b == std::wstring::npos) return L"";
    const size_t e = s.find_last_not_of(ws);
    return s.substr(b, e - b + 1);
}
}

bool Config::Load(const std::wstring& path) {
    std::vector<std::wstring> lines;
    if (!TextFile::ReadLines(path, lines)) return false;
    g_cfg.clear();
    for (const auto& raw : lines) {
        std::wstring line = Trim(raw);
        if (line.empty() || line[0] == L'#') continue;
        size_t sep = line.find(L'=');
        if (sep == std::wstring::npos) sep = line.find(L':');
        if (sep == std::wstring::npos) continue;
        std::wstring key = Trim(line.substr(0, sep));
        std::wstring value = Trim(line.substr(sep + 1));
        if (!key.empty() && key.front() == L'\ufeff') key.erase(0, 1);
        g_cfg[key] = value;
    }
    return true;
}

bool Config::Save(const std::wstring& path) {
    std::wstringstream ss;
    for (const auto& [k, v] : g_cfg) ss << k << L" = " << v << L"\n";
    return TextFile::WriteText(path, ss.str(), false, true);
}

std::wstring Config::GetString(const std::wstring& key, const std::wstring& defaultValue) {
    const auto it = g_cfg.find(key);
    return it == g_cfg.end() ? defaultValue : it->second;
}

int Config::GetInt(const std::wstring& key, int defaultValue) {
    const auto it = g_cfg.find(key);
    if (it == g_cfg.end()) return defaultValue;
    try { return std::stoi(it->second); } catch (...) { return defaultValue; }
}

void Config::SetString(const std::wstring& key, const std::wstring& value) { g_cfg[key] = value; }
void Config::SetInt(const std::wstring& key, int value) { g_cfg[key] = std::to_wstring(value); }

} // namespace pet::core
