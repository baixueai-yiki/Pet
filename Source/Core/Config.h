#pragma once

#include "Core/FileSystem.h"

#include <string>

namespace pet::core {

class Config {
public:
    Config() = delete;

    static bool Load(const std::wstring& path);
    static bool Save(const std::wstring& path);

    static std::wstring GetString(const std::wstring& key, const std::wstring& defaultValue = L"");
    static int GetInt(const std::wstring& key, int defaultValue = 0);

    static void SetString(const std::wstring& key, const std::wstring& value);
    static void SetInt(const std::wstring& key, int value);
};

} // namespace pet::core
