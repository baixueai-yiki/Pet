#include <windows.h>
#include <windowsx.h>
#include "../Input/InputDispatcher.h"
#include "../Render/Render.h"
#include "Window.h"
#include "../../Game/Pet/Pet.h"
#include "../../Game/Chat/Chat.h"

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_NCHITTEST:
    {
        // 只有宠物区域可点击，其余区域点击穿透
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        if (x >= g_pet.x && x <= g_pet.x + g_pet.w &&
            y >= g_pet.y && y <= g_pet.y + g_pet.h)
        {
            return HTCLIENT;
        }
        return HTTRANSPARENT;
    }

    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_LBUTTONDBLCLK:
        HandleInput(hwnd, msg, wParam, lParam);
        break;

    case WM_PAINT:
    {
        // 进入/退出绘制流程，确保 WM_PAINT 被正确消费
        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);
        RendererRender(ps.hdc);
        EndPaint(hwnd, &ps);
        break;
    }

    case WM_ERASEBKGND:
        return 1;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_USER + 1:
        ChatShowInput(hwnd);
        return 0;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 0;
}

HWND CreateMainWindow(HINSTANCE hInstance)
{
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"PetWindow";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(NULL_BRUSH));

    ATOM atom = RegisterClassW(&wc);
    if (atom == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
        return nullptr;

    HWND hwnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        wc.lpszClassName,
        L"Pet",
        WS_POPUP,
        0, 0,
        GetSystemMetrics(SM_CXSCREEN),
        GetSystemMetrics(SM_CYSCREEN),
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (hwnd)
        SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);

    return hwnd;
}
