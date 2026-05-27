#include "Core/Logger.h"

#include "Core/TextFile.h"
#include "Core/Timer.h"

namespace pet::core {
namespace {
std::wstring g_logPath = Path::GetExeDir() + L"\\pet.log";
}

void Logger::SetLogFilePath(const std::wstring& path) { g_logPath = path; }
void Logger::Info(const std::wstring& message) { Write(L"INFO", message); }
void Logger::Warn(const std::wstring& message) { Write(L"WARN", message); }
void Logger::Error(const std::wstring& message) { Write(L"ERROR", message); }

void Logger::Write(const std::wstring& level, const std::wstring& message) {
    const std::wstring line = L"[" + Timer::NowLocalTimeString() + L"] [" + level + L"] " + message + L"\n";
    TextFile::WriteText(g_logPath, line, true, true);
}

} // namespace pet::core
