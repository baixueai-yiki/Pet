#include <windows.h>
#include <windowsx.h>
#include <imm.h>
#include "ChatComponent.h"
#include "AudioComponent.h"
#include "DiaryComponent.h"
#include "../PetActor.h"
// Core
#include "Core/Path.h"
#include "Core/TextFile.h"
// Runtime
#include "Runtime/Scheduler.h"
#include "Runtime/StateManager.h"
// Engine
#include "Engine/Input/InputDispatcher.h"
// UI
#include "Systems/UI/UIPanels/ChatPanel/BubbleChatPanel.h"
#include "Systems/UI/UIPanels/ChatPanel/InputChatPanel.h"
#include "Systems/UI/UIPanels/ChatPanel/OptionChatPanel.h"
#include "Systems/UI/UIPanels/ToolPanel/SettingToolPanel.h"
#include "Systems/UI/UIPanels/ToolPanel/TaskToolPanel.h"
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
#include <cwctype>
#include <tlhelp32.h>
static std::map<std::wstring, std::wstring> s_dialogMap;
static std::map<std::wstring, std::wstring> s_buttonMap;
static std::map<std::wstring, std::wstring> s_buttonLabelMap;
static std::wstring s_dialogPath;
static FILETIME s_dialogWriteTime = {};
static bool s_hasDialogTime = false;
struct TaskEntry
{
    std::wstring title;
    HICON icon;
    DWORD pid;
    std::wstring processName;
};
static std::vector<TaskEntry> s_taskEntries;
static std::set<std::wstring> s_gameActiveKeys;
static std::set<std::wstring> s_gameActiveGameKeys;
static std::set<std::wstring> s_gameActiveWorkKeys;
static int s_monitorState = 0;
static std::set<std::wstring> s_diaryLoggedCategories;
static std::map<std::wstring, int> s_diaryCategoryCounts;
static HANDLE s_quitTimer = nullptr;
static const DWORD kQuitDelayMs = 5000;
static HWND s_mainHwnd = nullptr;
static bool s_sleepPromptActive = false;
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

// 鍓嶇疆澹版槑
static void HideKillButton();
static bool IsAppWindow(HWND hwnd);
void ChatTalk(HWND hwnd, const wchar_t* text, int durationMs);
static void BuildTaskListEntries();

static std::wstring Trim(const std::wstring& text);

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
    if (!TextFile::ReadText(GetStatePath(), text))
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

bool LoadIdleMap(std::map<std::wstring, std::wstring>& mapOut)
{
    // \u5148\u5c1d\u8bd5 JSON\uff0c\u518d\u56de\u9000 txt
    std::wstring text;
    if (!TextFile::ReadText(GetAssetPath(L"chat\\chat_idle.json"), text))
    {
        if (!TextFile::ReadText(GetAssetPath(L"chat\\chat_dialogue.json"), text))
            return false;
    }

    mapOut.clear();
    // JSON \u89e3\u6790\uff08\u4e3b\u6587\u4ef6 chat_idle.json\uff09
    {
        size_t pos = 0;
        while (pos < text.size()) {
            pos = text.find(L'"', pos); if (pos == std::wstring::npos) break;
            size_t keyEnd = text.find(L'"', pos + 1); if (keyEnd == std::wstring::npos) break;
            std::wstring key = text.substr(pos + 1, keyEnd - pos - 1); pos = keyEnd + 1;
            pos = text.find(L':', pos); if (pos == std::wstring::npos) break; pos++;
            while (pos < text.size() && iswspace(text[pos])) pos++;
            if (pos >= text.size()) break;
            if (text[pos] == L'"') {
                size_t valEnd = text.find(L'"', pos + 1); if (valEnd == std::wstring::npos) break;
                mapOut[key] = text.substr(pos + 1, valEnd - pos - 1); pos = valEnd + 1;
            } else if (text[pos] == L'{') {
                size_t objEnd = text.find(L'}', pos); if (objEnd == std::wstring::npos) break;
                std::wstring obj = text.substr(pos + 1, objEnd - pos - 1); pos = objEnd + 1;
                std::wstring valText, valMood, valAction;
                size_t tp = obj.find(L"\"text\""); if (tp != std::wstring::npos) {
                    tp = obj.find(L'"', tp + 6); if (tp != std::wstring::npos) {
                        size_t te = obj.find(L'"', tp + 1); if (te != std::wstring::npos)
                            valText = obj.substr(tp + 1, te - tp - 1);
                    } }
                size_t mp = obj.find(L"\"mood\""); if (mp != std::wstring::npos) {
                    mp = obj.find(L'"', mp + 6); if (mp != std::wstring::npos) {
                        size_t me = obj.find(L'"', mp + 1); if (me != std::wstring::npos)
                            valMood = obj.substr(mp + 1, me - mp - 1);
                    } }
                size_t ap = obj.find(L"\"action\""); if (ap != std::wstring::npos) {
                    ap = obj.find(L'"', ap + 8); if (ap != std::wstring::npos) {
                        size_t ae = obj.find(L'"', ap + 1); if (ae != std::wstring::npos)
                            valAction = obj.substr(ap + 1, ae - ap - 1);
                    } }
                mapOut[key] = valText + (valMood.empty() ? L"" : L"\uff0c" + valMood)
                                        + (valAction.empty() ? L"" : L"|" + valAction);
            } else continue;
        }
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
        std::wstring key = L"chat_idle_" + std::to_wstring(i);
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
    if (!TextFile::ReadLines(path, lines))
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
    // 鍏堣 diary_script.txt
    LoadDiaryMapFromFile(L"diary_script.txt", out);
    // 鍐嶈 monitor_game.txt锛堝厑璁告妸 diary_ 鍐欏湪杩欓噷锛?    LoadDiaryMapFromFile(L"monitor_game.txt", out);
    return !out.empty();
}

static std::wstring GetDiaryCategoryForKey(const std::wstring& key)
{
    if (key.rfind(L"chat_idle_", 0) == 0)
        return L"idle";
    if (key.rfind(L"chat_sleep_", 0) == 0)
        return L"sleep";
    if (key.rfind(L"chat_morning_", 0) == 0 ||
        key.rfind(L"chat_lunch_", 0) == 0 ||
        key.rfind(L"chat_dinner_", 0) == 0 ||
        key.rfind(L"chat_night_", 0) == 0 ||
        key.rfind(L"chat_afternoon_", 0) == 0)
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

    EventEmit(L"diary.append", it->second);
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

    EventEmit(L"diary.append", it->second);
    ++count;
}

bool GetIdleTextByKey(const std::wstring& key, std::wstring& out, std::wstring& keyUsed)
{
    if (key.empty())
        return false;
    std::map<std::wstring, std::wstring> map;
    if (!LoadIdleMap(map))
        return false;

    std::vector<std::wstring> candidates;
    for (int i = 1; i <= 10; ++i)
    {
        std::wstring k = L"chat_" + key + L"_" + std::to_wstring(i);
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

    auto it = map.find(L"chat_" + key);
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
    AudioComponent::PlayAudioAuto(L"audio\\" + key);
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
    if (!TextFile::ReadLines(GetAssetPath(L"chat\\" + file), lines))
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

static void UpdateMonitorState()
{
    // 鐢ㄥ綋鍓嶄换鍔″垪琛ㄧ殑杩涚▼鍚嶅仛鍖归厤锛堝彲瑙佺獥鍙ｅ搴旂殑杩涚▼锛?    BuildTaskListEntries();
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

struct SleepPromptContext
{
    HWND hwnd;
    std::map<std::wstring, std::wstring> gameMap;
};

static DWORD WINAPI SleepPromptThread(LPVOID param)
{
    SleepPromptContext* ctx = static_cast<SleepPromptContext*>(param);
    if (!ctx)
    {
        s_sleepPromptActive = false;
        return 0;
    }

    int ret = MessageBoxW(ctx->hwnd,
        L"鐜板湪宸叉槸鍑屾櫒锛屾槸鍚﹀叧闂父鎴忚繘绋嬶紵",
        L"鎻愮ず",
        MB_YESNO | MB_ICONQUESTION | MB_TOPMOST);
    if (ret == IDYES)
        KillProcessesByKeywords(ctx->gameMap);

    s_sleepPromptActive = false;
    delete ctx;
    return 0;
}

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
        // work/game 鍏辩敤涓€娆￠厤棰濓紝life 鍗曠嫭涓ゆ閰嶉
        if (matchedCategory == L"work" || matchedCategory == L"game")
            TryAppendDiaryForKeyword(matchedKey, L"work_game", 1);
        else
            TryAppendDiaryForKeyword(matchedKey, L"life", 2);
        return true;
    }
    return false;
}

static std::wstring GetIdleOverrideKeyForHour(int hour)
{
    const int morning   = Setting::GetInt(L"早安时间", 7);
    const int lunch     = Setting::GetInt(L"午餐时间", 12);
    const int afternoon = Setting::GetInt(L"下午时间", 15);
    const int dinner    = Setting::GetInt(L"晚餐时间", 18);
    const int night     = Setting::GetInt(L"晚安时间", 22);

    if (hour == morning)   return L"morning";
    if (hour == lunch)     return L"lunch";
    if (hour == afternoon) return L"afternoon";
    if (hour == dinner)    return L"dinner";
    if (hour == night)     return L"night";
    return L"";
}

bool ChatTryIdleLine(HWND hwnd)
{
    int hour = GetLocalHour();
    std::wstring key = GetIdleOverrideKeyForHour(hour);
    if (key.empty())
        key = L"idle";   // 非特殊时间用闲言（sleep 时段暂不使用）

    std::wstring text, keyUsed;
    if (!GetIdleTextByKey(key, text, keyUsed))
        return false;

    ChatTalk(hwnd, text.c_str());
    ChatRecordInteraction();

    // 播放 action 指定的音频
    {
        std::map<std::wstring, std::wstring> map;
        if (LoadIdleMap(map)) {
            auto it = map.find(keyUsed);
            if (it != map.end()) {
                size_t actPos = it->second.find(L'|');
                if (actPos != std::wstring::npos) {
                    std::wstring act = it->second.substr(actPos + 1);
                    AudioComponent::PlayAudioAsset(L"audio\\" + act);
                }
    }}}

    // 尝试记录日记
    TryAppendDiaryForKey(keyUsed);

    // 补充语句：概率 = arousal% * 2
    int arousal = PetGetArousal();
    if (!s_idleSeeded)
    {
        s_idleSeeded = true;
        srand(static_cast<unsigned int>(GetTickCount()));
    }
    if ((rand() % 100) < (arousal * 2))
    {
        // 补充语句也尝试从 idle 里取第二轮
        std::wstring followText, followKey;
        if (GetIdleTextByKey(key, followText, followKey))
            ChatTalk(hwnd, followText.c_str());
    }
    return true;
}

bool ChatTrySleepLine(HWND hwnd)
{
    std::wstring text, keyUsed;
    if (!GetIdleTextByKey(L"sleep", text, keyUsed))
    {
        // 兜底：chat_idle.txt 没有 sleep_ 条目时用默认语句
        text = L"晚安……该休息了哦。";
        keyUsed = L"chat_sleep";
    }

    ChatTalk(hwnd, text.c_str());
    ChatRecordInteraction();

    {
        std::map<std::wstring, std::wstring> map;
        if (LoadIdleMap(map)) {
            auto it = map.find(keyUsed);
            if (it != map.end()) {
                size_t actPos = it->second.find(L'|');
                if (actPos != std::wstring::npos) {
                    std::wstring act = it->second.substr(actPos + 1);
                    AudioComponent::PlayAudioAsset(L"audio\\" + act);
                }
    }}}

    TryAppendDiaryForKey(keyUsed);

    int delta = (rand() % 2) ? 1 : -1;
    PetCycleAdvanceDay(delta);
    return true;
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

// 閰嶇疆鏂囦欢璇诲彇
	static void LoadDialogConfig()
	{
	    s_dialogMap.clear();
	    s_buttonMap.clear();
	    s_buttonLabelMap.clear();
	    std::wstring text; std::wstring path;
	    if (TextFile::ReadText(GetAssetPath(L"chat\\chat_pet.json"), text))
	        path = GetAssetPath(L"chat\\chat_pet.json");
	    else if (TextFile::ReadText(GetAssetPath(L"chat\\chat_safeword.txt"), text))
	        path = GetAssetPath(L"chat\\chat_safeword.txt");
	    else return;
    // JSON 解析
    size_t p = 0;
    while (p < text.size()) {
        p = text.find(L'"', p); if (p == std::wstring::npos) break;
        size_t ke = text.find(L'"', p + 1); if (ke == std::wstring::npos) break;
        std::wstring key = text.substr(p + 1, ke - p - 1); p = ke + 1;
        p = text.find(L':', p); if (p == std::wstring::npos) break;
        p++; while (p < text.size() && iswspace(text[p])) p++;
        if (p >= text.size()) break;
        std::wstring vt, vl, l1, l2, action;
        if (text[p] == L'{') {
            size_t oe = text.find(L'}', p); if (oe == std::wstring::npos) break;
            std::wstring obj = text.substr(p + 1, oe - p - 1); p = oe + 1;
            // "text"
            size_t tp = obj.find(L"\"text\""); if (tp != std::wstring::npos) {
                tp = obj.find(L'"', tp + 6); if (tp != std::wstring::npos) {
                    size_t te = obj.find(L'"', tp + 1); if (te != std::wstring::npos)
                        vt = obj.substr(tp + 1, te - tp - 1);
                } }
            // "label"
            size_t lp = obj.find(L"\"label\""); if (lp != std::wstring::npos) {
                lp = obj.find(L'"', lp + 7); if (lp != std::wstring::npos) {
                    size_t le = obj.find(L'"', lp + 1); if (le != std::wstring::npos)
                        vl = obj.substr(lp + 1, le - lp - 1);
                } }
            // "label1":  → 找冒号后的引号跳过 ":
            size_t l1p = obj.find(L"\"label1\""); if (l1p != std::wstring::npos) {
                l1p = obj.find(L':', l1p + 7); if (l1p != std::wstring::npos) {
                    l1p++; while (l1p < obj.size() && obj[l1p] != L'"') l1p++;
                    if (l1p < obj.size()) {
                        size_t l1e = obj.find(L'"', l1p + 1); if (l1e != std::wstring::npos)
                            l1 = obj.substr(l1p + 1, l1e - l1p - 1);
                }}}
            // "action":
            size_t ap = obj.find(L"\"action\""); if (ap != std::wstring::npos) {
                ap = obj.find(L':', ap + 8); if (ap != std::wstring::npos) {
                    ap++; while (ap < obj.size() && obj[ap] != L'"') ap++;
                    if (ap < obj.size()) {
                        size_t ae = obj.find(L'"', ap + 1); if (ae != std::wstring::npos)
                            action = obj.substr(ap + 1, ae - ap - 1);
                }}}
            // "label2":
            size_t l2p = obj.find(L"\"label2\""); if (l2p != std::wstring::npos) {
                l2p = obj.find(L':', l2p + 7); if (l2p != std::wstring::npos) {
                    l2p++; while (l2p < obj.size() && obj[l2p] != L'"') l2p++;
                    if (l2p < obj.size()) {
                        size_t l2e = obj.find(L'"', l2p + 1); if (l2e != std::wstring::npos)
                            l2 = obj.substr(l2p + 1, l2e - l2p - 1);
                }}}
        } else if (text[p] == L'"') {
            size_t ve = text.find(L'"', p + 1); if (ve == std::wstring::npos) break;
            vt = text.substr(p + 1, ve - p - 1); p = ve + 1;
        } else continue;
        if (key.empty()) continue;
        s_dialogMap[key] = vt + (action.empty() ? L"" : L"\x01" + action);
        if (!vl.empty())  { s_buttonLabelMap[key] = vl; s_buttonMap[key] = vt; }
        if (!l1.empty()) { s_buttonLabelMap[key] = l1; }
        if (!l2.empty()) { s_buttonMap[key] = l2; }
    }
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
}

// 瀵硅瘽杈撳嚭
void ChatTalk(HWND hwnd, const wchar_t* text, int durationMs)
{
    BubbleChatPanel::Show(hwnd, text, durationMs);
}


// 鏂囧瓧杈撳叆澶勭悊閫昏緫
void ChatEnsureDialogLoaded()
{
    if (s_dialogMap.empty() || DialogFileChanged())
        LoadDialogConfig();
}

static std::wstring s_lastMatchKey;

const std::wstring* ChatLookupDialog(const std::wstring& input)
{
    ChatEnsureDialogLoaded();
    s_lastMatchKey.clear();
    auto it = s_dialogMap.find(input);
    if (it != s_dialogMap.end()) { s_lastMatchKey = input; return &it->second; }
    // 关键词模糊匹配
    std::vector<std::pair<const std::wstring*, std::wstring>> matches;
    for (const auto& kv : s_dialogMap) {
        if (!kv.first.empty() && input.find(kv.first) != std::wstring::npos)
            matches.push_back({&kv.second, kv.first});
    }
    if (!matches.empty()) {
        auto& m = matches[rand() % matches.size()];
        s_lastMatchKey = m.second;
        return m.first;
    }
    return nullptr;
}

std::wstring ChatLastMatchedKey() { return s_lastMatchKey; }

const std::wstring* ChatLookupButton(const std::wstring& key)
{
    ChatEnsureDialogLoaded();
    auto it = s_buttonMap.find(key);
    return (it != s_buttonMap.end()) ? &it->second : nullptr;
}

// 鎸夐挳杈撳叆澶勭悊閫昏緫

std::wstring ChatGetButtonLabel(const std::wstring& key, const std::wstring& fallback)
{
    ChatEnsureDialogLoaded();
    auto it = s_buttonLabelMap.find(key);
    return (it != s_buttonLabelMap.end()) ? it->second : fallback;
}


// 鏄剧ず鏂囧瓧杈撳叆绐楀彛
void ChatShowInput(HWND hwndParent)
{
    InputChatPanel::Show(hwndParent);
}

void ChatShowButtonInput(HWND hwndParent, const std::wstring& key1, const std::wstring& key2)
{
    OptionChatPanel::Show(hwndParent, key1, key2);
}

void ChatRecordInteraction()
{
    s_state.lastInteraction = GetUnixTimeSeconds();
}

ChatSystem& ChatSystem::Get()
{
    static ChatSystem instance;
    return instance;
}

void ChatSystem::Init(HWND hwnd)
{
    ChatInit(hwnd);
}

void ChatInit(HWND hwnd)
{
    s_mainHwnd = hwnd;
}


