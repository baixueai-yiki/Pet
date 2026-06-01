#pragma once

#include <windows.h>

namespace pet::engine::window {

struct WindowStyle {
    DWORD style = WS_POPUP;
    DWORD exStyle = WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW;
    bool topMost = true;
    bool clickThrough = false;
    BYTE alpha = 255;
    bool useColorKey = true;
    COLORREF colorKey = RGB(0, 0, 0);
};

class WindowCore {
public:
    static void ApplyStyle(HWND hwnd, const WindowStyle& style);
    static void SetTopMost(HWND hwnd, bool topMost);
    static void SetClickThrough(HWND hwnd, bool clickThrough);
    static void SetAlpha(HWND hwnd, BYTE alpha);
    static void SetColorKey(HWND hwnd, COLORREF colorKey);
};

} // namespace pet::engine::window
