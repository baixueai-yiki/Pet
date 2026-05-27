#include "Engine/Window/WindowLifecycle.h"

namespace pet::engine::window {

bool WindowLifecycle::RegisterClass(const std::wstring& className, WNDPROC wndProc, HINSTANCE instance) {
    WNDCLASSW wc = {};
    wc.lpfnWndProc = wndProc;
    wc.hInstance = instance;
    wc.lpszClassName = className.c_str();
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);

    const ATOM atom = RegisterClassW(&wc);
    return atom != 0 || GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
}

HWND WindowLifecycle::Create(const WindowDesc& desc, WNDPROC wndProc, HINSTANCE instance) {
    if (!RegisterClass(desc.className, wndProc, instance)) {
        return nullptr;
    }

    HWND hwnd = CreateWindowExW(
        desc.style.exStyle,
        desc.className.c_str(),
        desc.title.c_str(),
        desc.style.style,
        desc.x,
        desc.y,
        desc.width,
        desc.height,
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

} // namespace pet::engine::window
