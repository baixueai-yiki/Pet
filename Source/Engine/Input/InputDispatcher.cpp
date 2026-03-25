#include "InputDispatcher.h"
#include "../../Game/Chat/Chat.h"
#include "../../Game/Pet/Pet.h"
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

// 记录最近六次右键点击时间，用于识别“慢三连+快三连”组合
static DWORD s_rightClickTimes[6] = {};

// 统一处理主窗口所有鼠标消息，拖拽/穿透/聊天触发都在这里处理
void HandleInput(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    int x = GET_X_LPARAM(lParam);
    int y = GET_Y_LPARAM(lParam);

    switch (msg)
    {
    case WM_MOUSEMOVE:
    {
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
            g_pet.isDragging = true;
            g_pet.dragOffsetX = x - g_pet.x;
            g_pet.dragOffsetY = y - g_pet.y;
        }
        break;
    }

    case WM_LBUTTONUP:
        // 松开左键结束拖拽
        g_pet.isDragging = false;
        InvalidateRect(hwnd, nullptr, TRUE);
        break;

    case WM_RBUTTONDOWN:
    {
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
