#include "InputDispatcher.h"
#include "../../Core/PetState.h"
#include <windows.h>
#include <windowsx.h>
#include <cwctype>
#include <string>

// 回调列表（Systems 层注册）
static InputCallback s_onPoke = nullptr;
static InputCallback s_onDragUpdate = nullptr;
static InputCallback s_onRightClick = nullptr;
static InputCallback s_onDoubleClick = nullptr;
static InputCallback s_onZhiZhi = nullptr;

void RegisterOnPoke(InputCallback cb)           { s_onPoke = std::move(cb); }
void RegisterOnDragUpdate(InputCallback cb)     { s_onDragUpdate = std::move(cb); }
void RegisterOnRightClick(InputCallback cb)     { s_onRightClick = std::move(cb); }
void RegisterOnDoubleClick(InputCallback cb)    { s_onDoubleClick = std::move(cb); }
void RegisterOnZhiZhi(InputCallback cb)         { s_onZhiZhi = std::move(cb); }

// 判断给定点是否落在当前宠物绘制区域
static bool IsInsidePet(int x, int y)
{
    return x >= g_pet.x &&
           x <= g_pet.x + g_pet.w &&
           y >= g_pet.y &&
           y <= g_pet.y + g_pet.h;
}

static bool s_leftDown = false;
static bool s_dragMoved = false;
static bool s_dragInteractionLogged = false;
static int s_downX = 0;
static int s_downY = 0;
static unsigned long long s_pokeCount = 0;
static HANDLE s_pendingPokeTimer = nullptr;

unsigned long long GetPokeCount()
{
    return s_pokeCount;
}

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
                s_dragInteractionLogged = true;
                g_pet.dragOffsetX = s_downX - g_pet.x;
                g_pet.dragOffsetY = s_downY - g_pet.y;
            }
        }

        if (g_pet.isDragging)
        {
            g_pet.x = x - g_pet.dragOffsetX;
            g_pet.y = y - g_pet.dragOffsetY;
            InvalidateRect(hwnd, nullptr, TRUE);
            if (s_onDragUpdate) s_onDragUpdate();
        }

        break;
    }

    case WM_LBUTTONDOWN:
    {
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
        if (s_leftDown && !s_dragMoved && IsInsidePet(x, y))
        {
            ++s_pokeCount;
            // 延迟触发戳戳，留给双击窗口判断
            if (s_pendingPokeTimer)
                DeleteTimerQueueTimer(nullptr, s_pendingPokeTimer, nullptr);
            CreateTimerQueueTimer(&s_pendingPokeTimer, nullptr,
                [](PVOID, BOOLEAN) {
                    s_pendingPokeTimer = nullptr;
                    if (s_onPoke) s_onPoke();
                },
                nullptr, 250, 0, WT_EXECUTEDEFAULT);
        }
        s_leftDown = false;
        s_dragMoved = false;
        s_dragInteractionLogged = false;
        g_pet.isDragging = false;
        InvalidateRect(hwnd, nullptr, TRUE);
        break;

    case WM_LBUTTONDBLCLK:
    {
        // 取消戳戳，换 zhizhi
        if (s_pendingPokeTimer)
        {
            DeleteTimerQueueTimer(nullptr, s_pendingPokeTimer, nullptr);
            s_pendingPokeTimer = nullptr;
        }
        if (IsInsidePet(x, y))
        {
            if (s_onZhiZhi) s_onZhiZhi();
            if (s_onDoubleClick) s_onDoubleClick();
        }
        break;
    }

    case WM_RBUTTONDOWN:
    {
        if (!IsInsidePet(x, y))
            break;
        if (s_onRightClick) s_onRightClick();
        break;
    }

    case WM_MOUSEWHEEL:
        // Wheel events handled by WindowLifecycle overlay check
        break;

    default:
        break;
    }
}
