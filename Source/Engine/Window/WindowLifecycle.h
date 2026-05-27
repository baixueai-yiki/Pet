#pragma once

#include "Engine/Window/WindowCore.h"

#include <string>

namespace pet::engine::window {

struct WindowDesc {
    std::wstring className = L"PetMainWindow";
    std::wstring title = L"Pet";
    int x = CW_USEDEFAULT;
    int y = CW_USEDEFAULT;
    int width = 800;
    int height = 600;
    WindowStyle style;
};

class WindowLifecycle {
public:
    static bool RegisterClass(const std::wstring& className, WNDPROC wndProc, HINSTANCE instance = GetModuleHandleW(nullptr));
    static HWND Create(const WindowDesc& desc, WNDPROC wndProc, HINSTANCE instance = GetModuleHandleW(nullptr));
    static void Show(HWND hwnd);
    static void Destroy(HWND hwnd);
};

} // namespace pet::engine::window
