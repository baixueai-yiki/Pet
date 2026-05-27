#pragma once

#include <string>

namespace pet::core {

class Path {
public:
    Path() = delete;

    static std::wstring GetExeDir();
    static std::wstring GetAssetPath(const std::wstring& relative);
    static std::wstring GetImagePath(const std::wstring& file);
    static std::wstring GetConfigPath(const std::wstring& file);
};

} // namespace pet::core
