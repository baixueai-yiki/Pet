#include "render.h"
#include "common.h"

void Render(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    HBRUSH brush = CreateSolidBrush(RGB(255, 200, 200));

    RECT rect;
    rect.left = g_charX;
    rect.top = g_charY;
    rect.right = g_charX + g_charW;
    rect.bottom = g_charY + g_charH;

    FillRect(hdc, &rect, brush);

    DeleteObject(brush);
    EndPaint(hwnd, &ps);
}