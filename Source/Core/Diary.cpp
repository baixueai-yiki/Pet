#include "Diary.h"
#include "Path.h"
#include "../Game/Chat/Chat.h"
#include <windows.h>
#include <fstream>
#include <string>
#include <ctime>
#include <vector>
#include <sstream>

// 宽字符串转 UTF-8，确保写入日志时编码正确
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

// 判断文件是否为空，用于决定是否写入 UTF-8 BOM
static bool FileIsEmpty(const std::wstring& path)
{
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in.is_open())
        return true;
    return in.tellg() == 0;
}

// 读取文本文件为逐行内容（自动识别 UTF-8 BOM）
static bool ReadFileLines(const std::wstring& path, std::vector<std::wstring>& lines)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
        return false;

    std::string data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    if (data.empty())
        return true;

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

    std::wstring wdata(static_cast<size_t>(wlen), L'\0');
    MultiByteToWideChar(codePage, 0, data.data(), static_cast<int>(data.size()), &wdata[0], wlen);

    std::wistringstream iss(wdata);
    std::wstring line;
    while (std::getline(iss, line))
    {
        if (!line.empty() && line.back() == L'\r')
            line.pop_back();
        lines.push_back(line);
    }
    return true;
}

static std::wstring ReplaceAll(std::wstring text, const std::wstring& from, const std::wstring& to)
{
    if (from.empty())
        return text;
    size_t pos = 0;
    while ((pos = text.find(from, pos)) != std::wstring::npos)
    {
        text.replace(pos, from.size(), to);
        pos += to.size();
    }
    return text;
}

// 将模板键中的 {月}/{日} 替换为当前日期，生成最终键
static std::wstring ResolveDiaryKeyTemplate(const std::wstring& keyTemplate)
{
    time_t now = time(nullptr);
    struct tm localTime = {};
    localtime_s(&localTime, &now);

    std::wstring key = keyTemplate;
    key = ReplaceAll(key, L"{月}", std::to_wstring(localTime.tm_mon + 1));
    key = ReplaceAll(key, L"{日}", std::to_wstring(localTime.tm_mday));
    return key;
}

// 从 diary_script.txt 读取指定键的文本，找不到则返回 fallback
static std::wstring GetDiaryScriptValue(const std::wstring& keyTemplate, const std::wstring& fallback)
{
    if (keyTemplate.empty())
        return fallback;

    std::vector<std::wstring> lines;
    if (!ReadFileLines(GetConfigPath(L"diary_script.txt"), lines))
        return fallback;

    const std::wstring key = ResolveDiaryKeyTemplate(keyTemplate);

    for (const auto& lineRaw : lines)
    {
        if (lineRaw.empty() || lineRaw[0] == L'#')
            continue;
        size_t sep = lineRaw.find(L'=');
        if (sep == std::wstring::npos)
            sep = lineRaw.find(L':');
        if (sep == std::wstring::npos)
            continue;

        std::wstring k = lineRaw.substr(0, sep);
        if (!k.empty() && k.front() == L'\ufeff')
            k.erase(0, 1);
        if (k == key)
            return lineRaw.substr(sep + 1);
    }
    return fallback;
}

// 追加写入一条日记（分隔线 + 日期 + 时间 + 正文）
static void AppendDiaryEntry()
{
    const std::wstring path = GetExeDir() + L"\\diary.txt";
    const bool empty = FileIsEmpty(path);

    std::ofstream out(path, std::ios::binary | std::ios::app);
    if (!out.is_open())
        return;

    if (empty)
    {
        // 仅在第一次写入时添加 UTF-8 BOM，便于编辑器识别编码
        const unsigned char bom[] = { 0xEF, 0xBB, 0xBF };
        out.write(reinterpret_cast<const char*>(bom), sizeof(bom));
    }

    // 获取本地时间（年/月/日/时/分/秒）
    time_t now = time(nullptr);
    struct tm localTime = {};
    localtime_s(&localTime, &now);

    const std::wstring sep = L"====================================================";
    std::wstring entry;
    entry.reserve(256);
    entry += sep + L"\n";
    // 日期行：2026年3月9日
    entry += std::to_wstring(localTime.tm_year + 1900) + L"年" +
             std::to_wstring(localTime.tm_mon + 1) + L"月" +
             std::to_wstring(localTime.tm_mday) + L"日\n";
    // 时间行：15时56分0秒
    entry += std::to_wstring(localTime.tm_hour) + L"时" +
             std::to_wstring(localTime.tm_min) + L"分" +
             std::to_wstring(localTime.tm_sec) + L"秒\n";
    // 正文内容：可通过键从 diary_script.txt 中取值
    //这是日记的第一段，主要为了说明今天是什么日子
    entry += GetDiaryScriptValue(L"{月}月{日}日", L"今天和你相处得很开心。") + L"\n";
    //这是日记的第二段，主要为了表达对你的关心
    entry += GetDiaryScriptValue(L"line_2", L"希望你也能好好照顾自己。") + L"\n";
    entry += sep + L"\n"; //这行的意思是这个分割线====================================也是正文的一部分，和开头的分割线一样

    // 写入 UTF-8 文本，避免中文乱码
    const std::string utf8 = WideToUtf8(entry);
    out.write(utf8.data(), static_cast<std::streamsize>(utf8.size()));
}

// 写入 state.json（只在退出时写一次）
static void WriteStateJson()
{
    long long lastInteraction = 0;
    int valence = 8;
    int arousal = 8;
    // 从 Chat 模块获取当前状态快照
    ChatGetStateSnapshot(lastInteraction, valence, arousal);

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

// 程序退出时的统一入口：先写状态，再追加日记
void OnProgramExit()
{
    WriteStateJson();
    AppendDiaryEntry();
}
