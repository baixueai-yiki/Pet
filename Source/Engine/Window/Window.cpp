#include <windows.h>
#include "../Input/InputDispatcher.h"
#include "../Render/Renderer.h"
#include "Window.h"

// 主窗口的消息回调，把输入事件交由输入系统处理，绘制事件交由渲染系统处理
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    // 鼠标相关消息全部转给输入调度器，便于统一处理拖拽等逻辑
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_LBUTTONDBLCLK:
        HandleInput(hwnd, msg, wParam, lParam);
        break;

    case WM_PAINT:
    {
        // 交由 GDI+ 渲染当前宠物画面
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RendererRender(hdc);
        EndPaint(hwnd, &ps);
        break;
    }

    case WM_ERASEBKGND:
        // 避免抖动，返回非零让系统跳过默认擦除
        return 1;

    case WM_DESTROY:
        // 主窗口销毁则退出消息循环
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 0;
}

// 创建全屏透明窗口，作为宠物的顶层显示与交互区域
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
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT,
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
        // 设置半透明标志，使窗口本体透明但渲染内容可见
        SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);

    return hwnd;
}
