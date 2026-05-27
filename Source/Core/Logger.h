#pragma once

#include "Core/FileSystem.h"

#include <string>

namespace pet::core {

class Logger {
public:
    Logger() = delete;

    static void SetLogFilePath(const std::wstring& path);
    static void Info(const std::wstring& message);
    static void Warn(const std::wstring& message);
    static void Error(const std::wstring& message);

private:
    static void Write(const std::wstring& level, const std::wstring& message);
};

} // namespace pet::core
