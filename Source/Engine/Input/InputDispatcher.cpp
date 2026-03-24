#include "InputDispatcher.h"
#include "../../Game/Chat/Chat.h"
#include "../../Game/Pet/Pet.h"
#include <windows.h>
#include <windowsx.h>  // 这里包含 GET_X_LPARAM 等宏

extern PetState g_pet;

// 判断指定点是否落在宠物贴图区域内
static bool IsInsidePet(int x, int y)
{
    return x >= g_pet.x &&
           x <= g_pet.x + g_pet.w &&
           y >= g_pet.y &&
           y <= g_pet.y + g_pet.h;
}

// 记录最近几次鼠标右键按下时间，用于识别特殊三连击组合
static DWORD s_rightClickTimes[6] = {};

void HandleInput(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    int x = GET_X_LPARAM(lParam);
    int y = GET_Y_LPARAM(lParam);

    switch (msg)
    {
    case WM_MOUSEMOVE:
    {
        // 根据鼠标是否在宠物上调整窗口是否透传，借此实现透明窗口下的穿透
        if (IsInsidePet(x, y))
        {
            SetWindowLong(hwnd, GWL_EXSTYLE,
                GetWindowLong(hwnd, GWL_EXSTYLE) & ~WS_EX_TRANSPARENT);
        }
        else
        {
            SetWindowLong(hwnd, GWL_EXSTYLE,
                GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_TRANSPARENT);
        }

        // 如果处于拖拽状态，则更新宠物位置并请求重绘
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
        // 鼠标左键落在宠物区域内时开始拖拽
        if (IsInsidePet(x, y))
        {
            g_pet.isDragging = true;
            g_pet.dragOffsetX = x - g_pet.x;
            g_pet.dragOffsetY = y - g_pet.y;
        }
        break;
    }

    case WM_LBUTTONUP:
        // 释放左键即停止拖拽并刷新界面
        g_pet.isDragging = false;
        InvalidateRect(hwnd, nullptr, TRUE);
        break;

    case WM_RBUTTONDOWN:
    {
        // 滚动右键时间，把最新的时间推入队列，保持窗口大小为6
        for (int i = 0; i < 5; ++i)
            s_rightClickTimes[i] = s_rightClickTimes[i + 1];
        s_rightClickTimes[5] = GetTickCount();

        const DWORD slowMax = 1000;
        const DWORD fastMax = 300;

        bool slowTriple = (s_rightClickTimes[3] - s_rightClickTimes[0] <= slowMax);
        bool fastTriple = (s_rightClickTimes[5] - s_rightClickTimes[3] <= fastMax);

        // 只有满足“慢三连”+“快三连”，才弹出聊天输入框
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
