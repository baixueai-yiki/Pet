#include "Diary.h"
#include "Path.h"
#include "../Runtime/Scheduler.h"
#include "../Runtime/EventBus.h"
#include "TextFile.h"
#include <windows.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

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

void DiaryInit()
{
    EventSubscribe(L"diary.append", [](const Event& evt) {
        if (!evt.payload.empty())
            DiaryAppendWritingLine(evt.payload);
    });
}

static std::wstring Trim(const std::wstring& s)
{
    const wchar_t* ws = L" \t\r\n";
    size_t b = s.find_first_not_of(ws);
    size_t e = s.find_last_not_of(ws);
    if (b == std::wstring::npos)
        return L"";
    return s.substr(b, e - b + 1);
}

static bool GetDiaryScriptLineForDate(const DateTime& dt, std::wstring& out)
{
    out.clear();
    std::vector<std::wstring> lines;
    if (!TextFile::ReadLines(GetConfigPath(L"diary_script.txt"), lines))
        return false;

    const std::wstring keyA = std::to_wstring(dt.month) + L"月" + std::to_wstring(dt.day) + L"日";
    const std::wstring keyB = (dt.month < 10 ? L"0" : L"") + std::to_wstring(dt.month) +
                              L"月" + (dt.day < 10 ? L"0" : L"") + std::to_wstring(dt.day) + L"日";

    for (const auto& raw : lines)
    {
        std::wstring line = Trim(raw);
        if (line.empty() || line[0] == L'#')
            continue;
        size_t sep = line.find(L':');
        if (sep == std::wstring::npos)
            sep = line.find(L'：');
        if (sep == std::wstring::npos)
            sep = line.find(L'=');
        if (sep == std::wstring::npos)
            continue;
        std::wstring key = Trim(line.substr(0, sep));
        if (!key.empty() && key.front() == L'\ufeff')
            key.erase(0, 1);
        std::wstring value = Trim(line.substr(sep + 1));
        if (value.empty())
            continue;
        if (key == keyA || key == keyB)
        {
            out = value;
            return true;
        }
    }
    return false;
}

void OnProgramStart()
{
    const std::wstring path = GetConfigPath(L"diary_writing.txt");
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
}

void DiaryAppendWritingLine(const std::wstring& line)
{
    const std::wstring path = GetConfigPath(L"diary_writing.txt");
    const bool empty = TextFile::FileIsEmpty(path);

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
    const bool empty = TextFile::FileIsEmpty(diaryPath);

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
    entry += std::to_wstring(localTime.year) + L"年" +
             std::to_wstring(localTime.month) + L"月" +
             std::to_wstring(localTime.day) + L"日\n";
    entry += std::to_wstring(localTime.hour) + L"时" +
             std::to_wstring(localTime.minute) + L"分" +
             std::to_wstring(localTime.second) + L"秒\n";

    std::wstring festivalLine;
    if (GetDiaryScriptLineForDate(localTime, festivalLine))
        entry += festivalLine + L"\n";

    std::wstring writing;
    if (TextFile::ReadText(GetConfigPath(L"diary_writing.txt"), writing) && !writing.empty())
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
