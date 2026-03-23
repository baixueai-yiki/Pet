#include <windows.h>
#include "common.h"
#include "render.h"
#include "input.h"
#include "chat.h"

#pragma comment(lib, "user32.lib")

int g_charX = 200;
int g_charY = 200;
int g_charW = 200;
int g_charH = 200;

bool g_isDragging = false;
int g_offsetX = 0;
int g_offsetY = 0;

HWND g_hwnd = nullptr;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_NCHITTEST:
    {
        POINT pt = { LOWORD(lParam), HIWORD(lParam) };
        ScreenToClient(hwnd, &pt);

        if (IsInsideCharacter(pt.x, pt.y))
            return HTCLIENT;
        else
            return HTTRANSPARENT;
    }

    case WM_LBUTTONDOWN:
        HandleDragMouseDown(hwnd, lParam);
        break;

    case WM_MOUSEMOVE:
        HandleDragMouseMove(hwnd, lParam);
        break;

    case WM_LBUTTONUP:
        HandleDragMouseUp();
        break;

    case WM_PAINT:
        Render(hwnd);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 0;
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int)
{
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"DesktopPet";

    RegisterClass(&wc);

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    HWND hwnd = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST,
        wc.lpszClassName,
        L"Pet",
        WS_POPUP,
        0, 0, screenW, screenH,
        NULL, NULL, hInst, NULL
    );

    g_hwnd = hwnd;

    SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);

    ShowWindow(hwnd, SW_SHOW);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}