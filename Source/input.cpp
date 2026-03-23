#include "input.h"
#include "common.h"

bool IsInsideCharacter(int x, int y)
{
    return x >= g_charX &&
           x <= g_charX + g_charW &&
           y >= g_charY &&
           y <= g_charY + g_charH;
}

void HandleDragMouseDown(HWND hwnd, LPARAM lParam)
{
    int x = LOWORD(lParam);
    int y = HIWORD(lParam);

    if (IsInsideCharacter(x, y))
    {
        g_isDragging = true;
        g_offsetX = x - g_charX;
        g_offsetY = y - g_charY;
        SetCapture(hwnd);
    }
}

void HandleDragMouseMove(HWND hwnd, LPARAM lParam)
{
    if (!g_isDragging) return;

    int x = LOWORD(lParam);
    int y = HIWORD(lParam);

    g_charX = x - g_offsetX;
    g_charY = y - g_offsetY;

    InvalidateRect(hwnd, NULL, FALSE);
}

void HandleDragMouseUp()
{
    g_isDragging = false;
    ReleaseCapture();
}