#pragma once

#include "Core/FileSystem.h"

#include <string>
#include <vector>

namespace pet::core {

class TextFile {
public:
    TextFile() = delete;

    static bool ReadText(const std::wstring& path, std::wstring& out);
    static bool ReadLines(const std::wstring& path, std::vector<std::wstring>& lines);
    static bool WriteText(const std::wstring& path, const std::wstring& content, bool append = false, bool ensureBom = true);
    static bool FileIsEmpty(const std::wstring& path);
};

} // namespace pet::core
