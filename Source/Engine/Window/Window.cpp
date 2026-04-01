#include <windows.h>
#include <windowsx.h>
#include <fstream>
#include <sstream>
#include <string>
#include <cwctype>
#include <vector>
#include "../Input/InputDispatcher.h"
#include "../Render/Render.h"
#include "Window.h"
#include "../../Core/Path.h"
#include "../../Core/Diary.h"
#include "../../Core/TextFile.h"
#include "../../Systems/Setting/Setting.h"
#include "../../Runtime/Scheduler.h"
#include "../../Systems/Pet/Pet.h"
#include "../../Systems/Chat/Chat.h"
#include "../../Systems/Audio/Audio.h"

static const UINT_PTR kIdleCheckTimer = 2;
static const UINT kIdleCheckMs = 60000;

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
static UINT GetRefreshIntervalMs()
{
    int fps = Setting::GetInt(L"fps", 60);
    if (fps != 30 && fps != 60 && fps != 120)
        fps = 60;
    return static_cast<UINT>(1000 / fps);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static bool s_exitHandled = false;
    switch (msg)
    {
    case WM_CREATE:
        // 使用定时器让拖动时的重绘更顺滑
        SetTimer(hwnd, 1, GetRefreshIntervalMs(), nullptr);
        SetTimer(hwnd, kIdleCheckTimer, kIdleCheckMs, nullptr);
        ChatInit(hwnd);
        AudioInit();
        DiaryInit();
        SchedulerClear();
        ScheduleEveryMs(L"tick.minute", kIdleCheckMs);
        OnProgramStart();
        break;


    case WM_NCHITTEST:
    {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        // 如果设置面板在显示，允许对话区域接收事件
        if (Setting::IsPointInsideOverlay(x, y))
            return HTCLIENT;
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
    case WM_RBUTTONDOWN:
        if (Setting::IsOverlayVisible() && Setting::HandleOverlayMouse(hwnd, msg, wParam, lParam))
            break;
        HandleInput(hwnd, msg, wParam, lParam);
        break;
    case WM_MOUSEWHEEL:
        if (Setting::IsOverlayVisible() && Setting::HandleOverlayMouse(hwnd, msg, wParam, lParam))
            break;
        break;
    case WM_TIMER:
        // 拖动时按稳定频率重绘
        if (wParam == kIdleCheckTimer)
        {
            SchedulerTick();
        }
        else
        {
            if (g_pet.isDragging)
                InvalidateRect(hwnd, nullptr, FALSE);
        }
        break;


    case WM_PAINT:
    {
        // 进入/退出绘制流程，确保绘制消息被正确消费
        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);

        // 使用双缓冲减少拖动时闪烁
        RECT rc;
        GetClientRect(hwnd, &rc);
        int w = rc.right - rc.left;
        int h = rc.bottom - rc.top;

        HDC memDC = CreateCompatibleDC(ps.hdc);
        HBITMAP memBmp = CreateCompatibleBitmap(ps.hdc, w, h);
        HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);

        // 先清成黑色（与颜色键一致），再绘制宠物
        HBRUSH black = (HBRUSH)GetStockObject(BLACK_BRUSH);
        FillRect(memDC, &rc, black);
        RendererRender(memDC);

        BitBlt(ps.hdc, 0, 0, w, h, memDC, 0, 0, SRCCOPY);

        SelectObject(memDC, oldBmp);
        DeleteObject(memBmp);
        DeleteDC(memDC);
        EndPaint(hwnd, &ps);
        break;
    }

    case WM_ERASEBKGND:
        return 1;

    case WM_DESTROY:
        if (!s_exitHandled)
        {
            s_exitHandled = true;
            OnProgramExit();
        }
        PostQuitMessage(0);
        return 0;

    case WM_QUERYENDSESSION:
        if (!s_exitHandled)
        {
            s_exitHandled = true;
            OnProgramExit();
        }
        return TRUE;

    case WM_ENDSESSION:
        if (wParam && !s_exitHandled)
        {
            s_exitHandled = true;
            OnProgramExit();
        }
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
