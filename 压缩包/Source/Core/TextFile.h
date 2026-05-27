#pragma once
#include <string>
#include <vector>

namespace TextFile
{
    bool ReadLines(const std::wstring& path, std::vector<std::wstring>& lines);
    bool ReadText(const std::wstring& path, std::wstring& out);
    bool WriteText(const std::wstring& path, const std::wstring& content, bool append = false, bool ensureBom = true);
    bool FileIsEmpty(const std::wstring& path);
}

