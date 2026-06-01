#pragma once

#include "Engine/Window/WindowCore.h"

#include <string>

namespace pet::engine::window {

struct WindowDesc {
    std::wstring className = L"PetMainWindow";
    std::wstring title = L"Pet";
    int x = 0;
    int y = 0;
    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);
    WindowStyle style;
};

class WindowLifecycle {
public:
    static bool RegisterWindowClass(const std::wstring& className, WNDPROC wndProc, HINSTANCE instance = GetModuleHandleW(nullptr));
    static HWND Create(const WindowDesc& desc, WNDPROC wndProc, HINSTANCE instance = GetModuleHandleW(nullptr));
    static void Show(HWND hwnd);
    static void Destroy(HWND hwnd);
    static LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

} // namespace pet::engine::window
