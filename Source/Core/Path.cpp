#include "Core/Path.h"

#include <windows.h>

namespace pet::core {

std::wstring Path::GetExeDir() {
    wchar_t path[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    std::wstring full(path);
    const size_t pos = full.find_last_of(L"\\/");
    return (pos == std::wstring::npos) ? full : full.substr(0, pos);
}

std::wstring Path::GetAssetPath(const std::wstring& relative) {
    return GetExeDir() + L"\\assets\\" + relative;
}

std::wstring Path::GetImagePath(const std::wstring& file) {
    return GetAssetPath(L"images\\" + file);
}

std::wstring Path::GetConfigPath(const std::wstring& file) {
    return GetExeDir() + L"\\config\\" + file;
}

} // namespace pet::core
