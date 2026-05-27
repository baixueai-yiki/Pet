#include "InputDispatcher.h"
#include "../../Systems/Chat/Chat.h"
#include "../../Systems/Pet/Pet.h"
#include "../../Core/Path.h"
#include "../../Core/TextFile.h"
#include "../../Systems/Setting/Setting.h"
#include "../../Systems/UI/UIManager.h"
#include "../../Runtime/EventBus.h"
#include <cwctype>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <windows.h>
#include <windowsx.h>  // 包含坐标解析等宏

extern PetState g_pet;


// 判断给定点是否落在当前宠物绘制区域，用于拖拽和点击判断
static bool IsInsidePet(int x, int y)
{
    return x >= g_pet.x &&
           x <= g_pet.x + g_pet.w &&
           y >= g_pet.y &&
           y <= g_pet.y + g_pet.h;
}


// 记录最近六次右键点击时间，用于识别“慢三连+快三连”组合
static DWORD s_rightClickTimes[6] = {};
static int s_rightClickCount = 0;
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
    EventEmit(L"audio.play", kSounds[idx]);
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
        UI::DispatchMouseClick(x, y);
        break;

    case WM_RBUTTONDOWN:
    {
        if (!IsInsidePet(x, y))
            break;
        UI::DispatchMouseClick(x, y);
        break;
    }

    case WM_MOUSEWHEEL:
        UI::DispatchMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam));
        break;

    default:
        break;
    }
}
