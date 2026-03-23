#include "InputDispatcher.h"
#include "../../Game/Chat/Chat.h"
#include "../../Game/Pet/Pet.h"
#include <windows.h>

extern PetState g_pet;

static bool IsInsidePet(int x, int y)
{
    return x >= g_pet.x &&
           x <= g_pet.x + g_pet.w &&
           y >= g_pet.y &&
           y <= g_pet.y + g_pet.h;
}

static DWORD s_rightClickTimes[6] = {};

void HandleInput(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    int x = GET_X_LPARAM(lParam);
    int y = GET_Y_LPARAM(lParam);

    switch (msg)
    {
    case WM_MOUSEMOVE:
    {
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
        if (IsInsidePet(x, y))
        {
            g_pet.isDragging = true;
            g_pet.dragOffsetX = x - g_pet.x;
            g_pet.dragOffsetY = y - g_pet.y;
        }
        break;
    }

    case WM_LBUTTONUP:
        g_pet.isDragging = false;
        InvalidateRect(hwnd, nullptr, TRUE);
        break;

    case WM_RBUTTONDOWN:
    {
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
