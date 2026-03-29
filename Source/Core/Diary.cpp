#include "Diary.h"
#include "Path.h"
#include "../Runtime/Scheduler.h"
#include <windows.h>
#include <fstream>
#include <string>

static std::string WideToUtf8(const std::wstring& text)
{
    if (text.empty())
        return std::string();
    int len = WideCharToMultiByte(CP_UTF8, 0, text.c_str(),
        static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    if (len <= 0)
        return std::string();
    std::string out(static_cast<size_t>(len), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.c_str(),
        static_cast<int>(text.size()), &out[0], len, nullptr, nullptr);
    return out;
}

static StateSnapshotProvider s_stateProvider = nullptr;

void DiarySetStateSnapshotProvider(StateSnapshotProvider provider)
{
    s_stateProvider = provider;
}

static bool FileIsEmpty(const std::wstring& path)
{
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in.is_open())
        return true;
    return in.tellg() == 0;
}

static bool ReadFileText(const std::wstring& path, std::wstring& out)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
        return false;

    std::string data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    if (data.empty())
    {
        out.clear();
        return true;
    }

    bool utf8 = (data.size() >= 3 &&
                 static_cast<unsigned char>(data[0]) == 0xEF &&
                 static_cast<unsigned char>(data[1]) == 0xBB &&
                 static_cast<unsigned char>(data[2]) == 0xBF);
    if (utf8)
        data.erase(0, 3);

    UINT codePage = utf8 ? CP_UTF8 : CP_ACP;
    int wlen = MultiByteToWideChar(codePage, 0, data.data(), static_cast<int>(data.size()), nullptr, 0);
    if (wlen <= 0)
        return false;

    out.assign(static_cast<size_t>(wlen), L'\0');
    MultiByteToWideChar(codePage, 0, data.data(), static_cast<int>(data.size()), &out[0], wlen);
    return true;
}

void OnProgramStart()
{
    const std::wstring path = GetConfigPath(L"diary_writing.txt");
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
}

void DiaryAppendWritingLine(const std::wstring& line)
{
    const std::wstring path = GetConfigPath(L"diary_writing.txt");
    const bool empty = FileIsEmpty(path);

    std::ofstream out(path, std::ios::binary | std::ios::app);
    if (!out.is_open())
        return;

    if (empty)
    {
        const unsigned char bom[] = { 0xEF, 0xBB, 0xBF };
        out.write(reinterpret_cast<const char*>(bom), sizeof(bom));
    }

    std::wstring text = line;
    if (!text.empty() && text.back() != L'\n')
        text += L"\n";

    const std::string utf8 = WideToUtf8(text);
    out.write(utf8.data(), static_cast<std::streamsize>(utf8.size()));
}

static void AppendDiaryEntry()
{
    const std::wstring diaryPath = GetExeDir() + L"\\diary.txt";
    const bool empty = FileIsEmpty(diaryPath);

    std::ofstream out(diaryPath, std::ios::binary | std::ios::app);
    if (!out.is_open())
        return;

    if (empty)
    {
        const unsigned char bom[] = { 0xEF, 0xBB, 0xBF };
        out.write(reinterpret_cast<const char*>(bom), sizeof(bom));
    }

    const DateTime localTime = GetLocalDateTime();

    std::wstring entry;
    entry.reserve(256);
    entry += L"==========\n";
    entry += std::to_wstring(localTime.year) + L"\u5e74" +
             std::to_wstring(localTime.month) + L"\u6708" +
             std::to_wstring(localTime.day) + L"\u65e5\n";
    entry += std::to_wstring(localTime.hour) + L"\u65f6" +
             std::to_wstring(localTime.minute) + L"\u5206" +
             std::to_wstring(localTime.second) + L"\u79d2\n";

    std::wstring writing;
    if (ReadFileText(GetConfigPath(L"diary_writing.txt"), writing) && !writing.empty())
        entry += writing + L"\n";

    const std::string utf8 = WideToUtf8(entry);
    out.write(utf8.data(), static_cast<std::streamsize>(utf8.size()));
}

static void WriteStateJson()
{
    long long lastInteraction = 0;
    int valence = 8;
    int arousal = 8;
    if (s_stateProvider)
        s_stateProvider(lastInteraction, valence, arousal);

    const std::wstring configDir = GetExeDir() + L"\\config";
    CreateDirectoryW(configDir.c_str(), nullptr);
    const std::wstring path = configDir + L"\\state.json";

    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out.is_open())
        return;

    out << "{\n";
    out << "  \"last_interaction_time\": " << lastInteraction << ",\n";
    out << "  \"valence\": " << valence << ",\n";
    out << "  \"arousal\": " << arousal << "\n";
    out << "}\n";
}

void OnProgramExit()
{
    WriteStateJson();
    AppendDiaryEntry();
}
