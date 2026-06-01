#include "Engine/Window/WindowLifecycle.h"

#include "Engine/Input/InputDispatcher.h"
#include "Engine/Render/Renderer.h"
#include "Systems/Pet/PetActor.h"

#include <windowsx.h>

namespace pet::engine::window {

bool WindowLifecycle::RegisterWindowClass(const std::wstring& className, WNDPROC wndProc, HINSTANCE instance) {
    WNDCLASSW wc = {};
    wc.lpfnWndProc = wndProc;
    wc.hInstance = instance;
    wc.lpszClassName = className.c_str();
    wc.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));

    const ATOM atom = RegisterClassW(&wc);
    return atom != 0 || GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
}

HWND WindowLifecycle::Create(const WindowDesc& desc, WNDPROC wndProc, HINSTANCE instance) {
    if (!RegisterWindowClass(desc.className, wndProc, instance)) {
        return nullptr;
    }

    const int width = desc.width > 0 ? desc.width : GetSystemMetrics(SM_CXSCREEN);
    const int height = desc.height > 0 ? desc.height : GetSystemMetrics(SM_CYSCREEN);
    const int x = desc.x;
    const int y = desc.y;

    HWND hwnd = CreateWindowExW(
        desc.style.exStyle,
        desc.className.c_str(),
        desc.title.c_str(),
        desc.style.style,
        x,
        y,
        width,
        height,
        nullptr,
        nullptr,
        instance,
        nullptr
    );

    if (hwnd) {
        WindowCore::ApplyStyle(hwnd, desc.style);
    }
    return hwnd;
}

void WindowLifecycle::Show(HWND hwnd) {
    if (!hwnd) return;
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
}

void WindowLifecycle::Destroy(HWND hwnd) {
    if (!hwnd) return;
    DestroyWindow(hwnd);
}

LRESULT WindowLifecycle::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    using namespace pet;

    if (msg == WM_NCHITTEST) {
        const POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        RECT rc = {};
        GetWindowRect(hwnd, &rc);

        const auto& state = systems::pet::PetActor::Get().GetRenderState();
        const RECT petRc = {
            state.x,
            state.y,
            state.x + state.width,
            state.y + state.height
        };

        if (PtInRect(&rc, pt) && !PtInRect(&petRc, pt)) {
            return HTTRANSPARENT;
        }
        return HTCLIENT;
    }

    if (engine::input::InputDispatcher::Dispatch(msg, wParam, lParam)) {
        return 0;
    }

    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps = {};
        BeginPaint(hwnd, &ps);

        RECT rc = {};
        GetClientRect(hwnd, &rc);
        const int w = rc.right - rc.left;
        const int h = rc.bottom - rc.top;

        HDC memDC = CreateCompatibleDC(ps.hdc);
        HBITMAP memBmp = CreateCompatibleBitmap(ps.hdc, w, h);
        HBITMAP oldBmp = static_cast<HBITMAP>(SelectObject(memDC, memBmp));

        HBRUSH black = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
        FillRect(memDC, &rc, black);
        engine::render::Renderer::Render(memDC, systems::pet::PetActor::Get().GetRenderState());

        BitBlt(ps.hdc, 0, 0, w, h, memDC, 0, 0, SRCCOPY);

        SelectObject(memDC, oldBmp);
        DeleteObject(memBmp);
        DeleteDC(memDC);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

} // namespace pet::engine::window
