#pragma once

#include <windows.h>

namespace pet::engine::window {

struct WindowStyle {
    DWORD style = WS_POPUP;
    DWORD exStyle = WS_EX_LAYERED | WS_EX_TOPMOST;
    bool topMost = true;
    bool clickThrough = false;
    BYTE alpha = 255;
};

class WindowCore {
public:
    static void ApplyStyle(HWND hwnd, const WindowStyle& style);
    static void SetTopMost(HWND hwnd, bool topMost);
    static void SetClickThrough(HWND hwnd, bool clickThrough);
    static void SetAlpha(HWND hwnd, BYTE alpha);
};

} // namespace pet::engine::window
