#pragma once

#include "Core/Path.h"

#include <string>

namespace pet::core {

class FileSystem {
public:
    FileSystem() = delete;

    static bool Exists(const std::wstring& path);
    static bool EnsureDirectory(const std::wstring& dirPath);
    static bool ReadBinary(const std::wstring& path, std::string& data);
    static bool WriteBinary(const std::wstring& path, const std::string& data, bool append = false);
    static bool IsFileEmpty(const std::wstring& path);
};

} // namespace pet::core
