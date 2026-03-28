#include <windows.h>
#include <windowsx.h>
#include <imm.h>
#include "Chat.h"
#include "Core/Path.h"
#include "Core/Diary.h"
#include "../../Game/Pet/Pet.h"
#include "../../Engine/Input/InputDispatcher.h"
#include <string>
#include <fstream>
#include <map>
#include <vector>
#include <sstream>
#include <cstring>
#include <set>
#include <shellapi.h>
#include <iomanip>
#include <fstream>
#include <chrono>
#include <ctime>
#include <cwctype>
#include <tlhelp32.h>
#include "Game/Audio/Audio.h"

// 全局静态状态（仅当前 cpp 使用）
static HWND s_hInputWnd = nullptr;// 当前输入窗口句柄
static HWND s_hButtonWnd = nullptr;// 当前按钮窗口句柄
static std::wstring s_inputText;// 当前输入框中的文本内容
static POINT s_dragOffset;// 鼠标拖拽偏移
static bool s_dragging = false;// 拖拽状态
static std::map<std::wstring, std::wstring> s_dialogMap;// 聊天配置表
static std::map<std::wstring, std::wstring> s_buttonMap;// 按钮选项配置
static std::map<std::wstring, std::wstring> s_buttonLabelMap;// 按钮显示文本配置
static HFONT s_inputFont = nullptr; // 统一输入窗口字体，避免中文显示乱码
static std::wstring s_imeText; // 输入法组合字符串
static std::wstring s_buttonKey1; // 当前按钮1逻辑键
static std::wstring s_buttonKey2; // 当前按钮2逻辑键
static const int kInputWidth = 300;
static const int kInputHeight = 40;
static const int kButtonHeight = 80;

static int GetInputWidth()
{
    return (g_pet.w > 0) ? g_pet.w : kInputWidth;
}

static void GetInputRect(int& x, int& y, int& w, int& h)
{
    w = GetInputWidth();
    h = kInputHeight;
    x = g_pet.x;
    y = g_pet.y - h - 10;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
}

static void GetButtonRect(int& x, int& y, int& w, int& h)
{
    w = GetInputWidth();
    h = kButtonHeight;
    x = g_pet.x;
    y = g_pet.y - h - 10;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
}
static std::wstring s_dialogPath;
static FILETIME s_dialogWriteTime = {};
static bool s_hasDialogTime = false;
static HWND s_hTalkWnd = nullptr;
static std::wstring s_talkText;
static HWND s_hTaskWnd = nullptr;
struct TaskEntry
{
    std::wstring title;
    HICON icon;
    DWORD pid;
    std::wstring processName;
};
static std::vector<TaskEntry> s_taskEntries;
static int s_taskSelected = -1;
static HWND s_hKillBtn = nullptr;
static DWORD s_taskSelectedPid = 0;
static std::set<std::wstring> s_gameActiveKeys;
static std::set<std::wstring> s_gameActiveGameKeys;
static std::set<std::wstring> s_gameActiveWorkKeys;
static int s_monitorState = 0;
static std::set<std::wstring> s_diaryLoggedCategories;
static std::map<std::wstring, int> s_diaryCategoryCounts;
static HFONT s_talkFont = nullptr;
static const int kTalkMaxWidth = 240;
static const int kTalkPadding = 8;
static const int kTalkCorner = 10;
static const int kTalkTail = 10;
static bool s_talkOnRight = true;
static const UINT_PTR kTalkAutoHideTimer = 1;
static const UINT kTalkAutoHideMs = 3000;
static HANDLE s_quitTimer = nullptr;
static const DWORD kQuitDelayMs = 5000;
static HWND s_mainHwnd = nullptr;
static const int kTaskHeight = 200;
static const UINT_PTR kTaskRefreshTimer = 2;
static const UINT kTaskRefreshMs = 5000;
static const int kTaskPadding = 10;
static const int kTaskLineH = 24;
static const int kTaskIcon = 16;
struct ChatState
{
    long long lastInteraction;
    int valence;
    int arousal;
};

static ChatState s_state = { 0, 8, 8 };
static bool s_stateLoaded = false;
static bool s_idleSeeded = false;

#pragma comment(lib, "shell32.lib")

static bool IsTaskListTrigger(const std::wstring& text)
{
    return text == L"任务管理器";
}

// forward decls
static void HideKillButton();
static bool IsAppWindow(HWND hwnd);
void ChatTalk(HWND hwnd, const wchar_t* text);
static void BuildTaskListEntries();

static bool ReadFileLines(const std::wstring& path, std::vector<std::wstring>& lines);
static std::wstring Trim(const std::wstring& text);

static int ReadSettingsInt(const std::wstring& key, int defaultValue)
{
    std::vector<std::wstring> lines;
    if (!ReadFileLines(GetConfigPath(L"settings.txt"), lines))
        return defaultValue;

    for (const auto& lineRaw : lines)
    {
        std::wstring line = Trim(lineRaw);
        if (line.empty() || line[0] == L'#')
            continue;
        size_t eq = line.find(L'=');
        if (eq == std::wstring::npos)
            continue;
        std::wstring k = Trim(line.substr(0, eq));
        if (!k.empty() && k.front() == L'\ufeff')
            k.erase(0, 1);
        std::wstring v = Trim(line.substr(eq + 1));
        if (k == key)
        {
            int val = defaultValue;
            std::wistringstream iss(v);
            iss >> val;
            return val;
        }
    }

    return defaultValue;
}
static VOID CALLBACK QuitTimerProc(PVOID, BOOLEAN)
{
    if (s_mainHwnd)
        PostMessageW(s_mainHwnd, WM_CLOSE, 0, 0);
}

static std::wstring Trim(const std::wstring& text)
{
    const wchar_t* ws = L" \t\r\n";
    size_t start = text.find_first_not_of(ws);
    size_t end = text.find_last_not_of(ws);
    if (start == std::wstring::npos)
        return L"";
    return text.substr(start, end - start + 1);
}

#pragma comment(lib, "imm32.lib")


static bool ReadFileLines(const std::wstring& path, std::vector<std::wstring>& lines)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
        return false;

    std::string data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    if (data.empty())
        return true;

    auto IsLikelyUtf8 = [](const std::string& s) -> bool
    {
        size_t i = 0;
        const size_t n = s.size();
        while (i < n)
        {
            unsigned char c = static_cast<unsigned char>(s[i]);
            if (c <= 0x7F)
            {
                ++i;
                continue;
            }
            size_t need = 0;
            if ((c & 0xE0) == 0xC0) need = 1;
            else if ((c & 0xF0) == 0xE0) need = 2;
            else if ((c & 0xF8) == 0xF0) need = 3;
            else return false;
            if (i + need >= n) return false;
            for (size_t j = 1; j <= need; ++j)
            {
                unsigned char cc = static_cast<unsigned char>(s[i + j]);
                if ((cc & 0xC0) != 0x80) return false;
            }
            i += need + 1;
        }
        return true;
    };

    bool utf8 = (data.size() >= 3 &&
                 static_cast<unsigned char>(data[0]) == 0xEF &&
                 static_cast<unsigned char>(data[1]) == 0xBB &&
                 static_cast<unsigned char>(data[2]) == 0xBF);
    if (utf8)
        data.erase(0, 3);
    else if (IsLikelyUtf8(data))
        utf8 = true;

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

static bool ReadFileText(const std::wstring& path, std::wstring& text)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
        return false;

    std::string data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    if (data.empty())
    {
        text.clear();
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

    text.assign(static_cast<size_t>(wlen), L'\0');
    MultiByteToWideChar(codePage, 0, data.data(), static_cast<int>(data.size()), &text[0], wlen);
    return true;
}

static long long GetUnixTimeSeconds()
{
    return static_cast<long long>(time(nullptr));
}

static bool ParseJsonInt64(const std::wstring& text, const std::wstring& key, long long& out)
{
    std::wstring pattern = L"\"" + key + L"\"";
    size_t pos = text.find(pattern);
    if (pos == std::wstring::npos)
        return false;
    pos = text.find(L':', pos + pattern.size());
    if (pos == std::wstring::npos)
        return false;
    ++pos;
    while (pos < text.size() && iswspace(text[pos]))
        ++pos;
    if (pos >= text.size())
        return false;
    bool neg = false;
    if (text[pos] == L'-')
    {
        neg = true;
        ++pos;
    }
    if (pos >= text.size() || !iswdigit(text[pos]))
        return false;
    long long value = 0;
    while (pos < text.size() && iswdigit(text[pos]))
    {
        value = value * 10 + (text[pos] - L'0');
        ++pos;
    }
    out = neg ? -value : value;
    return true;
}

static void ClampState(ChatState& state)
{
    if (state.valence < 1)
        state.valence = 1;
    if (state.valence > 15)
        state.valence = 15;
    if (state.arousal < 1)
        state.arousal = 1;
    if (state.arousal > 15)
        state.arousal = 15;
    if (state.lastInteraction < 0)
        state.lastInteraction = 0;
}

static std::wstring GetStatePath()
{
    return GetConfigPath(L"state.json");
}

static void SaveState(const ChatState& state)
{
    const std::wstring configDir = GetExeDir() + L"\\config";
    CreateDirectoryW(configDir.c_str(), nullptr);

    const std::wstring path = GetStatePath();
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out.is_open())
        return;

    out << "{\n";
    out << "  \"last_interaction_time\": " << state.lastInteraction << ",\n";
    out << "  \"valence\": " << state.valence << ",\n";
    out << "  \"arousal\": " << state.arousal << "\n";
    out << "}\n";
}

static bool TryLoadState(ChatState& out)
{
    std::wstring text;
    if (!ReadFileText(GetStatePath(), text))
        return false;

    bool any = false;
    ChatState tmp = out;
    long long value = 0;
    if (ParseJsonInt64(text, L"last_interaction_time", value))
    {
        tmp.lastInteraction = value;
        any = true;
    }
    if (ParseJsonInt64(text, L"valence", value))
    {
        tmp.valence = static_cast<int>(value);
        any = true;
    }
    if (ParseJsonInt64(text, L"arousal", value))
    {
        tmp.arousal = static_cast<int>(value);
        any = true;
    }
    if (!any)
        return false;

    ClampState(tmp);
    if (tmp.lastInteraction == 0)
        tmp.lastInteraction = GetUnixTimeSeconds();

    out = tmp;
    return true;
}

static void EnsureStateLoaded()
{
    if (s_stateLoaded)
        return;

    ChatState loaded = s_state;
    if (!TryLoadState(loaded))
    {
        loaded.lastInteraction = GetUnixTimeSeconds();
        ClampState(loaded);
    }
    s_state = loaded;
    s_stateLoaded = true;
}

struct IdleEntry
{
    std::wstring key;
    std::wstring text;
};

static bool LoadIdleMap(std::map<std::wstring, std::wstring>& mapOut)
{
    std::vector<std::wstring> lines;
    if (!ReadFileLines(GetAssetPath(L"chat\\chat_idle.txt"), lines))
        return false;

    mapOut.clear();
    for (const auto& lineRaw : lines)
    {
        std::wstring line = Trim(lineRaw);
        if (line.empty() || line[0] == L'#')
            continue;
        size_t eq = line.find(L'=');
        if (eq == std::wstring::npos)
            continue;
        std::wstring key = Trim(line.substr(0, eq));
        if (!key.empty() && key.front() == L'\ufeff')
            key.erase(0, 1);
        std::wstring value = Trim(line.substr(eq + 1));
        mapOut[key] = value;
    }
    return !mapOut.empty();
}

static bool LoadIdleLines(std::vector<IdleEntry>& linesOut)
{
    std::map<std::wstring, std::wstring> map;
    if (!LoadIdleMap(map))
        return false;

    linesOut.clear();
    for (int i = 1; i <= 5; ++i)
    {
        std::wstring key = L"idle_" + std::to_wstring(i);
        auto it = map.find(key);
        if (it != map.end() && !it->second.empty())
            linesOut.push_back({ key, it->second });
    }
    return !linesOut.empty();
}

static void LoadDiaryMapFromFile(const std::wstring& file, std::map<std::wstring, std::wstring>& out)
{
    std::vector<std::wstring> lines;
    const std::wstring path = (file == L"diary_script.txt")
        ? GetConfigPath(file)
        : GetAssetPath(L"chat\\" + file);
    if (!ReadFileLines(path, lines))
        return;

    for (const auto& lineRaw : lines)
    {
        std::wstring line = Trim(lineRaw);
        if (line.empty() || line[0] == L'#')
            continue;
        size_t sep = line.find(L'=');
        if (sep == std::wstring::npos)
            sep = line.find(L':');
        if (sep == std::wstring::npos)
            continue;
        std::wstring key = Trim(line.substr(0, sep));
        if (!key.empty() && key.front() == L'\ufeff')
            key.erase(0, 1);
        std::wstring value = Trim(line.substr(sep + 1));
        if (!key.empty() && !value.empty() && key.rfind(L"diary_", 0) == 0)
            out[key] = value;
    }
}

static bool LoadDiaryScriptMap(std::map<std::wstring, std::wstring>& out)
{
    out.clear();
    // 先读 diary_script.txt
    LoadDiaryMapFromFile(L"diary_script.txt", out);
    // 再读 monitor_game.txt（允许把 diary_ 写在这里）
    LoadDiaryMapFromFile(L"monitor_game.txt", out);
    return !out.empty();
}

static std::wstring GetDiaryCategoryForKey(const std::wstring& key)
{
    if (key.rfind(L"idle_", 0) == 0)
        return L"idle";
    if (key.rfind(L"sleep_", 0) == 0)
        return L"sleep";
    if (key.rfind(L"morning_", 0) == 0 ||
        key.rfind(L"lunch_", 0) == 0 ||
        key.rfind(L"dinner_", 0) == 0 ||
        key.rfind(L"night_", 0) == 0)
        return L"greeting";
    return L"";
}

static void TryAppendDiaryForKey(const std::wstring& key)
{
    if (key.empty())
        return;
    const std::wstring category = GetDiaryCategoryForKey(key);
    if (category.empty())
        return;
    if (s_diaryLoggedCategories.find(category) != s_diaryLoggedCategories.end())
        return;

    std::map<std::wstring, std::wstring> map;
    if (!LoadDiaryScriptMap(map))
        return;

    const std::wstring diaryKey = L"diary_" + key;
    auto it = map.find(diaryKey);
    if (it == map.end() || it->second.empty())
        return;

    DiaryAppendWritingLine(it->second);
    s_diaryLoggedCategories.insert(category);
}

static void TryAppendDiaryForKeyword(const std::wstring& key, const std::wstring& category, int maxCount)
{
    if (key.empty())
        return;
    if (category.empty() || maxCount <= 0)
        return;
    int& count = s_diaryCategoryCounts[category];
    if (count >= maxCount)
        return;

    std::map<std::wstring, std::wstring> map;
    if (!LoadDiaryScriptMap(map))
        return;

    const std::wstring diaryKey = L"diary_" + key;
    auto it = map.find(diaryKey);
    if (it == map.end() || it->second.empty())
        return;

    DiaryAppendWritingLine(it->second);
    ++count;
}

static bool GetIdleTextByKey(const std::wstring& key, std::wstring& out, std::wstring& keyUsed)
{
    if (key.empty())
        return false;
    std::map<std::wstring, std::wstring> map;
    if (!LoadIdleMap(map))
        return false;

    std::vector<std::wstring> candidates;
    for (int i = 1; i <= 10; ++i)
    {
        std::wstring k = key + L"_" + std::to_wstring(i);
        auto it = map.find(k);
        if (it != map.end() && !it->second.empty())
            candidates.push_back(it->second);
    }
    if (!candidates.empty())
    {
        if (!s_idleSeeded)
        {
            s_idleSeeded = true;
            srand(static_cast<unsigned int>(GetTickCount()));
        }
        const int idx = rand() % static_cast<int>(candidates.size());
        out = candidates[static_cast<size_t>(idx)];
        keyUsed = key + L"_" + std::to_wstring(idx + 1);
        return true;
    }

    auto it = map.find(key);
    if (it == map.end() || it->second.empty())
        return false;
    out = it->second;
    keyUsed = key;
    return true;
}

static bool GetRandomVariantLine(const std::wstring& baseKey, int maxIndex, IdleEntry& out)
{
    if (baseKey.empty() || maxIndex <= 0)
        return false;
    std::map<std::wstring, std::wstring> map;
    if (!LoadIdleMap(map))
        return false;
    std::vector<IdleEntry> variants;
    for (int i = 1; i <= maxIndex; ++i)
    {
        std::wstring key = baseKey + L"_" + std::to_wstring(i);
        auto it = map.find(key);
        if (it != map.end() && !it->second.empty())
            variants.push_back({ key, it->second });
    }
    if (variants.empty())
        return false;
    if (!s_idleSeeded)
    {
        s_idleSeeded = true;
        srand(static_cast<unsigned int>(GetTickCount()));
    }
    out = variants[static_cast<size_t>(rand()) % variants.size()];
    return true;
}

static void PlayIdleSound(const std::wstring& key)
{
    if (key.empty())
        return;
    PlayAudioAsset(L"audio\\" + key + L".wav");
}

static std::wstring ToLowerCopy(std::wstring s)
{
    for (auto& ch : s)
        ch = static_cast<wchar_t>(towlower(ch));
    return s;
}

static std::wstring GetProcessNameByPid(DWORD pid)
{
    if (pid == 0)
        return L"";
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!hProcess)
        return L"";
    wchar_t path[MAX_PATH] = {};
    DWORD size = MAX_PATH;
    if (!QueryFullProcessImageNameW(hProcess, 0, path, &size))
    {
        CloseHandle(hProcess);
        return L"";
    }
    CloseHandle(hProcess);
    const wchar_t* name = wcsrchr(path, L'\\');
    return name ? (name + 1) : path;
}

static bool LoadKeywordMap(const std::wstring& file, std::map<std::wstring, std::wstring>& out)
{
    std::vector<std::wstring> lines;
    if (!ReadFileLines(GetAssetPath(L"chat\\" + file), lines))
        return false;

    out.clear();
    for (const auto& lineRaw : lines)
    {
        std::wstring line = Trim(lineRaw);
        if (line.empty() || line[0] == L'#')
            continue;
        size_t sep = line.find(L'=');
        if (sep == std::wstring::npos)
            sep = line.find(L':');
        if (sep == std::wstring::npos)
            continue;
        std::wstring key = Trim(line.substr(0, sep));
        if (!key.empty() && key.front() == L'\ufeff')
            key.erase(0, 1);
        std::wstring value = Trim(line.substr(sep + 1));
        if (!key.empty() && !value.empty())
            out[key] = value;
    }
    return !out.empty();
}

static bool MatchAnyKeyword(const std::map<std::wstring, std::wstring>& map)
{
    for (const auto& kv : map)
    {
        const std::wstring keyLower = ToLowerCopy(kv.first);
        for (const auto& entry : s_taskEntries)
        {
            const std::wstring nameLower = ToLowerCopy(entry.processName);
            const std::wstring titleLower = ToLowerCopy(entry.title);
            if ((!nameLower.empty() && nameLower.find(keyLower) != std::wstring::npos) ||
                (!titleLower.empty() && titleLower.find(keyLower) != std::wstring::npos))
                return true;
        }
    }
    return false;
}

// 每分钟根据当前运行进程更新状态（0/1/2）
static void UpdateMonitorState()
{
    // 用当前任务列表的进程名做匹配（可见窗口对应的进程）
    BuildTaskListEntries();
    if (s_taskEntries.empty())
    {
        s_monitorState = 0;
        return;
    }

    std::map<std::wstring, std::wstring> gameMap;
    std::map<std::wstring, std::wstring> workMap;
    LoadKeywordMap(L"monitor_game.txt", gameMap);
    LoadKeywordMap(L"monitor_work.txt", workMap);

    const bool hasWork = MatchAnyKeyword(workMap);
    const bool hasGame = MatchAnyKeyword(gameMap);
    if (hasWork)
        s_monitorState = 2;
    else if (hasGame)
        s_monitorState = 1;
    else
        s_monitorState = 0;
}

static bool IsSleepHour(int hour)
{
    return hour >= 0 && hour < 6;
}

static bool KillProcessesByKeywords(const std::map<std::wstring, std::wstring>& map)
{
    if (map.empty())
        return false;

    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE)
        return false;

    bool killedAny = false;
    PROCESSENTRY32W pe = {};
    pe.dwSize = sizeof(pe);
    if (Process32FirstW(snap, &pe))
    {
        do
        {
            if (pe.th32ProcessID == GetCurrentProcessId())
                continue;
            std::wstring exe = ToLowerCopy(pe.szExeFile);
            for (const auto& kv : map)
            {
                const std::wstring keyLower = ToLowerCopy(kv.first);
                if (exe.find(keyLower) == std::wstring::npos)
                    continue;
                HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                if (h)
                {
                    if (TerminateProcess(h, 0))
                        killedAny = true;
                    CloseHandle(h);
                }
                break;
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return killedAny;
}

// 检测运行中的应用进程名/窗口标题是否包含关键词，命中时说出对应的话
static bool CheckGameKeywords(HWND hwnd)
{
    std::map<std::wstring, std::wstring> lifeMap;
    LoadKeywordMap(L"monitor_life.txt", lifeMap);
    std::map<std::wstring, std::wstring> gameMap;
    std::map<std::wstring, std::wstring> workMap;
    LoadKeywordMap(L"monitor_game.txt", gameMap);
    LoadKeywordMap(L"monitor_work.txt", workMap);

    UpdateMonitorState();

    if (s_taskEntries.empty())
    {
        s_gameActiveKeys.clear();
        s_gameActiveGameKeys.clear();
        s_gameActiveWorkKeys.clear();
        return false;
    }

    std::set<std::wstring> current;
    std::set<std::wstring> currentGame;
    std::set<std::wstring> currentWork;
    const std::wstring* reply = nullptr;
    std::wstring matchedKey;
    std::wstring matchedCategory;
    auto ScanMap = [&](const std::map<std::wstring, std::wstring>& map,
                       std::set<std::wstring>& currentOut,
                       std::set<std::wstring>& activeSet,
                       const std::wstring& category,
                       const std::wstring** outReply,
                       std::wstring* outKey,
                       std::wstring* outCategory)
    {
        for (const auto& kv : map)
        {
            const std::wstring keyLower = ToLowerCopy(kv.first);
            bool found = false;
            for (const auto& entry : s_taskEntries)
            {
                const std::wstring nameLower = ToLowerCopy(entry.processName);
                const std::wstring titleLower = ToLowerCopy(entry.title);
                if ((!nameLower.empty() && nameLower.find(keyLower) != std::wstring::npos) ||
                    (!titleLower.empty() && titleLower.find(keyLower) != std::wstring::npos))
                {
                    found = true;
                    break;
                }
            }
            if (found)
            {
                currentOut.insert(kv.first);
                if (!*outReply && activeSet.find(kv.first) == activeSet.end())
                {
                    *outReply = &kv.second;
                    *outKey = kv.first;
                    *outCategory = category;
                }
            }
        }
    };

    // 优先工作，其次游戏，最后日常
    if (!workMap.empty())
        ScanMap(workMap, currentWork, s_gameActiveWorkKeys, L"work", &reply, &matchedKey, &matchedCategory);
    if (!reply && !gameMap.empty())
        ScanMap(gameMap, currentGame, s_gameActiveGameKeys, L"game", &reply, &matchedKey, &matchedCategory);
    if (!reply && !lifeMap.empty())
        ScanMap(lifeMap, current, s_gameActiveKeys, L"life", &reply, &matchedKey, &matchedCategory);

    s_gameActiveKeys = std::move(current);
    s_gameActiveGameKeys = std::move(currentGame);
    s_gameActiveWorkKeys = std::move(currentWork);

    if (reply)
    {
        ChatTalk(hwnd, reply->c_str());
        // work/game 共用一次配额，life 单独两次配额
        if (matchedCategory == L"work" || matchedCategory == L"game")
            TryAppendDiaryForKeyword(matchedKey, L"work_game", 1);
        else
            TryAppendDiaryForKeyword(matchedKey, L"life", 2);
        return true;
    }
    return false;
}

static int GetLocalHour()
{
    time_t now = time(nullptr);
    struct tm localTime = {};
    localtime_s(&localTime, &now);
    return localTime.tm_hour;
}

static std::wstring GetIdleOverrideKeyForHour(int hour)
{
    const int morning = ReadSettingsInt(L"早安时间", 7);
    const int lunch = ReadSettingsInt(L"午餐时间", 12);
    const int dinner = ReadSettingsInt(L"晚餐时间", 18);
    const int night = ReadSettingsInt(L"晚安时间", 22);

    if (hour == morning)
        return L"morning";
    if (hour == lunch)
        return L"lunch";
    if (hour == dinner)
        return L"dinner";
    if (hour == night)
        return L"night";
    return L"";
}

static bool UpdateDialogMetadata(const std::wstring& path)
{
    WIN32_FILE_ATTRIBUTE_DATA data;
    if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &data))
        return false;

    s_dialogWriteTime = data.ftLastWriteTime;
    s_dialogPath = path;
    s_hasDialogTime = true;
    return true;
}

static bool DialogFileChanged()
{
    if (!s_hasDialogTime || s_dialogPath.empty())
        return false;

    WIN32_FILE_ATTRIBUTE_DATA data;
    if (!GetFileAttributesExW(s_dialogPath.c_str(), GetFileExInfoStandard, &data))
        return false;

    return memcmp(&data.ftLastWriteTime, &s_dialogWriteTime, sizeof(FILETIME)) != 0;
}

// 配置文件读取
static void LoadDialogConfig()
{
    s_dialogMap.clear();
    s_buttonMap.clear();
    s_buttonLabelMap.clear();

    std::vector<std::wstring> lines;
    const std::wstring path = GetAssetPath(L"chat\\chat_safeword.txt");
    if (!ReadFileLines(path, lines))
        return;

    for (const auto& lineRaw : lines)
    {
        std::wstring line = Trim(lineRaw);
        if (line.empty())
            continue;

        // 三段式：key=label=reply
        size_t firstEq = line.find(L'=');
        if (firstEq == std::wstring::npos)
            continue;
        size_t secondEq = line.find(L'=', firstEq + 1);

        std::wstring key = Trim(line.substr(0, firstEq));
        // 移除 BOM
        if (!key.empty() && key.front() == L'\ufeff')
            key.erase(0, 1);

        std::wstring value;
        std::wstring label;

        if (secondEq != std::wstring::npos)
        {
            label = Trim(line.substr(firstEq + 1, secondEq - firstEq - 1));
            value = Trim(line.substr(secondEq + 1));
        }
        else
        {
            value = Trim(line.substr(firstEq + 1));
        }

        // 判断是否为按钮选项 / 按钮显示文本
        if (!key.empty() && key[0] == L'#')
        {
            // 约定：#按钮1=对应点击后的回复
            s_buttonMap[key.substr(1)] = value;
            continue;
        }

        // 若是三段式，认为这是按钮配置：key=按钮显示文本=点击回复
        if (!label.empty())
        {
            s_buttonLabelMap[key] = label;
            s_buttonMap[key] = value;
            continue;
        }

        const std::wstring labelPrefixEn = L"button_text:";
        const std::wstring labelPrefixCn = L"按钮文本:";
        if (key.rfind(labelPrefixEn, 0) == 0)
        {
            s_buttonLabelMap[key.substr(labelPrefixEn.size())] = value;
            continue;
        }
        if (key.rfind(labelPrefixCn, 0) == 0)
        {
            s_buttonLabelMap[key.substr(labelPrefixCn.size())] = value;
            continue;
        }

        s_dialogMap[key] = value;
    }

    UpdateDialogMetadata(path);
}

static void MeasureTalkText(int maxWidth, int& outW, int& outH)
{
    HDC hdc = GetDC(nullptr);
    RECT rc = { 0, 0, maxWidth, 0 };
    if (s_talkFont)
        SelectObject(hdc, s_talkFont);
    DrawTextW(hdc, s_talkText.c_str(), -1, &rc, DT_CALCRECT | DT_WORDBREAK);
    ReleaseDC(nullptr, hdc);

    outW = (rc.right - rc.left) + kTalkPadding * 2;
    outH = (rc.bottom - rc.top) + kTalkPadding * 2;
    if (outW < 80) outW = 80;
    if (outH < 32) outH = 32;
}

static void PositionTalkWindow(int w, int h)
{
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    int x = g_pet.x + g_pet.w + 10;
    int y = g_pet.y + (g_pet.h - h) / 2;
    if (x + w > screenW)
    {
        x = g_pet.x - w - 10;
        s_talkOnRight = false;
    }
    else
    {
        s_talkOnRight = true;
    }
    if (x < 0) x = 0;
    if (y + h > screenH) y = screenH - h;
    if (y < 0) y = 0;

    SetWindowPos(s_hTalkWnd, HWND_TOPMOST, x, y, w, h, SWP_NOACTIVATE | SWP_SHOWWINDOW);
}

static void PositionTaskWindow(int w, int h)
{
    int x = g_pet.x;
    int y = g_pet.y - h - 10;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    SetWindowPos(s_hTaskWnd, HWND_TOPMOST, x, y, w, h, SWP_NOACTIVATE | SWP_SHOWWINDOW);
}

static bool IsAppWindow(HWND hwnd)
{
    if (!IsWindowVisible(hwnd) || IsIconic(hwnd))
        return false;

    wchar_t title[256];
    if (GetWindowTextW(hwnd, title, 256) <= 0)
        return false;

    wchar_t cls[256];
    GetClassNameW(hwnd, cls, 256);
    if (wcscmp(cls, L"PetWindow") == 0 ||
        wcscmp(cls, L"ChatInputWnd") == 0 ||
        wcscmp(cls, L"ChatButtonWnd") == 0 ||
        wcscmp(cls, L"ChatTalkWnd") == 0 ||
        wcscmp(cls, L"TaskListWnd") == 0)
        return false;

    LONG ex = GetWindowLongW(hwnd, GWL_EXSTYLE);
    if (ex & WS_EX_TOOLWINDOW)
        return false;

    return true;
}

static BOOL CALLBACK EnumAppWindows(HWND hwnd, LPARAM lParam)
{
    auto* list = reinterpret_cast<std::vector<std::wstring>*>(lParam);
    if (IsAppWindow(hwnd))
    {
        wchar_t title[256];
        GetWindowTextW(hwnd, title, 256);
        list->push_back(title);
    }
    return TRUE;
}

static void ClearTaskEntries()
{
    for (auto& e : s_taskEntries)
    {
        if (e.icon)
            DestroyIcon(e.icon);
    }
    s_taskEntries.clear();
    s_taskSelected = -1;
    if (s_hKillBtn)
        ShowWindow(s_hKillBtn, SW_HIDE);
}

static HICON GetWindowIconByPath(HWND hwnd)
{
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == 0)
        return nullptr;

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!hProcess)
        return nullptr;

    wchar_t path[MAX_PATH] = {};
    DWORD size = MAX_PATH;
    if (!QueryFullProcessImageNameW(hProcess, 0, path, &size))
    {
        CloseHandle(hProcess);
        return nullptr;
    }
    CloseHandle(hProcess);

    SHFILEINFOW sfi = {};
    if (SHGetFileInfoW(path, 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_SMALLICON))
        return sfi.hIcon;

    return nullptr;
}

static void BuildTaskListEntries()
{
    DWORD prevPid = s_taskSelectedPid;
    ClearTaskEntries();

    std::vector<HWND> hwnds;
    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
        auto* list = reinterpret_cast<std::vector<HWND>*>(lParam);
        if (IsAppWindow(hwnd))
            list->push_back(hwnd);
        return TRUE;
    }, reinterpret_cast<LPARAM>(&hwnds));

    std::set<DWORD> uniquePid;
    for (HWND hwnd : hwnds)
    {
        wchar_t title[256];
        if (GetWindowTextW(hwnd, title, 256) <= 0)
            continue;
        std::wstring t = title;

        DWORD pid = 0;
        GetWindowThreadProcessId(hwnd, &pid);
        if (pid == 0)
            continue;

        if (uniquePid.insert(pid).second)
        {
            TaskEntry entry;
            entry.title = t;
            entry.icon = GetWindowIconByPath(hwnd);
            entry.pid = pid;
            entry.processName = GetProcessNameByPid(pid);
            s_taskEntries.push_back(entry);
        }
    }

    if (prevPid != 0)
    {
        for (int i = 0; i < static_cast<int>(s_taskEntries.size()); ++i)
        {
            if (s_taskEntries[i].pid == prevPid)
            {
                s_taskSelected = i;
                s_taskSelectedPid = prevPid;
                return;
            }
        }
        s_taskSelected = -1;
        s_taskSelectedPid = 0;
        HideKillButton();
    }
}

static bool KillSelectedTask()
{
    if (s_taskSelected < 0 || s_taskSelected >= static_cast<int>(s_taskEntries.size()))
        return false;

    DWORD pid = s_taskEntries[s_taskSelected].pid;
    if (pid == 0 || pid == GetCurrentProcessId())
        return false;

    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!hProcess)
        return false;

    BOOL ok = TerminateProcess(hProcess, 0);
    CloseHandle(hProcess);
    return ok == TRUE;
}

static void HideKillButton()
{
    if (s_hKillBtn)
        ShowWindow(s_hKillBtn, SW_HIDE);
}

static void ShowKillButtonAt(int x, int y)
{
    if (!s_hKillBtn)
        return;

    const int btnW = 90;
    const int btnH = 26;
    SetWindowPos(s_hKillBtn, nullptr, x, y, btnW, btnH, SWP_NOACTIVATE | SWP_SHOWWINDOW);
    ShowWindow(s_hKillBtn, SW_SHOW);
}

static LRESULT CALLBACK TaskListProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_TIMER:
        if (wParam == kTaskRefreshTimer)
        {
            BuildTaskListEntries();
            InvalidateRect(hwnd, nullptr, TRUE);
        }
        return 0;
    case WM_LBUTTONDOWN:
        // 左键任意处收回任务管理器
        DestroyWindow(hwnd);
        return 0;
    case WM_RBUTTONDOWN:
    {
        int y = GET_Y_LPARAM(lParam);
        int index = (y - kTaskPadding) / kTaskLineH;
        if (index >= 0 && index < static_cast<int>(s_taskEntries.size()))
        {
            if (s_taskSelected == index)
            {
                s_taskSelected = -1;
                s_taskSelectedPid = 0;
                HideKillButton();
                InvalidateRect(hwnd, nullptr, TRUE);
                return 0;
            }

            s_taskSelected = index;
            s_taskSelectedPid = s_taskEntries[index].pid;
            InvalidateRect(hwnd, nullptr, TRUE);
            if (s_hKillBtn)
                EnableWindow(s_hKillBtn, TRUE);

            // 按钮显示在被选中项右对齐
            RECT rc;
            GetClientRect(hwnd, &rc);
            const int btnW = 90;
            const int btnH = 26;
            int itemTop = kTaskPadding + index * kTaskLineH;
            int bx = rc.right - kTaskPadding - btnW;
            int by = itemTop + (kTaskLineH - btnH) / 2;
            ShowKillButtonAt(bx, by);
        }
        else
        {
            s_taskSelected = -1;
            HideKillButton();
        }
        return 0;
    }
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);
        HBRUSH brush = CreateSolidBrush(RGB(255, 240, 245));
        FillRect(hdc, &rc, brush);
        DeleteObject(brush);

        if (s_inputFont)
            SelectObject(hdc, s_inputFont);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(50, 50, 50));

        int x = kTaskPadding;
        int y = kTaskPadding;
        for (int i = 0; i < static_cast<int>(s_taskEntries.size()); ++i)
        {
            const auto& e = s_taskEntries[i];
            if (i == s_taskSelected)
            {
                RECT sel = { x - 4, y - 2, rc.right - kTaskPadding, y + kTaskLineH };
                HBRUSH selBrush = CreateSolidBrush(RGB(255, 220, 230));
                FillRect(hdc, &sel, selBrush);
                DeleteObject(selBrush);
            }
            if (e.icon)
                DrawIconEx(hdc, x, y + 4, e.icon, kTaskIcon, kTaskIcon, 0, nullptr, DI_NORMAL);

            RECT trc = { x + kTaskIcon + 8, y, rc.right - kTaskPadding, y + kTaskLineH };
            DrawTextW(hdc, e.title.c_str(), -1, &trc, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);

            y += kTaskLineH;
            if (y + kTaskLineH > rc.bottom)
                break;
        }
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_DESTROY:
        KillTimer(hwnd, kTaskRefreshTimer);
        ClearTaskEntries();
        s_hKillBtn = nullptr;
        s_hTaskWnd = nullptr;
        return 0;
    case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == 1)
        {
            if (MessageBoxW(hwnd, L"确定要强行结束该进程吗？", L"清理进程", MB_OKCANCEL | MB_ICONWARNING) == IDOK)
                KillSelectedTask();
        }
        return 0;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}
static LRESULT CALLBACK TalkProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_TIMER:
        if (wParam == kTalkAutoHideTimer)
        {
            KillTimer(hwnd, kTalkAutoHideTimer);
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    case WM_LBUTTONDOWN:
        DestroyWindow(hwnd);
        return 0;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);
        HBRUSH brush = CreateSolidBrush(RGB(255, 250, 230));
        HPEN pen = CreatePen(PS_SOLID, 1, RGB(220, 200, 170));
        HGDIOBJ oldBrush = SelectObject(hdc, brush);
        HGDIOBJ oldPen = SelectObject(hdc, pen);

        // 圆角气泡
        RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, kTalkCorner, kTalkCorner);

        // 尖角（朝向桌宠）
        POINT tail[3];
        if (s_talkOnRight)
        {
            tail[0] = { rc.left, rc.top + (rc.bottom - rc.top) / 2 - kTalkTail };
            tail[1] = { rc.left - kTalkTail, rc.top + (rc.bottom - rc.top) / 2 };
            tail[2] = { rc.left, rc.top + (rc.bottom - rc.top) / 2 + kTalkTail };
        }
        else
        {
            tail[0] = { rc.right, rc.top + (rc.bottom - rc.top) / 2 - kTalkTail };
            tail[1] = { rc.right + kTalkTail, rc.top + (rc.bottom - rc.top) / 2 };
            tail[2] = { rc.right, rc.top + (rc.bottom - rc.top) / 2 + kTalkTail };
        }
        Polygon(hdc, tail, 3);

        SelectObject(hdc, oldBrush);
        SelectObject(hdc, oldPen);
        DeleteObject(brush);
        DeleteObject(pen);

        if (s_talkFont)
            SelectObject(hdc, s_talkFont);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(50, 50, 50));

        RECT textRc = rc;
        textRc.left += kTalkPadding;
        textRc.top += kTalkPadding;
        textRc.right -= kTalkPadding;
        textRc.bottom -= kTalkPadding;
        DrawTextW(hdc, s_talkText.c_str(), -1, &textRc, DT_WORDBREAK);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_DESTROY:
        KillTimer(hwnd, kTalkAutoHideTimer);
        s_hTalkWnd = nullptr;
        return 0;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// 对话输出
void ChatTalk(HWND hwnd, const wchar_t* text)
{
    if (hwnd)
        s_mainHwnd = hwnd;
    s_talkText = text ? text : L"";

    if (!s_talkFont)
    {
        s_talkFont = CreateFontW(
            18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
            L"Microsoft YaHei UI");
    }

    WNDCLASSW wc = {};
    wc.lpfnWndProc = TalkProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"ChatTalkWnd";
    static bool registered = false;
    if (!registered)
    {
        RegisterClassW(&wc);
        registered = true;
    }

    if (!s_hTalkWnd)
    {
        s_hTalkWnd = CreateWindowExW(
            WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
            wc.lpszClassName,
            nullptr,
            WS_POPUP,
            0, 0, 10, 10,
            hwnd,
            nullptr,
            GetModuleHandle(NULL),
            nullptr
        );
        SetLayeredWindowAttributes(s_hTalkWnd, 0, 235, LWA_ALPHA);
    }

    int w = 0, h = 0;
    MeasureTalkText(kTalkMaxWidth, w, h);
    PositionTalkWindow(w, h);
    ShowWindow(s_hTalkWnd, SW_SHOW);
    InvalidateRect(s_hTalkWnd, nullptr, TRUE);
    SetTimer(s_hTalkWnd, kTalkAutoHideTimer, kTalkAutoHideMs, nullptr);
}


static void ChatShowTaskList(HWND hwndParent)
{
    if (!s_inputFont)
    {
        s_inputFont = CreateFontW(
            20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
            L"Microsoft YaHei UI");
    }

    BuildTaskListEntries();

    WNDCLASSW wc = {};
    wc.lpfnWndProc = TaskListProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"TaskListWnd";
    static bool registered = false;
    if (!registered)
    {
        RegisterClassW(&wc);
        registered = true;
    }

    if (!s_hTaskWnd)
    {
        const int width = GetInputWidth();
        const int height = kTaskHeight;
        s_hTaskWnd = CreateWindowExW(
            WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
            wc.lpszClassName,
            nullptr,
            WS_POPUP,
            0, 0, width, height,
            hwndParent,
            nullptr,
            GetModuleHandle(NULL),
            nullptr
        );
        SetLayeredWindowAttributes(s_hTaskWnd, 0, 220, LWA_ALPHA);
    }

    if (!s_hKillBtn)
    {
        s_hKillBtn = CreateWindowW(L"BUTTON", L"清理",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            0, 0, 90, 26,
            s_hTaskWnd, (HMENU)1, GetModuleHandle(NULL), nullptr);
        if (s_inputFont)
            SendMessageW(s_hKillBtn, WM_SETFONT, (WPARAM)s_inputFont, TRUE);
        EnableWindow(s_hKillBtn, FALSE);
        ShowWindow(s_hKillBtn, SW_HIDE);
    }
    else
    {
        EnableWindow(s_hKillBtn, FALSE);
        ShowWindow(s_hKillBtn, SW_HIDE);
    }

    ChatUpdateTaskListPosition();
    ShowWindow(s_hTaskWnd, SW_SHOW);
    InvalidateRect(s_hTaskWnd, nullptr, TRUE);
    int refreshSec = ReadSettingsInt(L"任务刷新秒", 5);
    if (refreshSec < 1) refreshSec = 1;
    if (refreshSec > 60) refreshSec = 60;
    SetTimer(s_hTaskWnd, kTaskRefreshTimer, refreshSec * 1000, nullptr);
}




















































































































































































































































































































































































// 文字输入处理逻辑
void ChatHandleInput(HWND hwnd, const std::wstring& input)
{
    ChatRecordInteraction();

    if (s_dialogMap.empty() || DialogFileChanged())
        LoadDialogConfig();

    const std::wstring normalized = Trim(input);
    auto it = s_dialogMap.find(normalized);

    if (IsTaskListTrigger(normalized))
    {
        ChatShowTaskList(hwnd);
        return;
    }

    // 特殊关键词：我爱你
    if (normalized == L"我爱你")
    {
        if (it != s_dialogMap.end())
            ChatTalk(hwnd, it->second.c_str());
        else
            ChatTalk(hwnd, L"要结束病娇游戏吗？"); // 未配置输出语句时的默认输出

        // 弹出按钮输入
        ChatShowButtonInput(hwnd, L"我爱你_1", L"我爱你_2");
        return;
    }

    // 普通输入
    if (it != s_dialogMap.end())
        ChatTalk(hwnd, it->second.c_str());
    else
    {
        auto def = s_dialogMap.find(L"默认");
        if (def != s_dialogMap.end())
            ChatTalk(hwnd, def->second.c_str());
        else
            ChatTalk(hwnd, L"……我不太明白你在说什么。");
    }
}

// 按钮输入处理逻辑
static void ChatHandleButtonInput(HWND buttonWnd, const std::wstring& key)
{
    ChatRecordInteraction();

    if (s_buttonMap.empty())
        LoadDialogConfig();

    if (!s_mainHwnd)
        s_mainHwnd = GetParent(buttonWnd);

    auto it = s_buttonMap.find(key);
    if (it != s_buttonMap.end())
        ChatTalk(GetParent(buttonWnd), it->second.c_str());

    DestroyWindow(buttonWnd);
    s_hButtonWnd = nullptr;

    // 点击按钮后：先显示文本，确认后延时退出进程
    if (key == L"我爱你_2")
    {
        if (s_quitTimer)
        {
            DeleteTimerQueueTimer(nullptr, s_quitTimer, nullptr);
            s_quitTimer = nullptr;
        }
        CreateTimerQueueTimer(&s_quitTimer, nullptr, QuitTimerProc, nullptr,
            kQuitDelayMs, 0, WT_EXECUTEDEFAULT);
    }
}

static std::wstring GetButtonLabel(const std::wstring& key, const std::wstring& fallback)
{
    auto it = s_buttonLabelMap.find(key);
    return (it != s_buttonLabelMap.end()) ? it->second : fallback;
}

// 文字输入窗口过程
static LRESULT CALLBACK TextInputProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static bool flash = false;

    switch (msg)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);
        HBRUSH hBrush = CreateSolidBrush(flash ? RGB(255, 200, 220) : RGB(255, 240, 245));
        FillRect(hdc, &rc, hBrush);
        DeleteObject(hBrush);

        if (s_inputFont)
            SelectObject(hdc, s_inputFont);
        SetBkMode(hdc, TRANSPARENT);
        std::wstring display = s_inputText + s_imeText;
        DrawTextW(hdc, display.c_str(), -1, &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        EndPaint(hwnd, &ps);

        flash = false;
        break;
    }
    case WM_LBUTTONDOWN:
        flash = true;
        InvalidateRect(hwnd, nullptr, TRUE);
        s_dragOffset.x = LOWORD(lParam);
        s_dragOffset.y = HIWORD(lParam);
        s_dragging = true;
        SetCapture(hwnd);
        break;
    case WM_LBUTTONUP:
        s_dragging = false;
        ReleaseCapture();
        break;
    case WM_MOUSEMOVE:
        if (s_dragging)
        {
            POINT pt = { LOWORD(lParam), HIWORD(lParam) };
            ClientToScreen(hwnd, &pt);
            SetWindowPos(hwnd, HWND_TOPMOST, pt.x - s_dragOffset.x, pt.y - s_dragOffset.y,
                0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
        }
        break;
    case WM_CHAR:
        if (wParam == VK_RETURN)
        {
            // 回车提交前，把输入法组合串并入正式文本
            if (!s_imeText.empty())
            {
                s_inputText += s_imeText;
                s_imeText.clear();
            }
            DestroyWindow(hwnd);
        }
        else if (wParam == VK_BACK)
        {
            if (!s_inputText.empty())
                s_inputText.pop_back();
            InvalidateRect(hwnd, nullptr, TRUE);
        }
        else
        {
            s_inputText.push_back((wchar_t)wParam);
            InvalidateRect(hwnd, nullptr, TRUE);
        }
        break;
    case WM_IME_COMPOSITION:
    {
        HIMC hImc = ImmGetContext(hwnd);
        if (hImc)
        {
            // 处理输入法的临时组合字符串
            if (lParam & GCS_COMPSTR)
            {
                LONG len = ImmGetCompositionStringW(hImc, GCS_COMPSTR, nullptr, 0);
                if (len > 0)
                {
                    std::wstring buf(len / sizeof(wchar_t), L'\0');
                    ImmGetCompositionStringW(hImc, GCS_COMPSTR, &buf[0], len);
                    s_imeText = buf;
                }
            }

            // 处理已提交的结果字符串
            if (lParam & GCS_RESULTSTR)
            {
                LONG len = ImmGetCompositionStringW(hImc, GCS_RESULTSTR, nullptr, 0);
                if (len > 0)
                {
                    std::wstring buf(len / sizeof(wchar_t), L'\0');
                    ImmGetCompositionStringW(hImc, GCS_RESULTSTR, &buf[0], len);
                    s_inputText += buf;
                }
                s_imeText.clear();
            }
            ImmReleaseContext(hwnd, hImc);
        }
        InvalidateRect(hwnd, nullptr, TRUE);
        return 0;
    }
    case WM_DESTROY:
        if (!s_inputText.empty())
        {
            ChatHandleInput(GetParent(hwnd), s_inputText);
        }
        s_hInputWnd = nullptr;
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// 按钮输入窗口过程
static LRESULT CALLBACK ButtonInputProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);
        HBRUSH hBrush = CreateSolidBrush(RGB(255, 240, 245));
        FillRect(hdc, &rc, hBrush);
        DeleteObject(hBrush);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED)
        {
            int id = LOWORD(wParam);
            std::wstring key = (id == 1) ? s_buttonKey1 : s_buttonKey2;
            ChatHandleButtonInput(hwnd, key);
        }
        break;
    case WM_DESTROY:
        s_hButtonWnd = nullptr;
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// 显示文字输入窗口
void ChatShowInput(HWND hwndParent)
{
    // 已展开则收起（右键切换展开/收起）
    if (s_hInputWnd)
    {
        DestroyWindow(s_hInputWnd);
        s_hInputWnd = nullptr;
        return;
    }

    const int width = GetInputWidth();
    const int height = kInputHeight;
    s_inputText.clear();

    WNDCLASSW wc = {};
    wc.lpfnWndProc = TextInputProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"ChatInputWnd";
    static bool registered = false;
    if (!registered)
    {
        RegisterClassW(&wc);
        registered = true;
    }

    s_hInputWnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
        wc.lpszClassName,
        nullptr,
        WS_POPUP,
        150, 150,
        width, height,
        hwndParent,
        nullptr,
        GetModuleHandle(NULL),
        nullptr
    );
    if (!s_hInputWnd) return;
    if (!s_inputFont)
    {
        s_inputFont = CreateFontW(
            20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
            L"Microsoft YaHei UI");
    }
    if (s_inputFont)
        SendMessageW(s_hInputWnd, WM_SETFONT, (WPARAM)s_inputFont, TRUE);
    SetLayeredWindowAttributes(s_hInputWnd, 0, 220, LWA_ALPHA);
    ShowWindow(s_hInputWnd, SW_SHOW);
    SetFocus(s_hInputWnd);

    ChatUpdateInputPosition();
}

void ChatUpdateInputPosition()
{
    if (!s_hInputWnd)
        return;

    int x = 0, y = 0, w = 0, h = 0;
    GetInputRect(x, y, w, h);
    SetWindowPos(s_hInputWnd, HWND_TOPMOST, x, y, w, h,
        SWP_NOACTIVATE | SWP_SHOWWINDOW);
}

void ChatUpdateButtonPosition()
{
    if (!s_hButtonWnd)
        return;

    int x = 0, y = 0, w = 0, h = 0;
    GetButtonRect(x, y, w, h);
    SetWindowPos(s_hButtonWnd, HWND_TOPMOST, x, y, w, h,
        SWP_NOACTIVATE | SWP_SHOWWINDOW);
}

void ChatUpdateTalkPosition()
{
    if (!s_hTalkWnd)
        return;

    int w = 0, h = 0;
    MeasureTalkText(kTalkMaxWidth, w, h);
    PositionTalkWindow(w, h);
}

void ChatUpdateTaskListPosition()
{
    if (!s_hTaskWnd)
        return;

    const int width = GetInputWidth();
    const int height = kTaskHeight;
    PositionTaskWindow(width, height);
}

void ChatRecordInteraction()
{
    EnsureStateLoaded();
    s_state.lastInteraction = GetUnixTimeSeconds();
}

void ChatTickIdleCheck(HWND hwnd)
{
    EnsureStateLoaded();

    // 允许外部手动修改 valence / arousal
    ChatState fileState = s_state;
    if (TryLoadState(fileState))
    {
        s_state.valence = fileState.valence;
        s_state.arousal = fileState.arousal;
        s_state.lastInteraction = fileState.lastInteraction;
    }

    const long long now = GetUnixTimeSeconds();
    UpdateMonitorState();
    if (CheckGameKeywords(hwnd))
    {
        s_state.lastInteraction = now;
        return;
    }

    const int valence = s_state.valence;
    const int arousal = s_state.arousal;
    int thresholdMinutes = (15 - arousal) * (15 - valence);
    if (thresholdMinutes < 0)
        thresholdMinutes = 0;
    const long long thresholdSeconds = static_cast<long long>(thresholdMinutes) * 60;

    if (now - s_state.lastInteraction < thresholdSeconds)
        return;

    const int hour = GetLocalHour();
    const std::wstring overrideKey = GetIdleOverrideKeyForHour(hour);
    if (!overrideKey.empty())
    {
        std::wstring text;
        std::wstring keyUsed;
        if (GetIdleTextByKey(overrideKey, text, keyUsed))
        {
            ChatTalk(hwnd, text.c_str());
            PlayIdleSound(keyUsed);
            TryAppendDiaryForKey(keyUsed);
            s_state.lastInteraction = now;
            return;
        }
    }

    const bool sleepMode = (s_monitorState == 1 && IsSleepHour(hour));
    IdleEntry idle;
    if (sleepMode)
    {
        if (!GetRandomVariantLine(L"sleep", 5, idle))
            return;
    }
    else
    {
        if (!GetRandomVariantLine(L"idle", 5, idle))
            return;
    }

    ChatTalk(hwnd, idle.text.c_str());
    PlayIdleSound(idle.key);
    TryAppendDiaryForKey(idle.key);

    if (sleepMode)
    {
        std::map<std::wstring, std::wstring> gameMap;
        LoadKeywordMap(L"monitor_game.txt", gameMap);
        if (!gameMap.empty())
        {
            int ret = MessageBoxW(hwnd,
                L"现在已是凌晨，是否关闭游戏进程？",
                L"提示",
                MB_YESNO | MB_ICONQUESTION | MB_TOPMOST);
            if (ret == IDYES)
                KillProcessesByKeywords(gameMap);
        }
    }

    // 避免每分钟重复触发：触发后视作“被动互动”刷新时间戳
    s_state.lastInteraction = now;
}

void ChatGetStateSnapshot(long long& lastInteraction, int& valence, int& arousal)
{
    EnsureStateLoaded();
    lastInteraction = s_state.lastInteraction;
    valence = s_state.valence;
    arousal = s_state.arousal;
}

// 显示按钮输入窗口
void ChatShowButtonInput(HWND hwndParent, const std::wstring& key1, const std::wstring& key2)
{
    const int width = GetInputWidth();
    const int height = kButtonHeight;

    s_buttonKey1 = key1;
    s_buttonKey2 = key2;

    WNDCLASSW wc = {};
    wc.lpfnWndProc = ButtonInputProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"ChatButtonWnd";
    static bool registered = false;
    if (!registered)
    {
        RegisterClassW(&wc);
        registered = true;
    }

    s_hButtonWnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
        wc.lpszClassName,
        nullptr,
        WS_POPUP,
        0, 0,
        width, height,
        hwndParent,
        nullptr,
        GetModuleHandle(NULL),
        nullptr
    );

    if (!s_hButtonWnd) return;
    SetLayeredWindowAttributes(s_hButtonWnd, 0, 230, LWA_ALPHA);

    const std::wstring label1 = GetButtonLabel(key1, key1);
    const std::wstring label2 = GetButtonLabel(key2, key2);

    const int btnW = width - 20 * 2;
    const int btnH = 28;
    HWND btn1 = CreateWindowW(L"BUTTON", label1.c_str(), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        20, 10, btnW, btnH, s_hButtonWnd, (HMENU)1, GetModuleHandle(NULL), nullptr);
    HWND btn2 = CreateWindowW(L"BUTTON", label2.c_str(), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        20, 10 + btnH + 12, btnW, btnH, s_hButtonWnd, (HMENU)2, GetModuleHandle(NULL), nullptr);

    if (s_inputFont)
    {
        SendMessageW(btn1, WM_SETFONT, (WPARAM)s_inputFont, TRUE);
        SendMessageW(btn2, WM_SETFONT, (WPARAM)s_inputFont, TRUE);
    }

    ShowWindow(s_hButtonWnd, SW_SHOW);
    ChatUpdateButtonPosition();
}
