#include "InputDispatcher.h"
#include "../../Game/Chat/Chat.h"
#include "../../Game/Pet/Pet.h"
#include "../../Core/Path.h"
#include "../../Game/Audio/Audio.h"
#include <cwctype>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <windows.h>
#include <windowsx.h>  // contains GET_X_LPARAM, etc.

extern PetState g_pet;


// 判断给定点是否落在当前宠物绘制区域，用于拖拽和点击判断
static bool IsInsidePet(int x, int y)
{
    return x >= g_pet.x &&
           x <= g_pet.x + g_pet.w &&
           y >= g_pet.y &&
           y <= g_pet.y + g_pet.h;
}

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

    std::wstring wdata(static_cast<size_t>(wlen), L"\0"[0]);
    MultiByteToWideChar(codePage, 0, data.data(), static_cast<int>(data.size()), &wdata[0], wlen);

    std::wistringstream iss(wdata);
    std::wstring line;
    while (std::getline(iss, line))
    {
        if (!line.empty() && line.back() == L"\r"[0])
            line.pop_back();
        lines.push_back(line);
    }
    return true;
}


// 记录最近六次右键点击时间，用于识别“慢三连+快三连”组合
static DWORD s_rightClickTimes[6] = {};
static bool s_audioSeeded = false;
static bool s_leftDown = false;
static bool s_dragMoved = false;
static bool s_dragInteractionLogged = false;
static int s_downX = 0;
static int s_downY = 0;
static unsigned long long s_pokeCount = 0;

unsigned long long GetPokeCount()
{
    return s_pokeCount;
}

static void PlayRandomPokeSound()
{
    if (!s_audioSeeded)
    {
        s_audioSeeded = true;
        srand(static_cast<unsigned int>(GetTickCount()));
    }

    static const wchar_t* kSounds[] = {
        L"audio\\poke_nya.wav",
        L"audio\\poke_find.wav",
        L"audio\\poke_ah.wav",
        L"audio\\poke_poke.wav"
    };
    const int idx = rand() % (sizeof(kSounds) / sizeof(kSounds[0]));
    PlayAudioAsset(kSounds[idx]);
}

static std::wstring Trim(const std::wstring& s)
{
    size_t b = 0;
    while (b < s.size() && iswspace(s[b]))
        ++b;
    size_t e = s.size();
    while (e > b && iswspace(s[e - 1]))
        --e;
    return s.substr(b, e - b);
}

static std::wstring ToLower(std::wstring s)
{
    for (auto& ch : s)
        ch = static_cast<wchar_t>(towlower(ch));
    return s;
}

static std::wstring ReadStringSetting(const std::wstring& key, const std::wstring& defaultValue)
{
    std::vector<std::wstring> lines;
    if (!ReadFileLines(GetConfigPath(L"settings.txt"), lines))
        return defaultValue;

    for (const auto& lineRaw : lines)
    {
        std::wstring line = lineRaw;
        if (line.empty() || line[0] == L'#')
            continue;
        size_t eq = line.find(L'=');
        if (eq == std::wstring::npos)
            continue;
        std::wstring k = Trim(line.substr(0, eq));
        std::wstring v = Trim(line.substr(eq + 1));
        std::wstring kLower = ToLower(k);
        std::wstring keyLower = ToLower(key);
        if (kLower == keyLower ||
            (keyLower == L"chat_trigger" && (k == L"唤出方式" || k == L"对话触发")))
            return v;
    }
    return defaultValue;
}

static std::wstring NormalizeTrigger(std::wstring v)
{
    v = ToLower(Trim(v));
    if (v == L"right_click_once" || v == L"rightclickonce" || v == L"one" || v == L"1" || v == L"右键一次" || v == L"右键1次" || v == L"右键" || v == L"求救")
        return L"right_click_once";
    if (v == L"right_click_combo" || v == L"rightclickcombo" || v == L"combo" || v == L"6" || v == L"右键六次" || v == L"右键6次")
        return L"right_click_combo";
    return L"right_click_combo";
}
// 统一处理主窗口所有鼠标消息，拖拽/穿透/聊天触发都在这里处理
void HandleInput(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    int x = GET_X_LPARAM(lParam);
    int y = GET_Y_LPARAM(lParam);

    switch (msg)
    {
    case WM_MOUSEMOVE:
    {
        if (s_leftDown && !g_pet.isDragging)
        {
            const int dx = x - s_downX;
            const int dy = y - s_downY;
            if ((dx * dx + dy * dy) > 9)
            {
                g_pet.isDragging = true;
                s_dragMoved = true;
                if (!s_dragInteractionLogged)
                {
                    s_dragInteractionLogged = true;
                    ChatRecordInteraction();
                }
                g_pet.dragOffsetX = s_downX - g_pet.x;
                g_pet.dragOffsetY = s_downY - g_pet.y;
            }
        }

        // 拖拽时更新宠物位置并触发重绘
        if (g_pet.isDragging)
        {
            g_pet.x = x - g_pet.dragOffsetX;
            g_pet.y = y - g_pet.dragOffsetY;
            ChatUpdateInputPosition();
            ChatUpdateTalkPosition();
            ChatUpdateButtonPosition();
            ChatUpdateTaskListPosition();
            InvalidateRect(hwnd, nullptr, TRUE);
        }

        break;
    }

    case WM_LBUTTONDOWN:
    {
        // 只有点击在宠物区域内才开始拖拽
        if (IsInsidePet(x, y))
        {
            s_leftDown = true;
            s_dragMoved = false;
            s_dragInteractionLogged = false;
            s_downX = x;
            s_downY = y;
        }
        break;
    }

    case WM_LBUTTONUP:
        // 松开左键结束拖拽
        if (s_leftDown && !s_dragMoved && IsInsidePet(x, y))
        {
            ++s_pokeCount;
            PlayRandomPokeSound();
            ChatRecordInteraction();
        }
        s_leftDown = false;
        s_dragMoved = false;
        s_dragInteractionLogged = false;
        g_pet.isDragging = false;
        InvalidateRect(hwnd, nullptr, TRUE);
        break;

    case WM_RBUTTONDOWN:
    {
        // 仅在宠物区域内触发聊天
        if (!IsInsidePet(x, y))
            break;

        std::wstring trigger = NormalizeTrigger(ReadStringSetting(L"唤出方式", L"right_click_combo"));
        if (trigger == L"right_click_once")
        {
            ChatShowInput(hwnd);
            break;
        }

        // 右键时间序列滑动，用来检测慢三连 + 快三连
        for (int i = 0; i < 5; ++i)
            s_rightClickTimes[i] = s_rightClickTimes[i + 1];
        s_rightClickTimes[5] = GetTickCount();

        const DWORD slowMax = 1000;
        const DWORD fastMax = 300;

        bool slowTriple = (s_rightClickTimes[3] - s_rightClickTimes[0] <= slowMax);
        bool fastTriple = (s_rightClickTimes[5] - s_rightClickTimes[3] <= fastMax);

        if (slowTriple && fastTriple)
        {
            for (int i = 0; i < 6; ++i)
                s_rightClickTimes[i] = 0;
            ChatShowInput(hwnd);
        }
        break;
    }

    default:
        break;
    }
}
